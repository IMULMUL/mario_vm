#ifdef __cplusplus
extern "C" {
#endif

#include "native_Array.h"

/** Array */

var_t* native_Array_constructor(vm_t* vm, var_t* env, void* data) {
    (void)vm; (void)data;
    var_t* this_v = get_obj(env, THIS);
    this_v->is_array = 1;
    var_t* members = var_new_obj_no_proto(vm, NULL, NULL);
    node_t* n = var_add(this_v, "_ARRAY_", members);
    n->be_unenumerable = 1;
	n->invisable = 1;
    return this_v;
}
    

var_t* native_Array_toString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* arr = get_obj(env, THIS);
	mstr_t* ret = mstr_new("");

	mstr_t* str = mstr_new("");
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_to_str(n->var, str);
			if(i > 0)
				mstr_add(ret, ',');
			mstr_append(ret, str->cstr);
		}
	}
	mstr_free(str);

	var_t* v = var_new_str(vm, ret->cstr);
	mstr_free(ret);
	return v;
}

var_t* native_Array_join(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* arr = get_obj(env, THIS);
	const char* j = get_str(env, "c");
	mstr_t* ret = mstr_new("");

	mstr_t* str = mstr_new("");
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_to_str(n->var, str);
			if(i > 0)
				mstr_append(ret, j);
			mstr_append(ret, str->cstr);
		}
	}
	mstr_free(str);

	var_t* v = var_new_str(vm, ret->cstr);
	mstr_free(ret);
	return v;
}

var_t* native_Array_forEach(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_t* f = get_obj(env, "f");

	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			/* Build callback args as a proper array in forward order
			 * (element, index, array) then reverse, matching call_m_func's
			 * contract (see native_Map_forEach). The old var_new/var_add form
			 * was invisible to var_array_size, so the callback received no
			 * arguments and its parameters were all undefined. */
			var_t* args = var_new_array(vm);
			var_array_add(args, n->var);
			var_array_add(args, var_new_int(vm, i));
			var_array_add(args, arr);
			var_array_reverse(args);
			var_t* res = call_m_func(vm, env, f, args);
			var_unref(args);
			if(res != NULL)	
				var_unref(res);
		}
	}
	return NULL;
}

var_t* native_Array_map(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* arr = get_obj(env, THIS);
	var_t* f = get_obj(env, "f");
	var_t* ret = var_new_array(vm);
	if(f == NULL || f->type == V_UNDEF) {
		var_instance_from(ret, arr);
		return ret;
	}
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			/* callback args in forward order then reversed (call_m_func contract). */
			var_t* args = var_new_array(vm);
			var_array_add(args, n->var);
			var_array_add(args, var_new_int(vm, i));
			var_array_add(args, arr);
			var_array_reverse(args);
			var_t* res = call_m_func(vm, env, f, args);
			var_unref(args);
			if(res != NULL) {
				var_array_add(ret, res); // ret takes a ref to res
				var_unref(res);          // release call_m_func's ref
			}
		}
	}
	var_instance_from(ret, arr);
	return ret;
}

var_t* native_Array_filter(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* arr = get_obj(env, THIS);
	var_t* f = get_obj(env, "f");
	var_t* ret = var_new_array(vm);
	if(f == NULL || f->type == V_UNDEF) {
		var_instance_from(ret, arr);
		return ret;
	}
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_t* args = var_new_array(vm);
			var_array_add(args, n->var);
			var_array_add(args, var_new_int(vm, i));
			var_array_add(args, arr);
			var_array_reverse(args);
			var_t* res = call_m_func(vm, env, f, args);
			var_unref(args);
			bool keep = (res != NULL) && var_get_bool(res);
			if(res != NULL)
				var_unref(res);
			if(keep)
				var_array_add(ret, n->var); // shares the original element
		}
	}
	var_instance_from(ret, arr);
	return ret;
}

var_t* native_Array_reduce(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* arr = get_obj(env, THIS);
	var_t* f = get_obj(env, "f");
	uint32_t sz = var_array_size(arr);

	if(f == NULL || f->type == V_UNDEF)
		return NULL;

	var_t* acc = NULL;
	bool acc_owned = false; // true once acc is a call_m_func result we must release
	uint32_t start;
	var_t* initial = get_obj(env, "initial");
	if(initial != NULL && initial->type != V_UNDEF) {
		acc = initial; // borrowed from env; do not free
		start = 0;
	}
	else {
		if(sz == 0)
			return NULL; // JS throws here; return undefined instead
		node_t* n0 = var_array_get(arr, 0);
		acc = (n0 != NULL) ? n0->var : NULL; // borrowed from arr
		start = 1;
	}

	uint32_t i;
	for(i=start; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n == NULL)
			continue;
		var_t* args = var_new_array(vm);
		var_array_add(args, (acc != NULL) ? acc : var_new(vm));
		var_array_add(args, n->var);
		var_array_add(args, var_new_int(vm, i));
		var_array_add(args, arr);
		var_array_reverse(args);
		var_t* res = call_m_func(vm, env, f, args);
		var_unref(args);
		if(acc_owned && acc != NULL)
			var_unref(acc); // release the previous owned accumulator
		acc = res;          // owned (refs>=1)
		acc_owned = true;
	}

	if(acc == NULL)
		return NULL;
	/* Return the accumulator borrowed (refs matching the VM contract): drop the
	 * single reference call_m_func handed us, mirroring func_call's own refs--
	 * for a script return. Guarded so we never underflow. */
	if(acc_owned && acc->refs > 0)
		acc->refs--;
	return acc;
}

var_t* native_Array_reverse(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_array_reverse(arr);
	return arr;
}

var_t* native_Array_concat(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t args_num = get_func_args_num(env);
	uint32_t i;
	for(i=0; i<args_num; ++i) {
		var_t* arg = get_func_arg(env, i);
		if(!arg->is_array) { //not array type, just added to target array.
			var_array_add(arr, arg);
		}
		else { // array type , add members to target array.
			uint32_t sz = var_array_size(arg);
			uint32_t j;
			for(j=0; j<sz; ++j) {
				node_t* n = var_array_get(arg, j);
				if(n != NULL) {
					var_array_add(arr, n->var);
				}
			}
		}
	}
	return arr;
}

var_t* native_Array_push(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t args_num = get_func_args_num(env);
	uint32_t i;
	for(i=0; i<args_num; ++i) {
		var_t* arg = get_func_arg(env, i);
		var_array_add(arr, arg);
	}
	return arr;
}

var_t* native_Array_unshift(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t args_num = get_func_args_num(env);
	uint32_t i;
	for(i=args_num; i>0; --i) {
		var_t* arg = get_func_arg(env, i-1);
		var_array_add_head(arr, arg);
	}
	return arr;
}

var_t* native_Array_pop(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_t* ret = NULL;
	uint32_t sz = var_array_size(arr);
	if(sz == 0)
		return NULL;
	
	node_t* n = var_array_remove(arr, sz-1);
	ret = var_ref(n->var);
	node_free(n);
	var_unref(ret);
	return ret;
}

var_t* native_Array_shift(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_t* ret = NULL;
	uint32_t sz = var_array_size(arr);
	if(sz == 0)
		return NULL;
	
	node_t* n = var_array_remove(arr, 0);
	ret = var_ref(n->var);
	node_free(n);
	var_unref(ret);
	return ret;
}

var_t* native_Array_slice(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t sz = var_array_size(arr);
	int32_t start = get_int(env, "start");
	if(start < 0) 
		start = sz + start;

	int32_t end;
	var_t* end_var= get_obj(env, "end");
	if(end_var == NULL || end_var->type == V_UNDEF) 
		end = sz;
	else 
		end = var_get_int(end_var);
	if(end < 0) end = sz + end;

	uint32_t i;
	var_t* ret = var_new_array(vm);
	for(i=start; i<end; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_array_add(ret, n->var);
		}
	}
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_Array_isArray(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* obj = get_obj(env, "obj");
	return var_new_bool(vm, obj->is_array);
}

var_t* native_Array_length(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* arr = get_obj(env, THIS);
	uint32_t sz = var_array_size(arr);
	return var_new_int(vm, sz);
}

#define CLS_ARRAY "Array"

/* ES6: Array is iterable; [Symbol.iterator] yields each element. */
var_t* native_Array_iterator(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* this_v = get_obj(env, THIS);
	return vm_new_array_iterator(vm, this_v); /* refs=0 */
}

void reg_native_Array(vm_t* vm) {
  var_t* cls = vm_new_class(vm, CLS_ARRAY);
	vm_reg_native(vm, cls, "constructor()", native_Array_constructor, NULL);
	vm_reg_native(vm, cls, "toString()", native_Array_toString, NULL); 
	vm_reg_native(vm, cls, "forEach(f)", native_Array_forEach, NULL); 
	vm_reg_native(vm, cls, "map(f)", native_Array_map, NULL);
	vm_reg_native(vm, cls, "filter(f)", native_Array_filter, NULL);
	vm_reg_native(vm, cls, "reduce(f, initial)", native_Array_reduce, NULL);
	vm_reg_native(vm, cls, "reverse()", native_Array_reverse, NULL); 
	vm_reg_native(vm, cls, "concat()", native_Array_concat, NULL); 
	vm_reg_native(vm, cls, "join(c)", native_Array_join, NULL); 
	vm_reg_native(vm, cls, "push()", native_Array_push, NULL); 
	vm_reg_native(vm, cls, "pop()", native_Array_pop, NULL); 
	vm_reg_native(vm, cls, "shift()", native_Array_shift, NULL); 
	vm_reg_native(vm, cls, "unshift()", native_Array_unshift, NULL); 
	vm_reg_native(vm, cls, "slice(start, end)", native_Array_slice, NULL); 
	vm_reg_native(vm, cls, "isArray(obj)", native_Array_isArray, NULL); 
	vm_reg_native(vm, cls, "length()", native_Array_length, NULL); 
	vm_reg_native(vm, cls, SYMKEY_ITERATOR "()", native_Array_iterator, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
