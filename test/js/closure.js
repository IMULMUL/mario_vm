function f( ) {
	let x = "closure test: ";
	let y = 0;

	function f2() {
		function f1() {
			console.log(x + y);
			y++;
			return y;
		}
		return f1;
	}
	return f2();
}

var fc = f();
while(true) {
	if(fc() >= 10)
		break;
}
