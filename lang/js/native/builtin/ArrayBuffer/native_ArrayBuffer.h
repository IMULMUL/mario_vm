#ifndef MARIO_ARRAYBUFFER
#define MARIO_ARRAYBUFFER

#include "mario.h"

void reg_native_ArrayBuffer(vm_t* vm);

/* Create a fresh zero-filled ArrayBuffer object of `byteLength` bytes. Used by
 * the TypedArray constructors (Phase 4) to allocate their backing store. The
 * returned var has refs=0; the caller must root it (var_ref, or attach it as a
 * member) before any further allocation that could trigger a GC. */
var_t* native_ArrayBuffer_new(vm_t* vm, uint32_t byteLength);

#endif
