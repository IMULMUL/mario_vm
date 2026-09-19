#ifndef MARIO_NATIVE_ABORTCONTROLLER
#define MARIO_NATIVE_ABORTCONTROLLER

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registers the WHATWG abort primitives: `AbortController` (a `signal` plus
 * abort(reason)) and `AbortSignal` (extends EventTarget; aborted/reason/onabort,
 * throwIfAborted(), and the statics abort(reason) / timeout(ms) / any([...])).
 *
 * AbortSignal chains its prototype to the EventTarget prototype, so it inherits
 * addEventListener/removeEventListener/dispatchEvent and satisfies
 * `signal instanceof EventTarget`. Must be registered AFTER reg_native_EventTarget.
 * Neither class exists in the browser's js_web.c, so this is purely additive. */
void reg_native_AbortController(vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif
