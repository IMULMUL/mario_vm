#ifdef __cplusplus
extern "C" {
#endif

#include "native_Function.h"
#include <string.h>

#define CLS_FUNCTION "Function"

/* Function.prototype.call/apply/bind.
 *
 * Function objects in this VM carry Object.prototype as their "prototype"
 * member (var_new_func), and that same member doubles as the instance prototype
 * used by `new f()` (do_new reads var_get_prototype(func)). Relinking functions
 * to a real Function.prototype would therefore break `new`. Instead find_func()
 * (mario.c) resolves the three universal methods off this class's prototype for
 * any callable receiver that does not shadow them, so every function - script or
 * native - gains call/apply/bind without touching the prototype wiring.
 *
 * In all three natives the receiver (`fn` in `fn.call(...)`) arrives as env's
 * `this`; the declared leading arg is thisArg and any trailing arguments follow
 * in env's natural-order "arguments" array. mario_apply_var borrows its args
 * array and returns a baseline refs==0 value that func_call adopts. */

/* Hidden, invisable members a bound function carries its captured state in.
 * They live on the bound function object itself, so the func->data self-pointer
 * installed by native_Function_bind stays valid for as long as the bound
 * function - and therefore its captured target/this/args - is reachable. This
 * mirrors Proxy.revocable's __revoke closure (data == the owning proxy var). */
#define BIND_TARGET "@@bind_target"
#define BIND_THIS   "@@bind_this"
#define BIND_ARGS   "@@bind_args"

/* The trampoline installed as a bound function's native. data == the bound
 * function var; the captured target/this/leading-args are read back off it and
 * concatenated with the arguments supplied at this call site. */
static var_t* native_bound_call(vm_t* vm, var_t* env, void* data) {
	var_t* bound = (var_t*)data;
	if(bound == NULL)
		return var_new(vm);

	var_t* target  = var_find_own_member_var(bound, BIND_TARGET);
	var_t* thisArg = var_find_own_member_var(bound, BIND_THIS);
	var_t* preArgs = var_find_own_member_var(bound, BIND_ARGS);

	/* Hold the captured values across the inner call: mario_apply_var runs the
	 * target (a nested vm_run where an opportunistic gc may fire) and these are
	 * bare C pointers until it returns. An extra ref keeps each alive regardless
	 * of whether `bound` itself happens to be rooted at that instant. */
	var_ref(target);
	var_ref(thisArg);
	var_ref(preArgs);

	var_t* args = var_new_array(vm);
	uint32_t pn = (preArgs != NULL) ? var_array_size(preArgs) : 0;
	for(uint32_t i = 0; i < pn; i++) {
		var_t* a = var_array_get_var(preArgs, i);
		var_array_add(args, (a != NULL) ? a : var_new(vm));
	}
	uint32_t cn = get_func_args_num(env);
	for(uint32_t i = 0; i < cn; i++) {
		var_t* a = get_func_arg(env, i);
		var_array_add(args, (a != NULL) ? a : var_new(vm));
	}

	var_t* res = mario_apply_var(vm, target, thisArg, args);
	var_unref(args);
	var_unref(preArgs);
	var_unref(thisArg);
	var_unref(target);
	return res;
}

/* Function.prototype.call(thisArg, ...args) */
var_t* native_Function_call(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* target = get_obj(env, THIS);
	var_t* thisArg = get_func_arg(env, 0);
	uint32_t n = get_func_args_num(env);
	var_t* args = var_new_array(vm);
	for(uint32_t i = 1; i < n; i++) {
		var_t* a = get_func_arg(env, i);
		var_array_add(args, (a != NULL) ? a : var_new(vm));
	}
	var_t* res = mario_apply_var(vm, target, thisArg, args);
	var_unref(args);
	return res;
}

/* Function.prototype.apply(thisArg, argsArray): like call, but the trailing
 * arguments arrive as a single (borrowed) array. A missing/null array is
 * handled by mario_apply_var as "no arguments". */
var_t* native_Function_apply(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* target = get_obj(env, THIS);
	var_t* thisArg = get_func_arg(env, 0);
	var_t* args = get_func_arg(env, 1);
	return mario_apply_var(vm, target, thisArg, args);
}

/* Function.prototype.bind(thisArg, ...args): mint a bound-function object that
 * prepends the captured leading arguments and pins `this`. */
var_t* native_Function_bind(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* target = get_obj(env, THIS);
	if(!var_is_callable(target)) {
		vm_throw_type_native(vm, "TypeError", "bind target is not callable");
		return var_new(vm);
	}
	var_t* thisArg = get_func_arg(env, 0);

	var_t* preArgs = var_new_array(vm);
	uint32_t n = get_func_args_num(env);
	for(uint32_t i = 1; i < n; i++) {
		var_t* a = get_func_arg(env, i);
		var_array_add(preArgs, (a != NULL) ? a : var_new(vm));
	}

	var_t* bound = var_new_native_func(vm, native_bound_call, NULL);
	if(bound == NULL) {
		var_unref(preArgs);
		return var_new(vm);
	}
	var_get_func(bound)->data = bound;   // self-pointer, valid while bound is reachable

	node_t* tn = var_add(bound, BIND_TARGET, target);
	tn->invisable = 1; tn->be_unenumerable = 1;
	node_t* hn = var_add(bound, BIND_THIS, (thisArg != NULL) ? thisArg : var_new(vm));
	hn->invisable = 1; hn->be_unenumerable = 1;
	/* preArgs is a baseline refs==0 array; var_add takes the one owning reference
	 * for the member, so it must NOT be unref'd here (doing so frees it and the
	 * bound function loses its captured leading arguments). target/thisArg are
	 * borrowed from env and likewise only gain the member's reference. */
	node_t* an = var_add(bound, BIND_ARGS, preArgs);
	an->invisable = 1; an->be_unenumerable = 1;

	return bound;   // refs==0 baseline; func_call adopts it
}

void reg_native_Function(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_FUNCTION);
	vm->builtin_vars.var_Function = cls;
	vm_reg_native(vm, cls, "call(thisArg)", native_Function_call, NULL);
	vm_reg_native(vm, cls, "apply(thisArg, args)", native_Function_apply, NULL);
	vm_reg_native(vm, cls, "bind(thisArg)", native_Function_bind, NULL);
	vm_reg_var(vm, cls, SYMKEY_TOSTRINGTAG, var_new_str(vm, "Function"), true);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
