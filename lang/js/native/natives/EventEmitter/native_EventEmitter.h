#ifndef MARIO_NATIVE_EVENTEMITTER
#define MARIO_NATIVE_EVENTEMITTER

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registers Node's `EventEmitter` (the core of the `events` module) as a global
 * class. Since the standalone engine exposes no require()/module loader, the
 * class is published directly on vm->root by vm_new_class, so scripts can do
 * `new EventEmitter()` - the same shape Node offers via require('events').
 *
 * Surface: on/addListener, once, off/removeListener, removeAllListeners, emit,
 * listenerCount, listeners, rawListeners, eventNames, prependListener,
 * prependOnceListener, setMaxListeners/getMaxListeners, plus the
 * EventEmitter.listenerCount(emitter, type) static and defaultMaxListeners.
 *
 * Listeners live in a hidden "@@events" object keyed by event name; each entry
 * is an array of {cb, once} records. emit() fires a snapshot (so a listener that
 * mutates the list is safe) with `this` bound to the emitter and returns whether
 * any listener ran - matching Node's boolean contract. This is distinct from the
 * DOM EventTarget (native_EventTarget.c): EventEmitter passes the emit() args
 * straight through, whereas EventTarget dispatches an Event object. */
void reg_native_EventEmitter(vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif
