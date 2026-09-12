#ifndef MARIO_SHAREDARRAYBUFFER
#define MARIO_SHAREDARRAYBUFFER

#include "mario.h"

void reg_native_SharedArrayBuffer(vm_t* vm);

/* Create a fresh zero-filled SharedArrayBuffer of `byteLength` bytes. Identical
 * in layout to an ArrayBuffer (the var IS its byte holder: value=bytes,
 * size=byteLength) except for the @@exotic="shared" marker, which lets
 * var_is_arraybuffer() accept it so DataView/TypedArray views can be built over
 * it unchanged. The returned var has refs=0; the caller must root it before any
 * further allocation that could trigger a GC. */
var_t* native_SharedArrayBuffer_new(vm_t* vm, uint32_t byteLength);

#endif
