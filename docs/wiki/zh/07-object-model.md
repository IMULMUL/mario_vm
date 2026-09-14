# 第 7 章 · 对象模型与作用域

本章讲解 Mario 如何用 C 结构体表示 JavaScript 世界里的一切——数字、字符串、对象、数组、函数、类。这是理解 VM 行为的基础。

## 7.1 万物皆 var_t

在 Mario 里，脚本中的**每一个值**都是一个 `var_t`（`mario.h`）：

```c
typedef struct st_var {
    uint32_t  magic: 8;      // 0 表示这是 var（区别于 node 的 1）
    uint32_t  type:10;       // 值类型：V_INT/V_FLOAT/V_STRING/V_OBJECT/V_BOOL/V_NULL/V_UNDEF
    uint32_t  status: 4;     // GC 状态：FREE/GC_FREE/GC/REF
    uint32_t  is_array:2;    // 是数组？
    uint32_t  is_func:2;     // 是函数？
    uint32_t  is_class:2;    // 是类？
    uint32_t  gc_marking:2;  // GC 标记中（防环）
    uint32_t  gc_marked:2;   // GC 已标记（存活）
    uint32_t  refs;          // 引用计数

    uint32_t  size;          // value 的字节大小（bytes 类型用）
    void*     value;         // 实际值：int*/float*/char*/func_t* ...

    free_func_t free_func;   // 如何释放 value
    free_func_t on_destroy;  // 销毁前回调

    struct st_var* prev, *next;  // 用于 var 链表（GC 链/空闲链）
    hash_map_t     children;     // 成员表：name → node_t
    struct st_vm*  vm;
} var_t;
```

关键设计：

- **`type` + `value` 的组合**表示具体值。`value` 是 `void*`：
  - `V_INT`：`value` 指向一个 `int`；
  - `V_FLOAT`：指向 `float`；
  - `V_STRING`：指向 `char*` 字符串；
  - `V_OBJECT`：`value` 通常为 NULL，成员存在 `children` 里；
  - 函数：`is_func=1`，`value` 指向 `func_t`。
- **`children` 是一张哈希表**（`hash_map_t`），把成员名映射到 `node_t`。对象、数组、类、甚至全局 root 都靠它存放成员。
- **位域（bit-field）压缩**：`magic:8`、`type:10` 等把多个标志塞进一个 32 位字，节省内存——这对嵌入式很重要。

### 值的类型

```c
#define V_UNDEF  0   // undefined
#define V_INT    1   // 整数
#define V_FLOAT  2   // 浮点
#define V_STRING 3   // 字符串
#define V_OBJECT 4   // 对象/数组/类
#define V_BOOL   5   // 布尔
#define V_NULL   6   // null
#define V_INT64  7   // int64_t（精确大整数：2^31..2^63-1）
#define V_FLOAT64 8  // double（规范浮点：所有浮点字面量/带小数结果）
#define V_BIGINT 9   // bignum_t*（任意精度整数；typeof → "bigint"）
```

> ES6+ 扩展了数值类型：除了早期的 `V_INT`/`V_FLOAT`，现在还区分精确的 64 位整数 `V_INT64`、规范双精度 `V_FLOAT64`，以及任意精度的 `V_BIGINT`（对应 `123n` 字面量与 BigInt 内建类，见第 11 章）。

### 创建值的工厂函数

`mario.h` 提供了一整套 `var_new_*`：

```c
var_t* var_new(vm_t* vm);                       // undefined
var_t* var_new_int(vm_t* vm, int i);            // 整数
var_t* var_new_float(vm_t* vm, float f);        // 浮点
var_t* var_new_str(vm_t* vm, const char* s);    // 字符串
var_t* var_new_bool(vm_t* vm, bool b);          // 布尔
var_t* var_new_null(vm_t* vm);                  // null
var_t* var_new_array(vm_t* vm);                 // 数组
var_t* var_new_obj(vm_t* vm, var_t* proto, ...);// 带原型的对象
var_t* var_new_func(vm_t* vm, func_t* func);    // 函数
```

注意 `var_new_int` 会给整数也设置原型（`Number.prototype`）：

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

这就是为什么脚本里能写 `(5).toString()`——整数也有 `Number` 的原型链。

## 7.2 成员即 node_t

对象的每个属性/方法/数组元素，都不是直接存 `var_t`，而是包一层 `node_t`（`mario.h`）：

```c
typedef struct st_node {
    uint32_t  magic: 8;          // 1 表示这是 node
    uint32_t  be_const : 8;      // 是常量（const 声明，不可改）
    uint32_t  be_inherited : 8;  // 是从原型继承来的
    uint32_t  be_unenumerable:4; // 不可枚举（for-in 跳过，如 prototype、native 方法）
    uint32_t  invisable : 4;     // 不可见（如 prototype 链接本身）
    uint32_t  ncache_instr;      // 成员访问缓存相关
    char*     name;              // 成员名（数组元素名为 NULL）
    var_t*    var;               // 指向实际的值
} node_t;
```

**为什么要多包一层 node？** 因为属性除了「值」之外还有一堆元信息：是不是常量、能不能被枚举、是不是继承来的。把这些放在 node 上，`var_t` 就能保持纯粹的「值」语义。

### 增删查成员

```c
node_t* var_add(var_t* var, const char* name, var_t* add);       // 加成员（存在则替换）
node_t* var_find_own_member(var_t* var, const char* name);       // 只查自身
node_t* var_find_member(var_t* obj, const char* name);           // 自身 + 原型链
var_t*  var_find_member_var(var_t* obj, const char* name);       // 同上，直接返回 var
```

`var_add` 用哈希表存储，查找是 O(1)：

```c
node_t* var_add(var_t* var, const char* name, var_t* add) {
    node_t* node = (name[0]!=0) ? var_find_raw(var, name) : NULL;
    if(node == NULL) {                          // 不存在 → 新建
        node = node_new(var->vm, name, add);
        hash_map_add(&var->children, name, node);
    } else if(add != NULL) {
        node_replace(node, add);                // 存在 → 替换值
    }
    return node;
}
```

### 数组

数组也是 `var_t`（`is_array=1`），它的元素以**无名字**的 `node_t` 存放，通过下标访问：

```c
node_t* var_array_get(var_t* var, int32_t index);   // 按下标取
node_t* var_array_add(var_t* var, var_t* add);      // 追加
uint32_t var_array_size(var_t* var);                // 长度
void var_array_reverse(var_t* var);                 // 反转（函数参数用）
```

`var_new_array` 会把元素挂在一个隐藏成员 `_ARRAY_` 下，并设 `be_unenumerable`。数组的原型是 `Array.prototype`，因此能调用 `push`/`length` 等内建方法（见第 9 章）。

## 7.3 原型链：Mario 版继承

Mario 的对象继承靠**原型（prototype）**实现，与 JavaScript 语义一致。

- 每个对象通过一个名为 `"prototype"` 的隐藏成员指向它的原型对象：

```c
var_t* var_get_prototype(var_t* var) { return get_obj(var, PROTOTYPE); }  // PROTOTYPE = "prototype"

void var_set_prototype(var_t* var, var_t* proto) {
    node_t* ret = var_add(var, PROTOTYPE, proto);
    ret->invisable = 1;            // 不可见
    ret->be_unenumerable = 1;      // for-in 不会遍历到它
}
```

- 查找成员时，先查自身，再沿原型链向上：

```c
node_t* var_find_member(var_t* obj, const char* name) {
    node_t* node = var_find_own_member(obj, name);   // ① 查自身
    if(node == NULL)
        node = vm_find_in_class(obj, name);          // ② 查原型链
    return node;
}

node_t* vm_find_in_class(var_t* var, const char* name) {
    var_t* proto = var_get_prototype(var);
    while(proto != NULL) {                            // 沿原型链上溯
        node_t* ret = var_find_own_member(proto, name);
        if(ret != NULL) {
            if(ret->var->is_func)
                return ret;                           // 方法：直接返回（共享）
            ret = var_add(var, name, var_clone(ret->var));  // 字段：克隆到自身
            ret->be_inherited = 1;
            return ret;
        }
        proto = var_get_prototype(proto);
    }
    return NULL;
}
```

注意区别：**方法**（函数）沿原型链共享同一份；**普通字段**第一次访问时会被克隆一份到对象自身（`be_inherited=1` 标记）。

### instanceof

`var_instanceof` 沿原型链检查是否遇到目标原型：

```c
bool var_instanceof(var_t* var, var_t* proto) {
    var_t* v = var_get_prototype(proto);  // 取类的 prototype
    if(v != NULL) proto = v;
    var_t* protov = var_get_prototype(var);
    while(protov != NULL) {               // 沿对象的原型链上溯
        if(protov == proto) return true;
        protov = var_get_prototype(protov);
    }
    return false;
}
```

## 7.4 类与继承的运行时表示

`class` 语句在运行时由 `handle_class` → `vm_new_class` 处理：

```c
var_t* vm_new_class(vm_t* vm, const char* cls) {
    node_t* n = vm_load_node(vm, cls, true);   // 在全局创建类名变量
    var_t* cls_var = n->var;
    cls_var->type = V_OBJECT;
    if(var_get_prototype(cls_var) == NULL)
        var_set_prototype(cls_var, var_new_obj_no_proto(vm, NULL, NULL));
    if(strcmp(cls, "Object") != 0)
        do_extends(vm, cls_var, "Object");      // 所有类默认继承 Object
    return cls_var;
}
```

**类本身是一个 `var_t`**，它有一个 prototype 对象，类里定义的方法都挂在这个 prototype 上。

`extends` 由 `do_extends` → `var_set_father` 实现，它把「子类的 prototype」的原型指向「父类的 prototype」，从而串起原型链：

```c
static void var_set_father(var_t* var, var_t* father) {
    var_t* proto = var_get_prototype(var);          // 子类的 prototype
    var_t* super_proto = var_get_prototype(father); // 父类的 prototype
    var_set_prototype(proto, super_proto);          // 子类proto.__proto__ = 父类proto
}
```

### new 的过程

`new Foo(a, b)` 由 `handle_new` → `do_new` → `new_obj` 完成：

```c
var_t* new_obj(vm_t* vm, const char* name, int arg_num) {
    node_t* n = vm_load_node(vm, name, false);        // 找到类
    var_t* protoV = var_get_prototype(n->var);        // 取类的 prototype
    obj = var_new_obj(vm, protoV, NULL, NULL);        // 新对象，原型 = 类的 prototype

    // 找构造函数：类本身是函数则用它，否则找 prototype 上的 constructor
    var_t* constructor = n->var->is_func ? n->var
                         : var_find_member_var(protoV, CONSTRUCTOR);
    if(constructor != NULL) {
        func_call(vm, obj, constructor, arg_num);     // 以 obj 为 this 调用构造函数
        obj = vm_pop2(vm);
    }
    return obj;
}
```

新对象的 `this` 就是它自己，构造函数往 `this` 上挂字段。这也解释了 `test/js/class.js` 里 `function f(){ this.age=18 } var a=new f()` 的写法——用函数当构造器。

## 7.5 函数：func_t

函数值（`is_func=1` 的 `var_t`）的 `value` 指向一个 `func_t`（`mario.h`）：

```c
typedef struct st_func {
    native_func_t  native;     // 原生函数：C 函数指针（脚本函数为 NULL）
    int8_t  regular: 4;        // 是否普通函数（get/set 为非普通）
    int8_t  is_static: 4;      // 是否静态
    int8_t  is_generator: 4;   // ES6 `function*` / 生成器方法
    int8_t  is_arrow: 4;       // ES6 箭头函数：词法 this、无 prototype、不可构造
    PC      pc;                // 脚本函数：函数体在字节码里的入口
    void*   data;              // 传给 native 的用户数据
    m_array_t args;            // 形参名列表
    var_t*  owner;             // 所属对象（用于 super）
    var_t*  owner_var;         // 拥有本 func_t 的 var_t 回指针（供 GC 沿词法链 root 住 func_t）
    struct { var_t* var; struct st_func* func; } closure;  // 闭包：捕获的环境
    var_t*  closure_func_ref;  // 对 closure.func 所属 owner var 的持有引用（防止外层函数被提前回收而悬空）
} func_t;
```

除了「脚本/原生」的区分，`func_t` 还用 `is_generator`、`is_arrow` 标记 ES6 的生成器函数与箭头函数（箭头函数捕获词法 `this`、无 `prototype`、不可 `new`）。`owner_var` 与 `closure_func_ref` 是为了让 GC 能沿闭包词法链正确 root 住 `func_t`，避免外层函数被提前回收后内层闭包的 `closure.func` 悬空（详见第 8 章）。

两种函数：

| 类型 | 判别 | 执行方式 |
| --- | --- | --- |
| **脚本函数** | `native == NULL` | `vm->pc = func->pc`，递归 `vm_run` 执行字节码 |
| **原生函数** | `native != NULL` | 直接调用 `func->native(vm, env, data)`（见第 9 章） |

脚本函数体的解析见 `func_def`：从 `FUNC` 指令后逐个读 `LOAD`（形参名），遇到 `JMP` 时记录函数体入口 `func->pc` 并跳过整段体。

## 7.6 闭包

当一个函数返回另一个函数时，内层函数需要「记住」外层的局部变量——这就是闭包。Mario 通过 `func_t.closure` 实现。

`handle_return` 里，如果返回的值是函数，会调用：

```c
static bool func_set_closure(var_t* var, var_t* closure, func_t* closure_func) {
    func_t* func = var_get_func(var);
    func->closure.var  = var_ref(closure);       // 捕获定义时的作用域环境（env）
    func->closure.func = closure_func;           // 捕获外层函数（形成闭包链）
    return true;
}
```

之后查找变量时（第 6 章的 `vm_find_in_scopes`），如果当前在函数作用域里，会**先沿闭包链查找**：

```c
if(sc != NULL && sc->is_func) {
    var_t* closure = sc->func->closure.var;
    func_t* closure_func = sc->func->closure.func;
    while(closure != NULL && closure_func != NULL) {
        ret = var_find_own_member(closure, name);   // 在被捕获的环境里找
        if(ret != NULL) return ret;
        closure = closure_func->closure.var;         // 沿闭包链继续向外
        closure_func = closure_func->closure.func;
    }
}
```

因为闭包用 `var_ref` 持有了外层 `env`，即使外层函数已返回，那些变量也不会被 GC 回收。`test/js/closure.js` 演示了这一机制。

## 7.7 全局对象 root

`vm->root` 是全局作用域的容器。所有全局变量、全局函数、内建类（`Object`/`String`/`Console`……）都作为成员挂在 `root` 下。`vm_find_in_scopes` 查到最后一步就是 `var_find_own_member(vm->root, name)`。

当你写 `console.log(...)`，`console` 就是在 `root` 里找到的一个对象（由 `reg_all_natives` 注册，见第 9 章）。

## 7.8 小结

| 概念 | Mario 的实现 |
| --- | --- |
| 值 | `var_t`（type + value + children） |
| 属性/元素 | `node_t`（name + var + 元标志） |
| 对象 | `is_array=0` 的 `var_t`，成员在 `children` |
| 数组 | `is_array=1` 的 `var_t`，元素为无名 node |
| 函数 | `is_func=1` 的 `var_t`，`value` 指向 `func_t` |
| 类 | `type=V_OBJECT` 的 `var_t`，方法挂在其 prototype |
| 继承 | 原型链：`prototype` 隐藏成员串起来 |
| 闭包 | `func_t.closure` 捕获定义环境，查找时沿闭包链 |
| 全局 | `vm->root` |

这些对象在内存里如何被创建、复用和回收？进入 [第 8 章 · 内存管理与垃圾回收](08-gc.md)。
