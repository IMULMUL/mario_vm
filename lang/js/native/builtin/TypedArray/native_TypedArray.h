#ifndef MARIO_TYPEDARRAY
#define MARIO_TYPEDARRAY

#include "mario.h"

/* Registers the 11 concrete TypedArray constructors (Int8Array .. BigUint64Array)
 * with their shared prototype surface. Each constructor carries its TA_* element
 * type code in func->data, so one handler set serves all 11. */
void reg_native_TypedArray(vm_t* vm);

#endif
