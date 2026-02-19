console.log("benchmark while loop");
console.log("-------------------------");
var start_while = Date.now();
var a = 0;
while(a <= 10000000) {
	if((a % 1000000) == 0) {
		console.log(a);
	}
	a++;
}
var end_while = Date.now();

console.log("\nbenchmark for loop");
console.log("-------------------------");
var start_for = Date.now();
var b = {i: 0};
for(var i=0; i<=10000000; i++) {
	if((i % 1000000) == 0) {
		console.log(b.i);
	}
	b.i++;
}
var end_for = Date.now();

console.log("\n-------------------------");
console.log("while loop time: " + (end_while - start_while) + "ms");
console.log("for loop time  : " + (end_for - start_for) + "ms");

