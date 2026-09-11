# 第 9 章 · Native 扩展与内建类

脚本本身只能做纯计算。要打印输出、操作字符串、处理 JSON、拿系统时间，就需要**原生（native）函数**——用 C 实现、注册进 VM、供脚本调用的函数。本章讲解如何编写和注册它们。

## 9.1 内建类总览

Mario 的 JS 前端自带一批内建类，分布在 [`lang/js/native/`](../../lang/js/native/)：

```
native/
├── builtin/            # 语言级基础类
│   ├── Object/         # Object
│   ├── Error/          # Error
│   ├── Array/          # Array：push/pop/length/...
│   ├── String/         # String：substring/indexOf/...
│   ├── Number/         # Number：toString(radix)
│   ├── Console/        # console.log / console.write
│   └── Promise/        # Promise
└── natives/            # 扩展类
    ├── JSON/           # JSON.stringify / JSON.parse
    ├── Date/           # Date
    └── Math/           # Math（默认未编入，见 lang.mk 注释）
```

注册入口分两级：

```c
// natives_all.c
void reg_all_natives(vm_t* vm) {
    reg_builtin_natives(vm);   // Object/Error/Array/String/Console/Number/Promise
    load_basic_classes(vm);    // 把内建类指针缓存进 vm->builtin_vars，并创建 console 对象
    reg_natives(vm);           // Date/JSON
}
```

`reg_all_natives` 就是第 2 章里传给 `vm_init` 的 `on_init` 回调——VM 初始化时自动把所有内建类注册好。

`load_basic_classes` 会把常用类的 `var_t*` 缓存进 `vm->builtin_vars`（`var_Object`/`var_String`/`var_Number`/`var_Array`/`var_Error`），并创建全局 `console` 对象：

```c
static inline void load_basic_classes(vm_t* vm) {
    vm->builtin_vars.var_Object = vm_load_var(vm, "Object", false);
    vm->builtin_vars.var_String = vm_load_var(vm, "String", false);
    ...
    var_t* console = new_obj(vm, "Console", 0);
    var_add(vm->root, "console", console);   // 全局 console
}
```

这就是脚本里能直接写 `console.log(...)` 的原因。

## 9.2 原生函数的签名

所有原生函数都是同一个 C 函数原型（`native_func_t`，见 `mario.h`）：

```c
typedef var_t* (*native_func_t)(struct st_vm* vm, var_t* env, void* data);
```

| 参数 | 含义 |
| --- | --- |
| `vm` | 当前虚拟机实例，用来创建返回值、访问内建类 |
| `env` | **调用环境对象**：里面有 `arguments` 数组、`this`、以及按形参名绑定的参数 |
| `data` | 注册时传入的用户数据指针（可存 C 侧上下文） |
| 返回值 | 一个 `var_t*`，作为函数返回值压栈；返回 `NULL` 视为返回 undefined |

## 9.3 从 env 取参数

`mario.h` 提供了一套便捷函数从 `env` 取数据：

```c
var_t*      get_func_args(var_t* env);              // 取 arguments 数组
uint32_t    get_func_args_num(var_t* env);          // 实参个数
var_t*      get_func_arg(var_t* env, uint32_t i);   // 第 i 个实参（var_t*）
int         get_func_arg_int(var_t* env, uint32_t i);
bool        get_func_arg_bool(var_t* env, uint32_t i);
float       get_func_arg_float(var_t* env, uint32_t i);
const char* get_func_arg_str(var_t* env, uint32_t i);

var_t*      get_obj(var_t* obj, const char* name);  // 按名字取成员
int         get_int(var_t* obj, const char* name);
const char* get_str(var_t* obj, const char* name);
```

`get_func_args(env)` 本质就是 `get_obj(env, "arguments")`——第 6 章讲过，`func_call` 会把实参打包成 `arguments` 数组放进 `env`。

按**形参名**取参数用 `get_obj(env, "参数名")` 或 `get_int(env, "参数名")`，这些名字来自注册时的声明字符串（见 9.5）。

## 9.4 一个完整例子：Console

看最简单的 [`native_Console.c`](../../lang/js/native/builtin/console/native_Console.c)：

```c
// 把 arguments 里所有参数转成字符串、空格拼接
static mstr_t* args_to_str(var_t* args) {
    mstr_t* ret = mstr_new("");
    uint32_t sz = var_array_size(args);
    for(uint32_t i=0; i<sz; ++i) {
        node_t* n = var_array_get(args, i);
        if(n != NULL) {
            var_to_str(n->var, str);       // 任意值 → 字符串
            if(i > 0) mstr_add(ret, ' ');
            mstr_append(ret, str->cstr);
        }
    }
    return ret;
}

var_t* native_println(vm_t* vm, var_t* env, void* data) {
    var_t* v = get_func_args(env);         // 取所有实参
    mstr_t* ret = args_to_str(v);
    mstr_add(ret, '\n');
    _platform_out(ret->cstr);              // 通过平台函数输出
    mstr_free(ret);
    return NULL;                           // 无返回值（undefined）
}

void reg_native_Console(vm_t* vm) {
    var_t* cls = vm_new_class(vm, "Console");
    vm_reg_static(vm, cls, "write(v)", native_print,   NULL);
    vm_reg_static(vm, cls, "log(v)",   native_println, NULL);
}
```

要点：
- 输出通过 `_platform_out`（第 1 章的平台层），而不是直接 `printf`，保证可移植。
- `var_to_str` 能把任意 `var_t`（数字/对象/数组）转成可读字符串。
- 返回 `NULL` 表示函数没有返回值。

## 9.5 注册 API 与声明字符串

三个注册函数（`mario.h` / `mario.c`）：

```c
node_t* vm_reg_var(vm_t* vm, var_t* cls, const char* name, var_t* var, bool be_const);
node_t* vm_reg_native(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data);
node_t* vm_reg_static(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data);
```

- `cls == NULL` 表示注册到**全局**（root）；否则注册到该类的 prototype 上。
- `vm_reg_native` 注册**实例方法**（要 `new` 出对象或用值原型调用，能访问 `this`）。
- `vm_reg_static` 注册**静态方法**（`func->is_static=true`，直接类名调用，如 `console.log`）。

### decl 声明字符串

`decl` 形如 `"方法名(参数1, 参数2)"`，`vm_reg_native` 会解析它：

```c
node_t* vm_reg_native(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data) {
    // 读 '(' 之前的部分作为方法名
    // 读括号内、以逗号分隔的部分作为形参名，存入 func->args
    func->native = native;
    func->data = data;
    // 挂到 cls 的 prototype（或 root）上
}
```

例如 `"toString(radix)"`：方法名 `toString`，一个形参 `radix`。于是在原生函数里可以用 `get_int(env, "radix")` 按名字取到这个参数。

> 声明里的形参名很重要：`func_call` 会按 `func->args` 里的名字，把实参绑定到 `env` 上。这样原生函数既能按名字取参（`get_obj(env,"radix")`），也能按下标取参（`get_func_arg(env,0)`）。

## 9.6 实例方法与 this：Number.toString

看 [`native_Number.c`](../../lang/js/native/builtin/number/native_Number.c) 的实例方法，它需要访问调用者（`this`）：

```c
var_t* native_Number_toString(vm_t* vm, var_t* env, void* data) {
    var_t* v = get_obj(env, THIS);         // THIS = "this"，取调用者
    if(v->type == V_INT) {
        int radix = get_int(env, "radix"); // 按形参名取参数
        if(radix < 2 || radix > 36) radix = 10;
        s = mstr_from_int(var_get_int(v), radix);
    } else {
        s = mstr_from_float(var_get_float(v));
    }
    return var_new_str(vm, s);             // 返回新字符串
}

void reg_native_Number(vm_t* vm) {
    var_t* cls = vm_new_class(vm, "Number");
    vm_reg_native(vm, cls, "toString(radix)",     native_Number_toString,     NULL);
    vm_reg_native(vm, cls, "constructor(value)",  native_Number_constructor,  NULL);
}
```

回顾第 7 章：`var_new_int` 会给整数设置 `Number.prototype` 作为原型。所以当脚本写 `(255).toString(16)` 时，整数沿原型链找到 `Number.toString`，`func_call` 把这个整数作为 `this` 放进 `env`，原生函数就能读到它并转成十六进制字符串 `"ff"`。

`constructor` 是特殊方法名（`CONSTRUCTOR = "constructor"`）：`new Number(5)` 时，`new_obj` 会找到 prototype 上的 `constructor` 并以新对象为 `this` 调用它。

## 9.7 编写你自己的原生类

假设要加一个 `Math.abs(x)`：

```c
// native_math_abs.c
#include "mario.h"

static var_t* native_abs(vm_t* vm, var_t* env, void* data) {
    int x = get_func_arg_int(env, 0);      // 第 0 个实参
    return var_new_int(vm, x < 0 ? -x : x);
}

void reg_native_MyMath(vm_t* vm) {
    var_t* cls = vm_new_class(vm, "Math");
    vm_reg_static(vm, cls, "abs(x)", native_abs, NULL);  // 静态方法
}
```

接入步骤：

1. 在 [`lang/js/lang.mk`](../../lang/js/lang.mk) 的 `NATIVE_OBJS` 里加上你的 `.o`（Math 目录已有示例，默认被注释掉了）。
2. 在 `reg_natives()`（`natives.c`）或 `reg_builtin_natives()` 里调用 `reg_native_MyMath(vm)`。
3. `make` 重新编译。

脚本里即可使用：

```javascript
console.log(Math.abs(-42));   // 输出 42
```

## 9.8 全局函数与全局变量

注册到 `root`（`cls == NULL`）就是全局的：

```c
// 全局函数 print()（见 mario/demos/tinyjs/demo.c）
vm_reg_static(vm, NULL, "print()", native_print, NULL);

// 全局变量
vm_reg_var(vm, NULL, "VERSION", var_new_int(vm, 3), true);  // const
```

命令行程序还用 `var_add(vm->root, "_args", args)` 把命令行参数注入为全局数组 `_args`。

## 9.9 小结

- 原生函数统一签名 `var_t* f(vm_t* vm, var_t* env, void* data)`。
- 参数从 `env` 取：按名 `get_obj/get_int(env,"name")`，或按下标 `get_func_arg(env,i)`；`this` 用 `get_obj(env, THIS)`。
- 注册用 `vm_reg_native`（实例方法）/ `vm_reg_static`（静态方法）/ `vm_reg_var`（变量），`decl` 字符串声明名字与形参。
- 内建类通过 `reg_all_natives` 在 VM 初始化时自动装载。
- 扩展只需：写原生函数 → 注册 → 加进 `lang.mk` → 重新 `make`。

最后一章 [第 10 章 · 字节码文件与工具链](10-mbc-and-tools.md)，讲预编译文件与如何把 Mario 嵌入你自己的程序。
