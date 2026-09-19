// Combined Node/Web global smoke: verifies the additive builtins coexist.
let fails = 0;
function chk(c, m){ if(!c){ fails++; console.log("SMOKE FAIL: "+m); } }

// Node core
chk(typeof process === "object" && Array.isArray(process.argv), "process.argv");
chk(typeof process.nextTick === "function", "process.nextTick");
chk((typeof Buffer === "function" || typeof Buffer === "object") && Buffer.isBuffer(Buffer.from("hi")), "Buffer");
chk(Buffer.from("6869", "hex").toString() === "hi", "Buffer hex roundtrip");
chk(typeof setTimeout === "function" && typeof queueMicrotask === "function", "timers");

// Web platform
chk(typeof URL === "function" && new URL("http://a/b?c=1").pathname === "/b", "URL");
chk(typeof URLSearchParams === "function", "URLSearchParams");
chk(typeof EventTarget === "function" && typeof Event === "function", "EventTarget/Event");
chk(typeof AbortController === "function" && new AbortController().signal instanceof AbortSignal, "AbortController");
chk(typeof structuredClone === "function", "structuredClone");
chk(typeof crypto === "object" && typeof crypto.randomUUID === "function", "crypto");
chk(typeof performance === "object" && typeof performance.now() === "number", "performance");

// P2 additions: setImmediate / EventEmitter / WritableStream
chk(typeof setImmediate === "function" && typeof clearImmediate === "function", "setImmediate/clearImmediate");
chk(typeof EventEmitter === "function", "EventEmitter");
let _ee = new EventEmitter();
let _eeHit = 0;
_ee.on("ping", function (v) { _eeHit += v; });
_ee.emit("ping", 2); _ee.emit("ping", 3);
chk(_eeHit === 5, "EventEmitter on/emit forwards args");
let _onceN = 0;
_ee.once("o", function () { _onceN++; }); _ee.emit("o"); _ee.emit("o");
chk(_onceN === 1, "EventEmitter once fires a single time");
chk(typeof WritableStream === "function", "WritableStream");
let _wrote = [];
let _ws = new WritableStream({ write(c) { _wrote.push(c); } });
let _wr = _ws.getWriter();
_wr.write("x"); _wr.write("y");
chk(_wrote.join("") === "xy", "WritableStream getWriter/write");

// cross-feature: abort -> event -> structuredClone of a payload
let ac = new AbortController();
let got = null;
ac.signal.addEventListener("abort", function(e){ got = e.type; });
ac.abort();
chk(got === "abort", "abort dispatched event");
let clone = structuredClone({ list: [1,2,{deep:true}], when: new Date(5000) });
chk(clone.list[2].deep === true && clone.when.getTime() === 5000, "structuredClone deep+Date");

// async event loop drain
let order = [];
setTimeout(function(){ order.push("t"); }, 0);
queueMicrotask(function(){ order.push("m"); });
setImmediate(function(){ order.push("i"); });
process.nextTick(function(){ order.push("n"); });
Promise.resolve().then(function(){
  order.push("p");
});
setTimeout(function(){
  chk(order.length >= 3, "event loop drained tasks: " + JSON.stringify(order));
  console.log(fails === 0 ? "__SMOKE_OK__" : ("__SMOKE_FAIL__ " + fails));
  console.log("node/web smoke: " + (fails === 0 ? "all passed" : (fails + " failed")));
}, 5);
