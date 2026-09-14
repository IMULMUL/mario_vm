# Mario VM

**Mario VM** is a tiny, dependency-free **bytecode virtual machine** written in pure C, bundled with a complete **JavaScript** language frontend. It is designed to be small enough to embed on resource-constrained devices, yet complete enough to run a modern ES6+ JavaScript dialect.

This repository = **Mario kernel** + **JavaScript frontend (lexer + compiler)** + **built-in native class library** + **command-line runner**.

> The kernel is based on [Mario](https://github.com/misazhu/mario.git); this project extends it with native classes, dynamic script/module loading, and a full ES6+ JavaScript frontend.

---

## Highlights

- **Tiny & portable** — the engine core is essentially `mario/mario.c` + `mario/mario.h`. No third-party libraries; even `malloc`/`free`/`print` are abstracted into three platform function pointers, so it ports easily to embedded systems.
- **Stack-based bytecode VM** — a 32-bit instruction set, an operand stack, and a scope stack. Compilation and execution are fully separated phases.
- **Complete ES6+ JavaScript** — `let`/`const`, template literals, arrow functions, destructuring, `class`, `for...of`, generators, `async`/`await`, `**`, optional chaining, nullish coalescing, `BigInt`, and more.
- **Rich built-in classes** — `Object`, `Array`, `String`, `Number`, `Symbol`, `Error`, `Map`/`Set`/`WeakMap`/`WeakSet`, `Promise`, `Proxy`/`Reflect`, `BigInt`, `ArrayBuffer`/`DataView`/`TypedArray`, `SharedArrayBuffer`/`Atomics`, `WeakRef`/`FinalizationRegistry`, `RegExp`, `JSON`, `Math`, `Date`, `Console`.
- **Hybrid garbage collection** — reference counting as the primary strategy, mark-and-sweep as a safety net, plus a free variable buffer pool for reuse and opportunistic GC safe-points in the run loop.
- **Precompiled bytecode** — compile `.js` into a binary `.mbc` once and load it directly on-device, skipping the whole compilation phase.
- **Embeddable** — a clean C API to create a VM, run scripts, and call JavaScript functions from C (and vice versa).
- **Language-pluggable** — the VM knows only bytecode. Implement one `compile()` function and you can run your own language on Mario.

### Test status

The repository ships a standards-based ES6+ test suite. A clean build passes all of it:

```
=== es6_full.js: 854 passed, 0 failed ===
ALL TESTS PASSED
```

---

## Build

Requirements: a C compiler (`gcc`/`clang`) and `make`.

```bash
make                 # produces build/mario
```

For a debug build (with `-g` and memory-debug instrumentation):

```bash
make clean
make MARIO_DEBUG=yes
```

> **Tip:** always `make clean` before `make` after pulling changes that touch core data structures. Mixing stale object files with newly compiled ones can cause an ABI mismatch and crash at startup.

---

## Run

```bash
./build/mario <file.js>            # compile and run a JavaScript file
./build/mario <file.mbc>           # load and run precompiled bytecode (no compiler needed)
```

Command-line options:

| Option | Meaning |
| --- | --- |
| *(none)* | Compile (if `.js`) and run the script |
| `-c` | **Compile only** — generate a `.mbc` bytecode file and exit. The output name defaults to the input with `.js` replaced by `.mbc`; pass a second argument to choose it explicitly |
| `-a` | **Disassemble** — dump the compiled bytecode as readable text instead of running it |
| `-d` | Accepted for compatibility (debug placeholder) |

Examples:

```bash
./build/mario test/js/es6_full.js          # run the full ES6+ suite
./build/mario -a test/js/class.js          # see the bytecode class.js compiles to
./build/mario -c app.js                    # produce app.mbc
./build/mario -c app.js out/app.mbc        # produce a named .mbc
./build/mario app.mbc                      # run the precompiled bytecode
```

Command-line arguments after the script name are exposed to the script as a global array `_args`.

---

## Embedding in C

Mario's most important use case is as a scripting engine embedded in an application. Minimal skeleton (see [`mario/demos/js_call/demo.c`](mario/demos/js_call/demo.c)):

```c
#include "mario.h"

/* 1. Implement the platform functions. */
static void out(const char* str) { printf("%s", str); }
void platform_init(void) {
    _platform_malloc = malloc;
    _platform_free   = free;
    _platform_out    = out;
}

/* 2. Declare the language compiler (provided by the lang layer). */
bool compile(bytecode_t* bc, const char* input);

int main(void) {
    platform_init();

    /* 3. Create and initialize the VM. */
    vm_t* vm = vm_new(compile, VAR_CACHE_MAX_DEF, LOAD_NCACHE_MAX_DEF);
    vm_init(vm, NULL, NULL);          /* pass on_init to register your own natives */

    /* 4. Load and run a script. */
    const char* js = "function jsFunc(s, n){ return \"Hello '\"+s+\"' (\"+n+\")!\\n\"; }";
    vm_load_run(vm, js);

    /* 5. Call a script function from C. */
    var_t* ret = call_m_func_by_name(vm, NULL, "jsFunc", 2,
                     var_new_str(vm, "JS world"),
                     var_new_int(vm, 100));
    if (ret != NULL) {
        mario_printf("%s", var_get_str(ret));
        var_unref(ret);
    }

    vm_close(vm);   /* 6. Release. */
    return 0;
}
```

To expose your own C functions to scripts, register native classes/functions with `vm_reg_static` / `vm_reg_native` / `vm_reg_var` — see the [Native Extensions](https://github.com/MisaZhu/mario_js/wiki/09-natives) chapter.

---

## Project structure

```
mario/                     VM kernel (language-agnostic)
├── mario.h / mario.c        data structures, bytecode generation, GC, execution engine
├── lex/mario_lex.*          basic lexer
├── bcdump/bcdump.*          bytecode disassembler
└── demos/                   embedding examples (js_call, tinyjs)

lang/js/                   JavaScript language frontend
├── compiler.c               recursive-descent compiler (JS → bytecode)
├── lang.mk                  build rules and native object list
└── native/
    ├── builtin/             Object, Array, String, Number, Symbol, Error, Console,
    │                        Map/Set, Promise, Proxy/Reflect, BigInt, ArrayBuffer/
    │                        DataView/TypedArray, SharedArrayBuffer/Atomics,
    │                        WeakRef/FinalizationRegistry, RegExp
    └── natives/             Math, Date, JSON

bin/
├── mario/mario.c            command-line runner (main)
└── lib/                     mbc.c (.mbc read/write), js.c (script/include loading)

test/js/                   example scripts and the ES6+ test suite
docs/wiki/                 documentation (zh/ and en/)
build/mario                the compiled binary
```

---

## Documentation

A complete, chapter-by-chapter guide lives in [`docs/wiki/`](docs/wiki) in both Chinese and English, and is published on the project Wiki:

**👉 [https://github.com/MisaZhu/mario_js/wiki](https://github.com/MisaZhu/mario_js/wiki)**

| # | Chapter |
| --- | --- |
| 1 | Project Overview and Architecture |
| 2 | Quick Start: Build and Run |
| 3 | Bytecode Instruction Set in Detail |
| 4 | The Lexer |
| 5 | The Compiler (Recursive-Descent Parsing) |
| 6 | The VM Execution Engine |
| 7 | Object Model and Scopes |
| 8 | Memory Management and Garbage Collection |
| 9 | Native Extensions and Built-in Classes |
| 10 | Bytecode Files and the Toolchain |
| 11 | ES6+ Language Feature Support |

---

## Known limitations

- **No ES Modules** — `import`/`export` are unsupported; Mario is a single-file script engine. Use the `include "..."` mechanism or `.mbc` for cross-file composition.
- **No regex literals** `/.../` — construct regular expressions with `new RegExp("pattern", "flags")` (the `RegExp` object itself is fully supported).
- **No `Boolean` wrapper class** — boolean primitives work fully; only the `Boolean()` constructor is absent.

See [Chapter 11 · ES6+ Language Feature Support](https://github.com/MisaZhu/mario_js/wiki/11-es6-support) for the complete feature matrix.

---

## License

See [LICENSE](LICENSE).
