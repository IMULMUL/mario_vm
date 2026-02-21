function f( ) {
	let x = "closure test: ";
	function f2() {
		let y = 0;
		function f1() {
			console.log(x + y);
			y++;
			return y;
		}
		return f1;
	}
	return f2;
}

var f2 = f();
var f1 = f2();
while(true) {
	if(f1() >= 10)
		break;
}
