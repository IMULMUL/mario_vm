// ES6 async/await test suite (synchronous implementation).
// In this VM async functions run synchronously and return a real Promise, so
// .then() callbacks fire immediately and captured values can be asserted inline.

var __pass = 0;
var __fail = 0;

function ok(cond, msg) {
    if (cond) { __pass++; console.log("  ✓ " + msg); }
    else { __fail++; console.log("  ✗ " + msg); }
}

function eq(actual, expected, msg) {
    if (actual === expected) { __pass++; console.log("  ✓ " + msg); }
    else { __fail++; console.log("  ✗ " + msg + " (expected " + expected + ", got " + actual + ")"); }
}

// ---------------------------------------------------------------------------
console.log("1. async functions return a Promise");
async function retFive() { return 5; }
eq(typeof retFive(), "object", "async call yields a Promise object");
var r1;
retFive().then(function (v) { r1 = v; });
eq(r1, 5, "explicit return value delivered to then");

// a plain (non-async) function is NOT wrapped
function plain() { return 3; }
eq(typeof plain(), "number", "plain function returns a number, not a Promise");

// ---------------------------------------------------------------------------
console.log("\n2. await unwraps promises");
async function awaitResolve() {
    var v = await Promise.resolve(7);
    return v * 2;
}
var r2;
awaitResolve().then(function (v) { r2 = v; });
eq(r2, 14, "await Promise.resolve then compute");

async function awaitNonPromise() {
    var v = await 9;   // awaiting a non-promise passes it through
    return v + 1;
}
var r3;
awaitNonPromise().then(function (v) { r3 = v; });
eq(r3, 10, "await on a non-promise passes through");

// ---------------------------------------------------------------------------
console.log("\n3. multiple sequential awaits");
async function multiAwait() {
    var a = await Promise.resolve(1);
    var b = await Promise.resolve(2);
    var c = await Promise.resolve(3);
    return a + b + c;
}
var r4;
multiAwait().then(function (v) { r4 = v; });
eq(r4, 6, "three sequential awaits sum correctly");

// ---------------------------------------------------------------------------
console.log("\n4. awaiting another async function");
async function inner() { return 4; }
async function outer() {
    var v = await inner();
    return v * 10;
}
var r5;
outer().then(function (v) { r5 = v; });
eq(r5, 40, "await nested async function result");

// ---------------------------------------------------------------------------
console.log("\n5. fall-through and bare return resolve to undefined");
async function noReturn() { var x = 1; }
var r6 = "sentinel";
noReturn().then(function (v) { r6 = v; });
eq(typeof r6, "undefined", "async fall-through resolves undefined");

async function bareReturn() { return; }
var r7 = "sentinel";
bareReturn().then(function (v) { r7 = v; });
eq(typeof r7, "undefined", "bare return resolves undefined");

// ---------------------------------------------------------------------------
console.log("\n6. async method and async arrow");
var obj = {
    base: 100,
    async fetch() {
        var v = await Promise.resolve(1);
        return this.base + v;
    }
};
var r8;
obj.fetch().then(function (v) { r8 = v; });
eq(r8, 101, "async object method with await and this");

var arrowAsync = async () => {
    var v = await Promise.resolve(6);
    return v + 1;
};
var r9;
arrowAsync().then(function (v) { r9 = v; });
eq(r9, 7, "async arrow function (block body)");

// ---------------------------------------------------------------------------
console.log("\n7. nested plain function inside async stays synchronous");
async function withNested() {
    function helper() { return 3; }   // plain function, not wrapped
    var h = helper();
    return h + await Promise.resolve(1);
}
var r10;
withNested().then(function (v) { r10 = v; });
eq(r10, 4, "plain nested function returns a plain value");

// ---------------------------------------------------------------------------
console.log("\n8. chaining .then on an async result");
async function chainStart() { return 2; }
var r11;
chainStart().then(function (v) { return v * 5; }).then(function (v) { r11 = v; });
eq(r11, 10, "two-link then chain on an async result");

// ---------------------------------------------------------------------------
console.log("");
console.log("=== es6_async.js: " + __pass + " passed, " + __fail + " failed ===");
if (__fail === 0) { console.log("ALL TESTS PASSED"); }
else { console.log("SOME TESTS FAILED"); }
