#ifndef MARIO_NATIVE_EVENTTARGET
#define MARIO_NATIVE_EVENTTARGET

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registers the DOM/WHATWG event trio: `EventTarget` (addEventListener /
 * removeEventListener / dispatchEvent backed by a hidden listener list), `Event`
 * (type/bubbles/cancelable/composed + preventDefault/stopPropagation/
 * stopImmediatePropagation/composedPath and the phase constants), and
 * `CustomEvent` (extends Event, adds `detail`).
 *
 * These are standalone (no DOM tree), so dispatch fires the target's own
 * listeners at the AT_TARGET phase - exactly Node's EventTarget semantics.
 * AbortSignal (native_AbortController) chains its prototype to the EventTarget
 * prototype returned by native_EventTarget_prototype(). */
void reg_native_EventTarget(vm_t* vm);

/* The EventTarget class var (bound on vm->root by reg_native_EventTarget), or
 * NULL if not yet registered. Lets other builtins chain prototypes to it. */
var_t* native_EventTarget_class(vm_t* vm);

/* Dispatch `event` on `target` from C (sets target/currentTarget/eventPhase,
 * fires addEventListener listeners then the on<type> handler, honours
 * stopImmediatePropagation and `once`). Used by AbortController.abort(). */
void native_EventTarget_dispatch(vm_t* vm, var_t* target, var_t* event);

/* Build a plain `Event` instance of the given type from C (returned with the
 * usual var-returning-native refcount contract). */
var_t* native_Event_new(vm_t* vm, const char* type);

/* Register a listener on `target` from C (capture=false). Used by
 * AbortSignal.any() to wire source signals to the composite. */
void native_EventTarget_add_listener(vm_t* vm, var_t* target, const char* type, var_t* cb, bool once);

#ifdef __cplusplus
}
#endif

#endif
