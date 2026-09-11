// ES6 core language features test suite
// Covers: template literals, default & rest parameters, spread (array/call/object),
//         array & object destructuring, object literal shorthand/method/computed keys,
//         for...of (arrays and strings), string index access, concise arrow bodi
//         concatenation, and var hoisting (function-scoped, not block-scoped).

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
console.log("1. Template literals");
var who = "World";
var num = 3;
eq(`Hello, ${who}!`, "Hello, World!", "simple interpolation");
eq(`${num} + ${num} = ${num + num}`, "3 + 3 = 6", "expression interpolation");
eq(`nested ${ `inner ${1 + 1}` } end`, "nested inner 2 end", "nested template");
eq(`multi
line`, "multi\nline", "multi-line template");
eq(`esc ${ "\\t" } tab`, "esc \\t tab", "literal text preserved");

// ---------------------------------------------------------------------------
console.log("\n2. Default parameters");
function power(base, exp = 2) { return base ** exp; }
eq(power(5), 25, "default applied when arg omitted");
eq(power(5, 3), 125, "explicit arg overrides default");
function greet(name, greeting = "Hi") { return greeting + ", " + name; }
eq(greet("Bob"), "Hi, Bob", "default string param");
eq(greet("Bob", "Hey"), "Hey, Bob", "override string param");

// ---------------------------------------------------------------------------
console.log("\n3. Rest parameters");
function sumAll(...nums) {
    var t = 0;
    for (var i = 0; i < nums.length; i++) { t += nums[i]; }
    return t;
}
eq(sumAll(1, 2, 3, 4), 10, "rest collects all args");
eq(sumAll(), 0, "rest with no args is empty");
function headThen(first, ...rest) { return first + "|" + rest.join(","); }
eq(headThen(1, 2, 3), "1|2,3", "rest after a leading param");

// ---------------------------------------------------------------------------
console.log("\n4. Spread operator");
var base = [1, 2, 3];
var spread = [0, ...base, 4];
eq(spread.join(","), "0,1,2,3,4", "spread in array literal");
function add3(x, y, z) { return x + y + z; }
var args = [1, 2, 3];
eq(add3(...args), 6, "spread in call arguments");
var o1 = { a: 1, b: 2 };
var o2 = { ...o1, c: 3 };
eq(o2.a, 1, "object spread copies a");
eq(o2.b, 2, "object spread copies b");
eq(o2.c, 3, "object spread adds c");
var o3 = { ...o1, a: 99 };
eq(o3.a, 99, "later key overrides spread value");

// ---------------------------------------------------------------------------
console.log("\n5. Array destructuring");
var [p, q] = [10, 20];
eq(p, 10, "first element");
eq(q, 20, "second element");
var [h1, , h3] = [1, 2, 3];
eq(h1, 1, "hole: first");
eq(h3, 3, "hole: third");
var [head, ...tail] = [1, 2, 3, 4];
eq(head, 1, "destructuring rest: head");
eq(tail.join(","), "2,3,4", "destructuring rest: tail");

// ---------------------------------------------------------------------------
console.log("\n6. Object destructuring");
var point = { x: 5, y: 6 };
var { x, y } = point;
eq(x, 5, "shorthand x");
eq(y, 6, "shorthand y");
var { x: aliasX } = point;
eq(aliasX, 5, "rename binding");

// ---------------------------------------------------------------------------
console.log("\n7. Object literal shorthand / method / computed keys");
var vname = "val";
var keyName = "dyn";
var obj = {
    vname,
    keyName,
    [keyName + "amic"]: 42,
    speak() { return "spoken"; },
    n: 7
};
eq(obj.vname, "val", "property shorthand (vname)");
eq(obj.keyName, "dyn", "property shorthand (keyName)");
eq(obj.dynamic, 42, "computed key");
eq(obj.speak(), "spoken", "method shorthand");
eq(obj.n, 7, "normal key alongside shorthand");

// ---------------------------------------------------------------------------
console.log("\n8. for...of loops");
var total = 0;
for (var e of [1, 2, 3, 4]) { total += e; }
eq(total, 10, "iterate array literal");
var joined = "";
var items = ["a", "b", "c"];
for (var it of items) { joined += it; }
eq(joined, "abc", "iterate array variable");
var gridSum = 0;
var grid = [[1, 2], [3, 4]];
for (var row of grid) {
    for (var cell of row) { gridSum += cell; }
}
eq(gridSum, 10, "nested for...of");

// ---------------------------------------------------------------------------
console.log("\n9. Exponent operator");
eq(2 ** 10, 1024, "2 ** 10");
eq(2 ** 3 ** 2, 512, "right-associative 2 ** (3 ** 2)");
var bexp = 2;
bexp **= 3;
eq(bexp, 8, "**= assignment");
var bexp2 = 3;
bexp2 **= 3;
eq(bexp2, 27, "**= assignment (3**3)");

// ---------------------------------------------------------------------------
console.log("\n10. Number + string concatenation (regression guard)");
eq(1 + "abc", "1abc", "int on the left of +");
eq("abc" + 1, "abc1", "int on the right of +");
var nn = 5;
eq(nn + " items", "5 items", "variable int on the left");
eq(2 + 3 + "x", "5x", "arithmetic then concat");
eq(true + "!", "true!", "bool on the left of +");
var acc = "v=";
acc += 9;
eq(acc, "v=9", "string += number");

// ---------------------------------------------------------------------------
console.log("\n11. Concise arrow bodies (=> expr)");
var dbl = (x) => x * 2;
eq(dbl(4), 8, "parenthesized param, expression body");
var inc = x => x + 1;
eq(inc(4), 5, "bare param, expression body");
var add2 = (a, b) => a + b;
eq(add2(3, 5), 8, "two params, expression body");
var answer = () => 42;
eq(answer(), 42, "no params, expression body");
var hi = n => "Hi " + n;
eq(hi("Bo"), "Hi Bo", "body is a string concatenation");
var tmpl = n => `n=${n}`;
eq(tmpl(2), "n=2", "body is a template literal");
var sign = x => x < 0 ? "neg" : "pos";
eq(sign(-1), "neg", "body is a ternary (negative)");
eq(sign(1), "pos", "body is a ternary (positive)");
var gt4 = x => x > 4;
ok(gt4(5), "body returns boolean true");
ok(!gt4(2), "body returns boolean false");
var acc11 = 0;
[1, 2, 3].forEach(v => acc11 = acc11 + v);
eq(acc11, 6, "concise arrow with assignment body as forEach callback");
function applyTo(f, vals) { var out = []; for (var v of vals) { out.push(f(v)); } return out; }
eq(applyTo(x => x * 2, [1, 2, 3]).join(","), "2,4,6", "concise arrow passed to a higher-order function");
var blockArrow = x => { return x * 3; };
eq(blockArrow(4), 12, "block body still works alongside concise form");

// ---------------------------------------------------------------------------
console.log("\n12. String for...of and index access");
var chars = "";
for (var ch of "hello") { chars += ch; }
eq(chars, "hello", "for...of over a string literal");
var rev = "";
for (var c of "abc") { rev = c + rev; }
eq(rev, "cba", "reverse a string via for...of");
var charCount = 0;
var word = "test";
for (var w of word) { charCount++; }
eq(charCount, 4, "for...of over a string variable counts chars");
var sname = "Mario";
eq(sname[0], "M", "string index access: first char");
eq(sname[4], "o", "string index access: last char");
eq(sname.length(), 5, "string length()");
ok(typeof sname[99] === "undefined", "out-of-range string index is undefined");

// ---------------------------------------------------------------------------
console.log("\n13. Array map / filter / reduce (with arrow callbacks)");
var nums = [1, 2, 3, 4];
var doubled = nums.map(x => x * 2);
eq(doubled.join(","), "2,4,6,8", "map with concise arrow");
eq(doubled.length(), 4, "map preserves length");
eq(nums.map(function (x) { return x + 1; }).join(","), "2,3,4,5", "map with block function callback");
var big = nums.filter(x => x > 2);
eq(big.join(","), "3,4", "filter keeps matching elements");
eq(nums.filter(x => x > 100).length(), 0, "filter with no matches is empty");
eq(nums.reduce((a, b) => a + b, 0), 10, "reduce sum with initial value");
eq(nums.reduce((a, b) => a * b), 24, "reduce product without initial value");
eq(["a", "b", "c"].reduce((a, b) => a + b, ""), "abc", "reduce string concat with initial");
eq(nums.filter(x => x > 1).map(x => x * 10).join(","), "20,30,40", "filter().map() chain");
eq(["a", "b"].map((v, i) => i + ":" + v).join(","), "0:a,1:b", "map callback receives index");
eq([10, 20].reduce((a, v, i) => a + v + i, 0), 31, "reduce callback receives index");

// ---------------------------------------------------------------------------
console.log("\n14. var hoisting (function-scoped, not block-scoped)");
// A `var` declared inside a block/loop must survive that block (hoisted to the
// enclosing function or global scope), matching ES5 semantics. The values below
// use computed expressions so these assertions test scoping, not assignment.
if (true) { var blockVar = 5; }
eq(blockVar, 5, "var in an if-block survives the block");
for (var li = 0; li < 3; li++) { var lastSquare = li * li; }
eq(lastSquare, 4, "var in a for-body survives the loop");
eq(li, 3, "for-loop counter (var) survives the loop");
function hoistInFunc() {
    var a = 1;
    if (a) { var b = 2; }
    for (var k = 0; k < 3; k++) { var c = k * 10; }
    return a + b + c;
}
eq(hoistInFunc(), 23, "vars in nested blocks/loops hoist to the function scope (1+2+20)");
function whileHoist() {
    var w = 0;
    var seen = 0;
    while (w < 3) { seen = w; w = w + 1; }
    return seen;
}
eq(whileHoist(), 2, "var assigned in a while-body keeps its last value");
function sharedBinding() {
    var count = 0;
    for (var z = 0; z < 3; z++) { var marker = z * z; count = count + 1; }
    return count + ":" + marker;
}
eq(sharedBinding(), "3:4", "loop-body var is a single function-scoped binding");
var redecl = 1;
var redecl = 2;
eq(redecl, 2, "var can be redeclared in the same scope");

// ---------------------------------------------------------------------------
console.log("\n15. Operators build a new value instead of mutating a shared one");
// `var b = a` leaves both bindings referencing ONE var_t, and integer literals are
// shared through the VM's literal cache, so an in-place ++/--/+= used to corrupt
// every alias of the value - which is also what made nested for...of lose whole
// iterations (the cached `0` index literal was stepped in place to the size).
var ax = 1;
var ay = ax;
ax++;
eq(ay, 1, "alias keeps its value after x++");
eq(ax, 2, "x++ steps the binding itself");
var bm = 1;
var bn = bm;
bm += 1;
eq(bn, 1, "alias keeps its value after x += 1");
eq(bm, 2, "x += 1 steps the binding itself");
var dm = 5;
var dn = dm;
dm--;
eq(dn, 5, "alias keeps its value after x--");
var pm = 5;
var pn = pm;
++pm;
eq(pn, 5, "alias keeps its value after ++x");
var st = 10;
eq(st++, 10, "postfix ++ yields the old value");
eq(st, 11, "postfix ++ still steps the binding");
eq(++st, 12, "prefix ++ yields the new value");
eq(st--, 12, "postfix -- yields the old value");
eq(--st, 10, "prefix -- yields the new value");
var ca = 2;
eq(ca += 3, 5, "+= yields the new value");
eq(ca -= 1, 4, "-= yields the new value");
eq(ca *= 3, 12, "*= yields the new value");
eq(ca /= 4, 3, "/= yields the new value");
eq(ca %= 2, 1, "%= yields the new value");
var sg = "ab";
var sh = sg;
sg += "cd";
eq(sh, "ab", "string += does not corrupt the aliased literal");
eq(sg, "abcd", "string += appends");
var om = { n: 1 };
var omAlias = om.n;
om.n++;
eq(om.n, 2, "object member ++ steps the member");
eq(omAlias, 1, "object member ++ leaves the earlier read alone");
var ae = [1, 2, 3];
ae[1]++;
eq(ae[1], 3, "array element ++ steps the element");
eq(ae[0], 1, "array element ++ leaves its neighbours alone");
function countUp() {
    var t = 0;
    for (var q = 0; q < 3; q++) { t = t + 1; }
    return t;
}
eq(countUp(), 3, "cached loop literal is pristine on call 1");
eq(countUp(), 3, "cached loop literal is pristine on call 2");
eq(countUp(), 3, "cached loop literal is pristine on call 3");
// a float operand must not be written into an int-typed value buffer
var fi = 1;
fi += 0.5;
eq(fi, 1.5, "int += float promotes to a float");
// string + float used to read the string's byte buffer as a 4-byte int
eq("v=" + 1.5, "v=1.500000", "string + float concatenates");
eq(1.5 + 2.25, 3.75, "float + float adds");

// ---------------------------------------------------------------------------
console.log("\n16. Nested loops keep independent counters");
function nestedOf(a) {
    var n = 0;
    for (var i of a) { for (var j of a) { n = n + 1; } }
    return n;
}
eq(nestedOf([1, 2, 3]), 9, "nested for...of 3x3 runs 9 times");
eq(nestedOf([1, 2, 3]), 9, "nested for...of 3x3 runs 9 times again");
eq(nestedOf([1, 2, 3, 4]), 16, "nested for...of 4x4 runs 16 times");
function tripleOf(a) {
    var n = 0;
    for (var i of a) {
        for (var j of a) {
            for (var k of a) { n = n + 1; }
        }
    }
    return n;
}
eq(tripleOf([1, 2, 3]), 27, "triple for...of 3x3x3 runs 27 times");
eq(tripleOf([1, 2, 3]), 27, "triple for...of runs 27 times again");
function nestedOfLet(a) {
    let n = 0;
    for (let i of a) { for (let j of a) { n++; } }
    return n;
}
eq(nestedOfLet([1, 2, 3]), 9, "nested for...of with let and n++");
function nestedFor(a) {
    var n = 0;
    for (var i = 0; i < a.length; i++) {
        for (var j = 0; j < a.length; j++) { n++; }
    }
    return n;
}
eq(nestedFor([1, 2, 3]), 9, "nested classic for 3x3");
// for...in over an array is a separate pre-existing gap: the elements live under
// the hidden _ARRAY_ member, so keys() does not enumerate them.
function nestedIn(o) {
    var n = 0;
    for (var i in o) { for (var j in o) { n++; } }
    return n;
}
eq(nestedIn({ a: 1, b: 2, c: 3 }), 9, "nested for...in over an object 3x3");
function strOf() {
    var out = "";
    for (var ch of "ab") { out = out + ch; }
    return out;
}
eq(strOf(), "ab", "for...of over a string");
function whileUp() {
    var i = 0;
    var sum = 0;
    while (i < 4) { sum += i; i++; }
    return sum;
}
eq(whileUp(), 6, "while with += and ++");
eq(whileUp(), 6, "while with += and ++ again");

// ---------------------------------------------------------------------------
console.log("\n17. A function returned out of its defining scope keeps it alive");
// The closure env still owns the node that holds the returned function, and that
// function's func_t owns the env, so tearing the pair down walks the same var
// twice; each call below must still get its own captured binding.
function counterFactory() {
    let count = 0;
    function bump() { count++; return count; }
    return bump;
}
var cf1 = counterFactory();
var cf2 = counterFactory();
eq(cf1(), 1, "closure counter starts at 1");
eq(cf1(), 2, "closure counter keeps stepping");
eq(cf2(), 1, "a second closure has its own captured binding");
eq(cf1(), 3, "the first closure is unaffected by the second");
function capturedParam(seed) {
    let label = "n";
    function next() { seed = seed + 1; return label + seed; }
    return next;
}
var cpf = capturedParam(10);
eq(cpf(), "n11", "closure captures the parameter");
eq(cpf(), "n12", "captured parameter keeps stepping");

// ---------------------------------------------------------------------------
console.log("");
console.log("=== es6.js: " + __pass + " passed, " + __fail + " failed ===");
if (__fail === 0) { console.log("ALL TESTS PASSED"); }
else { console.log("SOME TESTS FAILED"); }
