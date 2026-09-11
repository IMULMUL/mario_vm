#ifdef __cplusplus
extern "C" {
#endif

#include "native_Map.h"
#include <string.h>

#define CLS_MAP "Map"

/* Map stores its entries in a C struct kept in this->value.
 * keys/vals are parallel dynamic arrays preserving insertion order.
 * Each key/val holds one owning reference; the "size" member mirrors
 * md->size so that `map.size` resolves as a normal property. */
typedef struct {
    var_t**  keys;
    var_t**  vals;
    uint32_t size;
    uint32_t cap;
} map_data;

static void map_free(void* p) {
    map_data* md = (map_data*)p;
    if (md == NULL) return;
    for (uint32_t i = 0; i < md->size; ++i) {
        var_unref(md->keys[i]);
        var_unref(md->vals[i]);
    }
    if (md->keys) mario_free(md->keys);
    if (md->vals) mario_free(md->vals);
    mario_free(md);
}

static map_data* get_map(var_t* this_v) {
    if (this_v == NULL) return NULL;
    map_data* md = (map_data*)this_v->value;
    if (md == NULL) {
        md = (map_data*)mario_malloc(sizeof(map_data));
        md->keys = NULL; md->vals = NULL; md->size = 0; md->cap = 0;
        this_v->value = md;
        this_v->free_func = map_free;
    }
    return md;
}

static void map_grow(map_data* md) {
    uint32_t newcap = md->cap ? md->cap * 2 : 8;
    var_t** nk = (var_t**)mario_malloc(sizeof(var_t*) * newcap);
    var_t** nv = (var_t**)mario_malloc(sizeof(var_t*) * newcap);
    for (uint32_t i = 0; i < md->size; ++i) { nk[i] = md->keys[i]; nv[i] = md->vals[i]; }
    if (md->keys) mario_free(md->keys);
    if (md->vals) mario_free(md->vals);
    md->keys = nk; md->vals = nv; md->cap = newcap;
}

static bool map_key_eq(var_t* a, var_t* b) {
    if (a == b) return true;
    if (a == NULL || b == NULL) return false;
    bool an = (a->type == V_INT || a->type == V_FLOAT || a->type == V_BOOL);
    bool bn = (b->type == V_INT || b->type == V_FLOAT || b->type == V_BOOL);
    if (an && bn) return var_get_float(a) == var_get_float(b);
    if (a->type != b->type) return false;
    switch (a->type) {
        case V_STRING: {
            const char* sa = var_get_str(a);
            const char* sb = var_get_str(b);
            return sa && sb && strcmp(sa, sb) == 0;
        }
        case V_NULL:
        case V_UNDEF:
            return true;
        default:
            return false; /* objects/functions: identity only (a==b handled above) */
    }
}

static int32_t map_find(map_data* md, var_t* key) {
    for (uint32_t i = 0; i < md->size; ++i)
        if (map_key_eq(md->keys[i], key)) return (int32_t)i;
    return -1;
}

static void map_sync_size(var_t* this_v, map_data* md) {
    var_t* szm = var_find_own_member_var(this_v, "size");
    if (szm != NULL) var_set_int(szm, (int)md->size);
}

/* core insert/update; takes owning refs on key & value only when new */
static void map_set_impl(var_t* this_v, map_data* md, var_t* key, var_t* value) {
    int32_t idx = map_find(md, key);
    if (idx >= 0) {
        var_t* old = md->vals[idx];
        md->vals[idx] = var_ref(value);
        var_unref(old);
        return;
    }
    if (md->size >= md->cap) map_grow(md);
    md->keys[md->size] = var_ref(key);
    md->vals[md->size] = var_ref(value);
    md->size++;
    map_sync_size(this_v, md);
}

var_t* native_Map_constructor(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = (map_data*)mario_malloc(sizeof(map_data));
    md->keys = NULL; md->vals = NULL; md->size = 0; md->cap = 0;
    this_v->value = md;
    this_v->free_func = map_free;

    var_t* sz = var_new_int(vm, 0);
    node_t* sn = var_add(this_v, "size", sz);
    sn->be_unenumerable = 1;

    /* optional iterable of [key, value] pairs */
    var_t* iter = get_obj(env, "iterable");
    if (iter != NULL && iter->type == V_OBJECT && iter->is_array) {
        uint32_t n = var_array_size(iter);
        for (uint32_t i = 0; i < n; ++i) {
            var_t* pair = var_array_get_var(iter, i);
            if (pair != NULL && pair->is_array && var_array_size(pair) >= 2) {
                var_t* k = var_array_get_var(pair, 0);
                var_t* v = var_array_get_var(pair, 1);
                map_set_impl(this_v, md, k, v);
            }
        }
    }
    return this_v;
}

var_t* native_Map_set(vm_t* vm, var_t* env, void* data) {
    (void)vm; (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* key = get_obj(env, "key");
    var_t* value = get_obj(env, "value");
    map_set_impl(this_v, md, key, value);
    return this_v;
}

var_t* native_Map_get(vm_t* vm, var_t* env, void* data) {
    (void)vm; (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* key = get_obj(env, "key");
    int32_t idx = map_find(md, key);
    if (idx < 0) return NULL; /* undefined */
    return md->vals[idx];     /* borrowed; func_call adds the stack ref */
}

var_t* native_Map_has(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* key = get_obj(env, "key");
    return var_new_bool(vm, map_find(md, key) >= 0);
}

var_t* native_Map_delete(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* key = get_obj(env, "key");
    int32_t idx = map_find(md, key);
    if (idx < 0) return var_new_bool(vm, false);
    var_unref(md->keys[idx]);
    var_unref(md->vals[idx]);
    for (uint32_t i = (uint32_t)idx; i + 1 < md->size; ++i) {
        md->keys[i] = md->keys[i + 1];
        md->vals[i] = md->vals[i + 1];
    }
    md->size--;
    map_sync_size(this_v, md);
    return var_new_bool(vm, true);
}

var_t* native_Map_clear(vm_t* vm, var_t* env, void* data) {
    (void)vm; (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    for (uint32_t i = 0; i < md->size; ++i) {
        var_unref(md->keys[i]);
        var_unref(md->vals[i]);
    }
    md->size = 0;
    map_sync_size(this_v, md);
    return NULL;
}

var_t* native_Map_forEach(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* cb = get_obj(env, "callback");
    if (cb == NULL || cb->type == V_UNDEF) return NULL;
    for (uint32_t i = 0; i < md->size; ++i) {
        var_t* args = var_new_array(vm);
        var_array_add(args, md->vals[i]);
        var_array_add(args, md->keys[i]);
        var_array_add(args, this_v);
        var_array_reverse(args);
        var_t* res = call_m_func(vm, this_v, cb, args);
        var_unref(args);
        if (res != NULL) var_unref(res);
    }
    return NULL;
}

var_t* native_Map_keys(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* arr = var_new_array(vm);
    for (uint32_t i = 0; i < md->size; ++i)
        var_array_add(arr, md->keys[i]);
    return arr;
}

var_t* native_Map_values(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* arr = var_new_array(vm);
    for (uint32_t i = 0; i < md->size; ++i)
        var_array_add(arr, md->vals[i]);
    return arr;
}

var_t* native_Map_entries(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* this_v = get_obj(env, THIS);
    map_data* md = get_map(this_v);
    var_t* arr = var_new_array(vm);
    for (uint32_t i = 0; i < md->size; ++i) {
        var_t* pair = var_new_array(vm);
        var_array_add(pair, md->keys[i]);
        var_array_add(pair, md->vals[i]);
        var_array_add(arr, pair); // arr's node takes the only ref to pair
    }
    return arr;
}

void reg_native_Map(vm_t* vm) {
    var_t* cls = vm_new_class(vm, CLS_MAP);
    vm_reg_native(vm, cls, "constructor(iterable)", native_Map_constructor, NULL);
    vm_reg_native(vm, cls, "set(key, value)", native_Map_set, NULL);
    vm_reg_native(vm, cls, "get(key)", native_Map_get, NULL);
    vm_reg_native(vm, cls, "has(key)", native_Map_has, NULL);
    vm_reg_native(vm, cls, "delete(key)", native_Map_delete, NULL);
    vm_reg_native(vm, cls, "clear()", native_Map_clear, NULL);
    vm_reg_native(vm, cls, "forEach(callback)", native_Map_forEach, NULL);
    vm_reg_native(vm, cls, "keys()", native_Map_keys, NULL);
    vm_reg_native(vm, cls, "values()", native_Map_values, NULL);
    vm_reg_native(vm, cls, "entries()", native_Map_entries, NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
