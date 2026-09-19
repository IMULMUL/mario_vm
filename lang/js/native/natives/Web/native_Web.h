#ifndef MARIO_NATIVE_WEB
#define MARIO_NATIVE_WEB

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registers the shared Web/Node globals that are otherwise missing from the
 * standalone build:
 *
 *   - structuredClone(value, options): a deep clone of the structured-cloneable
 *     subset (primitives, Array, plain Object, Date, RegExp, Map, Set) with
 *     cycle preservation; functions/symbols and non-cloneable hosts raise a
 *     DataCloneError.
 *   - crypto.getRandomValues(a) / crypto.randomUUID(): the WebCrypto surface
 *     both Node and the browser expose.
 *   - performance.now() / performance.timeOrigin (+ mark/measure stubs).
 *
 * In the browser build jsnative/natives/js_web.c republishes crypto and
 * performance onto vm->root during page init (after vm_init), cleanly replacing
 * these; structuredClone is not provided there, so it stays additive. Neither
 * crypto nor performance nor structuredClone is a DOM class, so this must be
 * registered from the shared reg_builtin_natives chain (natives_builtin.c). */
void reg_native_Web(vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif
