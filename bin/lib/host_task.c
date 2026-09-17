/*
 * host_task.c - CLI-host task/timer shim for the standalone mario runner.
 *
 * native_Promise.c schedules reaction drains through js_dom_add_timer(), which
 * in the embedded engine lives in ewebview's jsnative/natives/js_dom.c and is
 * driven by the engine's event loop. The standalone build/mario CLI links none
 * of that, so this file provides the symbol.
 *
 * The CLI has no wall-clock event loop and no timer table, so js_dom_add_timer()
 * reports failure by returning 0. That is the documented contract the callers
 * already handle: promise_schedule_drain() then runs the reaction inline
 * ("table full: never lose reactions - run them now"). Inline reactions are also
 * what the ES6 test suite asserts against (it checks a promise result on the
 * very next synchronous statement after .then()), so a deferred pump would
 * regress it. host_task_drain() is therefore a no-op safety net today; it exists
 * so main() has a single post-run hook if a real deferred queue (e.g. a native
 * setTimeout/queueMicrotask) is added later.
 */
#include "mario.h"
#include <stdint.h>
#include <stdbool.h>

/* No CLI timer table: return 0 so callers fall back to running work inline. */
int js_dom_add_timer(vm_t* vm, var_t* cb, uint32_t ms, bool repeat) {
    (void)vm; (void)cb; (void)ms; (void)repeat;
    return 0;
}

/* Accepted for symmetry with the embedded host; nothing to clear. */
void js_dom_clear_timer(vm_t* vm, int id) { (void)vm; (void)id; }

/* Post-run hook. No deferred tasks exist in the CLI today (see above), so this
 * is intentionally empty; kept as the single place to drain a future queue. */
void host_task_drain(vm_t* vm) { (void)vm; }
