# Chapter 2 · Quick Start: Build and Run

This chapter gets Mario running and introduces the several ways to use the command-line tool.

## 2.1 Build

The project uses GNU Make. In the repository root, run:

```bash
make
```

On success it produces the executable `build/mario` in the `build/` directory and prints `done`.

The build is driven jointly by the root [`Makefile`](../../../Makefile) and [`lang/js/lang.mk`](../../../lang/js/lang.mk):

- `Makefile` compiles the kernel (`mario/mario.o`, `mario/lex/mario_lex.o`, `mario/bcdump/bcdump.o`) and the command-line program (`bin/mario/*`).
- `lang.mk` lists the object files of the JS language layer and all built-in native classes.

To clean build artifacts:

```bash
make clean
```

### Enable debugging (optional)

```bash
make MARIO_DEBUG=yes
```

This adds `-g -DMARIO_DEBUG` and enables the memory-debugging allocator (`malloc_debug`/`free_debug`) in `bin/lib/mem_debug.c`, used to track down memory leaks and out-of-bounds accesses.

## 2.2 Run a script

The simplest usage is to pass the `.js` file directly as an argument:

```bash
./build/mario test/js/class.js
```

The program reads the file → compiles it to bytecode → executes it. There are several example scripts under `test/js/` you can try right away:

| File | Demonstrates |
| --- | --- |
| `test/js/class.js` | functions, classes, inheritance, `super`, `for...in` |
| `test/js/closure.js` | closures |
| `test/js/promise.js` | Promise |
| `test/js/string.js` | built-in string methods |
| `test/js/grammar_es5.js` | comprehensive ES5 grammar |
| `test/js/bench.js` | performance benchmark |

## 2.3 Command-line arguments

Argument parsing lives in `doargs()` in [`bin/mario/mario.c`](../../../bin/mario/mario.c), handled via `getopt` with the option string `"cda"`. The usage message the program actually prints is:

```
Usage: mario (-c/d/a) <filename>
```

An optional `[output]` argument may follow `<filename>` (used as the output filename only in `-c` compile mode):

| Argument | Meaning |
| --- | --- |
| none | **Run mode**: compile and execute the script |
| `-a` | **Dump mode**: compile only, disassemble the bytecode into readable text and print it, without executing |
| `-c` | **Compile mode**: compile only, write the bytecode into a `.mbc` file (precompilation) |

> Note: the `getopt` option string in the source is `"cda"`, but the current `doargs()` only actually handles `c`/`a`; `-d` is not wired up yet.
>
> Also, at the very start `main()` calls `setvbuf(stdout, NULL, _IOLBF, 0)` to make standard output **line-buffered**, so that even when output is redirected to a file or pipe, `console.log` content appears promptly instead of waiting for the 4 KiB buffer to fill or the process to exit.

## 2.4 Inspecting bytecode: `-a`

This is the most useful feature for learning Mario. Write a minimal script:

```javascript
// /tmp/t.js
var a = 1 + 2;
console.log(a);
```

Run:

```bash
./build/mario -a /tmp/t.js
```

You get the real output:

```
mstr_index| value
---------------------------------------
0x000000 | this
0x000001 | a
0x000002 | console
0x000003 | log$1

pc_index | opr_code   ; instruction
---------------------------------------
00000000 | 0x00100001 ; VAR     "a"
00000001 | 0x00300001 ; LOAD    "a"
00000002 | 0x00D00001 ; INTS    1
00000003 | 0x00D00002 ; INTS    2
00000004 | 0x01EFFFFF ; PLUS
00000005 | 0x004FFFFF ; ASIGN
00000006 | 0x04AFFFFF ; POP
00000007 | 0x00300002 ; LOAD    "console"
00000008 | 0x00300001 ; LOAD    "a"
00000009 | 0x01300003 ; CALLO   "log$1"
00000010 | 0x04AFFFFF ; POP
00000011 | 0x058FFFFF ; END
---------------------------------------
```

The output has two parts:

1. **String table (mstr_table)**: all identifiers/string constants that appear; instructions reference them by "index", avoiding embedding strings inside instructions.
2. **Instruction sequence**: each line is `pc | machine code ; mnemonic operand`.

Don't rush to understand every column; Chapter 3 breaks down this 32-bit machine code thoroughly. Here, just build an intuition:

- `VAR "a"` declares the variable `a`;
- `LOAD "a"` pushes `a` onto the stack (as the assignment target);
- `INTS 1` / `INTS 2` push the two integers;
- `PLUS` pops the two numbers, adds them, pushes the result back;
- `ASIGN` assigns the top-of-stack value to the target below it;
- `POP` discards the statement result;
- `CALLO "log$1"` calls the object member method `log` (`$1` means 1 argument);
- `END` finishes.

## 2.5 Precompiling: `-c`

Compile a script into a `.mbc` binary bytecode file, which can then be loaded and run directly, saving repeated compilation:

```bash
# No output name given: automatically replaces the input's .js with .mbc, producing /tmp/t.mbc
./build/mario -c /tmp/t.js

# You can also specify the output filename explicitly
./build/mario -c /tmp/t.js /tmp/out.mbc

# Run the precompiled bytecode directly
./build/mario /tmp/t.mbc
```

> When `-c` is not followed by an output filename, `main()` takes the input filename and replaces its `.js` with `.mbc` as the default output name.

The `.mbc` file read/write implementation is in [`bin/lib/mbc.c`](../../../bin/lib/mbc.c); format details are in Chapter 10.

## 2.6 Common runtime issues

- **`Failed to create VM...`**: means the three platform function pointers (`_platform_malloc`/`_platform_free`/`_platform_out`) are not set. The command-line program already sets them in `platform_init()`; if you embed Mario in your own program, be sure to set them first (see Chapter 10).
- **`compile error at (line: X, col: Y)`**: a compile-time syntax error; the position is computed by `compile_error_pos()` through the lexer.
- **Script produces no output**: confirm you called `console.log(...)`, which is provided by the built-in `Console` class (see Chapter 9).

Next: [Chapter 3 · Bytecode Instruction Set](03-bytecode.md), to fully understand that string of machine code above.
