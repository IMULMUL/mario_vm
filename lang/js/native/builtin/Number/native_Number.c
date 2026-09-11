#ifdef __cplusplus
extern "C" {
#endif

#include "native_Number.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/** Number */

var_t* native_Number_constructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* this_v = NULL;
	var_t* v = get_obj(env, "value");
	if(v != NULL) {
		if(v->type == V_INT) {
			this_v = var_new_int(vm, var_get_int(v));
		}
		else if(v->type == V_FLOAT) {
			this_v = var_new_float(vm, var_get_float(v));
		}
	}

	if(this_v == NULL)
		this_v = var_new_int(vm, 0);
	var_instance_from(this_v, get_obj(env, THIS));
	return this_v;
}

var_t* native_Number_toString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char *s;

	var_t* v = get_obj(env, THIS);
	if(v->type == V_INT) {
		int radix = get_int(env, "radix");
		if(radix < 2 || radix > 36)
			radix = 10;
		s = mstr_from_int(var_get_int(v), radix);
	}
	else {
		s = mstr_from_float(var_get_float(v));
	}
	var_t* ret = var_new_str(vm, s);
	return ret;
}

/*===== ES6 Number statics =====*/

/* mario stores NaN as a V_FLOAT whose payload is not self-equal. */
static bool num_is_nan(var_t* v) {
	if(v == NULL || v->value == NULL || v->type != V_FLOAT)
		return false;
	float f = *(float*)v->value;
	return f != f;
}

/* A real number with no fractional part (ints always qualify; NaN/Inf never). */
static bool num_is_int(var_t* v) {
	if(v == NULL)
		return false;
	if(v->type == V_INT)
		return true;
	if(v->type == V_FLOAT) {
		float f = *(float*)v->value;
		if(f != f || isinf(f))
			return false;
		return f == floorf(f);
	}
	return false;
}

/* Number.isInteger(x): no coercion, so a non-number is never an integer. */
var_t* native_Number_isInteger(vm_t* vm, var_t* env, void* data) {
	(void)data;
	return var_new_bool(vm, num_is_int(get_func_arg(env, 0)));
}

/* Number.isFinite(x): a real number that is neither NaN nor Infinity. */
var_t* native_Number_isFinite(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_func_arg(env, 0);
	if(v == NULL)
		return var_new_bool(vm, false);
	if(v->type == V_INT)
		return var_new_bool(vm, true);
	if(v->type == V_FLOAT) {
		float f = *(float*)v->value;
		return var_new_bool(vm, (f == f) && !isinf(f));
	}
	return var_new_bool(vm, false);
}

/* Number.isNaN(x): true only for the NaN value itself; never coerces. */
var_t* native_Number_isNaN(vm_t* vm, var_t* env, void* data) {
	(void)data;
	return var_new_bool(vm, num_is_nan(get_func_arg(env, 0)));
}

/* Number.isSafeInteger(x): an integer within the safe magnitude. mario stores
 * every non-integer number as a 32-bit float, so 2**53-1 arrives as the float32
 * 9007199254740992.0f; comparing at that same precision keeps the boundary
 * inclusive because (float)9007199254740991.0 rounds to that exact value. */
var_t* native_Number_isSafeInteger(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_func_arg(env, 0);
	if(!num_is_int(v))
		return var_new_bool(vm, false);
	float af = fabsf(var_get_float(v));
	return var_new_bool(vm, af <= (float)9007199254740991.0);
}

/* Number.parseInt(s, radix): skip leading whitespace, honour a 0x prefix when no
 * radix is given, then strtol. Returns NaN when nothing was parsed. */
var_t* native_Number_parseInt(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_func_arg(env, 0);
	if(v == NULL)
		return var_new_float(vm, (float)NAN);
	mstr_t* s = mstr_new("");
	var_to_str(v, s);
	const char* p = s->cstr;
	while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' || *p == '\f')
		p++;
	int radix = 0;
	var_t* r = get_func_arg(env, 1);
	if(r != NULL && (r->type == V_INT || r->type == V_FLOAT))
		radix = var_get_int(r);
	if(radix == 0) {
		if(p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { radix = 16; p += 2; }
		else radix = 10;
	}
	char* end = NULL;
	long val = strtol(p, &end, radix);
	mstr_free(s);
	if(end == p)
		return var_new_float(vm, (float)NAN);
	return var_new_int(vm, (int)val);
}

/* Number.parseFloat(s): skip leading whitespace, then strtod. */
var_t* native_Number_parseFloat(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_func_arg(env, 0);
	if(v == NULL)
		return var_new_float(vm, (float)NAN);
	mstr_t* s = mstr_new("");
	var_to_str(v, s);
	const char* p = s->cstr;
	while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' || *p == '\f')
		p++;
	char* end = NULL;
	double val = strtod(p, &end);
	mstr_free(s);
	if(end == p)
		return var_new_float(vm, (float)NAN);
	return var_new_float(vm, (float)val);
}

#define CLS_NUMBER "Number"

void reg_native_Number(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_NUMBER);
	vm_reg_native(vm, cls, "toString(radix)", native_Number_toString, NULL); 
	vm_reg_native(vm, cls, "constructor(value)", native_Number_constructor, NULL); 

	/* ES6 static methods (live on the class prototype, like Math's statics). */
	vm_reg_static(vm, cls, "isInteger(a)", native_Number_isInteger, NULL);
	vm_reg_static(vm, cls, "isFinite(a)", native_Number_isFinite, NULL);
	vm_reg_static(vm, cls, "isNaN(a)", native_Number_isNaN, NULL);
	vm_reg_static(vm, cls, "isSafeInteger(a)", native_Number_isSafeInteger, NULL);
	vm_reg_static(vm, cls, "parseInt(s,radix)", native_Number_parseInt, NULL);
	vm_reg_static(vm, cls, "parseFloat(s)", native_Number_parseFloat, NULL);

	/* Constant values. MAX_VALUE/MIN_VALUE are omitted: a float32 cannot hold
	 * the IEEE-754 double extremes without overflowing to Infinity. */
	vm_reg_var(vm, cls, "EPSILON", var_new_float(vm, (float)2.220446049250313e-16), true);
	vm_reg_var(vm, cls, "MAX_SAFE_INTEGER", var_new_float(vm, (float)9007199254740991.0), true);
	vm_reg_var(vm, cls, "MIN_SAFE_INTEGER", var_new_float(vm, (float)-9007199254740991.0), true);
	vm_reg_var(vm, cls, "POSITIVE_INFINITY", var_new_float(vm, (float)INFINITY), true);
	vm_reg_var(vm, cls, "NEGATIVE_INFINITY", var_new_float(vm, (float)-INFINITY), true);
	vm_reg_var(vm, cls, "NaN", var_new_float(vm, (float)NAN), true);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
