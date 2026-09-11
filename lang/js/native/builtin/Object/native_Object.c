#ifdef __cplusplus
extern "C" {
#endif

#include "native_Object.h"
#include <stdio.h>

/** Object */

/* Defined below (ES6 statics section); forward-declared for native_Object_keys. */
static void var_own_keys(vm_t* vm, var_t* var, var_t* keys_var, bool enum_only);

/* From native_Symbol.c: resolve a "@@S:..." property-key string back to its
 * canonical symbol object (borrowed ref). Used by getOwnPropertySymbols. */
extern var_t* symbol_lookup_by_key(vm_t* vm, const char* key);

var_t* native_Object_create(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* proto = get_obj(env, "proto");
	var_t* ret = var_new_obj_no_proto(vm, NULL, NULL);
	var_set_prototype(ret, proto);
	return ret;
}

var_t* native_Object_getPrototypeOf(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* obj = get_obj(env, "obj");
	return var_get_prototype(obj);
}

var_t* native_Object_hasOwnProperty(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_obj(env, THIS);
	const char* name = get_str(env, "name");
	node_t* n = var_find_own_member(obj, name);
	return var_new_bool(vm, (n != NULL && n->be_inherited == 0 && n->invisable == 0));
}

// Callback function for hash_map_iterate in var_properties_num
typedef struct {
	vm_t* vm;
	var_t* var;
	var_t* keys_var;
	bool enumerable;
	uint32_t num;
	hash_map_t* seen_properties; // 用于快速检查属性是否已经存在的哈希表
} properties_callback_data;

static void properties_callback(const char* key, void* value, void* user_data) {
	properties_callback_data* data = (properties_callback_data*)user_data;
	node_t* node = (node_t*)value;
	
	if(node != NULL && 
		node->be_inherited == 0 &&
		node->invisable == 0 &&
		node->var != data->keys_var) {
		if(!node->be_unenumerable || !data->enumerable) {
			// 使用哈希表快速检查属性是否已经存在
			if(hash_map_get(data->seen_properties, node->name) == NULL) {
				// 将属性添加到哈希表中，标记为已存在
				hash_map_add(data->seen_properties, node->name, (void*)"");
				// 将属性名称添加到结果数组中
				var_array_add(data->keys_var, var_new_str(data->vm, node->name));
				data->num++;
			}
		}
	}
}

static inline uint32_t var_properties_num(vm_t* vm, var_t* var, var_t* keys_var, bool enumerable) {
	uint32_t num = 0;

	/* keys_var and the transient var_new_str() results are held only by C locals
	 * here, so they are invisible to gc(): an allocation-driven collection in the
	 * middle of the walk would sweep keys_var (V_ST_GC, unreachable from a root)
	 * and corrupt the prototype-chain traversal into an infinite loop. Defer gc
	 * until the result array is fully built and handed back to the caller. */
	vm->gc.gc_defer++;

	// 创建并初始化哈希表，用于快速检查属性是否已经存在
	hash_map_t* seen_properties = hash_map_new();
	
	// Use hash_map_iterate to traverse the properties
	properties_callback_data data;
	data.vm = vm;
	data.var = var;
	data.keys_var = keys_var;
	data.enumerable = enumerable;
	data.num = 0;
	data.seen_properties = seen_properties;
	
	hash_map_iterate(&var->children, properties_callback, &data);
	num += data.num;

	// 处理原型链上的属性
	var_t* proto = var_get_prototype(var);
	while(proto != NULL) {
		// 递归调用时传递同一个哈希表，确保整个原型链上的属性都不重复
		properties_callback_data proto_data;
		proto_data.vm = vm;
		proto_data.var = proto;
		proto_data.keys_var = keys_var;
		proto_data.enumerable = enumerable;
		proto_data.num = 0;
		proto_data.seen_properties = seen_properties;
		
		hash_map_iterate(&proto->children, properties_callback, &proto_data);
		num += proto_data.num;
		
		proto = var_get_prototype(proto);
	}
	
	// 释放哈希表
	hash_map_free(seen_properties, mario_free, NULL);

	vm->gc.gc_defer--;

	return num;
}

var_t* native_Object_keys(vm_t* vm, var_t* env, void* data) {
	(void)data;
	/* Object.keys(obj) is a static: the target is argument 0, NOT `this` (which
	 * is the Object constructor). Using THIS enumerated Object's own (unenumerable
	 * statics) members and returned []. Match native_Object_values/entries: own
	 * enumerable string keys only, no prototype chain. */
	var_t* obj = get_func_arg(env, 0);
	var_t* keys = var_new_array(vm);
	var_own_keys(vm, obj, keys, true);
	return keys;
}

/* for-in enumeration helper `__enum_keys(obj)`. Arrays yield their indices as
 * strings ("0","1",...) first, matching JS for-in over arrays; then own and
 * inherited enumerable string keys are appended (deduplicated) via
 * var_properties_num. Returns a fresh array (refs=0). */
var_t* native_enum_keys(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* keys = var_new_array(vm);
	if(obj != NULL && obj->type == V_OBJECT) {
		vm->gc.gc_defer++; /* keys unrooted while we build it */
		if(obj->is_array) {
			uint32_t n = var_array_size(obj);
			for(uint32_t i = 0; i < n; i++) {
				char buf[24];
				snprintf(buf, sizeof(buf), "%u", i);
				var_array_add(keys, var_new_str(vm, buf));
			}
		}
		var_properties_num(vm, obj, keys, true);
		vm->gc.gc_defer--;
	}
	return keys;
}

var_t* native_Object_defineProperty(vm_t* vm, var_t* env, void* data) {
	var_t* obj = get_obj(env, "obj");
	const char* name = get_str(env, "name");
	var_t* descriptor = get_obj(env, "descriptor");
	var_t* v = var_find_own_member_var(descriptor, "value");
	node_t* node = var_add(obj, name, v);

	v = var_find_own_member_var(descriptor, "writable");
	if(v != NULL)
		node->be_const = !var_get_bool(v);

	v = var_find_own_member_var(descriptor, "enumerable");
	if(v != NULL)
		node->be_unenumerable = !var_get_bool(v);

	v = var_find_own_member_var(descriptor, "configurable");
	if(v != NULL)
		node->be_const = !var_get_bool(v);
	return NULL;
}

/* ---- ES6+ Object statics ---- */

/* Collect OWN property keys (no prototype chain). When enum_only is true,
 * non-enumerable members are skipped. */
typedef struct {
	vm_t* vm;
	var_t* keys_var;
	bool enum_only;
	hash_map_t* seen;
} own_keys_cb_data;

static void own_keys_cb(const char* key, void* value, void* user_data) {
	(void)key;
	own_keys_cb_data* d = (own_keys_cb_data*)user_data;
	node_t* node = (node_t*)value;
	if(node == NULL || node->be_inherited || node->invisable)
		return;
	if(d->enum_only && node->be_unenumerable)
		return;
	if(hash_map_get(d->seen, node->name) != NULL)
		return;
	hash_map_add(d->seen, node->name, (void*)"");
	var_array_add(d->keys_var, var_new_str(d->vm, node->name));
}

static void var_own_keys(vm_t* vm, var_t* var, var_t* keys_var, bool enum_only) {
	if(var == NULL)
		return;
	vm->gc.gc_defer++; // keys_var is unrooted here; see var_properties_num().
	hash_map_t* seen = hash_map_new();
	own_keys_cb_data d;
	d.vm = vm;
	d.keys_var = keys_var;
	d.enum_only = enum_only;
	d.seen = seen;
	hash_map_iterate(&var->children, own_keys_cb, &d);
	hash_map_free(seen, mario_free, NULL);
	vm->gc.gc_defer--;
}

static bool var_is_nan(var_t* v) {
	if(v == NULL || v->value == NULL)
		return false;
	if(v->type == V_FLOAT) {
		float f = *(float*)v->value;
		return f != f;
	}
	if(v->type == V_FLOAT64) {
		double d = *(double*)v->value;
		return d != d;
	}
	return false;
}

static var_t* new_plain_obj(vm_t* vm) {
	return var_new_obj(vm, var_get_prototype(vm->builtin_vars.var_Object), NULL, NULL);
}

var_t* native_Object_assign(vm_t* vm, var_t* env, void* data) {
	(void)data;
	uint32_t argc = get_func_args_num(env);
	var_t* target = argc > 0 ? get_func_arg(env, 0) : NULL;
	if(target == NULL)
		return var_new(vm);
	uint32_t i;
	for(i = 1; i < argc; i++) {
		var_t* src = get_func_arg(env, i);
		if(src == NULL)
			continue;
		var_t* keys = var_new_array(vm);
		var_own_keys(vm, src, keys, true);
		uint32_t sz = var_array_size(keys);
		uint32_t j;
		for(j = 0; j < sz; j++) {
			var_t* kv = var_array_get_var(keys, (int32_t)j);
			const char* k = var_get_str(kv);
			var_t* v = var_find_own_member_var(src, k);
			if(v != NULL)
				var_add(target, k, v);
		}
		var_unref(keys);
	}
	return target;
}

var_t* native_Object_is(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* a = get_func_arg(env, 0);
	var_t* b = get_func_arg(env, 1);
	bool an = var_is_nan(a), bn = var_is_nan(b);
	if(an && bn)
		return var_new_bool(vm, true);
	if(an != bn)
		return var_new_bool(vm, false);
	/* +0 !== -0 (checked per matching float tag, before the generic compare). */
	if(a != NULL && b != NULL && a->value != NULL && b->value != NULL &&
			a->type == b->type) {
		if(a->type == V_FLOAT) {
			float fa = *(float*)a->value, fb = *(float*)b->value;
			if(fa == 0.0f && fb == 0.0f)
				/* 1/+0 == +inf, 1/-0 == -inf: distinguishes the two zeros. */
				return var_new_bool(vm, (1.0f / fa) == (1.0f / fb));
		}
		else if(a->type == V_FLOAT64) {
			double da = *(double*)a->value, db = *(double*)b->value;
			if(da == 0.0 && db == 0.0)
				return var_new_bool(vm, (1.0 / da) == (1.0 / db));
		}
	}
	if(a == NULL || b == NULL)
		return var_new_bool(vm, a == b);
	if(a->type != b->type)
		return var_new_bool(vm, false);
	switch(a->type) {
		case V_INT: return var_new_bool(vm, *(int*)a->value == *(int*)b->value);
		case V_INT64: return var_new_bool(vm, *(int64_t*)a->value == *(int64_t*)b->value);
		case V_FLOAT: return var_new_bool(vm, *(float*)a->value == *(float*)b->value);
		case V_FLOAT64: return var_new_bool(vm, *(double*)a->value == *(double*)b->value);
		case V_BOOL: return var_new_bool(vm, var_get_bool(a) == var_get_bool(b));
		case V_STRING: return var_new_bool(vm, strcmp(var_get_str(a), var_get_str(b)) == 0);
		/* undefined and null are singletons by value: two distinct var_t's of the
		 * same type are still Object.is-equal. Without these cases they fell to the
		 * default pointer compare and Object.is(undefined, undefined) was false,
		 * breaking every eq(x, undefined) assertion. Types already match here. */
		case V_UNDEF:
		case V_NULL: return var_new_bool(vm, true);
		default: return var_new_bool(vm, a == b);
	}
}

var_t* native_Object_values(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* ret = var_new_array(vm);
	var_t* keys = var_new_array(vm);
	var_own_keys(vm, obj, keys, true);
	uint32_t sz = var_array_size(keys), j;
	for(j = 0; j < sz; j++) {
		const char* k = var_get_str(var_array_get_var(keys, (int32_t)j));
		var_t* v = var_find_own_member_var(obj, k);
		var_array_add(ret, v != NULL ? v : var_new(vm));
	}
	var_unref(keys);
	return ret;
}

var_t* native_Object_entries(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* ret = var_new_array(vm);
	var_t* keys = var_new_array(vm);
	var_own_keys(vm, obj, keys, true);
	uint32_t sz = var_array_size(keys), j;
	for(j = 0; j < sz; j++) {
		var_t* kv = var_array_get_var(keys, (int32_t)j);
		const char* k = var_get_str(kv);
		var_t* v = var_find_own_member_var(obj, k);
		var_t* pair = var_new_array(vm);
		var_array_add(pair, var_new_str(vm, k));
		var_array_add(pair, v != NULL ? v : var_new(vm));
		var_array_add(ret, pair);
	}
	var_unref(keys);
	return ret;
}

var_t* native_Object_fromEntries(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* arr = get_func_arg(env, 0);
	var_t* ret = new_plain_obj(vm);
	if(arr == NULL)
		return ret;
	uint32_t sz = var_array_size(arr), j;
	for(j = 0; j < sz; j++) {
		var_t* pair = var_array_get_var(arr, (int32_t)j);
		if(pair == NULL)
			continue;
		var_t* kv = var_array_get_var(pair, 0);
		var_t* vv = var_array_get_var(pair, 1);
		if(kv != NULL)
			var_add(ret, var_get_str(kv), vv != NULL ? vv : var_new(vm));
	}
	return ret;
}

var_t* native_Object_freeze(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	if(obj != NULL) {
		var_t* keys = var_new_array(vm);
		var_own_keys(vm, obj, keys, false);
		uint32_t sz = var_array_size(keys), j;
		for(j = 0; j < sz; j++) {
			const char* k = var_get_str(var_array_get_var(keys, (int32_t)j));
			node_t* n = var_find_own_member(obj, k);
			if(n != NULL)
				n->be_const = true;
		}
		var_unref(keys);
		node_t* mark = var_add(obj, "@frozen", var_new_bool(vm, true));
		if(mark != NULL)
			mark->invisable = 1;
	}
	return obj != NULL ? obj : var_new(vm);
}

var_t* native_Object_isFrozen(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* obj = get_func_arg(env, 0);
	node_t* n = obj != NULL ? var_find_own_member(obj, "@frozen") : NULL;
	return var_new_bool(vm, n != NULL);
}

var_t* native_Object_getOwnPropertyNames(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* ret = var_new_array(vm);
	var_own_keys(vm, obj, ret, false);
	return ret;
}

/* Object.getOwnPropertySymbols(obj): the symbol keys of obj's OWN properties.
 * A symbol-keyed member is stored under the symbol's "@@S:..." key string (see
 * handle_memberv / var_symbol_key) and marked non-enumerable; resolve each such
 * own member back to its canonical symbol object via the symbol registry. */
typedef struct {
	vm_t* vm;
	var_t* arr;
} own_syms_cb_data;

static void own_syms_cb(const char* key, void* value, void* user_data) {
	(void)key;
	own_syms_cb_data* d = (own_syms_cb_data*)user_data;
	node_t* node = (node_t*)value;
	if(node == NULL || node->be_inherited || node->invisable || node->name == NULL)
		return;
	if(strncmp(node->name, SYMKEY_PREFIX, strlen(SYMKEY_PREFIX)) != 0)
		return;
	var_t* sym = symbol_lookup_by_key(d->vm, node->name);
	if(sym != NULL)
		var_array_add(d->arr, sym); /* var_array_add (node_new) refs it */
}

var_t* native_Object_getOwnPropertySymbols(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* ret = var_new_array(vm);
	if(obj == NULL)
		return ret;
	vm->gc.gc_defer++; /* ret is unrooted here; see var_own_keys(). */
	own_syms_cb_data d;
	d.vm = vm;
	d.arr = ret;
	hash_map_iterate(&obj->children, own_syms_cb, &d);
	vm->gc.gc_defer--;
	return ret;
}

var_t* native_Object_getOwnPropertyDescriptor(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	const char* name = get_func_arg_str(env, 1);
	if(obj == NULL)
		return var_new(vm);
	node_t* n = var_find_own_member(obj, name);
	if(n == NULL || n->be_inherited)
		return var_new(vm);
	var_t* d = new_plain_obj(vm);
	var_add(d, "value", n->var != NULL ? n->var : var_new(vm));
	var_add(d, "writable", var_new_bool(vm, !n->be_const));
	var_add(d, "enumerable", var_new_bool(vm, !n->be_unenumerable));
	var_add(d, "configurable", var_new_bool(vm, !n->be_const));
	return d;
}

/* Object.hasOwn(obj, key): own-property check (no prototype chain), the modern
 * replacement for obj.hasOwnProperty(key). */
var_t* native_Object_hasOwn(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	const char* name = get_func_arg_str(env, 1);
	node_t* n = (obj != NULL) ? var_find_own_member(obj, name) : NULL;
	return var_new_bool(vm, n != NULL && n->be_inherited == 0 && n->invisable == 0);
}

/* Object.setPrototypeOf(obj, proto): re-point obj's [[Prototype]]; returns obj. */
var_t* native_Object_setPrototypeOf(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* proto = get_func_arg(env, 1);
	if(obj != NULL && proto != NULL)
		var_set_prototype(obj, proto);
	return obj != NULL ? obj : var_new(vm);
}

/* Object.getOwnPropertyDescriptors(obj): a plain object mapping every own key to
 * its {value,writable,enumerable,configurable} descriptor. */
var_t* native_Object_getOwnPropertyDescriptors(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* ret = new_plain_obj(vm);
	if(obj == NULL)
		return ret;
	var_t* keys = var_new_array(vm);
	var_own_keys(vm, obj, keys, false);
	uint32_t sz = var_array_size(keys), j;
	vm->gc.gc_defer++; /* ret/keys unrooted while we build them */
	for(j = 0; j < sz; j++) {
		const char* k = var_get_str(var_array_get_var(keys, (int32_t)j));
		node_t* n = var_find_own_member(obj, k);
		if(n == NULL || n->be_inherited)
			continue;
		var_t* d = new_plain_obj(vm);
		var_add(d, "value", n->var != NULL ? n->var : var_new(vm));
		var_add(d, "writable", var_new_bool(vm, !n->be_const));
		var_add(d, "enumerable", var_new_bool(vm, !n->be_unenumerable));
		var_add(d, "configurable", var_new_bool(vm, !n->be_const));
		var_add(ret, k, d);
	}
	vm->gc.gc_defer--;
	var_unref(keys);
	return ret;
}

/* Object.seal(obj) / isSealed: mark the object sealed. mario does not model
 * configurable-vs-writable separately, so seal is tracked with a hidden flag and
 * reported back by isSealed; it deliberately does NOT freeze values. */
var_t* native_Object_seal(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	if(obj != NULL) {
		node_t* mark = var_add(obj, "@sealed", var_new_bool(vm, true));
		if(mark != NULL)
			mark->invisable = 1;
	}
	return obj != NULL ? obj : var_new(vm);
}

var_t* native_Object_isSealed(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	node_t* n = (obj != NULL) ? var_find_own_member(obj, "@sealed") : NULL;
	return var_new_bool(vm, n != NULL);
}

/* Object.preventExtensions(obj) / isExtensible: hidden flag, mirrors seal. */
var_t* native_Object_preventExtensions(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	if(obj != NULL) {
		node_t* mark = var_add(obj, "@noext", var_new_bool(vm, true));
		if(mark != NULL)
			mark->invisable = 1;
	}
	return obj != NULL ? obj : var_new(vm);
}

var_t* native_Object_isExtensible(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	node_t* n = (obj != NULL) ? var_find_own_member(obj, "@noext") : NULL;
	return var_new_bool(vm, n == NULL);
}

/* Object.defineProperties(obj, descriptors): apply each own key of `descriptors`
 * through the same logic as defineProperty. */
var_t* native_Object_defineProperties(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_func_arg(env, 0);
	var_t* descriptors = get_func_arg(env, 1);
	if(obj == NULL)
		return var_new(vm);
	if(descriptors == NULL)
		return obj;
	var_t* keys = var_new_array(vm);
	var_own_keys(vm, descriptors, keys, false);
	uint32_t sz = var_array_size(keys), j;
	for(j = 0; j < sz; j++) {
		const char* k = var_get_str(var_array_get_var(keys, (int32_t)j));
		var_t* desc = var_find_own_member_var(descriptors, k);
		if(desc == NULL)
			continue;
		var_t* v = var_find_own_member_var(desc, "value");
		node_t* node = var_add(obj, k, v != NULL ? v : var_new(vm));
		v = var_find_own_member_var(desc, "writable");
		if(v != NULL)
			node->be_const = !var_get_bool(v);
		v = var_find_own_member_var(desc, "enumerable");
		if(v != NULL)
			node->be_unenumerable = !var_get_bool(v);
		v = var_find_own_member_var(desc, "configurable");
		if(v != NULL)
			node->be_const = !var_get_bool(v);
	}
	var_unref(keys);
	return obj;
}

/* __obj_rest(src, excludedKeysArray): internal helper for object-rest
 * destructuring `const { a, ...rest } = src`. Returns a new object holding
 * src's own enumerable properties whose key is not in excludedKeysArray. */
var_t* native_obj_rest(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* src = get_func_arg(env, 0);
	var_t* excluded = get_func_arg(env, 1);
	var_t* ret = new_plain_obj(vm);
	if(src == NULL)
		return ret;
	var_t* keys = var_new_array(vm);
	var_own_keys(vm, src, keys, true);
	uint32_t sz = var_array_size(keys), j;
	for(j = 0; j < sz; j++) {
		const char* k = var_get_str(var_array_get_var(keys, (int32_t)j));
		bool skip = false;
		if(excluded != NULL) {
			uint32_t esz = var_array_size(excluded), e;
			for(e = 0; e < esz; e++) {
				var_t* ev = var_array_get_var(excluded, (int32_t)e);
				if(ev != NULL && strcmp(var_get_str(ev), k) == 0) {
					skip = true;
					break;
				}
			}
		}
		if(!skip) {
			var_t* v = var_find_own_member_var(src, k);
			var_add(ret, k, v != NULL ? v : var_new(vm));
		}
	}
	var_unref(keys);
	return ret;
}

#define CLS_OBJECT "Object"

void reg_native_Object(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_OBJECT);
	vm_reg_static(vm, cls, "create(proto)", native_Object_create, NULL); 
	vm_reg_static(vm, cls, "getPrototypeOf(obj)", native_Object_getPrototypeOf, NULL); 
	vm_reg_static(vm, cls, "hasOwnProperty(name)", native_Object_hasOwnProperty, NULL); 
	vm_reg_static(vm, cls, "keys()", native_Object_keys, NULL); 
	vm_reg_static(vm, cls, "defineProperty(obj, name, descriptor)", native_Object_defineProperty, NULL); 
	vm_reg_static(vm, cls, "assign(target, s1)", native_Object_assign, NULL);
	vm_reg_static(vm, cls, "is(a, b)", native_Object_is, NULL);
	vm_reg_static(vm, cls, "values(obj)", native_Object_values, NULL);
	vm_reg_static(vm, cls, "entries(obj)", native_Object_entries, NULL);
	vm_reg_static(vm, cls, "fromEntries(arr)", native_Object_fromEntries, NULL);
	vm_reg_static(vm, cls, "freeze(obj)", native_Object_freeze, NULL);
	vm_reg_static(vm, cls, "isFrozen(obj)", native_Object_isFrozen, NULL);
	vm_reg_static(vm, cls, "getOwnPropertyNames(obj)", native_Object_getOwnPropertyNames, NULL);
	vm_reg_static(vm, cls, "getOwnPropertySymbols(obj)", native_Object_getOwnPropertySymbols, NULL);
	vm_reg_static(vm, cls, "getOwnPropertyDescriptor(obj, prop)", native_Object_getOwnPropertyDescriptor, NULL);
	vm_reg_static(vm, cls, "getOwnPropertyDescriptors(obj)", native_Object_getOwnPropertyDescriptors, NULL);
	vm_reg_static(vm, cls, "hasOwn(obj, key)", native_Object_hasOwn, NULL);
	vm_reg_static(vm, cls, "setPrototypeOf(obj, proto)", native_Object_setPrototypeOf, NULL);
	vm_reg_static(vm, cls, "defineProperties(obj, descriptors)", native_Object_defineProperties, NULL);
	vm_reg_static(vm, cls, "seal(obj)", native_Object_seal, NULL);
	vm_reg_static(vm, cls, "isSealed(obj)", native_Object_isSealed, NULL);
	vm_reg_static(vm, cls, "preventExtensions(obj)", native_Object_preventExtensions, NULL);
	vm_reg_static(vm, cls, "isExtensible(obj)", native_Object_isExtensible, NULL);
	vm_reg_native(vm, NULL, "__obj_rest(src, excluded)", native_obj_rest, NULL);
	/* for-in lowering (stmt_for_in) calls this by INSTR_CALL "__enum_keys$1". */
	vm_reg_native(vm, NULL, "__enum_keys(o)", native_enum_keys, NULL);
	/* globalThis: the global object itself. vm->root already holds every global
	 * (Object, Array, isNaN, ...), so exposing it under the standard name makes
	 * `globalThis.X` resolve. The self-member forms a cycle the gc mark phase
	 * already guards against. */
	vm_reg_var(vm, NULL, "globalThis", vm->root, true);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
