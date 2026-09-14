# Chapter 7 · The Object Model and Scopes

This chapter explains how Mario uses C structs to represent everything in the JavaScript world — numbers, strings, objects, arrays, functions, classes. It is the foundation for understanding VM behavior.

## 7.1 Everything is a var_t

In Mario, **every value** in a script is a `var_t` (`mario.h`):

```c
typedef struct st_var {
    uint32_t  magic: 8;      // 0 means this is a var (vs. node's 1)
    uint32_t  type:10;       // value type: V_INT/V_FLOAT/V_STRING/V_OBJECT/V_BOOL/V_NULL/V_UNDEF
    uint32_t  status: 4;     // GC status: FREE/GC_FREE/GC/REF
    uint32_t  is_array:2;    // is an array?
    uint32_t  is_func:2;     // is a function?
    uint32_t  is_class:2;    // is a class?
    uint32_t  gc_marking:2;  // GC marking in progress (cycle guard)
    uint32_t  gc_marked:2;   // GC marked (alive)
    uint32_t  refs;          // reference count

    uint32_t  size;          // byte size of value (used by the bytes type)
    void*     value;         // the actual value: int*/float*/char*/func_t* ...

    free_func_t free_func;   // how to free value
    free_func_t on_destroy;  // callback before destruction

    struct st_var* prev, *next;  // for the var linked list (GC list / free list)
    hash_map_t     children;     // member table: name → node_t
    struct st_vm*  vm;
} var_t;
```

Key design:

- The **combination of `type` + `value`** represents a concrete value. `value` is a `void*`:
  - `V_INT`: `value` points to an `int`;
  - `V_FLOAT`: points to a `float`;
  - `V_STRING`: points to a `char*` string;
  - `V_OBJECT`: `value` is usually NULL, members live in `children`;
  - function: `is_func=1`, `value` points to a `func_t`.
- **`children` is a hash map** (`hash_map_t`) mapping member names to `node_t`. Objects, arrays, classes, and even the global root all store their members through it.
- **Bit-field compression**: `magic:8`, `type:10` etc. pack multiple flags into one 32-bit word, saving memory — important for embedded systems.

### Value types

```c
#define V_UNDEF  0   // undefined
#define V_INT    1   // integer
#define V_FLOAT  2   // float
#define V_STRING 3   // string
#define V_OBJECT 4   // object/array/class
#define V_BOOL   5   // boolean
#define V_NULL   6   // null
#define V_INT64  7   // int64_t (exact large integers: 2^31..2^63-1)
#define V_FLOAT64 8  // double (canonical float: every float literal / fractional result)
#define V_BIGINT 9   // bignum_t* (arbitrary-precision integer; typeof -> "bigint")
```

> ES6+ expanded the numeric types: besides the early `V_INT`/`V_FLOAT`, there are now the exact 64-bit integer `V_INT64`, the canonical double `V_FLOAT64`, and the arbitrary-precision `V_BIGINT` (corresponding to `123n` literals and the BigInt built-in class, see Chapter 11).

### Factory functions for creating values

`mario.h` provides a full set of `var_new_*`:

```c
var_t* var_new(vm_t* vm);                       // undefined
var_t* var_new_int(vm_t* vm, int i);            // integer
var_t* var_new_float(vm_t* vm, float f);        // float
var_t* var_new_str(vm_t* vm, const char* s);    // string
var_t* var_new_bool(vm_t* vm, bool b);          // boolean
var_t* var_new_null(vm_t* vm);                  // null
var_t* var_new_array(vm_t* vm);                 // array
var_t* var_new_obj(vm_t* vm, var_t* proto, ...);// object with a prototype
var_t* var_new_func(vm_t* vm, func_t* func);    // function
```

Note that `var_new_int` also gives integers a prototype (`Number.prototype`):

```c
inline var_t* var_new_int(vm_t* vm, int i) {
    var_t* var = var_new(vm);
    var->type = V_INT;
    var->value = mario_malloc(sizeof(int));
    *((int*)var->value) = i;
    var_set_prototype(var, var_get_prototype(vm->builtin_vars.var_Number));
    return var;
}
```

This is why a script can write `(5).toString()` — integers also have `Number`'s prototype chain.

## 7.2 A member is a node_t

Each property/method/array element of an object is not stored directly as a `var_t`, but wrapped in a `node_t` (`mario.h`):

```c
typedef struct st_node {
    uint32_t  magic: 8;          // 1 means this is a node
    uint32_t  be_const : 8;      // is a constant (const declaration, cannot change)
    uint32_t  be_inherited : 8;  // inherited from a prototype
    uint32_t  be_unenumerable:4; // non-enumerable (for-in skips it, e.g. prototype, native methods)
    uint32_t  invisable : 4;     // invisible (e.g. the prototype link itself)
    uint32_t  ncache_instr;      // member-access cache related
    char*     name;              // member name (NULL for array elements)
    var_t*    var;               // points to the actual value
} node_t;
```

**Why wrap in an extra node layer?** Because a property carries a bunch of metadata besides its "value": is it a constant, can it be enumerated, is it inherited. Putting these on the node keeps `var_t` a pure "value" semantic.

### Adding/removing/finding members

```c
node_t* var_add(var_t* var, const char* name, var_t* add);       // add a member (replace if it exists)
node_t* var_find_own_member(var_t* var, const char* name);       // search own only
node_t* var_find_member(var_t* obj, const char* name);           // own + prototype chain
var_t*  var_find_member_var(var_t* obj, const char* name);       // same, returns the var directly
```

`var_add` stores in a hash map, so lookup is O(1):

```c
node_t* var_add(var_t* var, const char* name, var_t* add) {
    node_t* node = (name[0]!=0) ? var_find_raw(var, name) : NULL;
    if(node == NULL) {                          // doesn't exist → create
        node = node_new(var->vm, name, add);
        hash_map_add(&var->children, name, node);
    } else if(add != NULL) {
        node_replace(node, add);                // exists → replace the value
    }
    return node;
}
```

### Arrays

An array is also a `var_t` (`is_array=1`); its elements are stored as **nameless** `node_t`s, accessed by index:

```c
node_t* var_array_get(var_t* var, int32_t index);   // get by index
node_t* var_array_add(var_t* var, var_t* add);      // append
uint32_t var_array_size(var_t* var);                // length
void var_array_reverse(var_t* var);                 // reverse (used for function arguments)
```

`var_new_array` hangs the elements under a hidden member `_ARRAY_` and sets `be_unenumerable`. An array's prototype is `Array.prototype`, so it can call built-in methods like `push`/`length` (see Chapter 9).

## 7.3 The prototype chain: Mario-style inheritance

Mario's object inheritance is implemented via **prototypes**, consistent with JavaScript semantics.

- Every object points to its prototype object through a hidden member named `"prototype"`:

```c
var_t* var_get_prototype(var_t* var) { return get_obj(var, PROTOTYPE); }  // PROTOTYPE = "prototype"

void var_set_prototype(var_t* var, var_t* proto) {
    node_t* ret = var_add(var, PROTOTYPE, proto);
    ret->invisable = 1;            // invisible
    ret->be_unenumerable = 1;      // for-in won't traverse it
}
```

- When looking up a member, it searches own first, then walks up the prototype chain:

```c
node_t* var_find_member(var_t* obj, const char* name) {
    node_t* node = var_find_own_member(obj, name);   // ① search own
    if(node == NULL)
        node = vm_find_in_class(obj, name);          // ② search the prototype chain
    return node;
}

node_t* vm_find_in_class(var_t* var, const char* name) {
    var_t* proto = var_get_prototype(var);
    while(proto != NULL) {                            // walk up the prototype chain
        node_t* ret = var_find_own_member(proto, name);
        if(ret != NULL) {
            if(ret->var->is_func)
                return ret;                           // method: return directly (shared)
            ret = var_add(var, name, var_clone(ret->var));  // field: clone onto self
            ret->be_inherited = 1;
            return ret;
        }
        proto = var_get_prototype(proto);
    }
    return NULL;
}
```

Note the difference: **methods** (functions) share the same copy along the prototype chain; **ordinary fields** are cloned onto the object itself on first access (marked `be_inherited=1`).

### instanceof

`var_instanceof` walks the prototype chain checking whether it meets the target prototype:

```c
bool var_instanceof(var_t* var, var_t* proto) {
    var_t* v = var_get_prototype(proto);  // take the class's prototype
    if(v != NULL) proto = v;
    var_t* protov = var_get_prototype(var);
    while(protov != NULL) {               // walk up the object's prototype chain
        if(protov == proto) return true;
        protov = var_get_prototype(protov);
    }
    return false;
}
```

## 7.4 Runtime representation of classes and inheritance

A `class` statement is handled at runtime by `handle_class` → `vm_new_class`:

```c
var_t* vm_new_class(vm_t* vm, const char* cls) {
    node_t* n = vm_load_node(vm, cls, true);   // create the class-name variable globally
    var_t* cls_var = n->var;
    cls_var->type = V_OBJECT;
    if(var_get_prototype(cls_var) == NULL)
        var_set_prototype(cls_var, var_new_obj_no_proto(vm, NULL, NULL));
    if(strcmp(cls, "Object") != 0)
        do_extends(vm, cls_var, "Object");      // all classes inherit Object by default
    return cls_var;
}
```

**A class is itself a `var_t`**; it has a prototype object, and the methods defined in the class all hang on that prototype.

`extends` is implemented by `do_extends` → `var_set_father`, which points the "subclass's prototype"'s prototype at the "superclass's prototype", chaining the prototype chain:

```c
static void var_set_father(var_t* var, var_t* father) {
    var_t* proto = var_get_prototype(var);          // subclass's prototype
    var_t* super_proto = var_get_prototype(father); // superclass's prototype
    var_set_prototype(proto, super_proto);          // subclassProto.__proto__ = superclassProto
}
```

### The new process

`new Foo(a, b)` is done by `handle_new` → `do_new` → `new_obj`:

```c
var_t* new_obj(vm_t* vm, const char* name, int arg_num) {
    node_t* n = vm_load_node(vm, name, false);        // find the class
    var_t* protoV = var_get_prototype(n->var);        // take the class's prototype
    obj = var_new_obj(vm, protoV, NULL, NULL);        // new object, prototype = class's prototype

    // find the constructor: if the class itself is a function use it, else find constructor on the prototype
    var_t* constructor = n->var->is_func ? n->var
                         : var_find_member_var(protoV, CONSTRUCTOR);
    if(constructor != NULL) {
        func_call(vm, obj, constructor, arg_num);     // call the constructor with obj as this
        obj = vm_pop2(vm);
    }
    return obj;
}
```

The new object's `this` is itself; the constructor hangs fields onto `this`. This also explains the `function f(){ this.age=18 } var a=new f()` pattern in `test/js/class.js` — using a function as a constructor.

## 7.5 Functions: func_t

The `value` of a function value (a `var_t` with `is_func=1`) points to a `func_t` (`mario.h`):

```c
typedef struct st_func {
    native_func_t  native;     // native function: C function pointer (NULL for script functions)
    int8_t  regular: 4;        // is a regular function (get/set are non-regular)
    int8_t  is_static: 4;      // is static
    int8_t  is_generator: 4;   // ES6 `function*` / generator method
    int8_t  is_arrow: 4;       // ES6 arrow function: lexical this, no prototype, not constructible
    PC      pc;                // script function: the body's entry in the bytecode
    void*   data;              // user data passed to the native
    m_array_t args;            // parameter-name list
    var_t*  owner;             // owning object (used for super)
    var_t*  owner_var;         // back-pointer to the var_t owning this func_t (lets the GC root the func_t along the lexical chain)
    struct { var_t* var; struct st_func* func; } closure;  // closure: the captured environment
    var_t*  closure_func_ref;  // owned reference to closure.func's owner var (prevents the outer function being collected early and dangling)
} func_t;
```

Besides the "script/native" distinction, `func_t` also uses `is_generator` and `is_arrow` to mark ES6 generator functions and arrow functions (an arrow function captures the lexical `this`, has no `prototype`, and cannot be `new`-ed). `owner_var` and `closure_func_ref` let the GC correctly root the `func_t` along the closure's lexical chain, preventing an inner closure's `closure.func` from dangling after the outer function is collected early (details in Chapter 8).

Two kinds of function:

| Type | Discriminant | Execution |
| --- | --- | --- |
| **Script function** | `native == NULL` | `vm->pc = func->pc`, recursively `vm_run` the bytecode |
| **Native function** | `native != NULL` | directly call `func->native(vm, env, data)` (see Chapter 9) |

Parsing of a script function body is in `func_def`: after the `FUNC` instruction it reads `LOAD`s one by one (parameter names); on hitting `JMP` it records the body entry `func->pc` and skips over the whole body.

## 7.6 Closures

When a function returns another function, the inner function needs to "remember" the outer local variables — that's a closure. Mario implements it via `func_t.closure`.

In `handle_return`, if the returned value is a function, it calls:

```c
static bool func_set_closure(var_t* var, var_t* closure, func_t* closure_func) {
    func_t* func = var_get_func(var);
    func->closure.var  = var_ref(closure);       // capture the defining scope environment (env)
    func->closure.func = closure_func;           // capture the outer function (forming a closure chain)
    return true;
}
```

Later, when looking up a variable (`vm_find_in_scopes` from Chapter 6), if currently in a function scope it **first searches along the closure chain**:

```c
if(sc != NULL && sc->is_func) {
    var_t* closure = sc->func->closure.var;
    func_t* closure_func = sc->func->closure.func;
    while(closure != NULL && closure_func != NULL) {
        ret = var_find_own_member(closure, name);   // look in the captured environment
        if(ret != NULL) return ret;
        closure = closure_func->closure.var;         // continue outward along the closure chain
        closure_func = closure_func->closure.func;
    }
}
```

Because the closure holds the outer `env` with `var_ref`, even after the outer function has returned those variables won't be collected by the GC. `test/js/closure.js` demonstrates this mechanism.

## 7.7 The global object root

`vm->root` is the container of the global scope. All global variables, global functions, and built-in classes (`Object`/`String`/`Console`…) hang as members under `root`. The last step of `vm_find_in_scopes` is `var_find_own_member(vm->root, name)`.

When you write `console.log(...)`, `console` is an object found in `root` (registered by `reg_all_natives`, see Chapter 9).

## 7.8 Summary

| Concept | Mario's implementation |
| --- | --- |
| Value | `var_t` (type + value + children) |
| Property/element | `node_t` (name + var + meta flags) |
| Object | a `var_t` with `is_array=0`, members in `children` |
| Array | a `var_t` with `is_array=1`, elements as nameless nodes |
| Function | a `var_t` with `is_func=1`, `value` points to a `func_t` |
| Class | a `var_t` with `type=V_OBJECT`, methods on its prototype |
| Inheritance | prototype chain: strung together by the hidden `prototype` member |
| Closure | `func_t.closure` captures the defining environment; lookups walk the closure chain |
| Global | `vm->root` |

How are these objects created, reused, and collected in memory? Go to [Chapter 8 · Memory Management and Garbage Collection](08-gc.md).
