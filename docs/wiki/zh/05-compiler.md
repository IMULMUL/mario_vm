# 第 5 章 · 编译器（递归下降解析）

编译器的任务：读 token 流，按语法规则组织，输出字节码。全部实现在 [`lang/js/compiler.c`](../../lang/js/compiler.c)，入口是：

```c
bool js_compile(bytecode_t *bc, const char* input);
```

它采用的是一种经典、易读的语法分析技术——**递归下降（recursive descent）+ 运算符优先级（precedence climbing）**。

## 5.1 两个基础动作：chkread 与 skip_empty

编译器几乎所有解析函数都靠这两个动作推进：

```c
// 断言当前 token 是 expected，然后读取下一个 token；不匹配则报错返回 false
bool lex_chkread(lex_t* lex, uint32_t expected_tk);

// 跳过空行（连续的 '\n'）
bool lex_skip_empty(lex_t* l);
```

`lex_chkread` 是「消费一个期望的 token」。例如解析 `if (...)` 时，先 `lex_chkread(l, LEX_R_IF)` 吃掉 `if`，再 `lex_chkread(l, '(')` 吃掉左括号。若当前 token 不是期望值，就打印 `lex got 'X' expected 'Y'` 并定位错误行列。

## 5.2 表达式解析：一条优先级链

Mario 用「函数层层调用」来表达运算符优先级。**优先级越高的运算，被越深层的函数处理**。调用链自顶向下是：

```
base        赋值 = += -= *= /= %=（最低优先级，右结合）
 └ ternary   三元 ? :
    └ logic   || && | & ^
       └ condition  == != === !== <= >= < > instanceof
          └ shift   << >> >>>
             └ expr      + - ++ --（含一元 - ++ --）
                └ term      * / %
                   └ unary     ! typeof
                      └ factor    原子：字面量、变量、括号、调用、成员访问（最高优先级）
```

读法：解析 `base` 时，它会先调用 `ternary` 解析左操作数；`ternary` 又调用 `logic`……一路降到 `factor` 处理最基本的原子。返回上层时，如果发现了本级对应的运算符，就再解析一个同级的右操作数，然后生成一条运算指令。

### 举例：`term`（处理 `* / %`）

```c
bool term(lex_t* l, bytecode_t* bc) {
    if (!unary(l, bc)) return false;         // 先解析左操作数

    while (l->tk=='*' || l->tk=='/' || l->tk=='%') {
        LEX_TYPES op = (LEX_TYPES)l->tk;
        if (!lex_chkread(l, l->tk)) return false;  // 吃掉运算符
        if (!unary(l, bc)) return false;           // 解析右操作数
        if (op=='*') bc_gen(bc, INSTR_MULTI);      // 生成对应指令
        else if (op=='/') bc_gen(bc, INSTR_DIV);
        else bc_gen(bc, INSTR_MOD);
    }
    return true;
}
```

`while` 循环让 `1*2*3` 这样的**同级左结合**运算被正确处理：算完 `1*2` 后，循环再检测到 `*`，继续算 `(1*2)*3`。

`expr`（处理 `+ -`）、`condition`（比较）、`logic`（逻辑）都是同样的套路，只是运算符和生成的指令不同。

### factor：处理原子与后缀

`factor()` 是最复杂的一层，因为它要区分很多东西：

| 当前 token | 处理 |
| --- | --- |
| `(` | 括号表达式；若后面跟 `=>` 则是箭头函数 |
| `true`/`false`/`null`/`undefined` | 生成 `TRUE`/`FALSE`/`NULL`/`UNDEF` |
| `INT`/`FLOAT`/`STR` | 生成 `INT`/`FLOAT`/`STR` |
| `function` | 函数定义 `factor_def_func` |
| `class` | 类定义 `factor_def_class` |
| `new` | 对象创建 `factor_new` |
| `{` | JSON 对象字面量 `factor_json` |
| `[` | 数组字面量 `factor_array` |
| `ID` | 变量/函数调用/数组访问/成员访问 |

`factor` 有一个 `member` 参数，区分「这是一个独立表达式」还是「这是 `.` 右边的成员」：

- 独立场景下 `foo` → `LOAD "foo"`；
- 成员场景下 `.foo` → `GET "foo"`（从栈顶对象取字段）。

`factor` 末尾统一处理链式成员访问：

```c
if (l->tk == '.') {          // a.b.c 中的 .
    lex_chkread(l, '.');
    factor(l, bc, true);     // member=true，递归解析右侧
}
```

函数调用与数组访问也在 `ID` 分支里处理：

```c
if (l->tk == '(')      factor_call_func(...);   // foo(...) → CALL "foo$N"
else if (l->tk == '[') factor_array_access(...);// foo[i]   → LOAD/GET + ARRAY_AT
```

调用时 `gen_func_name` 会把参数个数拼进名字（`foo` → `foo$2`），对应第 3 章讲的 `parse_func_name`。

## 5.3 语句解析

顶层入口 `statement()` 按当前 token 分派到各语句处理器：

| token | 语句 | 处理函数 |
| --- | --- | --- |
| `{` | 代码块 | `stmt_block` |
| `var`/`let`/`const` | 变量声明 | `stmt_var` |
| `class` | 类定义 | `factor_def_class` |
| `function` | 函数声明 | `stmt_function` |
| `if` | 条件 | `stmt_if` |
| `while` | 循环 | `stmt_while` |
| `for` | 循环（含 for-in） | `stmt_for` |
| `break`/`continue` | 循环控制 | `stmt_break`/`stmt_continue` |
| `return` | 返回 | `stmt_return` |
| `throw`/`try` | 异常 | `stmt_throw`/`stmt_try` |
| `include` | 引入模块 | `stmt_include` |
| 其它（ID/数字/字符串/`[`/`-`/`++`…） | 普通表达式语句 | `base` |

普通表达式语句执行后会 `bc_gen(bc, INSTR_POP)`，因为表达式会在栈上留下一个结果，而语句不需要它，必须弹出以保持栈平衡。

### 语句结束符

```c
static bool is_stmt_end(int tk) { return (tk==';' || tk=='\n' || tk==0); }
```

Mario 允许用 `;` **或换行**结束语句，这就是它能写「无分号 JS」的原因。

## 5.4 控制流如何生成跳转（重点）

控制流是编译里最烧脑的部分，因为要用「预留 + 回填」处理未知的跳转目标。

### if / else（`stmt_if`）

```c
base(l, bc);                    // 编译条件，结果压栈
PC pc = bc_reserve(bc);         // 预留一个跳转槽（还不知道跳哪）
statement(l, bc);               // 编译 then 分支

if (l->tk == LEX_R_ELSE) {
    PC pc2 = bc_reserve(bc);                 // 再预留一个（跳过 else）
    bc_set_instr(bc, pc, INSTR_NJMP, ...);   // 回填：条件假 → 跳到 else
    statement(l, bc);                        // 编译 else 分支
    bc_set_instr(bc, pc2, INSTR_JMP, ...);   // 回填：then 结束 → 跳过 else
} else {
    bc_set_instr(bc, pc, INSTR_NJMP, ...);   // 条件假 → 跳过 then
}
```

`bc_reserve` 先占位，`bc_set_instr` 在知道了目标位置后回填偏移。`NJMP`（条件假跳转）实现了「条件不成立就跳过分支」。

### while（`stmt_while`）与真实字节码

先看一段真实 dump（源码 `var i=0; while(i<3){ i=i+1; }` 的循环部分）：

```
00000005 | LOOP              ; 进入循环作用域
00000006 | NIL               ; 预留（while 无 init）
00000007 | JMP     2         ; 跳到条件判断（continue 锚点）
00000008 | JMP     12        ; break 锚点占位（稍后回填）
00000009 | LOAD    "i"       ; ┐
00000010 | INTS    3         ; ├ 条件 i < 3
00000011 | LES               ; ┘
00000012 | NJMPB   4         ; 条件假 → 向后跳 4 到 break 出口
00000013 | LOAD    "i"       ; ┐
00000014 | LOAD    "i"       ; │
00000015 | INTS    1         ; ├ 循环体 i = i + 1
00000016 | PLUS              ; │
00000017 | ASIGN             ; │
00000018 | POP               ; ┘
00000019 | JMPB    12        ; 无条件向后跳到条件（回到 00007 附近）
00000020 | LOOPE             ; 离开循环作用域（break 出口）
```

对照 `stmt_while` 的实现：

```c
bc_gen(bc, INSTR_LOOP);                          // 进入循环作用域
PC pc = bc_reserve(bc);                          // init 槽（while 用不到）
PC pc_condition = bc_add_instr(bc, pc, INSTR_JMP, pc+2)-1;  // 跳到条件（continue 锚点）
PC pc_break = bc_reserve(bc);                    // break 锚点占位

// 编译条件 ...
bc_add_instr(bc, pc_break, INSTR_NJMPB, ...);    // 条件假 → 跳出循环

stmt_loop_block(l, bc);                          // 编译循环体
bc_add_instr(bc, pc_condition, INSTR_JMPB, ...); // 回填：体结束后跳回条件
pc = bc_gen(bc, INSTR_LOOP_END);
bc_set_instr(bc, pc_break, INSTR_JMP, pc-1);     // 回填 break 锚点 → 指向 LOOPE
```

关键概念——**锚点（anchor）**：
- **continue 锚点**：`pc_condition`，`continue` 语句会跳到这里（重新判断条件）。
- **break 锚点**：`pc_break`，`break` 语句会跳到这里（离开循环）。

`INSTR_LOOP`/`INSTR_LOOP_END` 这对指令在运行时把这两个锚点记录进 `scope_t`，供 `handle_break`/`handle_continue` 使用（见第 6 章）。

### for（`stmt_for`）

标准 `for(init; cond; iter)` 的字节码布局更复杂，它把「条件」和「迭代器」的位置重排，使得循环体内只需一次向后跳转。编译器还特判了 `for (var k in obj)` 形式（`stmt_for_in`），它会生成一段代码：把对象的 `keys()` 存进隐藏变量 `__for_in_keys`，用下标 `__for_in_idx` 遍历。

### try / catch（`stmt_try`）

```c
PC pc = bc_gen(bc, INSTR_TRY);
bc_add_instr(bc, pc, INSTR_JMP, pc+2);   // 进入 try 作用域
PC pc_cache = bc_reserve(bc);            // catch 锚点：抛异常时跳到这
statement(l, bc);                        // try 体
PC pce = bc_reserve(bc);                 // 正常结束 → 跳过 catch
bc_set_instr(bc, pc_cache, INSTR_JMP, ...);  // 回填 catch 锚点
// 解析 catch(x) ...
bc_gen_str(bc, INSTR_CATCH, "x");        // 把抛出的值绑定到 x
statement(l, bc);                        // catch 体
pc = bc_gen(bc, INSTR_TRY_END) - 1;
bc_set_instr(bc, pce, INSTR_JMP, pc);    // 回填：try 正常结束跳过 catch
```

`INSTR_TRY`/`INSTR_TRY_END` 在运行时形成一个「try 作用域」，`handle_throw` 抛异常时会沿作用域栈回溯，找到最近的 try 作用域，把 `pc` 设成它记录的 catch 锚点。

## 5.5 函数与类的编译

### 函数（`factor_def_func`）

生成的字节码结构（第 3 章已提及）：

```
FUNC                       ; 函数定义开始
LOAD "arg1"                ; 形参名依次压入 func->args
LOAD "arg2"
JMP  <跳过函数体>           ; 定义时不执行体
<函数体字节码>
RETURN / RETURNV           ; 若源码没写 return，编译器补一条 RETURN
```

`bc_reserve` 预留 `JMP`，函数体编译完后 `bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC)` 回填，让定义处能跳过整段函数体。

### 类（`factor_def_class`）

```
CLASS "Base"               ; 创建类
EXTENDS "Parent"           ; 可选：继承
<成员...>                  ; 每个方法：FUNC... + MEMBERN "方法名"
                           ; 每个字段 x=...：LOAD x + 值 + ASIGN + POP
CLASS_END                  ; 结束
```

类方法编译成 `FUNC` + `MEMBERN "名字"`，运行时挂到类的 prototype 上（见第 7 章）。

## 5.6 编译主循环

```c
bool js_compile(bytecode_t *bc, const char* input) {
    lex_t lex;
    lex_init(&lex, input);
    lex_get_next_token(&lex);       // 预读第一个 token

    bool ret = true;
    while (lex.tk != LEX_EOF && ret) {
        ret = statement(&lex, bc);  // 一条条编译语句
        lex_skip_empty(&lex);
    }
    if (ret) bc_gen(bc, INSTR_END); // 收尾：END
    else compile_error_pos(&lex, -1);

    lex_release(&lex);
    return ret;
}
```

整个编译器不到 1800 行，却覆盖了 ES5 的主要语法。它的清晰之处在于：**每个语法结构 = 一个函数**，函数之间的调用关系 = 语法的嵌套关系 = 运算符的优先级关系。

下一章 [第 6 章 · 虚拟机执行引擎](06-vm.md)，看这些字节码如何被真正执行。
