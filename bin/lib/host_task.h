#ifndef MARIO_HOST_TASK_H
#define MARIO_HOST_TASK_H

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CLI-host timer table + event loop (bin/lib/host_task.c).
 *
 * Provides js_dom_add_timer for the standalone build, registers the timer
 * globals (setTimeout/setInterval/clearTimeout/clearInterval/queueMicrotask)
 * plus the await spin hook, and drains the event loop after vm_run.
 *
 * This is CLI-only host glue: the browser build does not compile host_task.c
 * (its setTimeout and js_dom_add_timer come from jsnative/natives/js_dom.c), so
 * these entry points are declared here and called from bin/mario/mario.c rather
 * than from the shared reg_builtin_natives chain. */
void host_task_register(vm_t* vm);
void host_task_drain(vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif
