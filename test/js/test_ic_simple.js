// Simple test script for inline cache correctness

// Test 1: Basic property access
console.log('Test 1: Basic property access');
var obj = {
    name: 'Mario',
    age: 30,
    score: 1000
};

// Access properties multiple times
var n1 = obj.name;
var a1 = obj.age;
var s1 = obj.score;

var n2 = obj.name;
var a2 = obj.age;
var s2 = obj.score;

console.log('First access: name=' + n1 + ', age=' + a1 + ', score=' + s1);
console.log('Second access: name=' + n2 + ', age=' + a2 + ', score=' + s2);
console.log('Values match:', n1 === n2 && a1 === a2 && s1 === s2);

// Test 2: Property modification and cache invalidation
console.log('\nTest 2: Property modification and cache invalidation');
obj.name = 'Luigi';
obj.age = 28;
obj.score = 900;

// Access properties again
var n3 = obj.name;
var a3 = obj.age;
var s3 = obj.score;

var n4 = obj.name;
var a4 = obj.age;
var s4 = obj.score;

console.log('First access after modification: name=' + n3 + ', age=' + a3 + ', score=' + s3);
console.log('Second access after modification: name=' + n4 + ', age=' + a4 + ', score=' + s4);
console.log('Values match:', n3 === n4 && a3 === a4 && s3 === s4);
console.log('Values updated correctly:', n3 === 'Luigi' && a3 === 28 && s3 === 900);

// Test 3: Multiple objects with same property names
console.log('\nTest 3: Multiple objects with same property names');
var obj1 = { x: 1, y: 2 };
var obj2 = { x: 10, y: 20 };
var obj3 = { x: 100, y: 200 };

var x1 = obj1.x;
var y1 = obj1.y;
var x2 = obj2.x;
var y2 = obj2.y;
var x3 = obj3.x;
var y3 = obj3.y;

console.log('obj1: x=' + x1 + ', y=' + y1);
console.log('obj2: x=' + x2 + ', y=' + y2);
console.log('obj3: x=' + x3 + ', y=' + y3);
console.log('Values correct:', x1 === 1 && y1 === 2 && x2 === 10 && y2 === 20 && x3 === 100 && y3 === 200);

// Test 4: Complex object with nested properties
console.log('\nTest 4: Complex object with nested properties');
var complexObj = {
    person: {
        name: 'Mario',
        age: 30
    },
    stats: {
        score: 1000,
        level: 10
    }
};

var p1 = complexObj.person;
var n5 = p1.name;
var a5 = p1.age;

var s5 = complexObj.stats.score;
var l5 = complexObj.stats.level;

console.log('Person: name=' + n5 + ', age=' + a5);
console.log('Stats: score=' + s5 + ', level=' + l5);
console.log('Values correct:', n5 === 'Mario' && a5 === 30 && s5 === 1000 && l5 === 10);

console.log('\nAll tests completed!');
