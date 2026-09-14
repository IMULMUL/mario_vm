# 第 4 章 · 词法分析器（Lexer）

词法分析是编译的第一步：把一串字符切成一个个有意义的 **token（记号）**，比如把 `var a = 1 + 2;` 切成 `var`、`a`、`=`、`1`、`+`、`2`、`;`。

Mario 的词法分析分成两层：

1. **基础词法**（[`mario/lex/mario_lex.c`](../../mario/lex/mario_lex.c)）：与语言无关，负责识别标识符、数字、字符串、单字符符号、空白与注释。
2. **JS 扩展词法**（[`lang/js/compiler.c`](../../lang/js/compiler.c) 顶部）：在基础词法之上，识别多字符运算符（`==`、`+=`、`=>`）和保留字（`if`、`while`、`class`……）。

## 4.1 词法分析器状态：lex_t

```c
typedef struct st_lex {
    const char*  data;                        // 源码字符串
    int32_t      data_pos;                    // 当前读取位置
    int32_t      data_start, data_end;        // 起止位置
    char         curr_ch, next_ch;            // 当前字符 + 预读的一个字符
    uint32_t     tk;                          // 当前 token 类型
    mstr_t*      tk_str;                      // 当前 token 的文本（ID/数字/字符串的内容）
    int32_t      tk_start, tk_end, tk_last_end;  // token 在源码中的位置（用于报错定位）
} lex_t;
```

设计要点：

- **单字符预读**：`curr_ch` 是当前字符，`next_ch` 是下一个字符。很多判断需要「看后面一个字符」，例如判断 `0x` 开头的十六进制、`//` 注释、`==` 运算符。
- **`tk` 用一个整数同时表示两类 token**：
  - ASCII 字符本身（如 `+` `-` `(` `;`）直接用其字符码；
  - 特殊 token 用 `≥256` 的枚举值（见下）。

## 4.2 基础 token 类型

定义在 [`mario/lex/mario_lex.h`](../../mario/lex/mario_lex.h)：

```c
typedef enum {
    LEX_EOF  = 0,      // 输入结束
    LEX_ID   = 256,    // 标识符，如 foo、_bar
    LEX_INT,           // 整数，如 42、0x2A
    LEX_FLOAT,         // 浮点，如 3.14、1e-5
    LEX_STR,           // 字符串字面量
    LEX_BASIC_END      // 基础类型结束标记（JS 扩展从这之后编号）
} lex_basic_type_t;
```

JS 编译器在 `LEX_BASIC_END` 之后继续定义自己的 token（`compiler.c` 里的 `LEX_TYPES`）：多字符运算符 `LEX_EQUAL`(==)、`LEX_PLUSEQUAL`(+=)……以及保留字 `LEX_R_IF`、`LEX_R_WHILE`……

```c
typedef enum {
    LEX_EQUAL = LEX_BASIC_END,  // ==
    LEX_TYPEEQUAL,              // ===
    ...
    LEX_R_IF, LEX_R_ELSE, LEX_R_WHILE, ...  // 保留字
    LEX_R_LIST_END
} LEX_TYPES;
```

这种「接力式编号」保证基础层与 JS 层的 token 值不冲突。

## 4.3 字符分类工具

`mario_lex.c` 开头是一组小而清晰的判定函数：

| 函数 | 判定 |
| --- | --- |
| `is_whitespace(ch)` | 空格、`\t`、`\n`、`\r` |
| `is_space(ch)` | 空格、`\t`、`\r`（不含换行） |
| `is_numeric(ch)` | `0`–`9` |
| `is_hexadecimal(ch)` | 十六进制位 |
| `is_alpha(ch)` | 字母或下划线 `_` |

`is_space` 与 `is_whitespace` 的区别很关键：换行 `\n` 是 whitespace 但不是 space。Mario 把 `\n` 当作**语句结束符**处理（见第 5 章 `is_stmt_end`），所以换行不能像普通空格一样被无脑跳过——这也是为什么它能支持「不写分号，用换行结束语句」。

## 4.4 推进与跳过

```c
void lex_get_nextch(lex_t* lex);        // curr_ch = next_ch; next_ch = 下一个字符
void lex_skip_whitespace(lex_t* lex);   // 跳过所有空白（含换行）
bool lex_skip_comments_line(lex_t*, "//");   // 跳过行注释
bool lex_skip_comments_block(lex_t*, "/*", "*/");  // 跳过块注释
```

`lex_get_nextch` 是最底层的推进动作：把 `next_ch` 移到 `curr_ch`，再从 `data` 读一个新字符到 `next_ch`，`data_pos++`。读到末尾时 `next_ch` 置 0。

## 4.5 识别一个基础 token：lex_get_basic_token

这是基础词法的核心（`mario_lex.c`），逻辑分三支：

### ① 标识符（ID）
以字母/下划线开头，持续吸收字母和数字：

```c
if (is_alpha(lex->curr_ch)) {
    while (is_alpha(lex->curr_ch) || is_numeric(lex->curr_ch)) {
        mstr_add(lex->tk_str, lex->curr_ch);
        lex_get_nextch(lex);
    }
    lex->tk = LEX_ID;
}
```

### ② 数字（INT / FLOAT）
支持十进制、`0x` 十六进制、小数点、科学计数法 `e/E`：

```c
} else if (is_numeric(lex->curr_ch)) {
    // 处理前导 0 和 0x
    // 吸收数字（十六进制时吸收 hex 位）
    // 遇到 '.' 且后面是数字 → 转为 LEX_FLOAT，吸收小数部分
    // 遇到 'e'/'E' → 转为 LEX_FLOAT，可选 '-'，吸收指数
}
```

### ③ 字符串（双引号）
处理转义字符 `\n \r \t \" \\`：

```c
} else if (lex->curr_ch == '"') {
    lex_get_nextch(lex);
    while (lex->curr_ch && lex->curr_ch != '"') {
        if (lex->curr_ch == '\\') { /* 处理转义 */ }
        else mstr_add(lex->tk_str, lex->curr_ch);
        lex_get_nextch(lex);
    }
    lex_get_nextch(lex);
    lex->tk = LEX_STR;
}
```

如果三种都不匹配，`tk` 保持 `LEX_EOF`，交给上层处理（可能是单字符符号或 JS 单引号字符串）。

## 4.6 组装：lex_get_next_token

JS 层把上面的能力组装成完整的取 token 流程（`compiler.c`）：

```c
void lex_get_next_token(lex_t* lex) {
    lex->tk = LEX_EOF;
    mstr_reset(lex->tk_str);

    lex_skip_whitespace(lex);                    // 跳空白
    if (lex_skip_comments_line(lex, "//")) {     // 行注释 → 递归重取
        lex_get_next_token(lex); return;
    }
    if (lex_skip_comments_block(lex, "/*","*/")) { // 块注释 → 递归重取
        lex_get_next_token(lex); return;
    }

    lex_token_start(lex);
    lex_get_basic_token(lex);                    // 先试基础 token

    if (lex->tk == LEX_ID) {
        lex_get_reserved_word(lex);              // ID 可能是保留字，进一步识别
    } else if (lex->tk == LEX_EOF) {
        if (lex->curr_ch == '\'') {
            lex_get_js_str(lex);                 // JS 单引号字符串
        } else {
            lex_get_char_token(lex);             // 单字符符号
            lex_get_op_token(lex);               // 尝试组合成多字符运算符
        }
    }
    lex_token_end(lex);
}
```

三个「后处理」步骤：

- **`lex_get_reserved_word`**：如果刚读到的是标识符，用一连串 `strcmp` 检查它是不是 `if`/`while`/`class`/`function`…… 若是，就把 `tk` 改成对应的保留字 token。
- **`lex_get_js_str`**：处理单引号字符串 `'...'`，比双引号多了 `\x`（十六进制）、八进制转义的支持。
- **`lex_get_op_token`**：处理多字符运算符。例如读到 `=` 且 `curr_ch=='='` → 变成 `LEX_EQUAL`(`==`)，若再有一个 `=` → `LEX_TYPEEQUAL`(`===`)。同理处理 `!=`、`<=`、`<<`、`>>`、`>>>`、`++`、`--`、`&&`、`||`、`=>`（箭头函数）等。

## 4.7 位置记录与报错定位

`lex_token_start` / `lex_token_end` 记录 token 在源码里的起止位置。配合：

```c
void lex_get_pos(lex_t* lex, int* line, int* col, int pos);
```

它从头扫描到 `pos`，统计经过了多少个 `\n`，算出行号与列号。编译器报错时（`compile_error_pos`）就靠它输出「第几行第几列」。

> 注意 `lex_token_start` 里 `tk_start = data_pos - 2`、`lex_token_end` 里 `tk_end = data_pos - 3` 这些「魔数偏移」，是因为词法器始终预读了字符，位置需要回退校正。

## 4.8 小结

- 基础词法只认「通用的东西」：ID、数字、双引号字符串、单字符、空白、注释。
- JS 层负责「语言特有的东西」：单引号字符串、多字符运算符、保留字。
- 一个 token 的类型放在 `lex->tk`，文本内容放在 `lex->tk_str`。
- 编译器通过反复调用 `lex_get_next_token` 逐个消费 token，并在需要时用 `lex_chkread(expected)` 校验并前进（见第 5 章）。

下一章 [第 5 章 · 编译器](05-compiler.md)，看这些 token 如何被组织成字节码。
