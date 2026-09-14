# 第 2 章 · 快速开始：构建与运行

本章带你把 Mario 跑起来，并认识命令行工具的几种用法。

## 2.1 构建

项目使用 GNU Make。在仓库根目录执行：

```bash
make
```

成功后会在 `build/` 目录生成可执行文件 `build/mario`，并输出 `done`。

构建过程由根目录的 [`Makefile`](../../Makefile) 与 [`lang/js/lang.mk`](../../lang/js/lang.mk) 共同驱动：

- `Makefile` 负责编译内核（`mario/mario.o`、`mario/lex/mario_lex.o`、`mario/bcdump/bcdump.o`）与命令行程序（`bin/mario/*`）。
- `lang.mk` 负责列出 JS 语言层与所有内建 native 类的目标文件。

清理构建产物：

```bash
make clean
```

### 打开调试（可选）

```bash
make MARIO_DEBUG=yes
```

这会加上 `-g -DMARIO_DEBUG`，并启用 `bin/lib/mem_debug.c` 里的内存调试分配器（`malloc_debug`/`free_debug`），用于排查内存泄漏与越界。

## 2.2 运行一个脚本

最简单的用法，直接把 `.js` 文件作为参数：

```bash
./build/mario test/js/class.js
```

程序会读取文件 → 编译成字节码 → 执行。`test/js/` 下有若干示例脚本可以直接尝试：

| 文件 | 演示内容 |
| --- | --- |
| `test/js/class.js` | 函数、类、继承、`super`、`for...in` |
| `test/js/closure.js` | 闭包 |
| `test/js/promise.js` | Promise |
| `test/js/string.js` | 字符串内建方法 |
| `test/js/grammar_es5.js` | ES5 语法综合 |
| `test/js/bench.js` | 性能基准 |

## 2.3 命令行参数

参数解析在 [`bin/mario/mario.c`](../../bin/mario/mario.c) 的 `doargs()` 里，通过 `getopt` 处理：

```
Usage: mario (-c/d/a) <filename> [output]
```

| 参数 | 含义 |
| --- | --- |
| 无参数 | **运行模式**：编译并执行脚本 |
| `-a` | **dump 模式**：只编译，把字节码反汇编成可读文本打印出来，不执行 |
| `-c` | **编译模式**：只编译，把字节码写入 `.mbc` 文件（预编译） |

> 说明：源码里还定义了 `-d`，但当前 `doargs()` 只对 `c`/`a` 做了处理。

## 2.4 查看字节码：`-a`

这是学习 Mario 最有用的功能。写一个最小脚本：

```javascript
// /tmp/t.js
var a = 1 + 2;
console.log(a);
```

执行：

```bash
./build/mario -a /tmp/t.js
```

得到真实输出：

```
mstr_index| value
---------------------------------------
0x000000 | this
0x000001 | a
0x000002 | console
0x000003 | log$1

pc_index | opr_code   ; instruction
---------------------------------------
00000000 | 0x00100001 ; VAR     "a"
00000001 | 0x00300001 ; LOAD    "a"
00000002 | 0x00D00001 ; INTS    1
00000003 | 0x00D00002 ; INTS    2
00000004 | 0x01EFFFFF ; PLUS
00000005 | 0x004FFFFF ; ASIGN
00000006 | 0x04AFFFFF ; POP
00000007 | 0x00300002 ; LOAD    "console"
00000008 | 0x00300001 ; LOAD    "a"
00000009 | 0x01300003 ; CALLO   "log$1"
00000010 | 0x04AFFFFF ; POP
00000011 | 0x058FFFFF ; END
---------------------------------------
```

输出分成两部分：

1. **字符串表（mstr_table）**：所有出现过的标识符/字符串常量，指令用「下标」引用它们，避免在指令里内嵌字符串。
2. **指令序列**：每一行是 `pc | 机器码 ; 助记符 操作数`。

先别急着理解每一列，第 3 章会把这个 32 位机器码拆开讲清楚。这里先建立一个直觉：

- `VAR "a"` 声明变量 `a`；
- `LOAD "a"` 把 `a` 压栈（作为赋值目标）；
- `INTS 1` / `INTS 2` 把两个整数压栈；
- `PLUS` 弹出两个数、相加、压回；
- `ASIGN` 把栈顶值赋给下面的目标；
- `POP` 丢弃语句结果；
- `CALLO "log$1"` 调用对象成员方法 `log`（`$1` 表示 1 个参数）；
- `END` 结束。

## 2.5 预编译：`-c`

把脚本编译成 `.mbc` 二进制字节码文件，之后可以直接加载运行，省去重复编译：

```bash
# 生成 /tmp/t.mbc
./build/mario -c /tmp/t.js

# 直接运行预编译字节码
./build/mario /tmp/t.mbc
```

`.mbc` 文件的读写实现在 [`bin/lib/mbc.c`](../../bin/lib/mbc.c)，格式细节见第 10 章。

## 2.6 常见运行问题

- **`Failed to create VM...`**：说明三个平台函数指针（`_platform_malloc`/`_platform_free`/`_platform_out`）没有设置。命令行程序已在 `platform_init()` 里设好；如果你在自己程序里嵌入 Mario，务必先设置它们（见第 10 章）。
- **`compile error at (line: X, col: Y)`**：编译期语法错误，位置由 `compile_error_pos()` 通过词法分析器计算得出。
- **脚本没有输出**：确认调用了 `console.log(...)`，它由内建的 `Console` 类提供（见第 9 章）。

下一步：[第 3 章 · 字节码指令集详解](03-bytecode.md)，彻底搞懂上面那串机器码。
