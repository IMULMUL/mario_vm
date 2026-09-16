#ifndef MARIO_TYPEDARRAY
#define MARIO_TYPEDARRAY

#include "mario.h"

/* Registers the 11 concrete TypedArray constructors (Int8Array .. BigUint64Array)
 * with their shared prototype surface. Each constructor carries its TA_* element
 * type code in func->data, so one handler set serves all 11. */
void reg_native_TypedArray(vm_t* vm);

/* Build a fresh TypedArray of element type `et` (e.g. TA_UINT8) over a NEW
 * ArrayBuffer holding a copy of bytes[0..len). Returns refs=0; the caller adopts
 * it as a return value (or roots it) before further allocation. */
var_t* native_TypedArray_from_bytes(vm_t* vm, int et, const uint8_t* bytes, int64_t len);

#endif
