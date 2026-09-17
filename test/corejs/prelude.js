// prelude.js - minimal browser-ish globals for the core-js bundle harness.
//
// The core-js bundle unconditionally probes a handful of host globals during its
// `globalThis` detection and engine sniffing. We define ONLY the names that are
// missing; we never reassign the `globalThis` binding itself (assigning to it
// trips a known VM bug that aborts the whole run - see native_Object.c).
(function () {
  var g = globalThis;

  function defIfMissing(name, value) {
    if (typeof g[name] === 'undefined') g[name] = value;
  }

  // Aliases core-js probes while resolving the global object. Each must satisfy
  // `it.Math === Math`, which holds because they all point at the real global.
  defIfMissing('self', g);
  defIfMissing('window', g);
  defIfMissing('global', g);

  // navigator: read for engine/version detection (no real browser here).
  if (typeof g.navigator === 'undefined') {
    g.navigator = { userAgent: 'mario-js', product: 'mario', vendor: '' };
  }

  // Minimal document stub. core-js feature-detects DOM APIs but guards access,
  // so a near-empty object is enough to keep unconditional reads from throwing.
  if (typeof g.document === 'undefined') {
    g.document = {
      documentElement: {},
      createElement: function () { return { style: {} }; },
      createElementNS: function () { return { style: {} }; }
    };
  }

  // queueMicrotask: route onto the native Promise microtask path if absent.
  if (typeof g.queueMicrotask !== 'function') {
    g.queueMicrotask = function (fn) { Promise.resolve().then(fn); };
  }
})();
