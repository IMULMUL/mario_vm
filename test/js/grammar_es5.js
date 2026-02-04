// ES5 Grammar Test Suite (Mario-compatible)

console.log("=== ES5 Grammar Test Suite ===");

// 1. Strict Mode
console.log("\n1. === Strict Mode ===");
try {
    "use strict";
    console.log("Strict mode enabled");
} catch (e) {
    console.log("Strict mode not supported:", e);
}

// 2. Variable Declarations (var only)
var i = 0;
var message = "Hello, ES5!";
console.log("2. Variables: i=" + i + ", message=" + message);

// 3. Loops
console.log("\n3. === Loops ===");

// while loop
console.log("3.1 while loop:");
while (i < 5) {
    console.log("   " + i);
    i++;
}

// for loop
console.log("3.2 for loop:");
for (var j = 0; j < 5; j++) {
    console.log("   " + j);
}

// for-in loop (may not be supported)
console.log("3.3 for-in loop:");
try {
    var obj = { a: 1, b: 2, c: 3 };
    for (var key in obj) {
        if (obj.hasOwnProperty(key)) {
            console.log("   " + key + ": " + obj[key]);
        }
    }
} catch (e) {
    console.log("   for-in loop not supported:", e);
}

// 4. Functions
console.log("\n4. === Functions ===");

// Regular function
function add(x, y) {
    return x + y;
}
console.log("4.1 add(2, 3) = " + add(2, 3));

// Function expression
var multiply = function(x, y) {
    return x * y;
};
console.log("4.2 multiply(2, 3) = " + multiply(2, 3));

// Nested function
function outer() {
    var outerVar = "outer";
    
    function inner() {
        var innerVar = "inner";
        return outerVar + ", " + innerVar;
    }
    
    return inner();
}
console.log("4.3 Nested function: " + outer());

// 5. Objects
console.log("\n5. === Objects ===");

// Object literal
var person = {
    name: "John",
    age: 30,
    address: {
        street: "123 Main St",
        city: "New York"
    },
    greet: function() {
        return "Hello, my name is " + this.name;
    }
};

console.log("5.1 Person object:");
console.log("   name: " + person.name);
console.log("   age: " + person.age);
console.log("   address: " + person.address.street + ", " + person.address.city);
console.log("   greet(): " + person.greet());

// Object methods (ES5) - may not be supported
console.log("5.2 Object.defineProperty:");
try {
    var obj2 = {};
    Object.defineProperty(obj2, "name", {
        value: "Mario",
        writable: true,
        enumerable: true,
        configurable: true
    });
    console.log("   obj2.name: " + obj2.name);
} catch (e) {
    console.log("   Object.defineProperty not supported:", e);
}

// Object.create - may not be supported
console.log("5.3 Object.create:");
try {
    var proto = { greet: function() { return "Hello"; } };
    var obj3 = Object.create(proto);
    console.log("   obj3.greet(): " + obj3.greet());
} catch (e) {
    console.log("   Object.create not supported:", e);
}

// 6. Arrays
console.log("\n6. === Arrays ===");

var arr = [1, 2, 3, 4, 5];
console.log("6.1 Original array:");
for (var k = 0; k < arr.length; k++) {
    console.log("   [" + k + "] = " + arr[k]);
}
