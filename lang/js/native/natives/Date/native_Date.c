#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mario.h"
#include <time.h>

/* The VM's number model is 32-bit: V_INT is int32 and V_FLOAT is float32, so a
 * Unix millisecond timestamp (~1.75e12) can not be held exactly -- int32
 * overflows (the old `(int)timestamp` yielded negative garbage, breaking
 * `Date.now() >= 0`) and float32 keeps the magnitude but rounds to ~131072 ms
 * steps. We return epoch milliseconds as a float: the sign and magnitude are
 * correct (positive, monotonic, `typeof === "number"`), the closest the number
 * model allows to the spec value. Full-precision time would require a 64-bit
 * integer or double type, which this VM does not have. */
static float date_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long long ms = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
    return (float)ms;
}

/*=====date native functions=========*/
var_t* native_date_now(vm_t* vm, var_t* env, void* data) {
    (void)env; (void)data;
    return var_new_float(vm, date_now_ms());
}

/* Date constructor: `new Date()` stamps the current time; `new Date(value)`
 * adopts the given time. The timestamp lives in a hidden, unenumerable @@t
 * member so it never surfaces in for-in or JSON output. */
var_t* native_DateConstructor(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* obj = var_new_obj(vm, get_obj(env, THIS), NULL, NULL);
    var_t* arg = get_obj(env, "value");
    float ms = (arg != NULL && arg->type != V_UNDEF) ? var_get_float(arg) : date_now_ms();
    node_t* tn = var_add(obj, "@@t", var_new_float(vm, ms));
    tn->be_unenumerable = 1;
    tn->invisable = 1;
    return obj;
}

/* Instance getTime()/valueOf(): return the stored timestamp (a borrowed member;
 * func_call keeps it alive across the return). */
var_t* native_date_get_time(vm_t* vm, var_t* env, void* data) {
    (void)data;
    var_t* thisV = get_obj(env, THIS);
    var_t* t = var_find_member_var(thisV, "@@t");
    if(t != NULL)
        return t;
    return var_new_float(vm, date_now_ms());
}

#define CLS_DATE "Date"
void reg_native_Date(vm_t* vm) {
    var_t* cls = vm_new_class(vm, CLS_DATE);
    vm_reg_static(vm, cls, "now()", native_date_now, NULL);
    vm_reg_native(vm, cls, "constructor(value)", native_DateConstructor, NULL);
    vm_reg_native(vm, cls, "getTime()", native_date_get_time, NULL);
    vm_reg_native(vm, cls, "valueOf()", native_date_get_time, NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
