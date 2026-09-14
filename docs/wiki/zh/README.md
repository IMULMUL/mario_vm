# Mario VM Wiki

> 语言 / Language：中文（当前） · [English](../en/README.md)

欢迎来到 **Mario VM** 的入门文档。这套 Wiki 面向第一次接触本项目的读者，从整体设计一路讲到字节码指令、编译器与虚拟机内部实现，力求做到「零基础上手」。

Mario 是一个**极小的、单文件的字节码虚拟机引擎**，不依赖任何第三方库，因此可以运行在绝大多数嵌入式系统上。本仓库在 Mario 内核之上，扩展出了一套 JavaScript 语言前端（词法分析 + 编译器）、丰富的内建类（native classes），以及一个命令行运行器。

---

## 阅读顺序

建议按下面的顺序阅读，每一章都建立在前一章的概念之上：

| 章节 | 标题 | 内容简介 |
| --- | --- | --- |
| 第 1 章 | [项目概览与整体架构](01-overview.md) | 项目是什么、由哪些模块组成、一条脚本从文本到执行经历了什么 |
| 第 2 章 | [快速开始：构建与运行](02-quickstart.md) | 如何编译出 `mario`，如何运行 `.js`、如何 dump 字节码 |
| 第 3 章 | [字节码指令集详解](03-bytecode.md) | 32 位指令编码格式、字符串表、全部指令的含义与栈行为 |
| 第 4 章 | [词法分析器](04-lexer.md) | 源码如何被切分成 token，基础词法与 JS 扩展词法 |
| 第 5 章 | [编译器（递归下降解析）](05-compiler.md) | 表达式优先级、语句编译、控制流如何生成跳转 |
| 第 6 章 | [虚拟机执行引擎](06-vm.md) | `vm_run` 主循环、指令分发表、栈与作用域栈的协作 |
| 第 7 章 | [对象模型与作用域](07-object-model.md) | `var_t`/`node_t`、原型链、类与继承、闭包 |
| 第 8 章 | [内存管理与垃圾回收](08-gc.md) | 引用计数 + 标记清除的混合 GC、变量缓冲池 |
| 第 9 章 | [Native 扩展与内建类](09-natives.md) | 如何用 C 注册原生函数/类，参数如何传递 |
| 第 10 章 | [字节码文件与工具链](10-mbc-and-tools.md) | `.mbc` 预编译文件格式、dump 工具、嵌入到你自己的程序 |
| 第 11 章 | [ES6+ 语言特性支持](11-es6-support.md) | 完整 ES6 及后续增补：语法特性、内建对象、支持矩阵与已知限制 |

---

## 一分钟理解 Mario

```
JavaScript 源码 (文本)
        │
        ▼   ① 词法分析 (lex)         ── 第 4 章
   Token 流
        │
        ▼   ② 编译器 (compiler)      ── 第 5 章
   字节码 (bytecode_t: 指令数组 + 字符串表)   ── 第 3 章
        │
        ▼   ③ 虚拟机 (vm_run)         ── 第 6 章
   基于栈的解释执行 ──► 操作 var_t 对象模型  ── 第 7 章
        │
        ▼   ④ 调用 C 实现的原生函数 (natives)  ── 第 9 章
   输出 / 副作用
```

整个引擎的核心只有一对文件：[`mario/mario.h`](../../../mario/mario.h) 与 [`mario/mario.c`](../../../mario/mario.c)。语言前端（这里是 JavaScript）是**可替换的**——你只需要实现一个 `bool compile(bytecode_t *bc, const char* input)` 函数，就能让 Mario 运行你自己的语言。

---

## 关键源码地图

| 路径 | 作用 |
| --- | --- |
| `mario/mario.h` / `mario/mario.c` | 虚拟机内核：数据结构、字节码生成、GC、执行引擎 |
| `mario/lex/mario_lex.*` | 基础词法分析器（与语言无关） |
| `mario/bcdump/bcdump.*` | 把字节码反汇编成可读文本 |
| `lang/js/compiler.c` | JavaScript 编译器（递归下降解析器） |
| `lang/js/native/...` | 内建类：Object / Array / String / Number / Symbol / Error / Map / Set / Promise / Proxy / Reflect / BigInt / ArrayBuffer / DataView / TypedArray / WeakRef / RegExp / JSON / Math / Date 等（完整清单见第 9、11 章） |
| `bin/mario/mario.c` | 命令行运行器 `main()` |
| `bin/lib/mbc.c` | `.mbc` 字节码文件的读写 |
| `test/js/*.js` | 示例脚本 |
| `mario/demos/` | 把 Mario 嵌入到 C 程序的示例 |

> 提示：文档中的文件链接使用相对路径，可在支持 Markdown 的编辑器/仓库浏览器中直接点击跳转。
