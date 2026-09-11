#ifndef MARIO_NATIVE_SYMBOL
#define MARIO_NATIVE_SYMBOL

#include "mario.h"

void reg_native_Symbol(vm_t* vm);

/* Resolve the canonical symbol object for a property-key string ("@@S:...").
 * Borrowed ref (owned by the symbol registry) or NULL. Used by
 * Object.getOwnPropertySymbols to map symbol-keyed members back to symbols. */
var_t* symbol_lookup_by_key(vm_t* vm, const char* key);

#endif
