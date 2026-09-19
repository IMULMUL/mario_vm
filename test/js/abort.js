// AbortController / AbortSignal self-check (WHATWG subset).
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) { passed++; }
    else { failed++; console.log("FAIL " + msg + ": got " + JSON.stringify(got) + " want " + JSON.stringify(want)); }
}

function finish() {
    console.log(failed === 0 ? "__ABORT_OK__" : ("__ABORT_FAIL__ " + failed));
    console.log("AbortController/AbortSignal: " + passed + " passed, " + failed + " failed");
}

// --- initial state ----------------------------------------------------------
let c0 = new AbortController();
eq(c0.signal.aborted, false, "signal.aborted initially false");
eq(c0.signal.reason, undefined, "signal.reason initially undefined");
eq(c0 instanceof AbortController, true, "controller instanceof AbortController");
eq(c0.signal instanceof AbortSignal, true, "signal instanceof AbortSignal");
eq(c0.signal instanceof EventTarget, true, "signal instanceof EventTarget (chain)");
eq(typeof c0.signal.addEventListener, "function", "signal inherits addEventListener");
eq(Object.prototype.toString.call(c0), "[object AbortController]", "controller toStringTag");
eq(Object.prototype.toString.call(c0.signal), "[object AbortSignal]", "signal toStringTag");

// --- abort() default reason + event ----------------------------------------
let c1 = new AbortController();
let fired = 0, onFired = 0, evTargetOk = false;
c1.signal.addEventListener("abort", function(ev) { fired++; evTargetOk = (ev.type === "abort"); });
c1.signal.onabort = function() { onFired++; };
let ret = c1.signal.dispatchEvent; // sanity: method present
eq(typeof ret, "function", "signal has dispatchEvent");
c1.abort();
eq(c1.signal.aborted, true, "aborted after abort()");
eq(c1.signal.reason instanceof Error, true, "default reason is an Error");
eq(c1.signal.reason.name, "AbortError", "default reason name AbortError");
eq(fired, 1, "abort listener fired once");
eq(onFired, 1, "onabort handler fired once");
eq(evTargetOk, true, "abort event has type 'abort'");
c1.abort();               // idempotent
eq(fired, 1, "second abort() does not re-fire");

// --- abort(customReason) ----------------------------------------------------
let c2 = new AbortController();
let myReason = new Error("custom");
c2.abort(myReason);
eq(c2.signal.reason === myReason, true, "custom reason preserved by identity");

// --- throwIfAborted ---------------------------------------------------------
let c3 = new AbortController();
let threw = false;
try { c3.signal.throwIfAborted(); } catch (e) { threw = true; }
eq(threw, false, "throwIfAborted no-throw when not aborted");
c3.abort("stop");
threw = false; let caught = null;
try { c3.signal.throwIfAborted(); } catch (e) { threw = true; caught = e; }
eq(threw, true, "throwIfAborted throws when aborted");
eq(caught, "stop", "throwIfAborted throws the reason value");

// --- AbortSignal.abort(reason) ---------------------------------------------
let s1 = AbortSignal.abort();
eq(s1.aborted, true, "AbortSignal.abort() already aborted");
eq(s1.reason.name, "AbortError", "AbortSignal.abort() default reason");
let s2 = AbortSignal.abort("boom");
eq(s2.reason, "boom", "AbortSignal.abort(reason) preserves reason");

// --- AbortSignal.any([...]) -------------------------------------------------
let early = AbortSignal.abort("early");
let comp1 = AbortSignal.any([early]);
eq(comp1.aborted, true, "any() with already-aborted source is aborted");
eq(comp1.reason, "early", "any() adopts already-aborted reason");

let cA = new AbortController();
let cB = new AbortController();
let comp2 = AbortSignal.any([cA.signal, cB.signal]);
eq(comp2.aborted, false, "any() composite not aborted initially");
cB.abort("fromB");
eq(comp2.aborted, true, "any() aborts when a source aborts");
eq(comp2.reason, "fromB", "any() adopts the aborting source's reason");

// --- AbortSignal.timeout(ms): verified after the CLI event loop runs --------
let tsig = AbortSignal.timeout(5);
eq(tsig.aborted, false, "timeout signal not aborted synchronously");
eq(tsig instanceof AbortSignal, true, "timeout returns an AbortSignal");
setTimeout(function() {
    eq(tsig.aborted, true, "timeout signal aborted after delay");
    eq(tsig.reason.name, "TimeoutError", "timeout reason name TimeoutError");
    finish();
}, 40);
