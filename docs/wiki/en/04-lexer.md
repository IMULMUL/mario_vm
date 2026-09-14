# Chapter 4 · The Lexer

Lexical analysis is the first step of compilation: cutting a string of characters into meaningful **tokens**, e.g. splitting `var a = 1 + 2;` into `var`, `a`, `=`, `1`, `+`, `2`, `;`.

Mario's lexical analysis is split into two layers:

1. **Basic lexer** ([`mario/lex/mario_lex.c`](../../../mario/lex/mario_lex.c)): language-agnostic; recognizes identifiers, numbers, strings, single-character symbols, whitespace and comments.
2. **JS extended lexer** (top of [`lang/js/compiler.c`](../../../lang/js/compiler.c)): on top of the basic lexer, recognizes multi-character operators (`==`, `+=`, `=>`) and reserved words (`if`, `while`, `class`, …).

## 4.1 Lexer state: lex_t

```c
typedef struct st_lex {
    const char*  data;                        // source string
    int32_t      data_pos;                    // current read position
    int32_t      data_start, data_end;        // start/end positions
    char         curr_ch, next_ch;            // current char + one lookahead char
    uint32_t     tk;                          // current token type
    mstr_t*      tk_str;                      // text of the current token (contents of ID/number/string)
    int32_t      tk_start, tk_end, tk_last_end;  // token position in the source (for error reporting)
} lex_t;
```

Design points:

- **Single-character lookahead**: `curr_ch` is the current character, `next_ch` is the next one. Many decisions need to "look at the next character", e.g. detecting `0x`-prefixed hex, `//` comments, `==` operators.
- **`tk` uses one integer to represent two kinds of token**:
  - ASCII characters themselves (like `+` `-` `(` `;`) use their character code directly;
  - special tokens use enum values `≥256` (see below).

## 4.2 Basic token types

Defined in [`mario/lex/mario_lex.h`](../../../mario/lex/mario_lex.h):

```c
typedef enum {
    LEX_EOF  = 0,      // end of input
    LEX_ID   = 256,    // identifier, e.g. foo, _bar
    LEX_INT,           // integer, e.g. 42, 0x2A
    LEX_FLOAT,         // float, e.g. 3.14, 1e-5
    LEX_STR,           // string literal
    LEX_BIGINT,        // BigInt literal (with an n suffix, e.g. 123n)
    LEX_BASIC_END      // end marker for basic types (JS extensions are numbered after this)
} lex_basic_type_t;
```

The JS compiler continues defining its own tokens after `LEX_BASIC_END` (`LEX_TYPES` in `compiler.c`): multi-character operators `LEX_EQUAL`(==), `LEX_PLUSEQUAL`(+=)… and reserved words `LEX_R_IF`, `LEX_R_WHILE`…

```c
typedef enum {
    LEX_EQUAL = LEX_BASIC_END,  // ==
    LEX_TYPEEQUAL,              // ===
    ...
    LEX_POWER, LEX_POWEREQUAL,  // **  **=
    LEX_OPTCHAIN,               // ?.   optional chaining
    LEX_NULLISH,                // ??   nullish coalescing
    LEX_NULLISHEQUAL,           // ??=
    LEX_OREQUALOR, LEX_ANDEQUALAND,  // ||=  &&=
    // reserved words
    LEX_R_IF, LEX_R_ELSE, LEX_R_WHILE, ...
    LEX_R_ASYNC, LEX_R_AWAIT,   // async / await
    LEX_R_DELETE, LEX_R_IN,     // delete / in
    LEX_R_SWITCH, LEX_R_CASE, LEX_R_DEFAULT,  // switch / case / default
    LEX_R_LIST_END
} LEX_TYPES;
```

This "relay numbering" guarantees the basic-layer and JS-layer token values don't collide. With the addition of ES6+ features, the JS layer gained exponentiation (`**`), optional chaining (`?.`), nullish coalescing (`??`) and a family of logical/nullish assignment operators, plus reserved words like `async`/`await`/`delete`/`in`/`switch`/`case`/`default`.

## 4.3 Character-classification helpers

At the top of `mario_lex.c` is a set of small, clear predicate functions:

| Function | Tests for |
| --- | --- |
| `is_whitespace(ch)` | space, `\t`, `\n`, `\r` |
| `is_space(ch)` | space, `\t`, `\r` (not newline) |
| `is_numeric(ch)` | `0`–`9` |
| `is_hexadecimal(ch)` | a hex digit |
| `is_alpha(ch)` | a letter or underscore `_` |

The difference between `is_space` and `is_whitespace` is crucial: a newline `\n` is whitespace but not space. Mario treats `\n` as a **statement terminator** (see `is_stmt_end` in Chapter 5), so newlines cannot be skipped mindlessly like ordinary spaces — this is why it can support "end a statement with a newline instead of a semicolon".

## 4.4 Advancing and skipping

```c
void lex_get_nextch(lex_t* lex);        // curr_ch = next_ch; next_ch = the next character
void lex_skip_whitespace(lex_t* lex);   // skip all whitespace (including newlines)
bool lex_skip_comments_line(lex_t*, "//");   // skip a line comment
bool lex_skip_comments_block(lex_t*, "/*", "*/");  // skip a block comment
```

`lex_get_nextch` is the lowest-level advance: move `next_ch` into `curr_ch`, then read a new character from `data` into `next_ch`, `data_pos++`. At end of input `next_ch` is set to 0.

## 4.5 Recognizing a basic token: lex_get_basic_token

This is the core of the basic lexer (`mario_lex.c`); the logic has three branches:

### ① Identifier (ID)
Starts with a letter/underscore, keeps absorbing letters and digits:

```c
if (is_alpha(lex->curr_ch)) {
    while (is_alpha(lex->curr_ch) || is_numeric(lex->curr_ch)) {
        mstr_add(lex->tk_str, lex->curr_ch);
        lex_get_nextch(lex);
    }
    lex->tk = LEX_ID;
}
```

### ② Number (INT / FLOAT / BIGINT)
Supports decimal, `0x` hex, decimal point, scientific notation `e/E`; if the number is followed by an `n` suffix it is recognized as `LEX_BIGINT`:

```c
} else if (is_numeric(lex->curr_ch)) {
    // handle a leading 0 and 0x
    // absorb digits (hex digits when hexadecimal)
    // on '.' followed by a digit → become LEX_FLOAT, absorb the fraction
    // on 'e'/'E' → become LEX_FLOAT, optional '-', absorb the exponent
}
```

### ③ String (double quotes)
Handles escape characters `\n \r \t \" \\`:

```c
} else if (lex->curr_ch == '"') {
    lex_get_nextch(lex);
    while (lex->curr_ch && lex->curr_ch != '"') {
        if (lex->curr_ch == '\\') { /* handle escape */ }
        else mstr_add(lex->tk_str, lex->curr_ch);
        lex_get_nextch(lex);
    }
    lex_get_nextch(lex);
    lex->tk = LEX_STR;
}
```

If none of the three match, `tk` stays `LEX_EOF` and is handed to the upper layer (it may be a single-character symbol or a JS single-quoted string).

## 4.6 Assembly: lex_get_next_token

The JS layer assembles the above capabilities into the full token-fetching flow (`compiler.c`):

```c
void lex_get_next_token(lex_t* lex) {
    lex->tk = LEX_EOF;
    mstr_reset(lex->tk_str);

    lex_skip_whitespace(lex);                    // skip whitespace
    if (lex_skip_comments_line(lex, "//")) {     // line comment → recurse and re-fetch
        lex_get_next_token(lex); return;
    }
    if (lex_skip_comments_block(lex, "/*","*/")) { // block comment → recurse and re-fetch
        lex_get_next_token(lex); return;
    }

    lex_token_start(lex);
    lex_get_basic_token(lex);                    // try a basic token first

    if (lex->tk == LEX_ID) {
        lex_get_reserved_word(lex);              // an ID may be a reserved word; identify further
    } else if (lex->tk == LEX_EOF) {
        if (lex->curr_ch == '\'') {
            lex_get_js_str(lex);                 // JS single-quoted string
        } else {
            lex_get_char_token(lex);             // single-character symbol
            lex_get_op_token(lex);               // try to combine into a multi-character operator
        }
    }
    lex_token_end(lex);
}
```

Three "post-processing" steps:

- **`lex_get_reserved_word`**: if what was just read is an identifier, a chain of `strcmp` checks whether it is `if`/`while`/`class`/`function`…; if so, `tk` is changed to the corresponding reserved-word token.
- **`lex_get_js_str`**: handles single-quoted strings `'...'`; compared with double quotes it additionally supports `\x` (hex) and octal escapes.
- **`lex_get_op_token`**: handles multi-character operators. For example, reading `=` with `curr_ch=='='` → becomes `LEX_EQUAL`(`==`); with one more `=` → `LEX_TYPEEQUAL`(`===`). Likewise it handles `!=`, `<=`, `<<`, `>>`, `>>>`, `++`, `--`, `&&`, `||`, `=>` (arrow function), as well as the ES6+ additions `**`, `?.` (optional chaining), `??` (nullish coalescing), `??=`, `||=`, `&&=`, etc.

> Template strings (backtick `` ` ``) are handled rather specially: the lexer treats the opening backtick as a **single-character token** handed to the upper layer, and the real template parsing happens at compile time in `factor_template()` (it scans characters directly, handles `${...}` nesting and escapes, then calls `lex_get_next_token` to return to the normal token stream). See Chapters 5 and 11.

## 4.7 Position tracking and error location

`lex_token_start` / `lex_token_end` record a token's start/end position in the source. Combined with:

```c
void lex_get_pos(lex_t* lex, int* line, int* col, int pos);
```

which scans from the beginning to `pos`, counts how many `\n` were passed, and computes the line and column numbers. When the compiler reports an error (`compile_error_pos`) it relies on this to output "line X, column Y".

> Note the "magic-number offsets" in `lex_token_start` (`tk_start = data_pos - 2`) and `lex_token_end` (`tk_end = data_pos - 3`): because the lexer always reads one character ahead, positions need to be corrected backwards.

## 4.8 Summary

- The basic lexer only recognizes "generic things": IDs, numbers, double-quoted strings, single characters, whitespace, comments.
- The JS layer handles "language-specific things": single-quoted strings, multi-character operators, reserved words.
- A token's type goes in `lex->tk`, its text content in `lex->tk_str`.
- The compiler consumes tokens one by one by repeatedly calling `lex_get_next_token`, and when needed uses `lex_chkread(expected)` to validate and advance (see Chapter 5).

Next: [Chapter 5 · The Compiler](05-compiler.md), to see how these tokens are organized into bytecode.
