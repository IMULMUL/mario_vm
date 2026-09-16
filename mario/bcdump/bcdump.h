#ifndef MARIO_BC_DUMP_H
#define MARIO_BC_DUMP_H

#include "mario.h"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

mstr_t* bc_dump(bytecode_t* bc);

/* TEMP taobao diag: print `radius` instructions around `center` to stderr. */
void bc_dump_window(bytecode_t* bc, PC center, PC radius);

#ifdef __cplusplus /* __cplusplus */
}
#endif

#endif