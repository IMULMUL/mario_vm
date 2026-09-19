#ifndef MARIO_NATIVE_PROCESS
#define MARIO_NATIVE_PROCESS

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registers the Node.js `process` global (a singleton object on vm->root) with a
 * minimal but practical surface: env / argv / platform / version / versions /
 * pid / execArgv / stdout / stderr, plus exit() / nextTick() / cwd() / env
 * lookups. Enough for bundled Node code that feature-detects `process` and reads
 * process.env.NODE_ENV, schedules process.nextTick, or writes to process.stdout. */
void reg_native_Process(vm_t* vm);

/* Refresh process.argv from vm->root's "_args" (the CLI argv snapshot). The
 * process global is built during vm_init, before main() installs _args, so the
 * host calls this once after init_args() to populate argv. Safe to call when
 * _args or process is absent (no-op). */
void native_Process_set_argv(vm_t* vm);

#ifdef __cplusplus
}
#endif

#endif
