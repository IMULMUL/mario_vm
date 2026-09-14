// Object.defineProperty accessor descriptors ({get}/{set}) must install working
// accessors, mirroring object-literal get/set. This is what webpack's ESM export
// runtime relies on: Object.defineProperty(exports, key, {enumerable:true, get:fn}).
var pass = 0, fail = 0;
function ok(c, m){ if(c){pass++;} else {fail++; console.log("FAIL: "+m);} }

var o = {};
Object.defineProperty(o, "a", { enumerable:true, get: function(){ return 42; } });
ok(o.a === 42, "getter returns 42, got " + o.a);

var backed = 7;
Object.defineProperty(o, "b", {
  get: function(){ return backed; },
  set: function(v){ backed = v * 2; }
});
ok(o.b === 7, "get b == 7, got " + o.b);
o.b = 5;
ok(backed === 10, "setter doubled, backed=" + backed);
ok(o.b === 10, "get b == 10 after set, got " + o.b);

// defineProperty returns the target (chaining), per spec.
var r = Object.defineProperty({}, "x", { value: 1 });
ok(r !== undefined && r.x === 1, "returns target with x=1");

// webpack-style export getter over a module namespace object.
var exports = {};
var definition = {
  fn: function(){ return "called"; },
  val: 99
};
Object.keys(definition).forEach(function(k){
  Object.defineProperty(exports, k, { enumerable:true, get: function(){ return definition[k]; } });
});
ok(typeof exports.fn === "function", "export fn is a function");
ok(exports.val === 99, "export val == 99, got " + exports.val);

// strict-mode indirect call form used by webpack: (0, mod.fn)().
// NOTE: the direct method-call form `exports.fn()` is a known, pre-existing
// engine limitation shared with object-literal getters: find_func() returns the
// raw getter instead of invoking it, so `o.accessor()` calls the getter rather
// than the function it returns. Webpack's harmony exports always use the
// indirect `(0, mod.fn)()` form below, which resolves the getter first and then
// calls the result correctly.
var fnRef = exports.fn;
ok(fnRef() === "called", "resolved export fn() works, got " + fnRef());
var indirect = (0, exports.fn)();
ok(indirect === "called", "indirect (0,fn)() works, got " + indirect);

console.log("t_defineproperty_accessor: pass=" + pass + " fail=" + fail);
