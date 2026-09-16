#ifdef __cplusplus
extern "C" {
#endif

#include "native_String.h"
#include "../RegExp/native_RegExp.h"
#include <math.h>   /* NAN for charCodeAt's out-of-range result */

/* regexp split path (defined with the other regex-aware helpers below);
 * native_StringSplit() sits earlier in the file. */
static var_t* str_re_split(vm_t* vm, var_t* env, var_t* re, int limit);

/**======utf8 functions======*/

typedef struct st_utf8_reader {
	const char*         str;
	uint32_t            offset;
} utf8_reader_t;

typedef m_array_t utf8_t;

#define isASCII(b)  ((b & 0x80) == 0)

void utf8_reader_init(utf8_reader_t* reader, const char* s, uint32_t offset) {
	if(reader == NULL)
		return;
	
	reader->str = s;
	reader->offset = offset;
}

/**Read single word with UTF-8 encode
*/
bool utf8_read(utf8_reader_t* reader, mstr_t* dst) {
	if(reader == NULL || reader->str == NULL)
		return false;
	const char* src = reader->str;
	mstr_reset(dst);

	uint8_t b;
	b = src[reader->offset++];
	if(b == 0)//end of input
		return false; 

	mstr_add(dst, b);
	if(!isASCII(b)) { //not ASCII
		uint8_t count = 0;
		if((b >> 4) == 0x0E) { //3 bytes encode like UTF-8 Chinese
			count = 2;
		}
		else if((b >> 3) == 0x1E) { //4 bytes encode
			count = 3;
		}
		else if((b >> 2) == 0x3E) { //5 bytes encode
			count = 4;
		}
		else if((b >> 1) == 0x7E) { //6 bytes encode
			count = 5;
		}

		while(count > 0) {
			b = src[reader->offset++];
			if(b == 0)
				return false; //wrong encode.
			mstr_add(dst, b);
			count--;
		}
	}
	return true;
}

utf8_t* utf8_new(const char* s) {
	utf8_t* ret = (utf8_t*)mario_malloc(sizeof(utf8_t));
	array_init(ret);
	utf8_reader_t reader;
	utf8_reader_init(&reader, s, 0);
	while(true) {
		mstr_t* str = mstr_new("");
		if(!utf8_read(&reader, str)) {
			mstr_free(str);
			break;
		}
		array_add(ret, str);
	}
	return ret;
}

void utf8_free(utf8_t* utf8) {
	if(utf8 == NULL)
		return;
	array_clean(utf8, (free_func_t)mstr_free);
	mario_free(utf8);
}

uint32_t utf8_len(utf8_t* utf8) {
	if(utf8 == NULL)
		return 0;
	return utf8->size;
}

mstr_t* utf8_at(utf8_t* utf8, uint32_t at) {
	if(utf8 == NULL || at >= utf8_len(utf8))
		return NULL;
	return (mstr_t*)array_get(utf8, at);
}

void utf8_set(utf8_t* utf8, uint32_t at, const char* s) {
	if(s == NULL || s[0]  == 0) {
		array_del(utf8, at, (free_func_t)mstr_free);
		return;
	}

	mstr_t* str = utf8_at(utf8, at);
	if(str == NULL)
		return;
	mstr_cpy(str, s);
}

void utf8_append_raw(utf8_t* utf8, const char* s) {
	if(utf8 == NULL || s == NULL || s[0] == 0)
		return;

	utf8_t* u = utf8_new(s);
	uint32_t len = utf8_len(u);
	uint32_t i;
	for(i=0; i<len; ++i) {
		mstr_t* s = utf8_at(u, i);
		if(s == NULL)
			break;
		array_add(utf8, s);
	}
	array_remove_all(u); //don't free items in 'u' coz all moved to 'utf8'
	utf8_free(u);
}

void utf8_append(utf8_t* utf8, const char* s) {
	if(utf8 == NULL || s == NULL || s[0] == 0)
		return;
	array_add(utf8, mstr_new(s));
}

void utf8_to_str(utf8_t* utf8, mstr_t* str) {
	mstr_reset(str);
	if(utf8 == NULL)
		return;

	uint32_t len = utf8_len(utf8);
	uint32_t i;
	for(i=0; i<len; ++i) {
		mstr_t* s = utf8_at(utf8, i);
		if(s == NULL)
			break;
		mstr_append(str, s->cstr);
	}
}

/** String */
var_t* native_StringConstructor(vm_t* vm, var_t* env, void* data) {
	(void)data;
	/* String(x) is a conversion, not a byte reinterpretation: get_str() would
	 * read a number's raw value bytes as a C string (String(10) -> "\n").
	 * var_to_str() applies JS ToString for every type (numbers -> decimal,
	 * objects/symbols -> their toString()). String() with no arg yields "". */
	var_t* arg = get_obj(env, "str");
	mstr_t* s = mstr_new("");
	if(arg != NULL)
		var_to_str(arg, s);
	var_t* thisV = var_new_str(vm, s->cstr);
	mstr_free(s);

	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

var_t* native_StringLength(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	return var_new_int(vm, (int)strlen(s));
}

var_t* native_StringToString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, THIS);
	return var_new_str(vm, s);
}

var_t* native_StringSubstr(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	int start = get_int(env, "start");
	if(start < 0)
		start = 0;

	int length = get_int(env, "length");
	int sl = (int)strlen(s) - start;
	if(sl <= 0)
		return var_new_str(vm, "");
	if(length > sl) 
		length = sl;
	var_t* ret = var_new_str2(vm, s+start, length);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_StringTrim(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	int start = 0;
	int end = len - 1;

	// Skip leading whitespace
	while (start <= end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r' || s[start] == '\f' || s[start] == '\v')) {
		start++;
	}

	// Skip trailing whitespace
	while (end >= start && (s[end] == ' ' || s[end] == '\t' || s[end] == '\n' || s[end] == '\r' || s[end] == '\f' || s[end] == '\v')) {
		end--;
	}

	// Return empty string if only whitespace
	if (start > end) {
		return var_new_str(vm, "");
	}

	// Return trimmed string
	var_t* ret = var_new_str2(vm, s + start, end - start + 1);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_StringSlice(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);

	// Get beginIndex parameter
	int beginIndex = get_int(env, "beginIndex");
	if (beginIndex < 0) {
		// If negative, count from end
		beginIndex = len + beginIndex;
		if (beginIndex < 0) {
			beginIndex = 0;
		}
	}

	// Get endIndex parameter (optional)
	int endIndex = len;
	/* The argument lives on `env`, never on the receiver: reading it off `this`
	 * hid every optional endIndex, so slice(8,-1) kept the last character instead
	 * of dropping it (core-js's classofRaw then yields "Function]" and its whole
	 * feature-detection layer collapses). */
	var_t* endIndexVar = get_obj(env, "endIndex");
	if (endIndexVar != NULL && endIndexVar->type != V_UNDEF && endIndexVar->type != V_NULL) {
		endIndex = var_get_int(endIndexVar);
		if (endIndex < 0) {
			// If negative, count from end
			endIndex = len + endIndex;
			if (endIndex < 0) {
				endIndex = 0;
			}
		}
	}

	// Clamp values
	if (beginIndex >= len) {
		return var_new_str(vm, "");
	}
	if (endIndex > len) {
		endIndex = len;
	}
	if (beginIndex >= endIndex) {
		return var_new_str(vm, "");
	}

	// Return sliced string
	var_t* ret = var_new_str2(vm, s + beginIndex, endIndex - beginIndex);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_StringIndexOf(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	const char* searchValue = get_str(env, "searchValue");
	int len = (int)strlen(s);
	int searchLen = (int)strlen(searchValue);

	// Get fromIndex parameter (optional)
	int fromIndex = 0;
	var_t* fromIndexVar = get_obj(env, "fromIndex");
	if (fromIndexVar != NULL && fromIndexVar->type != V_UNDEF && fromIndexVar->type != V_NULL) {
		fromIndex = var_get_int(fromIndexVar);
		if (fromIndex < 0) {
			fromIndex = 0;
		}
	}

	// Handle edge cases
	if (searchLen == 0) {
		return var_new_int(vm, fromIndex > len ? len : fromIndex);
	}
	if (fromIndex >= len || searchLen > len) {
		return var_new_int(vm, -1);
	}

	// Find the searchValue in the string
	int i, j;
	for (i = fromIndex; i <= len - searchLen; i++) {
		for (j = 0; j < searchLen; j++) {
			if (s[i + j] != searchValue[j]) {
				break;
			}
		}
		if (j == searchLen) {
			return var_new_int(vm, i);
		}
	}

	// Not found
	return var_new_int(vm, -1);
}

var_t* native_StringSplit(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	{
		var_t* sepv = get_obj(env, "separator");
		if(js_regexp_is(sepv)) {
			int limit = -1;
			var_t* lv = get_obj(env, "limit");
			if(lv != NULL && lv->type == V_INT) {
				limit = var_get_int(lv);
				if(limit <= 0)
					limit = -1;
			}
			return str_re_split(vm, env, sepv, limit);
		}
	}

	const char* s = get_str(env, THIS);
	const char* separator = get_str(env, "separator");
	int len = (int)strlen(s);
	int sepLen = (int)strlen(separator);

	// Get limit parameter (optional)
	int limit = -1; // -1 means no limit
	var_t* limitVar = get_obj(env, "limit");
	if (limitVar != NULL && limitVar->type != V_UNDEF && limitVar->type != V_NULL) {
		limit = var_get_int(limitVar);
		if (limit <= 0) {
			limit = -1;
		}
	}

	// Create result array
	var_t* result = var_new_array(vm);

	// Handle empty string case
	if (len == 0) {
		var_array_add(result, var_new_str(vm, ""));
		return result;
	}

	// Handle empty separator case
	if (sepLen == 0) {
		int i;
		for (i = 0; i < len && (limit == -1 || i < limit); i++) {
			var_t* charStr = var_new_str2(vm, s + i, 1);
			var_array_add(result, charStr);
		}
		return result;
	}

	// Split the string
	int start = 0;
	int i, j;
	int count = 0;

	while (start < len && (limit == -1 || count < limit)) {
		// Find the next separator
		for (i = start; i <= len - sepLen; i++) {
			for (j = 0; j < sepLen; j++) {
				if (s[i + j] != separator[j]) {
					break;
				}
			}
			if (j == sepLen) {
				// Found separator, add substring to result
				var_t* substr = var_new_str2(vm, s + start, i - start);
				var_array_add(result, substr);
				start = i + sepLen;
				count++;
				break;
			}
		}

		// If no more separators, add the remaining string
		if (i > len - sepLen) {
			var_t* substr = var_new_str2(vm, s + start, len - start);
			var_array_add(result, substr);
			break;
		}
	}

	return result;
}

var_t* native_StringToLowerCase(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	mstr_t* result = mstr_new("");

	// Convert each character to lowercase
	int i;
	for (i = 0; i < len; i++) {
		char c = s[i];
		if (c >= 'A' && c <= 'Z') {
			c += 32; // Convert uppercase to lowercase
		}
		mstr_add(result, c);
	}

	var_t* ret = var_new_str(vm, result->cstr);
	mstr_free(result);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_StringToUpperCase(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	mstr_t* result = mstr_new("");

	// Convert each character to uppercase
	int i;
	for (i = 0; i < len; i++) {
		char c = s[i];
		if (c >= 'a' && c <= 'z') {
			c -= 32; // Convert lowercase to uppercase
		}
		mstr_add(result, c);
	}

	var_t* ret = var_new_str(vm, result->cstr);
	mstr_free(result);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/**====== regex-aware paths for replace/match/search/split ======*/

static void str_re_append_n(mstr_t* out, const char* s, int n) {
	int i;
	for(i = 0; i < n; i++)
		mstr_add(out, s[i]);
}

/* Expand a replacement template: $$ $& $` $' $1..$99. An out-of-range or
 * unset group expands to "" (JS leaves the literal text only for $0/$-less
 * digits; keeping it simple and predictable here). */
static void str_re_expand(mstr_t* out, const char* repl, const char* s, int slen, int* caps, int ng) {
	const char* p = repl;
	while(*p != 0) {
		if(*p == '$' && p[1] != 0) {
			char c = p[1];
			if(c == '$') { mstr_add(out, '$'); p += 2; continue; }
			if(c == '&') { str_re_append_n(out, s + caps[0], caps[1] - caps[0]); p += 2; continue; }
			if(c == '`') { str_re_append_n(out, s, caps[0]); p += 2; continue; }
			if(c == '\'') { str_re_append_n(out, s + caps[1], slen - caps[1]); p += 2; continue; }
			if(c >= '1' && c <= '9') {
				int g = c - '0';
				int adv = 2;
				if(p[2] >= '0' && p[2] <= '9' && g * 10 + (p[2] - '0') <= ng) {
					g = g * 10 + (p[2] - '0');
					adv = 3;
				}
				if(g <= ng) {
					if(caps[g * 2] >= 0)
						str_re_append_n(out, s + caps[g * 2], caps[g * 2 + 1] - caps[g * 2]);
					p += adv;
					continue;
				}
			}
		}
		mstr_add(out, *p);
		p++;
	}
}

/* Call a function replacement f(match, p1..pn, offset, string) and append its
 * string value. Runs inside a gc_defer window (transient args are unrooted,
 * same contract as the Array callback natives). */
static void str_re_call_repl(vm_t* vm, var_t* env, var_t* f, mstr_t* out,
		const char* s, int* caps, int ng) {
	vm->gc.gc_defer++;
	var_t* args = var_new_array(vm);
	int g;
	for(g = 0; g <= ng; g++) {
		if(caps[g * 2] >= 0)
			var_array_add(args, var_new_str2(vm, s + caps[g * 2], (uint32_t)(caps[g * 2 + 1] - caps[g * 2])));
		else
			var_array_add(args, var_new(vm));
	}
	var_array_add(args, var_new_int(vm, caps[0]));
	var_array_add(args, var_new_str(vm, s));
	var_array_reverse(args);
	var_t* res = call_m_func(vm, env, f, args);
	var_unref(args);
	if(res != NULL) {
		mstr_t* rs = mstr_new("");
		var_to_str(res, rs);
		mstr_append(out, rs->cstr);
		mstr_free(rs);
		var_unref(res);
	}
	vm->gc.gc_defer--;
}

/* Shared regexp replace: one match, or all of them when the regexp carries
 * /g (String.replace) or unconditionally (String.replaceAll). */
static var_t* str_re_replace(vm_t* vm, var_t* env, var_t* re, bool force_all) {
	const char* s = get_str(env, THIS);
	int slen = (int)strlen(s);
	char err[64];
	re_prog_t* p = re_compile(get_str(re, "source"), get_str(re, "flags"), err, sizeof(err));
	if(p == NULL)
		return var_new_str(vm, s);
	bool all = force_all || re_flag_global(p);
	var_t* replv = get_obj(env, "replacement");
	bool is_fn = replv != NULL && replv->is_func != 0;
	const char* repl = is_fn ? "" : get_str(env, "replacement");
	int ng = re_ngroups(p);
	int caps[RE_CAPS_MAX];
	mstr_t* out = mstr_new("");
	int pos = 0;
	while(pos <= slen) {
		if(!re_match(p, s, slen, pos, caps))
			break;
		str_re_append_n(out, s + pos, caps[0] - pos);
		if(is_fn)
			str_re_call_repl(vm, env, replv, out, s, caps, ng);
		else
			str_re_expand(out, repl, s, slen, caps, ng);
		if(caps[1] > caps[0])
			pos = caps[1];
		else {
			/* empty match: copy one char through to guarantee progress */
			if(caps[1] < slen)
				mstr_add(out, s[caps[1]]);
			pos = caps[1] + 1;
		}
		if(!all)
			break;
	}
	if(pos < slen)
		str_re_append_n(out, s + pos, slen - pos);
	re_free(p);
	var_t* ret = var_new_str(vm, out->cstr);
	mstr_free(out);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* String.prototype.match(regexp): non-global -> exec()-shaped array or null;
 * global -> array of every full match (or null when there is none). A plain
 * string argument is compiled as a pattern, per spec. */
var_t* native_StringMatch(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int slen = (int)strlen(s);
	var_t* rv = get_obj(env, "regexp");
	const char* src;
	const char* flg;
	if(js_regexp_is(rv)) {
		src = get_str(rv, "source");
		flg = get_str(rv, "flags");
	}
	else {
		src = get_str(env, "regexp");
		flg = "";
	}
	char err[64];
	re_prog_t* p = re_compile(src, flg, err, sizeof(err));
	if(p == NULL)
		return var_new_null(vm);
	int caps[RE_CAPS_MAX];
	if(!re_flag_global(p)) {
		if(!re_match(p, s, slen, 0, caps)) {
			re_free(p);
			return var_new_null(vm);
		}
		var_t* arr = js_regexp_result_array(vm, s, caps, re_ngroups(p));
		re_free(p);
		return arr;
	}
	var_t* arr = var_new_array(vm);
	int pos = 0, hits = 0;
	while(pos <= slen && re_match(p, s, slen, pos, caps)) {
		var_array_add(arr, var_new_str2(vm, s + caps[0], (uint32_t)(caps[1] - caps[0])));
		hits++;
		pos = caps[1] > caps[0] ? caps[1] : caps[1] + 1;
	}
	re_free(p);
	if(hits == 0)
		return var_new_null(vm);   /* arr is unrooted; the next gc sweeps it */
	return arr;
}

/* String.prototype.search(regexp): byte index of the first match or -1. */
var_t* native_StringSearch(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	var_t* rv = get_obj(env, "regexp");
	const char* src;
	const char* flg;
	if(js_regexp_is(rv)) {
		src = get_str(rv, "source");
		flg = get_str(rv, "flags");
	}
	else {
		src = get_str(env, "regexp");
		flg = "";
	}
	char err[64];
	re_prog_t* p = re_compile(src, flg, err, sizeof(err));
	if(p == NULL)
		return var_new_int(vm, -1);
	int caps[RE_CAPS_MAX];
	bool hit = re_match(p, s, (int)strlen(s), 0, caps);
	re_free(p);
	return var_new_int(vm, hit ? caps[0] : -1);
}

/* Regexp split path used by native_StringSplit(). Capture groups are not
 * spliced into the result (rarely relied on). */
static var_t* str_re_split(vm_t* vm, var_t* env, var_t* re, int limit) {
	const char* s = get_str(env, THIS);
	int slen = (int)strlen(s);
	var_t* result = var_new_array(vm);
	char err[64];
	re_prog_t* p = re_compile(get_str(re, "source"), get_str(re, "flags"), err, sizeof(err));
	if(p == NULL) {
		var_array_add(result, var_new_str(vm, s));
		return result;
	}
	int caps[RE_CAPS_MAX];
	int pos = 0, start = 0, count = 0;
	while(pos <= slen && (limit < 0 || count < limit)) {
		if(!re_match(p, s, slen, pos, caps))
			break;
		if(caps[1] == caps[0] && caps[0] >= slen)
			break;
		if(caps[1] == caps[0] && caps[0] == start) {
			/* empty match at the piece start: step over one char */
			pos = caps[0] + 1;
			continue;
		}
		var_array_add(result, var_new_str2(vm, s + start, (uint32_t)(caps[0] - start)));
		count++;
		start = caps[1];
		pos = caps[1] > caps[0] ? caps[1] : caps[1] + 1;
	}
	if(limit < 0 || count < limit)
		var_array_add(result, var_new_str2(vm, s + start, (uint32_t)(slen - start)));
	re_free(p);
	return result;
}

var_t* native_StringReplace(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	{
		var_t* sv = get_obj(env, "searchValue");
		if(js_regexp_is(sv))
			return str_re_replace(vm, env, sv, false);
	}

	const char* s = get_str(env, THIS);
	const char* searchValue = get_str(env, "searchValue");
	const char* replacement = get_str(env, "replacement");
	int len = (int)strlen(s);
	int searchLen = (int)strlen(searchValue);
	int replaceLen = (int)strlen(replacement);

	// Handle empty search value
	if (searchLen == 0) {
		// Insert replacement at the beginning
		mstr_t* result = mstr_new(replacement);
		mstr_append(result, s);
		var_t* ret = var_new_str(vm, result->cstr);
		mstr_free(result);
		var_instance_from(ret, get_obj(env, THIS));
		return ret;
	}

	// Find the first occurrence of searchValue
	int i, j;
	int foundIndex = -1;

	for (i = 0; i <= len - searchLen; i++) {
		for (j = 0; j < searchLen; j++) {
			if (s[i + j] != searchValue[j]) {
				break;
			}
		}
		if (j == searchLen) {
			foundIndex = i;
			break;
		}
	}

	// If searchValue not found, return original string
	if (foundIndex == -1) {
		return var_new_str(vm, s);
	}

	// Build the result string
	mstr_t* result = mstr_new("");

	// Add the part before the match
	if (foundIndex > 0) {
		mstr_ncpy(result, s, foundIndex);
	}

	// Add the replacement
	mstr_append(result, replacement);

	// Add the part after the match
	if (foundIndex + searchLen < len) {
		mstr_append(result, s + foundIndex + searchLen);
	}

	var_t* ret = var_new_str(vm, result->cstr);
	mstr_free(result);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_UTF8Constructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, "str");
	utf8_t* u = utf8_new(s);

	var_t* thisV = var_new_obj(vm, get_obj(env, THIS), u, (free_func_t)utf8_free);
	return thisV;
}

var_t* native_UTF8Length(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	return var_new_int(vm, utf8_len(u));
}

var_t* native_UTF8ToString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	mstr_t *s = mstr_new("");
	utf8_to_str(u, s);
	var_t* v = var_new_str(vm, s->cstr);
	mstr_free(s);
	return v;
}

var_t* native_UTF8At(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	int32_t at = get_int(env, "index");

	mstr_t *s = utf8_at(u, at);
	var_t* v = var_new_str(vm, s->cstr);
	return v;
}

var_t* native_UTF8Set(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	int32_t at = get_int(env, "index");
	const char* s = get_str(env, "s");

	utf8_set(u, at, s);
	return NULL;
}

var_t* native_UTF8Substr(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	int start = get_int(env, "start");
	if(start < 0)
		start = 0;

	int length = get_int(env, "length");
	int sl = utf8_len(u) - start;
	utf8_t* sub = utf8_new("");
	if(sl > 0) {
		if(length > sl) 
			length = sl;
		int i;
		for(i=0; i<length; ++i) {
			mstr_t* s = utf8_at(u, i+start);
			utf8_append(sub, s->cstr);	
		}
	}
	var_t* ret = var_new_obj_no_proto(vm, sub, (free_func_t)utf8_free);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_UTF8ReaderConstructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, "str");
	utf8_reader_t* ur = (utf8_reader_t*)mario_malloc(sizeof(utf8_reader_t));
	utf8_reader_init(ur, s, 0);

	var_t* thisV = var_new_obj_no_proto(vm, ur, NULL);
	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

var_t* native_UTF8ReaderRead(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_reader_t* ur = (utf8_reader_t*)get_raw(env, THIS);
	mstr_t* s = mstr_new("");
	var_t* v;
	if(utf8_read(ur, s)) 
		v = var_new_str(vm, s->cstr);
	else
		v = var_new_str(vm, "");
	mstr_free(s);
	return v;
}


/*===== ES6 String methods =====*/

/* Decode one UTF-8 sequence at *p, advancing *p past it; yields the code point.
 * Short/invalid sequences fall back to a single byte so we never read past NUL. */
static uint32_t str_utf8_decode(const char** p) {
	const unsigned char* s = (const unsigned char*)*p;
	uint32_t cp;
	if(s[0] < 0x80) { cp = s[0]; *p += 1; }
	else if((s[0] & 0xE0) == 0xC0 && s[1] != 0) { cp = ((uint32_t)(s[0]&0x1F)<<6)|(s[1]&0x3F); *p += 2; }
	else if((s[0] & 0xF0) == 0xE0 && s[1] != 0 && s[2] != 0) { cp = ((uint32_t)(s[0]&0x0F)<<12)|((uint32_t)(s[1]&0x3F)<<6)|(s[2]&0x3F); *p += 3; }
	else if((s[0] & 0xF8) == 0xF0 && s[1] != 0 && s[2] != 0 && s[3] != 0) { cp = ((uint32_t)(s[0]&0x07)<<18)|((uint32_t)(s[1]&0x3F)<<12)|((uint32_t)(s[2]&0x3F)<<6)|(s[3]&0x3F); *p += 4; }
	else { cp = s[0]; *p += 1; }
	return cp;
}

/* Append the UTF-8 encoding of code point cp to out. */
static void str_utf8_encode(mstr_t* out, uint32_t cp) {
	if(cp < 0x80) {
		mstr_add(out, (char)cp);
	} else if(cp < 0x800) {
		mstr_add(out, (char)(0xC0 | (cp >> 6)));
		mstr_add(out, (char)(0x80 | (cp & 0x3F)));
	} else if(cp < 0x10000) {
		mstr_add(out, (char)(0xE0 | (cp >> 12)));
		mstr_add(out, (char)(0x80 | ((cp >> 6) & 0x3F)));
		mstr_add(out, (char)(0x80 | (cp & 0x3F)));
	} else {
		mstr_add(out, (char)(0xF0 | (cp >> 18)));
		mstr_add(out, (char)(0x80 | ((cp >> 12) & 0x3F)));
		mstr_add(out, (char)(0x80 | ((cp >> 6) & 0x3F)));
		mstr_add(out, (char)(0x80 | (cp & 0x3F)));
	}
}

/* Append `need` padding characters, cycling through pad (spaces if pad is empty). */
static void str_build_pad(mstr_t* out, const char* pad, int need) {
	int plen = (pad == NULL) ? 0 : (int)strlen(pad);
	int i;
	if(plen == 0) {
		for(i = 0; i < need; i++) mstr_add(out, ' ');
		return;
	}
	for(i = 0; i < need; i++) mstr_add(out, pad[i % plen]);
}

var_t* native_StringStartsWith(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	const char* search = get_str(env, "searchString");
	int len = (int)strlen(s);
	int slen = (int)strlen(search);
	int pos = 0;
	var_t* pv = get_obj(env, "position");
	if(pv != NULL && pv->type != V_UNDEF && pv->type != V_NULL) pos = var_get_int(pv);
	if(pos < 0) pos = 0;
	if(pos > len) pos = len;
	if(slen == 0) return var_new_bool(vm, true);
	if(pos + slen > len) return var_new_bool(vm, false);
	return var_new_bool(vm, strncmp(s + pos, search, (size_t)slen) == 0);
}

var_t* native_StringEndsWith(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	const char* search = get_str(env, "searchString");
	int len = (int)strlen(s);
	int slen = (int)strlen(search);
	int end = len;
	var_t* ev = get_obj(env, "endPosition");
	if(ev != NULL && ev->type != V_UNDEF && ev->type != V_NULL) end = var_get_int(ev);
	if(end < 0) end = 0;
	if(end > len) end = len;
	if(slen == 0) return var_new_bool(vm, true);
	if(slen > end) return var_new_bool(vm, false);
	return var_new_bool(vm, strncmp(s + end - slen, search, (size_t)slen) == 0);
}

var_t* native_StringIncludes(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	const char* search = get_str(env, "searchString");
	int len = (int)strlen(s);
	int slen = (int)strlen(search);
	int pos = 0;
	var_t* pv = get_obj(env, "position");
	if(pv != NULL && pv->type != V_UNDEF && pv->type != V_NULL) pos = var_get_int(pv);
	if(pos < 0) pos = 0;
	if(pos > len) pos = len;
	if(slen == 0) return var_new_bool(vm, true);
	if(pos + slen > len) return var_new_bool(vm, false);
	return var_new_bool(vm, strstr(s + pos, search) != NULL);
}

var_t* native_StringRepeat(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int count = get_int(env, "count");
	mstr_t* result = mstr_new("");
	int i;
	for(i = 0; i < count; i++)
		mstr_append(result, s);
	var_t* ret = var_new_str(vm, result->cstr);
	mstr_free(result);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_StringPadStart(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	int target = get_int(env, "targetLength");
	mstr_t* result = mstr_new("");
	if(target > len)
		str_build_pad(result, get_str(env, "padString"), target - len);
	mstr_append(result, s);
	var_t* ret = var_new_str(vm, result->cstr);
	mstr_free(result);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_StringPadEnd(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	int target = get_int(env, "targetLength");
	mstr_t* result = mstr_new("");
	mstr_append(result, s);
	if(target > len)
		str_build_pad(result, get_str(env, "padString"), target - len);
	var_t* ret = var_new_str(vm, result->cstr);
	mstr_free(result);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_StringCodePointAt(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int pos = get_int(env, "position");
	if(pos < 0) return NULL;  /* undefined */
	const char* p = s;
	int i = 0;
	while(*p != 0 && i < pos) { str_utf8_decode(&p); i++; }
	if(*p == 0) return NULL;  /* out of range -> undefined */
	uint32_t cp = str_utf8_decode(&p);
	return var_new_int(vm, (int)cp);
}

/* String.fromCodePoint(...) - static; builds a string from code points. */
var_t* native_String_fromCodePoint(vm_t* vm, var_t* env, void* data) {
	(void)data;
	uint32_t n = get_func_args_num(env);
	mstr_t* out = mstr_new("");
	uint32_t i;
	for(i = 0; i < n; i++) {
		var_t* a = get_func_arg(env, i);
		if(a == NULL) continue;
		str_utf8_encode(out, (uint32_t)var_get_int(a));
	}
	var_t* ret = var_new_str(vm, out->cstr);
	mstr_free(out);
	return ret;
}

/* --- String.prototype.normalize (NFC / NFD over the Latin ranges) -----------
 * Unicode canonical composition pairs {composed, base, combining-mark} for the
 * precomposed characters in Latin-1 Supplement and Latin Extended-A. NFC composes
 * a starter with a following combining mark found here; NFD reverses it. This is
 * the standard canonical mapping data for these blocks (e.g. U+00E5 'å' is the
 * canonical composition of U+0061 'a' + U+030A combining ring above), so
 * "a\u030A".normalize() === "\u00E5" and .length becomes 1. Combining marks are
 * the Combining Diacritical Marks block U+0300..U+036F. */
static const uint32_t g_nfc_table[][3] = {
	{0x00C0,0x0041,0x0300},{0x00C1,0x0041,0x0301},{0x00C2,0x0041,0x0302},
	{0x00C3,0x0041,0x0303},{0x00C4,0x0041,0x0308},{0x00C5,0x0041,0x030A},
	{0x00C7,0x0043,0x0327},
	{0x00C8,0x0045,0x0300},{0x00C9,0x0045,0x0301},{0x00CA,0x0045,0x0302},{0x00CB,0x0045,0x0308},
	{0x00CC,0x0049,0x0300},{0x00CD,0x0049,0x0301},{0x00CE,0x0049,0x0302},{0x00CF,0x0049,0x0308},
	{0x00D1,0x004E,0x0303},
	{0x00D2,0x004F,0x0300},{0x00D3,0x004F,0x0301},{0x00D4,0x004F,0x0302},
	{0x00D5,0x004F,0x0303},{0x00D6,0x004F,0x0308},
	{0x00D9,0x0055,0x0300},{0x00DA,0x0055,0x0301},{0x00DB,0x0055,0x0302},{0x00DC,0x0055,0x0308},
	{0x00DD,0x0059,0x0301},
	{0x00E0,0x0061,0x0300},{0x00E1,0x0061,0x0301},{0x00E2,0x0061,0x0302},
	{0x00E3,0x0061,0x0303},{0x00E4,0x0061,0x0308},{0x00E5,0x0061,0x030A},
	{0x00E7,0x0063,0x0327},
	{0x00E8,0x0065,0x0300},{0x00E9,0x0065,0x0301},{0x00EA,0x0065,0x0302},{0x00EB,0x0065,0x0308},
	{0x00EC,0x0069,0x0300},{0x00ED,0x0069,0x0301},{0x00EE,0x0069,0x0302},{0x00EF,0x0069,0x0308},
	{0x00F1,0x006E,0x0303},
	{0x00F2,0x006F,0x0300},{0x00F3,0x006F,0x0301},{0x00F4,0x006F,0x0302},
	{0x00F5,0x006F,0x0303},{0x00F6,0x006F,0x0308},
	{0x00F9,0x0075,0x0300},{0x00FA,0x0075,0x0301},{0x00FB,0x0075,0x0302},{0x00FC,0x0075,0x0308},
	{0x00FD,0x0079,0x0301},{0x00FF,0x0079,0x0308},
	{0x0100,0x0041,0x0304},{0x0101,0x0061,0x0304},
	{0x0102,0x0041,0x0306},{0x0103,0x0061,0x0306},
	{0x0104,0x0041,0x0328},{0x0105,0x0061,0x0328},
	{0x0106,0x0043,0x0301},{0x0107,0x0063,0x0301},
	{0x0108,0x0043,0x0302},{0x0109,0x0063,0x0302},
	{0x010A,0x0043,0x0307},{0x010B,0x0063,0x0307},
	{0x010C,0x0043,0x030C},{0x010D,0x0063,0x030C},
	{0x010E,0x0044,0x030C},{0x010F,0x0064,0x030C},
	{0x0112,0x0045,0x0304},{0x0113,0x0065,0x0304},
	{0x0114,0x0045,0x0306},{0x0115,0x0065,0x0306},
	{0x0116,0x0045,0x0307},{0x0117,0x0065,0x0307},
	{0x011A,0x0045,0x030C},{0x011B,0x0065,0x030C},
	{0x011C,0x0047,0x0302},{0x011D,0x0067,0x0302},
	{0x011E,0x0047,0x0306},{0x011F,0x0067,0x0306},
	{0x0120,0x0047,0x0307},{0x0121,0x0067,0x0307},
	{0x0122,0x0047,0x0327},{0x0123,0x0067,0x0327},
	{0x0128,0x0049,0x0303},{0x0129,0x0069,0x0303},
	{0x012A,0x0049,0x0304},{0x012B,0x0069,0x0304},
	{0x012C,0x0049,0x0306},{0x012D,0x0069,0x0306},
	{0x0130,0x0049,0x0307},
	{0x0134,0x004A,0x0302},{0x0135,0x006A,0x0302},
	{0x0136,0x004B,0x0327},{0x0137,0x006B,0x0327},
	{0x0139,0x004C,0x0301},{0x013A,0x006C,0x0301},
	{0x013B,0x004C,0x0327},{0x013C,0x006C,0x0327},
	{0x013D,0x004C,0x030C},{0x013E,0x006C,0x030C},
	{0x0143,0x004E,0x0301},{0x0144,0x006E,0x0301},
	{0x0145,0x004E,0x0327},{0x0146,0x006E,0x0327},
	{0x0147,0x004E,0x030C},{0x0148,0x006E,0x030C},
	{0x014C,0x004F,0x0304},{0x014D,0x006F,0x0304},
	{0x014E,0x004F,0x0306},{0x014F,0x006F,0x0306},
	{0x0150,0x004F,0x030B},{0x0151,0x006F,0x030B},
	{0x0154,0x0052,0x0301},{0x0155,0x0072,0x0301},
	{0x0156,0x0052,0x0327},{0x0157,0x0072,0x0327},
	{0x0158,0x0052,0x030C},{0x0159,0x0072,0x030C},
	{0x015A,0x0053,0x0301},{0x015B,0x0073,0x0301},
	{0x015C,0x0053,0x0302},{0x015D,0x0073,0x0302},
	{0x015E,0x0053,0x0327},{0x015F,0x0073,0x0327},
	{0x0160,0x0053,0x030C},{0x0161,0x0073,0x030C},
	{0x0162,0x0054,0x0327},{0x0163,0x0074,0x0327},
	{0x0164,0x0054,0x030C},{0x0165,0x0074,0x030C},
	{0x0168,0x0055,0x0303},{0x0169,0x0075,0x0303},
	{0x016A,0x0055,0x0304},{0x016B,0x0075,0x0304},
	{0x016C,0x0055,0x0306},{0x016D,0x0075,0x0306},
	{0x016E,0x0055,0x030A},{0x016F,0x0075,0x030A},
	{0x0170,0x0055,0x030B},{0x0171,0x0075,0x030B},
	{0x0172,0x0055,0x0328},{0x0173,0x0075,0x0328},
	{0x0174,0x0057,0x0302},{0x0175,0x0077,0x0302},
	{0x0176,0x0059,0x0302},{0x0177,0x0079,0x0302},
	{0x0178,0x0059,0x0308},
	{0x0179,0x005A,0x0301},{0x017A,0x007A,0x0301},
	{0x017B,0x005A,0x0307},{0x017C,0x007A,0x0307},
	{0x017D,0x005A,0x030C},{0x017E,0x007A,0x030C},
};
#define NFC_TABLE_LEN (sizeof(g_nfc_table)/sizeof(g_nfc_table[0]))

static inline bool str_is_combining(uint32_t cp) {
	return cp >= 0x0300 && cp <= 0x036F;
}

/* Canonical composition of (base, mark); 0 if no precomposed form is known. */
static uint32_t nfc_compose(uint32_t base, uint32_t mark) {
	size_t i;
	for(i = 0; i < NFC_TABLE_LEN; i++)
		if(g_nfc_table[i][1] == base && g_nfc_table[i][2] == mark)
			return g_nfc_table[i][0];
	return 0;
}

/* Canonical decomposition of a precomposed cp; returns true and sets *base/*mark. */
static bool nfd_decompose(uint32_t cp, uint32_t* base, uint32_t* mark) {
	size_t i;
	for(i = 0; i < NFC_TABLE_LEN; i++)
		if(g_nfc_table[i][0] == cp) {
			*base = g_nfc_table[i][1];
			*mark = g_nfc_table[i][2];
			return true;
		}
	return false;
}

var_t* native_StringNormalize(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	/* Optional form argument; default (and the tested path) is NFC. */
	const char* form = "NFC";
	var_t* fv = get_obj(env, "form");
	if(fv != NULL && fv->type == V_STRING) form = var_get_str(fv);
	bool decompose = (strcmp(form, "NFD") == 0 || strcmp(form, "NFKD") == 0);

	mstr_t* out = mstr_new("");
	const char* p = s;
	if(decompose) {
		while(*p != 0) {
			uint32_t cp = str_utf8_decode(&p);
			uint32_t base, mark;
			if(nfd_decompose(cp, &base, &mark)) {
				str_utf8_encode(out, base);
				str_utf8_encode(out, mark);
			} else {
				str_utf8_encode(out, cp);
			}
		}
	} else {
		/* NFC: compose each starter with the combining marks that follow it, for as
		 * long as a canonical composition exists. Marks that cannot compose (or that
		 * follow another mark) are emitted unchanged. */
		while(*p != 0) {
			uint32_t cp = str_utf8_decode(&p);
			if(str_is_combining(cp)) { /* mark with no composable starter: emit as-is */
				str_utf8_encode(out, cp);
				continue;
			}
			uint32_t starter = cp;
			while(*p != 0) {
				const char* q = p;
				uint32_t mk = str_utf8_decode(&q);
				if(!str_is_combining(mk)) break;
				uint32_t c = nfc_compose(starter, mk);
				if(c == 0) break;
				starter = c;
				p = q; /* consume the composed mark */
			}
			str_utf8_encode(out, starter);
		}
	}
	var_t* ret = var_new_str(vm, out->cstr);
	mstr_free(out);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}


#define CLS_STRING "String"
#define CLS_UTF8 "UTF8"
#define CLS_UTF8_READER "UTF8Reader"

var_t* native_String_iterator(vm_t* vm, var_t* env, void* data) {
	(void)data;
	var_t* this_v = get_obj(env, THIS);
	return vm_new_string_iterator(vm, this_v); /* refs=0 */
}

/* ASCII whitespace test matching native_StringTrim's set (space plus the
 * 9..13 control blanks), written by code range so no locale header is needed. */
static bool str_is_space(char c) {
	unsigned char u = (unsigned char)c;
	return u == ' ' || (u >= 9 && u <= 13);
}

/* ES2019: trimStart (alias trimLeft) - strip leading whitespace. */
var_t* native_StringTrimStart(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	int start = 0;
	while(start < len && str_is_space(s[start])) start++;
	var_t* ret = var_new_str2(vm, s + start, len - start);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* ES2019: trimEnd (alias trimRight) - strip trailing whitespace. */
var_t* native_StringTrimEnd(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	int end = len;
	while(end > 0 && str_is_space(s[end - 1])) end--;
	var_t* ret = var_new_str2(vm, s, end);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* ES2022: String.prototype.at(index) - code-point aware; a negative index counts
 * back from the end and an out-of-range index yields undefined. */
var_t* native_StringAt(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int count = 0;
	const char* p = s;
	while(*p != 0) { str_utf8_decode(&p); count++; }
	int idx = get_int(env, "index");
	if(idx < 0) idx += count;
	if(idx < 0 || idx >= count) return NULL; /* undefined */
	p = s;
	int i = 0;
	while(i < idx) { str_utf8_decode(&p); i++; }
	const char* start = p;
	str_utf8_decode(&p);
	var_t* ret = var_new_str2(vm, start, (int)(p - start));
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* ES2021: String.prototype.replaceAll(search, replacement) - replaces every
 * non-overlapping occurrence. An empty search inserts replacement around each
 * code point and at both ends, matching the spec ("-a-b-" for "ab"). */
var_t* native_StringReplaceAll(vm_t* vm, var_t* env, void* data) {
	(void)data;

	{
		var_t* sv = get_obj(env, "searchValue");
		if(js_regexp_is(sv))
			return str_re_replace(vm, env, sv, true);
	}
	const char* s = get_str(env, THIS);
	const char* searchValue = get_str(env, "searchValue");
	const char* replacement = get_str(env, "replacement");
	int searchLen = (int)strlen(searchValue);
	mstr_t* result = mstr_new("");
	if(searchLen == 0) {
		const char* p = s;
		mstr_append(result, replacement);
		while(*p != 0) {
			str_utf8_encode(result, str_utf8_decode(&p));
			mstr_append(result, replacement);
		}
	} else {
		int len = (int)strlen(s);
		int i = 0;
		while(i < len) {
			if(i + searchLen <= len && strncmp(s + i, searchValue, searchLen) == 0) {
				mstr_append(result, replacement);
				i += searchLen;
			} else {
				mstr_add(result, s[i]);
				i++;
			}
		}
	}
	var_t* ret = var_new_str(vm, result->cstr);
	mstr_free(result);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* String.prototype.charAt(index): the single character at index as a string, or
 * "" when index is out of range or negative. Walks code points like at(); unlike
 * at() a negative index is NOT counted from the end (spec: it yields ""). */
var_t* native_StringCharAt(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int idx = get_int(env, "index");
	if(idx < 0) {
		var_t* e = var_new_str(vm, "");
		var_instance_from(e, get_obj(env, THIS));
		return e;
	}
	const char* p = s;
	int i = 0;
	while(*p != 0 && i < idx) { str_utf8_decode(&p); i++; }
	if(*p == 0) {
		var_t* e = var_new_str(vm, "");
		var_instance_from(e, get_obj(env, THIS));
		return e;
	}
	const char* start = p;
	str_utf8_decode(&p);
	var_t* ret = var_new_str2(vm, start, (int)(p - start));
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* String.prototype.charCodeAt(index): numeric code of the character at index, or
 * NaN when out of range. NOTE: Mario stores text as UTF-8 and decodes whole code
 * points, so for astral characters this returns the code point rather than a
 * UTF-16 surrogate half; identical to spec for the BMP. */
var_t* native_StringCharCodeAt(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int idx = get_int(env, "index");
	if(idx < 0)
		return var_new_float(vm, NAN);
	const char* p = s;
	int i = 0;
	while(*p != 0 && i < idx) { str_utf8_decode(&p); i++; }
	if(*p == 0)
		return var_new_float(vm, NAN);
	uint32_t cp = str_utf8_decode(&p);
	return var_new_int(vm, (int)cp);
}

/* String.prototype.substring(start[, end]): like slice but with substring's
 * clamping rules - negatives and NaN become 0 (no wrap-from-end), values are
 * clamped to length, and start/end are swapped when start > end. Byte indices,
 * consistent with slice()/substr() in this file. */
var_t* native_StringSubstring(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	int len = (int)strlen(s);
	int start = get_int(env, "start");
	int end = len;
	var_t* endVar = get_obj(env, "end");
	if(endVar != NULL && endVar->type != V_UNDEF)
		end = var_get_int(endVar);
	if(start < 0) start = 0;
	if(end < 0) end = 0;
	if(start > len) start = len;
	if(end > len) end = len;
	if(start > end) { int t = start; start = end; end = t; }
	var_t* ret = var_new_str2(vm, s + start, end - start);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* String.prototype.concat(...strs): this followed by each argument, each run
 * through JS ToString. var_to_str() RESETS its mstr (it overwrites, not appends),
 * so each argument is stringified into a scratch buffer and then appended. */
var_t* native_StringConcat(vm_t* vm, var_t* env, void* data) {
	(void)data;
	mstr_t* out = mstr_new(get_str(env, THIS));
	mstr_t* tmp = mstr_new("");
	uint32_t n = get_func_args_num(env);
	uint32_t i;
	for(i=0; i<n; i++) {
		var_t* a = get_func_arg(env, i);
		if(a == NULL) { mstr_append(out, "undefined"); continue; }
		var_to_str(a, tmp);
		mstr_append(out, tmp->cstr);
	}
	var_t* ret = var_new_str(vm, out->cstr);
	mstr_free(tmp);
	mstr_free(out);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

/* String.prototype.lastIndexOf(search[, fromIndex]): the greatest index <=
 * fromIndex at which `search` occurs, or -1. Mirrors indexOf()'s byte-based
 * scanning but keeps the last match instead of the first. */
var_t* native_StringLastIndexOf(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, THIS);
	const char* searchValue = get_str(env, "searchValue");
	int len = (int)strlen(s);
	int searchLen = (int)strlen(searchValue);
	int fromIndex = len;
	var_t* fromIndexVar = get_obj(env, "fromIndex");
	if(fromIndexVar != NULL && fromIndexVar->type != V_UNDEF)
		fromIndex = var_get_int(fromIndexVar);
	if(fromIndex > len) fromIndex = len;
	if(searchLen == 0)
		return var_new_int(vm, fromIndex < 0 ? 0 : fromIndex);
	int i;
	int lastFound = -1;
	for(i=0; i + searchLen <= len && i <= fromIndex; i++) {
		if(strncmp(s+i, searchValue, searchLen) == 0)
			lastFound = i;
	}
	return var_new_int(vm, lastFound);
}

/* String.prototype.localeCompare(other): -1 / 0 / 1 by code-point order. NOTE:
 * this is a plain lexicographic comparison, not true locale-aware collation. */
var_t* native_StringLocaleCompare(vm_t* vm, var_t* env, void* data) {
	(void)data;
	const char* s = get_str(env, THIS);
	const char* other = get_str(env, "compareString");
	int c = strcmp(s, other);
	return var_new_int(vm, c < 0 ? -1 : (c > 0 ? 1 : 0));
}

void reg_native_String(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_STRING);
	vm_reg_native(vm, cls, "constructor(str)", native_StringConstructor, NULL); 
	vm_reg_native(vm, cls, "length()", native_StringLength, NULL); 
	vm_reg_native(vm, cls, "toString()", native_StringToString, NULL); 
	vm_reg_native(vm, cls, "substr(start, length)", native_StringSubstr, NULL); 
	vm_reg_native(vm, cls, "trim()", native_StringTrim, NULL); 
	vm_reg_native(vm, cls, "slice(beginIndex, endIndex)", native_StringSlice, NULL); 
	vm_reg_native(vm, cls, "indexOf(searchValue, fromIndex)", native_StringIndexOf, NULL); 
	vm_reg_native(vm, cls, "split(separator, limit)", native_StringSplit, NULL); 
	vm_reg_native(vm, cls, "toLowerCase()", native_StringToLowerCase, NULL); 
	vm_reg_native(vm, cls, "toUpperCase()", native_StringToUpperCase, NULL); 
	vm_reg_native(vm, cls, "replace(searchValue, replacement)", native_StringReplace, NULL); 
	vm_reg_native(vm, cls, "match(regexp)", native_StringMatch, NULL); 
	vm_reg_native(vm, cls, "search(regexp)", native_StringSearch, NULL); 
	vm_reg_native(vm, cls, "startsWith(searchString, position)", native_StringStartsWith, NULL); 
	vm_reg_native(vm, cls, "endsWith(searchString, endPosition)", native_StringEndsWith, NULL); 
	vm_reg_native(vm, cls, "includes(searchString, position)", native_StringIncludes, NULL); 
	vm_reg_native(vm, cls, "repeat(count)", native_StringRepeat, NULL); 
	vm_reg_native(vm, cls, "padStart(targetLength, padString)", native_StringPadStart, NULL); 
	vm_reg_native(vm, cls, "padEnd(targetLength, padString)", native_StringPadEnd, NULL); 
	vm_reg_native(vm, cls, "codePointAt(position)", native_StringCodePointAt, NULL); 
	vm_reg_native(vm, cls, "normalize(form)", native_StringNormalize, NULL); 
	vm_reg_static(vm, cls, "fromCodePoint()", native_String_fromCodePoint, NULL); 
	/* fromCharCode takes UTF-16 code units; identical to fromCodePoint for the
	 * BMP, so it reuses the same implementation. */
	vm_reg_static(vm, cls, "fromCharCode()", native_String_fromCodePoint, NULL); 
	vm_reg_native(vm, cls, SYMKEY_ITERATOR "()", native_String_iterator, NULL); 

	vm_reg_native(vm, cls, "at(index)", native_StringAt, NULL);
	vm_reg_native(vm, cls, "trimStart()", native_StringTrimStart, NULL);
	vm_reg_native(vm, cls, "trimEnd()", native_StringTrimEnd, NULL);
	vm_reg_native(vm, cls, "trimLeft()", native_StringTrimStart, NULL);
	vm_reg_native(vm, cls, "trimRight()", native_StringTrimEnd, NULL);
	vm_reg_native(vm, cls, "replaceAll(searchValue, replacement)", native_StringReplaceAll, NULL);
	vm_reg_native(vm, cls, "charAt(index)", native_StringCharAt, NULL);
	vm_reg_native(vm, cls, "charCodeAt(index)", native_StringCharCodeAt, NULL);
	vm_reg_native(vm, cls, "substring(start, end)", native_StringSubstring, NULL);
	vm_reg_native(vm, cls, "concat()", native_StringConcat, NULL);
	vm_reg_native(vm, cls, "lastIndexOf(searchValue, fromIndex)", native_StringLastIndexOf, NULL);
	vm_reg_native(vm, cls, "localeCompare(compareString)", native_StringLocaleCompare, NULL);

	cls = vm_new_class(vm, CLS_UTF8);
	vm_reg_native(vm, cls, "constructor(str)", native_UTF8Constructor, NULL); 
	vm_reg_native(vm, cls, "length()", native_UTF8Length, NULL); 
	vm_reg_native(vm, cls, "toString()", native_UTF8ToString, NULL); 
	vm_reg_native(vm, cls, "at(index)", native_UTF8At, NULL); 
	vm_reg_native(vm, cls, "set(index, s)", native_UTF8Set, NULL); 
	vm_reg_native(vm, cls, "substr(start, length)", native_UTF8Substr, NULL); 

	cls = vm_new_class(vm, CLS_UTF8_READER);
	vm_reg_native(vm, cls, "constructor(str)", native_UTF8ReaderConstructor, NULL); 
	vm_reg_native(vm, cls, "read()", native_UTF8ReaderRead, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
