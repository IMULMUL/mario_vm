// String relational comparison + string/number mixed comparison.
// Pinterest's minified bundles guard browser APIs with `"u">typeof window`
// (an obfuscated typeof-window check); when relational comparison of two
// strings fell through to false the guard failed and the CSRF token was
// sent empty, so the server answered "Bad CSRF token".
var fails = 0;
function ok(cond, msg) {
    if(cond) { console.log("ok  " + msg); }
    else { fails++; console.log("FAIL " + msg); }
}

// two strings: lexicographic by code unit
ok(("u" > "object") === true, '"u">"object"');
ok(("u" > "undefined") === false, '"u">"undefined"');
ok(("a" < "b") === true, '"a"<"b"');
ok(("b" > "a") === true, '"b">"a"');
ok(("abc" < "abd") === true, '"abc"<"abd"');
ok(("xyz" > "abc") === true, '"xyz">"abc"');
ok(("Z" < "a") === true, '"Z"<"a" (code-unit order)');
ok(("" <= "") === true, '""<=""');
ok(("a" >= "a") === true, '"a">="a"');
ok(("a" < "a") === false, '"a"<"a"');
ok(("a" > "a") === false, '"a">"a"');

// string vs number: ToNumber(string)
ok(("5" < 10) === true, '"5"<10');
ok(("5" == 5) === true, '"5"==5');
ok(("5" === 5) === false, '"5"===5 (type-strict)');
ok(("5" !== 5) === true, '"5"!==5');
ok(("x" < 5) === false, '"x"<5 (NaN)');
ok(("x" == 5) === false, '"x"==5 (NaN)');
ok(("1" == true) === true, '"1"==true');
ok(("" == 0) === true, '""==0');
ok((" 7 " == 7) === true, '" 7 "==7 (trimmed)');

// equality of two strings still exact
ok(("abc" == "abc") === true, '"abc"=="abc"');
ok(("abc" != "abd") === true, '"abc"!="abd"');

console.log(fails === 0 ? "ALL PASS" : (fails + " FAILED"));
if(fails !== 0) throw new Error("string compare regressions");
