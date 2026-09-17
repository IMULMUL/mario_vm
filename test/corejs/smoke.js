// smoke.js - post-load verification for the core-js bundle.
//
// Appended AFTER the bundle by run.sh, so every core-js polyfill has had its
// chance to install. Each check runs in its own try/catch and is recorded, so a
// single broken API never aborts the tail (the __COREJS_LOAD_OK__ sentinel must
// still print). The summary lists hard failures (wrong value / threw) and, for
// host-dependent APIs the CLI may legitimately lack, an informational note.
//
// Output goes through console.log (the CLI's `print` is not wired to stdout).
(function () {
  var pass = 0, fail = 0, fails = [], notes = [];

  function show(v) {
    try { var s = JSON.stringify(v); return s === undefined ? String(v) : s; }
    catch (e) { return String(v); }
  }
  function deepEq(a, b) {
    if (a === b) return true;
    if (typeof a === 'number' && typeof b === 'number' && a !== a && b !== b) return true; // NaN
    if (a && b && typeof a === 'object' && typeof b === 'object') {
      var aArr = typeof a.length === 'number' && !a.message && typeof a !== 'function';
      var bArr = typeof b.length === 'number' && !b.message && typeof b !== 'function';
      if (aArr && bArr) {
        if (a.length !== b.length) return false;
        for (var i = 0; i < a.length; i++) if (!deepEq(a[i], b[i])) return false;
        return true;
      }
      // plain object: compare own enumerable keys
      var ak = Object.keys(a).sort(), bk = Object.keys(b).sort();
      if (!deepEq(ak, bk)) return false;
      for (var k = 0; k < ak.length; k++) if (!deepEq(a[ak[k]], b[bk[k]])) return false;
      return true;
    }
    return false;
  }
  function eq(a, b, m) { if (!deepEq(a, b)) throw new Error((m || '') + ' expected ' + show(b) + ' got ' + show(a)); }
  function isTrue(c, m) { if (c !== true) throw new Error((m || '') + ' expected true, got ' + show(c)); }
  function check(name, fn) {
    try { fn(); pass++; }
    catch (e) { fail++; fails.push(name + ' :: ' + (e && e.message ? e.message : String(e))); }
  }
  // probe: report presence of an optional/host-dependent API without failing.
  function probe(name, present) {
    if (!present) notes.push('absent: ' + name);
    return present;
  }
  function fn_(x) { return typeof x === 'function'; }
  function hasMethod(obj, m) { return obj != null && fn_(obj[m]); }

  /* ---- Group 1: Symbol & well-known symbols ---- */
  check('Symbol.iterator etc. well-knowns', function () {
    var ks = ['iterator', 'asyncIterator', 'toStringTag', 'species', 'toPrimitive',
              'hasInstance', 'isConcatSpreadable', 'unscopables', 'match', 'matchAll',
              'replace', 'search', 'split'];
    for (var i = 0; i < ks.length; i++)
      if (typeof Symbol[ks[i]] !== 'symbol') throw new Error('Symbol.' + ks[i] + ' missing');
  });
  check('Symbol.for/keyFor registry', function () {
    isTrue(Symbol.for('smoke.k') === Symbol.for('smoke.k'), 'Symbol.for identity');
    eq(Symbol.keyFor(Symbol.for('smoke.k')), 'smoke.k', 'keyFor');
  });
  check('Symbol description', function () { eq(Symbol('d').description, 'd', 'description'); });
  check('Object.getOwnPropertySymbols', function () {
    var s = Symbol('p'); var o = {}; o[s] = 1;
    eq(Object.getOwnPropertySymbols(o).length, 1, 'own symbols');
  });
  if (probe('Symbol.isWellKnownSymbol', fn_(Symbol.isWellKnownSymbol)))
    check('Symbol.isWellKnownSymbol', function () {
      isTrue(Symbol.isWellKnownSymbol(Symbol.iterator) === true, 'iterator is WK');
      isTrue(Symbol.isWellKnownSymbol(Symbol('x')) === false, 'user sym not WK');
    });
  if (probe('Symbol.isRegisteredSymbol', fn_(Symbol.isRegisteredSymbol)))
    check('Symbol.isRegisteredSymbol', function () {
      isTrue(Symbol.isRegisteredSymbol(Symbol.for('r')) === true, 'registered');
    });
  if (probe('Symbol.metadata', typeof Symbol.metadata !== 'undefined'))
    check('Symbol.metadata is symbol', function () { eq(typeof Symbol.metadata, 'symbol', 'metadata'); });

  /* ---- Group 2: Iterator helpers ---- */
  if (probe('Iterator global', fn_(globalThis.Iterator) && fn_(Iterator.from))) {
    check('Iterator.from().map().toArray()', function () {
      eq(Iterator.from([1, 2, 3]).map(function (x) { return x * 2; }).toArray(), [2, 4, 6], 'map/toArray');
    });
    check('Iterator filter/take/drop', function () {
      eq(Iterator.from([1, 2, 3, 4, 5]).filter(function (x) { return x % 2; }).take(2).toArray(), [1, 3], 'filter/take');
      eq(Iterator.from([1, 2, 3]).drop(1).toArray(), [2, 3], 'drop');
    });
    check('Iterator reduce/forEach/some/every/find', function () {
      eq(Iterator.from([1, 2, 3]).reduce(function (a, b) { return a + b; }), 6, 'reduce');
      var s = 0; Iterator.from([1, 2]).forEach(function (x) { s += x; }); eq(s, 3, 'forEach');
      isTrue(Iterator.from([1, 2]).some(function (x) { return x === 2; }), 'some');
      isTrue(Iterator.from([1, 2]).every(function (x) { return x > 0; }), 'every');
      eq(Iterator.from([1, 2, 3]).find(function (x) { return x > 1; }), 2, 'find');
    });
    check('Iterator flatMap', function () {
      eq(Iterator.from([1, 2]).flatMap(function (x) { return [x, x]; }).toArray(), [1, 1, 2, 2], 'flatMap');
    });
  }
  check('Array iterator protocol (next + @@iterator self)', function () {
    var it = [1, 2].values();
    isTrue(fn_(it.next), 'values().next is fn');
    isTrue(fn_(it[Symbol.iterator]) && it[Symbol.iterator]() === it, '@@iterator returns self');
    eq(it.next().value, 1, 'first next');
  });
  check('Array keys/values/entries present', function () {
    isTrue(hasMethod([], 'keys') && hasMethod([], 'values') && hasMethod([], 'entries'), 'array iters');
    eq(Array.from([7, 8].entries()), [[0, 7], [1, 8]], 'entries shape');
  });

  /* ---- Group 3: Object ---- */
  if (probe('Object.groupBy', fn_(Object.groupBy)))
    check('Object.groupBy', function () {
      var g = Object.groupBy([1, 2, 3, 4], function (x) { return x % 2 ? 'odd' : 'even'; });
      eq(g.odd, [1, 3], 'odd'); eq(g.even, [2, 4], 'even');
    });
  check('Object.hasOwn/fromEntries/entries/assign/is', function () {
    isTrue(Object.hasOwn({ a: 1 }, 'a'), 'hasOwn');
    eq(Object.fromEntries([['a', 1], ['b', 2]]), { a: 1, b: 2 }, 'fromEntries');
    eq(Object.entries({ a: 1 }), [['a', 1]], 'entries');
    eq(Object.assign({}, { a: 1 }, { b: 2 }), { a: 1, b: 2 }, 'assign');
    isTrue(Object.is(NaN, NaN), 'Object.is NaN');
  });
  check('Object.getOwnPropertyDescriptors', function () {
    var d = Object.getOwnPropertyDescriptors({ get x() { return 1; } });
    isTrue(fn_(d.x.get), 'accessor descriptor get is fn');
  });

  /* ---- Group 4: Array ---- */
  check('Array.from/of/isArray', function () {
    eq(Array.from('ab'), ['a', 'b'], 'from string');
    eq(Array.of(1, 2), [1, 2], 'of');
    isTrue(Array.isArray([]), 'isArray');
  });
  check('Array at/flat/flatMap/findLast/findLastIndex', function () {
    eq([1, 2, 3].at(-1), 3, 'at');
    eq([1, [2, [3]]].flat(), [1, 2, [3]], 'flat');
    eq([1, 2].flatMap(function (x) { return [x, x]; }), [1, 1, 2, 2], 'flatMap');
    eq([1, 2, 3].findLast(function (x) { return x % 2; }), 3, 'findLast');
    eq([1, 2, 3].findLastIndex(function (x) { return x % 2; }), 2, 'findLastIndex');
  });
  check('Array copyWithin/fill/includes', function () {
    eq([1, 2, 3, 4, 5].copyWithin(0, 3), [4, 5, 3, 4, 5], 'copyWithin');
    eq([1, 2, 3].fill(0, 1), [1, 0, 0], 'fill');
    isTrue([1, 2, NaN].includes(NaN), 'includes NaN');
  });
  check('Array toReversed/toSorted/toSpliced/with', function () {
    eq([3, 1, 2].toReversed(), [2, 1, 3], 'toReversed');
    eq([3, 1, 2].toSorted(), [1, 2, 3], 'toSorted');
    eq([1, 2, 3].toSpliced(1, 1), [1, 3], 'toSpliced');
    eq([1, 2, 3].with(1, 9), [1, 9, 3], 'with');
  });
  if (probe('Array.prototype.group', fn_([].group)))
    check('Array.prototype.group', function () {
      var g = [1, 2, 3].group(function (x) { return x % 2 ? 'odd' : 'even'; });
      eq(g.odd, [1, 3], 'group odd');
    });

  /* ---- Group 5: String ---- */
  check('String replaceAll/at/pad/trim/codePointAt', function () {
    eq('a-b-c'.replaceAll('-', '+'), 'a+b+c', 'replaceAll');
    eq('abc'.at(-1), 'c', 'at');
    eq('5'.padStart(3, '0'), '005', 'padStart');
    eq('5'.padEnd(3, '0'), '500', 'padEnd');
    eq('  x '.trimStart(), 'x ', 'trimStart');
    eq('abc'.codePointAt(0), 97, 'codePointAt');
    eq(String.fromCodePoint(97, 98), 'ab', 'fromCodePoint');
  });
  if (probe('String.prototype.matchAll', fn_(''.matchAll)))
    check('String matchAll', function () {
      var out = []; for (var m of 'a1b2'.matchAll(new RegExp('(\\w)(\\d)', 'g'))) out.push(m[0]);
      eq(out, ['a1', 'b2'], 'matchAll');
    });
  if (probe('String.prototype.isWellFormed', fn_(''.isWellFormed)))
    check('String isWellFormed/toWellFormed', function () {
      isTrue('abc'.isWellFormed(), 'isWellFormed');
      eq('abc'.toWellFormed(), 'abc', 'toWellFormed');
    });
  check('String @@iterator', function () {
    eq(Array.from('ab'), ['a', 'b'], 'string iterator');
  });
  if (probe('String.raw', fn_(String.raw)))
    check('String.raw', function () { eq(String.raw`a\nb`, 'a\\nb', 'raw keeps backslash'); });

  /* ---- Group 6: Number / Math ---- */
  check('Number statics', function () {
    eq(Number.parseInt('42px'), 42, 'parseInt');
    eq(Number.parseFloat('1.5x'), 1.5, 'parseFloat');
    isTrue(Number.isInteger(3), 'isInteger');
    isTrue(Number.isSafeInteger(3), 'isSafeInteger');
    eq(Number.MAX_SAFE_INTEGER, 9007199254740991, 'MAX_SAFE_INTEGER');
    isTrue(Number.isFinite(1) && !Number.isFinite('1'), 'isFinite no coerce');
  });
  check('Math methods', function () {
    var ms = ['clz32', 'fround', 'imul', 'log10', 'log2', 'log1p', 'expm1', 'cbrt',
              'sign', 'trunc', 'sinh', 'cosh', 'tanh', 'asinh', 'acosh', 'atanh', 'hypot'];
    for (var i = 0; i < ms.length; i++) if (!fn_(Math[ms[i]])) throw new Error('Math.' + ms[i] + ' missing');
    eq(Math.clz32(1), 31, 'clz32'); eq(Math.sign(-5), -1, 'sign');
    eq(Math.trunc(1.9), 1, 'trunc'); eq(Math.cbrt(27), 3, 'cbrt');
    eq(Math.imul(3, 4), 12, 'imul'); eq(Math.hypot(3, 4), 5, 'hypot');
  });

  /* ---- Group 7: RegExp ---- */
  if (probe('RegExp named groups', (function () { try { return new RegExp('(?<x>a)').exec('a').groups != null; } catch (e) { return false; } })()))
    check('RegExp named capture groups', function () {
      eq(new RegExp('(?<y>\\d+)').exec('a12').groups.y, '12', 'named group');
    });
  check('RegExp flags/source/exec/test', function () {
    var r = new RegExp('a', 'g');
    isTrue(r.global, 'global flag');
    eq(r.source, 'a', 'source');
    isTrue(r.test('ba'), 'test');
    eq('bab'.replace(new RegExp('b', 'g'), 'X'), 'XaX', '@@replace');
  });
  if (probe('RegExp dotAll (s)', (function () { try { return new RegExp('.', 's').dotAll === true; } catch (e) { return false; } })()))
    check('RegExp dotAll', function () { isTrue(new RegExp('a.b', 's').test('a\nb'), 'dotAll matches newline'); });

  /* ---- Group 8: Promise ---- */
  check('Promise combinators exist', function () {
    var ps = ['all', 'allSettled', 'any', 'race', 'resolve', 'reject'];
    for (var i = 0; i < ps.length; i++) if (!fn_(Promise[ps[i]])) throw new Error('Promise.' + ps[i] + ' missing');
  });
  if (probe('Promise.withResolvers', fn_(Promise.withResolvers)))
    check('Promise.withResolvers shape', function () {
      var w = Promise.withResolvers();
      isTrue(w.promise instanceof Promise && fn_(w.resolve) && fn_(w.reject), 'withResolvers');
    });
  if (probe('Promise.try', fn_(Promise.try)))
    check('Promise.try returns promise', function () { isTrue(Promise.try(function () { return 1; }) instanceof Promise, 'try'); });
  check('AggregateError', function () {
    var e = new AggregateError([new Error('a')], 'agg');
    isTrue(e instanceof Error, 'AggregateError is Error');
    eq(e.errors.length, 1, 'errors len');
  });
  // async resolution: result printed during the post-run drain (not in summary).
  Promise.all([Promise.resolve(1), Promise.resolve(2)]).then(function (vs) {
    console.log('SMOKE-ASYNC Promise.all => ' + show(vs));
  }, function () { console.log('SMOKE-ASYNC Promise.all REJECTED'); });

  /* ---- Group 9: Map / Set ---- */
  check('Map basics + iterators', function () {
    var m = new Map([[1, 'a'], [2, 'b']]);
    eq(m.get(1), 'a', 'get'); eq(m.size, 2, 'size');
    m.set(3, 'c'); isTrue(m.has(3), 'has'); m.delete(3); isTrue(!m.has(3), 'delete');
    eq(Array.from(m.keys()), [1, 2], 'keys');
    eq(Array.from(m.values()), ['a', 'b'], 'values');
    var it = m.entries(); isTrue(fn_(it.next) && it[Symbol.iterator]() === it, 'map iter self');
  });
  check('Set basics + iterators', function () {
    var s = new Set([1, 2, 2, 3]);
    eq(s.size, 3, 'size'); isTrue(s.has(2), 'has');
    eq(Array.from(s.values()), [1, 2, 3], 'values');
    var it = s.keys(); isTrue(fn_(it.next) && it[Symbol.iterator]() === it, 'set iter self');
  });
  check('Set methods (union/intersection/difference/...)', function () {
    var a = new Set([1, 2, 3]), b = new Set([2, 3, 4]);
    var need = ['union', 'intersection', 'difference', 'symmetricDifference',
                'isSubsetOf', 'isSupersetOf', 'isDisjointFrom'];
    for (var i = 0; i < need.length; i++) if (!fn_(a[need[i]])) throw new Error('Set.' + need[i] + ' missing');
    eq(Array.from(a.union(b)).sort(), [1, 2, 3, 4], 'union');
    eq(Array.from(a.intersection(b)).sort(), [2, 3], 'intersection');
    eq(Array.from(a.difference(b)).sort(), [1], 'difference');
    eq(Array.from(a.symmetricDifference(b)).sort(), [1, 4], 'symmetricDifference');
    isTrue(new Set([1, 2]).isSubsetOf(a) === true, 'isSubsetOf');
    isTrue(a.isSupersetOf(new Set([1, 2])) === true, 'isSupersetOf');
    isTrue(new Set([9]).isDisjointFrom(a) === true, 'isDisjointFrom');
  });
  if (probe('Map.groupBy', fn_(Map.groupBy)))
    check('Map.groupBy', function () {
      var g = Map.groupBy([1, 2, 3, 4], function (x) { return x % 2 ? 'odd' : 'even'; });
      isTrue(g instanceof Map, 'returns Map');
      eq(g.get('odd'), [1, 3], 'odd group');
    });

  /* ---- Group 10: TypedArray / ArrayBuffer / DataView ---- */
  check('TypedArray toSorted/at/slice/fill', function () {
    var t = new Uint8Array([3, 1, 2]);
    eq(Array.from(t.toSorted()), [1, 2, 3], 'toSorted');
    eq(t.at(-1), 2, 'at');
    eq(Array.from(t.slice(1)), [1, 2], 'slice');
    var u = new Uint8Array(3); u.fill(7); eq(Array.from(u), [7, 7, 7], 'fill');
  });
  check('TypedArray iterators + methods', function () {
    var t = new Int32Array([5, 6, 7]);
    eq(Array.from(t.values()), [5, 6, 7], 'values');
    var need = ['entries', 'keys', 'forEach', 'map', 'filter', 'find', 'findIndex',
                'reduce', 'some', 'every', 'includes', 'indexOf', 'join', 'reverse',
                'subarray', 'copyWithin', 'set', 'sort', 'findLast'];
    for (var i = 0; i < need.length; i++) if (!fn_(t[need[i]])) throw new Error('%TypedArray%.' + need[i] + ' missing');
  });
  check('DataView accessors', function () {
    var b = new ArrayBuffer(8); var dv = new DataView(b);
    dv.setInt32(0, -1234); eq(dv.getInt32(0), -1234, 'int32 round-trip');
    dv.setUint8(4, 200); eq(dv.getUint8(4), 200, 'uint8');
    eq(b.byteLength, 8, 'byteLength');
  });

  /* ---- Group 11: Error handling ---- */
  check('Error cause', function () {
    var e = new Error('m', { cause: 'why' });
    eq(e.cause, 'why', 'cause');
    eq(e.message, 'm', 'message');
    isTrue(e instanceof Error, 'instanceof');
  });
  if (probe('Error.isError', fn_(Error.isError)))
    check('Error.isError', function () {
      isTrue(Error.isError(new Error('x')) === true, 'isError true');
      isTrue(Error.isError({}) === false, 'isError false');
    });

  /* ---- Group 12: Weak refs ---- */
  if (probe('WeakRef', fn_(globalThis.WeakRef)))
    check('WeakRef deref', function () {
      var o = { a: 1 }; var w = new WeakRef(o);
      isTrue(w.deref() === o, 'deref identity');
    });
  if (probe('FinalizationRegistry', fn_(globalThis.FinalizationRegistry)))
    check('FinalizationRegistry constructs', function () {
      var fr = new FinalizationRegistry(function () {});
      isTrue(fr != null, 'constructed');
    });

  /* ---- Group 13: Web / host builtins core-js polyfills ---- */
  if (probe('queueMicrotask', fn_(globalThis.queueMicrotask)))
    check('queueMicrotask callable', function () { queueMicrotask(function () {}); });
  if (probe('structuredClone', fn_(globalThis.structuredClone)))
    check('structuredClone', function () {
      var o = { a: 1, b: [2, 3] }; var c = structuredClone(o);
      eq(c, o, 'clone equal'); isTrue(c !== o && c.b !== o.b, 'deep clone');
    });
  if (probe('TextEncoder/TextDecoder', fn_(globalThis.TextEncoder) && fn_(globalThis.TextDecoder)))
    check('TextEncoder/TextDecoder round-trip', function () {
      var enc = new TextEncoder(); var dec = new TextDecoder();
      eq(dec.decode(enc.encode('hi')), 'hi', 'round-trip');
    });
  if (probe('URL/URLSearchParams', fn_(globalThis.URL) && fn_(globalThis.URLSearchParams)))
    check('URL parsing', function () {
      var u = new URL('http://x.com/p?a=1');
      eq(u.pathname, '/p', 'pathname');
      eq(new URLSearchParams('a=1&b=2').get('b'), '2', 'searchParams');
    });
  if (probe('atob/btoa', fn_(globalThis.atob) && fn_(globalThis.btoa)))
    check('atob/btoa round-trip', function () {
      eq(atob(btoa('hi')), 'hi', 'base64 round-trip');
    });
  if (probe('AbortController', fn_(globalThis.AbortController)))
    check('AbortController', function () {
      var ac = new AbortController(); isTrue(ac.signal != null, 'signal');
      ac.abort(); isTrue(ac.signal.aborted === true, 'aborted');
    });

  /* ---- Summary ---- */
  console.log('=== core-js SMOKE: ' + pass + ' passed, ' + fail + ' failed ===');
  for (var i = 0; i < fails.length; i++) console.log('  FAIL ' + fails[i]);
  for (var j = 0; j < notes.length; j++) console.log('  NOTE ' + notes[j]);
})();
