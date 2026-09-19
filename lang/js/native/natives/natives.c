#include "natives.h"
#include "Console/native_Console.h"
#include "TextEncoder/native_TextEncoder.h"
#include "Stream/native_Stream.h"
#include "Process/native_Process.h"
#include "Buffer/native_Buffer.h"
#include "URL/native_URL.h"
#include "EventTarget/native_EventTarget.h"
#include "AbortController/native_AbortController.h"
#include "EventEmitter/native_EventEmitter.h"
#include "Web/native_Web.h"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_natives(vm_t* vm) {
	reg_native_Console(vm);
	reg_native_TextEncoder(vm);
	reg_native_Stream(vm);
	reg_native_Process(vm);
	reg_native_Buffer(vm);
	reg_native_URL(vm);
	reg_native_EventTarget(vm);
	reg_native_AbortController(vm);
	reg_native_EventEmitter(vm);
	reg_native_Web(vm);
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
