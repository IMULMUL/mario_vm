// Writes whose receiver is a transient (rvalue) object: `f().x = v`, `f()[i] = v`.
// Regression for the w3.org FontFaceObserver breakage: `e.style.font = "..."` on a
// fresh wrapper left the value stack short by one and shifted the whole frame
// protocol ("can not find function 'all'").
var passed = 0, failed = 0;
function check(name, ok) {
	if(ok) passed++;
	else { failed++; console.log("FAIL: " + name); }
}

var shared = null;
function getObj() { var o = { x: 1, arr: [1, 2, 3] }; shared = o; return o; }
function getArr() { var a = [10, 20, 30]; shared = a; return a; }
function getNew() { return { x: 5 }; }

// plain assignment, expression value is the RHS
var r = (getObj().x = 42);
check("obj assign value", r === 42);
check("obj assign landed", shared.x === 42);

// assignment used as a statement (POP peephole) inside a function - the frame
// must stay balanced so the following call resolves against the right receiver.
function statementForm() {
	getNew().x = 7;
	getNew().y = "abc";
	return [1, 2].concat([3]).length;
}
check("statement form frame balance", statementForm() === 3);

// compound assignment and ++/--
getObj().x += 8;
check("obj += landed", shared.x === 9);
getObj().x++;
check("obj ++ landed", shared.x === 2);
--getObj().x;
check("obj -- landed", shared.x === 0);

// subscript writes
getArr()[1] = 99;
check("arr[i] = landed", shared[1] === 99);
getArr()[0] += 5;
check("arr[i] += landed", shared[0] === 15);
getArr()[2]++;
check("arr[i]++ landed", shared[2] === 31);
getObj()["x"] = "s";
check("obj[\"k\"] = landed", shared.x === "s");

// new member on a transient receiver
getObj().brandNew = true;
check("new member landed", shared.brandNew === true);

// writing into a member of a member of a transient receiver
getObj().arr[0] = -1;
check("nested member landed", shared.arr[0] === -1);

// try/catch around the transient write (the FontFaceObserver shape)
function fontProbe() {
	var el = { style: {} };
	function styleOf() { return el.style; }
	try { styleOf().font = "condensed 100px sans-serif"; } catch(q) {}
	var G = "" !== styleOf().font;
	return G && el.style.font === "condensed 100px sans-serif";
}
check("try/catch transient write", fontProbe() === true);

// a loop of transient writes must not drift the stack
function loopForm() {
	var last = 0;
	for(var i = 0; i < 200; i++) {
		getNew().x = i;
		last = i;
	}
	return last;
}
check("loop transient writes", loopForm() === 199);

// Promise.all still resolvable right after transient writes (the w3.org shape)
var promiseOk = false;
if(typeof Promise !== "undefined") {
	getNew().x = 1;
	Promise.all([Promise.resolve(1)]).then(function(v) { promiseOk = (v[0] === 1); });
} else {
	promiseOk = true;
}

// --- Truly-transient receiver (refs<=1, NOT aliased to a global) in argument /
// array position. The getObj()/getArr() above assign to `shared`, so the returned
// object keeps refs>1 and takes the persistent node path - they never exercised
// the transient rvalue path. These do: the old code pushed a bare rvalue, so
// vm_pop2node returned NULL and handle_asign consumed two operands while pushing
// none, desyncing the value stack by one and corrupting the enclosing call. ---
function brandNew() { return { p: 1 }; }        // fresh object, no other reference
function newArr() { return [1, 2, 3]; }
function threeArg(a, b, c) { return "a=" + a + " b=" + b + " c=" + c; }
check("transient named write in arg position",
	threeArg((brandNew().p = 5), 2, 3) === "a=5 b=2 c=3");
check("transient computed write in arg position",
	threeArg((brandNew()["p"] = 9), 2, 3) === "a=9 b=2 c=3");
check("transient array-elem write in arg position",
	threeArg((newArr()[0] = 7), 2, 3) === "a=7 b=2 c=3");
// a method call right after a transient write must still resolve its receiver
var resolver = { all: function(a) { return "ALL:" + a.length; } };
brandNew().p = 1;
check("receiver intact after transient write", resolver.all([1, 2, 3]) === "ALL:3");
check("Promise.all resolvable after transient write", promiseOk === true);

console.log("transient_write: " + passed + " passed, " + failed + " failed");
