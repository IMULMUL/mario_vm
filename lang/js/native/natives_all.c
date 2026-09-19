#include "mario.h"
#include "natives_builtin.h"
#include "natives.h"
#include "WeakRef/native_WeakRef.h"
#include "FinalizationRegistry/native_FinalizationRegistry.h"
#include <math.h>

static inline var_t* vm_load_var(vm_t* vm, const char* name, bool create) {
	node_t* n = vm_load_node(vm, name, create);
	if(n != NULL)
		return n->var;
	return NULL;
}

/* Global Boolean(value): ToBoolean coercion. mario has no Boolean class (no
 * wrapper objects), but bundled code calls `Boolean(x)` constantly. Strings
 * are truthy when non-empty; every object (symbols/functions included) is
 * truthy; numerics defer to var_get_bool. */
static var_t* native_Boolean_call(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_obj(env, "value");
	bool b = false;
	if(v != NULL) {
		switch(v->type) {
			case V_UNDEF: case V_NULL: b = false; break;
			case V_STRING: { const char* s = var_get_str(v); b = (s != NULL && s[0] != 0); } break;
			case V_OBJECT: b = true; break;
			default: b = var_get_bool(v); break;
		}
	}
	return var_new_bool(vm, b);
}

static inline void load_basic_classes(vm_t* vm) {
	vm->builtin_vars.var_Object = vm_load_var(vm, "Object", false);
	vm->builtin_vars.var_String = vm_load_var(vm, "String", false);
	vm->builtin_vars.var_Number = vm_load_var(vm, "Number", false);
	vm->builtin_vars.var_BigInt = vm_load_var(vm, "BigInt", false);
	vm->builtin_vars.var_Error = vm_load_var(vm, "Error", false);
	vm->builtin_vars.var_Array = vm_load_var(vm, "Array", false);

	var_t* console = new_obj(vm, "Console", 0);
	var_add(vm->root, "console", console);

	/* Global numeric constants. `undefined`/`null` are lexer keywords, but NaN and
	 * Infinity are ordinary global bindings; without them `Infinity` resolved to
	 * undefined (e.g. [1,[2,[3,[4]]]].flat(Infinity) only flattened one level) and
	 * NaN was indistinguishable from undefined. Registered const + unenumerable on
	 * vm->root, matching how a JS global value binding behaves. */
	vm_reg_var(vm, NULL, "Infinity", var_new_float(vm, (float)INFINITY), true);
	vm_reg_var(vm, NULL, "NaN", var_new_float(vm, (float)NAN), true);
	vm_reg_native(vm, NULL, "Boolean(value)", native_Boolean_call, NULL);
}

void reg_all_natives(vm_t* vm) {
	reg_builtin_natives(vm);
	/* reg_natives must run before load_basic_classes: Console/TextEncoder/URL/
	 * EventTarget and the rest of the platform classes now live here, and
	 * load_basic_classes instantiates `console` via new_obj(vm, "Console", 0).
	 * Registering the class after that point would leave the global `console`
	 * detached from its prototype (silent no-op for console.log). */
	reg_natives(vm);
	load_basic_classes(vm);
}
