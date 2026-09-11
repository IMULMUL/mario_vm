# 第 3 章 · 字节码指令集详解

字节码是编译器和虚拟机之间的「契约」。编译器把源码翻译成字节码，虚拟机逐条执行字节码。本章彻底讲清楚这套契约。

所有指令常量都定义在 [`mario/mario.h`](../../mario/mario.h) 的 `MARIO_BC` 区块。

## 3.1 一条指令 = 一个 32 位整数

Mario 的每条指令都是一个 `uint32_t`（类型别名 `PC`）。它被切成三段：

```
 31        28 27            20 19                        0
┌────────────┬────────────────┬────────────────────────────┐
│  OPTION    │    OPR_CODE    │      OFFSET / VALUE         │
│  (4 bits)  │    (8 bits)    │        (20 bits)            │
└────────────┴────────────────┴────────────────────────────┘
```

对应的宏（见 `mario.h`）：

```c
#define OFF_MASK 0x0FFFFF                                   // 低 20 位掩码
#define INS(ins, off) ((((int32_t)ins)<<20)&0xFFF00000) | ((off)&OFF_MASK))
#define OP(ins)  (((ins) >> 20) & 0xFF)   // 取出操作码（8 位）
#define OFF(ins) ((ins) & OFF_MASK)       // 取出操作数（20 位）
```

- **OPR_CODE（操作码）**：指明这条指令做什么，例如 `INSTR_PLUS`（加法）、`INSTR_CALL`（调用函数）。
- **OFFSET / VALUE（操作数）**：一个 20 位的字段，含义随指令而变：
  - 对 `LOAD`/`VAR` 等：它是**字符串表下标**（指向变量名）；
  - 对 `JMP`/`NJMP` 等：它是**跳转偏移量**；
  - 对 `INTS`（短整数）：它直接就是**整数值**；
  - 对无操作数指令（如 `PLUS`）：它被填成 `OFF_MASK`（全 1，即 `0xFFFFF`）。
- **OPTION（选项位）**：最高 4 位，目前主要用 `INSTR_OPT_CACHE`（`0x80000000`）标记「已被运行时优化」的指令（见 6.x 内联缓存）。

### 为什么是 20 位操作数

20 位意味着字符串表最多约 100 万个条目、跳转偏移最大约 ±100 万条指令，对小型脚本绰绰有余，同时把整条指令压缩进一个机器字，取值/解码都非常快。

### 回看第 2 章的例子

```
00000002 | 0x00D00001 ; INTS    1
```

拆开 `0x00D00001`：
- `OP = (0x00D00001 >> 20) & 0xFF = 0x0D` → `INSTR_INT_S`（短整数，dump 里显示 `INTS`）；
- `OFF = 0x00D00001 & 0xFFFFF = 1` → 整数值就是 `1`。

再看：

```
00000004 | 0x01EFFFFF ; PLUS
```
- `OP = 0x1E` → `INSTR_PLUS`；
- `OFF = 0xFFFFF`（全 1）→ 无操作数。

## 3.2 字符串表 mstr_table

指令里不直接存字符串，而是存字符串在 `mstr_table` 里的**下标**。这张表保存在 `bytecode_t.mstr_table`（一个 `m_array_t`）。

```c
typedef struct st_bytecode {
    PC          cindex;       // 已生成的指令条数（也是下一条写入位置）
    m_array_t   mstr_table;   // 字符串表
    PC*         code_buf;     // 指令数组
    uint32_t    buf_size;     // code_buf 容量
} bytecode_t;
```

好处：同一个标识符（如 `console`）在多处使用时只存一份；指令定长；`.mbc` 文件体积小。

相关辅助函数（`mario.c`）：

- `bc_getstrindex(bc, str)`：取得字符串下标，若不存在则插入表尾再返回下标（去重）。
- `bc_getstr(bc, i)`：由下标取回字符串（宏，越界返回 `""`）。

## 3.3 字节码是如何「生成」的

编译器不会一次性知道所有跳转目标，所以提供了一组生成/回填函数（`mario.h` 声明，`mario.c` 实现）：

| 函数 | 作用 |
| --- | --- |
| `bc_gen(bc, instr)` | 追加一条无操作数指令，返回它的 pc |
| `bc_gen_str(bc, instr, s)` | 追加一条以字符串下标为操作数的指令 |
| `bc_gen_int(bc, instr, i)` | 追加 `INT` + 紧随其后的一个 32 位整数（占两条） |
| `bc_gen_short(bc, instr, s)` | 追加短整数指令（值直接放进 20 位操作数） |
| `bc_reserve(bc)` | 预留一个空槽位，稍后回填 |
| `bc_set_instr(bc, anchor, op, target)` | 回填：把 `anchor` 处设为跳转到 `target` |
| `bc_add_instr(bc, anchor, op, target)` | 在末尾追加，并以 `anchor` 为基准计算相对偏移 |
| `bc_remove_instr(bc, from, num)` | 删除若干条指令 |

**回填（back-patching）**是理解编译器的关键：编译 `if` 时还不知道「条件为假要跳到哪」，于是先 `bc_reserve()` 占位，等 `then` 分支编译完、知道了目标地址，再用 `bc_set_instr()` 把跳转偏移填进去。

### INT / FLOAT 是双字指令

普通整数/浮点值可能超过 20 位，无法塞进单条指令的操作数。于是：

- `INSTR_INT`：占**两条**——第一条是指令本身，第二条 `code_buf[pc+1]` 存放完整的 32 位整数。
- `INSTR_FLOAT`：同样占两条，第二条以位拷贝方式存放 `float`。
- `INSTR_INT_S`（短整数）：值能放进 20 位时用它，只占**一条**，更省空间。dump 里显示为 `INTS`。

这就是为什么 `bcdump.c` 在遇到 `INSTR_INT`/`INSTR_FLOAT` 时会多读一行。

## 3.4 指令全集

下面按功能分类列出全部指令。操作码值取自 `mario.h`，「栈行为」描述执行时对操作数栈的影响。

### 3.4.1 常量与字面量

| 指令 | 码 | 栈行为 | 说明 |
| --- | --- | --- | --- |
| `INSTR_INT` | 0x007 | push int | 压入 32 位整数（双字） |
| `INSTR_INT_S` | 0x00D | push int | 压入短整数（单字，值在操作数里） |
| `INSTR_FLOAT` | 0x008 | push float | 压入浮点数（双字） |
| `INSTR_STR` | 0x009 | push string | 压入字符串（操作数是字符串表下标） |
| `INSTR_TRUE` | 0x043 | push true | 压入布尔真 |
| `INSTR_FALSE` | 0x044 | push false | 压入布尔假 |
| `INSTR_NULL` | 0x045 | push null | 压入 null |
| `INSTR_UNDEF` | 0x046 | push undefined | 压入 undefined |

### 3.4.2 变量与存取

| 指令 | 码 | 说明 |
| --- | --- | --- |
| `INSTR_VAR` | 0x001 | 声明变量 `x`（`var`） |
| `INSTR_SAFE_VAR` | 0x00E | 声明块级变量 `x`（`let`） |
| `INSTR_CONST` | 0x002 | 声明常量 `x`（`const`，不可再赋值） |
| `INSTR_LOAD` | 0x003 | 加载变量 `x` 并压栈（也用作赋值左值、成员访问的基对象） |
| `INSTR_STORE` | 0x005 | 弹栈并存入 `x` |
| `INSTR_GET` | 0x006 | 取对象成员字段（getfield） |
| `INSTR_ASIGN` | 0x004 | 赋值 `=`：弹出值与目标节点，写入 |
| `INSTR_POP` | 0x04A | 弹出栈顶并释放（丢弃语句结果） |

> `LOAD` 与 `GET` 的区别：`LOAD` 用于从作用域链查找变量或作为「取成员」的基对象；`GET` 用于 `.` 之后按名字取栈顶对象的成员。

### 3.4.3 算术运算（弹出操作数，压入结果）

| 指令 | 码 | 运算 |
| --- | --- | --- |
| `INSTR_PLUS` | 0x01E | `+`（数字相加 / 字符串拼接） |
| `INSTR_MINUS` | 0x01F | `-` |
| `INSTR_MULTI` | 0x01B | `*` |
| `INSTR_DIV` | 0x01C | `/` |
| `INSTR_MOD` | 0x01D | `%` |
| `INSTR_NEG` | 0x020 | 一元负号 `-x` |
| `INSTR_PPLUS` | 0x021 | 后缀 `x++` |
| `INSTR_MMINUS` | 0x022 | 后缀 `x--` |
| `INSTR_PPLUS_PRE` | 0x023 | 前缀 `++x` |
| `INSTR_MMINUS_PRE` | 0x024 | 前缀 `--x` |

### 3.4.4 复合赋值

| 指令 | 码 | 运算 |
| --- | --- | --- |
| `INSTR_PLUSEQ` | 0x02E | `+=` |
| `INSTR_MINUSEQ` | 0x02F | `-=` |
| `INSTR_MULTIEQ` | 0x030 | `*=` |
| `INSTR_DIVEQ` | 0x031 | `/=` |
| `INSTR_MODEQ` | 0x032 | `%=` |

### 3.4.5 位运算

| 指令 | 码 | 运算 |
| --- | --- | --- |
| `INSTR_LSHIFT` | 0x025 | `<<` |
| `INSTR_RSHIFT` | 0x026 | `>>` |
| `INSTR_URSHIFT` | 0x027 | `>>>`（无符号右移） |
| `INSTR_OR` | 0x035 | `\|` |
| `INSTR_XOR` | 0x036 | `^` |
| `INSTR_AND` | 0x037 | `&` |

### 3.4.6 比较与逻辑

| 指令 | 码 | 运算 |
| --- | --- | --- |
| `INSTR_EQ` | 0x028 | `==` |
| `INSTR_NEQ` | 0x029 | `!=` |
| `INSTR_TEQ` | 0x038 | `===`（严格相等） |
| `INSTR_NTEQ` | 0x039 | `!==` |
| `INSTR_LES` | 0x02D | `<` |
| `INSTR_LEQ` | 0x02A | `<=` |
| `INSTR_GRT` | 0x02C | `>` |
| `INSTR_GEQ` | 0x02B | `>=` |
| `INSTR_NOT` | 0x01A | `!` |
| `INSTR_AAND` | 0x033 | `&&`（短路） |
| `INSTR_OOR` | 0x034 | `\|\|`（短路） |
| `INSTR_TYPEOF` | 0x03A | `typeof` |
| `INSTR_INSTOF` | 0x055 | `instanceof` |

### 3.4.7 跳转与控制流

跳转指令的操作数是**相对偏移**，分「向前跳」和「向后跳」两组：

| 指令 | 码 | 说明 |
| --- | --- | --- |
| `INSTR_JMP` | 0x042 | 无条件向前跳 `offset` |
| `INSTR_JMPB` | 0x040 | 无条件向后跳 `offset`（Back） |
| `INSTR_NJMP` | 0x03F | 条件为假则向前跳（Not JMP） |
| `INSTR_NJMPB` | 0x041 | 条件为假则向后跳 |
| `INSTR_BREAK` | 0x03B | `break`：跳出最近的循环作用域 |
| `INSTR_CONTINUE` | 0x03C | `continue`：跳到循环的继续锚点 |
| `INSTR_RETURN` | 0x03D | 无返回值返回 |
| `INSTR_RETURNV` | 0x03E | 带返回值返回 |

跳转的具体语义（`handle_jmp` 等，见 `mario.c`）：

```c
handle_jmp :  vm->pc = vm->pc + offset - 1;   // 向前
handle_jmpb:  vm->pc = vm->pc - offset - 1;   // 向后
handle_njmp:  弹出栈顶；若为假 → 按方向跳转，否则顺序执行
```

`-1` 是因为取指令时 `pc` 已经自增过。

### 3.4.8 函数与调用

| 指令 | 码 | 说明 |
| --- | --- | --- |
| `INSTR_FUNC` | 0x00F | 定义函数 |
| `INSTR_FUNC_STC` | 0x019 | 定义静态函数（`static`） |
| `INSTR_FUNC_GET` | 0x010 | 定义类的 getter |
| `INSTR_FUNC_SET` | 0x011 | 定义类的 setter |
| `INSTR_CALL` | 0x012 | 调用普通函数并压入返回值 |
| `INSTR_CALLO` | 0x013 | 调用对象成员方法（`obj.x()`） |
| `INSTR_NEW` | 0x047 | `new` 创建对象 |

**函数名的编码约定**：调用指令的操作数指向一个形如 `名字$参数个数` 的字符串，例如 `log$1` 表示「函数 `log`，1 个参数」。解析函数是 `parse_func_name()`：

```c
// "log$1" → name="log", 返回 1
static int parse_func_name(const char* full, mstr_t* name);
```

这样做是因为 Mario 支持基于参数个数的简单重载/分派。

**函数体的组织**：`FUNC` 指令后面紧跟若干 `LOAD`（把形参名依次压入 `func->args`），再跟一条 `JMP`（跳过函数体，因为定义函数时不执行它），`JMP` 之后才是函数体字节码，以 `RETURN`/`RETURNV` 结束。`func_def()` 负责在运行时把这段解析成 `func_t`。

### 3.4.9 类与对象

| 指令 | 码 | 说明 |
| --- | --- | --- |
| `INSTR_CLASS` | 0x014 | 开始定义类 |
| `INSTR_CLASS_END` | 0x015 | 类定义结束 |
| `INSTR_EXTENDS` | 0x018 | 继承父类 |
| `INSTR_MEMBER` | 0x016 | 匿名成员（数组元素） |
| `INSTR_MEMBERN` | 0x017 | 具名成员（对象属性 / 类方法） |
| `INSTR_OBJ` | 0x04B | 开始 JSON 风格对象字面量 `{}` |
| `INSTR_OBJ_END` | 0x04C | 对象字面量结束 |
| `INSTR_ARRAY` | 0x00B | 开始数组字面量 `[]` |
| `INSTR_ARRAY_END` | 0x00C | 数组字面量结束 |
| `INSTR_ARRAY_AT` | 0x00A | 取 `arr[i]` 元素 |

### 3.4.10 作用域块

这些指令成对出现，运行时用于压入/弹出一层作用域（`scope_t`）：

| 指令 | 码 | 说明 |
| --- | --- | --- |
| `INSTR_BLOCK` / `INSTR_BLOCK_END` | 0x04D / 0x04E | 普通代码块 `{}` |
| `INSTR_LOOP` / `INSTR_LOOP_END` | 0x04F / 0x050 | 循环体（记录 break/continue 锚点） |
| `INSTR_TRY` / `INSTR_TRY_END` | 0x051 / 0x052 | try 块（记录 catch 锚点） |
| `INSTR_CATCH` | 0x054 | 把抛出的值绑定到 catch 变量 |
| `INSTR_THROW` | 0x053 | 抛出：回溯作用域栈找到最近的 try |

### 3.4.11 其它

| 指令 | 码 | 说明 |
| --- | --- | --- |
| `INSTR_NIL` | 0x000 | 空操作（也被用作「已优化掉」的占位） |
| `INSTR_INCLUDE` | 0x056 | `include` 引入其它脚本模块 |
| `INSTR_STRICT` | 0x057 | 进入严格模式（`"use strict"`） |
| `INSTR_CACHE` | 0x048 | 从变量缓存加载并压栈 |
| `INSTR_NCACHE` | 0x049 | 从成员访问缓存加载并压栈 |
| `INSTR_END` | 0x058 | 代码结束标记，`vm_run` 遇到即停止 |
| `INSTR_MAX` | 0x059 | 操作码上限（分发表大小） |

## 3.5 把指令连起来看：一个完整例子

`var a = 1 + 2;` 编译出的指令与栈变化：

| pc | 指令 | 执行后栈（底→顶） | 说明 |
| --- | --- | --- | --- |
| 0 | `VAR "a"` | （空） | 在当前作用域声明 `a` |
| 1 | `LOAD "a"` | `[a节点]` | 把赋值目标（`a` 的 node）压栈 |
| 2 | `INTS 1` | `[a节点, 1]` | 压入 1 |
| 3 | `INTS 2` | `[a节点, 1, 2]` | 压入 2 |
| 4 | `PLUS` | `[a节点, 3]` | 弹出 1、2，压入 3 |
| 5 | `ASIGN` | `[3]` 或 `[]` | 把 3 写入 `a` 节点 |
| 6 | `POP` | `[]` | 丢弃语句结果 |

关键理解：**赋值语句会先把「左值目标」（一个 `node_t`）压栈**，再计算右值，最后 `ASIGN` 把右值写入目标节点。这解释了为什么 `LOAD "a"` 出现在 `INTS` 之前。

## 3.6 反汇编器 bcdump

把机器码翻译回可读文本的工具在 [`mario/bcdump/bcdump.c`](../../mario/bcdump/bcdump.c)：

- `inmstr_str(ins)`：操作码 → 助记符字符串（一张大 `switch`）。
- `bc_dump(bc)`：先打印字符串表，再逐条打印指令；遇到 `INT`/`FLOAT` 会多读一条双字数据；跳转类指令按整数打印偏移，其余按字符串表下标打印。

命令行 `-a` 参数正是调用了它（见第 2 章）。

---

至此你已经能读懂任意一段 Mario 字节码。下一章 [第 4 章 · 词法分析器](04-lexer.md) 回到源头，看文本是如何变成 token 的。
