#include "mario.h"
#include "natives_builtin.h"
#include "natives.h"
#include <math.h>

static inline var_t* vm_load_var(vm_t* vm, const char* name, bool create) {
	node_t* n = vm_load_node(vm, name, create);
	if(n != NULL)
		return n->var;
	return NULL;
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
}

void reg_all_natives(vm_t* vm) {
	reg_builtin_natives(vm);
	load_basic_classes(vm);

	reg_natives(vm);
}
