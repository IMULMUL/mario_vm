var str = "MR:你好世界!!!";
console.log(str + ":raw length = " + str.length());

let u = new UTF8(str);
console.log(u.toString() + ":utf8 length = " + u.length());

u.set(4, "棒");
u.set(5, "");
console.log(u.toString());

/*let ur = new UTF8Reader(str);
console.log("utf8reader: ");
while(true) {
	let w = ur.read();
	if(w.length() == 0) {
		console.log("\n");
		break;
	}
	console.log("[" + w + "]");
}
*/
u = u.substr(3, 5);
console.log(u.toString());

// String.prototype.trim Test
console.log("=== String.prototype.trim Test ===");

// Test 1: Basic trim functionality
try {
    var str = "   Hello, World!   ";
    var trimmed = str.trim();
    console.log("✓ Basic trim works");
    console.log("   Original: '" + str + "'");
    console.log("   Trimmed: '" + trimmed + "'");
    console.log("   Length before: " + str.length);
    console.log("   Length after: " + trimmed.length);
} catch (e) {
    console.log("✗ Basic trim failed:", e);
}

// Test 2: Trim only leading whitespace
try {
    var str = "   Hello";
    var trimmed = str.trim();
    console.log("✓ Leading trim works");
    console.log("   Original: '" + str + "'");
    console.log("   Trimmed: '" + trimmed + "'");
} catch (e) {
    console.log("✗ Leading trim failed:", e);
}

// Test 3: Trim only trailing whitespace
try {
    var str = "Hello   ";
    var trimmed = str.trim();
    console.log("✓ Trailing trim works");
    console.log("   Original: '" + str + "'");
    console.log("   Trimmed: '" + trimmed + "'");
} catch (e) {
    console.log("✗ Trailing trim failed:", e);
}

// Test 4: Trim all whitespace
try {
    var str = "   \t\nHello\n\t   ";
    var trimmed = str.trim();
    console.log("✓ All whitespace trim works");
    console.log("   Original: '" + str + "'");
    console.log("   Trimmed: '" + trimmed + "'");
} catch (e) {
    console.log("✗ All whitespace trim failed:", e);
}

// Test 5: Trim empty string
try {
    var str = "";
    var trimmed = str.trim();
    console.log("✓ Empty string trim works");
    console.log("   Original: '" + str + "'");
    console.log("   Trimmed: '" + trimmed + "'");
} catch (e) {
    console.log("✗ Empty string trim failed:", e);
}

// Test 6: Trim only whitespace string
try {
    var str = "   \t\n   ";
    var trimmed = str.trim();
    console.log("✓ Only whitespace trim works");
    console.log("   Original: '" + str + "'");
    console.log("   Trimmed: '" + trimmed + "'");
    console.log("   Length before: " + str.length);
    console.log("   Length after: " + trimmed.length);
} catch (e) {
    console.log("✗ Only whitespace trim failed:", e);
}

console.log("\n=== Test Complete ===");

// String.prototype.indexOf Test
console.log("\n=== String.prototype.indexOf Test ===");

// Test 1: Basic indexOf functionality
try {
    var str = "Hello, World!";
    var index = str.indexOf("World");
    console.log("✓ Basic indexOf works");
    console.log("   String: '" + str + "'");
    console.log("   Search: 'World'");
    console.log("   Index: " + index);
} catch (e) {
    console.log("✗ Basic indexOf failed:", e);
}

// Test 2: indexOf with fromIndex
try {
    var str = "Hello, World!";
    var index = str.indexOf("o", 5);
    console.log("✓ indexOf with fromIndex works");
    console.log("   String: '" + str + "'");
    console.log("   Search: 'o' from index 5");
    console.log("   Index: " + index);
} catch (e) {
    console.log("✗ indexOf with fromIndex failed:", e);
}

// Test 3: indexOf with not found
try {
    var str = "Hello, World!";
    var index = str.indexOf("test");
    console.log("✓ indexOf with not found works");
    console.log("   String: '" + str + "'");
    console.log("   Search: 'test'");
    console.log("   Index: " + index);
} catch (e) {
    console.log("✗ indexOf with not found failed:", e);
}

// Test 4: indexOf with empty search value
try {
    var str = "Hello, World!";
    var index = str.indexOf("");
    console.log("✓ indexOf with empty search value works");
    console.log("   String: '" + str + "'");
    console.log("   Search: ''");
    console.log("   Index: " + index);
} catch (e) {
    console.log("✗ indexOf with empty search value failed:", e);
}

// Test 5: indexOf with single character
try {
    var str = "Hello, World!";
    var index = str.indexOf("H");
    console.log("✓ indexOf with single character works");
    console.log("   String: '" + str + "'");
    console.log("   Search: 'H'");
    console.log("   Index: " + index);
} catch (e) {
    console.log("✗ indexOf with single character failed:", e);
}

// Test 6: indexOf with negative fromIndex
try {
    var str = "Hello, World!";
    var index = str.indexOf("o", -5);
    console.log("✓ indexOf with negative fromIndex works");
    console.log("   String: '" + str + "'");
    console.log("   Search: 'o' from index -5");
    console.log("   Index: " + index);
} catch (e) {
    console.log("✗ indexOf with negative fromIndex failed:", e);
}

// Test 7: indexOf with fromIndex beyond string length
try {
    var str = "Hello, World!";
    var index = str.indexOf("o", 20);
    console.log("✓ indexOf with fromIndex beyond string length works");
    console.log("   String: '" + str + "'");
    console.log("   Search: 'o' from index 20");
    console.log("   Index: " + index);
} catch (e) {
    console.log("✗ indexOf with fromIndex beyond string length failed:", e);
}

console.log("\n=== indexOf Test Complete ===");

// String.prototype.split Test
console.log("\n=== String.prototype.split Test ===");

// Test 1: Basic split functionality
try {
    var str = "Hello, World!";
    var parts = str.split(", ");
    console.log("✓ Basic split works");
    console.log("   String: '" + str + "'");
    console.log("   Separator: ', '");
    console.log("   Parts: [" + parts + "]");
    console.log("   Length: " + parts.length());
} catch (e) {
    console.log("✗ Basic split failed:", e);
}

// Test 2: split with limit
try {
    var str = "a,b,c,d,e";
    var parts = str.split(",", 3);
    console.log("✓ split with limit works");
    console.log("   String: '" + str + "'");
    console.log("   Separator: ','");
    console.log("   Limit: 3");
    console.log("   Parts: [" + parts + "]");
    console.log("   Length: " + parts.length());
} catch (e) {
    console.log("✗ split with limit failed:", e);
}

// Test 3: split with empty separator
try {
    var str = "Hello";
    var parts = str.split("");
    console.log("✓ split with empty separator works");
    console.log("   String: '" + str + "'");
    console.log("   Separator: ''");
    console.log("   Parts: [" + parts + "]");
    console.log("   Length: " + parts.length());
} catch (e) {
    console.log("✗ split with empty separator failed:", e);
}

// Test 4: split with not found separator
try {
    var str = "Hello, World!";
    var parts = str.split("test");
    console.log("✓ split with not found separator works");
    console.log("   String: '" + str + "'");
    console.log("   Separator: 'test'");
    console.log("   Parts: [" + parts + "]");
    console.log("   Length: " + parts.length());
} catch (e) {
    console.log("✗ split with not found separator failed:", e);
}

// Test 5: split with empty string
try {
    var str = "";
    var parts = str.split(",");
    console.log("✓ split with empty string works");
    console.log("   String: '" + str + "'");
    console.log("   Separator: ','");
    console.log("   Parts: [" + parts + "]");
    console.log("   Length: " + parts.length());
} catch (e) {
    console.log("✗ split with empty string failed:", e);
}

// Test 6: split with multiple separators
try {
    var str = "a,b,,c";
    var parts = str.split(",");
    console.log("✓ split with multiple separators works");
    console.log("   String: '" + str + "'");
    console.log("   Separator: ','");
    console.log("   Parts: [" + parts + "]");
    console.log("   Length: " + parts.length());
} catch (e) {
    console.log("✗ split with multiple separators failed:", e);
}

console.log("\n=== split Test Complete ====");

// String.prototype.toLowerCase Test
console.log("\n=== String.prototype.toLowerCase Test ====");

// Test 1: Basic toLowerCase functionality
try {
    var str = "HELLO, WORLD!";
    var lowercased = str.toLowerCase();
    console.log("✓ Basic toLowerCase works");
    console.log("   Original: '" + str + "'");
    console.log("   Lowercased: '" + lowercased + "'");
} catch (e) {
    console.log("✗ Basic toLowerCase failed:", e);
}

// Test 2: toLowerCase with mixed case
try {
    var str = "Hello, World!";
    var lowercased = str.toLowerCase();
    console.log("✓ toLowerCase with mixed case works");
    console.log("   Original: '" + str + "'");
    console.log("   Lowercased: '" + lowercased + "'");
} catch (e) {
    console.log("✗ toLowerCase with mixed case failed:", e);
}

// Test 3: toLowerCase with already lowercase
try {
    var str = "hello, world!";
    var lowercased = str.toLowerCase();
    console.log("✓ toLowerCase with already lowercase works");
    console.log("   Original: '" + str + "'");
    console.log("   Lowercased: '" + lowercased + "'");
} catch (e) {
    console.log("✗ toLowerCase with already lowercase failed:", e);
}

// Test 4: toLowerCase with empty string
try {
    var str = "";
    var lowercased = str.toLowerCase();
    console.log("✓ toLowerCase with empty string works");
    console.log("   Original: '" + str + "'");
    console.log("   Lowercased: '" + lowercased + "'");
} catch (e) {
    console.log("✗ toLowerCase with empty string failed:", e);
}

// Test 5: toLowerCase with numbers and symbols
try {
    var str = "HELLO123!@#";
    var lowercased = str.toLowerCase();
    console.log("✓ toLowerCase with numbers and symbols works");
    console.log("   Original: '" + str + "'");
    console.log("   Lowercased: '" + lowercased + "'");
} catch (e) {
    console.log("✗ toLowerCase with numbers and symbols failed:", e);
}

console.log("\n=== toLowerCase Test Complete ====");

// String.prototype.toUpperCase Test
console.log("\n=== String.prototype.toUpperCase Test ====");

// Test 1: Basic toUpperCase functionality
try {
    var str = "hello, world!";
    var uppercased = str.toUpperCase();
    console.log("✓ Basic toUpperCase works");
    console.log("   Original: '" + str + "'");
    console.log("   Uppercased: '" + uppercased + "'");
} catch (e) {
    console.log("✗ Basic toUpperCase failed:", e);
}

// Test 2: toUpperCase with mixed case
try {
    var str = "Hello, World!";
    var uppercased = str.toUpperCase();
    console.log("✓ toUpperCase with mixed case works");
    console.log("   Original: '" + str + "'");
    console.log("   Uppercased: '" + uppercased + "'");
} catch (e) {
    console.log("✗ toUpperCase with mixed case failed:", e);
}

// Test 3: toUpperCase with already uppercase
try {
    var str = "HELLO, WORLD!";
    var uppercased = str.toUpperCase();
    console.log("✓ toUpperCase with already uppercase works");
    console.log("   Original: '" + str + "'");
    console.log("   Uppercased: '" + uppercased + "'");
} catch (e) {
    console.log("✗ toUpperCase with already uppercase failed:", e);
}

// Test 4: toUpperCase with empty string
try {
    var str = "";
    var uppercased = str.toUpperCase();
    console.log("✓ toUpperCase with empty string works");
    console.log("   Original: '" + str + "'");
    console.log("   Uppercased: '" + uppercased + "'");
} catch (e) {
    console.log("✗ toUpperCase with empty string failed:", e);
}

// Test 5: toUpperCase with numbers and symbols
try {
    var str = "hello123!@#";
    var uppercased = str.toUpperCase();
    console.log("✓ toUpperCase with numbers and symbols works");
    console.log("   Original: '" + str + "'");
    console.log("   Uppercased: '" + uppercased + "'");
} catch (e) {
    console.log("✗ toUpperCase with numbers and symbols failed:", e);
}

console.log("\n=== toUpperCase Test Complete ====");

// String.prototype.replace Test
console.log("\n=== String.prototype.replace Test ====");

// Test 1: Basic replace functionality
try {
    var str = "Hello, World!";
    var replaced = str.replace("World", "JavaScript");
    console.log("✓ Basic replace works");
    console.log("   Original: '" + str + "'");
    console.log("   Replaced: '" + replaced + "'");
} catch (e) {
    console.log("✗ Basic replace failed:", e);
}

// Test 2: replace with multiple occurrences (should only replace first)
try {
    var str = "Hello, Hello, World!";
    var replaced = str.replace("Hello", "Hi");
    console.log("✓ replace with multiple occurrences works");
    console.log("   Original: '" + str + "'");
    console.log("   Replaced: '" + replaced + "'");
} catch (e) {
    console.log("✗ replace with multiple occurrences failed:", e);
}

// Test 3: replace with not found string
try {
    var str = "Hello, World!";
    var replaced = str.replace("JavaScript", "Hi");
    console.log("✓ replace with not found string works");
    console.log("   Original: '" + str + "'");
    console.log("   Replaced: '" + replaced + "'");
} catch (e) {
    console.log("✗ replace with not found string failed:", e);
}

// Test 4: replace with empty search string
try {
    var str = "Hello, World!";
    var replaced = str.replace("", "Hi");
    console.log("✓ replace with empty search string works");
    console.log("   Original: '" + str + "'");
    console.log("   Replaced: '" + replaced + "'");
} catch (e) {
    console.log("✗ replace with empty search string failed:", e);
}

// Test 5: replace with empty replacement
try {
    var str = "Hello, World!";
    var replaced = str.replace("World", "");
    console.log("✓ replace with empty replacement works");
    console.log("   Original: '" + str + "'");
    console.log("   Replaced: '" + replaced + "'");
} catch (e) {
    console.log("✗ replace with empty replacement failed:", e);
}

// Test 6: replace with single character
try {
    var str = "Hello, World!";
    var replaced = str.replace("o", "0");
    console.log("✓ replace with single character works");
    console.log("   Original: '" + str + "'");
    console.log("   Replaced: '" + replaced + "'");
} catch (e) {
    console.log("✗ replace with single character failed:", e);
}

console.log("\n=== replace Test Complete ====");

console.log("\n=== All String Tests Complete ====");


