#ifndef MARIO_ATOMICS
#define MARIO_ATOMICS

#include "mario.h"

/* Registers the Atomics namespace object (Atomics.load/store/add/... ) with its
 * full static surface. Single-threaded engine: the read-modify-write ops are
 * exact synchronous accesses, wait() refuses (a lone agent may not block) and
 * notify() always reports zero woken waiters. See native_Atomics.c. */
void reg_native_Atomics(vm_t* vm);

#endif
