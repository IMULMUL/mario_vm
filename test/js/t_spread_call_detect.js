// Regression guard for call-argument spread detection.
//   callee("name", (e, t) => { let a = [...src]; ... })
// A call whose argument is an arrow/function whose BODY contains a spread
// (`[...x]`, `{...o}`, or a nested `f(...y)`) is NOT itself a spread call. The
// old scanner tracked only paren depth, so a `...` inside the body sat at paren
// depth 1 and mis-flagged the whole call, routing it down the *_SPREAD codegen
// path and dropping the callee (the function was never invoked). This locks the
// fixed behaviour: real top-level spread still works, nested spread does not
// leak, and a callee receiving a closure argument is actually called.
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) passed++;
    else { failed++; console.log("FAIL " + msg + ": got " + JSON.stringify(got) + " want " + JSON.stringify(want)); }
}

// --- a real top-level spread argument must still spread ----------------------
(function () {
    var seen = null;
    function f(a, b, c) { seen = [a, b, c]; return a + b + c; }
    var args = [1, 2, 3];
    eq(f(...args), 6, "top-level spread expands into positional args");
    eq(JSON.stringify(seen), "[1,2,3]", "spread delivered all three args");

    // mixed plain + spread
    eq(f(10, ...[20, 30]), 60, "mixed plain and spread args keep order");
})();

// --- a spread nested in an arrow body must NOT make the outer call a spread --
(function () {
    var calls = [];
    var obj = {
        make: function (name, cb) { calls.push(name); var A = function () {}; A.init = cb; return A; }
    };
    var src = [1, 2, 3];
    // The callback body contains an array spread; the call itself has none.
    var String_ = obj.make("$Type", (e, t) => { let a = [...src]; return a.length; });
    eq(calls.length, 1, "callee invoked exactly once (arrow-body array spread did not hijack the call)");
    eq(calls[0], "$Type", "callee received the correct first argument");
    eq(typeof String_.init, "function", "returned object carries the closure as .init");
    eq(String_.init(), 3, "the closure body runs and its spread produced the array");
})();

// --- object spread and nested-call spread inside a closure argument ----------
(function () {
    var hit = 0;
    function reg(nm, fn) { hit++; return fn; }
    var base = { x: 1 };
    var inner = function (...rest) { return rest.length; };
    // body has object spread {...base} and a nested spread call inner(...[1,2])
    var cb = reg("obj", (e) => { let o = { ...base, y: 2 }; return o.x + o.y + inner(...[1, 2]); });
    eq(hit, 1, "nested object/call spread inside the closure body did not divert the outer call");
    eq(cb(), 5, "closure body evaluated: 1 + 2 + 2 = 5");
})();

// --- a genuine spread arg that is an array literal containing a closure ------
(function () {
    function sum3(a, b, c) { return a() + b() + c(); }
    var fns = [...[() => 1, () => 2, () => 3]];
    eq(sum3(...fns), 6, "real spread of an array of closures still works");
})();

console.log(failed === 0 ? "__SPREAD_CALL_OK__" : ("__SPREAD_CALL_FAIL__ " + failed));
console.log("spread call regression: " + passed + " passed, " + failed + " failed");
