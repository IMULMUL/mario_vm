#ifdef __cplusplus
extern "C" {
#endif

#include "native_Promise.h"

#define CLS_PROMISE "Promise"

#define PROMISE_STATE_PENDING 0
#define PROMISE_STATE_FULFILLED 1
#define PROMISE_STATE_REJECTED 2

typedef struct {
    int state;
    var_t* value;
    var_t* fulfilled_callbacks;
    var_t* rejected_callbacks;
} promise_data;

static void promise_free(void* p) {
    promise_data* pd = (promise_data*)p;
    if (pd->value) {
        var_unref(pd->value);
    }
    if (pd->fulfilled_callbacks) {
        var_unref(pd->fulfilled_callbacks);
    }
    if (pd->rejected_callbacks) {
        var_unref(pd->rejected_callbacks);
    }
    mario_free(pd);
}

/* The gc mark phase ignores refcounts: anything reachable only from the
 * C-side promise_data would be swept while the promise object is alive
 * (same hazard as Set/Map's C storage). Mirror the C-held vars into a
 * hidden @@keep member so they stay gc-reachable. */
static void promise_anchor(vm_t* vm, var_t* promise, promise_data* pd) {
    vm->gc.gc_defer++; /* rebuild window: entries are unreachable between remove and re-add */
    var_t* keep = var_find_own_member_var(promise, "@@keep");
    if (keep == NULL) {
        node_t* kn = var_add(promise, "@@keep", var_new_array(vm));
        kn->invisable = 1;
        kn->be_unenumerable = 1;
        keep = kn->var;
    }
    var_t* arr = var_find_own_member_var(keep, "_ARRAY_");
    if (arr != NULL) {
        var_remove_all(arr); /* frees the buckets and leaves them NULL */
        hash_map_init(&arr->children);
    }
    if (pd->value) var_array_add(keep, pd->value);
    if (pd->fulfilled_callbacks) var_array_add(keep, pd->fulfilled_callbacks);
    if (pd->rejected_callbacks) var_array_add(keep, pd->rejected_callbacks);
    vm->gc.gc_defer--;
}

static var_t* get_promise_proto(vm_t* vm) {
    node_t* n = vm_load_node(vm, CLS_PROMISE, false);
    if (n != NULL && n->var != NULL) {
        return var_get_prototype(n->var);
    }
    return NULL;
}

/* Whether `x` is a Promise instance: walk its prototype chain looking for the
 * Promise prototype. Only promise objects carry a promise_data in ->value. */
static bool is_promise(vm_t* vm, var_t* x) {
    if (x == NULL || x->type != V_OBJECT) {
        return false;
    }
    var_t* proto = get_promise_proto(vm);
    if (proto == NULL) {
        return false;
    }
    var_t* p = var_get_prototype(x);
    int guard = 0;
    while (p != NULL && guard++ < 64) {
        if (p == proto) {
            return true;
        }
        p = var_get_prototype(p);
    }
    return false;
}

/* Promise adoption: a then-callback returning a promise resolves the chained
 * promise with that promise's settled value, not the promise object itself.
 * Consumes the ref carried by `result` when swapping it for the inner value. */
static var_t* promise_unwrap(vm_t* vm, var_t* result) {
    int guard = 0;
    while (result != NULL && is_promise(vm, result) && guard++ < 64) {
        promise_data* rpd = (promise_data*)result->value;
        var_t* inner = (rpd != NULL) ? rpd->value : NULL;
        if (inner == NULL) { /* pending/empty promise -> undefined */
            var_unref(result);
            return NULL;
        }
        var_ref(inner);
        var_unref(result);
        result = inner;
    }
    return result;
}

/* __await(x): the runtime helper for the ES `await` operator in this
 * synchronous implementation. It unwraps a (possibly nested) promise and
 * yields the underlying value; non-promise values pass through unchanged.
 * The returned var is borrowed - func_call keeps it alive across env
 * teardown, so we must NOT add our own reference here. */
var_t* native_await(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* x = get_obj(env, "x");
    int guard = 0;
    while (is_promise(vm, x) && guard++ < 64) {
        promise_data* pd = (promise_data*)x->value;
        if (pd == NULL || pd->value == NULL) {
            return NULL; /* await of an empty/pending promise -> undefined */
        }
        x = pd->value;
    }
    return x;
}

/* The executor's resolve/reject arguments: native closures whose data points
 * at the promise var. The promise stays alive across the executor call (it is
 * stack-anchored by the constructor), and these fns are members of the promise
 * itself, so the bare data pointer can not outlive its target in the
 * synchronous flows exercised here. */
static var_t* native_promise_resolve_cb(vm_t* vm, var_t* env, void* data) {
    var_t* promise = (var_t*)data;
    var_t* value = get_obj(env, "value");
    promise_data* pd = (promise_data*)promise->value;
    if (pd != NULL && pd->state == PROMISE_STATE_PENDING) {
        pd->state = PROMISE_STATE_FULFILLED;
        pd->value = (value != NULL) ? var_ref(value) : var_ref(var_new(vm));
        promise_anchor(vm, promise, pd);
        uint32_t n = var_array_size(pd->fulfilled_callbacks);
        for (uint32_t i = 0; i < n; i++) {
            var_t* cb = var_array_get_var(pd->fulfilled_callbacks, i);
            if (cb != NULL && cb->is_func) {
                var_t* args = var_new_array(vm);
                var_array_add(args, pd->value);
                var_t* r = call_m_func(vm, promise, cb, args);
                if (r != NULL) var_unref(r);
                var_unref(args);
            }
        }
    }
    return NULL;
}

static var_t* native_promise_reject_cb(vm_t* vm, var_t* env, void* data) {
    var_t* promise = (var_t*)data;
    var_t* reason = get_obj(env, "reason");
    promise_data* pd = (promise_data*)promise->value;
    if (pd != NULL && pd->state == PROMISE_STATE_PENDING) {
        pd->state = PROMISE_STATE_REJECTED;
        pd->value = (reason != NULL) ? var_ref(reason) : var_ref(var_new(vm));
        promise_anchor(vm, promise, pd);
        uint32_t n = var_array_size(pd->rejected_callbacks);
        for (uint32_t i = 0; i < n; i++) {
            var_t* cb = var_array_get_var(pd->rejected_callbacks, i);
            if (cb != NULL && cb->is_func) {
                var_t* args = var_new_array(vm);
                var_array_add(args, pd->value);
                var_t* r = call_m_func(vm, promise, cb, args);
                if (r != NULL) var_unref(r);
                var_unref(args);
            }
        }
    }
    return NULL;
}

var_t* native_PromiseConstructor(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* thisV = get_obj(env, THIS);
    var_t* executor = get_obj(env, "executor");

    promise_data* pd = (promise_data*)mario_malloc(sizeof(promise_data));
    pd->state = PROMISE_STATE_PENDING;
    pd->value = NULL;
    pd->fulfilled_callbacks = var_new_array(vm);
    pd->rejected_callbacks = var_new_array(vm);

    var_t* obj = var_new_obj_no_proto(vm, pd, promise_free);
    var_instance_from(obj, thisV);
    promise_anchor(vm, obj, pd);
    var_ref(obj); /* off the gc list: the executor call below may trigger a gc */
    vm_push(vm, obj); /* stack-anchored so its children get gc-marked during the call */

    if (executor != NULL) {
        node_t* rn = vm_reg_native_on(vm, obj, "__resolve(value)", native_promise_resolve_cb, obj);
        node_t* jn = vm_reg_native_on(vm, obj, "__reject(reason)", native_promise_reject_cb, obj);
        rn->invisable = 1;
        rn->be_unenumerable = 1;
        jn->invisable = 1;
        jn->be_unenumerable = 1;

        var_t* resolve_args = var_new_array(vm);
        var_array_add(resolve_args, rn->var);
        var_array_add(resolve_args, jn->var);

        call_m_func(vm, obj, executor, resolve_args);
        var_unref(resolve_args);
    }

    /* vm_pop would unref the anchor, putting obj back on the gc list where the
     * gc it may itself trigger sweeps it before we return. vm_pop2 keeps the
     * ref; drop both (anchor + ours) bare so obj stays OFF the gc list until
     * func_call re-refs it as the return value. */
    vm_pop2(vm);
    obj->refs -= 2;
    return obj;
}

var_t* native_PromiseResolve(vm_t* vm, var_t* env, void* data) {
    (void)data;
    (void)env;
    var_t* value = get_obj(env, "value");

    promise_data* pd = (promise_data*)mario_malloc(sizeof(promise_data));
    pd->state = PROMISE_STATE_FULFILLED;
    pd->value = var_ref(value);
    pd->fulfilled_callbacks = var_new_array(vm);
    pd->rejected_callbacks = var_new_array(vm);

    var_t* proto = get_promise_proto(vm);
    var_t* promise = var_new_obj(vm, proto, pd, promise_free);
    promise_anchor(vm, promise, pd);

    return promise;
}

var_t* native_PromiseReject(vm_t* vm, var_t* env, void* data) {
    (void)data;
    (void)env;
    var_t* reason = get_obj(env, "reason");

    promise_data* pd = (promise_data*)mario_malloc(sizeof(promise_data));
    pd->state = PROMISE_STATE_REJECTED;
    pd->value = var_ref(reason);
    pd->fulfilled_callbacks = var_new_array(vm);
    pd->rejected_callbacks = var_new_array(vm);

    var_t* proto = get_promise_proto(vm);
    var_t* promise = var_new_obj(vm, proto, pd, promise_free);
    promise_anchor(vm, promise, pd);

    return promise;
}

var_t* native_PromiseThen(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* promise = get_obj(env, THIS);
    var_t* onFulfilled = get_obj(env, "onFulfilled");
    var_t* onRejected = get_obj(env, "onRejected");

    promise_data* pd = (promise_data*)promise->value;

    if (pd == NULL) {
        pd = (promise_data*)mario_malloc(sizeof(promise_data));
        pd->state = PROMISE_STATE_PENDING;
        pd->value = NULL;
        pd->fulfilled_callbacks = var_new_array(vm);
        pd->rejected_callbacks = var_new_array(vm);
    }

    promise_data* newPd = (promise_data*)mario_malloc(sizeof(promise_data));
    newPd->state = pd->state;
    newPd->value = pd->value ? var_ref(pd->value) : NULL;
    newPd->fulfilled_callbacks = var_new_array(vm);
    newPd->rejected_callbacks = var_new_array(vm);

    var_t* proto = var_get_prototype(promise);
    var_t* newPromise = var_new_obj(vm, proto, newPd, promise_free);
    promise_anchor(vm, newPromise, newPd);
    var_ref(newPromise); /* off the gc list: the callback calls below may trigger a gc */
    vm_push(vm, newPromise); /* stack-anchored so its children get gc-marked during callbacks */

    if (pd->state == PROMISE_STATE_FULFILLED && onFulfilled != NULL) {
        var_t* args = var_new_array(vm);
        var_array_add(args, pd->value);
        var_t* result = call_m_func(vm, promise, onFulfilled, args);
        result = promise_unwrap(vm, result);
        var_t* old = newPd->value;
        if (result != NULL) {
            newPd->value = result; /* adopt the ref result already carries */
        } else {
            newPd->value = var_ref(var_new_null(vm));
        }
        promise_anchor(vm, newPromise, newPd); /* gc-reachable before any unref below */
        if (old) {
            var_unref(old);
        }
        var_unref(args);
    } else if (pd->state == PROMISE_STATE_REJECTED && onRejected != NULL) {
        var_t* args = var_new_array(vm);
        var_array_add(args, pd->value);
        var_t* result = call_m_func(vm, promise, onRejected, args);
        result = promise_unwrap(vm, result);
        var_t* old = newPd->value;
        if (result != NULL) {
            newPd->value = result; /* adopt the ref result already carries */
        } else {
            newPd->value = var_ref(var_new_null(vm));
        }
        newPd->state = PROMISE_STATE_FULFILLED;
        promise_anchor(vm, newPromise, newPd); /* gc-reachable before any unref below */
        if (old) {
            var_unref(old);
        }
        var_unref(args);
    } else if (pd->state == PROMISE_STATE_PENDING) {
        if (onFulfilled != NULL) {
            var_array_add(pd->fulfilled_callbacks, onFulfilled);
        }
        if (onRejected != NULL) {
            var_array_add(pd->rejected_callbacks, onRejected);
        }
    }

    vm_pop2(vm); /* pop the anchor keeping its ref (vm_pop could gc-sweep newPromise) */
    newPromise->refs -= 2; /* drop anchor ref + ours; stays off the gc list for the return */
    return newPromise;
}

var_t* native_PromiseCatch(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* promise = get_obj(env, THIS);
    var_t* onRejected = get_obj(env, "onRejected");

    var_t* new_env = var_new(vm);
    var_ref(new_env); /* keep it off the gc list while Then may run callbacks */
    var_add(new_env, THIS, promise);
    var_t* null_var = var_new_null(vm);
    var_add(new_env, "onFulfilled", null_var);
    var_unref(null_var);
    var_add(new_env, "onRejected", onRejected);

    var_t* result = native_PromiseThen(vm, new_env, NULL);
    var_unref(new_env);
    return result;
}

var_t* native_PromiseFinally(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* promise = get_obj(env, THIS);
    var_t* onFinally = get_obj(env, "onFinally");

    var_t* new_env = var_new(vm);
    var_ref(new_env); /* keep it off the gc list while Then may run callbacks */
    var_add(new_env, THIS, promise);
    var_add(new_env, "onFulfilled", onFinally);
    var_add(new_env, "onRejected", onFinally);

    var_t* result = native_PromiseThen(vm, new_env, NULL);
    var_unref(new_env);
    return result;
}

var_t* native_PromiseAll(vm_t* vm, var_t* env, void* data) {
    (void)data;
    (void)env;
    var_t* promises = get_obj(env, "promises");

    promise_data* pd = (promise_data*)mario_malloc(sizeof(promise_data));
    pd->state = PROMISE_STATE_FULFILLED;
    /* promise_free() unrefs pd->value, so promise_data must OWN a reference on the
     * result array (matching native_PromiseResolve's var_ref(value)); @@keep adds a
     * second gc-only ref. Without this own ref the array sat at refs=1 (anchor only)
     * and, when a then-callback returned this promise, promise_unwrap()'s release of
     * the promise let promise_free drop the array to 0 -- freeing the adopted value,
     * so the chained .then saw undefined. */
    pd->value = var_ref(var_new_array(vm));
    pd->fulfilled_callbacks = var_new_array(vm);
    pd->rejected_callbacks = var_new_array(vm);

    if (promises != NULL && promises->is_array) {
        uint32_t len = var_array_size(promises);
        for (uint32_t i = 0; i < len; i++) {
            var_t* item = var_array_get_var(promises, i);
            if (item == NULL) {
                continue;
            }
            if (is_promise(vm, item)) {
                promise_data* ipd = (promise_data*)item->value;
                if (ipd != NULL && ipd->state == PROMISE_STATE_REJECTED) {
                    /* reject fast: the result adopts the first rejection reason */
                    pd->state = PROMISE_STATE_REJECTED;
                    var_t* old = pd->value;
                    pd->value = ipd->value ? var_ref(ipd->value) : var_ref(var_new(vm));
                    if (old != NULL) var_unref(old);
                    break;
                }
                var_array_add(pd->value, (ipd != NULL && ipd->value != NULL) ? ipd->value : var_new(vm));
            } else {
                var_array_add(pd->value, item);
            }
        }
    }

    var_t* proto = get_promise_proto(vm);
    var_t* result = var_new_obj(vm, proto, pd, promise_free);
    promise_anchor(vm, result, pd);
    return result;
}

var_t* native_PromiseRace(vm_t* vm, var_t* env, void* data) {
    (void)data;
    (void)env;
    var_t* promises = get_obj(env, "promises");

    promise_data* pd = (promise_data*)mario_malloc(sizeof(promise_data));
    pd->state = PROMISE_STATE_PENDING;
    pd->value = NULL;
    pd->fulfilled_callbacks = var_new_array(vm);
    pd->rejected_callbacks = var_new_array(vm);

    if (promises != NULL && promises->is_array) {
        uint32_t len = var_array_size(promises);
        /* settle with the first already-settled entry (pending ones lose the race) */
        for (uint32_t i = 0; i < len; i++) {
            var_t* item = var_array_get_var(promises, i);
            if (item == NULL) {
                continue;
            }
            if (is_promise(vm, item)) {
                promise_data* ipd = (promise_data*)item->value;
                if (ipd != NULL && ipd->state != PROMISE_STATE_PENDING) {
                    pd->state = ipd->state;
                    pd->value = ipd->value ? var_ref(ipd->value) : var_ref(var_new(vm));
                    break;
                }
            } else {
                pd->state = PROMISE_STATE_FULFILLED;
                pd->value = var_ref(item);
                break;
            }
        }
    }

    var_t* proto = get_promise_proto(vm);
    var_t* result = var_new_obj(vm, proto, pd, promise_free);
    promise_anchor(vm, result, pd);
    return result;
}

var_t* native_PromiseAllSettled(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* promises = get_obj(env, "promises");

    promise_data* pd = (promise_data*)mario_malloc(sizeof(promise_data));
    pd->state = PROMISE_STATE_FULFILLED;
    /* Own a ref on the result array; see the identical note in native_PromiseAll. */
    pd->value = var_ref(var_new_array(vm));
    pd->fulfilled_callbacks = var_new_array(vm);
    pd->rejected_callbacks = var_new_array(vm);

    if (promises != NULL && promises->is_array) {
        uint32_t len = var_array_size(promises);
        for (uint32_t i = 0; i < len; i++) {
            var_t* item = var_array_get_var(promises, i);
            var_t* entry = var_new_obj(vm, NULL, NULL, NULL);
            if (item != NULL && is_promise(vm, item)) {
                promise_data* ipd = (promise_data*)item->value;
                if (ipd != NULL && ipd->state == PROMISE_STATE_REJECTED) {
                    var_add(entry, "status", var_new_str(vm, "rejected"));
                    var_add(entry, "reason", ipd->value);
                } else {
                    var_add(entry, "status", var_new_str(vm, "fulfilled"));
                    var_add(entry, "value", (ipd != NULL) ? ipd->value : NULL);
                }
            } else {
                var_add(entry, "status", var_new_str(vm, "fulfilled"));
                var_add(entry, "value", item);
            }
            var_array_add(pd->value, entry);
        }
    }

    var_t* proto = get_promise_proto(vm);
    var_t* result = var_new_obj(vm, proto, pd, promise_free);
    promise_anchor(vm, result, pd);
    return result;
}

/* Promise.any (ES2021): fulfills with the first fulfilled input; a non-promise
 * input counts as already fulfilled. If every input rejects -- or the iterable
 * is empty -- it rejects with an AggregateError holding all the rejection
 * reasons. A pending input can never settle in this synchronous VM, so an
 * all-or-partly-pending call with no fulfillment stays pending (mirroring
 * native_PromiseRace's treatment of unsettled inputs). */
var_t* native_PromiseAny(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* promises = get_obj(env, "promises");

    promise_data* pd = (promise_data*)mario_malloc(sizeof(promise_data));
    pd->state = PROMISE_STATE_PENDING;
    pd->value = NULL;
    pd->fulfilled_callbacks = var_new_array(vm);
    pd->rejected_callbacks = var_new_array(vm);

    bool fulfilled = false;
    bool sawPending = false;
    var_t* errors = var_new_array(vm); /* rejection reasons; refs=0 until adopted */

    if (promises != NULL && promises->is_array) {
        uint32_t len = var_array_size(promises);
        for (uint32_t i = 0; i < len; i++) {
            var_t* item = var_array_get_var(promises, i);
            if (item == NULL) {
                continue;
            }
            if (is_promise(vm, item)) {
                promise_data* ipd = (promise_data*)item->value;
                if (ipd == NULL || ipd->state == PROMISE_STATE_PENDING) {
                    sawPending = true;
                    continue;
                }
                if (ipd->state == PROMISE_STATE_FULFILLED) {
                    pd->state = PROMISE_STATE_FULFILLED;
                    pd->value = ipd->value ? var_ref(ipd->value) : var_ref(var_new(vm));
                    fulfilled = true;
                    break;
                }
                /* rejected: record the reason and keep looking for a fulfillment */
                var_array_add(errors, ipd->value ? ipd->value : var_new(vm));
            } else {
                /* a non-promise value is treated as already fulfilled */
                pd->state = PROMISE_STATE_FULFILLED;
                pd->value = var_ref(item);
                fulfilled = true;
                break;
            }
        }
    }

    if (!fulfilled) {
        if (sawPending) {
            /* some input may still settle (never, here) -> stay pending */
            var_unref(errors);
        } else {
            /* every input rejected, or the iterable was empty -> AggregateError */
            node_t* an = vm_load_node(vm, "AggregateError", false);
            var_t* aggProto = (an != NULL && an->var != NULL) ? var_get_prototype(an->var) : NULL;
            var_t* aggErr = var_new_obj(vm, aggProto, NULL, NULL);
            var_add(aggErr, "name", var_new_str(vm, "AggregateError"));
            var_add(aggErr, "message", var_new_str(vm, "All promises were rejected"));
            var_add(aggErr, "errors", errors); /* refs errors (0->1); owned by aggErr */
            pd->state = PROMISE_STATE_REJECTED;
            pd->value = var_ref(aggErr); /* pd owns one ref, matching Promise.all reject */
        }
    }

    var_t* proto = get_promise_proto(vm);
    var_t* result = var_new_obj(vm, proto, pd, promise_free);
    promise_anchor(vm, result, pd);
    return result;
}

/* Synchronous VM without an event loop: setTimeout never fires its callback.
 * A delayed promise therefore simply stays pending, which is exactly what
 * Promise.race semantics need here. */
static var_t* native_setTimeout(vm_t* vm, var_t* env, void* data) {
    (void)vm; (void)env; (void)data;
    return NULL;
}

void reg_native_Promise(vm_t* vm) {
    var_t* cls = vm_new_class(vm, CLS_PROMISE);
    vm_reg_native(vm, cls, "constructor(executor)", native_PromiseConstructor, NULL);
    vm_reg_static(vm, cls, "resolve(value)", native_PromiseResolve, NULL);
    vm_reg_static(vm, cls, "reject(reason)", native_PromiseReject, NULL);
    vm_reg_static(vm, cls, "all(promises)", native_PromiseAll, NULL);
    vm_reg_static(vm, cls, "allSettled(promises)", native_PromiseAllSettled, NULL);
    vm_reg_static(vm, cls, "race(promises)", native_PromiseRace, NULL);
    vm_reg_static(vm, cls, "any(promises)", native_PromiseAny, NULL);
    vm_reg_native(vm, cls, "then(onFulfilled, onRejected)", native_PromiseThen, NULL);
    vm_reg_native(vm, cls, "catch(onRejected)", native_PromiseCatch, NULL);
    vm_reg_native(vm, cls, "finally(onFinally)", native_PromiseFinally, NULL);

    /* Runtime helpers for the synchronous async/await support. These are
     * registered as global (free) functions so the compiler can emit
     * `INSTR_CALL "__await$1"` / `"__promise_resolve$1"`. */
    vm_reg_native(vm, NULL, "__await(x)", native_await, NULL);
    vm_reg_native(vm, NULL, "__promise_resolve(value)", native_PromiseResolve, NULL);
    vm_reg_native(vm, NULL, "setTimeout(cb, ms)", native_setTimeout, NULL);
}

#ifdef __cplusplus
}
#endif
