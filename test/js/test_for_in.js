var arr = ["aa", "bb", "cc"];
for (var key in arr) {
    console.log("   " + key + ": " + arr[key]);
}


var obj = new Date();
for (var key in obj) {
    console.log("   " + key + ": " + obj[key]);
}

console.log("\n");

var obj1 = {a: 0, b: 1, c: 2, d: 3};
for (var key in obj1) {
    console.log("   " + key + ": " + obj1[key]);
}

