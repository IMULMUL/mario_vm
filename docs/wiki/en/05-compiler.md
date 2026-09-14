# Chapter 5 · The Compiler (Recursive Descent)

The compiler's job: read the token stream, organize it according to grammar rules, and output bytecode. It is all implemented in [`lang/js/compiler.c`](../../../lang/js/compiler.c), with the entry point:

```c
bool js_compile(bytecode_t *bc, const char* input);
```

It uses a classic, readable parsing technique — **recursive descent + precedence climbing**.

## 5.1 Two basic actions: chkread and skip_empty

Almost every parsing function in the compiler advances via these two actions:

```c
// Assert the current token is `expected`, then read the next token; on mismatch report an error and return false
bool lex_chkread(lex_t* lex, uint32_t expected_tk);

// Skip empty lines (consecutive '\n')
bool lex_skip_empty(lex_t* l);
```

`lex_chkread` means "consume an expected token". For example, when parsing `if (...)`, it first `lex_chkread(l, LEX_R_IF)` to eat the `if`, then `lex_chkread(l, '(')` to eat the left parenthesis. If the current token isn't the expected value, it prints `lex got 'X' expected 'Y'` and locates the error line/column.

## 5.2 Expression parsing: a precedence chain

Mario expresses operator precedence through "layers of function calls". **The higher the precedence, the deeper the function that handles it**. The call chain from top to bottom is:

```
base        assignment = += -= *= /= %= (lowest precedence, right-associative)
 └ ternary   ternary ? :
    └ logic   || && | & ^
       └ condition  == != === !== <= >= < > instanceof
          └ shift   << >> >>>
             └ expr      + - ++ -- (including unary - ++ --)
                └ term      * / %
                   └ unary     ! typeof
                      └ factor    atoms: literals, variables, parentheses, calls, member access (highest precedence)
```

How to read it: when parsing `base`, it first calls `ternary` to parse the left operand; `ternary` calls `logic`… all the way down to `factor`, which handles the most basic atoms. When returning to an upper level, if an operator belonging to that level is found, it parses another right operand at the same level and then generates one operation instruction.

### Example: `term` (handles `* / %`)

```c
bool term(lex_t* l, bytecode_t* bc) {
    if (!unary(l, bc)) return false;         // parse the left operand first

    while (l->tk=='*' || l->tk=='/' || l->tk=='%') {
        LEX_TYPES op = (LEX_TYPES)l->tk;
        if (!lex_chkread(l, l->tk)) return false;  // eat the operator
        if (!unary(l, bc)) return false;           // parse the right operand
        if (op=='*') bc_gen(bc, INSTR_MULTI);      // generate the corresponding instruction
        else if (op=='/') bc_gen(bc, INSTR_DIV);
        else bc_gen(bc, INSTR_MOD);
    }
    return true;
}
```

The `while` loop makes **left-associative, same-level** operations like `1*2*3` work correctly: after computing `1*2`, the loop detects `*` again and continues with `(1*2)*3`.

`expr` (handles `+ -`), `condition` (comparison), and `logic` (logical) follow the same pattern, differing only in the operators and generated instructions. (Exponentiation `**` and the newer ES6+ operators are woven into the appropriate levels of this chain.)

### factor: handling atoms and postfix

`factor()` is the most complex layer, because it has to distinguish many things:

| Current token | Handling |
| --- | --- |
| `(` | parenthesized expression; if followed by `=>` it's an arrow function |
| `true`/`false`/`null`/`undefined` | generate `TRUE`/`FALSE`/`NULL`/`UNDEF` |
| `INT`/`FLOAT`/`STR` | generate `INT`/`FLOAT`/`STR` |
| `function` | function definition `factor_def_func` |
| `class` | class definition `factor_def_class` |
| `new` | object creation `factor_new` |
| `{` | JSON object literal `factor_json` |
| `[` | array literal `factor_array` |
| `ID` | variable / function call / array access / member access |

`factor` has a `member` parameter distinguishing "this is a standalone expression" from "this is the member to the right of a `.`":

- In the standalone case, `foo` → `LOAD "foo"`;
- In the member case, `.foo` → `GET "foo"` (fetch a field from the top-of-stack object).

At the end, `factor` uniformly handles chained member access:

```c
if (l->tk == '.') {          // the . in a.b.c
    lex_chkread(l, '.');
    factor(l, bc, true);     // member=true, recursively parse the right side
}
```

Function calls and array access are also handled in the `ID` branch:

```c
if (l->tk == '(')      factor_call_func(...);   // foo(...) → CALL "foo$N"
else if (l->tk == '[') factor_array_access(...);// foo[i]   → LOAD/GET + ARRAY_AT
```

When calling, `gen_func_name` appends the argument count into the name (`foo` → `foo$2`), corresponding to the `parse_func_name` described in Chapter 3.

## 5.3 Statement parsing

The top-level entry `statement()` dispatches to the various statement handlers based on the current token:

| token | Statement | Handler |
| --- | --- | --- |
| `{` | code block | `stmt_block` |
| `var`/`let`/`const` | variable declaration (incl. destructuring) | `stmt_var` / `stmt_var_destructure` |
| `class` | class definition | `factor_def_class` |
| `function` | function declaration | `stmt_function` |
| `async` | async function declaration / async-arrow expression | `factor_def_func` / `base` |
| `if` | conditional | `stmt_if` |
| `while` | loop | `stmt_while` |
| `do` | do-while loop | `stmt_do` |
| `for` | loop (incl. for-in / for-of) | `stmt_for` / `stmt_for_in` / `stmt_for_of` |
| `switch` | branch selection | `stmt_switch` |
| `break`/`continue` | loop control | `stmt_break`/`stmt_continue` |
| `return` | return | `stmt_return` |
| `throw`/`try` | exceptions | `stmt_throw`/`stmt_try` |
| `include` | import a module | `stmt_include` |
| `[a,b] = ...` | array destructuring assignment | `stmt_var_destructure` |
| other (ID/number/string/`` ` ``/`[`/`(`/`-`/`~`/`++`/`await`/`delete`…) | ordinary expression statement | `base` / `expr_seq` |

After an ordinary expression statement executes, `bc_gen(bc, INSTR_POP)` is emitted, because an expression leaves a result on the stack that the statement doesn't need; it must be popped to keep the stack balanced.

### Statement terminator

```c
static bool is_stmt_end(int tk) { return (tk==';' || tk=='\n' || tk==0); }
```

Mario allows a statement to end with `;` **or a newline**, which is why it can write "semicolon-free JS".

## 5.4 How control flow generates jumps (key section)

Control flow is the most brain-burning part of compilation, because it must handle unknown jump targets with "reserve + back-patch".

### if / else (`stmt_if`)

```c
base(l, bc);                    // compile the condition, push the result
PC pc = bc_reserve(bc);         // reserve a jump slot (don't know the target yet)
statement(l, bc);               // compile the then branch

if (l->tk == LEX_R_ELSE) {
    PC pc2 = bc_reserve(bc);                 // reserve another (to skip the else)
    bc_set_instr(bc, pc, INSTR_NJMP, ...);   // back-patch: condition false → jump to else
    statement(l, bc);                        // compile the else branch
    bc_set_instr(bc, pc2, INSTR_JMP, ...);   // back-patch: end of then → skip else
} else {
    bc_set_instr(bc, pc, INSTR_NJMP, ...);   // condition false → skip then
}
```

`bc_reserve` places a placeholder first; `bc_set_instr` back-patches the offset once the target position is known. `NJMP` (jump-if-false) implements "skip the branch if the condition doesn't hold".

### while (`stmt_while`) and real bytecode

First look at a real dump (the loop part of `var i=0; while(i<3){ i=i+1; }`):

```
00000005 | LOOP              ; enter the loop scope
00000006 | NIL               ; reserved (while has no init)
00000007 | JMP     2         ; jump to the condition test (continue anchor)
00000008 | JMP     12        ; break anchor placeholder (back-patched later)
00000009 | LOAD    "i"       ; ┐
00000010 | INTS    3         ; ├ condition i < 3
00000011 | LES               ; ┘
00000012 | NJMPB   4         ; condition false → jump back 4 to the break exit
00000013 | LOAD    "i"       ; ┐
00000014 | LOAD    "i"       ; │
00000015 | INTS    1         ; ├ loop body i = i + 1
00000016 | PLUS              ; │
00000017 | ASIGN             ; │
00000018 | POP               ; ┘
00000019 | JMPB    12        ; unconditional backward jump to the condition (near 00007)
00000020 | LOOPE             ; leave the loop scope (break exit)
```

Compared with the `stmt_while` implementation:

```c
bc_gen(bc, INSTR_LOOP);                          // enter the loop scope
PC pc = bc_reserve(bc);                          // init slot (unused by while)
PC pc_condition = bc_add_instr(bc, pc, INSTR_JMP, pc+2)-1;  // jump to condition (continue anchor)
PC pc_break = bc_reserve(bc);                    // break anchor placeholder

// compile the condition ...
bc_add_instr(bc, pc_break, INSTR_NJMPB, ...);    // condition false → break out of the loop

stmt_loop_block(l, bc);                          // compile the loop body
bc_add_instr(bc, pc_condition, INSTR_JMPB, ...); // back-patch: after the body jump back to the condition
pc = bc_gen(bc, INSTR_LOOP_END);
bc_set_instr(bc, pc_break, INSTR_JMP, pc-1);     // back-patch the break anchor → point at LOOPE
```

Key concept — **anchors**:
- **continue anchor**: `pc_condition`; a `continue` statement jumps here (re-test the condition).
- **break anchor**: `pc_break`; a `break` statement jumps here (leave the loop).

The `INSTR_LOOP`/`INSTR_LOOP_END` pair records these two anchors into the `scope_t` at runtime, for use by `handle_break`/`handle_continue` (see Chapter 6).

### for (`stmt_for`)

The bytecode layout for a standard `for(init; cond; iter)` is more complex; it rearranges the positions of the "condition" and the "iterator" so that the loop body needs only one backward jump. The compiler also special-cases the `for (var k in obj)` form (`stmt_for_in`), which generates code that stores the object's `keys()` into a hidden variable `__for_in_keys` and iterates with an index `__for_in_idx`; as well as the ES6 `for (x of iterable)` form (`stmt_for_of`), which is based on the iteration protocol (`GET_ITER` / `ITER_STEP`, see Chapter 3) to fetch values one by one.

### try / catch (`stmt_try`)

```c
PC pc = bc_gen(bc, INSTR_TRY);
bc_add_instr(bc, pc, INSTR_JMP, pc+2);   // enter the try scope
PC pc_cache = bc_reserve(bc);            // catch anchor: jump here when throwing
statement(l, bc);                        // try body
PC pce = bc_reserve(bc);                 // normal end → skip catch
bc_set_instr(bc, pc_cache, INSTR_JMP, ...);  // back-patch the catch anchor
// parse catch(x) ...
bc_gen_str(bc, INSTR_CATCH, "x");        // bind the thrown value to x
statement(l, bc);                        // catch body
pc = bc_gen(bc, INSTR_TRY_END) - 1;
bc_set_instr(bc, pce, INSTR_JMP, pc);    // back-patch: try ends normally, skip catch
```

`INSTR_TRY`/`INSTR_TRY_END` form a "try scope" at runtime; when `handle_throw` throws, it unwinds the scope stack to find the nearest try scope and sets `pc` to the catch anchor it recorded.

## 5.5 Compiling functions and classes

### Functions (`factor_def_func`)

The generated bytecode structure (mentioned in Chapter 3):

```
FUNC                       ; function definition begins
LOAD "arg1"                ; parameter names pushed into func->args in order
LOAD "arg2"
JMP  <skip the body>       ; defining does not execute the body
<function body bytecode>
RETURN / RETURNV           ; if the source has no return, the compiler adds a RETURN
```

`bc_reserve` reserves the `JMP`; after the body is compiled, `bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC)` back-patches it so the definition site can skip the entire body.

### Classes (`factor_def_class`)

```
CLASS "Base"               ; create the class
EXTENDS "Parent"           ; optional: inherit
<members...>               ; each method: FUNC... + MEMBERN "method name"
                           ; each field x=...: LOAD x + value + ASIGN + POP
CLASS_END                  ; done
```

Class methods compile to `FUNC` + `MEMBERN "name"` and are attached to the class's prototype at runtime (see Chapter 7).

## 5.6 The compilation main loop

```c
bool js_compile(bytecode_t *bc, const char* input) {
    lex_t lex;
    lex_init(&lex, input);
    lex_get_next_token(&lex);       // prefetch the first token

    bool ret = true;
    while (lex.tk != LEX_EOF && ret) {
        int32_t prev_pos = lex.data_pos;
        uint32_t prev_tk = lex.tk;
        ret = statement(&lex, bc);  // compile statements one by one
        lex_skip_empty(&lex);
        /* Safety net: if a statement consumed no input at all (an unhandled
         * leading token), bail out with a compile error rather than spinning
         * forever in this loop. */
        if (ret && lex.tk != LEX_EOF &&
            lex.data_pos == prev_pos && lex.tk == prev_tk) {
            mario_printf("compile error: unexpected token, made no progress! ");
            ret = false;
        }
    }
    if (ret) bc_gen(bc, INSTR_END); // finish: END
    else compile_error_pos(&lex, -1);

    lex_release(&lex);
    return ret;
}
```

The whole compiler is about 4400 lines ([`lang/js/compiler.c`](../../../lang/js/compiler.c)) and covers grammar from ES5 through a large portion of ES6+. Its clarity lies in: **each grammar construct = one function**, and the call relationships between functions = the nesting relationships of the grammar = the precedence relationships of the operators.

Next: [Chapter 6 · The VM Execution Engine](06-vm.md), to see how this bytecode is actually executed.
