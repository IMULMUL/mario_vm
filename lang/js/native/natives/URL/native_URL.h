#ifndef MARIO_NATIVE_URL
#define MARIO_NATIVE_URL

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registers the WHATWG `URL` and `URLSearchParams` globals. URL parses an
 * absolute URL (or a relative one resolved against a base) into its components,
 * exposed as own string members (href/protocol/host/hostname/port/pathname/
 * search/hash/origin/username/password) plus a live `searchParams`
 * URLSearchParams. URLSearchParams parses/serialises
 * application/x-www-form-urlencoded with get/getAll/set/append/delete/has/sort/
 * toString/forEach/entries/keys/values. */
void reg_native_URL(vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif
