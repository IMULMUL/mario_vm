function three(a,b,c){}
console.log("three.length =", three.length);
var nat = setTimeout;
console.log("native length =", nat && nat.length);
var b1 = three.bind(null, 1);
console.log("bound1 length =", b1.length);
var b2 = three.bind(null, 1, 2, 3, 4);
console.log("bound-over length =", b2.length);
