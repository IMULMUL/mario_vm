# 第 10 章 · 字节码文件与工具链

本章介绍 Mario 的「周边设施」：预编译字节码文件 `.mbc`、模块 `include` 机制，以及如何把 VM 嵌入到你自己的 C 程序里。

## 10.1 为什么要预编译字节码

编译（词法 + 语法分析）是有成本的。在嵌入式设备上，如果每次启动都重新解析同一段脚本，既慢又占内存。**预编译**的思路是：在开发机上把 `.js` 编译成二进制字节码 `.mbc`，设备上直接加载 `.mbc`，跳过整个编译阶段。

对应的命令行用法（第 2 章已提及）：

```bash
./build/mario -c app.js        # 生成 app.mbc
./build/mario app.mbc          # 直接加载运行，不需要编译器
```

实现在 [`bin/lib/mbc.c`](../../../bin/lib/mbc.c)。

## 10.2 .mbc 文件格式

`.mbc` 就是把 `bytecode_t` 的两部分（字符串表 + 指令数组）序列化到文件。格式如下：

```
┌────────────────────────────────────────┐
│ MAGIC_NO   (4 字节) = 0x19760427         │  魔数，校验文件合法性
│ VERSION    (4 字节) = 0x00000001         │  版本号
├────────────────────────────────────────┤
│ mstr_count (4 字节)                      │  字符串表条目数
│   ┌ len (4字节) │ 字符串内容 (len 字节) ┐  │  逐条：长度 + 内容（无结尾 \0）
│   └ ... 重复 mstr_count 次 ...          ┘  │
├────────────────────────────────────────┤
│ code_size  (4 字节) = 指令数 × 4          │  指令区总字节数
│   指令数组 (code_size 字节)               │  每条 4 字节，直接内存镜像
└────────────────────────────────────────┘
```

### 写入：gen_mbc

```c
static bool gen_mbc(int fd, vm_t* vm) {
    write(fd, MAGIC_NO);  write(fd, VERSION);
    // 字符串表：先写数量，再逐条写 [长度][内容]
    PC sz = vm->bc.mstr_table.size;
    write(fd, sz);
    for(i=0; i<sz; ++i) {
        uint32_t len = strlen(str);
        write(fd, len);  write(fd, str, len);
    }
    // 指令区：直接写 code_buf 的原始字节
    sz = vm->bc.cindex * 4;
    write(fd, sz);  write(fd, code_buf, sz);
}
```

### 读取：load_mbc

```c
static bool load_mbc(int fd, vm_t* vm) {
    read(fd, &i, 4);  if(i != MAGIC_NO) return false;   // 校验魔数
    read(fd, &version, 4);
    read(fd, &sz, 4);                                    // 字符串表
    for(i=0; i<sz; ++i) {
        read(fd, &len, 4);
        char* s = mario_malloc(len+1);
        read(fd, s, len);  s[len] = 0;
        array_add(&vm->bc.mstr_table, s);
    }
    read(fd, &sz, 4);                                    // 指令区
    vm->bc.code_buf = mario_malloc(sz);
    vm->bc.cindex = sz/4;
    read(fd, code_buf, sz);
}
```

读取后，`vm->bc` 就和一个刚编译完的字节码完全一样，可直接 `vm_run`。

> 注意：`.mbc` 是**平台相关**的二进制格式——指令是 4 字节整数直接镜像，字符串长度用 `uint32_t`。在不同字节序或不同 `PC` 宽度的平台之间不能直接通用。开发机与目标机架构一致时才安全。

命令行程序在加载 `.mbc` 前会先 `bc_release(&vm->bc)` 清空（因为 `vm_new` 时可能已有编译产物），再 `vm_load_mbc`。

## 10.3 模块引入：include

脚本里可以用 `include "xxx.js"` 引入其它脚本（对应 `INSTR_INCLUDE`）。加载逻辑在 [`bin/lib/js.c`](../../../bin/lib/js.c)：

```c
#define DEF_LIBS "/usr/local/mario"

static mstr_t* include_script(vm_t* vm, const char* name) {
    const char* path = getenv("MARIO_PATH");   // 环境变量指定库目录
    if(path == NULL) path = DEF_LIBS;          // 默认 /usr/local/mario
    // 先按当前路径 load_script_content(name)
    // 找不到再拼 $MARIO_PATH/libs/<lang>/name（<lang> 取自 _mario_lang，如 "js"）
}
```

VM 通过全局函数指针 `_load_m_func` 回调到 `include_script`。运行时的 `do_include`（`mario.c`）会：

1. 检查 `vm->included` 列表，避免重复引入同一模块；
2. 调用 `_load_m_func` 读取脚本内容；
3. 保存当前 `pc`，`vm_load_run` 执行被引入的脚本，再恢复 `pc`。

`include` 在**编译期**只是生成一条 `INSTR_INCLUDE` 指令，真正的加载发生在**运行时**，所以被引入的脚本会在执行到那一行时才编译运行。

## 10.4 把 Mario 嵌入你的 C 程序

这是 Mario 最重要的用法——作为脚本引擎嵌入到应用里。最小骨架（参考 [`mario/demos/js_call/demo.c`](../../../mario/demos/js_call/demo.c)）：

```c
#include "mario.h"

// ① 实现平台函数
static void out(const char* str) { printf("%s", str); }
void platform_init(void) {
    _platform_malloc = malloc;
    _platform_free   = free;
    _platform_out    = out;
}

// ② 声明语言编译器（由 lang 层提供）
bool compile(bytecode_t* bc, const char* input);

int main(void) {
    platform_init();

    // ③ 创建并初始化 VM
    vm_t* vm = vm_new(compile, VAR_CACHE_MAX_DEF, LOAD_NCACHE_MAX_DEF);
    vm_init(vm, NULL, NULL);          // 也可传 on_init 注册你自己的 native

    // ④ 加载并运行脚本
    const char* js = "function jsFunc(s, n){ return \"Hello '\"+s+\"' (\"+n+\")!\\n\"; }";
    vm_load_run(vm, js);

    // ⑤ 从 C 调用脚本函数
    var_t* ret = call_m_func_by_name(vm, NULL, "jsFunc", 2,
                     var_new_str(vm, "JS world"),
                     var_new_int(vm, 100));
    if(ret != NULL) {
        mario_printf("%s", var_get_str(ret));
        var_unref(ret);
    }

    vm_close(vm);   // ⑥ 释放
    return 0;
}
```

### 关键 API 速查

| 阶段 | 函数 | 说明 |
| --- | --- | --- |
| 创建 | `vm_new(compiler, var_cache_size, ncache_size)` | 挂上编译器，创建 VM |
| 初始化 | `vm_init(vm, on_init, on_close)` | 注册 native、内建类的回调 |
| 加载 | `vm_load(vm, s)` | 只编译 |
| 运行 | `vm_load_run(vm, s)` | 编译 + 运行 |
| 运行 | `vm_run(vm)` | 运行已加载的字节码 |
| 加载字节码 | `vm_load_mbc(vm, file)` | 从 `.mbc` 加载 |
| 生成字节码 | `vm_gen_mbc(vm, file)` | 编译结果存为 `.mbc` |
| 调 JS 函数 | `call_m_func_by_name(vm, obj, name, argc, ...)` | 从 C 调脚本函数 |
| 注册 native | `vm_reg_static/native/var(...)` | 见第 9 章 |
| 关闭 | `vm_close(vm)` | 触发 on_close、回收资源 |

### C ↔ JS 数据交换

- **C 调 JS**：`call_m_func_by_name`，可变参数传 `var_t*`，返回 `var_t*`（用完 `var_unref`）。
- **JS 调 C**：注册原生函数（第 9 章），JS 侧像调普通函数一样调用。
- **读写对象成员**：`get_obj_member(obj, name)` / `set_obj_member(obj, name, var)`；便捷取值 `get_int/get_str/get_float/get_bool`。

## 10.5 反汇编工具

- **bcdump**（[`mario/bcdump/bcdump.c`](../../../mario/bcdump/bcdump.c)）：把 `bytecode_t` 反汇编成可读文本，即命令行 `-a` 的输出（详见第 3 章）。核心是 `bc_dump(bc)`，返回一个 `mstr_t*`。demo 程序通过 `#include "bcdump/bcdump.h"` 使用它。

> 说明：仓库当前只提供反汇编（dump）工具，尚未包含字节码汇编器（assembler）。若需从文本指令重建 `.mbc`，可参考 `bc_dump` 的格式自行实现。

调试建议：写一段脚本 → `mario -a` 看它编译成什么指令 → 对照第 3、5 章理解 → 再用 `-c`/`.mbc` 验证加载路径。这是排查编译/执行问题最有效的工作流。

## 10.6 端到端串讲

现在把十章串起来，看 `console.log(1+2)` 完整的一生：

```
1. main 读取源码字符串                          （第 2 章）
2. vm_new 挂上 js_compile，vm_init 注册 Console  （第 1、9 章）
3. js_compile:
     lex_get_next_token 逐个切 token            （第 4 章）
     statement→base→...→factor 递归下降          （第 5 章）
     bc_gen_str/bc_gen 生成指令，字符串入 mstr_table（第 3 章）
   得到： LOAD "console" / INTS 1 / INTS 2 / PLUS / CALLO "log$1" / POP / END
4. vm_run 主循环取指-分发:                       （第 6 章）
     LOAD → 在 root 找到 console 对象压栈
     INTS/PLUS → 操作数栈算出 3
     CALLO → handle_call 找到 Console.log，func_call 调 native_println
              native 从 env 取 arguments，_platform_out 输出 "3\n"  （第 9 章）
     期间创建的临时 var_t 由引用计数/GC 回收      （第 8 章）
5. vm_close 释放                                 （第 10 章）
```

所有操作的都是 `var_t`/`node_t` 对象模型（第 7 章）。

---

## 结语

恭喜你读完了整套 Wiki！你现在应该能够：

- 看懂任意一段 Mario 字节码（`mario -a`）；
- 理解一门语言从文本到执行的完整链路；
- 用 C 编写原生函数扩展脚本能力；
- 把 Mario 嵌入自己的程序，并做 C↔JS 数据交换；
- 理解小型 VM 如何在资源受限环境做自动内存管理。

进一步学习建议：

1. **动手改**：给编译器加一个新语法（如正则字面量 `/.../`），给内建类加一个新方法（如 `String.prototype.reverse`）。
2. **单步看**：用 `MARIO_DEBUG=yes` 编译，配合 `mario_debug` 观察执行流。
3. **读测试**：`test/js/*.js` 是最好的行为规约，读它们并预测字节码，再用 `-a` 验证。尤其是 [`test/js/es6_full.js`](../../../test/js/es6_full.js)，它是完整的 ES6+ 行为规约。

作为一门 JavaScript，Mario 到底支持哪些 ES6+ 语法与内建对象？完整的特性矩阵、代码示例与已知限制见 [第 11 章 · ES6+ 语言特性支持](11-es6-support.md)。

回到 [Wiki 首页](README.md)。
