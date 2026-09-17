#ifdef __cplusplus
extern "C" {
#endif

#include "native_Stream.h"
#include "Promise/native_Promise.h"
#include <string.h>
#include <stdio.h>

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

/* Parked read requests keep their settle handles in an opaque C struct hung
 * off rec->value instead of hidden JS members: the member nodes proved
 * unreliable (the @@rs_res node was gone by the time a later enqueue settled
 * the read, so Next.js flight's parked read never resolved and the whole RSC
 * pipeline stalled). rec->value is opaque to the GC and to member surgery, and
 * the struct owns one ref per handle until rec itself is freed. `promise` owns
 * a ref on the settled promise too: the native resolve/reject closures carry it
 * as a bare data pointer, and the caller drops the read() result right after
 * .then(), so pk must anchor it until a later enqueue/close settles the read. */
typedef struct { var_t* promise; var_t* resolve; var_t* reject; } rs_park_t;

static void rs_park_free(void* p) {
	rs_park_t* pk = (rs_park_t*)p;
	if(pk == NULL) return;
	if(getenv("MARIO_RSDBG") != NULL)
		fprintf(stderr, "[rsdbg] park_free pk=%p res=%p rej=%p\n", (void*)pk,
			(void*)pk->resolve, (void*)pk->reject);
	if(pk->resolve != NULL) var_unref(pk->resolve);
	if(pk->reject != NULL) var_unref(pk->reject);
	if(pk->promise != NULL) var_unref(pk->promise);
	mario_free(pk);
}

static var_t* rs_park_fn(var_t* rec, bool want_rej) {
	rs_park_t* pk = (rec != NULL) ? (rs_park_t*)rec->value : NULL;
	if(pk == NULL) return NULL;
	return want_rej ? pk->reject : pk->resolve;
}

/* mario arrays keep `length` virtual (computed from the hidden _ARRAY_ map), so
 * reading a "length" member yields nothing; use the direct accessors. */
static uint32_t rs_len(var_t* arr) {
	return (arr != NULL) ? var_array_size(arr) : 0u;
}

/* Pop the front element, returning an OWNED var (NULL when empty).
 * var_array_del removes a key WITHOUT re-indexing the survivors (mario arrays
 * are index-keyed maps; Array.prototype.shift compensates by rebuilding), so a
 * bare del(0) would leave index 0 empty while the next chunk still sits at key
 * "1" - every later read() would then miss index 0 and report done:false with
 * an undefined chunk forever (an infinite reader.read().then() pump). Rebuild
 * the queue densely, exactly like native_Array_shift does. */
static var_t* rs_shift(var_t* arr) {
	if(arr == NULL || var_array_size(arr) == 0)
		return NULL;
	uint32_t sz = var_array_size(arr);
	var_t* v = var_array_get_var(arr, 0);
	if(v == NULL)
		return NULL;
	var_ref(v);   /* the owned reference we hand back */
	vm_t* vm = arr->vm;
	vm->gc.gc_defer++;
	var_t* rest = var_new_array(vm);
	uint32_t i;
	for(i = 1; i < sz; ++i) {
		var_t* e = var_array_get_var(arr, (int32_t)i);
		if(e != NULL) var_array_add(rest, e);
	}
	for(i = 0; i < sz; ++i)
		var_array_del(arr, (int32_t)i);
	uint32_t rs = var_array_size(rest);
	for(i = 0; i < rs; ++i) {
		var_t* e = var_array_get_var(rest, (int32_t)i);
		if(e != NULL) var_array_add(arr, e);
	}
	var_unref(rest);
	vm->gc.gc_defer--;
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

/* The deferred promise is built by promise_new_deferred() (native_Promise.c),
 * which returns a genuine builtin-prototype pending promise plus its
 * resolve/reject handles directly - no dependence on window.Promise (rokid's
 * webpack runtime replaces it with a constructor-less polyfill shim, which made
 * the old run-the-global-constructor path fail and stall the RSC flight reader
 * at an immediate done=true). */

/* Promise.resolve(v) / Promise.reject(v) via the engine-internal builders.
 * These do NOT touch window.Promise (rokid's bundle replaces it with a
 * constructor-less polyfill shim). Returns a baseline-refs owned var. */
static var_t* rs_promise_settle(vm_t* vm, const char* which, var_t* value) {
	if(strcmp(which, "reject") == 0)
		return promise_new_rejected(vm, value);
	return promise_new_resolved(vm, value);
}

/* Build the {value, done} read-result record. Adopts nothing.
 *
 * Refcount contract: the record comes back at baseline refs=0 (unowned) and
 * every holder takes its own ref - promise_new_resolved()/resolve_cb() ref it
 * into pd->value and promise_anchor() refs it into the promise's @@keep gc
 * array. Callers must therefore NOT var_unref() it after handing it over: that
 * dropped an owner that never existed, so the next @@keep rebuild (which
 * releases the old entries before re-adding them) took the record to 0 and
 * freed it while pd->value still pointed at it. The reaction then ran with a
 * recycled var_t (type already reused as V_UNDEF) and reader.read() resolved
 * with {value: undefined, done: undefined} - Next.js flight silently lost its
 * first chunk, which is where rokid.com's RSC payload (and its top nav) went. */
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
		var_t* fn = rs_park_fn(rec, is_err);
		var_t* arg = is_err ? (value ? value : var_new(vm)) : rs_result(vm, value, done);
		if(fn != NULL && fn->is_func) {
			var_t* a = var_new_array(vm);
			var_array_add(a, arg);
			var_t* r = call_m_func(vm, rec, fn, a);
			var_unref(a);
			if(r != NULL) var_unref(r);
		}
		/* NB: no var_unref(arg) for the !is_err record (rs_result's contract); the
		 * is_err branch borrows the caller's error value and must not touch it. */
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
	if(getenv("MARIO_RSDBG") != NULL)
		fprintf(stderr, "[rsdbg] enqueue stream=%p pending=%u\n", (void*)stream,
			(unsigned)rs_len(pending));
	if(rs_len(pending) > 0) {
		var_t* rec = rs_shift(pending);
		if(rec != NULL) {
			var_t* fn = rs_park_fn(rec, false);
			var_t* arg = rs_result(vm, chunk, false);
			if(getenv("MARIO_RSDBG") != NULL)
				fprintf(stderr, "[rsdbg] settle rec=%p fn=%p isfunc=%d chunk=%p ctype=%d arg=%p arefs=%d\n",
					(void*)rec, (void*)fn, (fn != NULL && fn->is_func) ? 1 : 0,
					(void*)chunk, (chunk != NULL) ? (int)chunk->type : -1,
					(void*)arg, (arg != NULL) ? (int)arg->refs : -1);
			if(fn != NULL && fn->is_func) {
				var_t* a = var_new_array(vm);
				var_array_add(a, arg);
				var_t* r = call_m_func(vm, rec, fn, a);
				var_unref(a);
				if(r != NULL) var_unref(r);
			}
			/* NB: no var_unref(arg) - see rs_result()'s refcount contract. */
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
	if(getenv("MARIO_RSDBG") != NULL)
		fprintf(stderr, "[rsdbg] close stream=%p\n", (void*)stream);
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
	if(stream == NULL) {
		if(getenv("MARIO_RSDBG") != NULL)
			fprintf(stderr, "[rsdbg] read BAIL reader=%p has NO stream -> reject\n", (void*)reader);
		return rs_promise_settle(vm, "reject", var_new_str(vm, "TypeError: reader has no stream"));
	}
	{ /* DIAG (temp): why does read() never report done? */
		if(getenv("MARIO_RSDBG") != NULL) {
			var_t* dq = get_obj(stream, RS_QUEUE);
			var_t* dc = get_obj(stream, RS_CLOSED);
			fprintf(stderr, "[rsdbg] read stream=%p qlen=%u closed=%d cerr=%p\n", (void*)stream,
				(unsigned)rs_len(dq), (dc != NULL ? (int)var_get_bool(dc) : -1), (void*)dc);
		}
	}

	var_t* err = get_obj(stream, RS_ERR);
	if(err != NULL && err->type != V_UNDEF)
		return rs_promise_settle(vm, "reject", err);

	var_t* q = get_obj(stream, RS_QUEUE);
	if(rs_len(q) > 0) {
		var_t* chunk = rs_shift(q);
		var_t* rec = rs_result(vm, chunk, false);
		if(chunk != NULL) var_unref(chunk);   /* rs_shift's owned ref; rec took its own */
		return rs_promise_settle(vm, "resolve", rec);   /* NB: no var_unref(rec) */
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
			return rs_promise_settle(vm, "resolve", rec);   /* NB: no var_unref(rec) */
		}
		if(get_obj(stream, RS_CLOSED) != NULL && var_get_bool(get_obj(stream, RS_CLOSED)))
			return rs_promise_settle(vm, "resolve", rs_result(vm, NULL, true));
	}

	/* Still nothing: park a deferred read. */
	var_t* res = NULL; var_t* rej = NULL;
	var_t* p = promise_new_deferred(vm, &res, &rej);
	if(p == NULL) {
		if(getenv("MARIO_RSDBG") != NULL)
			fprintf(stderr, "[rsdbg] read BAIL deferred=NULL -> done=true\n");
		return rs_promise_settle(vm, "resolve", rs_result(vm, NULL, true));
	}
	rs_park_t* pk = (rs_park_t*)mario_malloc(sizeof(rs_park_t));
	if(pk == NULL) {
		if(res != NULL) var_unref(res);
		if(rej != NULL) var_unref(rej);
		return rs_promise_settle(vm, "resolve", rs_result(vm, NULL, true));
	}
	pk->promise = var_ref(p);  /* keep the promise alive: the resolve/reject
	                            * closures carry it as a bare data pointer, and
	                            * the flight/pump caller drops the read() result
	                            * right after .then(), so without this ref p is
	                            * freed before a later enqueue settles it. */
	pk->resolve = res;         /* adopts promise_new_deferred's references */
	pk->reject  = rej;
	var_t* rec = var_new_obj(vm, NULL, pk, rs_park_free);
	/* Anchor the promise (and its two settle handles) as hidden members of rec so
	 * they stay gc-reachable: rec lives in the stream's pending array, but pk is
	 * an opaque C struct the GC cannot walk, and the resolve/reject closures only
	 * point at the promise through a bare func->data pointer. Without this a gc
	 * between park and settle could sweep the promise out from under pk. */
	rs_hidden(rec, "@@rs_p", p);
	rs_hidden(rec, "@@rs_res2", res);
	rs_hidden(rec, "@@rs_rej2", rej);
	var_t* pending = get_obj(stream, RS_PENDING);
	if(pending == NULL) { pending = var_new_array(vm); rs_hidden(stream, RS_PENDING, pending); }
	rs_push(pending, rec);
	if(getenv("MARIO_RSDBG") != NULL)
		fprintf(stderr, "[rsdbg] park p=%p rec=%p pk=%p res=%p rej=%p pend=%p refs=%u\n",
			(void*)p, (void*)rec, (void*)pk, (void*)pk->resolve, (void*)pk->reject,
			(void*)pending, (unsigned)rec->refs);
	/* NB: no var_unref(rec) here. rec is created at baseline refs=0 and the
	 * pending array adopts the single reference (var_array_add -> node_new ->
	 * var_ref). A trailing var_unref would drop the array's own ref to 0 and
	 * free rec immediately, leaving a dangling node in `pending` (rs_park_free
	 * ran before the later enqueue could settle the read -> flight stalled). */
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
	if(getenv("MARIO_RSDBG") != NULL)
		fprintf(stderr, "[rsdbg] getReader stream=%p reader=%p\n", (void*)stream, (void*)reader);
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
	if(getenv("MARIO_RSDBG") != NULL) {
		var_t* pull = (src != NULL) ? get_obj(src, "pull") : NULL;
		fprintf(stderr, "[rsdbg] ctor stream=%p src=%p start=%d pull=%d\n", (void*)stream,
			(void*)src, (start != NULL && start->is_func) ? 1 : 0,
			(pull != NULL && pull->is_func) ? 1 : 0);
	}
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
