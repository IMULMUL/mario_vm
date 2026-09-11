#include "natives_builtin.h"
#include "Object/native_Object.h"
#include "Number/native_Number.h"
#include "BigInt/native_BigInt.h"
#include "ArrayBuffer/native_ArrayBuffer.h"
#include "DataView/native_DataView.h"
#include "TypedArray/native_TypedArray.h"
#include "Array/native_Array.h"
#include "String/native_String.h"
#include "Promise/native_Promise.h"
#include "Console/native_Console.h"
#include "Error/native_Error.h"
#include "Map/native_Map.h"
#include "Set/native_Set.h"
#include "Symbol/native_Symbol.h"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_builtin_natives(vm_t* vm) {
	reg_native_Object(vm);
	reg_native_Error(vm);
	reg_native_Array(vm);
	reg_native_String(vm);
	reg_native_Console(vm);
	reg_native_Number(vm);
	reg_native_BigInt(vm);
	reg_native_ArrayBuffer(vm);
	reg_native_DataView(vm);
	reg_native_TypedArray(vm);
	reg_native_Promise(vm);
	reg_native_Map(vm);
	reg_native_Set(vm);
	reg_native_Symbol(vm);
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
