#ifndef MARIO_PROMISE
#define MARIO_PROMISE

#include "mario.h"

void reg_native_Promise(vm_t* vm);

/* Engine-internal promise construction, independent of the window.Promise
 * global (which page bundles may replace with a polyfill). See native_Promise.c.
 * promise_new_deferred returns a pending promise at baseline refs and hands
 * back var_ref'd resolve/reject handles owned by the caller; settle by calling
 * a handle with a single-element argument array. */
var_t* promise_new_deferred(vm_t* vm, var_t** out_resolve, var_t** out_reject);
var_t* promise_new_resolved(vm_t* vm, var_t* value);
var_t* promise_new_rejected(vm_t* vm, var_t* reason);

/* Debug ledger (MARIO_PROMLEDGER): dump every live PENDING promise that has a
 * registered onFulfilled callback, i.e. an await that can never progress. */
void mario_promise_ledger_dump(vm_t* vm);

#endif
