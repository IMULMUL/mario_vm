// EventTarget / Event / CustomEvent self-check (Node-compatible subset).
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) { passed++; }
    else { failed++; console.log("FAIL " + msg + ": got " + JSON.stringify(got) + " want " + JSON.stringify(want)); }
}
function ok(cond, msg) { eq(!!cond, true, msg); }

// --- Event basics -----------------------------------------------------------
let e = new Event("click");
eq(e.type, "click", "event.type");
eq(e.bubbles, false, "event.bubbles default");
eq(e.cancelable, false, "event.cancelable default");
eq(e.composed, false, "event.composed default");
eq(e.defaultPrevented, false, "event.defaultPrevented default");
eq(e.eventPhase, 0, "event.eventPhase NONE before dispatch");
eq(e.target, null, "event.target null before dispatch");
eq(e.currentTarget, null, "event.currentTarget null before dispatch");
eq(e.isTrusted, false, "event.isTrusted");
ok(typeof e.timeStamp === "number", "event.timeStamp is number");
eq(Object.prototype.toString.call(e), "[object Event]", "event toStringTag");

let e2 = new Event("x", { bubbles: true, cancelable: true, composed: true });
eq(e2.bubbles, true, "init bubbles");
eq(e2.cancelable, true, "init cancelable");
eq(e2.composed, true, "init composed");

// constants on prototype + constructor
eq(Event.NONE, 0, "Event.NONE");
eq(Event.CAPTURING_PHASE, 1, "Event.CAPTURING_PHASE");
eq(Event.AT_TARGET, 2, "Event.AT_TARGET");
eq(Event.BUBBLING_PHASE, 3, "Event.BUBBLING_PHASE");
eq(e2.AT_TARGET, 2, "instance sees AT_TARGET");

// preventDefault only when cancelable
e.preventDefault();
eq(e.defaultPrevented, false, "preventDefault no-op when not cancelable");
e2.preventDefault();
eq(e2.defaultPrevented, true, "preventDefault sets flag when cancelable");
eq(e2.returnValue, false, "preventDefault clears returnValue");

eq(Array.isArray(e.composedPath()), true, "composedPath returns array");

// --- EventTarget add/remove/dispatch ---------------------------------------
let t = new EventTarget();
let log = [];
function h1(ev) { log.push("h1:" + ev.type); }
function h2(ev) { log.push("h2:" + ev.type); }
t.addEventListener("ping", h1);
t.addEventListener("ping", h2);
let ret = t.dispatchEvent(new Event("ping"));
eq(ret, true, "dispatchEvent returns true when not cancelled");
eq(log.join(","), "h1:ping,h2:ping", "listeners fire in registration order");

// target/currentTarget set during dispatch
let seen = null;
t.addEventListener("chk", function(ev) { seen = (ev.target === t && ev.currentTarget === t && ev.eventPhase === 2); });
t.dispatchEvent(new Event("chk"));
eq(seen, true, "dispatch sets target/currentTarget/eventPhase");

// removeEventListener
log = [];
t.removeEventListener("ping", h1);
t.dispatchEvent(new Event("ping"));
eq(log.join(","), "h2:ping", "removeEventListener drops h1");

// non-cancelable dispatch return
eq(t.dispatchEvent(new Event("ping")), true, "dispatch returns true");

// --- once ------------------------------------------------------------------
let onceCount = 0;
let t2 = new EventTarget();
t2.addEventListener("o", function() { onceCount++; }, { once: true });
t2.dispatchEvent(new Event("o"));
t2.dispatchEvent(new Event("o"));
eq(onceCount, 1, "once listener fires only once");

// --- capture/other-type isolation ------------------------------------------
let t3 = new EventTarget();
let fired = [];
t3.addEventListener("a", function() { fired.push("a"); });
t3.addEventListener("b", function() { fired.push("b"); });
t3.dispatchEvent(new Event("a"));
eq(fired.join(","), "a", "only matching type fires");

// --- stopImmediatePropagation ----------------------------------------------
let t4 = new EventTarget();
let order = [];
t4.addEventListener("s", function(ev) { order.push(1); ev.stopImmediatePropagation(); });
t4.addEventListener("s", function(ev) { order.push(2); });
t4.dispatchEvent(new Event("s"));
eq(order.join(","), "1", "stopImmediatePropagation halts later listeners");

// --- dispatchEvent return reflects preventDefault --------------------------
let t5 = new EventTarget();
t5.addEventListener("c", function(ev) { ev.preventDefault(); });
eq(t5.dispatchEvent(new Event("c", { cancelable: true })), false, "dispatch returns false when default prevented");

// --- on<type> handler property ---------------------------------------------
let t6 = new EventTarget();
let onFired = 0;
t6.oncustom = function() { onFired++; };
t6.dispatchEvent(new Event("custom"));
eq(onFired, 1, "on<type> handler property fires");

// --- CustomEvent ------------------------------------------------------------
let ce = new CustomEvent("my", { detail: { id: 7 }, bubbles: true });
eq(ce.type, "my", "customEvent.type");
eq(ce.detail.id, 7, "customEvent.detail");
eq(ce.bubbles, true, "customEvent.bubbles");
eq(ce instanceof CustomEvent, true, "ce instanceof CustomEvent");
eq(ce instanceof Event, true, "ce instanceof Event (prototype chain)");
eq(Object.prototype.toString.call(ce), "[object CustomEvent]", "customEvent toStringTag");
// inherits Event methods through the chain
eq(typeof ce.preventDefault, "function", "customEvent inherits preventDefault");
eq(typeof ce.stopPropagation, "function", "customEvent inherits stopPropagation");
let ceNull = new CustomEvent("n");
eq(ceNull.detail, null, "customEvent detail defaults to null");

// CustomEvent dispatched through EventTarget carries detail
let t7 = new EventTarget();
let gotDetail = null;
t7.addEventListener("d", function(ev) { gotDetail = ev.detail; });
t7.dispatchEvent(new CustomEvent("d", { detail: "payload" }));
eq(gotDetail, "payload", "listener receives customEvent.detail");

// instanceof EventTarget
eq(t instanceof EventTarget, true, "target instanceof EventTarget");

console.log(failed === 0 ? "__EVENT_OK__" : ("__EVENT_FAIL__ " + failed));
console.log("EventTarget/Event/CustomEvent: " + passed + " passed, " + failed + " failed");
