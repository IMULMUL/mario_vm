# Chapter 3 · The Bytecode Instruction Set

Bytecode is the "contract" between the compiler and the virtual machine. The compiler translates source code into bytecode, and the VM executes it instruction by instruction. This chapter explains that contract thoroughly.

All instruction constants are defined in the `MARIO_BC` block of [`mario/mario.h`](../../../mario/mario.h).

## 3.1 One instruction = one 32-bit integer

Every Mario instruction is a `uint32_t` (type alias `PC`). It is split into three fields:

```
 31        28 27            20 19                        0
┌────────────┬────────────────┬────────────────────────────┐
│  OPTION    │    OPR_CODE    │      OFFSET / VALUE         │
│  (4 bits)  │    (8 bits)    │        (20 bits)            │
└────────────┴────────────────┴────────────────────────────┘
```

The corresponding macros (see `mario.h`):

```c
#define OFF_MASK 0x0FFFFF                                   // low 20-bit mask
#define INS(ins, off) ((((int32_t)ins)<<20)&0xFFF00000) | ((off)&OFF_MASK))
#define OP(ins)  (((ins) >> 20) & 0xFF)   // extract the opcode (8 bits)
#define OFF(ins) ((ins) & OFF_MASK)       // extract the operand (20 bits)
```

- **OPR_CODE (opcode)**: says what the instruction does, e.g. `INSTR_PLUS` (addition), `INSTR_CALL` (call a function).
- **OFFSET / VALUE (operand)**: a 20-bit field whose meaning varies by instruction:
  - For `LOAD`/`VAR` etc.: it is a **string-table index** (pointing to a variable name);
  - For `JMP`/`NJMP` etc.: it is a **jump offset**;
  - For `INTS` (short integer): it is directly the **integer value**;
  - For instructions with no operand (like `PLUS`): it is filled with `OFF_MASK` (all 1s, i.e. `0xFFFFF`).
- **OPTION (option bits)**: the top 4 bits, currently used mainly for `INSTR_OPT_CACHE` (`0x80000000`) to mark an instruction as "already runtime-optimized" (see the inline-cache section in Chapter 6).

### Why a 20-bit operand

20 bits means the string table can hold roughly 1 million entries and jump offsets can span about ±1 million instructions — plenty for small scripts — while keeping the whole instruction inside a single machine word, so fetching/decoding is very fast.

### Back to the Chapter 2 example

```
00000002 | 0x00D00001 ; INTS    1
```

Breaking down `0x00D00001`:
- `OP = (0x00D00001 >> 20) & 0xFF = 0x0D` → `INSTR_INT_S` (short integer, shown as `INTS` in the dump);
- `OFF = 0x00D00001 & 0xFFFFF = 1` → the integer value is `1`.

And:

```
00000004 | 0x01EFFFFF ; PLUS
```
- `OP = 0x1E` → `INSTR_PLUS`;
- `OFF = 0xFFFFF` (all 1s) → no operand.

## 3.2 The string table mstr_table

Instructions do not store strings directly; they store the string's **index** in the `mstr_table`. This table lives in `bytecode_t.mstr_table` (an `m_array_t`).

```c
typedef struct st_bytecode {
    PC          cindex;       // number of instructions generated (also the next write position)
    m_array_t   mstr_table;   // string table
    PC*         code_buf;     // instruction array
    uint32_t    buf_size;     // code_buf capacity
} bytecode_t;
```

Benefits: the same identifier (like `console`) is stored only once even if used in many places; instructions are fixed-length; `.mbc` files are compact.

Related helper functions (`mario.c`):

- `bc_getstrindex(bc, str)`: get a string's index; if it doesn't exist, append it to the table and return the new index (deduplication).
- `bc_getstr(bc, i)`: get the string back from an index (a macro; returns `""` if out of bounds).

## 3.3 How bytecode is "generated"

The compiler doesn't know all jump targets up front, so it provides a set of generate/back-patch functions (declared in `mario.h`, implemented in `mario.c`):

| Function | Purpose |
| --- | --- |
| `bc_gen(bc, instr)` | Append an instruction with no operand, return its pc |
| `bc_gen_str(bc, instr, s)` | Append an instruction whose operand is a string-table index |
| `bc_gen_int(bc, instr, i)` | Append `INT` followed by a 32-bit integer (occupies two slots) |
| `bc_gen_short(bc, instr, s)` | Append a short-integer instruction (value goes directly into the 20-bit operand) |
| `bc_reserve(bc)` | Reserve an empty slot to back-patch later |
| `bc_set_instr(bc, anchor, op, target)` | Back-patch: set the slot at `anchor` to jump to `target` |
| `bc_add_instr(bc, anchor, op, target)` | Append at the end and compute a relative offset based on `anchor` |
| `bc_remove_instr(bc, from, num)` | Remove a number of instructions |

**Back-patching** is the key to understanding the compiler: when compiling an `if` you don't yet know "where to jump if the condition is false", so you first `bc_reserve()` a placeholder; once the `then` branch is compiled and the target address is known, you use `bc_set_instr()` to fill in the jump offset.

### INT / FLOAT are double-word instructions

Ordinary integer/float values may exceed 20 bits and cannot fit into a single instruction's operand. So:

- `INSTR_INT`: occupies **two slots** — the first is the instruction itself, the second `code_buf[pc+1]` holds the full 32-bit integer.
- `INSTR_FLOAT`: also occupies two slots; the second stores the `float` via a bit copy.
- `INSTR_INT_S` (short integer): used when the value fits in 20 bits; occupies only **one slot**, saving space. Shown as `INTS` in the dump.

This is why `bcdump.c` reads an extra line when it encounters `INSTR_INT`/`INSTR_FLOAT`.

## 3.4 The complete instruction set

Below, all instructions are listed by category. Opcode values are taken from `mario.h`; "stack behavior" describes the effect on the operand stack at execution time.

### 3.4.1 Constants and literals

| Instruction | Code | Stack behavior | Description |
| --- | --- | --- | --- |
| `INSTR_INT` | 0x007 | push int | push a 32-bit integer (double-word) |
| `INSTR_INT_S` | 0x00D | push int | push a short integer (single-word, value in the operand) |
| `INSTR_FLOAT` | 0x008 | push float | push a float (double-word) |
| `INSTR_STR` | 0x009 | push string | push a string (operand is a string-table index) |
| `INSTR_TRUE` | 0x043 | push true | push boolean true |
| `INSTR_FALSE` | 0x044 | push false | push boolean false |
| `INSTR_NULL` | 0x045 | push null | push null |
| `INSTR_UNDEF` | 0x046 | push undefined | push undefined |

### 3.4.2 Variables and access

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_VAR` | 0x001 | declare variable `x` (`var`) |
| `INSTR_SAFE_VAR` | 0x00E | declare a block-scoped variable `x` (`let`) |
| `INSTR_CONST` | 0x002 | declare a constant `x` (`const`, cannot be reassigned) |
| `INSTR_LOAD` | 0x003 | load variable `x` and push it (also used as an assignment lvalue / base object for member access) |
| `INSTR_STORE` | 0x005 | pop and store into `x` |
| `INSTR_GET` | 0x006 | fetch an object member field (getfield) |
| `INSTR_ASIGN` | 0x004 | assignment `=`: pop the value and the target node, write it in |
| `INSTR_POP` | 0x04A | pop the top of stack and release it (discard a statement result) |

> Difference between `LOAD` and `GET`: `LOAD` looks up a variable along the scope chain, or serves as the base object for "fetch a member"; `GET` fetches a member by name from the top-of-stack object after a `.`.

### 3.4.3 Arithmetic (pop operands, push result)

| Instruction | Code | Operation |
| --- | --- | --- |
| `INSTR_PLUS` | 0x01E | `+` (numeric addition / string concatenation) |
| `INSTR_MINUS` | 0x01F | `-` |
| `INSTR_MULTI` | 0x01B | `*` |
| `INSTR_DIV` | 0x01C | `/` |
| `INSTR_MOD` | 0x01D | `%` |
| `INSTR_NEG` | 0x020 | unary minus `-x` |
| `INSTR_PPLUS` | 0x021 | postfix `x++` |
| `INSTR_MMINUS` | 0x022 | postfix `x--` |
| `INSTR_PPLUS_PRE` | 0x023 | prefix `++x` |
| `INSTR_MMINUS_PRE` | 0x024 | prefix `--x` |

### 3.4.4 Compound assignment

| Instruction | Code | Operation |
| --- | --- | --- |
| `INSTR_PLUSEQ` | 0x02E | `+=` |
| `INSTR_MINUSEQ` | 0x02F | `-=` |
| `INSTR_MULTIEQ` | 0x030 | `*=` |
| `INSTR_DIVEQ` | 0x031 | `/=` |
| `INSTR_MODEQ` | 0x032 | `%=` |

### 3.4.5 Bitwise operations

| Instruction | Code | Operation |
| --- | --- | --- |
| `INSTR_LSHIFT` | 0x025 | `<<` |
| `INSTR_RSHIFT` | 0x026 | `>>` |
| `INSTR_URSHIFT` | 0x027 | `>>>` (unsigned right shift) |
| `INSTR_OR` | 0x035 | `\|` |
| `INSTR_XOR` | 0x036 | `^` |
| `INSTR_AND` | 0x037 | `&` |

### 3.4.6 Comparison and logic

| Instruction | Code | Operation |
| --- | --- | --- |
| `INSTR_EQ` | 0x028 | `==` |
| `INSTR_NEQ` | 0x029 | `!=` |
| `INSTR_TEQ` | 0x038 | `===` (strict equality) |
| `INSTR_NTEQ` | 0x039 | `!==` |
| `INSTR_LES` | 0x02D | `<` |
| `INSTR_LEQ` | 0x02A | `<=` |
| `INSTR_GRT` | 0x02C | `>` |
| `INSTR_GEQ` | 0x02B | `>=` |
| `INSTR_NOT` | 0x01A | `!` |
| `INSTR_AAND` | 0x033 | logical AND (legacy path, `handle_logic`, boolean result) |
| `INSTR_OOR` | 0x034 | logical OR (legacy path, `handle_logic`, boolean result) |
| `INSTR_SCAND` | 0x087 | `&&` short-circuit (actual compiled output, returns the operand value itself) |
| `INSTR_SCOR` | 0x086 | `\|\|` short-circuit (actual compiled output, returns the operand value itself) |
| `INSTR_TYPEOF` | 0x03A | `typeof` |
| `INSTR_INSTOF` | 0x055 | `instanceof` |

> Note: source-level `&&` / `\|\|` are now compiled by the compiler into `INSTR_SCAND` / `INSTR_SCOR` (see `compiler.c`). They follow JS short-circuit semantics and **return the operand value itself** (rather than coercing to boolean); `INSTR_AAND` / `INSTR_OOR` are the earlier boolean-logic implementation (`handle_logic`) and are still kept in the dispatch table.

### 3.4.7 Jumps and control flow

Jump instructions take a **relative offset** as their operand, split into "jump forward" and "jump backward" groups:

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_JMP` | 0x042 | unconditional forward jump by `offset` |
| `INSTR_JMPB` | 0x040 | unconditional backward jump by `offset` (Back) |
| `INSTR_NJMP` | 0x03F | jump forward if the condition is false (Not JMP) |
| `INSTR_NJMPB` | 0x041 | jump backward if the condition is false |
| `INSTR_BREAK` | 0x03B | `break`: jump out of the nearest loop scope |
| `INSTR_CONTINUE` | 0x03C | `continue`: jump to the loop's continue anchor |
| `INSTR_RETURN` | 0x03D | return with no value |
| `INSTR_RETURNV` | 0x03E | return with a value |

The precise jump semantics (`handle_jmp` etc., see `mario.c`):

```c
handle_jmp :  vm->pc = vm->pc + offset - 1;   // forward
handle_jmpb:  vm->pc = vm->pc - offset - 1;   // backward
handle_njmp:  pop the top of stack; if false → jump in the given direction, otherwise fall through
```

The `-1` is because `pc` was already incremented when the instruction was fetched.

### 3.4.8 Functions and calls

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_FUNC` | 0x00F | define a function |
| `INSTR_FUNC_STC` | 0x019 | define a static function (`static`) |
| `INSTR_FUNC_GET` | 0x010 | define a class getter |
| `INSTR_FUNC_SET` | 0x011 | define a class setter |
| `INSTR_CALL` | 0x012 | call an ordinary function and push the return value |
| `INSTR_CALLO` | 0x013 | call an object member method (`obj.x()`) |
| `INSTR_NEW` | 0x047 | `new` — create an object |

**Function-name encoding convention**: the operand of a call instruction points to a string of the form `name$argcount`, e.g. `log$1` means "function `log`, 1 argument". The parsing function is `parse_func_name()`:

```c
// "log$1" → name="log", returns 1
static int parse_func_name(const char* full, mstr_t* name);
```

This is done because Mario supports simple overloading/dispatch based on argument count.

**Function-body organization**: a `FUNC` instruction is followed by several `LOAD`s (pushing the parameter names in order into `func->args`), then a `JMP` (to skip over the body, since defining a function does not execute it); the function body bytecode comes after the `JMP` and ends with `RETURN`/`RETURNV`. `func_def()` parses this into a `func_t` at runtime.

### 3.4.9 Classes and objects

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_CLASS` | 0x014 | begin defining a class |
| `INSTR_CLASS_END` | 0x015 | end of class definition |
| `INSTR_EXTENDS` | 0x018 | inherit from a parent class |
| `INSTR_MEMBER` | 0x016 | anonymous member (array element) |
| `INSTR_MEMBERN` | 0x017 | named member (object property / class method) |
| `INSTR_OBJ` | 0x04B | begin a JSON-style object literal `{}` |
| `INSTR_OBJ_END` | 0x04C | end of object literal |
| `INSTR_ARRAY` | 0x00B | begin an array literal `[]` |
| `INSTR_ARRAY_END` | 0x00C | end of array literal |
| `INSTR_ARRAY_AT` | 0x00A | fetch element `arr[i]` |

### 3.4.10 Scope blocks

These instructions come in pairs and are used at runtime to push/pop a scope layer (`scope_t`):

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_BLOCK` / `INSTR_BLOCK_END` | 0x04D / 0x04E | ordinary code block `{}` |
| `INSTR_LOOP` / `INSTR_LOOP_END` | 0x04F / 0x050 | loop body (records break/continue anchors) |
| `INSTR_TRY` / `INSTR_TRY_END` | 0x051 / 0x052 | try block (records the catch anchor) |
| `INSTR_CATCH` | 0x054 | bind the thrown value to the catch variable |
| `INSTR_THROW` | 0x053 | throw: unwind the scope stack to find the nearest try |

### 3.4.11 Miscellaneous

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_NIL` | 0x000 | no-op (also used as a placeholder for "optimized away") |
| `INSTR_INCLUDE` | 0x056 | `include` — bring in another script module |
| `INSTR_STRICT` | 0x057 | enter strict mode (`"use strict"`) |
| `INSTR_CACHE` | 0x048 | load from the variable cache and push |
| `INSTR_NCACHE` | 0x049 | load from the member-access cache and push |
| `INSTR_END` | 0x058 | end-of-code marker; `vm_run` stops when it hits this |
| `INSTR_MAX` | 0x090 | opcode upper bound (dispatch-table size) |

### 3.4.12 ES6+ extended instructions (0x059–0x087)

As ES6+ features were added, opcodes expanded from `0x059` all the way to `0x087` (`INSTR_MAX` is `0x090`). Grouped by theme:

**Spread / rest**

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_ARR_SPREAD` | 0x059 | pop an array, append all its elements to the array literal being built |
| `INSTR_OBJ_SPREAD` | 0x05A | pop an object, copy its members into the object literal being built |
| `INSTR_CALL_SPREAD` | 0x05B | pop an args array, call function `x` with runtime arity (`f(...a)`) |
| `INSTR_CALLO_SPREAD` | 0x05C | pop an args array, call `obj.x(...a)` (obj beneath the array) |
| `INSTR_NEW_SPREAD` | 0x05D | pop an args array, construct `new x(...a)` with runtime arity |
| `INSTR_CALLX_SPREAD` | 0x062 | pop an args array, call the function value beneath it (runtime arity) |
| `INSTR_CALLXO_SPREAD` | 0x074 | pop an args array, call the func value beneath it with the receiver beneath that (`obj[k](...a)`) |

**Exponent / unary / bitwise assignment**

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_POW` | 0x05F | `**` exponentiation |
| `INSTR_POWEQ` | 0x060 | `**=` |
| `INSTR_POS` | 0x068 | unary `+` (ToNumber of the value on the stack) |
| `INSTR_BNOT` | 0x07F | unary bitwise NOT `~x` (ToInt32 then invert; BigInt → -(x+1)) |
| `INSTR_BITANDEQ` | 0x080 | `&=` |
| `INSTR_BITOREQ` | 0x081 | `\|=` |
| `INSTR_BITXOREQ` | 0x082 | `^=` |
| `INSTR_LSHIFTEQ` | 0x083 | `<<=` |
| `INSTR_RSHIFTEQ` | 0x084 | `>>=` |
| `INSTR_URSHIFTEQ` | 0x085 | `>>>=` |

**Call forms / arrow functions / generators**

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_CALLX` | 0x061 | call the function value sitting below its n args (IIFE / `(expr)()`) |
| `INSTR_CALLXO` | 0x073 | call the function value on the stack with the receiver beneath it (`obj[k](..)`) |
| `INSTR_FUNC_ARROW` | 0x06F | define an ES6 arrow function (lexical `this`, no `prototype`, not constructible) |
| `INSTR_FUNC_GEN` | 0x067 | generator function/method definition (body organized like `INSTR_FUNC`) |
| `INSTR_YIELD` | 0x065 | pop the yielded value, suspend the generator; on resume push the value passed to `next()` |
| `INSTR_YIELD_STAR` | 0x066 | `yield*`: pop an iterable, delegate yields to it, push its return value |

**Objects / members / computed keys / prototype**

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_MEMBERV` | 0x05E | pop value, pop key, set `scope-obj[key] = value` (computed key) |
| `INSTR_GETW` | 0x064 | member fetch as an assignment target (invokes a setter if the property is an accessor) |
| `INSTR_SET_PROTO` | 0x06E | pop v, set the object-under-construction's `[[Prototype]]` to v (`{__proto__: v}`) |
| `INSTR_ARRAY_AT_M` | 0x072 | subscript that keeps the receiver (push receiver then member) for `obj[k](..)` |
| `INSTR_ARRAY_AT_W` | 0x07C | subscript as an assignment target; for `ta[i] = ..` / `ta[i] += ..` (TypedArray) |

**Optional chaining / nullish coalescing / logical assignment**

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_OPT_GET` | 0x069 | optional-chaining member fetch `?.x`; nullish base → undefined |
| `INSTR_NULLISH` | 0x06A | `??` short-circuit: if top is non-nullish jump x (keep it), else pop and fall through |
| `INSTR_OREQ` | 0x06B | `\|\|=` |
| `INSTR_ANDEQ` | 0x06C | `&&=` |
| `INSTR_NULLISHEQ` | 0x06D | `??=` |

**Iteration protocol / template strings / literals**

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_GET_ITER` | 0x070 | pop an iterable, push its iterator (`obj[Symbol.iterator]()`) |
| `INSTR_ITER_STEP` | 0x071 | peek the iterator and call `next()`; if done jump offset, else push `step.value` |
| `INSTR_TAG_RAW` | 0x063 | pop rawArr, pop stringsArr, set `stringsArr.raw = rawArr`, push stringsArr (tagged template) |
| `INSTR_INT64` | 0x075 | push an int64 literal held in 2 consecutive PC words |
| `INSTR_FLOAT64` | 0x076 | push a double literal held in 2 consecutive PC words |
| `INSTR_BIGINT` | 0x077 | push a BigInt literal; the digit string rides the mstr_table like `INSTR_STR` (arbitrary width) |

**delete / in / switch**

| Instruction | Code | Description |
| --- | --- | --- |
| `INSTR_DELETE` | 0x078 | pop obj, delete own member `$n`, push bool (`delete o.x`) |
| `INSTR_DELETE_AT` | 0x07A | pop key, pop obj, delete own member `[key]`, push bool (`delete o[k]`) |
| `INSTR_DELETE_VAR` | 0x07B | delete the global binding `$n`, push bool (`delete x`) |
| `INSTR_IN` | 0x079 | pop obj, pop key, push bool (`key in obj`: own + prototype chain) |
| `INSTR_SWITCH` | 0x07D | push a switch scope (break anchor); pairs with `INSTR_SWITCH_END` |
| `INSTR_SWITCH_END` | 0x07E | pop the switch scope |

> The precise semantics of these instructions are implemented in the various `handle_*` functions in [`mario/mario.c`](../../../mario/mario.c) and registered in `init_instr_table()` (which initializes the dispatch table `instr_table[INSTR_MAX]`); their ES6+ syntax-level correspondence is covered in Chapter 11.

## 3.5 Putting instructions together: a complete example

The instructions compiled from `var a = 1 + 2;` and the stack changes:

| pc | Instruction | Stack after execution (bottom→top) | Description |
| --- | --- | --- | --- |
| 0 | `VAR "a"` | (empty) | declare `a` in the current scope |
| 1 | `LOAD "a"` | `[a-node]` | push the assignment target (the node of `a`) |
| 2 | `INTS 1` | `[a-node, 1]` | push 1 |
| 3 | `INTS 2` | `[a-node, 1, 2]` | push 2 |
| 4 | `PLUS` | `[a-node, 3]` | pop 1, 2, push 3 |
| 5 | `ASIGN` | `[3]` or `[]` | write 3 into the `a` node |
| 6 | `POP` | `[]` | discard the statement result |

Key insight: **an assignment statement first pushes the "lvalue target" (a `node_t`)**, then computes the rvalue, and finally `ASIGN` writes the rvalue into the target node. This explains why `LOAD "a"` appears before `INTS`.

## 3.6 The disassembler bcdump

The tool that translates machine code back into readable text lives in [`mario/bcdump/bcdump.c`](../../../mario/bcdump/bcdump.c):

- `inmstr_str(ins)`: opcode → mnemonic string (one big `switch`).
- `bc_dump(bc)`: prints the string table first, then each instruction; for `INT`/`FLOAT` it reads an extra double-word of data; jump instructions print their offset as an integer, the rest print a string-table index.

The command-line `-a` argument calls exactly this (see Chapter 2).

---

At this point you can read any piece of Mario bytecode. The next chapter, [Chapter 4 · The Lexer](04-lexer.md), goes back to the source and shows how text becomes tokens.
