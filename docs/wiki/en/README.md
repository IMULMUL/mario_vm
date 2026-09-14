# Mario VM Wiki

> Language: English (current) · [中文](../zh/README.md)

Welcome to the introductory documentation for **Mario VM**. This Wiki is aimed at readers encountering the project for the first time, walking from the overall design all the way to the bytecode instructions, the compiler, and the virtual machine internals — striving to be "approachable from zero".

Mario is an **extremely small, single-file bytecode virtual machine engine** with no third-party library dependencies, so it can run on the vast majority of embedded systems. On top of the Mario kernel, this repository extends a JavaScript language frontend (lexer + compiler), a rich set of built-in classes (native classes), and a command-line runner.

---

## Reading order

We recommend reading in the order below; each chapter builds on the concepts of the previous one:

| Chapter | Title | Summary |
| --- | --- | --- |
| Chapter 1 | [Project Overview and Architecture](01-overview.md) | What the project is, which modules it consists of, what a script goes through from text to execution |
| Chapter 2 | [Quick Start: Build and Run](02-quickstart.md) | How to compile `mario`, how to run `.js`, how to dump bytecode |
| Chapter 3 | [Bytecode Instruction Set in Detail](03-bytecode.md) | The 32-bit instruction encoding format, the string table, the meaning and stack behavior of every instruction |
| Chapter 4 | [The Lexer](04-lexer.md) | How source code is sliced into tokens; basic lexing and JS extended lexing |
| Chapter 5 | [The Compiler (Recursive-Descent Parsing)](05-compiler.md) | Expression precedence, statement compilation, how control flow generates jumps |
| Chapter 6 | [The VM Execution Engine](06-vm.md) | The `vm_run` main loop, the instruction dispatch table, the cooperation of the stack and the scope stack |
| Chapter 7 | [Object Model and Scopes](07-object-model.md) | `var_t`/`node_t`, the prototype chain, classes and inheritance, closures |
| Chapter 8 | [Memory Management and Garbage Collection](08-gc.md) | The hybrid refcount + mark-sweep GC, the variable buffer pool |
| Chapter 9 | [Native Extensions and Built-in Classes](09-natives.md) | How to register native functions/classes in C, how arguments are passed |
| Chapter 10 | [Bytecode Files and the Toolchain](10-mbc-and-tools.md) | The `.mbc` precompiled file format, the dump tool, embedding into your own program |
| Chapter 11 | [ES6+ Language Feature Support](11-es6-support.md) | Complete ES6 and later additions: syntax features, built-in objects, the support matrix, and known limitations |

---

## Mario in one minute

```
JavaScript source (text)
        │
        ▼   ① lexing (lex)               ── Chapter 4
   Token stream
        │
        ▼   ② compiler                   ── Chapter 5
   Bytecode (bytecode_t: instruction array + string table)   ── Chapter 3
        │
        ▼   ③ virtual machine (vm_run)   ── Chapter 6
   Stack-based interpretation ──► operates on the var_t object model  ── Chapter 7
        │
        ▼   ④ calls C-implemented native functions (natives)  ── Chapter 9
   Output / side effects
```

The core of the whole engine is just a pair of files: [`mario/mario.h`](../../../mario/mario.h) and [`mario/mario.c`](../../../mario/mario.c). The language frontend (JavaScript here) is **replaceable** — you only need to implement a `bool compile(bytecode_t *bc, const char* input)` function to make Mario run your own language.

---

## Key source-code map

| Path | Role |
| --- | --- |
| `mario/mario.h` / `mario/mario.c` | VM kernel: data structures, bytecode generation, GC, execution engine |
| `mario/lex/mario_lex.*` | The basic lexer (language-agnostic) |
| `mario/bcdump/bcdump.*` | Disassembles bytecode into readable text |
| `lang/js/compiler.c` | The JavaScript compiler (recursive-descent parser) |
| `lang/js/native/...` | Built-in classes: Object / Array / String / Number / Symbol / Error / Map / Set / Promise / Proxy / Reflect / BigInt / ArrayBuffer / DataView / TypedArray / WeakRef / RegExp / JSON / Math / Date, etc. (full list in Chapters 9 and 11) |
| `bin/mario/mario.c` | The command-line runner `main()` |
| `bin/lib/mbc.c` | Reading/writing the `.mbc` bytecode file |
| `test/js/*.js` | Example scripts |
| `mario/demos/` | Examples of embedding Mario into a C program |

> Tip: file links in the docs use relative paths and can be clicked directly in a Markdown-aware editor/repository browser.
