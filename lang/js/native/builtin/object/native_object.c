#ifdef __cplusplus
extern "C" {
#endif

#include "native_object.h"

/** Object */

var_t* native_Object_create(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* proto = get_obj(env, "proto");
	var_t* ret = var_new_obj(vm, NULL, NULL);
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
	node_t* n = var_find(obj, name);
	return var_new_bool(vm, (n != NULL && n->be_inherited == 0 && n->invisable == 0));
}

static inline uint32_t var_properties_num(vm_t* vm, var_t* var, var_t* keys_var, bool enumerable) {
	uint32_t num = 0;
	uint32_t i;
	for(i=0; i<var->children.size; i++) {
		node_t* node = (node_t*)var->children.items[i];
		if(node != NULL && 
				node->be_inherited == 0 &&
				node->invisable == 0 &&
				node->var != keys_var) {
			if(!node->be_unenumerable || !enumerable) {
				var_array_add(keys_var, var_new_str(vm, node->name));
				++num;
			}
		}
	}

	var_t* proto = var_get_prototype(var);
	while(proto != NULL) {
		num += var_properties_num(vm, proto, keys_var, enumerable);
		proto = var_get_prototype(proto);
	}
	return num;
}

var_t* native_Object_getPropertiesNum(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_obj(env, THIS);
	if(obj->is_array)
		return var_new_int(vm, var_array_size(obj));

	bool enumerable = get_bool(env, "enumerable");
	node_t* keys_node = var_add(obj, "_property_keys_", var_new_array(vm));
	keys_node->be_unenumerable = 1;
	keys_node->invisable = 1;
	uint32_t num = var_properties_num(vm, obj, keys_node->var, enumerable);	
	return var_new_int(vm, num);
}

var_t* native_Object_getPropertyKey(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* obj = get_obj(env, THIS);
	uint32_t index = get_int(env, "index");
	if(obj->is_array)
		return var_new_int(vm, index);

	node_t* keys_node = var_find(obj, "_property_keys_");
	var_t* key = var_array_get_var(keys_node->var, index);
	return key;
}

#define CLS_OBJECT "Object"

void reg_native_object(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_OBJECT);
	vm_reg_static(vm, cls, "create(proto)", native_Object_create, NULL); 
	vm_reg_static(vm, cls, "getPrototypeOf(obj)", native_Object_getPrototypeOf, NULL); 
	vm_reg_static(vm, cls, "hasOwnProperty(name)", native_Object_hasOwnProperty, NULL); 
	vm_reg_static(vm, cls, "getPropertiesNum(enumerable)", native_Object_getPropertiesNum, NULL); 
	vm_reg_static(vm, cls, "getPropertyKey(index)", native_Object_getPropertyKey, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
