# Chapter 9 · Native Extensions and Built-in Classes

A script by itself can only do pure computation. To print output, manipulate strings, handle JSON, or get the system time, you need **native functions** — functions implemented in C, registered into the VM, and callable from scripts. This chapter explains how to write and register them.

## 9.1 Built-in classes overview

Mario's JS frontend ships with a large set of built-in classes, located in [`lang/js/native/`](../../../lang/js/native/), covering everything from basic types to ES6+ metaprogramming, binary data, weak references, etc. (the complete ES6+ built-in object list and usage is in [Chapter 11](11-es6-support.md)):

```
native/
├── builtin/            # language-level basic classes (reg_builtin_natives)
│   ├── Object/  Error/  Array/  String/  Number/  Symbol/  Console/
│   ├── Map/  Set/                     # includes WeakMap / WeakSet
│   ├── Promise/                       # + async/await runtime
│   ├── Proxy/  Reflect/               # metaprogramming
│   ├── BigInt/                        # arbitrary-precision integers
│   ├── ArrayBuffer/  DataView/  TypedArray/
│   ├── SharedArrayBuffer/  Atomics/
│   ├── WeakRef/  FinalizationRegistry/
│   └── RegExp/
└── natives/            # extension classes (reg_natives)
    ├── JSON/           # JSON.stringify / JSON.parse
    ├── Date/           # Date
    └── Math/           # Math
```

Registration has two levels:

```c
// natives_all.c
void reg_all_natives(vm_t* vm) {
    reg_builtin_natives(vm);   // Object/Error/Array/String/Console/Number/BigInt/
                               // ArrayBuffer/DataView/TypedArray/Promise/Map/Set/
                               // Symbol/Proxy/Reflect/WeakRef/FinalizationRegistry/
                               // SharedArrayBuffer/Atomics/RegExp
    load_basic_classes(vm);    // cache common class pointers into vm->builtin_vars, create console, register Infinity/NaN
    reg_natives(vm);           // Math/Date/JSON
}
```

`reg_all_natives` is exactly the `on_init` callback passed to `vm_init` in Chapter 2 — the VM registers all built-in classes automatically at initialization.

`load_basic_classes` caches common classes' `var_t*` into `vm->builtin_vars` (`var_Object`/`var_String`/`var_Number`/`var_BigInt`/`var_Error`/`var_Array`), creates the global `console` object, and registers the global constants `Infinity` / `NaN`:

```c
static inline void load_basic_classes(vm_t* vm) {
    vm->builtin_vars.var_Object = vm_load_var(vm, "Object", false);
    vm->builtin_vars.var_String = vm_load_var(vm, "String", false);
    ...
    var_t* console = new_obj(vm, "Console", 0);
    var_add(vm->root, "console", console);   // global console
    vm_reg_var(vm, NULL, "Infinity", var_new_float(vm, INFINITY), true);
    vm_reg_var(vm, NULL, "NaN",      var_new_float(vm, NAN),      true);
}
```

This is why a script can write `console.log(...)` directly.

## 9.2 The native function signature

All native functions share the same C prototype (`native_func_t`, see `mario.h`):

```c
typedef var_t* (*native_func_t)(struct st_vm* vm, var_t* env, void* data);
```

| Parameter | Meaning |
| --- | --- |
| `vm` | the current VM instance, used to create return values and access built-in classes |
| `env` | the **call environment object**: contains the `arguments` array, `this`, and arguments bound by parameter name |
| `data` | the user-data pointer passed at registration (can hold C-side context) |
| return value | a `var_t*`, pushed as the function's return value; returning `NULL` is treated as returning undefined |

## 9.3 Getting arguments from env

`mario.h` provides a set of convenience functions to fetch data from `env`:

```c
var_t*      get_func_args(var_t* env);              // get the arguments array
uint32_t    get_func_args_num(var_t* env);          // actual argument count
var_t*      get_func_arg(var_t* env, uint32_t i);   // the i-th actual argument (var_t*)
int         get_func_arg_int(var_t* env, uint32_t i);
bool        get_func_arg_bool(var_t* env, uint32_t i);
float       get_func_arg_float(var_t* env, uint32_t i);
const char* get_func_arg_str(var_t* env, uint32_t i);

var_t*      get_obj(var_t* obj, const char* name);  // get a member by name
int         get_int(var_t* obj, const char* name);
const char* get_str(var_t* obj, const char* name);
```

`get_func_args(env)` is essentially `get_obj(env, "arguments")` — as Chapter 6 explained, `func_call` packs the actual arguments into an `arguments` array placed in `env`.

To get arguments by **parameter name**, use `get_obj(env, "paramName")` or `get_int(env, "paramName")`; these names come from the declaration string at registration (see 9.5).

## 9.4 A complete example: Console

Look at the simplest one, [`native_Console.c`](../../../lang/js/native/builtin/Console/native_Console.c):

```c
// convert all arguments to strings and join them with spaces
static mstr_t* args_to_str(var_t* args) {
    mstr_t* ret = mstr_new("");
    uint32_t sz = var_array_size(args);
    for(uint32_t i=0; i<sz; ++i) {
        node_t* n = var_array_get(args, i);
        if(n != NULL) {
            var_to_str(n->var, str);       // any value → string
            if(i > 0) mstr_add(ret, ' ');
            mstr_append(ret, str->cstr);
        }
    }
    return ret;
}

var_t* native_println(vm_t* vm, var_t* env, void* data) {
    var_t* v = get_func_args(env);         // get all actual arguments
    mstr_t* ret = args_to_str(v);
    mstr_add(ret, '\n');
    _platform_out(ret->cstr);              // output via the platform function
    mstr_free(ret);
    return NULL;                           // no return value (undefined)
}

void reg_native_Console(vm_t* vm) {
    var_t* cls = vm_new_class(vm, "Console");
    vm_reg_static(vm, cls, "write(v)", native_print,   NULL);
    vm_reg_static(vm, cls, "log(v)",   native_println, NULL);
}
```

Key points:
- Output goes through `_platform_out` (the platform layer from Chapter 1), not `printf` directly, ensuring portability.
- `var_to_str` can turn any `var_t` (number/object/array) into a readable string.
- Returning `NULL` means the function has no return value.

## 9.5 Registration API and the declaration string

Three registration functions (`mario.h` / `mario.c`):

```c
node_t* vm_reg_var(vm_t* vm, var_t* cls, const char* name, var_t* var, bool be_const);
node_t* vm_reg_native(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data);
node_t* vm_reg_static(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data);
```

- `cls == NULL` means register to the **global** (root); otherwise register onto that class's prototype.
- `vm_reg_native` registers an **instance method** (requires `new`-ing an object or calling via a value's prototype; can access `this`).
- `vm_reg_static` registers a **static method** (`func->is_static=true`, called directly by class name, e.g. `console.log`).

### The decl declaration string

`decl` has the form `"methodName(param1, param2)"`, which `vm_reg_native` parses:

```c
node_t* vm_reg_native(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data) {
    // read the part before '(' as the method name
    // read the comma-separated parts inside the parentheses as parameter names, stored in func->args
    func->native = native;
    func->data = data;
    // attach to cls's prototype (or root)
}
```

For example `"toString(radix)"`: method name `toString`, one parameter `radix`. So inside the native function you can use `get_int(env, "radix")` to fetch that argument by name.

> The parameter names in the declaration matter: `func_call` binds the actual arguments onto `env` according to the names in `func->args`. This way a native function can fetch arguments both by name (`get_obj(env,"radix")`) and by index (`get_func_arg(env,0)`).

## 9.6 Instance methods and this: Number.toString

Look at the instance method in [`native_Number.c`](../../../lang/js/native/builtin/Number/native_Number.c), which needs to access the caller (`this`):

```c
var_t* native_Number_toString(vm_t* vm, var_t* env, void* data) {
    var_t* v = get_obj(env, THIS);         // THIS = "this", get the caller
    if(v->type == V_INT) {
        int radix = get_int(env, "radix"); // get the argument by parameter name
        if(radix < 2 || radix > 36) radix = 10;
        s = mstr_from_int(var_get_int(v), radix);
    } else {
        s = mstr_from_float(var_get_float(v));
    }
    return var_new_str(vm, s);             // return a new string
}

void reg_native_Number(vm_t* vm) {
    var_t* cls = vm_new_class(vm, "Number");
    vm_reg_native(vm, cls, "toString(radix)",     native_Number_toString,     NULL);
    vm_reg_native(vm, cls, "constructor(value)",  native_Number_constructor,  NULL);
}
```

Recall Chapter 7: `var_new_int` gives integers `Number.prototype` as their prototype. So when a script writes `(255).toString(16)`, the integer finds `Number.toString` along the prototype chain; `func_call` puts the integer into `env` as `this`, and the native function reads it and converts it to the hex string `"ff"`.

`constructor` is a special method name (`CONSTRUCTOR = "constructor"`): on `new Number(5)`, `new_obj` finds the `constructor` on the prototype and calls it with the new object as `this`.

## 9.7 Writing your own native class

Suppose you want to add a `Math.abs(x)`:

```c
// native_math_abs.c
#include "mario.h"

static var_t* native_abs(vm_t* vm, var_t* env, void* data) {
    int x = get_func_arg_int(env, 0);      // the 0th actual argument
    return var_new_int(vm, x < 0 ? -x : x);
}

void reg_native_MyMath(vm_t* vm) {
    var_t* cls = vm_new_class(vm, "Math");
    vm_reg_static(vm, cls, "abs(x)", native_abs, NULL);  // static method
}
```

Integration steps:

1. Add your `.o` to `NATIVE_OBJS` in [`lang/js/lang.mk`](../../../lang/js/lang.mk) (it already lists every built-in class's `.o`; just add a line following the pattern).
2. Call `reg_native_MyMath(vm)` in `reg_natives()` (`natives.c`) or `reg_builtin_natives()`.
3. `make` to recompile.

Then it's usable in scripts:

```javascript
console.log(Math.abs(-42));   // prints 42
```

## 9.8 Global functions and global variables

Registering to `root` (`cls == NULL`) makes it global:

```c
// global function print() (see mario/demos/tinyjs/demo.c)
vm_reg_static(vm, NULL, "print()", native_print, NULL);

// global variable
vm_reg_var(vm, NULL, "VERSION", var_new_int(vm, 3), true);  // const
```

The command-line program also uses `var_add(vm->root, "_args", args)` to inject the command-line arguments as a global array `_args`.

## 9.9 Summary

- Native functions have the uniform signature `var_t* f(vm_t* vm, var_t* env, void* data)`.
- Arguments come from `env`: by name `get_obj/get_int(env,"name")`, or by index `get_func_arg(env,i)`; `this` via `get_obj(env, THIS)`.
- Registration uses `vm_reg_native` (instance method) / `vm_reg_static` (static method) / `vm_reg_var` (variable); the `decl` string declares the name and parameters.
- Built-in classes are loaded automatically at VM initialization via `reg_all_natives`.
- Extending only requires: write the native function → register it → add it to `lang.mk` → `make` again.

Next: [Chapter 10 · Bytecode Files and the Toolchain](10-mbc-and-tools.md), covering precompiled files and how to embed Mario in your own program; the complete ES6+ built-in class list and usage is in [Chapter 11](11-es6-support.md).
