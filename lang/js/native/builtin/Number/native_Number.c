#ifdef __cplusplus
extern "C" {
#endif

#include "native_Number.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>   /* snprintf for toFixed/toPrecision */
#include <float.h>   /* FLT_MAX/FLT_MIN for MAX_VALUE/MIN_VALUE */

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

/* Number.prototype.toFixed(digits): fixed-point decimal string with `digits`
 * places after the point. digits is clamped to [0,100] (JS throws RangeError
 * outside that range; we clamp instead). NaN/Infinity render as their JS strings.
 * The underlying value is a float32 in this VM. */
var_t* native_Number_toFixed(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_obj(env, THIS);
	double d = (double)var_get_float(v);
	int digits = get_int(env, "digits");
	if(digits < 0) digits = 0;
	if(digits > 100) digits = 100;
	char buf[160];
	if(isnan(d))
		snprintf(buf, sizeof(buf), "NaN");
	else if(isinf(d))
		snprintf(buf, sizeof(buf), "%s", d < 0 ? "-Infinity" : "Infinity");
	else
		snprintf(buf, sizeof(buf), "%.*f", digits, d);
	return var_new_str(vm, buf);
}

/* Number.prototype.toPrecision(precision): `precision` significant digits, in
 * fixed or exponential form as %g decides. With no argument it falls back to the
 * plain number->string form. precision is clamped to [1,100]. */
var_t* native_Number_toPrecision(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_obj(env, THIS);
	var_t* p = get_obj(env, "precision");
	if(p == NULL || p->type == V_UNDEF)
		return var_new_str(vm, mstr_from_float(var_get_float(v)));
	double d = (double)var_get_float(v);
	int prec = var_get_int(p);
	if(prec < 1) prec = 1;
	if(prec > 100) prec = 100;
	char buf[160];
	if(isnan(d))
		snprintf(buf, sizeof(buf), "NaN");
	else if(isinf(d))
		snprintf(buf, sizeof(buf), "%s", d < 0 ? "-Infinity" : "Infinity");
	else
		snprintf(buf, sizeof(buf), "%.*g", prec, d);
	return var_new_str(vm, buf);
}

/* Number.prototype.valueOf(): the primitive numeric value of this Number. */
var_t* native_Number_valueOf(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_obj(env, THIS);
	if(v == NULL)
		return var_new_int(vm, 0);
	if(v->type == V_FLOAT)
		return var_new_float(vm, var_get_float(v));
	return var_new_int(vm, var_get_int(v));
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

/* Global isNaN(x). Unlike Number.isNaN (which never coerces), the global form
 * first converts its argument to a number: isNaN("foo")===true, isNaN("12")===false,
 * isNaN(undefined)===true, isNaN(null)===false (Number(null) is 0), isNaN(NaN)===true.
 * Objects/arrays are treated as NaN here (full ToPrimitive is out of scope). */
var_t* native_global_isNaN(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* v = get_func_arg(env, 0);
	bool nan = true;
	if(v != NULL) {
		switch(v->type) {
			case V_INT:
			case V_BOOL:
			case V_NULL: /* Number(null) === 0 */
				nan = false;
				break;
			case V_FLOAT: {
				float f = *(float*)v->value;
				nan = (f != f);
			} break;
			case V_STRING: {
				const char* p = var_get_str(v);
				while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' || *p == '\f')
					p++;
				if(*p == 0) {
					nan = false; /* Number("") === 0 */
				}
				else {
					char* end = NULL;
					strtod(p, &end);
					nan = (end == p); /* nothing consumed -> NaN */
				}
			} break;
			default: /* V_UNDEF and objects -> NaN */
				nan = true;
				break;
		}
	}
	return var_new_bool(vm, nan);
}

#define CLS_NUMBER "Number"

void reg_native_Number(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_NUMBER);
	vm_reg_native(vm, cls, "toString(radix)", native_Number_toString, NULL); 
	vm_reg_native(vm, cls, "toFixed(digits)", native_Number_toFixed, NULL);
	vm_reg_native(vm, cls, "toPrecision(precision)", native_Number_toPrecision, NULL);
	vm_reg_native(vm, cls, "valueOf()", native_Number_valueOf, NULL);
	vm_reg_native(vm, cls, "constructor(value)", native_Number_constructor, NULL); 

	/* ES6 static methods (live on the class prototype, like Math's statics). */
	vm_reg_static(vm, cls, "isInteger(a)", native_Number_isInteger, NULL);
	vm_reg_static(vm, cls, "isFinite(a)", native_Number_isFinite, NULL);
	vm_reg_static(vm, cls, "isNaN(a)", native_Number_isNaN, NULL);
	vm_reg_static(vm, cls, "isSafeInteger(a)", native_Number_isSafeInteger, NULL);
	vm_reg_static(vm, cls, "parseInt(s,radix)", native_Number_parseInt, NULL);
	vm_reg_static(vm, cls, "parseFloat(s)", native_Number_parseFloat, NULL);

	/* Constant values. MAX_VALUE/MIN_VALUE reflect THIS VM's float32 number model
	 * (FLT_MAX / FLT_MIN), not JS's IEEE-754 double extremes: a float32 cannot
	 * hold 1.79e308 without overflowing to Infinity, and the user has accepted the
	 * 32-bit model. MIN_VALUE is the smallest positive normal float32, matching
	 * JS's "smallest positive value" meaning (not "most negative"). */
	vm_reg_var(vm, cls, "MAX_VALUE", var_new_float(vm, (float)FLT_MAX), true);
	vm_reg_var(vm, cls, "MIN_VALUE", var_new_float(vm, (float)FLT_MIN), true);
	vm_reg_var(vm, cls, "EPSILON", var_new_float(vm, (float)2.220446049250313e-16), true);
	vm_reg_var(vm, cls, "MAX_SAFE_INTEGER", var_new_float(vm, (float)9007199254740991.0), true);
	vm_reg_var(vm, cls, "MIN_SAFE_INTEGER", var_new_float(vm, (float)-9007199254740991.0), true);
	vm_reg_var(vm, cls, "POSITIVE_INFINITY", var_new_float(vm, (float)INFINITY), true);
	vm_reg_var(vm, cls, "NEGATIVE_INFINITY", var_new_float(vm, (float)-INFINITY), true);
	vm_reg_var(vm, cls, "NaN", var_new_float(vm, (float)NAN), true);

	/* Global (free) functions on the root object. parseInt/parseFloat share the
	 * Number.* semantics; the global isNaN coerces its argument (see above). */
	vm_reg_native(vm, NULL, "parseInt(s, radix)", native_Number_parseInt, NULL);
	vm_reg_native(vm, NULL, "parseFloat(s)", native_Number_parseFloat, NULL);
	vm_reg_native(vm, NULL, "isNaN(x)", native_global_isNaN, NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
