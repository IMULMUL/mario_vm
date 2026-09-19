// EventEmitter self-check (Node `events` core, exposed as a global class).
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) passed++;
    else { failed++; console.log("FAIL " + msg + ": got " + JSON.stringify(got) + " want " + JSON.stringify(want)); }
}
function ok(cond, msg) { eq(!!cond, true, msg); }

// --- presence ---------------------------------------------------------------
eq(typeof EventEmitter, "function", "EventEmitter is a constructor");
let ee = new EventEmitter();
ok(ee instanceof EventEmitter, "instance of EventEmitter");
eq(typeof ee.on, "function", "on present");
eq(typeof ee.emit, "function", "emit present");
eq(typeof ee.once, "function", "once present");
eq(typeof ee.off, "function", "off present");
eq(typeof ee.removeAllListeners, "function", "removeAllListeners present");

// --- basic on/emit + args + this --------------------------------------------
let hits = [];
ee.on("data", function (a, b) { hits.push(this.tag + ":" + a + ":" + b); });
ee.tag = "E";
let had = ee.emit("data", 1, 2);
eq(had, true, "emit returns true when a listener ran");
eq(hits.length, 1, "listener fired once");
eq(hits[0], "E:1:2", "listener got args and emitter as this");

// emit with no listeners returns false
eq(ee.emit("nobody"), false, "emit returns false with no listeners");

// --- multiple listeners fire in registration order --------------------------
let order = [];
let m = new EventEmitter();
m.on("x", () => order.push("a"));
m.on("x", () => order.push("b"));
m.emit("x");
eq(order.join(","), "a,b", "listeners fire in order");
eq(m.listenerCount("x"), 2, "listenerCount = 2");

// --- once fires a single time ------------------------------------------------
let n = 0;
let o = new EventEmitter();
o.once("tick", () => n++);
o.emit("tick"); o.emit("tick"); o.emit("tick");
eq(n, 1, "once listener fires exactly once");
eq(o.listenerCount("tick"), 0, "once listener removed after firing");

// --- off / removeListener ----------------------------------------------------
let c = 0;
function inc() { c++; }
let r = new EventEmitter();
r.on("e", inc);
r.emit("e"); eq(c, 1, "listener ran");
r.off("e", inc);
r.emit("e"); eq(c, 1, "off removed the listener");
eq(r.listenerCount("e"), 0, "listenerCount 0 after off");

// --- chaining (on/once/off return the emitter) -------------------------------
let ch = new EventEmitter();
eq(ch.on("a", () => {}), ch, "on returns this");
eq(ch.once("b", () => {}), ch, "once returns this");
eq(ch.setMaxListeners(20), ch, "setMaxListeners returns this");
eq(ch.getMaxListeners(), 20, "getMaxListeners reflects setMaxListeners");

// --- removeAllListeners ------------------------------------------------------
let ra = new EventEmitter();
ra.on("p", () => {}); ra.on("p", () => {}); ra.on("q", () => {});
ra.removeAllListeners("p");
eq(ra.listenerCount("p"), 0, "removeAllListeners(type) cleared p");
eq(ra.listenerCount("q"), 1, "removeAllListeners(type) left q");
ra.removeAllListeners();
eq(ra.eventNames().length, 0, "removeAllListeners() cleared everything");

// --- listeners / eventNames --------------------------------------------------
let ls = new EventEmitter();
function f1() {} function f2() {}
ls.on("z", f1); ls.on("z", f2);
let arr = ls.listeners("z");
eq(arr.length, 2, "listeners() returns both");
eq(arr[0] === f1, true, "listeners()[0] is f1");
eq(ls.eventNames().join(","), "z", "eventNames lists registered types");

// --- prependListener fires first --------------------------------------------
let seq = [];
let pl = new EventEmitter();
pl.on("k", () => seq.push("second"));
pl.prependListener("k", () => seq.push("first"));
pl.emit("k");
eq(seq.join(","), "first,second", "prependListener runs before existing");

// --- defaultMaxListeners + instance listenerCount ---------------------------
let sc = new EventEmitter();
sc.on("s", () => {});
eq(sc.listenerCount("s"), 1, "instance listenerCount works");
eq(EventEmitter.defaultMaxListeners, 10, "defaultMaxListeners is 10");

// --- emit forwarding many args ----------------------------------------------
let got = null;
let ma = new EventEmitter();
ma.on("multi", (a, b, c, d) => { got = [a, b, c, d].join("|"); });
ma.emit("multi", "w", "x", "y", "z");
eq(got, "w|x|y|z", "emit forwards all trailing args");

console.log(failed === 0 ? "__EVENTEMITTER_OK__" : ("__EVENTEMITTER_FAIL__ " + failed));
console.log("EventEmitter: " + passed + " passed, " + failed + " failed");
