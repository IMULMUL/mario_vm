// ES6 collections test suite: Map, Set, Symbol

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
console.log("1. Map - basic operations");
var m = new Map();
m.set("a", 1);
m.set("b", 2);
eq(m.size, 2, "size after two sets");
eq(m.get("a"), 1, "get existing key");
eq(m.get("b"), 2, "get second key");
eq(m.has("a"), true, "has existing key");
eq(m.has("zzz"), false, "has missing key");
eq(typeof m.get("zzz"), "undefined", "get missing key is undefined");

// overwrite existing key
m.set("a", 100);
eq(m.get("a"), 100, "set overwrites existing key");
eq(m.size, 2, "size unchanged after overwrite");

// ---------------------------------------------------------------------------
console.log("\n2. Map - numeric keys and delete/clear");
var mn = new Map();
mn.set(1, "one");
mn.set(2, "two");
eq(mn.get(1), "one", "numeric key lookup");
eq(mn.delete(1), true, "delete existing returns true");
eq(mn.delete(1), false, "delete missing returns false");
eq(mn.size, 1, "size after delete");
mn.clear();
eq(mn.size, 0, "size after clear");

// ---------------------------------------------------------------------------
console.log("\n3. Map - keys / values / entries");
var mk = new Map();
mk.set("x", 10);
mk.set("y", 20);
eq(mk.keys().join(","), "x,y", "keys() returns array of keys");
eq(mk.values().join(","), "10,20", "values() returns array of values");
var ents = mk.entries();
eq(ents.length, 2, "entries() length");
eq(ents[0][0], "x", "entries()[0] key");
eq(ents[0][1], 10, "entries()[0] value");
eq(ents[1][0], "y", "entries()[1] key");
eq(ents[1][1], 20, "entries()[1] value");

// ---------------------------------------------------------------------------
console.log("\n4. Map - forEach and constructor from iterable");
var acc = "";
mk.forEach(function (value, key) { acc += key + "=" + value + ";"; });
eq(acc, "x=10;y=20;", "forEach visits (value, key) in order");
var m2 = new Map([["p", 1], ["q", 2]]);
eq(m2.size, 2, "constructor from iterable: size");
eq(m2.get("p"), 1, "constructor from iterable: get p");
eq(m2.get("q"), 2, "constructor from iterable: get q");

// ---------------------------------------------------------------------------
console.log("\n5. Set - basic operations and dedup");
var s = new Set();
s.add(1);
s.add(2);
s.add(2);
s.add(3);
eq(s.size, 3, "duplicate value collapsed");
eq(s.has(2), true, "has present value");
eq(s.has(9), false, "has absent value");
eq(s.delete(1), true, "delete present returns true");
eq(s.delete(1), false, "delete absent returns false");
eq(s.size, 2, "size after delete");
eq(s.values().join(","), "2,3", "values() returns remaining items");

// ---------------------------------------------------------------------------
console.log("\n6. Set - forEach, clear and constructor from iterable");
var sacc = 0;
var s2 = new Set([5, 6, 7]);
s2.forEach(function (v) { sacc += v; });
eq(sacc, 18, "forEach visits each value");
eq(s2.size, 3, "constructor from iterable: size");
s2.clear();
eq(s2.size, 0, "size after clear");
var s3 = new Set([1, 1, 2, 2, 3]);
eq(s3.size, 3, "constructor from iterable dedups");

// ---------------------------------------------------------------------------
console.log("\n7. Symbol");
var sym = Symbol("id");
eq(sym.description, "id", "description property");
eq(sym.toString(), "Symbol(id)", "toString format");
eq(Symbol("a") === Symbol("a"), false, "symbols are unique");
eq(sym === sym, true, "same symbol is identical to itself");
var holder = {};
holder[sym] = "secret";
eq(holder[sym], "secret", "symbol usable as object key");
eq(typeof Symbol(), "object", "Symbol() returns an object (simplified)");

// ---------------------------------------------------------------------------
console.log("");
console.log("=== es6_collections.js: " + __pass + " passed, " + __fail + " failed ===");
if (__fail === 0) { console.log("ALL TESTS PASSED"); }
else { console.log("SOME TESTS FAILED"); }
