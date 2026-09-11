#ifdef __cplusplus
extern "C" {
#endif

#include "native_Symbol.h"
#include <string.h>
#include <stdio.h>

/* ES6 Symbol.
 *
 * A symbol instance is a V_OBJECT whose prototype is the internal "_Symbol_"
 * prototype (providing toString). Two hidden own members carry its identity:
 *   "description" - the human readable description
 *   "@@symkey"    - the unique property-key string the symbol maps to
 * var_is_symbol()/var_symbol_key() (mario.c) read these, so a symbol can be
 * used as an object key via `{[sym]: v}` / `obj[sym]`.
 *
 * Well-known symbols (Symbol.iterator, ...) get fixed keys ("@@S:iterator");
 * user symbols get unique keys ("@@S:<desc>#<id>"); Symbol.for() keys are
 * "@@S:for:<key>" so the registry can look them up deterministically. */

static uint32_t g_sym_id = 0;

static var_t* make_symbol(vm_t* vm, var_t* proto, const char* key, const char* desc) {
    var_t* sym = var_new_obj(vm, proto, NULL, NULL);
    var_t* d = var_new_str(vm, desc ? desc : "");
    node_t* dn = var_add(sym, "description", d);
    if (dn != NULL) dn->be_unenumerable = 1;
    var_t* k = var_new_str(vm, key);
    node_t* kn = var_add(sym, SYM_MARKER, k);
    if (kn != NULL) { kn->be_unenumerable = 1; kn->invisable = 1; }
    return sym;
}

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
    char key[96];
    snprintf(key, sizeof(key), "%s%s#%u", SYMKEY_PREFIX, desc ? desc : "", g_sym_id++);
    return make_symbol(vm, proto, key, desc); /* refs=0, owned by caller */
}

static var_t* get_registry(vm_t* vm) {
    var_t* symfn = var_find_own_member_var(vm->root, "Symbol");
    if (symfn == NULL) return NULL;
    return var_find_own_member_var(symfn, "@@registry");
}

var_t* native_Symbol_for(vm_t* vm, var_t* env, void* data) {
    var_t* proto = (var_t*)data;
    var_t* kn = get_obj(env, "key");
    const char* k = (kn != NULL) ? var_get_str(kn) : "";
    char key[96];
    snprintf(key, sizeof(key), "%sfor:%s", SYMKEY_PREFIX, k ? k : "");
    var_t* registry = get_registry(vm);
    if (registry == NULL) return make_symbol(vm, proto, key, k);
    var_t* existing = var_find_own_member_var(registry, key);
    if (existing != NULL) return existing; /* borrowed; func_call adds the ref */
    var_t* s = make_symbol(vm, proto, key, k);
    var_add(registry, key, s); /* registry keeps it alive */
    return s;
}

var_t* native_Symbol_keyFor(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* s = get_obj(env, "symbol");
    if (s == NULL || !var_is_symbol(s)) return NULL; /* undefined */
    const char* key = var_symbol_key(s);
    const char* prefix = SYMKEY_PREFIX "for:";
    if (key != NULL && strncmp(key, prefix, strlen(prefix)) == 0) {
        var_t* d = var_find_own_member_var(s, "description");
        if (d != NULL) return d; /* borrowed */
    }
    return NULL; /* undefined */
}

void reg_native_Symbol(vm_t* vm) {
    /* Internal class provides the shared prototype for symbol instances. */
    var_t* cls = vm_new_class(vm, "_Symbol_");
    vm_reg_native(vm, cls, "toString()", native_Symbol_toString, NULL);
    var_t* proto = var_get_prototype(cls);

    /* Global callable Symbol(desc); capture its var so well-known symbols and
     * the registry can hang off it as own members. */
    node_t* symnode = vm_reg_native(vm, NULL, "Symbol(desc)", native_Symbol_call, proto);
    var_t* symfn = (symnode != NULL) ? symnode->var : NULL;
    if (symfn == NULL) return;

    /* registry backing Symbol.for()/keyFor(); rooted via the Symbol function. */
    var_t* registry = var_new_obj_no_proto(vm, NULL, NULL);
    node_t* rn = var_add(symfn, "@@registry", registry);
    if (rn != NULL) { rn->be_unenumerable = 1; rn->invisable = 1; }

    /* Well-known symbols as statics: Symbol.iterator etc. */
    struct { const char* name; const char* key; } wk[] = {
        { "iterator", SYMKEY_ITERATOR },
        { "asyncIterator", SYMKEY_ASYNCITERATOR },
        { "toStringTag", SYMKEY_TOSTRINGTAG },
        { "toPrimitive", SYMKEY_TOPRIMITIVE },
        { NULL, NULL }
    };
    for (int i = 0; wk[i].name != NULL; ++i) {
        char desc[64];
        snprintf(desc, sizeof(desc), "Symbol.%s", wk[i].name);
        var_t* s = make_symbol(vm, proto, wk[i].key, desc);
        node_t* n = var_add(symfn, wk[i].name, s);
        if (n != NULL) { n->be_unenumerable = 1; n->be_const = 1; }
    }

    vm_reg_native_on(vm, symfn, "for(key)", native_Symbol_for, proto);
    vm_reg_native_on(vm, symfn, "keyFor(symbol)", native_Symbol_keyFor, proto);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
