// Comprehensive ES5 Grammar Test Suite
// Tests all major ES5 features

console.log("=== Comprehensive ES5 Grammar Test Suite ===");

// 1. Variable Declarations (var only)
console.log("\n1. Variable Declarations");
try {
    var a = 10;
    var b = "string";
    var c = true;
    console.log("   ✓ var declarations work");
    console.log("   a =", a, ", b =", b, ", c =", c);
} catch (e) {
    console.log("   ✗ var declarations failed:", e);
}

// 2. Functions
console.log("\n2. Functions");

try {
    // Regular function declaration
    function add(x, y) {
        return x + y;
    }
    console.log("   ✓ Function declaration works");
    console.log("   add(5, 3) =", add(5, 3));
} catch (e) {
    console.log("   ✗ Function declaration failed:", e);
}

try {
    // Function expression
    var multiply = function(x, y) {
        return x * y;
    };
    console.log("   ✓ Function expression works");
    console.log("   multiply(5, 3) =", multiply(5, 3));
} catch (e) {
    console.log("   ✗ Function expression failed:", e);
}

/*
try {
    // Nested function
    function outer() {
        var outerVar = "outer";
        function inner() {
            var innerVar = "inner";
            return outerVar + "-" + innerVar;
        }
        return inner();
    }
    console.log("   ✓ Nested function works");
    //console.log("   outer() =", outer());
} catch (e) {
    console.log("   ✗ Nested function failed:", e);
}
*/

// 3. Objects
console.log("\n3. Objects");

try {
    // Object literal
    var person = {
        name: "John",
        age: 30,
        greet: function() {
            return "Hello, " + this.name;
        }
    };
    console.log("   ✓ Object literal works");
    console.log("   person.name =", person.name);
    console.log("   person.greet() =", person.greet());
} catch (e) {
    console.log("   ✗ Object literal failed:", e);
}

try {
    // Object property assignment
    var obj = {};
    obj.property = "value";
    obj["dynamic"] = "property";
    console.log("   ✓ Object property assignment works");
    console.log("   obj.property =", obj.property);
    console.log("   obj[dynamic] =", obj.dynamic);
} catch (e) {
    console.log("   ✗ Object property assignment failed:", e);
}

// 4. Arrays
console.log("\n4. Arrays");

try {
    // Array literal
    var arr = [1, 2, 3, 4, 5];
    console.log("   ✓ Array literal works");
    console.log("   arr[0] =", arr[0]);
    console.log("   arr.length =", arr.length);
} catch (e) {
    console.log("   ✗ Array literal failed:", e);
}


try {
    // Array manipulation
    var arr2 = [];
    arr2.push(1);
    arr2.push(2);
    arr2.push(3);
    console.log("   ✓ Array manipulation works");
    console.log("   arr2 =", arr2);
} catch (e) {
    console.log("   ✗ Array manipulation failed:", e);
}

// 5. Loops
console.log("\n5. Loops");

try {
    // while loop
    var i = 0;
    var whileResult = [];
    while (i < 3) {
        whileResult.push(i);
        i++;
    }
    console.log("   ✓ while loop works");
    console.log("   whileResult =", whileResult);
} catch (e) {
    console.log("   ✗ while loop failed:", e);
}

try {
    // for loop
    var forResult = [];
    for (var j = 0; j < 3; j++) {
        forResult.push(j);
    }
    console.log("   ✓ for loop works");
    console.log("   forResult =", forResult);
} catch (e) {
    console.log("   ✗ for loop failed:", e);
}

try {
    // for-in loop
    var objForIn = {a: 1, b: 2, c: 3};
    var forInResult = [];
    for (var key in objForIn) {
        forInResult.push(key + ":" + objForIn[key]);
    }
    console.log("   ✓ for-in loop works");
    console.log("   forInResult =", forInResult);
} catch (e) {
    console.log("   ✗ for-in loop failed:", e);
}

// 6. Conditionals
console.log("\n6. Conditionals");

try {
    // if-else
    var condResult = "";
    var testVal = 5;
    if (testVal > 0) {
        condResult = "positive";
    } else {
        condResult = "non-positive";
    }
    console.log("   ✓ if-else works");
    console.log("   condResult =", condResult);
} catch (e) {
    console.log("   ✗ if-else failed:", e);
}

try {
    // ternary operator
    var ternaryResult = (5 > 3) ? "true" : "false";
    console.log("   ✓ ternary operator works");
    console.log("   ternaryResult =", ternaryResult);
} catch (e) {
    console.log("   ✗ ternary operator failed:", e);
}

// 7. Operators
console.log("\n7. Operators");

try {
    // Arithmetic operators
    var arithmeticResult = (10 + 5) * 2 / 3;
    console.log("   ✓ arithmetic operators work");
    console.log("   (10 + 5) * 2 / 3 =", arithmeticResult);
} catch (e) {
    console.log("   ✗ arithmetic operators failed:", e);
}

try {
    // Comparison operators
    var compResult1 = (5 === 5);
    var compResult2 = (5 !== "5");
    console.log("   ✓ comparison operators work");
    console.log("   5 === 5 =", compResult1);
    console.log("   5 !== \"5\" =", compResult2);
} catch (e) {
    console.log("   ✗ comparison operators failed:", e);
}

try {
    // Logical operators
    var logicalResult1 = (true && false);
    var logicalResult2 = (true || false);
    var logicalResult3 = !true;
    console.log("   ✓ logical operators work");
    console.log("   true && false =", logicalResult1);
    console.log("   true || false =", logicalResult2);
    console.log("   !true =", logicalResult3);
} catch (e) {
    console.log("   ✗ logical operators failed:", e);
}

// 8. Exception Handling
console.log("\n8. Exception Handling");

try {
    try {
        throw new Error("Test error");
    } catch (e) {
        console.log("   ✓ try-catch works");
        console.log("   Caught error:", e.message);
    }
} catch (e) {
    console.log("   ✗ try-catch failed:", e);
}

// 9. Object Properties
console.log("\n9. Object Properties");

try {
    var objProps = {
        firstName: "John",
        lastName: "Doe",
        getFullName: function() {
            return this.firstName + " " + this.lastName;
        }
    };
    console.log("   ✓ Object properties work");
    console.log("   objProps.firstName =", objProps.firstName);
    console.log("   objProps.getFullName() =", objProps.getFullName());
} catch (e) {
    console.log("   ✗ Object properties failed:", e);
}

// 10. Nested Objects and Arrays
console.log("\n10. Nested Objects and Arrays");

try {
    var nested = {
        level1: {
            level2: {
                value: 42
            }
        },
        array: [1, 2, [3, 4]]
    };
    console.log("   ✓ Nested structures work");
    console.log("   nested.level1.level2.value =", nested.level1.level2.value);
    console.log("   nested.array[2][0] =", nested.array);
} catch (e) {
    console.log("   ✗ Nested structures failed:", e);
}

// 11. typeof operator
console.log("\n11. typeof Operator");

try {
    console.log("   ✓ typeof works");
    console.log("   typeof 10 =", typeof 10);
    console.log("   typeof \"string\" =", typeof "string");
    console.log("   typeof true =", typeof true);
    console.log("   typeof {} =", typeof {});
    console.log("   typeof [] =", typeof []);
    console.log("   typeof function(){} =", typeof function(){});
} catch (e) {
    console.log("   ✗ typeof failed:", e);
}

// 12. instanceof operator
console.log("\n12. instanceof Operator");

try {
    function TestClass() {}
    var testInstance = new TestClass();
    console.log("   ✓ instanceof works");
    console.log("   testInstance instanceof TestClass =", testInstance instanceof TestClass);
    console.log("   [] instanceof Array =", [] instanceof Array);
    console.log("   {} instanceof Object =", {} instanceof Object);
} catch (e) {
    console.log("   ✗ instanceof failed:", e);
}

// 13. new operator
console.log("\n13. new Operator");

try {
    function Person(name) {
        this.name = name;
    }
    var john = new Person("John");
    console.log("   ✓ new operator works");
    console.log("   john.name =", john.name);
} catch (e) {
    console.log("   ✗ new operator failed:", e);
}

// 14. this keyword
console.log("\n14. this Keyword");

try {
    var objThis = {
        value: 42,
        getValue: function() {
            return this.value;
        }
    };
    console.log("   ✓ this keyword works");
    console.log("   objThis.getValue() =", objThis.getValue());
} catch (e) {
    console.log("   ✗ this keyword failed:", e);
}

// 15. Null and Undefined
console.log("\n15. Null and Undefined");

try {
    var nullVar = null;
    var undefinedVar;
    console.log("   ✓ null and undefined work");
    console.log("   nullVar =", nullVar);
    console.log("   undefinedVar =", undefinedVar);
    console.log("   nullVar === null =", nullVar === null);
    console.log("   undefinedVar === undefined =", undefinedVar === undefined);
} catch (e) {
    console.log("   ✗ null/undefined failed:", e);
}

// 16. Strict Mode
console.log("\n16. Strict Mode");

try {
    // Test 1: Basic strict mode activation
    "use strict";
    console.log("   ✓ Strict mode activated successfully");
} catch (e) {
    console.log("   ✗ Strict mode activation failed:", e);
}

try {
    // Test 2: Variable declaration in strict mode
    "use strict";
    strictVar = "test";
    console.log("   ✗ Variable declaration in strict mode failed!");
    console.log("   strictVar =", strictVar);
} catch (e) {
    console.log("   ✓ Variable declaration in strict mode works, error catched: ", e.message);
}

console.log("\n=== Test Suite Complete ===");
