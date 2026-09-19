#ifndef MARIO_BC_DUMP_H
#define MARIO_BC_DUMP_H

#include "mario.h"
#include <stdio.h>

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

mstr_t* bc_dump(bytecode_t* bc);

/* Streaming dump for big bundles: bc_dump()'s single mstr_t caps at 64KB
 * (16-bit len bit-field) and corrupts multi-MB disassembly. */
void bc_dump_file(bytecode_t* bc, FILE* f);

/* TEMP taobao diag: print `radius` instructions around `center` to stderr. */
void bc_dump_window(bytecode_t* bc, PC center, PC radius);

#ifdef __cplusplus /* __cplusplus */
}
#endif

#endif