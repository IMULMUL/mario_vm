#ifdef __cplusplus
extern "C" {
#endif

#include "native_object.h"

/** Object */

var_t* native_Object_create(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* proto = get_obj(env, "proto");
	var_t* ret = var_new_obj(vm, NULL, NULL);
	var_from_prototype(ret, proto);
	return ret;
}

var_t* native_Object_getPrototypeOf(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* obj = get_obj(env, "obj");
	return var_get_prototype(obj);
}

var_t* native_Object_hasOwnProperty(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* obj = get_obj(env, THIS);
	const char* name = get_str(env, "name");
	node_t* n = var_find(obj, name);
	return var_new_bool(vm, (n != NULL && n->be_inherited == 0));
}

#define CLS_OBJECT "Object"

void reg_native_object(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_OBJECT);
	vm_reg_static(vm, cls, "create(proto)", native_Object_create, NULL); 
	vm_reg_static(vm, cls, "getPrototypeOf(obj)", native_Object_getPrototypeOf, NULL); 
	vm_reg_static(vm, cls, "hasOwnProperty(name)", native_Object_hasOwnProperty, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
