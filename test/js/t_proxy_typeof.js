// Regression: typeof must see through a Proxy to its target's callability.
// Pinterest's axios-style CancelToken does `if("function"!=typeof executor)
// throw TypeError("executor must be a function.")`; a proxied callback used to
// report "object" and abort the whole client bundle.
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) passed++;
    else { failed++; console.log("FAIL " + msg + ": got " + got + " want " + want); }
}

function plain(x) { return x * 2; }
var pf = new Proxy(plain, {});
eq(typeof pf, "function", "typeof proxy-of-function");
eq(pf(21), 42, "proxy-of-function invokes target");

var withApply = new Proxy(plain, { apply: (t, th, a) => a[0] + 100 });
eq(typeof withApply, "function", "typeof proxy with apply trap");
eq(withApply(1), 101, "apply trap honored");

// proxy of a non-function object stays "object"
eq(typeof new Proxy({}, {}), "object", "typeof proxy-of-object");
eq(typeof new Proxy([], {}), "object", "typeof proxy-of-array");

// class target
class K {}
eq(typeof new Proxy(K, {}), "function", "typeof proxy-of-class");

// the CancelToken-style guard must accept a proxied executor
function Ctor(e) { if (typeof e != "function") throw TypeError("executor must be a function."); this.got = typeof e; }
var ok = false;
try { ok = (new Ctor(pf)).got === "function"; } catch (err) { ok = false; }
eq(ok, true, "typeof-guard accepts proxied callback");

console.log(failed === 0 ? "__PROXY_TYPEOF_OK__" : ("__PROXY_TYPEOF_FAIL__ " + failed));
console.log("proxy typeof regression: " + passed + " passed, " + failed + " failed");
