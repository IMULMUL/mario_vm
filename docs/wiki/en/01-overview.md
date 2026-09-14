# Chapter 1 · Project Overview and Architecture

## 1.1 What is Mario

Mario is a **bytecode virtual machine (VM)** written in pure C. Its design goals can be summed up in three words:

- **Small**: the core engine is essentially a single pair of files, `mario.c` + `mario.h`, only a few thousand lines of code.
- **Dependency-free**: it relies on no third-party library, and even abstracts `malloc`/`free`/`print` into "platform function pointers", making it easy to port to embedded systems.
- **Language-pluggable**: the VM itself knows nothing about JavaScript. It only knows **bytecode**. The language frontend (lexer + compiler) is a separate module hooked into the VM through a function pointer.

This repository `mario_vm` = **Mario kernel** + **JavaScript language frontend** + **built-in class library** + **command-line tool**.

## 1.2 Layered structure

From top to bottom, the whole system is divided into four layers:

```
┌─────────────────────────────────────────────┐
│  App layer: bin/mario/mario.c (CLI main)      │  run .js / .mbc, dump bytecode
├─────────────────────────────────────────────┤
│  Language layer: lang/js/                     │  JS lexer, compiler, built-in native classes
│    - compiler.c      (JS → bytecode)          │
│    - native/...      (Console/String/Array...) │
├─────────────────────────────────────────────┤
│  Kernel layer: mario/mario.c                  │  bytecode structs, object model, GC, execution engine
│    - bytecode_t / var_t / node_t / vm_t        │
│    - vm_run() main loop                        │
├─────────────────────────────────────────────┤
│  Platform layer: _platform_malloc/free/out    │  three function pointers provided by the host
└─────────────────────────────────────────────┘
```

The **platform layer** is the key to porting. Look at `platform_init()` in [`bin/mario/mario.c`](../../../bin/mario/mario.c):

```c
void platform_init(void) {
    _platform_malloc = malloc;   // can also be the embedded system's own allocator
    _platform_free   = free;
    _platform_out    = out;      // output a string, usually to a serial port / stdout
}
```

As long as you implement these three functions on your platform, the Mario kernel can run.

## 1.3 The lifecycle of a script

Taking `test/js/class.js` as an example, here is how the data flows:

```
1. main() reads the .js file contents into a string s
2. vm_new(js_compile, ...) creates the VM and hooks in the JS compiler function pointer
3. vm_init(vm, reg_all_natives, ...) registers all built-in classes (Console/String/...)
4. vm_load(vm, s)
     └─► calls js_compile(bc, s)
           ├─ Lexical analysis: char stream → token stream
           └─ Syntax analysis: token stream → bytecode, written into bc.code_buf, strings into bc.mstr_table
5. vm_run(vm)
     └─► starting from pc=0, fetches each 32-bit instruction and dispatches it to the corresponding handler
           └─ handlers operate on var_t objects, push/pop, jump, call functions
6. vm_close(vm) releases resources
```

Step 4 (compilation) and step 5 (execution) are two completely separate phases. This means you can **compile without running** (produce a `.mbc` file, see Chapter 10), or **skip compilation and directly load** already-compiled bytecode.

## 1.4 Core data structures at a glance

These structures are all defined in [`mario/mario.h`](../../../mario/mario.h). Later chapters expand on them in detail; here we just build an overall impression:

| Structure | One-line description | See |
| --- | --- | --- |
| `bytecode_t` | Compilation product: an instruction array `code_buf` + a string table `mstr_table` | Chapter 3 |
| `vm_t` | VM instance: holds the bytecode, operand stack, scope stack, GC state, built-in variables | Chapter 6 |
| `var_t` | A "value" in the script world: integer/float/string/object/boolean/null/undefined, uniformly represented | Chapter 7 |
| `node_t` | A "member" of an object: a name + a `var_t`, the carrier of properties/array elements | Chapter 7 |
| `func_t` | A function: either a script function (records the bytecode entry `pc`) or a native function (records a C function pointer `native`) | Chapters 7, 9 |
| `scope_t` | A scope: functions/blocks/loops/try each form a scope layer, chained into a scope stack | Chapters 6, 7 |
| `lex_t` | Lexer state | Chapter 4 |

## 1.5 The stack-based execution model

Mario is a **stack-based** virtual machine, not a register-based one. All computation revolves around a single operand stack:

- To compute `1 + 2`: first push `1`, then push `2`, then the `PLUS` instruction pops the two values, adds them, and pushes the result back.
- This model has few instructions and a simple compiler implementation, which suits a small VM very well.

There are actually two "stacks" in the VM — don't confuse them:

1. **Operand stack** `vm->stack[]`: holds intermediate computation values (`var_t*` or `node_t*`).
2. **Scope stack** `vm->scope_stack[]`: holds nested scopes (function calls, code blocks, loops, try).

Chapter 6 explains in detail how these two stacks cooperate.

## 1.6 Why this project is worth studying

- It is **an excellent textbook for understanding "how a language is implemented"**: lexing → parsing → bytecode → execution, the whole pipeline in a few thousand lines of C, without heavy engineering noise.
- It shows **how to do memory management in a resource-constrained environment**: a hybrid GC of reference counting + mark-and-sweep, plus a variable-object buffer pool for reuse.
- It shows **how to design for extensibility**: a replaceable language frontend, registrable native classes, and injectable platform functions.

When you're ready, move on to [Chapter 2 · Quick Start](02-quickstart.md) and get it running first.
