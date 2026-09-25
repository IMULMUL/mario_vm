// Regression: booleans must coerce to numbers (ToNumber) in arithmetic ops.
// Pinterest's immer bundle (all/30.js) does `3*!!isSet(x)` inside its each()/type()
// dispatch; a VM that treated V_BOOL as non-numeric returned undefined, so a plain
// object took the array `.forEach` branch -> "can not find function 'forEach'".
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) passed++;
    else { failed++; console.log("FAIL " + msg + ": got " + got + " want " + want); }
}

// --- binary arithmetic with boolean operands ------------------------------
eq(3 * false, 0, "3*false");
eq(3 * true, 3, "3*true");
eq(false * 3, 0, "false*3");
eq(true * 3, 3, "true*3");
eq(3 - false, 3, "3-false");
eq(3 - true, 2, "3-true");
eq(3 / true, 3, "3/true");
eq(4 / true, 4, "4/true");
eq(3 % true, 0, "3%true");
eq(true + true, 2, "true+true (numeric add, not concat)");
eq(false + false, 0, "false+false");
eq(3 + false, 3, "3+false (numeric add)");
eq(3 + true, 4, "3+true (numeric add)");

// --- `+` still concatenates when the OTHER operand is a string -------------
eq("a" + true, "atrue", "string + bool concatenates");
eq(true + "a", "truea", "bool + string concatenates");

// --- immer's actual expression shape --------------------------------------
function isSet() { return false; }
eq(3 * !!isSet(), 0, "3*!!fn() (immer type() object case)");
function isArr(x) { return Array.isArray(x); }
function typeOf(x) { return isArr(x) ? 1 : 3 * !!isSet(); }
eq(typeOf({}), 0, "plain object -> type 0");
eq(typeOf([]), 1, "array -> type 1");
eq(0 === typeOf({}), true, "0===typeOf({}) dispatches to object branch");

// --- unary minus / bitwise on booleans ------------------------------------
eq(-true, -1, "-true");
eq(Object.is(-false, -0), true, "-false is -0");
eq(-null, -0, "-null is -0 (loose)");
eq(true & 1, 1, "true & 1 (ToInt32)");
eq(true | 0, 1, "true | 0");
eq(~true, -2, "~true == -(1+1)");
eq(~false, -1, "~false == -1");
eq(true << 1, 2, "true << 1");

// --- via variables ---------------------------------------------------------
var b = false, t = true;
eq(3 * b, 0, "3*varFalse");
eq(3 * t, 3, "3*varTrue");
eq(t + t, 2, "varTrue+varTrue");

console.log(failed === 0 ? "__BOOL_ARITH_OK__" : ("__BOOL_ARITH_FAIL__ " + failed));
console.log("bool arithmetic regression: " + passed + " passed, " + failed + " failed");
