#ifdef __cplusplus
extern "C" {
#endif

#include "native_Symbol.h"
#include <string.h>

/* Simplified Symbol:
 *   Symbol(desc) -> a unique object carrying a `description`.
 * Symbols are identity-unique (each call returns a distinct object),
 * expose `.description` and `.toString()` -> "Symbol(desc)".
 * The shared prototype (with toString) is created from an internal class
 * "_Symbol_" and passed to the global Symbol() function as native data. */

var_t* native_Symbol_toString(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    const char* desc = "";
    if (this_v != NULL) {
        var_t* d = var_find_own_member_var(this_v, "description");
        if (d != NULL) desc = var_get_str(d);
    }
    mstr_t* s = mstr_new("Symbol(");
    if (desc != NULL) mstr_append(s, desc);
    mstr_add(s, ')');
    var_t* r = var_new_str(vm, s->cstr);
    mstr_free(s);
    return r;
}

var_t* native_Symbol_call(vm_t* vm, var_t* env, void* data) {
    var_t* proto = (var_t*)data;
    const char* desc = get_str(env, "desc");
    var_t* sym = var_new_obj(vm, proto, NULL, NULL); /* refs=0, owned by caller */
    var_t* d = var_new_str(vm, desc ? desc : "");
    var_add(sym, "description", d);
    return sym;
}

void reg_native_Symbol(vm_t* vm) {
    /* Internal class provides the shared prototype for symbol instances. */
    var_t* cls = vm_new_class(vm, "_Symbol_");
    vm_reg_native(vm, cls, "toString()", native_Symbol_toString, NULL);
    var_t* proto = var_get_prototype(cls);
    /* Global callable Symbol(desc); `proto` travels as native data. */
    vm_reg_native(vm, NULL, "Symbol(desc)", native_Symbol_call, proto);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
