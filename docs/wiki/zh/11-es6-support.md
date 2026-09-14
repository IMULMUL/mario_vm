# 第 11 章 · ES6+ 语言特性支持

前面几章讲的是「引擎怎么工作」。本章讲的是「作为一门 JavaScript，它到底支持哪些语法和内建对象」——也就是 Mario 的 JS 前端对 **ES6（ES2015）及其后续版本增补** 的支持程度。

结论先行：**Mario 已经实现了完整的 ES6 特性集**，并额外覆盖了 ES2016～ES2022 里最常一起被提起的增补（`async/await`、对象 rest/spread、指数运算符、`BigInt`、类型化数组、`Proxy/Reflect`、`WeakRef`、`SharedArrayBuffer/Atomics` 等）。

> 权威依据：仓库里的 [`test/js/es6_full.js`](../../../test/js/es6_full.js) 是一份**按 ECMA-262 标准编写**的测试套件，共 **854 条断言，全部通过**（`=== es6_full.js: 854 passed, 0 failed ===`）。本章的每一处「支持」都能在里面对应到具体断言。

---

## 11.1 支持总览

### 11.1.1 一分钟跑起来

```bash
make                              # 编译出 build/mario
./build/mario test/js/es6_full.js # 运行完整 ES6 测试套件
```

正常会看到 38 个分区逐条打印 `✓`，最后汇总 `854 passed, 0 failed` 与 `ALL TESTS PASSED`。

### 11.1.2 双层实现

ES6 支持落在两个层面，正好对应前几章讲过的两大模块：

```
ES6 源码
  │
  ├─ 语法层：lang/js/compiler.c        （第 5 章）
  │    let/const、模板串、箭头函数、解构、class、
  │    for...of、生成器、async/await、** 运算符 …
  │
  └─ 内建对象层：lang/js/native/...     （第 9 章）
       Map/Set、Promise、Proxy/Reflect、BigInt、
       ArrayBuffer/TypedArray、RegExp、Symbol …
```

- **语法层**由编译器把新语法翻译成第 3 章的字节码指令（例如模板字符串会生成 `INSTR_STR` + 拼接，标签模板还引入了 `INSTR_TAG_RAW`）。
- **内建对象层**是一批用 C 写的 native 类，在 VM 初始化时由 `reg_all_natives` 注册（见第 9 章）。

### 11.1.3 特性支持矩阵

下表按 `es6_full.js` 的 38 个分区归纳，全部为 ✅ 支持：

| # | 特性区 | 关键能力 |
| --- | --- | --- |
| 1 | `let` / `const` | 块级作用域、TDZ、`for-let` 每轮新绑定、const 不可重绑定 |
| 2 | 模板字符串 | 插值、多行、嵌套、转义、**标签模板** + `String.raw` |
| 3 | 默认参数 | 省略/`undefined` 触发默认、默认值可引用前参、调用时求值 |
| 4 | rest 参数 | `...args` 收集为真数组 |
| 5 | spread / 对象 rest | 数组/调用/对象展开、对象 rest 解构 |
| 6 | 解构 | 数组/对象/嵌套/默认值/交换/参数解构 |
| 7 | 对象字面量增强 | 简写、计算键、方法、getter/setter、`__proto__` |
| 8 | 箭头函数 | 词法 `this`、无 `[[Construct]]`、无自身 `arguments` |
| 9 | class | constructor、方法、static、`extends`/`super`、访问器 |
| 10 | `for...of` / `for...in` | 迭代协议、`Symbol.iterator` |
| 11 | 生成器 | `function*`、`yield`、`yield*` |
| 12 | Symbol | 唯一性、well-known symbols、symbol 作键 |
| 13 | 集合 | `Map` / `Set` / `WeakMap` / `WeakSet` |
| 14 | Promise | 状态机、`then/catch/finally`、`all/allSettled/race` |
| 15 | `async` / `await` | 异步函数、await 展开、错误传播 |
| 16 | Array 静态与原型 | `from/of/find/includes/flat/...` |
| 17 | String 方法 | `startsWith/includes/repeat/padStart/...` |
| 18 | Number & Math | `Number.isInteger/isNaN/...`、`Math` 增补 |
| 19 | Object 静态方法 | `assign/is/keys/values/entries/freeze/...` |
| 20 | 运算符 | 指数 `**`、`new.target`、`typeof`/`instanceof` 边界 |
| 21 | 闭包与循环捕获 | `let` vs `var` 经典捕获差异 |
| 22 | 互操作与边角 | 各类语义一致性 |
| 23 | 全局函数 & `globalThis` | `isNaN/parseInt/parseFloat`、`globalThis` |
| 24 | Error 子类型 | `TypeError/RangeError/...`、`AggregateError` |
| 25 | Object 静态（ES2022） | `hasOwn/setPrototypeOf/seal/preventExtensions/...` |
| 26 | `Array.at` & String 增补 | `at/trimStart/End/replaceAll/...` |
| 27 | `Promise.any`（ES2021） | 任一成功即决议 |
| 28 | JSON | `stringify`/`parse`、往返一致 |
| 29 | Date | 构造、字段访问器、`toISOString`、`Date.now/parse/UTC` |
| 30 | `undefined`/`null` 相等 | 宽松/严格相等语义 |
| 31 | 内建增补 & 逻辑运算符 | `&&`/`||` 返回操作数语义等 |
| 32 | 64 位数值 | int64 精确大整数、double 精度、`MAX_SAFE_INTEGER` |
| 33 | BigInt | 任意精度字面量/运算/比较/位运算、进制转换 |
| 34 | ArrayBuffer & DataView | 字节缓冲、各宽度读写、大小端、slice |
| 35 | TypedArray | `Int8Array … BigUint64Array` 全套构造与方法 |
| 36 | Proxy / Reflect | 元编程陷阱、13 个 `Reflect` 静态方法 |
| 37 | WeakRef / FinalizationRegistry | 弱引用、`deref`、清理回调 |
| 38 | SharedArrayBuffer / Atomics | 共享内存、原子 load/store/add/… |

### 11.1.4 明确**不**支持的项

为了「不夸大」，这里同样列出当前的边界（详见 [11.4 已知限制](#114-已知限制)）：

- **ES Modules**：不支持 `import` / `export`。Mario 是单文件脚本引擎。
- **正则字面量 `/.../`**：词法层没有正则字面量 token；正则要用 `new RegExp("...")` 构造（`RegExp` 对象本身是支持的）。
- **`Boolean` 包装类**：未注册 `Boolean` 构造器（布尔值本身完全可用）。

---

## 11.2 语法层特性（编译器）

本节的所有语法都在 [`lang/js/compiler.c`](../../../lang/js/compiler.c) 中实现，编译产物是第 3 章的字节码。

### 11.2.1 `let` / `const` 与块级作用域

- `let`/`const` 是**块级作用域**：`{ let x = 1; }` 里的 `x` 不会泄漏到块外；而 `var` 仍是函数级、会泄漏。
- **TDZ（暂时性死区）**：在声明前读取 `let`/`const` 会抛错。
- `for (let i = ...)` **每轮迭代都是一个新的 `i` 绑定**，因此循环里创建的闭包各自捕获到正确的值；`for (var j = ...)` 则共享同一个 `j`。
- `const` 绑定不可重新赋值，但对象**内容**仍可变。

```javascript
const fns = [];
for (let i = 0; i < 3; i++) fns.push(() => i);
fns.map(f => f());            // [0, 1, 2] —— 每个闭包拿到自己的 i

const vfns = [];
for (var j = 0; j < 3; j++) vfns.push(() => j);
vfns.map(f => f());           // [3, 3, 3] —— 共享同一个 j

const obj = { n: 1 };
obj.n = 2;                    // OK：内容可变
// obj = {};                  // 抛错：const 绑定不可重赋值
```

### 11.2.2 模板字符串与标签模板

普通模板字符串由 `factor_template` 处理：扫描原始字符流，把文本块与 `${...}` 表达式交替编译成字符串拼接。支持插值、多行、嵌套、转义序列，插值会对对象调用 `toString`。

```javascript
const who = "World";
`Hello, ${who}!`;             // "Hello, World!"
`${1 + 2} = ${3}`;            // "3 = 3"
`line1
line2`;                       // 保留换行 "line1\nline2"
```

**标签模板**（tagged template）由 `factor_tagged_template` 处理：它构造「cooked」字符串数组及其 `raw` 数组（用 `INSTR_TAG_RAW` 挂到 `strings.raw`），再以 `(strings, ...values)` 调用标签函数。

```javascript
function tag(strings, ...values) {
    let out = "";
    for (let i = 0; i < strings.length; i++) {
        out += strings[i];
        if (i < values.length) out += "[" + values[i] + "]";
    }
    return out;
}
tag`a${1}b${2}c`;             // "a[1]b[2]c"

function raw(strings) { return strings.raw[0]; }
raw`a\nb`;                    // "a\\nb"（等价于 String.raw）
```

### 11.2.3 箭头函数

箭头函数使用**词法 `this`**（沿用定义处外层的 `this`），没有自己的 `arguments`，也不能被 `new`。

```javascript
const obj = {
    name: "mario",
    greet() {
        const f = () => this.name;   // 箭头函数捕获定义处的 this
        return f();
    }
};
obj.greet();                  // "mario"
[1, 2, 3].map(x => x * 2);    // [2, 4, 6]
```

### 11.2.4 默认参数 / rest / spread

```javascript
// 默认参数：省略或传 undefined 时触发；可引用前面的形参；调用时求值
function power(base, exp = 2) { return base ** exp; }
power(5);                     // 25
power(5, undefined);          // 25（undefined 触发默认）
power(5, null);               // 1（null 不触发默认，5 ** 0）

// rest：收集为真正的数组
function sum(...nums) { return nums.reduce((a, b) => a + b, 0); }
sum(1, 2, 3, 4);              // 10

// spread：数组字面量 / 调用实参 / 对象
const base = [1, 2, 3];
[0, ...base, 4];              // [0, 1, 2, 3, 4]
Math.max(...base);            // 3
const o2 = { ...{ a: 1 }, b: 2 };   // { a: 1, b: 2 }
```

### 11.2.5 解构赋值

支持数组解构、对象解构、嵌套解构、默认值、交换、以及函数参数解构；对象 rest（`...rest`）也可用于解构。

```javascript
const [a, b] = [1, 2];                 // a=1, b=2
const { x, y = 10 } = { x: 1 };        // x=1, y=10（默认值）
const { p, ...rest } = { p: 1, q: 2, r: 3 };  // p=1, rest={q:2,r:3}
let m = 1, n = 2; [m, n] = [n, m];     // 交换：m=2, n=1
function dist({ lat, lng }) { return lat + "," + lng; }  // 参数解构
```

### 11.2.6 对象字面量增强

属性简写、计算属性名、方法简写、getter/setter，以及 `__proto__` 设置原型。

```javascript
const name = "vm";
const o = {
    name,                        // 简写：{ name: name }
    ["k_" + 1]: "v",             // 计算键：k_1
    greet() { return "hi"; },    // 方法简写
    get size() { return 42; },   // getter
    set size(v) { /* ... */ },   // setter
};
const proto = { hello() { return "world"; } };
const child = { __proto__: proto };
child.hello();                   // "world"
```

### 11.2.7 class 与继承

`class` 支持 `constructor`、实例方法、`static` 静态方法、`extends` 继承、`super` 调用父类构造与方法、以及类内的 getter/setter 访问器。

```javascript
class Animal {
    constructor(name) { this.name = name; }
    speak() { return this.name + " makes a sound"; }
    static create(name) { return new Animal(name); }
}
class Dog extends Animal {
    constructor(name) { super(name); }         // 调用父构造
    speak() { return super.speak() + ": woof"; } // 调用父方法
}
new Dog("Rex").speak();         // "Rex makes a sound: woof"
```

### 11.2.8 `for...of` 与迭代协议

`for...of` 遍历任何实现了迭代协议（`Symbol.iterator`）的对象：数组、字符串、`Map`、`Set`、生成器、类型化数组等。`for...in` 遍历可枚举键。

```javascript
for (const ch of "abc") { /* 'a','b','c' */ }

const m = new Map([["k", 1]]);
for (const [key, val] of m) { /* 结合数组解构遍历键值对 */ }
```

### 11.2.9 生成器（`function*` / `yield`）

```javascript
function* gen() { yield 1; yield 2; }
const it = gen();
it.next();                      // { value: 1, done: false }
it.next();                      // { value: 2, done: false }
it.next();                      // { value: undefined, done: true }
```

支持 `yield*` 委托到另一个可迭代对象，生成器也可用作 `for...of` 的数据源。

### 11.2.10 `async` / `await`

`async` 函数返回 Promise，`await` 在其中展开异步值；错误按 `try/catch` 传播。它与第 14 分区的 `Promise` 一起构成完整的异步编程模型。

```javascript
async function load() {
    const v = await Promise.resolve(42);
    return v * 2;
}
load().then(r => console.log(r));   // 84
```

### 11.2.11 新增运算符

- **指数运算符 `**`**：`2 ** 10 === 1024`，右结合，等价于 `Math.pow`。
- **`new.target`**：在构造调用中指向被 `new` 的构造器，可用于区分「被 new」与「普通调用」。

---

## 11.3 内建对象层（native 类）

内建类分布在 [`lang/js/native/`](../../../lang/js/native/)，注册入口见第 9 章。当前的完整清单如下（远比早期版本丰富）：

```
native/
├── builtin/            # 语言级基础类（reg_builtin_natives）
│   ├── Object/  Error/  Array/  String/  Number/  Symbol/  Console/
│   ├── Map/     Set/                       # 内含 WeakMap / WeakSet
│   ├── Promise/                            # + async/await 运行时
│   ├── Proxy/   Reflect/                   # 元编程
│   ├── BigInt/                             # 任意精度整数
│   ├── ArrayBuffer/ DataView/ TypedArray/  # 二进制 & 类型化数组
│   ├── SharedArrayBuffer/ Atomics/         # 共享内存 & 原子操作
│   ├── WeakRef/ FinalizationRegistry/      # 弱引用 & 清理回调
│   └── RegExp/                             # 正则对象（new RegExp）
└── natives/            # 扩展类（reg_natives）
    ├── Math/  Date/  JSON/
```

> 注册顺序（[`natives_builtin.c`](../../../lang/js/native/builtin/natives_builtin.c)）：Object → Error → Array → String → Console → Number → BigInt → ArrayBuffer → DataView → TypedArray → Promise → Map → Set → Symbol → Proxy → Reflect → WeakRef → FinalizationRegistry → SharedArrayBuffer → Atomics → RegExp；随后 `load_basic_classes` 缓存常用类指针、创建全局 `console`、注册全局 `Infinity` / `NaN`；最后 `reg_natives` 注册 Math / Date / JSON。

### 11.3.1 集合：Map / Set / WeakMap / WeakSet

`Map`/`Set` 支持完整的增删查遍历与迭代协议；`WeakMap`（在 [`native_Map.c`](../../../lang/js/native/builtin/Map/native_Map.c) 中注册）与 `WeakSet`（在 [`native_Set.c`](../../../lang/js/native/builtin/Set/native_Set.c) 中注册）以对象为键、持弱引用。

```javascript
const m = new Map([["a", 1]]);
m.set("b", 2); m.get("a");      // 1
m.has("b"); m.size;             // true, 2
for (const [k, v] of m) { /* 遍历 */ }

const s = new Set([1, 2, 2, 3]);
s.size;                         // 3（自动去重）

const wm = new WeakMap(); const key = {};
wm.set(key, "v"); wm.get(key);  // "v"
```

### 11.3.2 元编程：Proxy / Reflect

`Proxy` 支持 `get/set/has/deleteProperty/ownKeys/apply/construct` 等陷阱，以及原型、可扩展性、描述符相关陷阱与可撤销代理；`Reflect` 提供对应的 13 个静态方法。

```javascript
const p = new Proxy({ a: 1 }, {
    get(t, k) { return k in t ? t[k] : 0; }
});
p.a;                            // 1
p.missing;                      // 0（被 get 陷阱拦截）
Reflect.ownKeys({ a: 1 });      // ["a"]
Reflect.has({ a: 1 }, "a");     // true
```

### 11.3.3 二进制与类型化数组

`ArrayBuffer` + `DataView` 提供按字节、按宽度、可选大小端的读写；`TypedArray` 覆盖 `Int8Array` 一直到 `BigUint64Array` 的全套类型，支持从长度/数组/类型化数组/buffer 构造，以及 `from/of/subarray/slice/set/fill/sort/map/filter/reduce/...` 等方法与 `@@iterator`。`SharedArrayBuffer` + `Atomics` 提供共享内存与原子操作（`load/store/add/sub/and/or/xor/exchange/compareExchange`）。

```javascript
const buf = new ArrayBuffer(8);
const dv = new DataView(buf);
dv.setInt32(0, 42); dv.getInt32(0);       // 42

const i32 = new Int32Array([1, 2, 3]);
i32.length; i32.map(x => x * 2);          // 3, [2,4,6]

const sab = new SharedArrayBuffer(4);
const view = new Int32Array(sab);
Atomics.add(view, 0, 5);                  // 返回旧值
```

### 11.3.4 大整数：BigInt 与 64 位数值

`BigInt` 提供任意精度整数：`n` 后缀字面量、四则/比较/位运算、`toString(radix)`、`BigInt()`/`Number()` 转换、`asIntN`/`asUintN`。同时引擎对 64 位整数做了精确处理（大整数字面量与运算不丢精度，超出安全范围时提升为 double）。BigInt 字面量由词法层的 `LEX_BIGINT` token 支持。

```javascript
const big = 9007199254740993n;            // 超过 2^53，仍精确
big + 1n;                                 // 9007199254740994n
(255n).toString(16);                      // "ff"
Number.MAX_SAFE_INTEGER;                  // 9007199254740991
```

### 11.3.5 弱引用：WeakRef / FinalizationRegistry

`WeakRef` 持有对目标的弱引用，`deref()` 在目标存活时返回它、被回收后返回 `undefined`；`FinalizationRegistry` 在目标被回收后触发清理回调，支持 `register`/`unregister`。

```javascript
let target = { id: 1 };
const ref = new WeakRef(target);
ref.deref().id;                           // 1

const reg = new FinalizationRegistry(held => { /* 清理 */ });
reg.register(target, "token");
```

### 11.3.6 正则：RegExp

`RegExp` 由一个自带的回溯正则引擎实现（[`native_RegExp.c`](../../../lang/js/native/builtin/RegExp/native_RegExp.c)）。支持通过 `new RegExp(pattern, flags)` 构造，提供 `test`/`exec`/`toString` 与 `source`/`flags`/`global`/`ignoreCase`/`multiline`/`sticky`/`lastIndex` 属性。

- **flags**：`g` `i` `m` `s` `y`（`u` 接受但忽略——字符串本就按 UTF-8 字节处理）。
- **语法**：字面量字符、`.`、`\d\D\w\W\s\S`、`\b\B`、`\n\t\r\f\v\0\xHH\uHHHH`、字符类（范围/取反）、量词 `* + ? {n} {n,} {n,m}`（贪婪 + 惰性）、捕获组、非捕获组 `(?:)`、先行断言 `(?=)`/`(?!)`、`|`、`^ $`、反向引用 `\1`–`\9`。
- `String.prototype` 的 `match`/`replace`/`split`/`search` 既接受字符串，也接受 `RegExp` 对象；`replace` 支持回调函数。

```javascript
const re = new RegExp("h(a+)lo", "i");
re.test("HAAALO");                        // true
re.exec("xhaaaloY");                      // ["haaalo", "aaa", index:1, input:...]
re.toString();                            // "/h(a+)lo/i"

"xaaay".match(new RegExp("a+", "g"));     // ["aaa"]
"a1b2".replace(new RegExp("[0-9]", "g"), s => "[" + s + "]");  // "a[1]b[2]"
```

> ⚠️ **不支持正则字面量 `/.../`**（词法层没有对应 token），也**不支持后行断言 `(?<=)` / `(?<!)`**。请改用 `new RegExp("...")`。

### 11.3.7 Symbol

`Symbol` 提供唯一值与 well-known symbols（如 `Symbol.iterator`、`Symbol.toStringTag`），可用作对象键。

```javascript
const s1 = Symbol("id"), s2 = Symbol("id");
s1 === s2;                                // false（唯一）
const o = { [Symbol.iterator]() { /* ... */ } };
```

### 11.3.8 其他：Object / Array / String / Number / JSON / Date / Math / Error / globalThis

- **Object**：`assign/is/keys/values/entries/freeze/seal/preventExtensions/hasOwn/setPrototypeOf/getPrototypeOf/defineProperty/...`
- **Array**：`from/of/isArray`，原型方法 `find/findIndex/includes/flat/flatMap/at/splice/reduce/reduceRight/...`
- **String**：`startsWith/endsWith/includes/repeat/padStart/padEnd/trimStart/trimEnd/replaceAll/at/charAt/charCodeAt/...`
- **Number**：`isInteger/isNaN/isFinite/parseInt/parseFloat`、`MAX_VALUE/MIN_VALUE/MAX_SAFE_INTEGER`、`toFixed/toPrecision/...`
- **Math**：`floor/ceil/round/atan2/fround/random/...`
- **JSON**：`stringify`（紧凑输出）/`parse`，支持往返一致
- **Date**：构造、全字段访问器、`toISOString/toUTCString/toString`、`Date.now/getTime/valueOf/parse/UTC`
- **Error**：`Error` 及子类型 `TypeError/RangeError/SyntaxError/...`、`AggregateError`
- **全局**：`globalThis`、`isNaN/parseInt/parseFloat`、`Infinity`、`NaN`

---

## 11.4 已知限制

| 项 | 状态 | 说明 / 替代方案 |
| --- | --- | --- |
| ES Modules（`import`/`export`） | ❌ 不支持 | 单文件脚本引擎；跨文件请用第 10 章的 `include`/`.mbc` 机制 |
| 正则字面量 `/.../` | ❌ 不支持 | 用 `new RegExp("pattern", "flags")` |
| 正则后行断言 `(?<=)` / `(?<!)` | ❌ 不支持 | 先行断言 `(?=)`/`(?!)` 可用 |
| `Boolean` 包装类 | ❌ 未注册 | 布尔原始值完全可用，只是没有 `Boolean()` 构造器 |
| `Atomics.wait` | ⚠️ 抛错 | 单线程运行时无阻塞等待；`notify` 唤醒 0 个等待者 |

除上表外，第 11.1.3 节矩阵中的 38 个特性区均已实现并通过测试。

---

## 11.5 如何自行验证

1. **跑完整套件**：`./build/mario test/js/es6_full.js`，应输出 `854 passed, 0 failed`。
2. **跑单项示例**：把本章任意代码片段存成 `.js`，用 `./build/mario your.js` 运行。
3. **看字节码**：想理解某个 ES6 语法被编译成了什么，可结合第 2 章的 dump 选项与第 3 章的指令表，观察例如标签模板生成的 `INSTR_TAG_RAW`。

---

## 11.6 小结

- Mario 的 JS 前端已实现**完整的 ES6 特性集**，并覆盖到 ES2022 的常用增补；`test/js/es6_full.js` 的 **854 条标准断言全部通过**。
- 支持分两层落地：**编译器**（语法：`let/const`、模板串、箭头函数、解构、`class`、`for...of`、生成器、`async/await`、`**` 等）与 **native 内建类**（对象：`Map/Set`、`Promise`、`Proxy/Reflect`、`BigInt`、`TypedArray`、`RegExp`、`WeakRef` 等）。
- 主要边界是 **无 ES Modules、无正则字面量、无 `Boolean` 包装类**；这些在嵌入式/单文件脚本场景下通常可以绕过。

至此，从字节码、词法、编译、执行、对象模型、GC、native，到本章的 ES6+ 语言特性，整套 Wiki 已经覆盖了 Mario VM 的全貌。返回 [Wiki 首页](README.md)。
