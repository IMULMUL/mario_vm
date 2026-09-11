// =============================================================================
// es6_full.js — A complete, standard ES6 (ES2015+) test suite.
//
// This is a *standard-spec* test suite: it exercises the full ES6 feature set
// as defined by ECMA-262 (plus the handful of later additions that are almost
// always lumped together with "ES6": async/await, object rest/spread, the
// exponent operator, and a few Array/String/Object/Number/Math statics).
//
// Every assertion below is written to hold true in a spec-compliant engine
// (modern V8 / Node / browser). Sections that a minimal engine does not
// implement are still included so that "nothing is missed".
//
// Feature coverage index:
//   1.  let / const (block scope, TDZ, redeclaration, immutability)
//   2.  Template literals (interpolation, nesting, multi-line, tagged)
//   3.  Default parameters
//   4.  Rest parameters
//   5.  Spread (array literal, call, object literal / rest)
//   6.  Destructuring (array, object, nested, defaults, swap, params)
//   7.  Object literal enhancements (shorthand, computed keys, methods,
//       getters/setters, __proto__)
//   8.  Arrow functions (lexical this, no [[Construct]], no arguments binding)
//   9.  Classes (constructor, methods, static, inheritance, super, accessors)
//   10. for...of / for...in and iteration protocol
//   11. Iterators & generators (function*, yield, yield*, Symbol.iterator)
//   12. Symbols (uniqueness, well-known symbols, symbol keys)
//   13. Collections (Map, Set, WeakMap, WeakSet)
//   14. Promise (states, then/catch/finally, all/allSettled/race, chaining)
//   15. async / await
//   16. Array statics & prototype methods (from, of, find, includes, ...)
//   17. String methods (startsWith, includes, repeat, padStart, ...)
//   18. Number / Math additions
//   19. Object statics (assign, is, keys, values, entries, freeze, ...)
//   20. Exponent operator, new.target, typeof/instanceof edge cases
//   21. Closures & the classic loop-capture (let vs var)
//   22. Tail-position / recursion & destructuring corner cases
// =============================================================================

// --- Runtime shim ------------------------------------------------------------
// Bare shells such as JavaScriptCore's `jsc` do not provide `console` (or
// `setTimeout`). Node and browsers already have them, so this only fills the
// gaps. It deliberately assigns through `globalThis` instead of declaring
// `var console`, which would otherwise shadow (and blank out) Node's console.
(function () {
    var g = (typeof globalThis !== "undefined") ? globalThis : this;
    if (typeof g.console === "undefined" || typeof g.console.log !== "function") {
        var printFn = (typeof print === "function") ? print : function () {};
        g.console = {
            log: function () {
                var parts = [];
                for (var i = 0; i < arguments.length; i++) {
                    parts.push(typeof arguments[i] === "string" ? arguments[i] : String(arguments[i]));
                }
                printFn(parts.join(" "));
            }
        };
    }
    if (typeof g.setTimeout !== "function") {
        // No-op timer: pending callbacks simply never fire, which is fine for
        // the Promise.race test (the synchronous promise always wins).
        g.setTimeout = function () { return 0; };
    }
})();
// -----------------------------------------------------------------------------

let __pass = 0;
let __fail = 0;

function ok(cond, msg) {
    if (cond) { __pass++; console.log("  \u2713 " + msg); }
    else { __fail++; console.log("  \u2717 " + msg); }
}

function eq(actual, expected, msg) {
    // Object.is-style: handles NaN === NaN and +0 !== -0 correctly.
    const same = Object.is(actual, expected);
    if (same) { __pass++; console.log("  \u2713 " + msg); }
    else { __fail++; console.log("  \u2717 " + msg + " (expected " + String(expected) + ", got " + String(actual) + ")"); }
}

function deepEq(actual, expected, msg) {
    const a = JSON.stringify(actual);
    const b = JSON.stringify(expected);
    if (a === b) { __pass++; console.log("  \u2713 " + msg); }
    else { __fail++; console.log("  \u2717 " + msg + " (expected " + b + ", got " + a + ")"); }
}

function throws(fn, msg) {
    let threw = false;
    try { fn(); } catch (e) { threw = true; }
    ok(threw, msg);
}

function noThrow(fn, msg) {
    let threw = false;
    try { fn(); } catch (e) { threw = true; }
    ok(!threw, msg);
}

function section(title) {
    console.log("\n" + title);
}

// =============================================================================
section("1. let / const");
// =============================================================================
(function () {
    let a = 1;
    const b = 2;
    a = 5;
    eq(a, 5, "let is reassignable");
    eq(b, 2, "const keeps its value");
    throws(() => { "use strict"; const c = 1; c = 2; }, "const reassignment throws");
})();

(function () {
    // Block scoping: inner `let` does not leak out.
    let outer = "outer";
    {
        let outer = "inner";
        eq(outer, "inner", "block-scoped let shadows outer");
    }
    eq(outer, "outer", "outer binding is unaffected by the block");
})();

(function () {
    // `var` is function-scoped and does leak out of a block.
    if (true) { var leaked = 42; }
    eq(leaked, 42, "var declared in a block leaks to function scope");
})();

(function () {
    // Temporal Dead Zone: accessing a let/const before initialization throws.
    function __tdzProbe() { const read = v; let v = 1; return read; }
    throws(() => __tdzProbe(), "TDZ: read before declaration throws");
})();

(function () {
    // let/const in a for loop create a fresh binding per iteration.
    const fns = [];
    for (let i = 0; i < 3; i++) { fns.push(() => i); }
    deepEq(fns.map(f => f()), [0, 1, 2], "for-let gives each closure its own i");

    const vfns = [];
    for (var j = 0; j < 3; j++) { vfns.push(() => j); }
    deepEq(vfns.map(f => f()), [3, 3, 3], "for-var shares one binding (all 3)");
})();

(function () {
    // const with a mutable object: the binding is fixed, contents are not.
    const obj = { n: 1 };
    obj.n = 2;
    eq(obj.n, 2, "const object contents are mutable");
    throws(() => { const arr = [1]; arr = [2]; }, "const binding cannot be reassigned");
})();

// =============================================================================
section("2. Template literals");
// =============================================================================
(function () {
    const who = "World";
    eq(`Hello, ${who}!`, "Hello, World!", "simple interpolation");
    eq(`${1 + 2} = ${3}`, "3 = 3", "expression interpolation");
    eq(`nested ${`inner ${1 + 1}`}`, "nested inner 2", "nested templates");
    eq(`line1
line2`, "line1\nline2", "multi-line template preserves newlines");
    eq(`a\tb`, "a\tb", "escape sequences are processed");
    const obj = { toString() { return "custom"; } };
    eq(`${obj}`, "custom", "interpolation invokes toString");
    eq(`${null}-${undefined}`, "null-undefined", "null/undefined interpolate as text");
})();

(function () {
    // Tagged template literals.
    function tag(strings, ...values) {
        let out = "";
        for (let i = 0; i < strings.length; i++) {
            out += strings[i];
            if (i < values.length) { out += "[" + values[i] + "]"; }
        }
        return out;
    }
    eq(tag`a${1}b${2}c`, "a[1]b[2]c", "tagged template collects strings and values");
    function raw(strings) { return strings.raw[0]; }
    eq(raw`a\nb`, "a\\nb", "String.raw via strings.raw");
})();

// =============================================================================
section("3. Default parameters");
// =============================================================================
(function () {
    function power(base, exp = 2) { return base ** exp; }
    eq(power(5), 25, "default applied when arg omitted");
    eq(power(5, 3), 125, "explicit arg overrides default");
    eq(power(5, undefined), 25, "undefined triggers the default");
    eq(power(5, null), 1, "null does NOT trigger default (5 ** 0 === 1)");

    function greet(name, greeting = "Hi") { return `${greeting}, ${name}`; }
    eq(greet("Bob"), "Hi, Bob", "default string param");
    eq(greet("Bob", "Hey"), "Hey, Bob", "override default string param");

    // Later defaults can reference earlier parameters.
    function f(a, b = a * 2) { return b; }
    eq(f(3), 6, "default references a previous parameter");

    // Defaults are evaluated at call time, not definition time.
    let counter = 0;
    function bump(v = ++counter) { return v; }
    eq(bump(), 1, "default expression evaluated at call time (1st)");
    eq(bump(), 2, "default expression evaluated at call time (2nd)");
})();

// =============================================================================
section("4. Rest parameters");
// =============================================================================
(function () {
    function sum(...nums) { return nums.reduce((a, b) => a + b, 0); }
    eq(sum(1, 2, 3, 4), 10, "rest collects all args");
    eq(sum(), 0, "rest with no args is empty");

    function headThen(first, ...rest) { return first + "|" + rest.join(","); }
    eq(headThen(1, 2, 3), "1|2,3", "rest after a leading param");

    // `arguments` is not the same as rest; rest is a real Array.
    function typeCheck(...args) { return Array.isArray(args); }
    ok(typeCheck(1, 2), "rest parameter is a real Array");

    throws(() => { eval("function bad(...a, ...b){}"); }, "only one rest param allowed (SyntaxError)");
})();

// =============================================================================
section("5. Spread & object rest/spread");
// =============================================================================
(function () {
    // Array spread
    const base = [1, 2, 3];
    eq([0, ...base, 4].join(","), "0,1,2,3,4", "spread in array literal");

    // Call spread
    function add3(x, y, z) { return x + y + z; }
    eq(add3(...[1, 2, 3]), 6, "spread in call arguments");

    // Spread as a shallow copy
    const copy = [...base];
    deepEq(copy, [1, 2, 3], "spread copies an array");
    ok(copy !== base, "spread produces a new array object");

    // String spread
    deepEq([..."abc"], ["a", "b", "c"], "spread a string into chars");

    // Object spread
    const o1 = { a: 1, b: 2 };
    const o2 = { ...o1, c: 3 };
    deepEq(o2, { a: 1, b: 2, c: 3 }, "object spread copies and adds");
    const o3 = { ...o1, a: 99 };
    eq(o3.a, 99, "later key overrides spread value");

    // Object rest in destructuring
    const { a, ...restObj } = { a: 1, b: 2, c: 3 };
    eq(a, 1, "object rest: extracted a");
    deepEq(restObj, { b: 2, c: 3 }, "object rest collects the remainder");
})();

// =============================================================================
section("6. Destructuring");
// =============================================================================
(function () {
    // Array destructuring
    const [p, q] = [10, 20];
    eq(p, 10, "array destructure first");
    eq(q, 20, "array destructure second");

    const [h1, , h3] = [1, 2, 3];
    eq(h1, 1, "array destructure with a hole: first");
    eq(h3, 3, "array destructure with a hole: third");

    const [head, ...tail] = [1, 2, 3, 4];
    eq(head, 1, "array destructure rest: head");
    deepEq(tail, [2, 3, 4], "array destructure rest: tail");

    const [d = 5] = [];
    eq(d, 5, "array destructure default when undefined");

    // Swap via destructuring
    let x = 1, y = 2;
    [x, y] = [y, x];
    eq(x, 2, "swap: x");
    eq(y, 1, "swap: y");

    // Object destructuring
    const point = { px: 5, py: 6 };
    const { px, py } = point;
    eq(px, 5, "object destructure shorthand px");
    eq(py, 6, "object destructure shorthand py");

    const { px: aliasX } = point;
    eq(aliasX, 5, "object destructure rename");

    const { missing = "def" } = {};
    eq(missing, "def", "object destructure default");

    // Nested destructuring
    const data = { user: { name: "Ann", tags: ["a", "b"] } };
    const { user: { name, tags: [firstTag] } } = data;
    eq(name, "Ann", "nested object destructure name");
    eq(firstTag, "a", "nested array-in-object destructure");

    // Destructuring in function parameters
    function dist({ a = 0, b = 0 } = {}) { return a + b; }
    eq(dist({ a: 3, b: 4 }), 7, "object destructuring in params");
    eq(dist(), 0, "param destructuring with default object");
    function arrDist([m, n]) { return m * n; }
    eq(arrDist([3, 4]), 12, "array destructuring in params");
})();

// =============================================================================
section("7. Object literal enhancements");
// =============================================================================
(function () {
    // Property shorthand
    const vname = "val", keyName = "dyn";
    const obj = { vname, keyName };
    eq(obj.vname, "val", "property shorthand (vname)");
    eq(obj.keyName, "dyn", "property shorthand (keyName)");

    // Computed property names
    const suffix = "amic";
    const computed = { ["dyn" + suffix]: 42, ["k" + (1 + 1)]: "two" };
    eq(computed.dynamic, 42, "computed key (concatenation)");
    eq(computed.k2, "two", "computed key (expression)");

    // Method shorthand
    const m = { speak() { return "spoken"; } };
    eq(m.speak(), "spoken", "method shorthand");

    // Getters and setters
    const g = {
        _v: 1,
        get v() { return this._v; },
        set v(nv) { this._v = nv * 10; }
    };
    eq(g.v, 1, "getter returns backing field");
    g.v = 5;
    eq(g.v, 50, "setter transforms the assigned value");

    // __proto__ in an object literal
    const proto = { hi() { return "hi"; } };
    const child = { __proto__: proto };
    eq(child.hi(), "hi", "__proto__ sets the prototype in a literal");
})();

// =============================================================================
section("8. Arrow functions");
// =============================================================================
(function () {
    const dbl = x => x * 2;
    eq(dbl(4), 8, "single param, concise body");
    const add2 = (a, b) => a + b;
    eq(add2(3, 5), 8, "two params, concise body");
    const answer = () => 42;
    eq(answer(), 42, "no params, concise body");
    const objRet = () => ({ k: 1 });
    deepEq(objRet(), { k: 1 }, "concise body returning an object literal");
    const block = x => { return x * 3; };
    eq(block(4), 12, "block body with explicit return");

    // Lexical `this`
    function Counter() {
        this.n = 0;
        this.bump = () => this.n++;
    }
    const c = new Counter();
    c.bump(); c.bump();
    eq(c.n, 2, "arrow captures lexical this");

    // Arrows have no prototype and are not constructible
    ok(!Object.prototype.hasOwnProperty.call(dbl, "prototype"), "arrow has no prototype property");
    throws(() => new dbl(), "arrow cannot be used with new");

    // Arrows have no own `arguments`
    const noArgs = () => typeof arguments;
    function wrapper() { return noArgs(); }
    eq(wrapper(), "object", "arrow uses the enclosing function's arguments");
})();

// =============================================================================
section("9. Classes");
// =============================================================================
(function () {
    class Animal {
        constructor(name) { this.name = name; }
        speak() { return this.name + " makes a noise"; }
        static create(name) { return new Animal(name); }
    }
    const a = new Animal("Rex");
    eq(a.speak(), "Rex makes a noise", "instance method");
    eq(Animal.create("Bob").name, "Bob", "static method");
    ok(typeof Animal.prototype.speak === "function", "method lives on the prototype");

    // Inheritance and super
    class Dog extends Animal {
        constructor(name, breed) {
            super(name);
            this.breed = breed;
        }
        speak() { return super.speak() + " (woof)"; }
    }
    const d = new Dog("Fido", "lab");
    eq(d.name, "Fido", "super constructor sets inherited field");
    eq(d.breed, "lab", "subclass constructor sets own field");
    eq(d.speak(), "Fido makes a noise (woof)", "super.method() call");
    ok(d instanceof Dog, "instanceof subclass");
    ok(d instanceof Animal, "instanceof superclass");

    // Accessors on a class
    class Box {
        constructor(w) { this._w = w; }
        get w() { return this._w; }
        set w(v) { this._w = v + 1; }
        static get label() { return "box"; }
    }
    const bx = new Box(3);
    eq(bx.w, 3, "class getter");
    bx.w = 10;
    eq(bx.w, 11, "class setter transforms value");
    eq(Box.label, "box", "static getter");

    // Class expression
    const Point = class { constructor(x) { this.x = x; } };
    eq(new Point(7).x, 7, "class expression");

    // Subclassing a built-in
    class MyArray extends Array {
        get first() { return this[0]; }
    }
    const ma = new MyArray();
    ma.push(9, 8);
    eq(ma.first, 9, "subclassing Array works");
    eq(ma.length, 2, "Array subclass keeps length semantics");

    // Methods are non-enumerable
    const keys = [];
    for (const k in d) { keys.push(k); }
    ok(!keys.includes("speak"), "class methods are not enumerable via for..in");
})();


// =============================================================================
section("10. for...of / for...in / iteration protocol");
// =============================================================================
(function () {
    // for...of over an array
    let total = 0;
    for (const e of [1, 2, 3, 4]) { total += e; }
    eq(total, 10, "for...of over array literal");

    // for...of over a string
    let chars = "";
    for (const ch of "abc") { chars += ch; }
    eq(chars, "abc", "for...of iterates string code units");

    // for...of over a Map (entries)
    const mp = new Map([["a", 1], ["b", 2]]);
    const pairs = [];
    for (const [k, v] of mp) { pairs.push(k + v); }
    deepEq(pairs, ["a1", "b2"], "for...of over Map yields [key,value]");

    // for...of with break/continue
    let sum = 0;
    for (const n of [1, 2, 3, 4, 5]) {
        if (n === 2) continue;
        if (n === 5) break;
        sum += n;
    }
    eq(sum, 8, "for...of honors break and continue (1+3+4)");

    // for...in enumerates keys (strings)
    const keys = [];
    for (const k in { x: 1, y: 2 }) { keys.push(k); }
    deepEq(keys, ["x", "y"], "for...in yields own enumerable keys");

    const idx = [];
    for (const i in [10, 20, 30]) { idx.push(i); }
    deepEq(idx, ["0", "1", "2"], "for...in over array yields string indices");

    // Manual iteration protocol
    function collect(iterable) {
        const out = [];
        const it = iterable[Symbol.iterator]();
        let step = it.next();
        while (!step.done) { out.push(step.value); step = it.next(); }
        return out;
    }
    deepEq(collect([7, 8, 9]), [7, 8, 9], "Symbol.iterator protocol drives for...of");

    // A custom iterable object
    const range = {
        from: 1, to: 3,
        [Symbol.iterator]() {
            let cur = this.from, last = this.to;
            return {
                next() {
                    return cur <= last ? { value: cur++, done: false }
                                       : { value: undefined, done: true };
                }
            };
        }
    };
    deepEq([...range], [1, 2, 3], "custom iterable works with spread");
})();

// =============================================================================
section("11. Generators");
// =============================================================================
(function () {
    function* counter(n) {
        for (let i = 0; i < n; i++) { yield i; }
    }
    const g = counter(3);
    deepEq(g.next(), { value: 0, done: false }, "generator first next()");
    deepEq(g.next(), { value: 1, done: false }, "generator second next()");
    deepEq([...counter(3)], [0, 1, 2], "generator is iterable");

    // Two-way communication with yield
    function* dialog() {
        const a = yield "q1";
        const b = yield "q2";
        return a + "|" + b;
    }
    const d = dialog();
    eq(d.next().value, "q1", "generator yields first question");
    eq(d.next("A").value, "q2", "generator receives value and yields again");
    eq(d.next("B").value, "A|B", "generator return value");

    // yield* delegation
    function* inner() { yield 1; yield 2; }
    function* outerGen() { yield 0; yield* inner(); yield 3; }
    deepEq([...outerGen()], [0, 1, 2, 3], "yield* delegates to another generator");

    // Generator methods on a class-like object
    const obj = {
        *[Symbol.iterator]() { yield "a"; yield "b"; }
    };
    deepEq([...obj], ["a", "b"], "generator method via computed key");

    // return() and throw()
    function* infinite() { let i = 0; while (true) { yield i++; } }
    const inf = infinite();
    inf.next(); inf.next();
    eq(inf.return("done").value, "done", "generator.return() ends iteration");

    function* thrower() { try { yield 1; } catch (e) { yield "caught:" + e; } }
    const t = thrower();
    t.next();
    eq(t.throw("boom").value, "caught:boom", "generator.throw() is catchable inside");
})();

// =============================================================================
section("12. Symbols");
// =============================================================================
(function () {
    const s1 = Symbol("id");
    const s2 = Symbol("id");
    ok(s1 !== s2, "two symbols with the same description differ");
    eq(s1.description, "id", "symbol description");
    eq(String(s1), "Symbol(id)", "String(symbol) format");
    eq(typeof s1, "symbol", "typeof a symbol");

    // Symbol as a unique object key (not enumerated by for..in / Object.keys)
    const key = Symbol("k");
    const o = { [key]: "secret", visible: 1 };
    eq(o[key], "secret", "symbol used as an object key");
    ok(!Object.keys(o).includes(String(key)), "symbol keys are hidden from Object.keys");
    deepEq(Object.keys(o), ["visible"], "only string keys enumerated");
    ok(Object.getOwnPropertySymbols(o).length === 1, "getOwnPropertySymbols finds it");

    // Well-known symbols exist
    ok(typeof Symbol.iterator === "symbol", "Symbol.iterator is a symbol");
    ok(typeof Symbol.toStringTag === "symbol", "Symbol.toStringTag is a symbol");

    // Global symbol registry
    eq(Symbol.for("app"), Symbol.for("app"), "Symbol.for returns the same registered symbol");
    eq(Symbol.keyFor(Symbol.for("app")), "app", "Symbol.keyFor reads back the key");

    // Symbol.toPrimitive
    const money = {
        [Symbol.toPrimitive](hint) {
            return hint === "number" ? 100 : "$100";
        }
    };
    eq(+money, 100, "Symbol.toPrimitive number hint");
    eq(`${money}`, "$100", "Symbol.toPrimitive string hint");
})();

// =============================================================================
section("13. Collections: Map / Set / WeakMap / WeakSet");
// =============================================================================
(function () {
    // --- Map ---
    const m = new Map();
    m.set("a", 1).set("b", 2);
    eq(m.size, 2, "Map size");
    eq(m.get("a"), 1, "Map get");
    eq(m.has("b"), true, "Map has present");
    eq(m.has("z"), false, "Map has absent");
    eq(m.get("z"), undefined, "Map get absent is undefined");
    m.set("a", 100);
    eq(m.get("a"), 100, "Map set overwrites");
    eq(m.size, 2, "Map size unchanged on overwrite");
    eq(m.delete("a"), true, "Map delete present returns true");
    eq(m.delete("a"), false, "Map delete absent returns false");
    m.clear();
    eq(m.size, 0, "Map clear");

    // Any value type as key (including objects & NaN)
    const ok1 = {}, ok2 = {};
    const mk = new Map([[ok1, "one"], [NaN, "nan"]]);
    eq(mk.get(ok1), "one", "Map uses object identity for keys");
    eq(mk.get(ok2), undefined, "different object is a different key");
    eq(mk.get(NaN), "nan", "Map treats NaN keys as equal (SameValueZero)");

    // Construction from iterable, keys/values/entries, forEach
    const m2 = new Map([["x", 10], ["y", 20]]);
    deepEq([...m2.keys()], ["x", "y"], "Map.keys()");
    deepEq([...m2.values()], [10, 20], "Map.values()");
    deepEq([...m2.entries()], [["x", 10], ["y", 20]], "Map.entries()");
    let acc = "";
    m2.forEach((v, k) => { acc += k + "=" + v + ";"; });
    eq(acc, "x=10;y=20;", "Map.forEach visits (value, key) in order");

    // --- Set ---
    const s = new Set([1, 2, 2, 3, 3, 3]);
    eq(s.size, 3, "Set deduplicates on construction");
    ok(s.has(2), "Set has present");
    ok(!s.has(9), "Set has absent");
    s.add(4).add(4);
    eq(s.size, 4, "Set add deduplicates");
    eq(s.delete(1), true, "Set delete present returns true");
    deepEq([...s], [2, 3, 4], "Set preserves insertion order");
    ok(new Set([NaN, NaN]).size === 1, "Set treats NaN as equal");

    // --- WeakMap / WeakSet ---
    const wm = new WeakMap();
    let wkey = {};
    wm.set(wkey, "wv");
    eq(wm.get(wkey), "wv", "WeakMap stores against an object key");
    eq(wm.has(wkey), true, "WeakMap has");
    throws(() => new WeakMap().set("primitive", 1), "WeakMap rejects non-object keys");

    const ws = new WeakSet();
    let so = {};
    ws.add(so);
    ok(ws.has(so), "WeakSet has added object");
    throws(() => new WeakSet().add(1), "WeakSet rejects primitives");
})();

// =============================================================================
section("14. Promise");
// =============================================================================
const __promiseTests = (function () {
    // Resolution
    let r1;
    Promise.resolve(5).then(v => { r1 = v; });
    return Promise.resolve(5).then(v => {
        eq(v, 5, "Promise.resolve delivers its value");
        // Chaining returns a new promise
        return Promise.resolve(1).then(a => a + 1).then(b => b * 3);
    }).then(result => {
        eq(result, 6, "promise chaining computes (1+1)*3");
        // Rejection & catch
        return Promise.reject(new Error("boom")).catch(e => "caught:" + e.message);
    }).then(msg => {
        eq(msg, "caught:boom", "catch handles a rejection");
        // finally runs for both states
        let fin = 0;
        return Promise.resolve(1).finally(() => { fin++; }).then(() => fin);
    }).then(finCount => {
        eq(finCount, 1, "finally runs on resolution");
        // Promise.all
        return Promise.all([Promise.resolve(1), Promise.resolve(2), Promise.resolve(3)]);
    }).then(all => {
        deepEq(all, [1, 2, 3], "Promise.all resolves all values in order");
        // Promise.all rejects fast
        return Promise.all([Promise.resolve(1), Promise.reject("x")]).catch(e => "all-rejected:" + e);
    }).then(msg => {
        eq(msg, "all-rejected:x", "Promise.all rejects if any rejects");
        // Promise.race
        return Promise.race([
            new Promise(res => setTimeout(() => res("slow"), 50)),
            Promise.resolve("fast")
        ]);
    }).then(raceWinner => {
        eq(raceWinner, "fast", "Promise.race settles with the first");
        // Promise.allSettled
        return Promise.allSettled([Promise.resolve(1), Promise.reject("e")]);
    }).then(settled => {
        eq(settled[0].status, "fulfilled", "allSettled fulfilled entry");
        eq(settled[0].value, 1, "allSettled fulfilled value");
        eq(settled[1].status, "rejected", "allSettled rejected entry");
        eq(settled[1].reason, "e", "allSettled rejected reason");
        // new Promise executor runs synchronously
        let executed = false;
        new Promise((resolve) => { executed = true; resolve(); });
        ok(executed, "executor body runs immediately");
    });
})();

// =============================================================================
section("15. async / await");
// =============================================================================
(async function () {
    await __promiseTests;
    async function retFive() { return 5; }
    ok(retFive() instanceof Promise, "async function returns a Promise");
    eq(await retFive(), 5, "await unwraps the returned promise");

    eq(await Promise.resolve(7) * 2, 14, "await a resolved promise");
    eq(await 9, 9, "await a non-promise passes it through");

    // Sequential awaits
    async function multi() {
        const a = await Promise.resolve(1);
        const b = await Promise.resolve(2);
        return a + b;
    }
    eq(await multi(), 3, "sequential awaits");

    // Awaiting another async function
    async function inner() { return 4; }
    async function outer() { return (await inner()) * 10; }
    eq(await outer(), 40, "await a nested async function");

    // try/catch around await on a rejection
    async function mayFail() { throw new Error("async-boom"); }
    let caught = "";
    try { await mayFail(); } catch (e) { caught = e.message; }
    eq(caught, "async-boom", "async throw is catchable with await");

    // async arrow function
    const arrowAsync = async () => (await Promise.resolve(6)) + 1;
    eq(await arrowAsync(), 7, "async arrow function");

    // async object method
    const obj = { base: 100, async fetch() { return this.base + (await Promise.resolve(1)); } };
    eq(await obj.fetch(), 101, "async method with this");

    // for-await-of over async iterables
    async function* agen() { yield 1; yield 2; yield 3; }
    const collected = [];
    for await (const v of agen()) { collected.push(v); }
    deepEq(collected, [1, 2, 3], "for-await-of over an async generator");

    // Fall-through resolves to undefined
    async function noReturn() { const x = 1; }
    eq(await noReturn(), undefined, "async fall-through resolves undefined");
})().then(() => {
    // =========================================================================
    section("16. Array statics & prototype methods");
    // =========================================================================
    (function () {
        // Array.from
        deepEq(Array.from("abc"), ["a", "b", "c"], "Array.from a string");
        deepEq(Array.from([1, 2, 3], x => x * 2), [2, 4, 6], "Array.from with mapFn");
        deepEq(Array.from({ length: 3 }, (_, i) => i), [0, 1, 2], "Array.from array-like");
        deepEq(Array.from(new Set([1, 1, 2])), [1, 2], "Array.from a Set");

        // Array.of
        deepEq(Array.of(1, 2, 3), [1, 2, 3], "Array.of");
        deepEq(Array.of(7), [7], "Array.of single number (not length)");

        const arr = [1, 2, 3, 4, 5];
        // find / findIndex
        eq(arr.find(x => x > 3), 4, "find first match");
        eq(arr.findIndex(x => x > 3), 3, "findIndex first match");
        eq(arr.find(x => x > 99), undefined, "find no match is undefined");
        eq(arr.findIndex(x => x > 99), -1, "findIndex no match is -1");

        // includes
        ok(arr.includes(3), "includes present");
        ok(!arr.includes(9), "includes absent");
        ok([NaN].includes(NaN), "includes finds NaN");

        // fill / copyWithin
        deepEq([1, 2, 3, 4].fill(0, 1, 3), [1, 0, 0, 4], "fill range");
        deepEq([1, 2, 3, 4, 5].copyWithin(0, 3), [4, 5, 3, 4, 5], "copyWithin");

        // entries / keys / values
        deepEq([...["a", "b"].entries()], [[0, "a"], [1, "b"]], "Array.entries");
        deepEq([...["a", "b"].keys()], [0, 1], "Array.keys");
        deepEq([...["a", "b"].values()], ["a", "b"], "Array.values");

        // flat / flatMap
        deepEq([1, [2, [3, [4]]]].flat(Infinity), [1, 2, 3, 4], "flat(Infinity)");
        deepEq([1, 2, 3].flatMap(x => [x, x * 2]), [1, 2, 2, 4, 3, 6], "flatMap");

        // classic ES5 methods still present
        eq([1, 2, 3].map(x => x * 2).join(","), "2,4,6", "map");
        eq([1, 2, 3].filter(x => x > 1).join(","), "2,3", "filter");
        eq([1, 2, 3].reduce((a, b) => a + b, 0), 6, "reduce");
        ok([1, 2, 3].some(x => x > 2), "some");
        ok([1, 2, 3].every(x => x > 0), "every");
        eq([3, 1, 2].sort().join(","), "1,2,3", "sort");
        eq([1, 2, 3].indexOf(2), 1, "indexOf");
        eq([1, 2, 3].lastIndexOf(3), 2, "lastIndexOf");
        deepEq([1, 2, 3].slice(1), [2, 3], "slice");
    })();

    // =========================================================================
    section("17. String methods");
    // =========================================================================
    (function () {
        const s = "Hello, World";
        ok(s.startsWith("Hello"), "startsWith");
        ok(!s.startsWith("World"), "startsWith negative");
        ok(s.endsWith("World"), "endsWith");
        ok(s.includes("o, W"), "includes substring");
        eq("ab".repeat(3), "ababab", "repeat");
        eq("5".padStart(3, "0"), "005", "padStart");
        eq("5".padEnd(3, "."), "5..", "padEnd");
        eq("abc".padStart(2), "abc", "padStart no-op when longer");
        eq("  trim  ".trim(), "trim", "trim");
        eq("\u0061\u030A".normalize().length, 1, "normalize combines marks");

        // Unicode helpers
        eq(String.fromCodePoint(0x1F600).length, 2, "fromCodePoint creates an astral char");
        eq("\u{1F600}", "\uD83D\uDE00", "unicode code point escape");
        eq("abc".codePointAt(0), 97, "codePointAt");

        // Destructuring a string iterates code points
        deepEq([..."a\u{1F600}b"], ["a", "\u{1F600}", "b"], "spread string by code point");
    })();

    // =========================================================================
    section("18. Number & Math additions");
    // =========================================================================
    (function () {
        ok(Number.isInteger(5), "Number.isInteger(5)");
        ok(!Number.isInteger(5.5), "Number.isInteger(5.5) is false");
        ok(Number.isFinite(10), "Number.isFinite(10)");
        ok(!Number.isFinite(Infinity), "Number.isFinite(Infinity) is false");
        ok(Number.isNaN(NaN), "Number.isNaN(NaN)");
        ok(!Number.isNaN("x"), "Number.isNaN does not coerce");
        ok(Number.isSafeInteger(2 ** 53 - 1), "Number.isSafeInteger max");
        eq(Number.parseInt("42px"), 42, "Number.parseInt");
        eq(Number.parseFloat("3.14abc"), 3.14, "Number.parseFloat");
        eq(Number.EPSILON > 0, true, "Number.EPSILON is positive");
        eq(Number.MAX_SAFE_INTEGER, 9007199254740991, "Number.MAX_SAFE_INTEGER");

        eq(Math.trunc(5.9), 5, "Math.trunc");
        eq(Math.trunc(-5.9), -5, "Math.trunc negative");
        eq(Math.sign(-3), -1, "Math.sign");
        eq(Math.cbrt(27), 3, "Math.cbrt");
        eq(Math.hypot(3, 4), 5, "Math.hypot");
        eq(Math.log2(8), 3, "Math.log2");
        eq(Math.log10(1000), 3, "Math.log10");
        ok(Math.imul(3, 4) === 12, "Math.imul");
        eq(Math.clz32(1), 31, "Math.clz32");
    })();

    // =========================================================================
    section("19. Object static methods");
    // =========================================================================
    (function () {
        const target = { a: 1 };
        Object.assign(target, { b: 2 }, { c: 3 });
        deepEq(target, { a: 1, b: 2, c: 3 }, "Object.assign merges sources");

        ok(Object.is(NaN, NaN), "Object.is(NaN, NaN)");
        ok(!Object.is(0, -0), "Object.is(0, -0) is false");

        const o = { x: 1, y: 2 };
        deepEq(Object.keys(o), ["x", "y"], "Object.keys");
        deepEq(Object.values(o), [1, 2], "Object.values");
        deepEq(Object.entries(o), [["x", 1], ["y", 2]], "Object.entries");
        deepEq(Object.fromEntries([["p", 1], ["q", 2]]), { p: 1, q: 2 }, "Object.fromEntries");

        const frozen = Object.freeze({ n: 1 });
        throws(() => { "use strict"; frozen.n = 2; }, "Object.freeze prevents mutation");
        ok(Object.isFrozen(frozen), "Object.isFrozen");

        const src = { a: 1, b: { c: 2 } };
        const clone = Object.assign({}, src);
        ok(clone !== src, "Object.assign creates a new object");
        ok(clone.b === src.b, "Object.assign is a shallow copy");

        eq(Object.getOwnPropertyNames(o).length, 2, "getOwnPropertyNames");
        ok(Object.getOwnPropertyDescriptor(target, "a").value === 1, "getOwnPropertyDescriptor");
    })();

    // =========================================================================
    section("20. Operators: exponent, new.target, typeof/instanceof");
    // =========================================================================
    (function () {
        eq(2 ** 10, 1024, "exponent operator");
        eq(2 ** 3 ** 2, 512, "exponent is right-associative");
        let b = 2; b **= 3;
        eq(b, 8, "**= compound assignment");
        eq((-2) ** 2, 4, "exponent with a negative base");

        // new.target
        let sawTarget = "unset";
        function Ctor() { sawTarget = new.target; }
        new Ctor();
        ok(sawTarget === Ctor, "new.target is the constructor under new");
        sawTarget = "unset";
        Ctor();
        eq(sawTarget, undefined, "new.target is undefined on a plain call");

        // typeof / instanceof edge cases
        eq(typeof undefined, "undefined", "typeof undefined");
        eq(typeof null, "object", "typeof null is object (historic quirk)");
        eq(typeof Symbol(), "symbol", "typeof symbol");
        eq(typeof (() => {}), "function", "typeof arrow function");
        ok([] instanceof Array, "array instanceof Array");
        ok({} instanceof Object, "object instanceof Object");
        ok(!(1 instanceof Number), "primitive is not instanceof Number");
    })();

    // =========================================================================
    section("21. Closures & loop capture");
    // =========================================================================
    (function () {
        function makeCounter() {
            let count = 0;
            return {
                inc() { return ++count; },
                get() { return count; }
            };
        }
        const c1 = makeCounter(), c2 = makeCounter();
        c1.inc(); c1.inc(); c2.inc();
        eq(c1.get(), 2, "closure c1 has its own state");
        eq(c2.get(), 1, "closure c2 is independent");

        // Capturing loop variables with let vs var
        const letFns = [], varFns = [];
        for (let i = 0; i < 3; i++) { letFns.push(() => i); }
        for (var k = 0; k < 3; k++) { varFns.push(() => k); }
        deepEq(letFns.map(f => f()), [0, 1, 2], "let loop capture is per-iteration");
        deepEq(varFns.map(f => f()), [3, 3, 3], "var loop capture is shared");

        // IIFE capture workaround for var
        const iifeFns = [];
        for (var m = 0; m < 3; m++) {
            iifeFns.push((n => () => n)(m));
        }
        deepEq(iifeFns.map(f => f()), [0, 1, 2], "IIFE recaptures var per iteration");
    })();

    // =========================================================================
    section("22. Corner cases & interop");
    // =========================================================================
    (function () {
        // Default + destructuring + rest combined
        function mixed({ a = 1, b } = {}, ...rest) {
            return [a, b, rest.length];
        }
        deepEq(mixed({ b: 2 }, 9, 8), [1, 2, 2], "default+destructure+rest combine");

        // Computed method names in a class-like object
        const name = "greet";
        const o = { [name]() { return "hi"; }, ["a" + "b"]: 1 };
        eq(o.greet(), "hi", "computed method name");
        eq(o.ab, 1, "computed property name");

        // Optional chaining & nullish coalescing (ES2020, commonly grouped with ES6)
        const nested = { a: { b: { c: 1 } } };
        eq(nested?.a?.b?.c, 1, "optional chaining reads a deep value");
        eq(nested?.x?.y, undefined, "optional chaining short-circuits to undefined");
        eq(null ?? "fallback", "fallback", "nullish coalescing on null");
        eq(0 ?? "fallback", 0, "nullish coalescing keeps 0");

        // Logical assignment (ES2021)
        let la = 0; la ||= 5; eq(la, 5, "||= assigns when falsy");
        let lb = 3; lb &&= 7; eq(lb, 7, "&&= assigns when truthy");
        let lc = null; lc ??= 9; eq(lc, 9, "??= assigns when nullish");

        // Numeric separators (ES2021)
        eq(1_000_000, 1000000, "numeric separators in integers");

        // Destructuring an array returned by a function
        function pair() { return [1, 2]; }
        const [pa, pb] = pair();
        eq(pa + pb, 3, "destructure a function's array result");
    })();

    // =========================================================================
    console.log("\n=== es6_full.js: " + __pass + " passed, " + __fail + " failed ===");
    if (__fail === 0) { console.log("ALL TESTS PASSED"); }
    else { console.log("SOME TESTS FAILED"); }
}).catch(e => {
    console.log("\nFATAL: an unexpected error aborted the suite: " + (e && e.stack ? e.stack : e));
    throw e;
});
