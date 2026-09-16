#ifdef __cplusplus
extern "C" {
#endif

#include "native_Stream.h"
#include <string.h>

/* ====== ReadableStream (WHATWG Streams, minimal-but-functional) ======
 * Enough of the surface for app bundles (Next.js RSC / fetch bodies) to build a
 * stream from an underlying source, read it through a reader, and observe
 * close/error:
 *   new ReadableStream({ start(c), pull(c), cancel(reason) })
 *   controller.enqueue(chunk) / close() / error(e) / desiredSize
 *   stream.getReader() -> reader.read() => Promise<{value,done}>,
 *                         reader.releaseLock() / cancel() / closed
 *   stream.locked / cancel() / tee() / pipeThrough(t)
 *
 * Chunks are queued in a hidden JS array; a read with data available resolves
 * immediately via Promise.resolve. A read on an empty, open stream calls
 * pull() once (synchronously) and, if that still yields nothing, returns a
 * DEFERRED promise whose resolve/reject are parked on a pending-request record
 * and fired by a later enqueue()/close()/error(). The deferred is built by
 * running the real Promise constructor with a native executor that captures
 * its __resolve/__reject handles. */

#define CLS_READABLESTREAM "ReadableStream"

#define RS_QUEUE    "@@rs_queue"
#define RS_CLOSED   "@@rs_closed"
#define RS_ERR      "@@rs_err"
#define RS_SRC      "@@rs_src"
#define RS_CTL      "@@rs_ctl"
#define RS_PENDING  "@@rs_pending"
#define RS_READER   "@@rs_reader"
#define RS_STREAM   "@@rs_stream"
#define RS_RES      "@@rs_res"
#define RS_REJ      "@@rs_rej"

typedef struct { var_t* resolve; var_t* reject; } rs_defer_t;

/* mario arrays keep `length` virtual (computed from the hidden _ARRAY_ map), so
 * reading a "length" member yields nothing; use the direct accessors. */
static uint32_t rs_len(var_t* arr) {
	return (arr != NULL) ? var_array_size(arr) : 0u;
}

/* Pop the front element, returning an OWNED var (NULL when empty). */
static var_t* rs_shift(var_t* arr) {
	if(arr == NULL || var_array_size(arr) == 0)
		return NULL;
	var_t* v = var_array_get_var(arr, 0);
	if(v == NULL)
		return NULL;
	var_ref(v);
	var_array_del(arr, 0);
	return v;
}

static void rs_push(var_t* arr, var_t* val) {
	if(arr != NULL && val != NULL)
		var_array_add(arr, val);
}

static node_t* rs_hidden(var_t* obj, const char* key, var_t* val) {
	node_t* n = var_add(obj, key, val);
	if(n != NULL) { n->invisable = 1; n->be_unenumerable = 1; }
	return n;
}

/* The deferred executor: the Promise constructor hands us (resolve, reject)
 * positionally; with no declared parameter names they land in `arguments`. */
static var_t* rs_executor(vm_t* vm, var_t* env, void* data) {
	(void)vm;
	rs_defer_t* d = (rs_defer_t*)data;
	var_t* argv = var_find_member_var(env, "arguments");
	if(argv != NULL) {
		var_t* r = var_find_member_var(argv, "0");
		var_t* j = var_find_member_var(argv, "1");
		if(r != NULL && r->is_func) d->resolve = var_ref(r);
		if(j != NULL && j->is_func) d->reject = var_ref(j);
	}
	return NULL;
}

/* Promise.resolve(v) / Promise.reject(v) via the registered statics. Returns an
 * owned var (hand straight back as the native result). */
static var_t* rs_promise_settle(vm_t* vm, const char* which, var_t* value) {
	var_t* cls = var_find_own_member_var(vm->root, "Promise");
	if(cls == NULL)
		return value ? var_ref(value) : var_new(vm);
	var_t* proto = var_get_prototype(cls);
	var_t* fn = (proto != NULL) ? get_obj(proto, which) : NULL;
	if(fn == NULL || !fn->is_func) fn = get_obj(cls, which);
	if(fn == NULL || !fn->is_func)
		return value ? var_ref(value) : var_new(vm);
	var_t* args = var_new_array(vm);
	var_array_add(args, value ? value : var_new(vm));
	var_t* p = call_m_func(vm, cls, fn, args);
	var_unref(args);
	return p ? p : var_new(vm);
}

/* A pending Promise plus the resolve/reject handles needed to settle it later.
 * The handles are copied onto `rec` (a pending-request record) by the caller. */
static var_t* rs_deferred(vm_t* vm, rs_defer_t* d) {
	d->resolve = NULL; d->reject = NULL;
	var_t* cls = var_find_own_member_var(vm->root, "Promise");
	if(cls == NULL) return NULL;
	var_t* proto = var_get_prototype(cls);
	var_t* ctor = (proto != NULL) ? get_obj(proto, "constructor") : NULL;
	if(ctor == NULL || !ctor->is_func) ctor = get_obj(cls, "constructor");
	if(ctor == NULL || !ctor->is_func) return NULL;
	var_t* executor = var_new_native_func(vm, rs_executor, d);
	var_ref(executor);
	var_t* thisV = var_new_obj(vm, proto, NULL, NULL);
	var_t* args = var_new_array(vm);
	var_array_add(args, executor);
	var_t* p = call_m_func(vm, thisV, ctor, args);
	var_unref(args);
	var_unref(executor);
	return p;
}

/* Build the {value, done} read-result record. Adopts nothing. */
static var_t* rs_result(vm_t* vm, var_t* value, bool done) {
	var_t* rec = var_new_obj(vm, NULL, NULL, NULL);
	var_add(rec, "value", value ? value : var_new(vm));
	var_add(rec, "done", var_new_bool(vm, done));
	return rec;
}

static var_t* rs_controller_of(vm_t* vm, var_t* stream);

/* Fulfil every parked read request. `done` selects the end-of-stream record. */
static void rs_drain_pending(vm_t* vm, var_t* stream, var_t* value, bool done, bool is_err) {
	var_t* pending = get_obj(stream, RS_PENDING);
	while(rs_len(pending) > 0) {
		var_t* rec = rs_shift(pending);
		if(rec == NULL) break;
		var_t* fn = get_obj(rec, is_err ? RS_REJ : RS_RES);
		var_t* arg = is_err ? (value ? value : var_new(vm)) : rs_result(vm, value, done);
		if(fn != NULL && fn->is_func) {
			var_t* a = var_new_array(vm);
			var_array_add(a, arg);
			var_t* r = call_m_func(vm, rec, fn, a);
			var_unref(a);
			if(r != NULL) var_unref(r);
		}
		if(!is_err && arg != NULL) var_unref(arg);
		var_unref(rec);
	}
}

/* controller.enqueue(chunk) */
static var_t* rs_ctl_enqueue(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* ctl = get_obj(env, THIS);
	var_t* chunk = get_obj(env, "chunk");
	var_t* stream = get_obj(ctl, RS_STREAM);
	if(stream == NULL) return NULL;
	var_t* pending = get_obj(stream, RS_PENDING);
	if(rs_len(pending) > 0) {
		var_t* rec = rs_shift(pending);
		if(rec != NULL) {
			var_t* fn = get_obj(rec, RS_RES);
			var_t* arg = rs_result(vm, chunk, false);
			if(fn != NULL && fn->is_func) {
				var_t* a = var_new_array(vm);
				var_array_add(a, arg);
				var_t* r = call_m_func(vm, rec, fn, a);
				var_unref(a);
				if(r != NULL) var_unref(r);
			}
			var_unref(arg);
			var_unref(rec);
			return NULL;
		}
	}
	var_t* q = get_obj(stream, RS_QUEUE);
	if(q == NULL) { q = var_new_array(vm); rs_hidden(stream, RS_QUEUE, q); }
	rs_push(q, chunk);
	return NULL;
}

/* controller.close() */
static var_t* rs_ctl_close(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* ctl = get_obj(env, THIS);
	var_t* stream = get_obj(ctl, RS_STREAM);
	if(stream == NULL) return NULL;
	rs_hidden(stream, RS_CLOSED, var_new_bool(vm, true));
	rs_drain_pending(vm, stream, NULL, true, false);
	return NULL;
}

/* controller.error(e) */
static var_t* rs_ctl_error(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* ctl = get_obj(env, THIS);
	var_t* e = get_obj(env, "e");
	var_t* stream = get_obj(ctl, RS_STREAM);
	if(stream == NULL) return NULL;
	rs_hidden(stream, RS_ERR, e ? e : var_new(vm));
	rs_drain_pending(vm, stream, e, false, true);
	return NULL;
}

static var_t* rs_controller_of(vm_t* vm, var_t* stream) {
	var_t* ctl = get_obj(stream, RS_CTL);
	if(ctl != NULL) return ctl;
	ctl = var_new_obj(vm, NULL, NULL, NULL);
	rs_hidden(ctl, RS_STREAM, stream);
	vm_reg_native_on(vm, ctl, "enqueue(chunk)", rs_ctl_enqueue, NULL);
	vm_reg_native_on(vm, ctl, "close()", rs_ctl_close, NULL);
	vm_reg_native_on(vm, ctl, "error(e)", rs_ctl_error, NULL);
	var_add(ctl, "desiredSize", var_new_int(vm, 1));
	rs_hidden(stream, RS_CTL, ctl);
	return ctl;
}

/* reader.read() => Promise<{value,done}> */
static var_t* rs_reader_read(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* reader = get_obj(env, THIS);
	var_t* stream = get_obj(reader, RS_STREAM);
	if(stream == NULL)
		return rs_promise_settle(vm, "reject", var_new_str(vm, "TypeError: reader has no stream"));

	var_t* err = get_obj(stream, RS_ERR);
	if(err != NULL && err->type != V_UNDEF)
		return rs_promise_settle(vm, "reject", err);

	var_t* q = get_obj(stream, RS_QUEUE);
	if(rs_len(q) > 0) {
		var_t* chunk = rs_shift(q);
		var_t* rec = rs_result(vm, chunk, false);
		if(chunk != NULL) var_unref(chunk);
		var_t* p = rs_promise_settle(vm, "resolve", rec);
		var_unref(rec);
		return p;
	}

	if(get_obj(stream, RS_CLOSED) != NULL && var_get_bool(get_obj(stream, RS_CLOSED)))
		return rs_promise_settle(vm, "resolve", rs_result(vm, NULL, true));

	/* Empty + open: give the source one synchronous pull chance. */
	var_t* src = get_obj(stream, RS_SRC);
	var_t* pull = (src != NULL) ? get_obj(src, "pull") : NULL;
	if(pull != NULL && pull->is_func) {
		var_t* ctl = rs_controller_of(vm, stream);
		var_t* a = var_new_array(vm);
		var_array_add(a, ctl);
		var_t* r = call_m_func(vm, src, pull, a);
		var_unref(a);
		if(r != NULL) var_unref(r);
		q = get_obj(stream, RS_QUEUE);
		if(rs_len(q) > 0) {
			var_t* chunk = rs_shift(q);
			var_t* rec = rs_result(vm, chunk, false);
			if(chunk != NULL) var_unref(chunk);
			var_t* p = rs_promise_settle(vm, "resolve", rec);
			var_unref(rec);
			return p;
		}
		if(get_obj(stream, RS_CLOSED) != NULL && var_get_bool(get_obj(stream, RS_CLOSED)))
			return rs_promise_settle(vm, "resolve", rs_result(vm, NULL, true));
	}

	/* Still nothing: park a deferred read. */
	rs_defer_t d;
	var_t* p = rs_deferred(vm, &d);
	if(p == NULL)
		return rs_promise_settle(vm, "resolve", rs_result(vm, NULL, true));
	var_t* rec = var_new_obj(vm, NULL, NULL, NULL);
	if(d.resolve) rs_hidden(rec, RS_RES, d.resolve); else rs_hidden(rec, RS_RES, var_new(vm));
	if(d.reject) rs_hidden(rec, RS_REJ, d.reject); else rs_hidden(rec, RS_REJ, var_new(vm));
	var_t* pending = get_obj(stream, RS_PENDING);
	if(pending == NULL) { pending = var_new_array(vm); rs_hidden(stream, RS_PENDING, pending); }
	rs_push(pending, rec);
	var_unref(rec);
	return p;
}

static var_t* rs_reader_releaseLock(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* reader = get_obj(env, THIS);
	var_t* stream = get_obj(reader, RS_STREAM);
	if(stream != NULL)
		rs_hidden(stream, RS_READER, var_new(vm));
	rs_hidden(reader, RS_STREAM, var_new(vm));
	return NULL;
}

static var_t* rs_reader_cancel(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* reader = get_obj(env, THIS);
	var_t* stream = get_obj(reader, RS_STREAM);
	if(stream != NULL) {
		rs_hidden(stream, RS_CLOSED, var_new_bool(vm, true));
		rs_drain_pending(vm, stream, NULL, true, false);
	}
	return rs_promise_settle(vm, "resolve", NULL);
}

/* stream.getReader() */
static var_t* rs_getReader(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* stream = get_obj(env, THIS);
	var_t* reader = var_new_obj(vm, NULL, NULL, NULL);
	rs_hidden(reader, RS_STREAM, stream);
	vm_reg_native_on(vm, reader, "read()", rs_reader_read, NULL);
	vm_reg_native_on(vm, reader, "releaseLock()", rs_reader_releaseLock, NULL);
	vm_reg_native_on(vm, reader, "cancel(reason)", rs_reader_cancel, NULL);
	rs_hidden(reader, "closed", rs_promise_settle(vm, "resolve", NULL));
	rs_hidden(stream, RS_READER, reader);
	return reader;
}

/* stream.cancel(reason) */
static var_t* rs_cancel(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* stream = get_obj(env, THIS);
	rs_hidden(stream, RS_CLOSED, var_new_bool(vm, true));
	rs_drain_pending(vm, stream, NULL, true, false);
	var_t* src = get_obj(stream, RS_SRC);
	var_t* cf = (src != NULL) ? get_obj(src, "cancel") : NULL;
	if(cf != NULL && cf->is_func) {
		var_t* r = call_m_func_by_name(vm, src, "cancel", 0);
		if(r != NULL) var_unref(r);
	}
	return rs_promise_settle(vm, "resolve", NULL);
}

/* stream.tee(): two independent readers are not modelled; hand back the same
 * stream twice so destructuring `const [a,b] = s.tee()` still yields streams. */
static var_t* rs_tee(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* stream = get_obj(env, THIS);
	var_t* arr = var_new_array(vm);
	var_array_add(arr, stream);
	var_array_add(arr, stream);
	return arr;
}

/* stream.pipeThrough(transform): return transform.readable when present. */
static var_t* rs_pipeThrough(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* t = get_obj(env, "transform");
	var_t* rd = (t != NULL) ? get_obj(t, "readable") : NULL;
	return rd ? var_ref(rd) : var_ref(get_obj(env, THIS));
}

/* new ReadableStream(underlyingSource, strategy) */
static var_t* rs_constructor(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* stream = get_obj(env, THIS);
	var_t* src = get_obj(env, "underlyingSource");
	rs_hidden(stream, RS_QUEUE, var_new_array(vm));
	rs_hidden(stream, RS_PENDING, var_new_array(vm));
	rs_hidden(stream, RS_CLOSED, var_new_bool(vm, false));
	rs_hidden(stream, RS_ERR, var_new(vm));
	rs_hidden(stream, RS_READER, var_new(vm));
	if(src != NULL && src->type == V_OBJECT)
		rs_hidden(stream, RS_SRC, src);
	var_t* ctl = rs_controller_of(vm, stream);
	var_add(stream, "locked", var_new_bool(vm, false));

	var_t* start = (src != NULL) ? get_obj(src, "start") : NULL;
	if(start != NULL && start->is_func) {
		var_t* a = var_new_array(vm);
		var_array_add(a, ctl);
		var_t* r = call_m_func(vm, src, start, a);
		var_unref(a);
		if(r != NULL) var_unref(r);
	}
	return stream;
}

void reg_native_Stream(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_READABLESTREAM);
	vm_reg_native(vm, cls, "constructor(underlyingSource, strategy)", rs_constructor, NULL);
	vm_reg_native(vm, cls, "getReader()", rs_getReader, NULL);
	vm_reg_native(vm, cls, "cancel(reason)", rs_cancel, NULL);
	vm_reg_native(vm, cls, "tee()", rs_tee, NULL);
	vm_reg_native(vm, cls, "pipeThrough(transform)", rs_pipeThrough, NULL);
	vm_reg_var(vm, cls, SYMKEY_TOSTRINGTAG, var_new_str(vm, "ReadableStream"), true);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
