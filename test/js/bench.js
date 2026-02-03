a = 0;

while(a < 10000000) {
	a++;
	if((a % 100000) == 0)
		debug(a);
}
