# Chapter 11 · ES6+ Language Feature Support

The previous chapters explained "how the engine works". This chapter explains "as a JavaScript implementation, exactly which syntax and built-in objects does it support" — that is, the degree to which Mario's JS frontend supports **ES6 (ES2015) and its subsequent incremental additions**.

Conclusion first: **Mario has implemented the complete ES6 feature set**, and additionally covers the most commonly mentioned additions from ES2016–ES2022 (`async/await`, object rest/spread, the exponentiation operator, `BigInt`, typed arrays, `Proxy/Reflect`, `WeakRef`, `SharedArrayBuffer/Atomics`, etc.).

> Authoritative basis: the repository's [`test/js/es6_full.js`](../../../test/js/es6_full.js) is a test suite **written against the ECMA-262 standard**, with **854 assertions, all passing** (`=== es6_full.js: 854 passed, 0 failed ===`). Every "supported" claim in this chapter maps to a concrete assertion there.

---

## 11.1 Support overview

### 11.1.1 Run it in one minute

```bash
make                              # builds build/mario
./build/mario test/js/es6_full.js # runs the full ES6 test suite
```

Normally you'll see 38 sections print `✓` line by line, ending with the summary `854 passed, 0 failed` and `ALL TESTS PASSED`.

### 11.1.2 Two-layer implementation

ES6 support lands at two layers, corresponding exactly to the two major modules from the previous chapters:

```
ES6 source
  │
  ├─ Syntax layer: lang/js/compiler.c        (Chapter 5)
  │    let/const, template literals, arrow functions, destructuring, class,
  │    for...of, generators, async/await, the ** operator …
  │
  └─ Built-in object layer: lang/js/native/...     (Chapter 9)
       Map/Set, Promise, Proxy/Reflect, BigInt,
       ArrayBuffer/TypedArray, RegExp, Symbol …
```

- The **syntax layer** has the compiler translate new syntax into Chapter 3's bytecode instructions (for example, template strings generate `INSTR_STR` + concatenation, and tagged templates also introduce `INSTR_TAG_RAW`).
- The **built-in object layer** is a batch of native classes written in C, registered by `reg_all_natives` at VM initialization (see Chapter 9).

### 11.1.3 Feature support matrix

The table below summarizes the 38 sections of `es6_full.js`; all are ✅ supported:

| # | Feature area | Key capabilities |
| --- | --- | --- |
| 1 | `let` / `const` | block scoping, TDZ, per-iteration fresh binding in `for-let`, const non-rebindable |
| 2 | Template literals | interpolation, multiline, nesting, escapes, **tagged templates** + `String.raw` |
| 3 | Default parameters | omitted/`undefined` triggers default, defaults may reference earlier params, evaluated at call time |
| 4 | Rest parameters | `...args` collected into a real array |
| 5 | Spread / object rest | array/call/object spread, object rest destructuring |
| 6 | Destructuring | array/object/nested/defaults/swap/parameter destructuring |
| 7 | Object literal enhancements | shorthand, computed keys, methods, getter/setter, `__proto__` |
| 8 | Arrow functions | lexical `this`, no `[[Construct]]`, no own `arguments` |
| 9 | class | constructor, methods, static, `extends`/`super`, accessors |
| 10 | `for...of` / `for...in` | iteration protocol, `Symbol.iterator` |
| 11 | Generators | `function*`, `yield`, `yield*` |
| 12 | Symbol | uniqueness, well-known symbols, symbol as key |
| 13 | Collections | `Map` / `Set` / `WeakMap` / `WeakSet` |
| 14 | Promise | state machine, `then/catch/finally`, `all/allSettled/race` |
| 15 | `async` / `await` | async functions, await unwrapping, error propagation |
| 16 | Array static & prototype | `from/of/find/includes/flat/...` |
| 17 | String methods | `startsWith/includes/repeat/padStart/...` |
| 18 | Number & Math | `Number.isInteger/isNaN/...`, `Math` additions |
| 19 | Object static methods | `assign/is/keys/values/entries/freeze/...` |
| 20 | Operators | exponentiation `**`, `new.target`, `typeof`/`instanceof` edge cases |
| 21 | Closures & loop capture | the classic `let` vs `var` capture difference |
| 22 | Interop & corner cases | semantic consistency across the board |
| 23 | Global functions & `globalThis` | `isNaN/parseInt/parseFloat`, `globalThis` |
| 24 | Error subtypes | `TypeError/RangeError/...`, `AggregateError` |
| 25 | Object static (ES2022) | `hasOwn/setPrototypeOf/seal/preventExtensions/...` |
| 26 | `Array.at` & String additions | `at/trimStart/End/replaceAll/...` |
| 27 | `Promise.any` (ES2021) | resolves on the first success |
| 28 | JSON | `stringify`/`parse`, round-trip consistency |
| 29 | Date | construction, field accessors, `toISOString`, `Date.now/parse/UTC` |
| 30 | `undefined`/`null` equality | loose/strict equality semantics |
| 31 | Built-in additions & logical operators | `&&`/`||` returning operand semantics, etc. |
| 32 | 64-bit numbers | exact int64 big integers, double precision, `MAX_SAFE_INTEGER` |
| 33 | BigInt | arbitrary-precision literals/arithmetic/comparison/bitwise, radix conversion |
| 34 | ArrayBuffer & DataView | byte buffers, reads/writes of each width, endianness, slice |
| 35 | TypedArray | the full set from `Int8Array … BigUint64Array`, constructors and methods |
| 36 | Proxy / Reflect | metaprogramming traps, 13 `Reflect` static methods |
| 37 | WeakRef / FinalizationRegistry | weak references, `deref`, cleanup callbacks |
| 38 | SharedArrayBuffer / Atomics | shared memory, atomic load/store/add/… |

### 11.1.4 Explicitly **unsupported** items

To avoid overstatement, here are the current boundaries as well (see [11.4 Known limitations](#114-known-limitations)):

- **ES Modules**: `import` / `export` are not supported. Mario is a single-file script engine.
- **Regex literals `/.../`**: the lexer has no regex-literal token; use `new RegExp("...")` to construct a regex (the `RegExp` object itself is supported).
- **`Boolean` wrapper class**: the `Boolean` constructor is not registered (boolean values themselves work fully).

---

## 11.2 Syntax-layer features (compiler)

All the syntax in this section is implemented in [`lang/js/compiler.c`](../../../lang/js/compiler.c); the compilation output is Chapter 3's bytecode.

### 11.2.1 `let` / `const` and block scoping

- `let`/`const` are **block-scoped**: `x` in `{ let x = 1; }` does not leak outside the block; `var` remains function-scoped and leaks.
- **TDZ (Temporal Dead Zone)**: reading a `let`/`const` before its declaration throws.
- `for (let i = ...)` **creates a fresh `i` binding on each iteration**, so closures created in the loop each capture the correct value; `for (var j = ...)` shares the same `j`.
- A `const` binding cannot be reassigned, but an object's **contents** remain mutable.

```javascript
const fns = [];
for (let i = 0; i < 3; i++) fns.push(() => i);
fns.map(f => f());            // [0, 1, 2] — each closure gets its own i

const vfns = [];
for (var j = 0; j < 3; j++) vfns.push(() => j);
vfns.map(f => f());           // [3, 3, 3] — shares the same j

const obj = { n: 1 };
obj.n = 2;                    // OK: contents are mutable
// obj = {};                  // throws: const binding cannot be reassigned
```

### 11.2.2 Template literals and tagged templates

Ordinary template literals are handled by `factor_template`: it scans the raw character stream, compiling text chunks and `${...}` expressions alternately into string concatenation. It supports interpolation, multiline, nesting, and escape sequences; interpolation calls `toString` on objects.

```javascript
const who = "World";
`Hello, ${who}!`;             // "Hello, World!"
`${1 + 2} = ${3}`;            // "3 = 3"
`line1
line2`;                       // preserves the newline "line1\nline2"
```

**Tagged templates** are handled by `factor_tagged_template`: it builds the "cooked" string array and its `raw` array (attached to `strings.raw` via `INSTR_TAG_RAW`), then calls the tag function as `(strings, ...values)`.

```javascript
function tag(strings, ...values) {
    let out = "";
    for (let i = 0; i < strings.length; i++) {
        out += strings[i];
        if (i < values.length) out += "[" + values[i] + "]";
    }
    return out;
}
tag`a${1}b${2}c`;             // "a[1]b[2]c"

function raw(strings) { return strings.raw[0]; }
raw`a\nb`;                    // "a\\nb" (equivalent to String.raw)
```

### 11.2.3 Arrow functions

Arrow functions use **lexical `this`** (inheriting the outer `this` at the definition site), have no own `arguments`, and cannot be `new`-ed.

```javascript
const obj = {
    name: "mario",
    greet() {
        const f = () => this.name;   // the arrow captures this at the definition site
        return f();
    }
};
obj.greet();                  // "mario"
[1, 2, 3].map(x => x * 2);    // [2, 4, 6]
```

### 11.2.4 Default parameters / rest / spread

```javascript
// Default parameters: triggered when omitted or passed undefined; may reference earlier params; evaluated at call time
function power(base, exp = 2) { return base ** exp; }
power(5);                     // 25
power(5, undefined);          // 25 (undefined triggers the default)
power(5, null);               // 1 (null does not trigger the default, 5 ** 0)

// rest: collected into a real array
function sum(...nums) { return nums.reduce((a, b) => a + b, 0); }
sum(1, 2, 3, 4);              // 10

// spread: array literal / call arguments / object
const base = [1, 2, 3];
[0, ...base, 4];              // [0, 1, 2, 3, 4]
Math.max(...base);            // 3
const o2 = { ...{ a: 1 }, b: 2 };   // { a: 1, b: 2 }
```

### 11.2.5 Destructuring assignment

Array destructuring, object destructuring, nested destructuring, defaults, swap, and function-parameter destructuring are all supported; object rest (`...rest`) can also be used in destructuring.

```javascript
const [a, b] = [1, 2];                 // a=1, b=2
const { x, y = 10 } = { x: 1 };        // x=1, y=10 (default)
const { p, ...rest } = { p: 1, q: 2, r: 3 };  // p=1, rest={q:2,r:3}
let m = 1, n = 2; [m, n] = [n, m];     // swap: m=2, n=1
function dist({ lat, lng }) { return lat + "," + lng; }  // parameter destructuring
```

### 11.2.6 Object literal enhancements

Property shorthand, computed property names, method shorthand, getter/setter, and `__proto__` for setting the prototype.

```javascript
const name = "vm";
const o = {
    name,                        // shorthand: { name: name }
    ["k_" + 1]: "v",             // computed key: k_1
    greet() { return "hi"; },    // method shorthand
    get size() { return 42; },   // getter
    set size(v) { /* ... */ },   // setter
};
const proto = { hello() { return "world"; } };
const child = { __proto__: proto };
child.hello();                   // "world"
```

### 11.2.7 class and inheritance

`class` supports `constructor`, instance methods, `static` methods, `extends` inheritance, `super` for calling the parent constructor and methods, and getter/setter accessors within a class.

```javascript
class Animal {
    constructor(name) { this.name = name; }
    speak() { return this.name + " makes a sound"; }
    static create(name) { return new Animal(name); }
}
class Dog extends Animal {
    constructor(name) { super(name); }         // call the parent constructor
    speak() { return super.speak() + ": woof"; } // call the parent method
}
new Dog("Rex").speak();         // "Rex makes a sound: woof"
```

### 11.2.8 `for...of` and the iteration protocol

`for...of` iterates any object implementing the iteration protocol (`Symbol.iterator`): arrays, strings, `Map`, `Set`, generators, typed arrays, etc. `for...in` iterates enumerable keys.

```javascript
for (const ch of "abc") { /* 'a','b','c' */ }

const m = new Map([["k", 1]]);
for (const [key, val] of m) { /* iterate key-value pairs with array destructuring */ }
```

### 11.2.9 Generators (`function*` / `yield`)

```javascript
function* gen() { yield 1; yield 2; }
const it = gen();
it.next();                      // { value: 1, done: false }
it.next();                      // { value: 2, done: false }
it.next();                      // { value: undefined, done: true }
```

`yield*` delegation to another iterable is supported, and generators can also serve as the data source for `for...of`.

### 11.2.10 `async` / `await`

An `async` function returns a Promise; `await` unwraps async values inside it; errors propagate via `try/catch`. Together with the `Promise` in section 14, it forms a complete asynchronous programming model.

```javascript
async function load() {
    const v = await Promise.resolve(42);
    return v * 2;
}
load().then(r => console.log(r));   // 84
```

### 11.2.11 New operators

- **Exponentiation operator `**`**: `2 ** 10 === 1024`, right-associative, equivalent to `Math.pow`.
- **`new.target`**: in a constructor call it points to the constructor being `new`-ed; useful for distinguishing "new-ed" from "ordinary call".

---

## 11.3 Built-in object layer (native classes)

Built-in classes are located in [`lang/js/native/`](../../../lang/js/native/); the registration entry point is in Chapter 9. The current complete list is as follows (far richer than early versions):

```
native/
├── builtin/            # language-level basic classes (reg_builtin_natives)
│   ├── Object/  Error/  Array/  String/  Number/  Symbol/  Console/
│   ├── Map/     Set/                       # includes WeakMap / WeakSet
│   ├── Promise/                            # + async/await runtime
│   ├── Proxy/   Reflect/                   # metaprogramming
│   ├── BigInt/                             # arbitrary-precision integers
│   ├── ArrayBuffer/ DataView/ TypedArray/  # binary & typed arrays
│   ├── SharedArrayBuffer/ Atomics/         # shared memory & atomic ops
│   ├── WeakRef/ FinalizationRegistry/      # weak references & cleanup callbacks
│   └── RegExp/                             # regex object (new RegExp)
└── natives/            # extension classes (reg_natives)
    ├── Math/  Date/  JSON/
```

> Registration order ([`natives_builtin.c`](../../../lang/js/native/builtin/natives_builtin.c)): Object → Error → Array → String → Console → Number → BigInt → ArrayBuffer → DataView → TypedArray → Promise → Map → Set → Symbol → Proxy → Reflect → WeakRef → FinalizationRegistry → SharedArrayBuffer → Atomics → RegExp; then `load_basic_classes` caches common class pointers, creates the global `console`, and registers the global `Infinity` / `NaN`; finally `reg_natives` registers Math / Date / JSON.

### 11.3.1 Collections: Map / Set / WeakMap / WeakSet

`Map`/`Set` support full add/delete/lookup/traversal and the iteration protocol; `WeakMap` (registered in [`native_Map.c`](../../../lang/js/native/builtin/Map/native_Map.c)) and `WeakSet` (registered in [`native_Set.c`](../../../lang/js/native/builtin/Set/native_Set.c)) key on objects and hold weak references.

```javascript
const m = new Map([["a", 1]]);
m.set("b", 2); m.get("a");      // 1
m.has("b"); m.size;             // true, 2
for (const [k, v] of m) { /* iterate */ }

const s = new Set([1, 2, 2, 3]);
s.size;                         // 3 (auto-deduplicated)

const wm = new WeakMap(); const key = {};
wm.set(key, "v"); wm.get(key);  // "v"
```

### 11.3.2 Metaprogramming: Proxy / Reflect

`Proxy` supports traps such as `get/set/has/deleteProperty/ownKeys/apply/construct`, plus prototype, extensibility, and descriptor-related traps and revocable proxies; `Reflect` provides the corresponding 13 static methods.

```javascript
const p = new Proxy({ a: 1 }, {
    get(t, k) { return k in t ? t[k] : 0; }
});
p.a;                            // 1
p.missing;                      // 0 (intercepted by the get trap)
Reflect.ownKeys({ a: 1 });      // ["a"]
Reflect.has({ a: 1 }, "a");     // true
```

### 11.3.3 Binary and typed arrays

`ArrayBuffer` + `DataView` provide byte-wise, width-wise, optionally endianness-aware reads/writes; `TypedArray` covers the full set from `Int8Array` to `BigUint64Array`, supporting construction from length/array/typed array/buffer, plus methods such as `from/of/subarray/slice/set/fill/sort/map/filter/reduce/...` and `@@iterator`. `SharedArrayBuffer` + `Atomics` provide shared memory and atomic operations (`load/store/add/sub/and/or/xor/exchange/compareExchange`).

```javascript
const buf = new ArrayBuffer(8);
const dv = new DataView(buf);
dv.setInt32(0, 42); dv.getInt32(0);       // 42

const i32 = new Int32Array([1, 2, 3]);
i32.length; i32.map(x => x * 2);          // 3, [2,4,6]

const sab = new SharedArrayBuffer(4);
const view = new Int32Array(sab);
Atomics.add(view, 0, 5);                  // returns the old value
```

### 11.3.4 Big integers: BigInt and 64-bit numbers

`BigInt` provides arbitrary-precision integers: the `n` suffix literal, arithmetic/comparison/bitwise operations, `toString(radix)`, `BigInt()`/`Number()` conversion, `asIntN`/`asUintN`. The engine also handles 64-bit integers exactly (big-integer literals and operations don't lose precision; values beyond the safe range are promoted to double). BigInt literals are supported by the lexer's `LEX_BIGINT` token.

```javascript
const big = 9007199254740993n;            // exceeds 2^53, still exact
big + 1n;                                 // 9007199254740994n
(255n).toString(16);                      // "ff"
Number.MAX_SAFE_INTEGER;                  // 9007199254740991
```

### 11.3.5 Weak references: WeakRef / FinalizationRegistry

`WeakRef` holds a weak reference to its target; `deref()` returns the target while it's alive and `undefined` after it's collected; `FinalizationRegistry` fires a cleanup callback after the target is collected, supporting `register`/`unregister`.

```javascript
let target = { id: 1 };
const ref = new WeakRef(target);
ref.deref().id;                           // 1

const reg = new FinalizationRegistry(held => { /* cleanup */ });
reg.register(target, "token");
```

### 11.3.6 Regular expressions: RegExp

`RegExp` is implemented by a self-contained backtracking regex engine ([`native_RegExp.c`](../../../lang/js/native/builtin/RegExp/native_RegExp.c)). It supports construction via `new RegExp(pattern, flags)`, providing `test`/`exec`/`toString` and the `source`/`flags`/`global`/`ignoreCase`/`multiline`/`sticky`/`lastIndex` properties.

- **flags**: `g` `i` `m` `s` `y` (`u` is accepted but ignored — strings are already handled as UTF-8 bytes).
- **Syntax**: literal characters, `.`, `\d\D\w\W\s\S`, `\b\B`, `\n\t\r\f\v\0\xHH\uHHHH`, character classes (ranges/negation), quantifiers `* + ? {n} {n,} {n,m}` (greedy + lazy), capture groups, non-capturing groups `(?:)`, lookaheads `(?=)`/`(?!)`, `|`, `^ $`, back-references `\1`–`\9`.
- `String.prototype`'s `match`/`replace`/`split`/`search` accept both strings and `RegExp` objects; `replace` supports callback functions.

```javascript
const re = new RegExp("h(a+)lo", "i");
re.test("HAAALO");                        // true
re.exec("xhaaaloY");                      // ["haaalo", "aaa", index:1, input:...]
re.toString();                            // "/h(a+)lo/i"

"xaaay".match(new RegExp("a+", "g"));     // ["aaa"]
"a1b2".replace(new RegExp("[0-9]", "g"), s => "[" + s + "]");  // "a[1]b[2]"
```

> ⚠️ **Regex literals `/.../` are not supported** (the lexer has no corresponding token), and **lookbehinds `(?<=)` / `(?<!)` are not supported**. Use `new RegExp("...")` instead.

### 11.3.7 Symbol

`Symbol` provides unique values and well-known symbols (such as `Symbol.iterator`, `Symbol.toStringTag`), usable as object keys.

```javascript
const s1 = Symbol("id"), s2 = Symbol("id");
s1 === s2;                                // false (unique)
const o = { [Symbol.iterator]() { /* ... */ } };
```

### 11.3.8 Others: Object / Array / String / Number / JSON / Date / Math / Error / globalThis

- **Object**: `assign/is/keys/values/entries/freeze/seal/preventExtensions/hasOwn/setPrototypeOf/getPrototypeOf/defineProperty/...`
- **Array**: `from/of/isArray`; prototype methods `find/findIndex/includes/flat/flatMap/at/splice/reduce/reduceRight/...`
- **String**: `startsWith/endsWith/includes/repeat/padStart/padEnd/trimStart/trimEnd/replaceAll/at/charAt/charCodeAt/...`
- **Number**: `isInteger/isNaN/isFinite/parseInt/parseFloat`, `MAX_VALUE/MIN_VALUE/MAX_SAFE_INTEGER`, `toFixed/toPrecision/...`
- **Math**: `floor/ceil/round/atan2/fround/random/...`
- **JSON**: `stringify` (compact output)/`parse`, round-trip consistent
- **Date**: construction, all field accessors, `toISOString/toUTCString/toString`, `Date.now/getTime/valueOf/parse/UTC`
- **Error**: `Error` and its subtypes `TypeError/RangeError/SyntaxError/...`, `AggregateError`
- **Globals**: `globalThis`, `isNaN/parseInt/parseFloat`, `Infinity`, `NaN`

---

## 11.4 Known limitations

| Item | Status | Description / workaround |
| --- | --- | --- |
| ES Modules (`import`/`export`) | ❌ Unsupported | single-file script engine; for cross-file use Chapter 10's `include`/`.mbc` mechanism |
| Regex literals `/.../` | ❌ Unsupported | use `new RegExp("pattern", "flags")` |
| Regex lookbehinds `(?<=)` / `(?<!)` | ❌ Unsupported | lookaheads `(?=)`/`(?!)` work |
| `Boolean` wrapper class | ❌ Not registered | boolean primitives work fully; there's just no `Boolean()` constructor |
| `Atomics.wait` | ⚠️ Throws | no blocking wait in a single-threaded runtime; `notify` wakes 0 waiters |

Apart from the table above, all 38 feature areas in the section 11.1.3 matrix are implemented and pass tests.

---

## 11.5 How to verify it yourself

1. **Run the full suite**: `./build/mario test/js/es6_full.js`, which should output `854 passed, 0 failed`.
2. **Run individual examples**: save any code snippet from this chapter as a `.js` file and run it with `./build/mario your.js`.
3. **Inspect the bytecode**: to understand what some ES6 syntax compiles into, combine Chapter 2's dump option with Chapter 3's instruction table — observe, for example, the `INSTR_TAG_RAW` generated by tagged templates.

---

## 11.6 Summary

- Mario's JS frontend has implemented the **complete ES6 feature set**, covering the common additions up to ES2022; the **854 standard assertions in `test/js/es6_full.js` all pass**.
- Support lands in two layers: the **compiler** (syntax: `let/const`, template literals, arrow functions, destructuring, `class`, `for...of`, generators, `async/await`, `**`, etc.) and the **native built-in classes** (objects: `Map/Set`, `Promise`, `Proxy/Reflect`, `BigInt`, `TypedArray`, `RegExp`, `WeakRef`, etc.).
- The main boundaries are **no ES Modules, no regex literals, no `Boolean` wrapper class**; these can usually be worked around in embedded/single-file scripting scenarios.

At this point, from bytecode, lexing, compilation, execution, the object model, GC, natives, to this chapter's ES6+ language features, the entire Wiki has covered the full picture of Mario VM. Return to the [Wiki Home](README.md).
