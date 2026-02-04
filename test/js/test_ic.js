// Test script for inline cache performance and correctness

// Test 1: Basic property access
console.log('Test 1: Basic property access');
var obj = {
    name: 'Mario',
    age: 30,
    score: 1000
};

// Access properties multiple times to test cache hit rate
var start = Date.now();
for (var i = 0; i < 1000000; i++) {
    var n = obj.name;
    var a = obj.age;
    var s = obj.score;
}
var end = Date.now();
console.log('Time for 1,000,000 property accesses:', end - start, 'ms');

// Test 2: Property modification and cache invalidation
console.log('\nTest 2: Property modification and cache invalidation');
obj.name = 'Luigi';
obj.age = 28;
obj.score = 900;

// Access properties again to test cache invalidation
start = Date.now();
for (var i = 0; i < 1000000; i++) {
    var n = obj.name;
    var a = obj.age;
    var s = obj.score;
}
end = Date.now();
console.log('Time for 1,000,000 property accesses after modification:', end - start, 'ms');
console.log('Updated values: name=' + obj.name + ', age=' + obj.age + ', score=' + obj.score);

// Test 3: Multiple objects with same property names
console.log('\nTest 3: Multiple objects with same property names');
var obj1 = { x: 1, y: 2 };
var obj2 = { x: 10, y: 20 };
var obj3 = { x: 100, y: 200 };

start = Date.now();
for (var i = 0; i < 1000000; i++) {
    var x1 = obj1.x;
    var y1 = obj1.y;
    var x2 = obj2.x;
    var y2 = obj2.y;
    var x3 = obj3.x;
    var y3 = obj3.y;
}
end = Date.now();
console.log('Time for 1,000,000 property accesses across 3 objects:', end - start, 'ms');

// Test 4: Array access (to ensure it doesn't interfere with IC)
console.log('\nTest 4: Array access');
var arr = [1, 2, 3, 4, 5];

start = Date.now();
for (var i = 0; i < 1000000; i++) {
    var a0 = arr[0];
    var a1 = arr[1];
    var a2 = arr[2];
}
end = Date.now();
console.log('Time for 1,000,000 array accesses:', end - start, 'ms');

console.log('\nAll tests completed!');
