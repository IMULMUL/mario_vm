#ifndef MARIO_NATIVE_BUFFER
#define MARIO_NATIVE_BUFFER

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registers the Node.js `Buffer` global. A mario Buffer instance IS a Uint8Array
 * (created via native_TypedArray_from_bytes) carrying a hidden @@is_buffer marker
 * and an own toString(encoding) that supports utf8 / latin1 / hex / base64, so it
 * inherits the full TypedArray surface (indexing, length, slice, subarray, ...)
 * exactly like the real thing while adding the Node-specific text codecs.
 * Static surface: from / alloc / allocUnsafe / isBuffer / byteLength / concat /
 * isEncoding. */
void reg_native_Buffer(vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif
