# Chapter 10 · Bytecode Files and the Toolchain

This chapter covers Mario's "supporting infrastructure": the precompiled bytecode file `.mbc`, the module `include` mechanism, and how to embed the VM into your own C program.

## 10.1 Why precompile bytecode

Compilation (lexing + parsing) has a cost. On embedded devices, re-parsing the same script on every startup is both slow and memory-hungry. The idea behind **precompilation** is: compile `.js` into binary bytecode `.mbc` on the development machine, then load `.mbc` directly on the device, skipping the entire compilation phase.

The corresponding command-line usage (already mentioned in Chapter 2):

```bash
./build/mario -c app.js        # generates app.mbc
./build/mario app.mbc          # load and run directly, no compiler needed
```

The implementation lives in [`bin/lib/mbc.c`](../../../bin/lib/mbc.c).

## 10.2 The .mbc file format

`.mbc` simply serializes the two parts of `bytecode_t` (the string table + the instruction array) to a file. The format is:

```
┌────────────────────────────────────────┐
│ MAGIC_NO   (4 bytes) = 0x19760427       │  magic number, validates the file
│ VERSION    (4 bytes) = 0x00000001       │  version number
├────────────────────────────────────────┤
│ mstr_count (4 bytes)                    │  number of string-table entries
│   ┌ len (4 bytes) │ content (len bytes)┐│  per entry: length + content (no trailing \0)
│   └ ... repeated mstr_count times ...  ┘│
├────────────────────────────────────────┤
│ code_size  (4 bytes) = instr count × 4  │  total bytes of the instruction area
│   instruction array (code_size bytes)   │  4 bytes each, a direct memory image
└────────────────────────────────────────┘
```

### Writing: gen_mbc

```c
static bool gen_mbc(int fd, vm_t* vm) {
    write(fd, MAGIC_NO);  write(fd, VERSION);
    // string table: write the count first, then each [length][content]
    PC sz = vm->bc.mstr_table.size;
    write(fd, sz);
    for(i=0; i<sz; ++i) {
        uint32_t len = strlen(str);
        write(fd, len);  write(fd, str, len);
    }
    // instruction area: write the raw bytes of code_buf directly
    sz = vm->bc.cindex * 4;
    write(fd, sz);  write(fd, code_buf, sz);
}
```

### Reading: load_mbc

```c
static bool load_mbc(int fd, vm_t* vm) {
    read(fd, &i, 4);  if(i != MAGIC_NO) return false;   // validate magic number
    read(fd, &version, 4);
    read(fd, &sz, 4);                                    // string table
    for(i=0; i<sz; ++i) {
        read(fd, &len, 4);
        char* s = mario_malloc(len+1);
        read(fd, s, len);  s[len] = 0;
        array_add(&vm->bc.mstr_table, s);
    }
    read(fd, &sz, 4);                                    // instruction area
    vm->bc.code_buf = mario_malloc(sz);
    vm->bc.cindex = sz/4;
    read(fd, code_buf, sz);
}
```

After reading, `vm->bc` is exactly the same as freshly compiled bytecode and can be run directly with `vm_run`.

> Note: `.mbc` is a **platform-dependent** binary format — instructions are direct 4-byte integer images, and string lengths use `uint32_t`. It is not portable across platforms with different endianness or a different `PC` width. It is only safe when the development and target machines share the same architecture.

Before loading `.mbc`, the command-line program first calls `bc_release(&vm->bc)` to clear it (since `vm_new` may already hold compilation output), then `vm_load_mbc`.

## 10.3 Module loading: include

A script can use `include "xxx.js"` to pull in another script (corresponding to `INSTR_INCLUDE`). The loading logic is in [`bin/lib/js.c`](../../../bin/lib/js.c):

```c
#define DEF_LIBS "/usr/local/mario"

static mstr_t* include_script(vm_t* vm, const char* name) {
    const char* path = getenv("MARIO_PATH");   // env var specifies the library dir
    if(path == NULL) path = DEF_LIBS;          // default /usr/local/mario
    // first try the current path via load_script_content(name)
    // if not found, build $MARIO_PATH/libs/<lang>/name (<lang> comes from _mario_lang, e.g. "js")
}
```

The VM calls back into `include_script` through the global function pointer `_load_m_func`. At runtime, `do_include` (`mario.c`) will:

1. Check the `vm->included` list to avoid including the same module twice;
2. Call `_load_m_func` to read the script content;
3. Save the current `pc`, run the included script with `vm_load_run`, then restore `pc`.

At **compile time**, `include` merely emits one `INSTR_INCLUDE` instruction; the actual loading happens at **runtime**, so an included script is only compiled and run when execution reaches that line.

## 10.4 Embedding Mario in your C program

This is Mario's most important use case — as a scripting engine embedded in an application. The minimal skeleton (see [`mario/demos/js_call/demo.c`](../../../mario/demos/js_call/demo.c)):

```c
#include "mario.h"

// ① implement the platform functions
static void out(const char* str) { printf("%s", str); }
void platform_init(void) {
    _platform_malloc = malloc;
    _platform_free   = free;
    _platform_out    = out;
}

// ② declare the language compiler (provided by the lang layer)
bool compile(bytecode_t* bc, const char* input);

int main(void) {
    platform_init();

    // ③ create and initialize the VM
    vm_t* vm = vm_new(compile, VAR_CACHE_MAX_DEF, LOAD_NCACHE_MAX_DEF);
    vm_init(vm, NULL, NULL);          // you can also pass on_init to register your own natives

    // ④ load and run the script
    const char* js = "function jsFunc(s, n){ return \"Hello '\"+s+\"' (\"+n+\")!\\n\"; }";
    vm_load_run(vm, js);

    // ⑤ call a script function from C
    var_t* ret = call_m_func_by_name(vm, NULL, "jsFunc", 2,
                     var_new_str(vm, "JS world"),
                     var_new_int(vm, 100));
    if(ret != NULL) {
        mario_printf("%s", var_get_str(ret));
        var_unref(ret);
    }

    vm_close(vm);   // ⑥ release
    return 0;
}
```

### Key API quick reference

| Phase | Function | Description |
| --- | --- | --- |
| Create | `vm_new(compiler, var_cache_size, ncache_size)` | attach the compiler, create the VM |
| Initialize | `vm_init(vm, on_init, on_close)` | callbacks for registering natives and built-in classes |
| Load | `vm_load(vm, s)` | compile only |
| Run | `vm_load_run(vm, s)` | compile + run |
| Run | `vm_run(vm)` | run already-loaded bytecode |
| Load bytecode | `vm_load_mbc(vm, file)` | load from `.mbc` |
| Generate bytecode | `vm_gen_mbc(vm, file)` | save the compilation result as `.mbc` |
| Call a JS function | `call_m_func_by_name(vm, obj, name, argc, ...)` | call a script function from C |
| Register a native | `vm_reg_static/native/var(...)` | see Chapter 9 |
| Close | `vm_close(vm)` | trigger on_close, reclaim resources |

### C ↔ JS data exchange

- **C calls JS**: `call_m_func_by_name`, passing `var_t*` as variadic arguments, returning a `var_t*` (`var_unref` it when done).
- **JS calls C**: register native functions (Chapter 9); the JS side calls them like ordinary functions.
- **Read/write object members**: `get_obj_member(obj, name)` / `set_obj_member(obj, name, var)`; convenience getters `get_int/get_str/get_float/get_bool`.

## 10.5 The disassembly tool

- **bcdump** ([`mario/bcdump/bcdump.c`](../../../mario/bcdump/bcdump.c)): disassembles a `bytecode_t` into readable text, i.e. the output of the command-line `-a` (see Chapter 3 for details). The core is `bc_dump(bc)`, which returns an `mstr_t*`. The demo programs use it via `#include "bcdump/bcdump.h"`.

> Note: the repository currently provides only the disassembly (dump) tool; a bytecode assembler is not yet included. If you need to rebuild a `.mbc` from textual instructions, you can implement it yourself by following the `bc_dump` format.

Debugging advice: write a script → run `mario -a` to see what instructions it compiles to → cross-reference Chapters 3 and 5 to understand it → then verify the load path with `-c`/`.mbc`. This is the most effective workflow for troubleshooting compile/run issues.

## 10.6 End-to-end walkthrough

Now let's tie the ten chapters together and follow the complete life of `console.log(1+2)`:

```
1. main reads the source-code string                   (Chapter 2)
2. vm_new attaches js_compile; vm_init registers Console (Chapters 1, 9)
3. js_compile:
     lex_get_next_token slices tokens one by one        (Chapter 4)
     statement→base→...→factor recursive descent        (Chapter 5)
     bc_gen_str/bc_gen emit instructions, strings enter mstr_table (Chapter 3)
   result: LOAD "console" / INTS 1 / INTS 2 / PLUS / CALLO "log$1" / POP / END
4. the vm_run main loop fetches and dispatches:          (Chapter 6)
     LOAD → find the console object in root, push it
     INTS/PLUS → the operand stack computes 3
     CALLO → handle_call finds Console.log; func_call invokes native_println
              the native reads arguments from env, _platform_out prints "3\n"  (Chapter 9)
     temporary var_t created along the way are reclaimed by refcount/GC  (Chapter 8)
5. vm_close releases everything                          (Chapter 10)
```

Everything operated on is the `var_t`/`node_t` object model (Chapter 7).

---

## Conclusion

Congratulations on finishing the whole Wiki! You should now be able to:

- Read any piece of Mario bytecode (`mario -a`);
- Understand the complete path a language takes from text to execution;
- Write native functions in C to extend scripting capabilities;
- Embed Mario in your own program and exchange data between C ↔ JS;
- Understand how a small VM does automatic memory management in a resource-constrained environment.

Further learning suggestions:

1. **Modify it hands-on**: add a new syntax to the compiler (e.g. a regex literal `/.../`), or add a new method to a built-in class (e.g. `String.prototype.reverse`).
2. **Step through it**: compile with `MARIO_DEBUG=yes` and use `mario_debug` to observe the execution flow.
3. **Read the tests**: `test/js/*.js` are the best behavioral spec — read them, predict the bytecode, then verify with `-a`. Especially [`test/js/es6_full.js`](../../../test/js/es6_full.js), which is a complete ES6+ behavioral spec.

As a JavaScript implementation, exactly which ES6+ syntax and built-in objects does Mario support? The complete feature matrix, code examples, and known limitations are in [Chapter 11 · ES6+ Language Feature Support](11-es6-support.md).

Back to the [Wiki Home](README.md).
