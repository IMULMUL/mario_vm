var a = 0;
while(a <= 10000000) {
	if((a % 100000) == 0)
		console.log(a);
	a++;
}

var b = {i: 0};
for(var i=0; i<=10000000; i++) {
	if((i % 100000) == 0)
		console.log(b.i);
	b.i++;
}

