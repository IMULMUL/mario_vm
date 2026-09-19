#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "native_Web.h"
#include "RegExp/native_RegExp.h"

/* ==========================================================================
 * PRNG - xorshift128+, lazily seeded from clock + address entropy. The engine
 * has no OS CSPRNG binding here; this mirrors js_web.c's web_rand_next() and is
 * adequate for crypto.getRandomValues()/randomUUID() shapes.
 * ========================================================================== */
static uint64_t g_rs[2] = { 0, 0 };

static uint64_t web_rand_next(void) {
	if(g_rs[0] == 0 && g_rs[1] == 0) {
		struct timespec ts;
		uint64_t a = 0, b = 0;
#if defined(CLOCK_MONOTONIC)
		clock_gettime(CLOCK_MONOTONIC, &ts);
		a = (uint64_t)ts.tv_nsec ^ ((uint64_t)ts.tv_sec << 32);
#endif
		clock_gettime(CLOCK_REALTIME, &ts);
		b = (uint64_t)ts.tv_nsec ^ ((uint64_t)ts.tv_sec << 20);
		g_rs[0] = a ^ (uint64_t)(uintptr_t)&g_rs ^ 0x9E3779B97F4A7C15ULL;
		g_rs[1] = b ^ (uint64_t)(uintptr_t)&ts  ^ 0xBF58476D1CE4E5B9ULL;
		if(g_rs[0] == 0) g_rs[0] = 0x243F6A8885A308D3ULL;
		if(g_rs[1] == 0) g_rs[1] = 0x13198A2E03707344ULL;
	}
	uint64_t s1 = g_rs[0];
	uint64_t s0 = g_rs[1];
	uint64_t result = s0 + s1;
	g_rs[0] = s0;
	s1 ^= s1 << 23;
	g_rs[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
	return result;
}

/* Milliseconds since the Unix epoch (wall clock), as a double. */
static double wall_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

/* ==========================================================================
 * structuredClone
 * ========================================================================== */

/* Cycle / shared-reference table: parallel arrays mapping an already-cloned
 * source var to its clone, so a repeated or self-referential object yields the
 * same clone (identity preserved) instead of recursing forever. */
typedef struct {
	vm_t*   vm;
	var_t** srcs;
	var_t** dsts;
	uint32_t n;
	uint32_t cap;
	bool     error;   /* DataCloneError raised */
} clone_ctx;

static void ctx_remember(clone_ctx* c, var_t* src, var_t* dst) {
	if(c->n >= c->cap) {
		uint32_t nc = c->cap ? c->cap * 2 : 16;
		uint32_t os = (uint32_t)(sizeof(var_t*) * c->cap);
		uint32_t nsz = (uint32_t)(sizeof(var_t*) * nc);
		var_t** ns = (var_t**)mario_realloc(c->srcs, os, nsz);
		var_t** nd = (var_t**)mario_realloc(c->dsts, os, nsz);
		if(ns == NULL || nd == NULL) return;   /* best effort: skip tracking */
		c->srcs = ns; c->dsts = nd; c->cap = nc;
	}
	c->srcs[c->n] = src;
	c->dsts[c->n] = dst;
	c->n++;
}

static var_t* ctx_lookup(clone_ctx* c, var_t* src) {
	for(uint32_t i = 0; i < c->n; ++i)
		if(c->srcs[i] == src) return c->dsts[i];
	return NULL;
}

static void ctx_free(clone_ctx* c) {
	if(c->srcs) mario_free(c->srcs);
	if(c->dsts) mario_free(c->dsts);
	c->srcs = NULL; c->dsts = NULL; c->n = 0; c->cap = 0;
}

/* Raise a DataCloneError (a DOMException this engine does not model, so an
 * Error-prototype object carrying the name). The native records it in
 * native_thrown; func_call delivers it as a catchable throw. */
static void clone_fail(vm_t* vm, clone_ctx* c, const char* what) {
	c->error = true;
	var_t* Err = var_find_member_var(vm->root, "Error");
	var_t* proto = (Err != NULL) ? var_get_prototype(Err) : NULL;
	var_t* err = var_new_obj(vm, proto, NULL, NULL);   /* refs==0 (unowned) */
	var_add(err, "name", var_new_str(vm, "DataCloneError"));
	var_add(err, "message", var_new_str(vm, what));
	if(vm->native_thrown != NULL) var_unref(vm->native_thrown);
	/* native_thrown must hold the single owning reference: var_ref lifts err from
	 * the refs==0 baseline to 1. Do NOT var_unref(err) afterwards - that would drop
	 * it back to 0 and free it, so func_call would deliver a freed var (undefined)
	 * instead of the DataCloneError. func_call's push(+1)/unref(-1) leaves it at 1
	 * on the stack, where the catch clause reads it. */
	vm->native_thrown = var_ref(err);
}

static var_t* clone_var(clone_ctx* c, var_t* s);

/* Build a fresh instance of a builtin class by running its constructor with
 * `self` as `this`. Used for RegExp/Map/Set, whose payloads live in a C struct
 * (or compiled program) only the constructor sets up. Returns refs==0. */
static var_t* construct_builtin(vm_t* vm, const char* cls_name, var_t* arg) {
	var_t* cls = var_find_member_var(vm->root, cls_name);
	if(cls == NULL) return NULL;
	var_t* proto = var_get_prototype(cls);
	var_t* self = var_new_obj(vm, proto, NULL, NULL);
	var_t* ctor = var_find_own_member_var(proto, "constructor");
	if(ctor == NULL || !ctor->is_func) return self;
	var_t* args = var_new_array(vm);
	if(arg != NULL) var_array_add(args, arg);
	var_array_reverse(args);   /* call_m_func wants last-arg-at-index-0 */
	var_t* r = call_m_func(vm, self, ctor, args);
	if(r != NULL && r != self) var_unref(r);
	var_unref(args);
	return self;
}

/* Iterate own enumerable members (skipping hidden/invisible/inherited ones) and
 * deep-clone each into `dst`. */
typedef struct { clone_ctx* c; var_t* dst; } member_clone_data;

static void member_clone_cb(const char* key, void* value, void* user_data) {
	(void)key;
	member_clone_data* d = (member_clone_data*)user_data;
	node_t* node = (node_t*)value;
	if(node == NULL || node->var == NULL) return;
	if(node->be_inherited || node->invisable || node->be_unenumerable) return;
	if(d->c->error) return;
	var_t* cv = clone_var(d->c, node->var);
	if(d->c->error) { if(cv != NULL) var_unref(cv); return; }
	var_add(d->dst, node->name, cv);   /* var_add takes the owning ref */
}

static var_t* clone_primitive(vm_t* vm, var_t* s) {
	switch(s->type) {
		case V_UNDEF:   return var_new(vm);
		case V_NULL:    return var_new_null(vm);
		case V_BOOL:    return var_new_bool(vm, var_get_bool(s));
		case V_INT:     return var_new_int(vm, var_get_int(s));
		case V_INT64:   return var_new_int64(vm, var_get_int64(s));
		case V_FLOAT:   return var_new_float(vm, var_get_float(s));
		case V_FLOAT64: return var_new_float64(vm, var_get_float64(s));
		case V_STRING:  return var_new_str(vm, var_get_str(s));
		case V_BIGINT: {
			bignum_t* b = var_get_bigint(s);
			return (b != NULL) ? var_new_bigint(vm, bn_clone(b)) : var_new(vm);
		}
		default: return NULL;
	}
}

static var_t* clone_object(clone_ctx* c, var_t* s) {
	vm_t* vm = c->vm;

	if(s->is_func || var_is_symbol(s) || var_is_proxy(s)) {
		clone_fail(vm, c, "structuredClone: value could not be cloned");
		return NULL;
	}

	/* Date: an object carrying the hidden "@@t" epoch-ms member. */
	var_t* tv = var_find_own_member_var(s, "@@t");
	if(tv != NULL) {
		var_t* d = var_new_obj(vm, var_get_prototype(var_find_member_var(vm->root, "Date")), NULL, NULL);
		node_t* tn = var_add(d, "@@t", var_new_int64(vm, var_get_int64(tv)));
		if(tn != NULL) { tn->be_unenumerable = 1; tn->invisable = 1; }
		ctx_remember(c, s, d);
		return d;
	}

	/* RegExp: reconstruct through the constructor so the program compiles. */
	if(js_regexp_is(s)) {
		var_t* pair = var_new_array(vm);
		var_array_add(pair, var_new_str(vm, get_str(s, "source")));
		var_array_add(pair, var_new_str(vm, get_str(s, "flags")));
		/* construct_builtin passes one arg; RegExp takes (pattern, flags). */
		var_t* cls = var_find_member_var(vm->root, "RegExp");
		var_t* proto = (cls != NULL) ? var_get_prototype(cls) : NULL;
		var_t* re = var_new_obj(vm, proto, NULL, NULL);
		var_t* ctor = (proto != NULL) ? var_find_own_member_var(proto, "constructor") : NULL;
		if(ctor != NULL && ctor->is_func) {
			var_array_reverse(pair);   /* [flags, source] -> call_m_func order */
			var_t* r = call_m_func(vm, re, ctor, pair);
			if(r != NULL && r != re) var_unref(r);
		}
		var_unref(pair);
		ctx_remember(c, s, re);
		return re;
	}

	/* Map / Set: detect by prototype identity, clone via the @@keep anchor.
	 * var_instanceof() takes the CLASS var and derives its prototype internally,
	 * so pass the class (not var_get_prototype of it, which would compare against
	 * Object.prototype and match every plain object). */
	var_t* mapcls = var_find_member_var(vm->root, "Map");
	var_t* setcls = var_find_member_var(vm->root, "Set");
	bool is_map = (mapcls != NULL) && var_instanceof(s, mapcls);
	bool is_set = (!is_map && setcls != NULL) && var_instanceof(s, setcls);
	if(is_map || is_set) {
		var_t* keep = var_find_own_member_var(s, "@@keep");
		var_t* entries = var_new_array(vm);
		if(keep != NULL) {
			uint32_t kn = var_array_size(keep);
			if(is_map) {
				for(uint32_t i = 0; i + 1 < kn; i += 2) {
					var_t* k = clone_var(c, var_array_get_var(keep, (int32_t)i));
					var_t* v = clone_var(c, var_array_get_var(keep, (int32_t)(i + 1)));
					if(c->error) break;
					var_t* pair = var_new_array(vm);
					var_array_add(pair, k);
					var_array_add(pair, v);
					var_array_add(entries, pair);
				}
			} else {
				for(uint32_t i = 0; i < kn; ++i) {
					var_t* v = clone_var(c, var_array_get_var(keep, (int32_t)i));
					if(c->error) break;
					var_array_add(entries, v);
				}
			}
		}
		var_t* m;
		if(!c->error) m = construct_builtin(vm, is_map ? "Map" : "Set", entries);
		else          m = var_new_obj(vm, var_get_prototype(is_map ? mapcls : setcls), NULL, NULL);
		var_unref(entries);
		if(c->error) { if(m != NULL) var_unref(m); return NULL; }
		ctx_remember(c, s, m);
		return m;
	}

	/* ArrayBuffer / TypedArray / DataView: not modelled for cloning here. */
	if(var_is_arraybuffer(s) || var_is_typedarray(s) || var_is_dataview(s)) {
		clone_fail(vm, c, "structuredClone: unsupported host object");
		return NULL;
	}

	/* Array: clone each element (holes collapse to undefined via var_array_get). */
	if(s->is_array) {
		var_t* arr = var_new_array(vm);
		ctx_remember(c, s, arr);
		uint32_t n = var_array_size(s);
		for(uint32_t i = 0; i < n; ++i) {
			var_t* cv = clone_var(c, var_array_get_var(s, (int32_t)i));
			if(c->error) { var_unref(arr); return NULL; }
			var_array_add(arr, cv);
		}
		return arr;
	}

	/* Plain object: deep-clone own enumerable members. */
	var_t* obj = var_new_obj(vm, var_get_prototype(var_find_member_var(vm->root, "Object")), NULL, NULL);
	ctx_remember(c, s, obj);
	member_clone_data d; d.c = c; d.dst = obj;
	vm->gc.gc_defer++;
	hash_map_iterate(&s->children, member_clone_cb, &d);
	vm->gc.gc_defer--;
	if(c->error) { var_unref(obj); return NULL; }
	return obj;
}

static var_t* clone_var(clone_ctx* c, var_t* s) {
	if(s == NULL) return var_new(c->vm);
	if(s->type != V_OBJECT)
		return clone_primitive(c->vm, s);
	var_t* seen = ctx_lookup(c, s);
	if(seen != NULL) return var_ref(seen);   /* shared/cyclic: reuse the clone */
	return clone_object(c, s);
}

static var_t* web_structuredClone(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* value = get_obj(env, "value");
	clone_ctx c;
	c.vm = vm; c.srcs = NULL; c.dsts = NULL; c.n = 0; c.cap = 0; c.error = false;
	var_t* out = NULL;
	if(value == NULL) out = var_new(vm);
	else if(value->type != V_OBJECT) out = clone_primitive(vm, value);
	else out = clone_var(&c, value);
	if(c.error) {
		if(out != NULL) var_unref(out);
		ctx_free(&c);
		return NULL;   /* native_thrown carries the DataCloneError */
	}
	/* clone_var returns refs==0; the return contract wants refs==0. */
	ctx_free(&c);
	return out;
}

/* ==========================================================================
 * crypto
 * ========================================================================== */

/* crypto.getRandomValues(a): fill an array (or a TypedArray's backing array)
 * with bytes 0..255, mirroring js_web.c. Returns the same array. */
static var_t* web_crypto_getRandomValues(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* arr = get_obj(env, "a");
	if(arr == NULL || !arr->is_array) return arr;
	int n = (int)var_array_size(arr);
	for(int i = 0; i < n; ++i) {
		node_t* nd = var_array_get(arr, i);
		uint64_t r = web_rand_next();
		if(nd != NULL && nd->var != NULL)
			node_replace(nd, var_new_int(vm, (int)(r & 0xFF)));
		else
			var_array_add(arr, var_new_int(vm, (int)(r & 0xFF)));
	}
	return arr;
}

/* crypto.randomUUID(): RFC 4122 v4 shape. */
static var_t* web_crypto_randomUUID(vm_t* vm, var_t* env, void* data) {
	(void)env; (void)data;
	unsigned char b[16];
	for(int i = 0; i < 16; ++i) b[i] = (unsigned char)(web_rand_next() & 0xFF);
	b[6] = (unsigned char)((b[6] & 0x0F) | 0x40);   /* version 4 */
	b[8] = (unsigned char)((b[8] & 0x3F) | 0x80);   /* variant 10 */
	static const char hex[] = "0123456789abcdef";
	char out[40];
	int o = 0;
	for(int i = 0; i < 16; ++i) {
		if(i == 4 || i == 6 || i == 8 || i == 10) out[o++] = '-';
		out[o++] = hex[(b[i] >> 4) & 0xF];
		out[o++] = hex[b[i] & 0xF];
	}
	out[o] = 0;
	return var_new_str(vm, out);
}

/* ==========================================================================
 * performance
 * ========================================================================== */

/* Monotonic origin, established once, so now() is a small increasing delta. */
static uint64_t g_perf_origin_ns = 0;

static uint64_t mono_ns(void) {
	struct timespec ts;
#if defined(CLOCK_MONOTONIC)
	clock_gettime(CLOCK_MONOTONIC, &ts);
#else
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static var_t* web_perf_now(vm_t* vm, var_t* env, void* data) {
	(void)env; (void)data;
	uint64_t now = mono_ns();
	return var_new_float64(vm, (double)(now - g_perf_origin_ns) / 1e6);
}

/* mark/measure/clearMarks/clearMeasures: no-op stubs (real scripts call them). */
static var_t* web_perf_noop(vm_t* vm, var_t* env, void* data) {
	(void)env; (void)data; (void)vm;
	return NULL;
}

/* ==========================================================================
 * Registration
 * ========================================================================== */

void reg_native_Web(vm_t* vm) {
	/* structuredClone: a global function. */
	vm_reg_static(vm, NULL, "structuredClone(value, options)", web_structuredClone, NULL);

	/* crypto namespace object. */
	var_t* crypto = var_new_obj_no_proto(vm, NULL, NULL);
	if(crypto != NULL) {
		vm_reg_native_on(vm, crypto, "getRandomValues(a)", web_crypto_getRandomValues, NULL);
		vm_reg_native_on(vm, crypto, "randomUUID()",       web_crypto_randomUUID,      NULL);
		var_add(crypto, SYMKEY_TOSTRINGTAG, var_new_str(vm, "Crypto"));
		var_add(vm->root, "crypto", crypto);
	}

	/* performance namespace object. */
	g_perf_origin_ns = mono_ns();
	var_t* perf = var_new_obj_no_proto(vm, NULL, NULL);
	if(perf != NULL) {
		vm_reg_native_on(vm, perf, "now()",             web_perf_now,   NULL);
		vm_reg_native_on(vm, perf, "mark(n)",           web_perf_noop,  NULL);
		vm_reg_native_on(vm, perf, "measure(n, a, b)",  web_perf_noop,  NULL);
		vm_reg_native_on(vm, perf, "clearMarks(n)",     web_perf_noop,  NULL);
		vm_reg_native_on(vm, perf, "clearMeasures(n)",  web_perf_noop,  NULL);
		var_add(perf, "timeOrigin", var_new_float64(vm, wall_ms()));
		var_add(perf, SYMKEY_TOSTRINGTAG, var_new_str(vm, "Performance"));
		var_add(vm->root, "performance", perf);
	}
}
