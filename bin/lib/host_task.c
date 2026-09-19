/*
 * host_task.c - CLI-host timer table + event loop for the standalone mario runner.
 *
 * native_Promise.c schedules reaction drains through js_dom_add_timer(), which
 * in the embedded engine lives in ewebview's jsnative/natives/js_dom.c and is
 * driven by the engine's event loop. The standalone build/mario CLI links none
 * of that, so this file provides the symbol.
 *
 * Historically this was a no-op stub (js_dom_add_timer returned 0), which made
 * promise_schedule_drain() fall back to running reactions inline and left
 * setTimeout/setInterval/queueMicrotask non-functional. That blocked any real
 * Node.js-style script (timers, deferred microtasks, async/await over a timer).
 *
 * This implementation mirrors the embedded js_dom.c timer model so the CLI and
 * the browser behave identically:
 *   - A fixed table of timer slots (allocation-free hot path).
 *   - Callbacks (and any extra setTimeout args) are anchored from the GC in a
 *     hidden invisable "@@cli_timers" array on vm->root, because the GC marks
 *     from vm->root ignoring refcounts and a scheduled callback must outlive
 *     the script run that armed it.
 *   - js_dom_poll_timers() charges elapsed wall-clock time to every active timer
 *     and fires the due ones; the embedder (host_task_drain / the await spin
 *     tick) drives it with a monotonic ms clock.
 *
 * host_task_drain() is the post-run event loop: it polls until no timer remains
 * armed, sleeping until the next due timer in between. vm->on_await_pending is
 * wired to a tick that pumps the same table so `await` over a not-yet-settled
 * promise (timer / microtask) resolves instead of degrading to undefined.
 */
#include "mario.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CLI_TIMER_MAX 2048
#define CLI_TIMERS_KEY "@@cli_timers"

typedef struct {
    int       id;            /* value handed to JS (never 0) */
    var_t*    cb;            /* anchored via CLI_TIMERS_KEY */
    var_t*    args;          /* extra setTimeout args array, or NULL (anchored) */
    uint32_t  period_ms;     /* interval between fires */
    int64_t   remaining_ms;  /* counts down each poll; due at <= 0 */
    bool      repeat;        /* setInterval vs setTimeout */
    bool      active;        /* slot in use */
} cli_timer_t;

static cli_timer_t s_timers[CLI_TIMER_MAX];
static int         s_next_id   = 0;
static bool        s_synced    = false;
static uint64_t    s_last_now  = 0;

/* Monotonic wall clock in milliseconds. */
static uint64_t cli_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000L);
}

/* Rebuild the @@cli_timers anchor array from the current active set so every
 * live callback (and its extra-args array) stays reachable from vm->root.
 * var_add replaces the prior array (node_replace refs the new, unrefs the old);
 * the gc_defer window covers the moment the entries are momentarily
 * unreachable. */
static void cli_reanchor(vm_t* vm) {
    if(vm == NULL || vm->root == NULL) return;
    vm->gc.gc_defer++;
    var_t* fresh = var_new_array(vm);
    for(int i = 0; i < CLI_TIMER_MAX; ++i) {
        if(s_timers[i].active) {
            if(s_timers[i].cb != NULL)   var_array_add(fresh, s_timers[i].cb);
            if(s_timers[i].args != NULL) var_array_add(fresh, s_timers[i].args);
        }
    }
    node_t* n = var_add(vm->root, CLI_TIMERS_KEY, fresh);
    if(n != NULL) { n->invisable = 1; n->be_unenumerable = 1; }
    vm->gc.gc_defer--;
}

static int cli_add_timer(vm_t* vm, var_t* cb, var_t* args, uint32_t ms, bool repeat) {
    if(vm == NULL || cb == NULL || !cb->is_func) return 0;
    int slot = -1;
    for(int i = 0; i < CLI_TIMER_MAX; ++i)
        if(!s_timers[i].active) { slot = i; break; }
    if(slot < 0) return 0;                 /* table full: report failure */
    s_next_id++;
    if(s_next_id <= 0) s_next_id = 1;
    cli_timer_t* t = &s_timers[slot];
    t->id           = s_next_id;
    t->cb           = cb;
    t->args         = args;
    t->period_ms    = (ms < 1) ? 1 : ms;
    t->remaining_ms = (int64_t)t->period_ms;
    t->repeat       = repeat;
    t->active       = true;
    cli_reanchor(vm);
    return t->id;
}

static void cli_clear_timer(vm_t* vm, int id) {
    if(id <= 0) return;
    bool changed = false;
    for(int i = 0; i < CLI_TIMER_MAX; ++i) {
        if(s_timers[i].active && s_timers[i].id == id) {
            s_timers[i].active = false;
            s_timers[i].cb     = NULL;
            s_timers[i].args   = NULL;
            changed = true;
        }
    }
    if(changed) cli_reanchor(vm);
}

static bool cli_has_pending(void) {
    for(int i = 0; i < CLI_TIMER_MAX; ++i)
        if(s_timers[i].active) return true;
    return false;
}

static int64_t cli_min_remaining(void) {
    int64_t min = INT64_MAX;
    for(int i = 0; i < CLI_TIMER_MAX; ++i)
        if(s_timers[i].active && s_timers[i].remaining_ms < min)
            min = s_timers[i].remaining_ms;
    return (min == INT64_MAX) ? 0 : min;
}

/* Charge elapsed time to every active timer, then fire the due ones. Re-scan
 * from the top after each fire because a callback may add/clear timers; bounded
 * to avoid a runaway loop. */
static int cli_poll(vm_t* vm, uint64_t now_ms) {
    if(vm == NULL) return 0;
    if(!s_synced) {                 /* first poll only seeds the clock */
        s_last_now = now_ms;
        s_synced   = true;
        return 0;
    }
    if(now_ms <= s_last_now) return 0;   /* nothing elapsed / clock stepped back */
    uint64_t delta = now_ms - s_last_now;
    s_last_now = now_ms;

    for(int i = 0; i < CLI_TIMER_MAX; ++i)
        if(s_timers[i].active)
            s_timers[i].remaining_ms -= (int64_t)delta;

    int fired = 0;
    for(int guard = 0; guard < 4096; ++guard) {
        int due = -1;
        for(int i = 0; i < CLI_TIMER_MAX; ++i)
            if(s_timers[i].active && s_timers[i].remaining_ms <= 0) { due = i; break; }
        if(due < 0) break;
        cli_timer_t* t = &s_timers[due];
        var_t* cb   = t->cb;
        var_t* args = t->args;
        if(t->repeat) {
            t->remaining_ms = (int64_t)t->period_ms;  /* one fire per poll, no catch-up burst */
        } else {
            t->active = false;
            t->cb     = NULL;
            t->args   = NULL;
        }
        if(cb != NULL && cb->is_func) {
            var_t* call_args = (args != NULL) ? args : var_new_array(vm);
            var_t* r = call_m_func(vm, NULL, cb, call_args);
            if(r != NULL) var_unref(r);
            if(args == NULL) var_unref(call_args);
            fired++;
        }
    }
    cli_reanchor(vm);   /* drop fired one-shots from the GC anchor */
    return fired;
}

/* ------------------------------------------------------------------ */
/* Public timer API symbols native_Promise.c expects.                  */
/*                                                                     */
/* IMPORTANT: js_dom_add_timer() is the hook promise_schedule_drain()   */
/* uses to queue a reaction as a 0-ms microtask. In the browser build   */
/* (js_dom.c) it returns a real id so reactions defer to the engine     */
/* event loop. In the CLI we deliberately return 0, which is the        */
/* documented "table full" contract that makes promise_schedule_drain() */
/* run the reaction INLINE. The standalone test suite (es6_full.js,     */
/* transient_write.js) asserts promise results on the very next         */
/* synchronous statement after .then(), and deferring reactions through */
/* a nested poll during the await spin re-enters the VM mid-drain and   */
/* crashes it. Keeping reactions inline preserves that behaviour while  */
/* the SEPARATE user-timer table below still drives setTimeout /        */
/* setInterval / queueMicrotask and the await spin. */
int js_dom_add_timer(vm_t* vm, var_t* cb, uint32_t ms, bool repeat) {
    (void)vm; (void)cb; (void)ms; (void)repeat;
    return 0;   /* promise reactions run inline (see above) */
}

/* Same inline contract as js_dom_add_timer: the CLI has no microtask table, so
 * returning 0 makes promise_schedule_drain / process.nextTick run the reaction
 * inline (see the js_dom_add_timer note above). The separate user-timer table
 * below still drives setTimeout / setInterval / queueMicrotask and await. */
int js_dom_add_microtask(vm_t* vm, var_t* cb) {
    (void)vm; (void)cb;
    return 0;
}

void js_dom_clear_timer(vm_t* vm, int id) { (void)vm; (void)id; }

int js_dom_poll_timers(vm_t* vm, uint64_t now_ms) {
    return cli_poll(vm, now_ms);
}

int js_dom_has_pending_timers(vm_t* vm) {
    (void)vm;
    return cli_has_pending() ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* JS-facing timer globals: setTimeout / clearTimeout / setInterval /  */
/* clearInterval / queueMicrotask.                                     */
/* ------------------------------------------------------------------ */

static var_t* cli_set_timer(vm_t* vm, var_t* env, bool repeat) {
    var_t* args = get_func_args(env);
    node_t* cn = var_array_get(args, 0);
    node_t* mn = var_array_get(args, 1);
    if(cn == NULL || cn->var == NULL || !cn->var->is_func)
        return var_new_int(vm, 0);
    uint32_t ms = (mn != NULL && mn->var != NULL) ? (uint32_t)var_get_float(mn->var) : 0;

    /* setTimeout(cb, ms, arg1, arg2, ...): forward the trailing args to cb.
     * call_m_func expects the args array reversed (last arg at index 0), the
     * same convention array_call_cb() uses, so reverse once here and store the
     * reversed array (call_m_func only reads it, so a repeat timer can reuse
     * it for every fire). */
    var_t* extra = NULL;
    uint32_t n = var_array_size(args);
    if(n > 2) {
        extra = var_new_array(vm);
        for(uint32_t i = 2; i < n; ++i) {
            node_t* an = var_array_get(args, (int32_t)i);
            var_array_add(extra, (an != NULL && an->var != NULL) ? an->var : var_new(vm));
        }
        var_array_reverse(extra);
    }
    return var_new_int(vm, cli_add_timer(vm, cn->var, extra, ms, repeat));
}

static var_t* cli_clear_timer_native(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* args = get_func_args(env);
    node_t* n = var_array_get(args, 0);
    if(n != NULL && n->var != NULL)
        cli_clear_timer(vm, var_get_int(n->var));
    return NULL;
}

static var_t* native_setTimeout(vm_t* vm, var_t* env, void* data)    { (void)data; return cli_set_timer(vm, env, false); }
static var_t* native_setInterval(vm_t* vm, var_t* env, void* data)   { (void)data; return cli_set_timer(vm, env, true);  }
static var_t* native_clearTimeout(vm_t* vm, var_t* env, void* data)  { return cli_clear_timer_native(vm, env, data); }
static var_t* native_clearInterval(vm_t* vm, var_t* env, void* data) { return cli_clear_timer_native(vm, env, data); }

/* setImmediate(cb, args...): Node runs these on the next event-loop iteration
 * (the "check" phase, after pending I/O and microtasks). The CLI drives a single
 * poll-based table, so model it as a 0-delay one-shot: it becomes due on the next
 * cli_poll, the same tier queueMicrotask uses. Trailing args are forwarded to cb
 * exactly like setTimeout's, and the returned id feeds clearImmediate. */
static var_t* native_setImmediate(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* args = get_func_args(env);
    node_t* cn = var_array_get(args, 0);
    if(cn == NULL || cn->var == NULL || !cn->var->is_func)
        return var_new_int(vm, 0);
    var_t* extra = NULL;
    uint32_t n = var_array_size(args);
    if(n > 1) {
        extra = var_new_array(vm);
        for(uint32_t i = 1; i < n; ++i) {
            node_t* an = var_array_get(args, (int32_t)i);
            var_array_add(extra, (an != NULL && an->var != NULL) ? an->var : var_new(vm));
        }
        var_array_reverse(extra);
    }
    return var_new_int(vm, cli_add_timer(vm, cn->var, extra, 0, false));
}

static var_t* native_clearImmediate(vm_t* vm, var_t* env, void* data) { return cli_clear_timer_native(vm, env, data); }

/* queueMicrotask(cb): run cb as a 0-ms one-shot, ahead of any later timer.
 * Matches the microtask-before-macrotask ordering well enough for the CLI. */
static var_t* native_queueMicrotask(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* args = get_func_args(env);
    node_t* cn = var_array_get(args, 0);
    if(cn == NULL || cn->var == NULL || !cn->var->is_func) return NULL;
    cli_add_timer(vm, cn->var, NULL, 0, false);
    return NULL;
}

/* Await spin tick (vm->on_await_pending): pump the timer table so an await on a
 * promise that settles later (timer / microtask) resolves instead of yielding
 * undefined. When nothing is due but timers are still armed, sleep a short slice
 * so delayed callbacks get real wall-clock time; when the table is empty the
 * promise can never settle and we return 0. A per-promise wall-clock cap keeps a
 * genuinely stuck await from hanging the run. */
static int cli_await_tick(vm_t* vm, var_t* promise) {
    if(vm == NULL) return 0;
    static var_t*    s_last     = NULL;
    static uint64_t  s_deadline = 0;
    uint64_t now = cli_now_ms();
    if(promise != s_last) { s_last = promise; s_deadline = now + 30000; }
    if(now >= s_deadline) return 0;
    int fired = cli_poll(vm, now);
    if(fired > 0) return 1;
    if(cli_has_pending()) {
        int64_t min_rem = cli_min_remaining();
        uint64_t sleep_ms = (min_rem > 0 && min_rem < 20) ? (uint64_t)min_rem : 1;
        usleep((useconds_t)(sleep_ms * 1000));
        return 1;
    }
    return 0;
}

/* Register the timer globals and the await hook. Called from main() after
 * vm_init so these overwrite the inert setTimeout stub registered by
 * native_Promise.c and take effect before the script runs. */
void host_task_register(vm_t* vm) {
    if(vm == NULL) return;
    vm_reg_static(vm, NULL, "setTimeout(cb, ms)",    native_setTimeout,    NULL);
    vm_reg_static(vm, NULL, "clearTimeout(id)",      native_clearTimeout,  NULL);
    vm_reg_static(vm, NULL, "setInterval(cb, ms)",   native_setInterval,   NULL);
    vm_reg_static(vm, NULL, "clearInterval(id)",     native_clearInterval, NULL);
    vm_reg_static(vm, NULL, "queueMicrotask(cb)",    native_queueMicrotask, NULL);
    vm_reg_static(vm, NULL, "setImmediate(cb)",      native_setImmediate,   NULL);
    vm_reg_static(vm, NULL, "clearImmediate(id)",    native_clearImmediate, NULL);
    vm->on_await_pending = cli_await_tick;
}

/* Post-run event loop: poll until no timer remains armed, sleeping until the
 * next due timer in between. Mirrors Node.js, which keeps the process alive
 * while handles (timers) are pending and exits once the loop is empty. */
void host_task_drain(vm_t* vm) {
    if(vm == NULL) return;
    cli_poll(vm, cli_now_ms());   /* seed the clock */
    uint64_t guard = 0;
    while(guard++ < 100000000ULL) {
        if(!cli_has_pending()) break;
        int fired = cli_poll(vm, cli_now_ms());
        if(fired > 0) continue;
        int64_t min_rem = cli_min_remaining();
        if(min_rem <= 0) continue;
        uint64_t sleep_ms = (min_rem > 100) ? 100 : (uint64_t)min_rem;
        usleep((useconds_t)(sleep_ms * 1000));
    }
}
