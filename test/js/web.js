// Web/Node globals self-check: structuredClone / crypto / performance.
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) { passed++; }
    else { failed++; console.log("FAIL " + msg + ": got " + JSON.stringify(got) + " want " + JSON.stringify(want)); }
}
function ok(cond, msg) { eq(!!cond, true, msg); }

// --- presence ---------------------------------------------------------------
eq(typeof structuredClone, "function", "structuredClone is a function");
eq(typeof crypto, "object", "crypto is an object");
eq(typeof performance, "object", "performance is an object");
eq(typeof crypto.getRandomValues, "function", "crypto.getRandomValues present");
eq(typeof crypto.randomUUID, "function", "crypto.randomUUID present");
eq(typeof performance.now, "function", "performance.now present");

// --- structuredClone: primitives -------------------------------------------
eq(structuredClone(42), 42, "clone number");
eq(structuredClone("hi"), "hi", "clone string");
eq(structuredClone(true), true, "clone bool");
eq(structuredClone(null), null, "clone null");
eq(structuredClone(undefined), undefined, "clone undefined");

// --- structuredClone: array / object deep copy + independence --------------
let a = [1, 2, [3, 4]];
let ac = structuredClone(a);
eq(JSON.stringify(ac), JSON.stringify(a), "clone array deep-equal");
ok(ac !== a, "clone array is a distinct object");
ok(ac[2] !== a[2], "clone nested array is distinct");
ac[2][0] = 99;
eq(a[2][0], 3, "mutating clone does not touch source");

let o = { x: 1, nested: { y: [1, 2] }, s: "str" };
let oc = structuredClone(o);
eq(oc.x, 1, "clone object field");
eq(oc.s, "str", "clone object string field");
ok(oc.nested !== o.nested, "clone object nested distinct");
eq(JSON.stringify(oc.nested.y), "[1,2]", "clone nested array preserved");

// --- structuredClone: cycles -----------------------------------------------
let cyc = { name: "root" };
cyc.self = cyc;
let cc = structuredClone(cyc);
eq(cc.name, "root", "cyclic clone field");
ok(cc.self === cc, "cyclic reference points at the clone");
ok(cc !== cyc, "cyclic clone is distinct from source");

// shared reference preserved (same source object cloned once)
let shared = { v: 1 };
let pair = { a: shared, b: shared };
let pc = structuredClone(pair);
ok(pc.a === pc.b, "shared reference identity preserved in clone");

// --- structuredClone: Date --------------------------------------------------
let d = new Date(1234567890);
let dc = structuredClone(d);
ok(dc instanceof Date, "clone Date instanceof Date");
eq(dc.getTime(), d.getTime(), "clone Date preserves time");
ok(dc !== d, "clone Date is distinct");

// --- structuredClone: RegExp ------------------------------------------------
let re = /ab+c/gi;
let rec = structuredClone(re);
eq(rec.source, "ab+c", "clone RegExp source");
eq(rec.flags, "gi", "clone RegExp flags");
eq(rec.test("xxABBCxx"), true, "cloned RegExp still matches");

// --- structuredClone: Map / Set --------------------------------------------
let m = new Map();
m.set("k", 1);
m.set("n", { deep: true });
let mc = structuredClone(m);
ok(mc instanceof Map, "clone Map instanceof Map");
eq(mc.size, 2, "clone Map size");
eq(mc.get("k"), 1, "clone Map primitive value");
ok(mc.get("n") !== m.get("n"), "clone Map deep value distinct");
eq(mc.get("n").deep, true, "clone Map deep value preserved");

let s = new Set([1, 2, 3]);
let sc = structuredClone(s);
ok(sc instanceof Set, "clone Set instanceof Set");
eq(sc.size, 3, "clone Set size");
eq(sc.has(2), true, "clone Set membership");

// --- structuredClone: rejects functions / symbols --------------------------
let threwFn = false;
try { structuredClone(function () {}); } catch (e) { threwFn = (e.name === "DataCloneError"); }
eq(threwFn, true, "structuredClone(function) throws DataCloneError");

// --- crypto.getRandomValues -------------------------------------------------
let buf = [0, 0, 0, 0, 0, 0, 0, 0];
let out = crypto.getRandomValues(buf);
eq(out === buf, true, "getRandomValues returns the same array");
eq(buf.length, 8, "getRandomValues keeps length");
let allInRange = true, anyNonZero = false;
for (let i = 0; i < buf.length; i++) {
    if (typeof buf[i] !== "number" || buf[i] < 0 || buf[i] > 255) allInRange = false;
    if (buf[i] !== 0) anyNonZero = true;
}
ok(allInRange, "getRandomValues bytes are 0..255");
ok(anyNonZero, "getRandomValues produced non-zero entropy");

// --- crypto.randomUUID ------------------------------------------------------
let uuid = crypto.randomUUID();
eq(typeof uuid, "string", "randomUUID returns a string");
eq(uuid.length, 36, "randomUUID length 36");
let uuidRe = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
ok(uuidRe.test(uuid), "randomUUID is a v4 UUID: " + uuid);
ok(crypto.randomUUID() !== uuid, "randomUUID yields distinct values");

// --- performance.now --------------------------------------------------------
eq(typeof performance.timeOrigin, "number", "performance.timeOrigin is a number");
ok(performance.timeOrigin > 1000000000000, "timeOrigin looks like epoch ms");
let t0 = performance.now();
let spin = 0; for (let i = 0; i < 200000; i++) spin += i;
let t1 = performance.now();
eq(typeof t0, "number", "performance.now returns a number");
ok(t1 >= t0, "performance.now is monotonic non-decreasing");
eq(typeof performance.mark, "function", "performance.mark stub present");
performance.mark("x"); performance.measure("m", "a", "b"); performance.clearMarks();

console.log(failed === 0 ? "__WEB_OK__" : ("__WEB_FAIL__ " + failed));
console.log("Web globals: " + passed + " passed, " + failed + " failed");
