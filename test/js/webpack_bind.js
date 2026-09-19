// Regression guard for the webpack runtime tail bind pattern:
//   (f = self.X = self.X || []).forEach(c.bind(null, 0)), f.push = c.bind(null, f.push.bind(f))
// A prior VM defect mis-resolved the `c` receiver in the nested member-write RHS
// to a null slot ("can not find function 'bind'"). This locks the fixed behaviour.
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) passed++;
    else { failed++; console.log("FAIL " + msg + ": got " + JSON.stringify(got) + " want " + JSON.stringify(want)); }
}
function ok(cond, msg) { eq(!!cond, true, msg); }

// --- the exact nested-bind assignment must not throw and must route correctly --
(function () {
    "use strict";
    var e, t, n, r, o, u, i, c, f, a = {}, l = {};
    var log = [];

    // c receives (boundOriginalPush, chunk) because bind(null, f.push.bind(f))
    // pins thisArg=null and prepends the bound original push as the leading arg.
    c = function (origPush, chunk) {
        log.push("c:" + (typeof origPush) + ":" + JSON.stringify(chunk));
        origPush(chunk);            // the bound Array.push actually appends to f
        return "ret:" + JSON.stringify(chunk);
    };

    var host = {};
    (f = host.webpackChunk = host.webpackChunk || []).forEach(c.bind(null, 0));

    var threw = false;
    try { f.push = c.bind(null, f.push.bind(f)); }
    catch (err) { threw = true; console.log("THREW: " + (err && err.message)); }
    eq(threw, false, "nested bind assignment does not throw (receiver `c` resolves)");
    eq(typeof f.push, "function", "f.push is a bound function after assignment");

    var rv = f.push(["chunkA"]);
    eq(rv, 'ret:["chunkA"]', "bound push returns c's value");
    eq(log.length, 1, "c invoked exactly once");
    eq(log[0], 'c:function:["chunkA"]', "c got the bound original push + chunk");
    eq(f.length, 1, "the bound original push actually appended to f");
    eq(JSON.stringify(f[0]), '["chunkA"]', "f[0] is the pushed chunk");
})();

// --- bind receiver resolution under a member write target (GETW + nested CALLO) -
(function () {
    var obj = { slot: null };
    var maker = { tag: "M", bind2: function (a, b) { return this.tag + ":" + a + ":" + b; } };
    // RHS is a method call on `maker` while LHS is a member write on `obj`.
    obj.slot = maker.bind2(1, 2);
    eq(obj.slot, "M:1:2", "method receiver on RHS of a member write resolves correctly");

    // Deeper: RHS call whose argument is itself a member call on a third object.
    var inner = { v: 7, get: function () { return this.v; } };
    var outer = { wrap: function (x) { return "w(" + x + ")"; } };
    obj.slot = outer.wrap(inner.get());
    eq(obj.slot, "w(7)", "nested member-call argument resolves its receiver");
})();

// --- Function.prototype.bind leading-arg prepending + this pinning -------------
(function () {
    function add(a, b, c) { return this.base + a + b + c; }
    var ctx = { base: 100 };
    var bound = add.bind(ctx, 1, 2);
    eq(bound(3), 106, "bind pins this and prepends leading args");
    eq(bound.length, 1, "bound function length reflects remaining params");

    // rebinding a bound function keeps the ORIGINAL this (a bound function's
    // thisArg is immutable) and concatenates leading args: [1,2] ++ [10] ++ [20].
    // add(a,b,c) uses only the first three -> this.base(100) + 1 + 2 + 10 = 113;
    // the trailing 20 is the 4th argument and is ignored.
    var rebound = bound.bind({ base: 999 }, 10);
    eq(rebound(20), 113, "rebind keeps original this, concatenates leading args"); // 100+1+2+10
    eq(rebound.length, 0, "rebound length = max(0, bound.length(1) - 1 prepended)");
})();

console.log(failed === 0 ? "__WEBPACK_BIND_OK__" : ("__WEBPACK_BIND_FAIL__ " + failed));
console.log("webpack bind regression: " + passed + " passed, " + failed + " failed");
