#include <string.h>
#include <stdlib.h>

#include "native_AbortController.h"
#include "../EventTarget/native_EventTarget.h"

#define CLS_ABORTCONTROLLER "AbortController"
#define CLS_ABORTSIGNAL "AbortSignal"

/* Hidden member used to anchor a captured var on a native callback so it stays
 * GC-reachable while a timer/listener holds the callback (func->data is a raw
 * void* the GC mark phase does not follow). */
#define ANCHOR "@@abort_anchor"

/* Own member, hidden from enumeration (mimics a prototype accessor). */
static void sig_set(var_t* self, const char* key, var_t* val) {
	node_t* n = var_add(self, key, val);
	if(n != NULL) n->be_unenumerable = 1;
}

/* An Error-prototype object with a custom name/message - stands in for a
 * DOMException ("AbortError"/"TimeoutError"), which this engine does not model.
 * Returned with refs==0; the caller stores it (var_add takes the owning ref). */
static var_t* make_named_error(vm_t* vm, const char* name, const char* message) {
	var_t* Err = var_find_member_var(vm->root, "Error");
	var_t* proto = (Err != NULL) ? var_get_prototype(Err) : NULL;
	var_t* err = var_new_obj(vm, proto, NULL, NULL);
	var_add(err, "name", var_new_str(vm, name));
	var_add(err, "message", var_new_str(vm, message));
	return err;
}

static void signal_init(vm_t* vm, var_t* sig) {
	sig_set(sig, "aborted", var_new_bool(vm, false));
	sig_set(sig, "reason", var_new(vm));          /* undefined until aborted */
	sig_set(sig, "onabort", var_new_null(vm));
}

/* Allocate a fresh AbortSignal instance (prototype = AbortSignal.prototype so it
 * inherits EventTarget and satisfies instanceof). */
static var_t* signal_new(vm_t* vm) {
	var_t* cls = var_find_member_var(vm->root, CLS_ABORTSIGNAL);
	var_t* proto = (cls != NULL) ? var_get_prototype(cls) : NULL;
	var_t* sig = var_new_obj(vm, proto, NULL, NULL);
	signal_init(vm, sig);
	return sig;
}

static bool signal_is_aborted(var_t* sig) {
	var_t* ab = var_find_own_member_var(sig, "aborted");
	return (ab != NULL && var_get_bool(ab));
}

/* Core abort: idempotent, sets aborted/reason and dispatches an "abort" event.
 * `reason` (may be NULL) is borrowed - var_add takes its own reference. */
static void signal_do_abort(vm_t* vm, var_t* sig, var_t* reason) {
	if(sig == NULL || signal_is_aborted(sig)) return;
	if(reason == NULL) reason = make_named_error(vm, "AbortError", "This operation was aborted");
	sig_set(sig, "aborted", var_new_bool(vm, true));
	sig_set(sig, "reason", reason);
	var_t* ev = native_Event_new(vm, "abort");
	native_EventTarget_dispatch(vm, sig, ev);
	var_unref(ev);
}

/* --------------------------------------------------------------------------
 * AbortController
 * -------------------------------------------------------------------------- */

static var_t* ac_constructor(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* self = get_obj(env, THIS);
	if(self == NULL) return NULL;
	var_t* sig = signal_new(vm);
	sig_set(self, "signal", sig);   /* var_add takes the owning ref */
	return self;
}

static var_t* ac_abort(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* self = get_obj(env, THIS);
	var_t* reason = get_obj(env, "reason");
	if(self == NULL) return NULL;
	var_t* sig = var_find_own_member_var(self, "signal");
	var_t* use = (reason != NULL && reason->type != V_UNDEF) ? reason : NULL;
	signal_do_abort(vm, sig, use);
	return NULL;
}

/* --------------------------------------------------------------------------
 * AbortSignal
 * -------------------------------------------------------------------------- */

static var_t* as_constructor(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* self = get_obj(env, THIS);
	if(self == NULL) return NULL;
	signal_init(vm, self);
	return self;
}

static var_t* as_throwIfAborted(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* self = get_obj(env, THIS);
	if(self != NULL && signal_is_aborted(self)) {
		var_t* reason = var_find_own_member_var(self, "reason");
		if(reason == NULL) reason = var_new(vm);   /* undefined */
		/* Throw the reason value itself (not a formatted message): record it in
		 * native_thrown; func_call delivers it after we return the dummy. */
		if(vm->native_thrown != NULL) var_unref(vm->native_thrown);
		vm->native_thrown = var_ref(reason);
	}
	return NULL;
}

/* AbortSignal.abort(reason): an already-aborted signal. */
static var_t* as_abort(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* reason = get_obj(env, "reason");
	var_t* sig = signal_new(vm);
	var_t* use = (reason != NULL && reason->type != V_UNDEF) ? reason : NULL;
	/* signal_new returns an unowned (refs==0) signal. signal_do_abort's dispatch
	 * stores it into the event's target/currentTarget/srcElement and then frees
	 * the event, which would cascade-unref the otherwise-unowned signal down to 0
	 * and free it before we can return it. Take a guard ref across the abort and
	 * release it with a bare refs-- (the same idiom func_call uses at its return)
	 * so the signal survives at the refs==0 baseline the return contract wants. */
	var_ref(sig);
	signal_do_abort(vm, sig, use);
	sig->refs--;
	return sig;
}

/* setTimeout trampoline: aborts the captured signal with a TimeoutError. */
static var_t* abort_timeout_cb(vm_t* vm, var_t* env, void* data) {
	(void)env;
	var_t* sig = (var_t*)data;
	if(sig == NULL || signal_is_aborted(sig)) return NULL;
	var_t* reason = make_named_error(vm, "TimeoutError", "The operation timed out");
	signal_do_abort(vm, sig, reason);
	return NULL;
}

/* AbortSignal.timeout(ms): a signal that aborts after ms via the global
 * setTimeout (present in both the CLI host_task and the browser js_dom). */
static var_t* as_timeout(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* msv = get_obj(env, "ms");
	uint32_t ms = (msv != NULL) ? (uint32_t)var_get_float(msv) : 0;
	var_t* sig = signal_new(vm);

	var_t* st = var_find_member_var(vm->root, "setTimeout");
	if(st == NULL || !st->is_func) return sig;   /* no timer: never aborts */

	var_t* tr = var_new_native_func(vm, abort_timeout_cb, sig);
	node_t* an = var_add(tr, ANCHOR, sig);       /* keep sig GC-reachable via tr */
	if(an != NULL) { an->invisable = 1; an->be_unenumerable = 1; }

	var_t* args = var_new_array(vm);
	var_array_add(args, tr);
	var_array_add(args, var_new_int(vm, (int)ms));
	var_array_reverse(args);                      /* call_m_func wants last-at-0 */
	var_t* r = call_m_func(vm, NULL, st, args);
	if(r != NULL) var_unref(r);
	var_unref(args);
	return sig;
}

/* abort listener installed on each source signal by AbortSignal.any. */
static var_t* abort_any_cb(vm_t* vm, var_t* env, void* data) {
	var_t* composite = (var_t*)data;
	var_t* reason = NULL;
	var_t* args = get_func_args(env);
	node_t* an = (args != NULL) ? var_array_get(args, 0) : NULL;
	var_t* ev = (an != NULL) ? an->var : NULL;
	if(ev != NULL) {
		var_t* src = var_find_member_var(ev, "currentTarget");
		if(src != NULL) reason = var_find_member_var(src, "reason");
	}
	signal_do_abort(vm, composite, reason);
	return NULL;
}

/* AbortSignal.any(signals): a composite that aborts when any source aborts (or
 * is already aborted). */
static var_t* as_any(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* signals = get_obj(env, "signals");
	var_t* composite = signal_new(vm);
	/* Guard the unowned composite across any dispatch (see as_abort). The listener
	 * anchors added below also keep it alive, but the guard covers the early-return
	 * path taken before any anchor exists. Released with a bare refs-- at return. */
	var_ref(composite);
	if(signals == NULL || signals->type != V_OBJECT) { composite->refs--; return composite; }

	uint32_t n = var_array_size(signals);
	for(uint32_t i = 0; i < n; ++i) {
		node_t* sn = var_array_get(signals, (int32_t)i);
		var_t* src = (sn != NULL) ? sn->var : NULL;
		if(src == NULL || src->type != V_OBJECT) continue;
		if(signal_is_aborted(src)) {
			signal_do_abort(vm, composite, var_find_own_member_var(src, "reason"));
			composite->refs--;
			return composite;   /* first already-aborted source wins */
		}
		var_t* cb = var_new_native_func(vm, abort_any_cb, composite);
		node_t* an = var_add(cb, ANCHOR, composite);   /* GC anchor (refs composite) */
		if(an != NULL) { an->invisable = 1; an->be_unenumerable = 1; }
		native_EventTarget_add_listener(vm, src, "abort", cb, false);
	}
	composite->refs--;
	return composite;
}

/* --------------------------------------------------------------------------
 * Registration
 * -------------------------------------------------------------------------- */

void reg_native_AbortController(vm_t* vm) {
	/* AbortSignal extends EventTarget (chain prototypes for inheritance +
	 * instanceof). EventTarget must already be registered. */
	var_t* as = vm_new_class(vm, CLS_ABORTSIGNAL);
	var_t* et = native_EventTarget_class(vm);
	if(et != NULL)
		var_set_prototype(var_get_prototype(as), var_get_prototype(et));
	vm_reg_native(vm, as, "constructor()", as_constructor, NULL);
	vm_reg_native(vm, as, "throwIfAborted()", as_throwIfAborted, NULL);
	vm_reg_static(vm, as, "abort(reason)", as_abort, NULL);
	vm_reg_static(vm, as, "timeout(ms)", as_timeout, NULL);
	vm_reg_static(vm, as, "any(signals)", as_any, NULL);
	vm_reg_var(vm, as, SYMKEY_TOSTRINGTAG, var_new_str(vm, CLS_ABORTSIGNAL), true);

	/* AbortController */
	var_t* ac = vm_new_class(vm, CLS_ABORTCONTROLLER);
	vm_reg_native(vm, ac, "constructor()", ac_constructor, NULL);
	vm_reg_native(vm, ac, "abort(reason)", ac_abort, NULL);
	vm_reg_var(vm, ac, SYMKEY_TOSTRINGTAG, var_new_str(vm, CLS_ABORTCONTROLLER), true);
}
