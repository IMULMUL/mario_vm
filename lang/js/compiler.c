/**
 * very tiny js script compiler.
 */

#include "lex/mario_lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* _mario_lang = "js";
/** Script Lex. -----------------------------*/

typedef enum {
    LEX_EQUAL = LEX_BASIC_END,
    LEX_TYPEEQUAL,
    LEX_NEQUAL,
    LEX_NTYPEQUAL,
    LEX_LEQUAL,
    LEX_LSHIFT,
    LEX_LSHIFTEQUAL,
    LEX_GEQUAL,
    LEX_RSHIFT,
    LEX_RSHIFTUNSIGNED,
    LEX_RSHIFTUNSIGNEQUAL,
    LEX_RSHIFTEQUAL,
    LEX_PLUSEQUAL,
    LEX_MINUSEQUAL,
    LEX_MULTIEQUAL,
    LEX_DIVEQUAL,
    LEX_MODEQUAL,
    LEX_PLUSPLUS,
    LEX_MINUSMINUS,
    LEX_ANDEQUAL,
    LEX_ANDAND,
    LEX_OREQUAL,
    LEX_OROR,
    LEX_XOREQUAL,
    LEX_POWER,
    LEX_POWEREQUAL,
    LEX_OPTCHAIN,      // ?.   optional chaining
    LEX_NULLISH,       // ??   nullish coalescing
    LEX_NULLISHEQUAL,  // ??=  nullish assignment
    LEX_OREQUALOR,     // ||=  logical OR assignment
    LEX_ANDEQUALAND,   // &&=  logical AND assignment
    // reserved words
    LEX_R_IF,
    LEX_R_ELSE,
    LEX_R_DO,
    LEX_R_WHILE,
    LEX_R_FOR,
    LEX_R_BREAK,
    LEX_R_CONTINUE,
    LEX_R_STATIC,
    LEX_R_FUNCTION,
    LEX_R_AFUNCTION,
    LEX_R_CLASS,
    LEX_R_EXTENDS,
    LEX_R_RETURN,
    LEX_R_VAR,
    LEX_R_SAFE_VAR,
    LEX_R_CONST,
    LEX_R_TRUE,
    LEX_R_FALSE,
    LEX_R_NULL,
    LEX_R_UNDEFINED,
    LEX_R_NEW,
    LEX_R_TYPEOF,
    LEX_R_VOID,
    LEX_R_INCLUDE,
    LEX_R_THROW,
    LEX_R_TRY,
    LEX_R_CATCH,
    LEX_R_INSTANCEOF,
    LEX_R_ASYNC,
    LEX_R_AWAIT,
    LEX_R_DELETE,     // `delete` unary operator
    LEX_R_IN,         // `in` relational operator (and for-in separator)
    LEX_R_SWITCH,     // `switch` statement
    LEX_R_CASE,       // `case` clause
    LEX_R_DEFAULT,    // `default` clause
    LEX_R_IMPORT,     // `import` module statement
    LEX_R_EXPORT,     // `export` module statement
    LEX_R_LIST_END /* always the last entry */
} LEX_TYPES;

void lex_get_op_token(lex_t* lex) {
    if (lex->tk == '=' && lex->curr_ch == '=') { // ==
        lex->tk = LEX_EQUAL;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // ===
            lex->tk = LEX_TYPEEQUAL;
            lex_get_nextch(lex);
        }
    } else if (lex->tk == '!' && lex->curr_ch == '=') { // !=
        lex->tk = LEX_NEQUAL;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // !==
            lex->tk = LEX_NTYPEQUAL;
            lex_get_nextch(lex);
        }
    } else if (lex->tk == '<' && lex->curr_ch == '=') {
        lex->tk = LEX_LEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '<' && lex->curr_ch == '<') {
        lex->tk = LEX_LSHIFT;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // <<=
            lex->tk = LEX_LSHIFTEQUAL;
            lex_get_nextch(lex);
        }
    } else if (lex->tk == '>' && lex->curr_ch == '=') {
        lex->tk = LEX_GEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '>' && lex->curr_ch == '>') {
        lex->tk = LEX_RSHIFT;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // >>=
            lex->tk = LEX_RSHIFTEQUAL;
            lex_get_nextch(lex);
        } else if (lex->curr_ch == '>') { // >>> or >>>=
            lex->tk = LEX_RSHIFTUNSIGNED;
            lex_get_nextch(lex);
            if (lex->curr_ch == '=') { // >>>=
                lex->tk = LEX_RSHIFTUNSIGNEQUAL;
                lex_get_nextch(lex);
            }
        }
    } else if (lex->tk == '+' && lex->curr_ch == '=') {
        lex->tk = LEX_PLUSEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '-' && lex->curr_ch == '=') {
        lex->tk = LEX_MINUSEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '*' && lex->curr_ch == '*') { // ** or **=
        lex->tk = LEX_POWER;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // **=
            lex->tk = LEX_POWEREQUAL;
            lex_get_nextch(lex);
        }
    } else if (lex->tk == '*' && lex->curr_ch == '=') {
        lex->tk = LEX_MULTIEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '/' && lex->curr_ch == '=') {
        lex->tk = LEX_DIVEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '%' && lex->curr_ch == '=') {
        lex->tk = LEX_MODEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '+' && lex->curr_ch == '+') {
        lex->tk = LEX_PLUSPLUS;
        lex_get_nextch(lex);
    } else if (lex->tk == '-' && lex->curr_ch == '-') {
        lex->tk = LEX_MINUSMINUS;
        lex_get_nextch(lex);
    } else if (lex->tk == '&' && lex->curr_ch == '=') {
        lex->tk = LEX_ANDEQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '&' && lex->curr_ch == '&') {
        lex->tk = LEX_ANDAND;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // &&=
            lex->tk = LEX_ANDEQUALAND;
            lex_get_nextch(lex);
        }
    } else if (lex->tk == '|' && lex->curr_ch == '=') {
        lex->tk = LEX_OREQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '|' && lex->curr_ch == '|') {
        lex->tk = LEX_OROR;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // ||=
            lex->tk = LEX_OREQUALOR;
            lex_get_nextch(lex);
        }
    } else if (lex->tk == '?' && lex->curr_ch == '?') { // ?? or ??=
        lex->tk = LEX_NULLISH;
        lex_get_nextch(lex);
        if (lex->curr_ch == '=') { // ??=
            lex->tk = LEX_NULLISHEQUAL;
            lex_get_nextch(lex);
        }
    } else if (lex->tk == '?' && lex->curr_ch == '.' && !is_numeric((unsigned char)lex->next_ch)) {
        // ?. optional chaining (but not `? .5` — a ternary with a float literal)
        lex->tk = LEX_OPTCHAIN;
        lex_get_nextch(lex);
    } else if (lex->tk == '^' && lex->curr_ch == '=') {
        lex->tk = LEX_XOREQUAL;
        lex_get_nextch(lex);
    } else if (lex->tk == '=' && lex->curr_ch == '>') {
        lex->tk = LEX_R_AFUNCTION;
        lex_get_nextch(lex);
    }
}

void lex_get_js_str(lex_t* lex) {
    // js style strings 
    lex_get_nextch(lex);
    while (lex->curr_ch && lex->curr_ch != '\'') {
        if (lex->curr_ch == '\\') {
            lex_get_nextch(lex);
            /* Shared spec-correct escape decoder (see mario_lex.c). Fixes the
             * former octal branch that consumed 3 chars unconditionally, which
             * desynced the lexer on `'\0'` (core-js String.raw/dedent module). */
            lex_read_escape(lex);
        } else {
            mstr_add(lex->tk_str, lex->curr_ch);
        }
        lex_get_nextch(lex);
    }
    lex_get_nextch(lex);
    lex->tk = LEX_STR;
}

void lex_get_reserved_word(lex_t* lex) {
    if (strcmp(lex->tk_str->cstr, "if") == 0) {
        lex->tk = LEX_R_IF;
    } else if (strcmp(lex->tk_str->cstr, "else") == 0) {
        lex->tk = LEX_R_ELSE;
    } else if (strcmp(lex->tk_str->cstr, "do") == 0) {
        lex->tk = LEX_R_DO;
    } else if (strcmp(lex->tk_str->cstr, "while") == 0) {
        lex->tk = LEX_R_WHILE;
    } else if (strcmp(lex->tk_str->cstr, "include") == 0) {
        lex->tk = LEX_R_INCLUDE;
    } else if (strcmp(lex->tk_str->cstr, "for") == 0) {
        lex->tk = LEX_R_FOR;
    } else if (strcmp(lex->tk_str->cstr, "break") == 0) {
        lex->tk = LEX_R_BREAK;
    } else if (strcmp(lex->tk_str->cstr, "continue") == 0) {
        lex->tk = LEX_R_CONTINUE;
    } else if (strcmp(lex->tk_str->cstr, "static") == 0) {
        lex->tk = LEX_R_STATIC;
    } else if (strcmp(lex->tk_str->cstr, "function") == 0) {
        lex->tk = LEX_R_FUNCTION;
    } else if (strcmp(lex->tk_str->cstr, "class") == 0) {
        lex->tk = LEX_R_CLASS;
    } else if (strcmp(lex->tk_str->cstr, "extends") == 0) {
        lex->tk = LEX_R_EXTENDS;
    } else if (strcmp(lex->tk_str->cstr, "return") == 0) {
        lex->tk = LEX_R_RETURN;
    } else if (strcmp(lex->tk_str->cstr, "var") == 0) {
        lex->tk = LEX_R_VAR;
    } else if (strcmp(lex->tk_str->cstr, "let") == 0) {
        lex->tk = LEX_R_SAFE_VAR;
    } else if (strcmp(lex->tk_str->cstr, "const") == 0) {
        lex->tk = LEX_R_CONST;
    } else if (strcmp(lex->tk_str->cstr, "true") == 0) {
        lex->tk = LEX_R_TRUE;
    } else if (strcmp(lex->tk_str->cstr, "false") == 0) {
        lex->tk = LEX_R_FALSE;
    } else if (strcmp(lex->tk_str->cstr, "null") == 0) {
        lex->tk = LEX_R_NULL;
    } else if (strcmp(lex->tk_str->cstr, "undefined") == 0) {
        lex->tk = LEX_R_UNDEFINED;
    } else if (strcmp(lex->tk_str->cstr, "new") == 0) {
        lex->tk = LEX_R_NEW;
    } else if (strcmp(lex->tk_str->cstr, "typeof") == 0) {
        lex->tk = LEX_R_TYPEOF;
    } else if (strcmp(lex->tk_str->cstr, "void") == 0) {
        lex->tk = LEX_R_VOID;
    } else if (strcmp(lex->tk_str->cstr, "throw") == 0) {
        lex->tk = LEX_R_THROW;
    } else if (strcmp(lex->tk_str->cstr, "try") == 0) {
        lex->tk = LEX_R_TRY;
    } else if (strcmp(lex->tk_str->cstr, "catch") == 0) {
        lex->tk = LEX_R_CATCH;
    } else if (strcmp(lex->tk_str->cstr, "instanceof") == 0) {
        lex->tk = LEX_R_INSTANCEOF;
    } else if (strcmp(lex->tk_str->cstr, "async") == 0) {
        /* `async` is a contextual keyword, not reserved: `function f(a, async)`,
         * `{async: 1}` and `x.async` all use it as a plain name. Treat it as the
         * keyword only when the same line continues with something an async
         * function/arrow head starts with: `function`, a parameter name, a
         * `(` parameter list, `*` (async generator method) or `[` (computed
         * method key). Read head invariant: curr_ch == data[data_pos-2]. */
        int32_t p = lex->data_pos - 2;
        while (p < lex->data_end && (lex->data[p] == ' ' || lex->data[p] == '\t')) {
            p++;
        }
        char c = (p < lex->data_end) ? lex->data[p] : 0;
        /* '#' starts an ES2022 private method name (`async #n(){}`), which is
         * a valid async-method head just like a plain name. */
        if (is_alpha((unsigned char)c) || c == '_' || c == '$' || c == '(' || c == '*' || c == '[' || c == '#') {
            lex->tk = LEX_R_ASYNC;
        }
    } else if (strcmp(lex->tk_str->cstr, "await") == 0) {
        lex->tk = LEX_R_AWAIT;
    } else if (strcmp(lex->tk_str->cstr, "delete") == 0) {
        lex->tk = LEX_R_DELETE;
    } else if (strcmp(lex->tk_str->cstr, "in") == 0) {
        lex->tk = LEX_R_IN;
    } else if (strcmp(lex->tk_str->cstr, "switch") == 0) {
        lex->tk = LEX_R_SWITCH;
    } else if (strcmp(lex->tk_str->cstr, "case") == 0) {
        lex->tk = LEX_R_CASE;
    } else if (strcmp(lex->tk_str->cstr, "default") == 0) {
        lex->tk = LEX_R_DEFAULT;
    } else if (strcmp(lex->tk_str->cstr, "import") == 0) {
        lex->tk = LEX_R_IMPORT;
    } else if (strcmp(lex->tk_str->cstr, "export") == 0) {
        lex->tk = LEX_R_EXPORT;
    }
}

void lex_get_next_token(lex_t* lex) {
    lex->tk = LEX_EOF;
    mstr_reset(lex->tk_str);

    lex_skip_whitespace(lex);
    //lex_skip_space(lex);
    if (lex_skip_comments_line(lex, "//")) {
        lex_get_next_token(lex);
        return;
    }
    if (lex_skip_comments_block(lex, "/*", "*/")) {
        lex_get_next_token(lex);
        return;
    }

    lex_token_start(lex);
    lex_get_basic_token(lex);

    if (lex->tk == LEX_ID) { //  IDs
        lex_get_reserved_word(lex);
    } else if (lex->tk == LEX_EOF) {
        if (lex->curr_ch == '\'') {
            // js style strings 
            lex_get_js_str(lex);
        } else {
            lex_get_char_token(lex);
            lex_get_op_token(lex);
        }
    }

    lex_token_end(lex);
}

const char* lex_get_token_str(int token, char* str) {
    if (token > 32 && token < 128) {
        str[0] = (char)token;
        return str;
    }
    switch (token) {
        case LEX_EOF:
            return "EOF";
        case LEX_ID:
            return "ID";
        case LEX_INT:
            return "INT";
        case LEX_FLOAT:
            return "FLOAT";
        case LEX_STR:
            return "STRING";
        case LEX_BIGINT:
            return "BIGINT";
        case LEX_EQUAL:
            return "==";
        case LEX_TYPEEQUAL:
            return "===";
        case LEX_NEQUAL:
            return "!=";
        case LEX_NTYPEQUAL:
            return "!==";
        case LEX_LEQUAL:
            return "<=";
        case LEX_LSHIFT:
            return "<<";
        case LEX_LSHIFTEQUAL:
            return "<<=";
        case LEX_GEQUAL:
            return ">=";
        case LEX_RSHIFT:
            return ">>";
        case LEX_RSHIFTUNSIGNED:
            return ">>";
        case LEX_RSHIFTEQUAL:
            return ">>=";
        case LEX_PLUSEQUAL:
            return "+=";
        case LEX_MINUSEQUAL:
            return "-=";
        case LEX_MULTIEQUAL:
            return "*=";
        case LEX_DIVEQUAL:
            return "/=";
        case LEX_MODEQUAL:
            return "%=";
        case LEX_PLUSPLUS:
            return "++";
        case LEX_MINUSMINUS:
            return "--";
        case LEX_ANDEQUAL:
            return "&=";
        case LEX_ANDAND:
            return "&&";
        case LEX_OREQUAL:
            return "|=";
        case LEX_OROR:
            return "||";
        case LEX_XOREQUAL:
            return "^=";
            // reserved words
        case LEX_R_IF:
            return "if";
        case LEX_R_ELSE:
            return "else";
        case LEX_R_DO:
            return "do";
        case LEX_R_WHILE:
            return "while";
        case LEX_R_FOR:
            return "for";
        case LEX_R_BREAK:
            return "break";
        case LEX_R_CONTINUE:
            return "continue";
        case LEX_R_STATIC:
            return "static";
        case LEX_R_FUNCTION:
            return "function";
        case LEX_R_CLASS:
            return "class";
        case LEX_R_EXTENDS:
            return "extends";
        case LEX_R_RETURN:
            return "return";
        case LEX_R_CONST:
            return "CONST";
        case LEX_R_VAR:
            return "var";
        case LEX_R_SAFE_VAR:
            return "let";
        case LEX_R_TRUE:
            return "true";
        case LEX_R_FALSE:
            return "false";
        case LEX_R_NULL:
            return "null";
        case LEX_R_UNDEFINED:
            return "undefined";
        case LEX_R_NEW:
            return "new";
        case LEX_R_INCLUDE:
            return "include";
        case LEX_R_DELETE:
            return "delete";
        case LEX_R_VOID:
            return "void";
        case LEX_R_IN:
            return "in";
        case LEX_R_SWITCH:
            return "switch";
        case LEX_R_CASE:
            return "case";
        case LEX_R_DEFAULT:
            return "default";
        case LEX_R_IMPORT:
            return "import";
        case LEX_R_EXPORT:
            return "export";
    }
    return "?[UNKNOW]";
}

/** Function-declaration hoisting (ES5 semantics): a `function f(){}`
 * declaration binds its name at the top of the enclosing statement sequence
 * (script body, function body or plain block), so calls textually before the
 * declaration resolve. Minified bundles rely on this everywhere.
 *
 * Strategy: before a statement sequence is compiled, hoist_begin() runs a
 * quiet scan pass over it: function declarations are redirected into the REAL
 * bytecode (via g_funcdecl_bc) while everything else is parsed into a scratch
 * buffer and discarded. The real pass then compiles normally with declarations
 * redirected to the scratch buffer, so each is emitted exactly once at the top.
 * The scan pass is best effort: a parse error only truncates hoisting - the
 * real pass reports the error. */
static bytecode_t* g_funcdecl_bc = NULL; /* redirect target for declarations */
static bytecode_t* g_vardecl_bc = NULL;  /* var-declaration hoist target */
static bool g_hoist_quiet = false;       /* true while the scan pass parses */

/* Embedder hook: silence all compile diagnostics (used by eval's speculative
 * expression-form attempt, where a statement-shaped body is EXPECTED to fail
 * and be retried as statements). hoist_begin saves/restores this flag around
 * its scan pass, so the setting survives a full js_compile unchanged. */
void js_compile_set_quiet(bool quiet) { g_hoist_quiet = quiet; }
static void hoist_begin(lex_t* l, bytecode_t* bc, bytecode_t* scratch, bool block, bool func,
                        bytecode_t** saved_redirect, bytecode_t** saved_vardecl);
static void hoist_end(bytecode_t* scratch, bytecode_t* saved_redirect, bytecode_t* saved_vardecl);

void compile_error_pos(lex_t* l, int pos) {
    if (g_hoist_quiet) {
        return;
    }
    int line = 1;
    int col;

    lex_get_pos(l, &line, &col, pos);
    mario_printf("compile error at (line: %d, col: %d)\n", line, col);
    if (getenv("MARIO_ERRDUMP") && l && l->data) {
        int at = (pos >= 0) ? pos : l->data_pos;
        int from = at - 300; if (from < 0) from = 0;
        int to = at + 120;
        fprintf(stderr, "[ERRDUMP] @%d ctx=[", at);
        for (int i = from; i < to && l->data[i]; i++) {
            char c = l->data[i];
            if (c == '\n') fputc('\\', stderr), fputc('n', stderr);
            else fputc(c, stderr);
            if (i == at) fputs(">>", stderr);
        }
        fprintf(stderr, "]\n");
    }
}

bool lex_chkread(lex_t* lex, uint32_t expected_tk);
bool lex_skip_empty(lex_t* l) {
    if (l->tk == '\n') { //skip empty lines.
        while (l->tk == '\n') {
            if (!lex_chkread(l, '\n')) {
                return false;
            }
        }
    }
    return true;
}

bool lex_chkread(lex_t* lex, uint32_t expected_tk) { //check read with empty line.
    if (lex->tk != expected_tk) {
        char s_tk[2] = {0};
        char s_expect[2] = {0};
        const char* stk = lex_get_token_str(lex->tk, s_tk);
        const char* sexp = lex_get_token_str(expected_tk, s_expect);

        if (!g_hoist_quiet) {
            mario_printf("lex got '%s' expected '%s'! ", stk, sexp);
            compile_error_pos(lex, -1);
        }
        return false;
    }
    lex_get_next_token(lex);
    return true;
}

/** Compiler -----------------------------*/

bool statement(lex_t*, bytecode_t*);
bool factor(lex_t*, bytecode_t*, bool member);
bool base(lex_t*, bytecode_t*);

void gen_func_name(const char* name, int arg_num, mstr_t* full) {
    mstr_reset(full);
    mstr_cpy(full, name);
    if (arg_num > 0) {
        mstr_append(full, "$");
        mstr_append(full, mstr_from_int(arg_num, 10));
    }
}

/* Scan the argument list (starting at the current '(' token) for a top-level
 * ES6 spread '...'. The lexer state is fully restored before returning, so the
 * caller can re-parse the arguments normally. */
static bool call_args_have_spread(lex_t* l) {
    if (l->tk != '(') {
        return false;
    }
    lex_t saved = *l;                 // shallow copy: keeps original tk_str ptr
    mstr_t* saved_tk_str = l->tk_str; // do not let scanning clobber caller token
    l->tk_str = mstr_new("");
    mstr_cpy(l->tk_str, saved_tk_str->cstr);

    bool found = false;
    int depth = 0;    // call-paren depth: a spread ARGUMENT lives at depth 1
    int brace = 0;    // { } depth: an arrow/block body or an object literal
    int bracket = 0;  // [ ] depth: an array literal
    while (true) {
        if (l->tk == LEX_EOF) {
            break;
        }
        if (l->tk == '(') {
            depth++;
        } else if (l->tk == ')') {
            depth--;
            if (depth <= 0) {
                break;
            }
        } else if (l->tk == '{') {
            brace++;
        } else if (l->tk == '}') {
            if (brace > 0) brace--;
        } else if (l->tk == '[') {
            bracket++;
        } else if (l->tk == ']') {
            if (bracket > 0) bracket--;
        } else if (depth == 1 && brace == 0 && bracket == 0 &&
                   l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
            /* A top-level `...expr` argument only. A spread nested inside an
             * arrow body (`(e,t)=>{[...a]}`), an object literal (`{...o}`) or an
             * array literal (`[...a]`) is NOT a call-argument spread. The old
             * paren-only scan counted those `...` at paren depth 1, so a call
             * like `o.xI("$ZodType", (e,t)=>{[...x]})` was mis-flagged, sent
             * down the *_SPREAD codegen path and lost its callee entirely (the
             * function was never invoked). Require brace/bracket depth 0 too. */
            found = true;
            break;
        }
        lex_get_next_token(l);
    }

    mstr_free(l->tk_str);
    *l = saved; // restore scalar state and original tk_str pointer
    return found;
}

int call_func(lex_t* l, bytecode_t* bc, bool* has_spread) {
    if (has_spread != NULL) {
        *has_spread = false;
    }
    bool spread = call_args_have_spread(l);

    if (!lex_chkread(l, '(')) {
        return -1;
    }

	lex_skip_empty(l);
	if(l->tk == ')') {
		lex_chkread(l, ')');
        if (spread) {
            // empty explicit list cannot actually contain spread; be safe
        }
		return 0;
	}

    if (spread) {
        // Build a runtime args array, then let the *_SPREAD instruction call
        // with dynamic arity. Order is preserved for mixed spread/plain args.
        bc_gen(bc, INSTR_ARRAY);
        while (true) {
            lex_skip_empty(l);
            if (l->tk == ')') {
                break;
            }
            if (l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
                lex_get_nextch(l); // -> 3rd '.'
                lex_get_nextch(l); // -> first char of expr
                lex_get_next_token(l);
                if (!base(l, bc)) {
                    return -1;
                }
                bc_gen(bc, INSTR_ARR_SPREAD);
            } else {
                if (!base(l, bc)) {
                    return -1;
                }
                bc_gen(bc, INSTR_MEMBER);
            }
            lex_skip_empty(l);
            if (l->tk != ')') {
                if (!lex_chkread(l, ',')) {
                    return -1;
                }
            } else {
                break;
            }
            lex_skip_empty(l);
        }
        if (!lex_chkread(l, ')')) {
            return -1;
        }
        bc_gen(bc, INSTR_ARRAY_END);
        if (has_spread != NULL) {
            *has_spread = true;
        }
        return 0; // arity is resolved at runtime
    }

    int arg_num = 0;
    while (true) {
        PC pc1 = bc->cindex;
        if (!base(l, bc)) {
            return -1;
        }
        PC pc2 = bc->cindex;
        if (pc2 > pc1) { //not empty, means valid arguemnt.
            arg_num++;
        }

        if (l->tk != ')') {
            if (!lex_chkread(l, ',')) {
                return -1;
            }
            /* ES2017 trailing comma in call arguments: `f(a, b,)`. */
            lex_skip_empty(l);
            if (l->tk == ')') {
                break;
            }
        } else {
            break;
        }
    }
    if (!lex_chkread(l, ')')) {
        return -1;
    }
    return arg_num;
}

bool stmt_loop_block(lex_t* l, bytecode_t* bc) {
    bool block = false;
    if (l->tk && l->tk == '{') {
        if (!lex_chkread(l, '{')) {
            return false;
        }
        block = true;
    }

    if (block) {
        lex_skip_empty(l);
        while (l->tk && l->tk != '}') {
            int32_t prev_pos = l->data_pos;
            uint32_t prev_tk = l->tk;
            if (!statement(l, bc)) {
                return false;
            }
            /* Safety net: a statement that consumed nothing (unhandled
             * token) would spin this loop forever - fail instead. */
            if (l->data_pos == prev_pos && l->tk == prev_tk) {
                if (!g_hoist_quiet) {
                    mario_printf("compile error: unexpected token, made no progress! ");
                    compile_error_pos(l, -1);
                }
                return false;
            }
        }
        lex_skip_empty(l);
        return lex_chkread(l, '}');
    }
    lex_skip_empty(l);
    return statement(l, bc);
}

bool stmt_block(lex_t* l, bytecode_t* bc, bool func) {
    bool doBlock = false;
    if (!lex_chkread(l, '{')) {
        return false;
    }

    if (!func) {
        doBlock = true;
    }

    if (doBlock) {
        bc_gen(bc, INSTR_BLOCK);
    }

    /* Hoist function declarations to the top of the block (ES5 semantics). */
    bytecode_t scratch;
    bytecode_t* saved_redirect;
    bytecode_t* saved_vardecl;
    hoist_begin(l, bc, &scratch, true, func, &saved_redirect, &saved_vardecl);

    bool ok = true;
    while (l->tk && l->tk != '}') {
        int32_t prev_pos = l->data_pos;
        uint32_t prev_tk = l->tk;
        if (!statement(l, bc)) {
            ok = false;
            break;
        }
        /* Safety net: never loop forever on a token statement() ignores. */
        if (l->data_pos == prev_pos && l->tk == prev_tk) {
            mario_printf("compile error: unexpected token, made no progress! ");
            compile_error_pos(l, -1);
            ok = false;
            break;
        }
    }
    hoist_end(&scratch, saved_redirect, saved_vardecl);
    if (!ok) {
        return false;
    }
    if (!lex_chkread(l, '}')) {
        return false;
    }

    if (doBlock) {
        bc_gen(bc, INSTR_BLOCK_END);
    }
    return true;
}

/** A parameter that carries a default-value expression (ES6). The expression
 *  source text is captured verbatim so that it can be compiled at the start of
 *  the function body (func_def reads only INSTR_LOAD names before the body
 *  jump, so default code cannot live in the argument-name section). */
typedef struct {
    mstr_t* name;
    mstr_t* expr;
} param_default_t;

static void free_param_default(void* p) {
    param_default_t* pd = (param_default_t*)p;
    if (pd != NULL) {
        if (pd->name != NULL) mstr_free(pd->name);
        if (pd->expr != NULL) mstr_free(pd->expr);
        mario_free(pd);
    }
}

/* Forward declarations for the recursive destructuring helpers (defined
 * further below), needed by function-parameter destructuring. */
static bool skip_balanced_pattern(lex_t* l);
static bool destructure_pattern(lex_t* l, bytecode_t* bc, opr_code_t decl_op, const char* src);

/* A destructuring function parameter: the pattern binds into leaf variables
 * from a hidden temp that receives arguments[positional]. `saved` is the
 * lexer state at the pattern start (re-parsed at body-emit time); `def` is an
 * optional whole-pattern default expression source. */
typedef struct {
    char    temp[32];
    lex_t   saved;
    mstr_t* def;
} param_destr_t;

static void free_param_destr(void* p) {
    param_destr_t* pd = (param_destr_t*)p;
    if (pd != NULL) {
        if (pd->def != NULL) mstr_free(pd->def);
        mario_free(pd);
    }
}

/* Monotonic counter used to name hidden temporaries introduced by
 * destructuring (both declarations and parameters). */
static int g_destr_counter = 0;

/** Scan a default-parameter expression starting at the current character and
 *  capture its source text into `out`. Scanning stops at the ',' or ')' that
 *  terminates the argument, honouring nesting and string/template literals.
 *  On return the lexer is positioned at the terminating character (which is
 *  not consumed). Returns the terminator, or 0 on error/EOF. */
static char scan_param_expr(lex_t* l, mstr_t* out) {
    int depth = 0;
    while (l->curr_ch) {
        char c = l->curr_ch;
        if (c == '"' || c == '\'' || c == '`') { // string / template literal
            char q = c;
            mstr_add(out, c);
            lex_get_nextch(l);
            while (l->curr_ch && l->curr_ch != q) {
                if (l->curr_ch == '\\') {
                    mstr_add(out, l->curr_ch);
                    lex_get_nextch(l);
                    if (l->curr_ch) {
                        mstr_add(out, l->curr_ch);
                        lex_get_nextch(l);
                    }
                    continue;
                }
                mstr_add(out, l->curr_ch);
                lex_get_nextch(l);
            }
            if (l->curr_ch == q) {
                mstr_add(out, l->curr_ch);
                lex_get_nextch(l);
            }
            continue;
        }
        if (c == '(' || c == '[' || c == '{') {
            depth++;
        } else if (c == ')' || c == ']' || c == '}') {
            if (depth == 0) {
                return c; // terminator (only ')' is valid here)
            }
            depth--;
        } else if (c == ',' && depth == 0) {
            return c; // terminator
        }
        mstr_add(out, c);
        lex_get_nextch(l);
    }
    return 0;
}

/** Compile a captured expression source string into bytecode using a temporary
 *  lexer. Used for default-parameter initialisers. */
static bool compile_captured_expr(const char* src, bytecode_t* bc) {
    lex_t sub;
    lex_init(&sub, src);
    lex_get_next_token(&sub);
    bool ok = base(&sub, bc);
    lex_release(&sub);
    return ok;
}

/* Destructuring assignment in expression position (defined with the other
 * destructuring helpers below; used by factor()). */
static bool destructure_assign_ex(lex_t* l, bytecode_t* bc, opr_code_t op, bool leave_value);
static bool peek_is_destr_assign(lex_t* l);

/* Source buffer of the js_compile() currently running (MARIO_SRCMAP keying). */
static const char* g_srcmap_data = NULL;

/** Non-zero while compiling the body of an `async` function. Used by
 *  stmt_return / func_params_and_body to wrap the returned value in a
 *  resolved Promise (Promise.resolve). Entering any function body resets it
 *  to that function's own async-ness, so nested functions are unaffected. */
static int g_async_depth = 0;

/* Set to 1 immediately before defining an `async` function/arrow. It is
 * consumed by factor_def_func / factor_def_afunc to establish g_async_depth
 * for that body and then cleared, so nested definitions default to sync. */
static int g_async_pending = 0;

/* Set to 1 when the class-body parser has already consumed a `static`
 * keyword (needed to look past it for `async` / `{`); factor_def_func
 * consumes it exactly like an in-place `static` token. */
static int g_static_pending = 0;

/* Set to 1 when the class-body parser consumed the `*` of a generator method
 * ahead of a computed key (`*[Symbol.iterator]() {}`); factor_def_func
 * consumes it exactly like an in-place `*` token. */
static int g_gen_pending = 0;

/* Set to 1 while defining a function EXPRESSION (base()). factor_def_func then
 * emits INSTR_FUNC_NAMED carrying the expression's own name so the VM can bind
 * that name to the function inside its body (ES named-function-expression
 * self-reference). Cleared right after, so declarations/methods are unaffected. */
static int g_func_selfname = 0;

/* Set when the immediately preceding subscript kept its receiver on the stack
 * (INSTR_ARRAY_AT_M) because a call `(` follows, telling the postfix `(` case
 * to emit INSTR_CALLXO (bind that receiver as `this`) instead of INSTR_CALLX. */
static int g_arrat_recv = 0;

/** Scan pass for function-declaration hoisting: parses the statement sequence
 * ahead (a block body up to its `}`, or the rest of the script) into `scratch`,
 * with function declarations redirected into the real `bc` so they are emitted
 * once at the sequence top. The lexer is restored to the sequence start and
 * declarations are redirected to `scratch` for the real pass that follows, so
 * they are parsed but not re-emitted. Returns the previous redirect target for
 * hoist_end(). Global codegen state touched by a (possibly failed) scan is
 * restored so the real pass starts clean. */
static void hoist_begin(lex_t* l, bytecode_t* bc, bytecode_t* scratch, bool block, bool func,
                        bytecode_t** out_redirect, bytecode_t** out_vardecl) {
    bc_init(scratch);
    *out_redirect = g_funcdecl_bc;
    *out_vardecl = g_vardecl_bc;
    bool saved_quiet = g_hoist_quiet;
    int saved_async_pend = g_async_pending;
    int saved_async_depth = g_async_depth;
    int saved_arrat = g_arrat_recv;

    lex_t saved = *l;
    mstr_t* saved_str = mstr_new(l->tk_str->cstr);
    g_funcdecl_bc = bc;
    /* ES5 `var` declarations hoist to the enclosing function/script top. A
     * function body or the script top level becomes the hoist target; plain
     * nested blocks inherit the enclosing target so `if(x){var o=1}` still
     * binds o at the function level (matching handle_var's runtime scope). */
    if (func || !block) {
        g_vardecl_bc = bc;
    }
    g_hoist_quiet = true;
    while (l->tk != LEX_EOF && (!block || l->tk != '}')) {
        int32_t prev_pos = l->data_pos;
        uint32_t prev_tk = l->tk;
        if (!statement(l, scratch)) {
            break;
        }
        /* Same no-progress safety net as the real loops. */
        if (l->tk != LEX_EOF && l->data_pos == prev_pos && l->tk == prev_tk) {
            break;
        }
    }
    g_async_pending = saved_async_pend;
    g_async_depth = saved_async_depth;
    g_arrat_recv = saved_arrat;
    g_hoist_quiet = saved_quiet;
    *l = saved;
    mstr_cpy(l->tk_str, saved_str->cstr);
    mstr_free(saved_str);

    /* The real pass emits declarations into the scratch buffer instead, and
     * declares vars in place (the hoisted declaration already bound them). */
    g_funcdecl_bc = scratch;
    g_vardecl_bc = NULL;
}

static void hoist_end(bytecode_t* scratch, bytecode_t* saved_redirect, bytecode_t* saved_vardecl) {
    g_funcdecl_bc = saved_redirect;
    g_vardecl_bc = saved_vardecl;
    bc_release(scratch);
}

/** Parse a function's parameter list (starting at '(') through its closing ')',
 *  emitting the argument-name LOAD instructions that func_def scans and filling
 *  the ES6 parameter metadata: `defaults` (default-valued params), `pdestrs`
 *  (destructuring params), `*rest_name` (a trailing ...rest, else left NULL) and
 *  `*positional` (the count of non-rest params). The caller owns the metadata
 *  and must clean it up; on failure this returns false WITHOUT freeing so the
 *  caller can unwind uniformly. The caller must already have emitted INSTR_FUNC
 *  (or a variant) so the LOADs land where func_def expects them. Shared by the
 *  ordinary-function path (func_params_and_body) and the parenthesised
 *  arrow-function path (factor), so both accept the same ES6 parameter forms. */
static bool func_parse_params(lex_t* l, bytecode_t* bc, m_array_t* defaults,
                              m_array_t* pdestrs, mstr_t** rest_name, int* positional) {
    lex_skip_empty(l);
    //do arguments
    if (!lex_chkread(l, '(')) {
        return false;
    }
    lex_skip_empty(l);

    while (l->tk != ')') {
        // ES6 rest parameter: ...name (must be the last parameter)
        if (l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
            lex_get_nextch(l); // -> 3rd '.'
            lex_get_nextch(l); // -> first char of the rest name
            lex_get_next_token(l);
            /* ES6 rest + destructuring pattern: `...[a, b]` / `...{a, b}`.
             * The slice lands in a hidden temp that the pattern then binds
             * its leaves from, so record it both as the rest target and as a
             * destructuring parameter sourced from that same temp. The
             * prelude assigns the slice before running the pattern binds. */
            if (l->tk == '[' || l->tk == '{') {
                char rtemp[32];
                param_destr_t* pd;
                snprintf(rtemp, sizeof(rtemp), "__pr%d", g_destr_counter++);
                *rest_name = mstr_new(rtemp);
                pd = (param_destr_t*)mario_malloc(sizeof(param_destr_t));
                memset(pd, 0, sizeof(*pd));
                snprintf(pd->temp, sizeof(pd->temp), "%s", rtemp);
                pd->saved = *l;                    // pattern-start lexer state
                if (!skip_balanced_pattern(l)) {
                    mario_free(pd);
                    break;
                }
                lex_skip_empty(l);
                if (l->tk == '=') {
                    mstr_t* ex = mstr_new("");
                    if (scan_param_expr(l, ex) == 0) {
                        mstr_free(ex);
                        mario_free(pd);
                        break;
                    }
                    pd->def = ex;
                    lex_get_next_token(l);
                    lex_skip_empty(l);
                }
                array_add(pdestrs, pd);
                continue;
            }
            if (l->tk != LEX_ID) {
                break;
            }
            *rest_name = mstr_new(l->tk_str->cstr);
            if (!lex_chkread(l, LEX_ID)) {
                break;
            }
            lex_skip_empty(l);
            continue;
        }

        // ES6 destructuring parameter: `{...}` or `[...]` (optionally `= default`)
        if (l->tk == '{' || l->tk == '[') {
            param_destr_t* pd = (param_destr_t*)mario_malloc(sizeof(param_destr_t));
            memset(pd, 0, sizeof(*pd));
            snprintf(pd->temp, sizeof(pd->temp), "__pn%d", g_destr_counter++);
            bc_gen_str(bc, INSTR_LOAD, pd->temp); // argname slot for func_def
            (*positional)++;
            pd->saved = *l;                        // pattern-start lexer state
            if (!skip_balanced_pattern(l)) {
                mario_free(pd);
                break;
            }
            lex_skip_empty(l);
            if (l->tk == '=') {
                mstr_t* ex = mstr_new("");
                if (scan_param_expr(l, ex) == 0) {
                    mstr_free(ex);
                    mario_free(pd);
                    break;
                }
                pd->def = ex;
                lex_get_next_token(l);
                lex_skip_empty(l);
            }
            array_add(pdestrs, pd);
            if (l->tk != ')') {
                if (!lex_chkread(l, ',')) {
                    break;
                }
                lex_skip_empty(l);
            }
            continue;
        }

        if (l->tk != LEX_ID && l->tk != LEX_R_UNDEFINED) {
            /* `undefined` is not a reserved word in ES5 - it is an ordinary
             * identifier, so `function(exports, undefined){}` is legal (a
             * classic pattern to secure a local undefined). Accept it as a
             * parameter name; value uses still compile to INSTR_UNDEF, which
             * matches the runtime value of an unpassed parameter. */
            break;
        }
        mstr_t* pname = mstr_new(l->tk_str->cstr);
        bc_gen_str(bc, INSTR_LOAD, pname->cstr); // argument name for func_def
        (*positional)++;
        if (!lex_chkread(l, l->tk)) {
            mstr_free(pname);
            break;
        }
        lex_skip_empty(l);

        // ES6 default parameter: name = <expr>
        if (l->tk == '=') {
            mstr_t* expr = mstr_new("");
            char term = scan_param_expr(l, expr);
            if (term == 0) {
                mstr_free(pname);
                mstr_free(expr);
                break;
            }
            param_default_t* pd = (param_default_t*)mario_malloc(sizeof(param_default_t));
            pd->name = pname; // transfer ownership
            pd->expr = expr;
            array_add(defaults, pd);
            lex_get_next_token(l); // load the terminator (',' or ')')
            lex_skip_empty(l);
        } else {
            mstr_free(pname);
        }

        if (l->tk != ')') {
            if (!lex_chkread(l, ',')) {
                break;
            }
            lex_skip_empty(l);
        }
    }
    if (!lex_chkread(l, ')')) {
        return false;
    }
    return true;
}

/** Emit the ES6 parameter prelude at the start of a function body: the
 *  default-value initialisers, the rest-parameter slice, and the
 *  destructuring-parameter bindings. The caller must have already reserved the
 *  body's leading JMP sentinel so this prelude runs before the body statements
 *  (func_def sets the entry pc to the instruction right after that sentinel). */
static bool func_emit_prelude(bytecode_t* bc, m_array_t* defaults,
                              m_array_t* pdestrs, mstr_t* rest_name, int positional) {
    // Emit ES6 default-parameter initialisers at the start of the body:
    //   if (typeof name === "undefined") name = <expr>;
    uint32_t di;
    for (di = 0; di < defaults->size; di++) {
        param_default_t* pd = (param_default_t*)array_get(defaults, di);
        bc_gen_str(bc, INSTR_LOAD, pd->name->cstr);
        bc_gen(bc, INSTR_TYPEOF);
        bc_gen_str(bc, INSTR_STR, "undefined");
        bc_gen(bc, INSTR_TEQ);
        PC pj = bc_reserve(bc);
        bc_gen_str(bc, INSTR_LOAD, pd->name->cstr); // assignment target
        if (!compile_captured_expr(pd->expr->cstr, bc)) {
            return false;
        }
        bc_gen(bc, INSTR_ASIGN);
        bc_gen(bc, INSTR_POP);
        bc_set_instr(bc, pj, INSTR_NJMP, ILLEGAL_PC);
    }

    // Emit ES6 rest-parameter initialiser: rest = arguments.slice(positional)
    if (rest_name != NULL) {
        bc_gen_str(bc, INSTR_VAR, rest_name->cstr);
        bc_gen_str(bc, INSTR_LOAD, rest_name->cstr); // assignment target
        bc_gen_str(bc, INSTR_LOAD, "arguments");
        bc_gen_int(bc, INSTR_INT, positional);
        bc_gen_str(bc, INSTR_CALLO, "slice$1");
        bc_gen(bc, INSTR_ASIGN);
        bc_gen(bc, INSTR_POP);
    }

    // Emit ES6 destructuring-parameter initialisers: bind leaf vars from the
    // hidden temp that received arguments[positional].
    uint32_t pi;
    for (pi = 0; pi < pdestrs->size; pi++) {
        param_destr_t* pd = (param_destr_t*)array_get(pdestrs, pi);
        if (pd->def != NULL) {
            bc_gen_str(bc, INSTR_LOAD, pd->temp);
            bc_gen(bc, INSTR_TYPEOF);
            bc_gen_str(bc, INSTR_STR, "undefined");
            bc_gen(bc, INSTR_TEQ);
            PC pj = bc_reserve(bc);
            bc_gen_str(bc, INSTR_LOAD, pd->temp); // assignment target
            if (!compile_captured_expr(pd->def->cstr, bc)) {
                return false;
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
            bc_set_instr(bc, pj, INSTR_NJMP, ILLEGAL_PC);
        }
        lex_t pl = pd->saved;
        pl.tk_str = mstr_new("");
        bool ok = destructure_pattern(&pl, bc, INSTR_VAR, pd->temp);
        mstr_free(pl.tk_str);
        if (!ok) {
            return false;
        }
    }
    return true;
}

/** Parse a function's parameter list (starting at '(') and body, emitting the
 *  argument-name instructions expected by func_def plus the body bytecode.
 *  Supports ES6 default parameters and a trailing rest parameter. The caller
 *  must already have emitted INSTR_FUNC (or a variant). */
bool func_params_and_body(lex_t* l, bytecode_t* bc) {
    m_array_t defaults;      // param_default_t* entries
    array_init(&defaults);
    m_array_t pdestrs;       // param_destr_t* entries (destructuring params)
    array_init(&pdestrs);
    mstr_t* rest_name = NULL; // ES6 rest parameter (...rest)
    int positional = 0;       // number of positional (non-rest) parameters

    if (!func_parse_params(l, bc, &defaults, &pdestrs, &rest_name, &positional)) {
        array_clean(&defaults, free_param_default);
        array_clean(&pdestrs, free_param_destr);
        if (rest_name != NULL) mstr_free(rest_name);
        return false;
    }
    lex_skip_empty(l);
    PC pc = bc_reserve(bc);

    if (!func_emit_prelude(bc, &defaults, &pdestrs, rest_name, positional)) {
        array_clean(&defaults, free_param_default);
        array_clean(&pdestrs, free_param_destr);
        if (rest_name != NULL) mstr_free(rest_name);
        return false;
    }

    stmt_block(l, bc, true);
    opr_code_t op = bc->code_buf[bc->cindex - 1] >> 16;

    if (op != INSTR_RETURN && op != INSTR_RETURNV) {
        if (g_async_depth > 0) {
            /* async function fell off the end -> resolve(undefined) */
            bc_gen(bc, INSTR_UNDEF);
            bc_gen_str(bc, INSTR_CALL, "__promise_resolve$1");
            bc_gen(bc, INSTR_RETURNV);
        } else {
            bc_gen(bc, INSTR_RETURN);
        }
    }
    bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC);

    // free captured parameter metadata
    array_clean(&defaults, free_param_default);
    array_clean(&pdestrs, free_param_destr);
    if (rest_name != NULL) {
        mstr_free(rest_name);
    }
    return true;
}

bool factor_def_func(lex_t* l, bytecode_t* bc, mstr_t* name) {
    /* Establish this function's async context, consuming g_async_pending so
     * that any nested definition defaults to synchronous. */
    int saved_async = g_async_depth;
    g_async_depth = g_async_pending;
    g_async_pending = 0;
    bool is_static = g_static_pending != 0;
    g_static_pending = 0;
    lex_skip_empty(l);

    if (l->tk == LEX_R_STATIC) {
        if (!lex_chkread(l, LEX_R_STATIC)) {
            return false;
        }
        is_static = true;
    }

    /* ES6 generator: `function*` or a `*method()` shorthand. The star precedes
     * the (optional) name, so detect it here before reading the name. */
    bool is_gen = g_gen_pending != 0;
    g_gen_pending = 0;
    lex_skip_empty(l);
    if (l->tk == '*') {
        if (!lex_chkread(l, '*')) {
            return false;
        }
        is_gen = true;
        lex_skip_empty(l);
    }

    /* we can have functions without names */
    if (l->tk == LEX_ID) {
        mstr_cpy(name, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
            return false;
        }
    } else if (l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END) {
        /* A reserved word as a method name: classes and object literals may
         * define `delete(e){...}`, `new(){...}`, `default(){...}`. The keyword
         * token keeps its source text in tk_str, so capture it like an ID. */
        mstr_cpy(name, l->tk_str->cstr);
        int tk = l->tk;
        if (!lex_chkread(l, tk)) {
            return false;
        }
    }
    if (l->tk == LEX_ID || (l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END &&
                            (strcmp(name->cstr, "get") == 0 || strcmp(name->cstr, "set") == 0))) {
        /* class get/set token; the accessor name may itself be a reserved
         * word (`get class(){}`, `set default(v){}`). */
        int ntk = l->tk;
        if (strcmp(name->cstr, "get") == 0) {
            mstr_cpy(name, l->tk_str->cstr);
            if (!lex_chkread(l, ntk)) {
                return false;
            }
            bc_gen(bc, INSTR_FUNC_GET);
        } else if (strcmp(name->cstr, "set") == 0) {
            mstr_cpy(name, l->tk_str->cstr);
            if (!lex_chkread(l, ntk)) {
                return false;
            }
            bc_gen(bc, INSTR_FUNC_SET);
        }
    } else {
        /* A named function EXPRESSION (g_func_selfname set by base()) must bind its
         * own name inside its body; emit INSTR_FUNC_NAMED carrying that name. Only
         * the plain (non-generator, non-static) form is covered - generator/static
         * NFE self-reference is rare and keeps the existing behaviour. */
        if (g_func_selfname && !is_gen && !is_static && name->cstr[0] != 0)
            bc_gen_str(bc, INSTR_FUNC_NAMED, name->cstr);
        else
            bc_gen(bc, is_gen ? INSTR_FUNC_GEN : (is_static ? INSTR_FUNC_STC : INSTR_FUNC));
    }
    bool ok = func_params_and_body(l, bc);
    g_async_depth = saved_async;
    return ok;
}

/** Compile an arrow-function body (everything after the `=>`). The optional
 *  `defaults`/`pdestrs`/`rest_name`/`positional` carry the ES6 parameter
 *  metadata captured while parsing a parenthesised parameter list; the concise
 *  single-parameter form (`x => ...`) has none and passes NULLs. The prelude is
 *  emitted right after the body's leading JMP sentinel so it runs before the
 *  body statements, exactly like an ordinary function's prelude. */
bool factor_def_afunc_ex(lex_t* l, bytecode_t* bc, m_array_t* defaults,
                         m_array_t* pdestrs, mstr_t* rest_name, int positional) {
    int saved_async = g_async_depth;
    g_async_depth = g_async_pending;
    g_async_pending = 0;
    lex_skip_empty(l);
    PC pc = bc_reserve(bc);

    if (defaults != NULL) {
        if (!func_emit_prelude(bc, defaults, pdestrs, rest_name, positional)) {
            g_async_depth = saved_async;
            return false;
        }
    }

    if (l->tk == '{') {
        // Block body: `=> { ... }`. Any missing return is patched below.
        statement(l, bc);
    } else {
        // ES6 concise body: `=> expr` is exactly `=> { return expr; }`.
        if (!base(l, bc)) {
            g_async_depth = saved_async;
            return false;
        }
        if (g_async_depth > 0) {
            // async concise body resolves the expression value into a promise.
            bc_gen_str(bc, INSTR_CALL, "__promise_resolve$1");
        }
        bc_gen(bc, INSTR_RETURNV);
    }

    opr_code_t op = bc->code_buf[bc->cindex - 1] >> 16;

    if (op != INSTR_RETURN && op != INSTR_RETURNV) {
        if (g_async_depth > 0) {
            bc_gen(bc, INSTR_UNDEF);
            bc_gen_str(bc, INSTR_CALL, "__promise_resolve$1");
            bc_gen(bc, INSTR_RETURNV);
        } else {
            bc_gen(bc, INSTR_RETURN);
        }
    }
    bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC);
    g_async_depth = saved_async;
    return true;
}

bool factor_def_afunc(lex_t* l, bytecode_t* bc) {
    return factor_def_afunc_ex(l, bc, NULL, NULL, NULL, 0);
}

static bool lex_chkread_stmt_end(lex_t* l) {
    if (l->tk == 0) {
        return true;
    }

    if (l->tk == ';') {
        return lex_chkread(l, ';');
    } else if (l->tk == '\n') {
        return lex_chkread(l, '\n');
    } else if (l->tk == '}') {
        /* ASI: a statement is also terminated by the closing '}' of its
         * enclosing block (`{break}`). Leave the '}' for the block parser. */
        return true;
    }
    return false;
    //return lex_chkread(l, ';');
}

bool factor_def_class(lex_t* l, bytecode_t* bc) {
    // actually parse a class...
    if (!lex_chkread(l, LEX_R_CLASS)) {
        return false;
    }
    mstr_t* name = mstr_new("");

    lex_skip_empty(l);
    /* we can have classes without names */
    if (l->tk == LEX_ID) {
        mstr_cpy(name, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
            mstr_free(name);
            return false;
        }
    }
    bc_gen_str(bc, INSTR_CLASS, name->cstr);

    lex_skip_empty(l);
    /*read extends*/
    if (l->tk == LEX_R_EXTENDS) {
        if (!lex_chkread(l, LEX_R_EXTENDS)) {
            mstr_free(name);
            return false;
        }
        lex_skip_empty(l);
        /* JS allows any left-hand-side expression after `extends` - a member
         * expression (`namespace.Base`), a call (`getBase()`), a parenthesised
         * expression, or a bare identifier - not just a single identifier.
         * Evaluate it onto the stack, then INSTR_EXTENDS_V pops that value and
         * links the class prototype chain (handle_class already pushed the class
         * scope carrying the class var). A bare identifier compiles to a plain
         * LOAD, so the common `extends Base` case is unchanged in behaviour. */
        if (!base(l, bc)) {
            mstr_free(name);
            return false;
        }
        bc_gen(bc, INSTR_EXTENDS_V);
    }

    lex_skip_empty(l);
    if (!lex_chkread(l, '{')) {
        mstr_free(name);
        return false;
    }
    lex_skip_empty(l);
    while (l->tk != '}') {
        /* ES2022 class fields: `name;`, `name = expr;`, `static name = expr;`.
         * An instance field's initializer runs per construction with `this`
         * bound to the new object, so it is compiled as a hidden zero-arg
         * function and registered on the class through INSTR_FIELDN; a static
         * field evaluates once at class-definition time (INSTR_STATICN). A
         * name followed by '(' is a method, not a field, so the lexer state is
         * saved and restored when the lookahead disproves a field. */
        {
            lex_t sv = *l;
            mstr_t* sv_tk = l->tk_str;
            l->tk_str = mstr_new("");
            mstr_cpy(l->tk_str, sv_tk->cstr);
            bool is_static_f = false;
            bool is_field = false;
            char fname[128];
            fname[0] = 0;
            if (l->tk == LEX_R_STATIC) {
                lex_chkread(l, LEX_R_STATIC);
                lex_skip_empty(l);
                is_static_f = true;
            }
            if (l->tk == LEX_ID) {
                snprintf(fname, sizeof(fname), "%s", l->tk_str->cstr);
                lex_chkread(l, LEX_ID);
                lex_skip_empty(l);
                if (l->tk == '=' || l->tk == ';' || l->tk == ',' || l->tk == '}')
                    is_field = true;
            }
            if (!is_field) {
                mstr_free(l->tk_str);
                *l = sv;
                l->tk_str = sv_tk;
            } else {
                mstr_free(l->tk_str); // drop the temp; sv_tk survives in sv
                l->tk_str = sv_tk;
                bool has_init = false;
                if (l->tk == '=') {
                    lex_chkread(l, '=');
                    lex_skip_empty(l);
                    has_init = true;
                } else if (l->tk == ';' || l->tk == ',') {
                    lex_chkread(l, l->tk);
                    lex_skip_empty(l);
                }
                if (is_static_f) {
                    if (has_init) {
                        if (!base(l, bc)) {
                            mstr_free(name);
                            return false;
                        }
                    } else {
                        bc_gen(bc, INSTR_UNDEF);
                    }
                    bc_gen_str(bc, INSTR_STATICN, fname);
                } else {
                    static uint32_t field_id = 0;
                    char fnn[40];
                    snprintf(fnn, sizeof(fnn), "@field$%u", field_id++);
                    bc_gen_str(bc, INSTR_FUNC, fnn);
                    PC pc = bc_reserve(bc);
                    if (has_init) {
                        if (!base(l, bc)) {
                            mstr_free(name);
                            return false;
                        }
                    } else {
                        bc_gen(bc, INSTR_UNDEF);
                    }
                    bc_gen(bc, INSTR_RETURNV);
                    bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC);
                    bc_gen_str(bc, INSTR_FIELDN, fname);
                }
                if (l->tk == ';' || l->tk == ',') {
                    lex_chkread(l, l->tk);
                }
                lex_skip_empty(l);
                continue;
            }
        }
        if (l->tk == LEX_ID && l->next_ch == '=') {
            bc_gen_str(bc, INSTR_LOAD, l->tk_str->cstr);
            if (!lex_chkread(l, LEX_ID)) {
                mstr_free(name);
                return false;
            }
            if (!lex_chkread(l, '=')) {
                mstr_free(name);
                return false;
            }
            if (!base(l, bc)) {
                mstr_free(name);
                return false;
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
            lex_chkread_stmt_end(l);
            lex_skip_empty(l);
        } else {
            /* ES6 computed method name: `[Symbol.iterator]() {...}`. Evaluate
             * the key at class-definition time and leave it on the stack under
             * the method value; INSTR_MEMBERV pops (value, key) and defines the
             * member with the runtime key. */
            bool computed_name = false;
            bool static_member = false;
            /* `static` is consumed here (not in factor_def_func) so the class
             * body can look past it for `static async m(){}`, `static [k](){}`
             * and the ES2022 static initialization block `static { ... }`. */
            if (l->tk == LEX_R_STATIC) {
                if (!lex_chkread(l, LEX_R_STATIC)) {
                    mstr_free(name);
                    return false;
                }
                lex_skip_empty(l);
                if (l->tk == '{') {
                    /* Static block: compile the body as a hidden zero-arg
                     * function and run it once, right now, with `this` bound
                     * to the class (INSTR_STATIC_BLK). */
                    static uint32_t sblk_id = 0;
                    char fnn[40];
                    snprintf(fnn, sizeof(fnn), "@static$%u", sblk_id++);
                    bc_gen_str(bc, INSTR_FUNC, fnn);
                    PC pc = bc_reserve(bc);
                    int saved_async = g_async_depth;
                    g_async_depth = 0;
                    bool ok = stmt_block(l, bc, true);
                    g_async_depth = saved_async;
                    if (!ok) {
                        mstr_free(name);
                        return false;
                    }
                    bc_gen(bc, INSTR_RETURN);
                    bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC);
                    bc_gen(bc, INSTR_STATIC_BLK);
                    lex_skip_empty(l);
                    if (l->tk == ';') {
                        lex_chkread(l, ';');
                        lex_skip_empty(l);
                    }
                    continue;
                }
                static_member = true;
            }
            /* ES async class method: `async foo() {...}` / `async *gen() {...}`.
             * Consumed here so a computed key (`async [k]()`) can follow. */
            bool async_member = false;
            if (l->tk == LEX_R_ASYNC) {
                if (!lex_chkread(l, LEX_R_ASYNC)) {
                    mstr_free(name);
                    return false;
                }
                lex_skip_empty(l);
                if (l->tk == '(') {
                    /* A method literally named `async` (`async(){...}`): the
                     * lexer keyworded it because `(` follows. Define it as a
                     * plain method of that name. */
                    mstr_cpy(name, "async");
                    int saved_async = g_async_depth;
                    g_async_depth = 0;
                    bc_gen(bc, static_member ? INSTR_FUNC_STC : INSTR_FUNC);
                    bool ok = func_params_and_body(l, bc);
                    g_async_depth = saved_async;
                    if (!ok) {
                        mstr_free(name);
                        return false;
                    }
                    lex_skip_empty(l);
                    bc_gen_str(bc, INSTR_MEMBERN, name->cstr);
                    continue;
                }
                async_member = true;
            }
            /* Generator method with a computed key: `*[Symbol.iterator]() {}`.
             * A `*` before a plain name is handled by factor_def_func itself. */
            bool gen_member = false;
            if (l->tk == '*') {
                if (!lex_chkread(l, '*')) {
                    mstr_free(name);
                    return false;
                }
                lex_skip_empty(l);
                gen_member = true;
            }
            /* `get [expr]() {}` / `set [expr](v) {}`: accessor keyword followed
             * by a computed key - the accessor marker must be emitted before
             * the key so the stack ends up (key, fn) for INSTR_MEMBERV. */
            if (l->tk == LEX_ID &&
                (strcmp(l->tk_str->cstr, "get") == 0 ||
                 strcmp(l->tk_str->cstr, "set") == 0)) {
                lex_t sv = *l;
                mstr_t* sv_tk = l->tk_str;
                l->tk_str = mstr_new("");
                mstr_cpy(l->tk_str, sv_tk->cstr);
                lex_chkread(l, LEX_ID);
                lex_skip_empty(l);
                bool comp_acc = (l->tk == '[');
                mstr_free(l->tk_str);
                *l = sv;
                l->tk_str = sv_tk;
                if (comp_acc) {
                    bc_gen(bc, strcmp(l->tk_str->cstr, "get") == 0 ?
                               INSTR_FUNC_GET : INSTR_FUNC_SET);
                    if (!lex_chkread(l, LEX_ID)) {
                        mstr_free(name);
                        return false;
                    }
                    lex_skip_empty(l);
                }
            }
            if (l->tk == '[') {
                if (!lex_chkread(l, '[')) {
                    mstr_free(name);
                    return false;
                }
                lex_skip_empty(l);
                if (!base(l, bc)) {
                    mstr_free(name);
                    return false;
                }
                lex_skip_empty(l);
                if (!lex_chkread(l, ']')) {
                    mstr_free(name);
                    return false;
                }
                lex_skip_empty(l);
                computed_name = true;
            }
            g_async_pending = async_member ? 1 : 0;
            g_gen_pending = gen_member ? 1 : 0;
            g_static_pending = static_member ? 1 : 0;
            if (!factor_def_func(l, bc, name)) {
                g_async_pending = 0;
                g_gen_pending = 0;
                g_static_pending = 0;
                mstr_free(name);
                return false;
            }
            lex_skip_empty(l);
            if (computed_name) {
                bc_gen(bc, INSTR_MEMBERV);
            } else {
                bc_gen_str(bc, INSTR_MEMBERN, name->cstr);
            }
        }
    }
    if (!lex_chkread(l, '}')) {
        mstr_free(name);
        return false;
    }
    bc_gen(bc, INSTR_CLASS_END);

    mstr_free(name);
    return true;
}

bool factor_new(lex_t* l, bytecode_t* bc) {
    // new -> create a new object
    if (!lex_chkread(l, LEX_R_NEW)) {
        return false;
    }
    /* ES6 `new.target`: the constructor of the current invocation, or undefined
     * for a plain call. `new` is already consumed; a following `.` + `target`
     * is the meta-property, not a constructor expression. */
    if (l->tk == '.') {
        if (!lex_chkread(l, '.')) {
            return false;
        }
        if (l->tk == LEX_ID && strcmp(l->tk_str->cstr, "target") == 0) {
            if (!lex_chkread(l, LEX_ID)) {
                return false;
            }
            bc_gen_str(bc, INSTR_LOAD, "@new.target");
            return true;
        }
        return false;
    }
    /* `new (expr)(args)`: the constructor is a computed expression rather than
     * a named binding (`new (cond ? A : B)(x)`, `new (function(){...})()`).
     * Compile the expression, then the argument list; NEWX picks the
     * constructor value off the stack (the CALLX equivalent for [[Construct]]). */
    if (l->tk == '(') {
        if (!lex_chkread(l, '(')) {
            return false;
        }
        lex_skip_empty(l);
        if (!base(l, bc)) {
            return false;
        }
        /* TS-emitted helpers parenthesise a comma expression as the
         * constructor: `new (n = void 0, n = Promise)(fn)`. Evaluate every
         * operand, discarding all but the last (the constructor value). */
        while (l->tk == ',') {
            if (!lex_chkread(l, ',')) {
                return false;
            }
            bc_gen(bc, INSTR_POP);
            lex_skip_empty(l);
            if (!base(l, bc)) {
                return false;
            }
        }
        lex_skip_empty(l);
        if (!lex_chkread(l, ')')) {
            return false;
        }
        /* `new (expr).member(args)`: a parenthesised base followed by a member
         * chain is still ONE constructor MemberExpression. JS binds the trailing
         * `.r(700)` to the `new` - i.e. `new ((n(m)).r)(700)`, NOT
         * `(new (n(m))).r(700)`. Without walking the chain here the compiler
         * constructed from the parenthesised value with zero args and left
         * `.r(args)` as a method call on the freshly built instance, which only
         * carries a `prototype` member ("can not find function 'r' on
         * object{prototype}" - pinterest's `new (n(845600)).r(700)`). Mirror the
         * `new A.b.c(args)` member walk below. */
        while (l->tk == '.' || l->tk == '[') {
            if (l->tk == '.') {
                if (!lex_chkread(l, '.')) {
                    return false;
                }
                if (l->tk != LEX_ID &&
                    !(l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END)) {
                    return false;
                }
                mstr_t* mem = mstr_new(l->tk_str->cstr);
                int tk = l->tk;
                if (!lex_chkread(l, tk)) {
                    mstr_free(mem);
                    return false;
                }
                bc_gen_str(bc, INSTR_GET, mem->cstr);
                mstr_free(mem);
            } else {
                if (!lex_chkread(l, '[')) {
                    return false;
                }
                if (!base(l, bc)) {
                    return false;
                }
                if (!lex_chkread(l, ']')) {
                    return false;
                }
                bc_gen(bc, INSTR_ARRAY_AT);
            }
        }
        int arg_num = 0;
        bool has_spread = false;
        if (l->tk == '(') {
            arg_num = call_func(l, bc, &has_spread);
            if (arg_num < 0) {
                return false;
            }
        }
        if (has_spread) {
            bc_gen_str(bc, INSTR_NEWX_SPREAD, "");
        } else {
            mstr_t* s = mstr_new("");
            gen_func_name("", arg_num, s);
            bc_gen_str(bc, INSTR_NEWX, s->cstr);
            mstr_free(s);
        }
        return true;
    }
    /* `new function(args){...}` / `new function name(args){...}`: an anonymous
     * (or named) function expression as the constructor - ES5 allows any
     * MemberExpression after `new`. Compile the function value, then the
     * optional argument list; NEWX constructs from the value on the stack. */
    if (l->tk == LEX_R_FUNCTION) {
        if (!lex_chkread(l, LEX_R_FUNCTION)) {
            return false;
        }
        mstr_t* fname = mstr_new("");
        if (!factor_def_func(l, bc, fname)) {
            mstr_free(fname);
            return false;
        }
        mstr_free(fname);
        int arg_num = 0;
        bool has_spread = false;
        if (l->tk == '(') {
            arg_num = call_func(l, bc, &has_spread);
            if (arg_num < 0) {
                return false;
            }
        }
        if (has_spread) {
            bc_gen_str(bc, INSTR_NEWX_SPREAD, "");
        } else {
            mstr_t* s = mstr_new("");
            gen_func_name("", arg_num, s);
            bc_gen_str(bc, INSTR_NEWX, s->cstr);
            mstr_free(s);
        }
        return true;
    }
    /* `new class {...}` / `new class extends B {...}`: an anonymous class
     * expression as the constructor (chat bundles emit `new class{send(e){}}`).
     * factor_def_class leaves the class value on the stack, so only the
     * optional argument list plus the NEWX construct remains. */
    if (l->tk == LEX_R_CLASS) {
        if (!factor_def_class(l, bc)) {
            return false;
        }
        int arg_num = 0;
        bool has_spread = false;
        if (l->tk == '(') {
            arg_num = call_func(l, bc, &has_spread);
            if (arg_num < 0) {
                return false;
            }
        }
        if (has_spread) {
            bc_gen_str(bc, INSTR_NEWX_SPREAD, "");
        } else {
            mstr_t* s = mstr_new("");
            gen_func_name("", arg_num, s);
            bc_gen_str(bc, INSTR_NEWX, s->cstr);
            mstr_free(s);
        }
        return true;
    }
    mstr_t* class_name = mstr_new("");
    mstr_cpy(class_name, l->tk_str->cstr);

    if (!lex_chkread(l, LEX_ID)) {
        mstr_free(class_name);
        return false;
    }
    /* `new A.b(args)` / `new A.b.c(args)`: a member expression as the
     * constructor (webpack harmony imports call `new _mod__.navigation(...)`).
     * Load the base, walk the chain with member reads so the constructor value
     * ends up on the stack, then construct it NEWX-style. Without this the
     * code below emitted `new A` and left `.b(args)` to parse as a method
     * call on the fresh object - "can not find function 'b'" on w3.org. */
    if (l->tk == '.' || l->tk == '[') {
        bc_gen_str(bc, INSTR_LOAD, class_name->cstr);
        while (l->tk == '.' || l->tk == '[') {
            if (l->tk == '.') {
                if (!lex_chkread(l, '.')) {
                    mstr_free(class_name);
                    return false;
                }
                /* Reserved words are valid property names: bundlers call
                 * `new mod.default(...)` for default-exported constructors. */
                if (l->tk != LEX_ID &&
                    !(l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END)) {
                    mstr_free(class_name);
                    return false;
                }
                mstr_t* mem = mstr_new(l->tk_str->cstr);
                int tk = l->tk;
                if (!lex_chkread(l, tk)) {
                    mstr_free(mem);
                    mstr_free(class_name);
                    return false;
                }
                bc_gen_str(bc, INSTR_GET, mem->cstr);
                mstr_free(mem);
            } else {
                /* computed constructor member: `new mod["default"](...)` */
                if (!lex_chkread(l, '[')) {
                    mstr_free(class_name);
                    return false;
                }
                if (!base(l, bc)) {
                    mstr_free(class_name);
                    return false;
                }
                if (!lex_chkread(l, ']')) {
                    mstr_free(class_name);
                    return false;
                }
                bc_gen(bc, INSTR_ARRAY_AT);
            }
        }
        int arg_num = 0;
        bool has_spread = false;
        if (l->tk == '(') {
            arg_num = call_func(l, bc, &has_spread);
            if (arg_num < 0) {
                mstr_free(class_name);
                return false;
            }
        }
        if (has_spread) {
            bc_gen_str(bc, INSTR_NEWX_SPREAD, "");
        } else {
            mstr_t* s = mstr_new("");
            gen_func_name("", arg_num, s);
            bc_gen_str(bc, INSTR_NEWX, s->cstr);
            mstr_free(s);
        }
        mstr_free(class_name);
        return true;
    }
    if (l->tk == '(') {
        bool has_spread = false;
        int arg_num = call_func(l, bc, &has_spread);
        if (arg_num < 0) {
            mstr_free(class_name);
            return false;
        }
        if (has_spread) {
            bc_gen_str(bc, INSTR_NEW_SPREAD, class_name->cstr);
        } else {
            mstr_t* s = mstr_new("");
            gen_func_name(class_name->cstr, arg_num, s);
            bc_gen_str(bc, INSTR_NEW, s->cstr);
            mstr_free(s);
        }
    } else {
        /* `new Foo` with no argument list: a NewExpression may omit Arguments
         * entirely, and means exactly `new Foo()`. Emitting nothing here (the
         * old behaviour) left the value stack one slot short, so the expression
         * evaluated to undefined AND desynced every later operand of the
         * enclosing statement - `console.log(x, (new Date))` then reported
         * "can not find function 'log'", and fontfaceobserver.js's
         * `(new Date).getTime()` cascaded into 'getTime'/'all'/'then' failures
         * on w3.org. gen_func_name with arg_num 0 yields the bare name, the
         * same payload `new Foo()` produces. */
        mstr_t* s = mstr_new("");
        gen_func_name(class_name->cstr, 0, s);
        bc_gen_str(bc, INSTR_NEW, s->cstr);
        mstr_free(s);
    }
    mstr_free(class_name);
    return true;
}

bool factor_json(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, '{')) {
        return false;
    }
    bc_gen(bc, INSTR_OBJ);
    lex_skip_empty(l);
    while (l->tk != '}') {
        lex_skip_empty(l);
        if (l->tk == '}') {
            break;
        }

        // ES2017 async method: {async name(params){body}}. `async` is only a
        // method modifier when the *next* token can start a property name
        // (ID / string / computed `[` / generator `*`). Otherwise it is an
        // ordinary property key, e.g. {async: true} or shorthand {async}, and
        // must fall through to the reserved-word-as-key branch below.
        bool is_async_method = false;
        if (l->tk == LEX_R_ASYNC) {
            lex_t asv = *l;                 // shallow copy: keeps original tk_str ptr
            mstr_t* asv_tk_str = l->tk_str; // peeking must not clobber caller token
            l->tk_str = mstr_new("");
            mstr_cpy(l->tk_str, asv_tk_str->cstr);
            lex_chkread(l, LEX_R_ASYNC);
            lex_skip_empty(l);
            bool looks_like_method =
                (l->tk == LEX_ID || l->tk == LEX_STR ||
                 l->tk == '[' || l->tk == '*');
            mstr_free(l->tk_str);
            *l = asv;                       // restore original position + tk_str
            if (looks_like_method) {
                lex_chkread(l, LEX_R_ASYNC);
                lex_skip_empty(l);
                is_async_method = true;
            }
        }

        // ES6 generator method: {*name(params){body}} / {*[expr](params){body}}.
        // After `{` or `,` a leading `*` can only mark a generator method (the
        // object-spread form is `...`, which starts with `.`).
        bool is_gen_method = false;
        if (l->tk == '*') {
            lex_chkread(l, '*');
            lex_skip_empty(l);
            is_gen_method = true;
        }

        // ES6 object spread: {...expr}
        if (l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
            lex_get_nextch(l); // -> 3rd '.'
            lex_get_nextch(l); // -> first char of expr
            lex_get_next_token(l);
            if (!base(l, bc)) {
                return false;
            }
            bc_gen(bc, INSTR_OBJ_SPREAD);
            lex_skip_empty(l);
            if (l->tk != '}') {
                if (!lex_chkread(l, ',')) {
                    return false;
                }
            }
            lex_skip_empty(l);
            continue;
        }

        // ES6 computed key: {[expr]: value} or a method {[expr](params){...}}
        if (l->tk == '[') {
            if (!lex_chkread(l, '[')) {
                return false;
            }
            lex_skip_empty(l);
            if (!base(l, bc)) { // push key value
                return false;
            }
            if (!lex_chkread(l, ']')) {
                return false;
            }
            lex_skip_empty(l);
            if (l->tk == '(') {
                /* Method shorthand with a computed key: the key value is
                 * already on the stack; define the function (pushed above it)
                 * then bind it with MEMBERV. */
                bc_gen(bc, is_gen_method ? INSTR_FUNC_GEN : INSTR_FUNC);
                int saved_async = g_async_depth;
                g_async_depth = is_async_method ? 1 : 0;
                bool body_ok = func_params_and_body(l, bc);
                g_async_depth = saved_async;
                if (!body_ok) {
                    return false;
                }
                bc_gen(bc, INSTR_MEMBERV); // scope-obj[key] = method
            } else {
                if (!lex_chkread(l, ':')) {
                    return false;
                }
                lex_skip_empty(l);
                if (!base(l, bc)) { // push member value
                    return false;
                }
                bc_gen(bc, INSTR_MEMBERV); // scope-obj[key] = value
            }
            lex_skip_empty(l);
            if (l->tk != '}') {
                if (!lex_chkread(l, ',')) {
                    return false;
                }
            }
            lex_skip_empty(l);
            continue;
        }

        mstr_t* id = mstr_new(l->tk_str->cstr);
        // we only allow strings or IDs on the left hand side of an initialisation
        if (l->tk == LEX_STR) {
            if (!lex_chkread(l, LEX_STR)) {
                mstr_free(id);
                return false;
            }
        } else if (l->tk == LEX_ID) {
            if (!lex_chkread(l, LEX_ID)) {
                mstr_free(id);
                return false;
            }
        } else if (l->tk == LEX_INT || l->tk == LEX_FLOAT) {
            /* Numeric literal key: {0:"a"}, {1.5:"x"}, {529e3:f} (common in
             * webpack module maps). JS applies ToPropertyKey to the number, i.e.
             * its CANONICAL string form, not the source text: {529e3:f} keys
             * "529000", {0x10:f} keys "16". Webpack ids are written in
             * scientific/hex form (`{529e3(e,t,n){...}}` required via
             * `n(529e3)`), so keeping the raw token "529e3" left the factory
             * under a key the float require (`n[529000]` -> "529000") never
             * matched - the module resolved to undefined and `.call` threw
             * ("can not find function 'call' on object{}", pinterest www/index).
             * Canonicalise exactly like bc_gen_str parses the literal and
             * var_to_str renders the resulting value. */
            int tk = l->tk;
            char text[128];
            snprintf(text, sizeof(text), "%s", id->cstr);
            if (tk == LEX_INT) {
                if (strstr(text, "0x") != NULL || strstr(text, "0X") != NULL) {
                    errno = 0;
                    unsigned long long h = strtoull(text, NULL, 16);
                    if (errno == ERANGE || h > 0x7FFFFFFFFFFFFFFFULL)
                        mstr_cpy(id, mstr_from_float64(strtod(text, NULL)));
                    else
                        mstr_cpy(id, mstr_from_int64((int64_t)h, 10));
                } else {
                    errno = 0;
                    long long ll = strtoll(text, NULL, 10);
                    if (errno == ERANGE)
                        mstr_cpy(id, mstr_from_float64(strtod(text, NULL)));
                    else
                        mstr_cpy(id, mstr_from_int64((int64_t)ll, 10));
                }
            } else {
                mstr_cpy(id, mstr_from_float64(strtod(text, NULL)));
            }
            if (!lex_chkread(l, tk)) {
                mstr_free(id);
                return false;
            }
        } else if (l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END) {
            /* A reserved word used as an object-literal key: {in:1}, {delete:1},
             * {new:1}, {if:1}. JS permits keywords as property names; `id`
             * already captured the word text from tk_str above, so just consume
             * the token exactly like an identifier. */
            int tk = l->tk;
            if (!lex_chkread(l, tk)) {
                mstr_free(id);
                return false;
            }
        } else {
            mstr_free(id);
            return false;
        }
        lex_skip_empty(l);

        /* ES6 accessor in an object literal: `get name() {...}` /
         * `set name(v) {...}`. Told apart from a method or property literally
         * named get/set by requiring a property-name token (ID, string,
         * number or reserved word) to follow the keyword. */
        if ((strcmp(id->cstr, "get") == 0 || strcmp(id->cstr, "set") == 0) &&
            (l->tk == LEX_ID || l->tk == LEX_STR || l->tk == LEX_INT || l->tk == LEX_FLOAT ||
             (l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END))) {
            bool is_get = (strcmp(id->cstr, "get") == 0);
            mstr_t* prop = mstr_new(l->tk_str->cstr);
            LEX_TYPES kt = (LEX_TYPES)l->tk;
            if (!lex_chkread(l, kt)) {
                mstr_free(id);
                mstr_free(prop);
                return false;
            }
            lex_skip_empty(l);
            if (l->tk != '(') {
                mstr_free(id);
                mstr_free(prop);
                return false;
            }
            bc_gen(bc, is_get ? INSTR_FUNC_GET : INSTR_FUNC_SET);
            int saved_async = g_async_depth;
            g_async_depth = 0;
            bool body_ok = func_params_and_body(l, bc);
            g_async_depth = saved_async;
            if (!body_ok) {
                mstr_free(id);
                mstr_free(prop);
                return false;
            }
            lex_skip_empty(l);
            bc_gen_str(bc, INSTR_MEMBERN, prop->cstr);
            mstr_free(id);
            mstr_free(prop);
            if (l->tk != '}') {
                if (!lex_chkread(l, ',')) {
                    return false;
                }
            }
            lex_skip_empty(l);
            continue;
        }

        if (l->tk == ':') { // normal property  key: value
            if (!lex_chkread(l, ':')) {
                mstr_free(id);
                return false;
            }
            lex_skip_empty(l);
            if (!base(l, bc)) {
                mstr_free(id);
                return false;
            }
            /* ES6 `__proto__: v` in an object literal sets the object's
             * [[Prototype]] instead of defining an own "__proto__" property. */
            if (strcmp(id->cstr, "__proto__") == 0) {
                bc_gen(bc, INSTR_SET_PROTO);
                mstr_free(id);
                lex_skip_empty(l);
                if (l->tk != '}') {
                    if (!lex_chkread(l, ',')) {
                        return false;
                    }
                }
                lex_skip_empty(l);
                continue;
            }
        } else if (l->tk == '(') { // ES6 method shorthand: name(params){body}
            bc_gen(bc, is_gen_method ? INSTR_FUNC_GEN : INSTR_FUNC);
            // func_params_and_body reads g_async_depth, so establish the async
            // context for this body directly (mirrors factor_def_func) and
            // restore it afterwards so nested definitions stay synchronous.
            int saved_async = g_async_depth;
            g_async_depth = is_async_method ? 1 : 0;
            bool body_ok = func_params_and_body(l, bc);
            g_async_depth = saved_async;
            if (!body_ok) {
                mstr_free(id);
                return false;
            }
        } else { // ES6 property shorthand: {x} == {x: x}
            bc_gen_str(bc, INSTR_LOAD, id->cstr);
        }
        lex_skip_empty(l);
        bc_gen_str(bc, INSTR_MEMBERN, id->cstr);
        if (l->tk != '}') {
            if (!lex_chkread(l, ',')) {
                mstr_free(id);
                return false;
            }
        }
        mstr_free(id);
        lex_skip_empty(l);
    }
    bc_gen(bc, INSTR_OBJ_END);
    return lex_chkread(l, '}');
}

bool factor_array(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, '[')) {
        return false;
    }
    bc_gen(bc, INSTR_ARRAY);
    lex_skip_empty(l);
    while (l->tk != ']') {
        lex_skip_empty(l);
        if (l->tk == ']') {
            break;
        }
        // ES6 array spread: ...expr
        if (l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
            lex_get_nextch(l); // -> 3rd '.'
            lex_get_nextch(l); // -> first char of expr
            lex_get_next_token(l);
            if (!base(l, bc)) {
                return false;
            }
            bc_gen(bc, INSTR_ARR_SPREAD);
        } else if (l->tk == ',') {
            // Array elision (hole): [,,3] / [1,,3]. A `,` at element position
            // means an empty slot; push an undefined placeholder so indices and
            // length stay correct. The trailing-comma step below consumes it.
            bc_gen(bc, INSTR_UNDEF);
            bc_gen(bc, INSTR_MEMBER);
        } else {
            if (!base(l, bc)) {
                return false;
            }
            bc_gen(bc, INSTR_MEMBER);
        }
        lex_skip_empty(l);
        if (l->tk != ']') {
            if (!lex_chkread(l, ',')) {
                return false;
            }
        }
        lex_skip_empty(l);
    }
    if (!lex_chkread(l, ']')) {
        return false;
    }
    bc_gen(bc, INSTR_ARRAY_END);
    return true;
}

bool factor_call_func(lex_t* l, bytecode_t* bc, mstr_t* name, bool member) {
    mstr_t* s = mstr_new("");

    bool has_spread = false;
    int arg_num = call_func(l, bc, &has_spread);
    if (arg_num < 0) {
        mstr_free(s);
        return false;
    }
    if (has_spread) {
        bc_gen_str(bc, member ? INSTR_CALLO_SPREAD : INSTR_CALL_SPREAD, name->cstr);
    } else {
        gen_func_name(name->cstr, arg_num, s);
        bc_gen_str(bc, member ? INSTR_CALLO : INSTR_CALL, s->cstr);
    }
    mstr_free(s);
    return true;
}

bool factor_array_access(lex_t* l, bytecode_t* bc, mstr_t* name, bool member) {
    bc_gen_str(bc, member ? INSTR_GET : INSTR_LOAD, name->cstr);

    if (!lex_chkread(l, '[')) {
        return false;
    }
    if (!base(l, bc)) {
        return false;
    }
    if (!lex_chkread(l, ']')) {
        return false;
    }
    /* `name[key](...)`: keep the receiver so the following call binds `this`. */
    if (l->tk == '(') {
        bc_gen(bc, INSTR_ARRAY_AT_M);
        g_arrat_recv = 1;
    } else {
        bc_gen(bc, INSTR_ARRAY_AT);
    }
    return true;
}

/** ES6 template literal: `text ${expr} text`.
 *  The opening backtick has already been consumed by the lexer as a char
 *  token, so l->curr_ch points at the first content character. We scan the
 *  raw character stream, emitting string chunks and compiling embedded
 *  expressions. Concatenation is done with INSTR_PLUS, always keeping the
 *  accumulated string as the left operand (the VM requires the left operand
 *  to be a string for concatenation). */
bool factor_template(lex_t* l, bytecode_t* bc) {
    mstr_t* chunk = mstr_new("");
    bool first = true; // whether the leading string piece has been emitted

    while (true) {
        char c = l->curr_ch;
        if (c == 0) { // unterminated template literal
            mstr_free(chunk);
            return false;
        }
        if (c == '`') { // closing backtick: flush the trailing chunk
            if (first) {
                bc_gen_str(bc, INSTR_STR, chunk->cstr);
            } else {
                bc_gen_str(bc, INSTR_STR, chunk->cstr);
                bc_gen(bc, INSTR_PLUS);
            }
            lex_get_nextch(l);
            break;
        }
        if (c == '$' && l->next_ch == '{') { // embedded expression
            if (first) {
                bc_gen_str(bc, INSTR_STR, chunk->cstr);
                first = false;
            } else {
                bc_gen_str(bc, INSTR_STR, chunk->cstr);
                bc_gen(bc, INSTR_PLUS);
            }
            mstr_reset(chunk);
            lex_get_nextch(l); // consume '$'
            lex_get_nextch(l); // consume '{'
            lex_get_next_token(l); // first token of the expression
            if (!base(l, bc)) {
                mstr_free(chunk);
                return false;
            }
            // base() stops at '}', the expression value is on the stack.
            bc_gen(bc, INSTR_PLUS);
            continue;
        }
        if (c == '\\') { // escape sequence
            lex_get_nextch(l);
            switch (l->curr_ch) {
                case 'n': mstr_add(chunk, '\n'); break;
                case 't': mstr_add(chunk, '\t'); break;
                case 'r': mstr_add(chunk, '\r'); break;
                case 'b': mstr_add(chunk, '\b'); break;
                case 'f': mstr_add(chunk, '\f'); break;
                case '0': mstr_add(chunk, '\0'); break;
                case '`': mstr_add(chunk, '`'); break;
                case '$': mstr_add(chunk, '$'); break;
                case '\\': mstr_add(chunk, '\\'); break;
                default: mstr_add(chunk, l->curr_ch);
            }
            lex_get_nextch(l);
            continue;
        }
        mstr_add(chunk, c);
        lex_get_nextch(l);
    }

    mstr_free(chunk);
    lex_get_next_token(l); // resume normal token stream after the template
    return true;
}

/* Skip a `${...}` substitution (no compilation). Assumes l->curr_ch is the '$'.
 * On return l->curr_ch is the char just past the matching '}'.
 * Token-based on purpose: the former character-level scan treated any quote
 * inside the substitution as a string opener, so a regex literal holding a bare
 * quote (`${s.replace(/'/g,"''")}`) swallowed the rest of the clause and the
 * two-pass switch compiler desynced, failing far away from the real cause.
 * skip_advance() already knows regex-vs-division context, strings and nested
 * templates, so the lexer decides what a quote means. */
static bool skip_advance(lex_t* l, uint32_t* prev);
static void skip_template_subst(lex_t* l) {
    lex_get_nextch(l); // consume '$' -> curr_ch == '{'
    lex_get_nextch(l); // consume '{' -> curr_ch == first char inside
    lex_get_next_token(l); // first token of the substitution
    int depth = 1;
    uint32_t prev = 0; // regex context: a leading '/' is a regex literal
    while (l->tk != LEX_EOF && depth > 0) {
        if (l->tk == '{') {
            depth++;
        } else if (l->tk == '}') {
            depth--;
            if (depth <= 0) {
                /* The '}' token is consumed; the lexer already advanced
                 * curr_ch to the character right after it. */
                return;
            }
        }
        if (!skip_advance(l, &prev)) {
            return;
        }
    }
}

/** ES6 tagged template: `tag`text ${expr} text``. The tag callable is already
 *  on the value stack. We build the cooked `strings` array and its `raw`
 *  array (static text, emitted first), then compile the substitution
 *  expressions, then invoke the tag with (strings, ...values) via CALLX.
 *  l->tk == '`' has been consumed; l->curr_ch points at the first content char. */
bool factor_tagged_template(lex_t* l, bytecode_t* bc) {
    lex_t saved = *l; // template-start lexer state (tk_str untouched by pass 1)

    // ---- PASS 1: collect cooked + raw chunks (character level) ----
    mstr_t* cooked[64];
    mstr_t* raw[64];
    int nchunks = 0;
    mstr_t* ck = mstr_new("");
    mstr_t* rw = mstr_new("");
    while (true) {
        char c = l->curr_ch;
        if (c == 0) {
            mstr_free(ck);
            mstr_free(rw);
            return false;
        }
        if (c == '`') {
            lex_get_nextch(l);
            break;
        }
        if (c == '$' && l->next_ch == '{') {
            if (nchunks < 64) {
                cooked[nchunks] = ck;
                raw[nchunks] = rw;
                nchunks++;
                ck = mstr_new("");
                rw = mstr_new("");
            } else {
                mstr_reset(ck);
                mstr_reset(rw);
            }
            skip_template_subst(l);
            continue;
        }
        if (c == '\\') {
            char e = l->next_ch;
            mstr_add(rw, '\\');
            mstr_add(rw, e);
            lex_get_nextch(l); // consume '\\' -> curr_ch == e
            switch (e) {
                case 'n': mstr_add(ck, '\n'); break;
                case 't': mstr_add(ck, '\t'); break;
                case 'r': mstr_add(ck, '\r'); break;
                case 'b': mstr_add(ck, '\b'); break;
                case 'f': mstr_add(ck, '\f'); break;
                case '0': mstr_add(ck, '\0'); break;
                case '`': mstr_add(ck, '`'); break;
                case '$': mstr_add(ck, '$'); break;
                case '\\': mstr_add(ck, '\\'); break;
                default: mstr_add(ck, e);
            }
            lex_get_nextch(l); // consume e
            continue;
        }
        mstr_add(ck, c);
        mstr_add(rw, c);
        lex_get_nextch(l);
    }
    if (nchunks < 64) {
        cooked[nchunks] = ck;
        raw[nchunks] = rw;
        nchunks++;
    } else {
        mstr_free(ck);
        mstr_free(rw);
    }

    // ---- emit cooked `strings` array, then `raw` array, then attach ----
    bc_gen(bc, INSTR_ARRAY);
    for (int i = 0; i < nchunks; i++) {
        bc_gen_str(bc, INSTR_STR, cooked[i]->cstr);
        bc_gen(bc, INSTR_MEMBER);
    }
    bc_gen(bc, INSTR_ARRAY_END);
    bc_gen(bc, INSTR_ARRAY);
    for (int i = 0; i < nchunks; i++) {
        bc_gen_str(bc, INSTR_STR, raw[i]->cstr);
        bc_gen(bc, INSTR_MEMBER);
    }
    bc_gen(bc, INSTR_ARRAY_END);
    bc_gen(bc, INSTR_TAG_RAW);

    for (int i = 0; i < nchunks; i++) {
        mstr_free(cooked[i]);
        mstr_free(raw[i]);
    }

    int nvals = nchunks > 0 ? nchunks - 1 : 0;

    // ---- PASS 2: compile the substitution expressions in order ----
    *l = saved;
    while (true) {
        char c = l->curr_ch;
        if (c == 0) {
            return false;
        }
        if (c == '`') {
            lex_get_nextch(l);
            break;
        }
        if (c == '$' && l->next_ch == '{') {
            lex_get_nextch(l); // '$'
            lex_get_nextch(l); // '{'
            lex_get_next_token(l);
            if (!base(l, bc)) {
                return false;
            }
            continue;
        }
        if (c == '\\') {
            lex_get_nextch(l);
            if (l->curr_ch) {
                lex_get_nextch(l);
            }
            continue;
        }
        lex_get_nextch(l);
    }
    lex_get_next_token(l); // resume normal token stream

    // ---- invoke tag(strings, ...values) ----
    mstr_t* s = mstr_new("");
    gen_func_name("", nvals + 1, s);
    bc_gen_str(bc, INSTR_CALLX, s->cstr);
    mstr_free(s);
    return true;
}

/* Scan the tail of a regex literal. Entered right after factor() saw '/'
 * (or '/=', when the pattern begins with '='): the lexer has consumed the
 * opening token, so curr_ch is the first pattern character. '/' inside a
 * [class] does not terminate, '\' escapes the next char, a newline or EOF
 * is a syntax error. Trailing letters are the flags. */
static bool lex_scan_regex(lex_t* l, mstr_t* pat, mstr_t* flags) {
    bool in_class = false;
    while (true) {
        char c = l->curr_ch;
        if (c == 0 || c == '\n') {
            if (!g_hoist_quiet) {
                mario_printf("unterminated regex literal! ");
                compile_error_pos(l, -1);
            }
            return false;
        }
        if (c == '\\') {
            mstr_add(pat, c);
            lex_get_nextch(l);
            if (l->curr_ch == 0) {
                if (!g_hoist_quiet) {
                    mario_printf("unterminated regex literal! ");
                    compile_error_pos(l, -1);
                }
                return false;
            }
            mstr_add(pat, l->curr_ch);
            lex_get_nextch(l);
            continue;
        }
        if (c == '[')
            in_class = true;
        else if (c == ']')
            in_class = false;
        else if (c == '/' && !in_class) {
            lex_get_nextch(l);
            break;
        }
        mstr_add(pat, c);
        lex_get_nextch(l);
    }
    while (is_alpha(l->curr_ch)) {
        mstr_add(flags, l->curr_ch);
        lex_get_nextch(l);
    }
    lex_get_next_token(l);
    return true;
}

/* Lookahead: does the parenthesised group starting at the current '(' token
 * form an arrow-function parameter list, i.e. is its matching ')' immediately
 * followed by `=>`? The lexer state is fully restored, so the caller can then
 * parse the group either as arrow parameters or as a comma expression. The two
 * need different code: a comma expression must emit POP between operands to
 * keep only the last value, but an arrow parameter list must NOT (func_def
 * scans the operands' name strings until the body JMP, and a stray POP would
 * truncate the parameter list). */
static bool paren_group_is_arrow(lex_t* l) {
    if (l->tk != '(') {
        return false;
    }
    lex_t saved = *l;                 // shallow copy: keeps original tk_str ptr
    mstr_t* saved_tk_str = l->tk_str; // do not let scanning clobber caller token
    l->tk_str = mstr_new("");
    mstr_cpy(l->tk_str, saved_tk_str->cstr);

    bool is_arrow = false;
    int depth = 0;
    while (true) {
        if (l->tk == LEX_EOF) {
            break;
        }
        if (l->tk == '(') {
            depth++;
        } else if (l->tk == ')') {
            depth--;
            if (depth <= 0) {
                lex_get_next_token(l); // token right after the matching ')'
                lex_skip_empty(l);
                is_arrow = (l->tk == LEX_R_AFUNCTION);
                break;
            }
        }
        lex_get_next_token(l);
    }

    mstr_free(l->tk_str);
    *l = saved; // restore scalar state and original tk_str pointer
    return is_arrow;
}

bool factor(lex_t* l, bytecode_t* bc, bool member) {
    if (member && l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END) {
        /* A reserved word used as a property name after '.', e.g. obj.return(),
         * gen.throw(), obj.new, obj.class. JS permits keywords as member names;
         * treat the keyword token exactly like an identifier and fall through to
         * the postfix chain so `a.return(x).value` keeps chaining. */
        mstr_t* name = mstr_new(l->tk_str->cstr);
        int tk = l->tk;
        if (!lex_chkread(l, tk)) {
            mstr_free(name);
            return false;
        }
        if (l->tk == '(') { // method call: obj.return(...)
            if (!factor_call_func(l, bc, name, member)) {
                mstr_free(name);
                return false;
            }
        } else if (l->tk == '[') { // obj.return[...]
            factor_array_access(l, bc, name, member);
        } else {
            bc_gen_str(bc, INSTR_GET, name->cstr);
        }
        mstr_free(name);
    } else if (l->tk == '(') {
        PC pc = bc_reserve(bc);
        /* Decide NOW whether this group is an arrow parameter list or a comma
         * expression. They need entirely different code: an arrow parameter list
         * is parsed by the shared ES6 parameter parser (func_parse_params) so it
         * accepts rest/default/destructuring parameters and emits the arg-name
         * LOADs that func_def scans, whereas a comma expression parses each
         * operand with base() and emits value-discarding POPs between them (a
         * POP inside an arrow parameter list would corrupt func_def's scan). */
        bool is_arrow = paren_group_is_arrow(l);
        if (is_arrow) {
            m_array_t defaults;
            array_init(&defaults);
            m_array_t pdestrs;
            array_init(&pdestrs);
            mstr_t* rest_name = NULL;
            int positional = 0;
            if (!func_parse_params(l, bc, &defaults, &pdestrs, &rest_name, &positional)) {
                array_clean(&defaults, free_param_default);
                array_clean(&pdestrs, free_param_destr);
                if (rest_name != NULL) mstr_free(rest_name);
                return false;
            }
            if (!lex_chkread(l, LEX_R_AFUNCTION)) {
                array_clean(&defaults, free_param_default);
                array_clean(&pdestrs, free_param_destr);
                if (rest_name != NULL) mstr_free(rest_name);
                return false;
            }
            bc_set_instr(bc, pc, INSTR_FUNC_ARROW, 0);
            bool ok = factor_def_afunc_ex(l, bc, &defaults, &pdestrs, rest_name, positional);
            array_clean(&defaults, free_param_default);
            array_clean(&pdestrs, free_param_destr);
            if (rest_name != NULL) mstr_free(rest_name);
            if (!ok) {
                return false;
            }
        } else {
            if (!lex_chkread(l, '(')) {
                return false;
            }
            lex_skip_empty(l);
            if (l->tk != ')') {
                if (!base(l, bc)) {
                    return false;
                }
                lex_skip_empty(l);
                while (l->tk == ',') {
                    if (!lex_chkread(l, ',')) {
                        return false;
                    }
                    /* Comma expression: drop the previous operand's value so
                     * only the last one survives. */
                    bc_gen(bc, INSTR_POP);
                    if (!base(l, bc)) {
                        return false;
                    }
                    lex_skip_empty(l);
                }
            }
            if (!lex_chkread(l, ')')) {
                return false;
            }
            bc_remove_instr(bc, pc, 1);
        }
    } else if (l->tk == LEX_R_TRUE) {
        if (!lex_chkread(l, LEX_R_TRUE)) {
            return false;
        }
        bc_gen(bc, INSTR_TRUE);
    } else if (l->tk == LEX_R_FALSE) {
        if (!lex_chkread(l, LEX_R_FALSE)) {
            return false;
        }
        bc_gen(bc, INSTR_FALSE);
    } else if (l->tk == LEX_R_NULL) {
        if (!lex_chkread(l, LEX_R_NULL)) {
            return false;
        }
        bc_gen(bc, INSTR_NULL);
    } else if (l->tk == LEX_R_UNDEFINED) {
        if (!lex_chkread(l, LEX_R_UNDEFINED)) {
            return false;
        }
        bc_gen(bc, INSTR_UNDEF);
    } else if (l->tk == LEX_INT) {
        bc_gen_str(bc, INSTR_INT, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_INT)) {
            return false;
        }
    } else if (l->tk == LEX_FLOAT) {
        bc_gen_str(bc, INSTR_FLOAT, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_FLOAT)) {
            return false;
        }
    } else if (l->tk == LEX_BIGINT) {
        /* BigInt literal: pool the digit string (with any 0x/0b/0o prefix) as the
         * INSTR_BIGINT payload; handle_bigint parses it into a bignum at run time. */
        bc_gen_str(bc, INSTR_BIGINT, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_BIGINT)) {
            return false;
        }
    } else if (l->tk == LEX_STR) {
        bc_gen_str(bc, INSTR_STR, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_STR)) {
            return false;
        }
    } else if (!member && (l->tk == '/' || l->tk == LEX_DIVEQUAL)) {
        /* A '/' where an expression is expected can only start a regex
         * literal (division needs a left operand, handled in term()). It is
         * compiled as `new RegExp(pattern, flags)` so the whole engine lives
         * in the RegExp native. A '/=' here is a pattern starting with '='. */
        mstr_t* pat = mstr_new("");
        mstr_t* flags = mstr_new("");
        if (l->tk == LEX_DIVEQUAL)
            mstr_add(pat, '=');
        if (!lex_scan_regex(l, pat, flags)) {
            mstr_free(pat);
            mstr_free(flags);
            return false;
        }
        bc_gen_str(bc, INSTR_STR, pat->cstr);
        bc_gen_str(bc, INSTR_STR, flags->cstr);
        bc_gen_str(bc, INSTR_NEW, "RegExp$2");
        mstr_free(pat);
        mstr_free(flags);
    } else if (l->tk == '`') { // ES6 template literal
        if (!factor_template(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_ASYNC) { // ES async function / async arrow
        if (!lex_chkread(l, LEX_R_ASYNC)) {
            return false;
        }
        lex_skip_empty(l);
        if (l->tk == LEX_R_FUNCTION) {
            if (!lex_chkread(l, LEX_R_FUNCTION)) {
                return false;
            }
            mstr_t* fname = mstr_new("");
            g_async_pending = 1;
            g_func_selfname = 1;
            factor_def_func(l, bc, fname);
            g_func_selfname = 0;
            mstr_free(fname);
        } else {
            /* async arrow: `async (a, b) => ...` or `async a => ...`. Parse the
             * arrow through factor(); g_async_pending marks the body async. */
            g_async_pending = 1;
            if (!factor(l, bc, member)) {
                g_async_pending = 0;
                return false;
            }
        }
    } else if (l->tk == LEX_R_FUNCTION) { //define function
        if (!lex_chkread(l, LEX_R_FUNCTION)) {
            return false;
        }
        mstr_t* fname = mstr_new("");
        g_func_selfname = 1;
        factor_def_func(l, bc, fname);
        g_func_selfname = 0;
        mstr_free(fname);
    } else if (l->tk == LEX_R_IMPORT && !member) {
        /* `import` in expression position: dynamic `import(spec)` or
         * `import.meta`. Declarative import statements never reach factor()
         * (statement() routes them to stmt_import), so anything else here is
         * a syntax error. */
        if (!lex_chkread(l, LEX_R_IMPORT)) {
            return false;
        }
        if (l->tk == '(') {
            /* import(spec) => Promise.resolve(<MODULE_V spec>): the module is
             * loaded synchronously by the loader hook; wrapping keeps the
             * caller's .then()/await contract. Stack shape for CALLO: the
             * receiver (Promise) below the single argument. */
            if (!lex_chkread(l, '(')) {
                return false;
            }
            lex_skip_empty(l);
            bc_gen_str(bc, INSTR_LOAD, "Promise");
            if (!base(l, bc)) {
                return false;
            }
            lex_skip_empty(l);
            if (l->tk == ',') { /* import(spec, options): options ignored */
                if (!lex_chkread(l, ',')) {
                    return false;
                }
                lex_skip_empty(l);
                if (l->tk != ')') {
                    if (!base(l, bc)) {
                        return false;
                    }
                    bc_gen(bc, INSTR_POP);
                    lex_skip_empty(l);
                }
            }
            if (!lex_chkread(l, ')')) {
                return false;
            }
            bc_gen(bc, INSTR_MODULE_V);
            bc_gen_str(bc, INSTR_CALLO, "resolve$1");
        } else if (l->tk == '.') {
            if (!lex_chkread(l, '.')) {
                return false;
            }
            if (l->tk != LEX_ID || strcmp(l->tk_str->cstr, "meta") != 0) {
                if (!g_hoist_quiet) {
                    mario_printf("import: expected 'meta' or '('! ");
                    compile_error_pos(l, -1);
                }
                return false;
            }
            if (!lex_chkread(l, LEX_ID)) {
                return false;
            }
            bc_gen(bc, INSTR_IMPORT_META);
        } else {
            if (!g_hoist_quiet) {
                mario_printf("import: expected 'meta' or '('! ");
                compile_error_pos(l, -1);
            }
            return false;
        }
    } else if (l->tk == LEX_R_CLASS) { //define class
        factor_def_class(l, bc);
    } else if (l->tk == LEX_R_NEW) { //new object
        if (!factor_new(l, bc)) {
            return false;
        }
    } else if ((l->tk == '{' || l->tk == '[') && peek_is_destr_assign(l)) {
        /* `({a, b} = obj)` / `x && ([a, b] = pair)`: destructuring assignment
         * used as an expression (an object/array literal can never be directly
         * followed by `=`). Bind the leaves and leave the RHS value on the stack. */
        if (!destructure_assign_ex(l, bc, 0, true)) {
            return false;
        }
    } else if (l->tk == '{') { // JSON-style object definition
        factor_json(l, bc);
    } else if (l->tk == '[') { // JSON-style array 
        factor_array(l, bc);
    } else if (l->tk == LEX_ID && !member && strcmp(l->tk_str->cstr, "yield") == 0) {
        /* ES6 generator `yield [expr]` / `yield* expr`. `yield` is not lexed as
         * a reserved word, so match it by name (and never as a member `.yield`).
         * The operand is an assignment-level expression; a bare `yield` with no
         * operand yields undefined. */
        if (!lex_chkread(l, LEX_ID)) {
            return false;
        }
        lex_skip_empty(l);
        bool is_star = false;
        if (l->tk == '*') {
            if (!lex_chkread(l, '*')) {
                return false;
            }
            is_star = true;
            lex_skip_empty(l);
        }
        bool bare = (!is_star) && (l->tk == ';' || l->tk == ')' || l->tk == ']' ||
                                   l->tk == '}' || l->tk == ',' || l->tk == '\n' ||
                                   l->tk == ':' || l->tk == 0);
        if (bare) {
            bc_gen(bc, INSTR_UNDEF);
        } else if (!base(l, bc)) {
            return false;
        }
        bc_gen(bc, is_star ? INSTR_YIELD_STAR : INSTR_YIELD);
    } else if (l->tk == LEX_ID) {
        mstr_t* name = mstr_new(l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
            mstr_free(name);
            return false;
        }

        if (l->tk == '(') { // function call
            if (!factor_call_func(l, bc, name, member)) {
                mstr_free(name);
                return false;
            }
        } else if (l->tk == '[') { // array access
            factor_array_access(l, bc, name, member);
        } else {
            if (member) {
                bc_gen_str(bc, INSTR_GET, name->cstr);
            } else if (l->tk == '.') {
                bc_gen_str(bc, INSTR_LOAD, name->cstr);
            } else if (l->tk == LEX_R_AFUNCTION) {
                if (!lex_chkread(l, LEX_R_AFUNCTION)) {
                    return false;
                }
                bc_gen(bc, INSTR_FUNC_ARROW);
                bc_gen_str(bc, INSTR_LOAD, name->cstr);
                factor_def_afunc(l, bc);
            } else {
                /* Bare-identifier rvalue read: LOADV pushes the binding's current
                 * VALUE (a snapshot) so the operand is not aliased by a later
                 * reassignment of the same binding while it sits on the value
                 * stack (e.g. the first `m` in `f(m, m++, m)`). */
                bc_gen_str(bc, INSTR_LOADV, name->cstr);
            }
        }
        mstr_free(name);
    }
	else {
		return false;
	}

    // Postfix chain: member access (.) and subscript ([]) may repeat on the
    // value now on the stack, e.g. a.b[0][1], f()[2], [1,2][0]. Each '.'
    // recurses into factor(member=true), which continues the chain itself.
    while (true) {
        if (l->tk == '.') { // followed by member fetch
            if (!lex_chkread(l, '.')) {
                return false;
            }
            if (!factor(l, bc, true)) {
                return false;
            }
        } else if (l->tk == LEX_OPTCHAIN) { // ES2020 optional chaining `?.member`
            /* The base value is already on the stack. OPT_GET short-circuits a
             * nullish base to undefined, and because each `?.` re-checks the
             * value the previous link produced, a chain like `a?.b?.c` collapses
             * to undefined as soon as any link is nullish. */
            if (!lex_chkread(l, LEX_OPTCHAIN)) {
                return false;
            }
            if (l->tk == LEX_ID || (l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END)) {
                /* A reserved word is a legal member name here too (`x?.import`,
                 * `x?.default`); its source text is still in tk_str. */
                mstr_t* name = mstr_new(l->tk_str->cstr);
                int tk = l->tk;
                if (!lex_chkread(l, tk)) {
                    mstr_free(name);
                    return false;
                }
                if (l->tk == '(') {
                    /* Optional method call `base?.m(args)`. The base stays on
                     * the stack as the receiver so `this` binds like `base.m()`
                     * (CALLO); a nullish base short-circuits to undefined
                     * without evaluating the arguments. Same guard layout as
                     * the `?.(` / `?.[` forms below. */
                    PC pc_guard = bc_reserve(bc);
                    bc_gen(bc, INSTR_UNDEF);
                    PC pc_skip = bc_reserve(bc);
                    PC pc_body = bc->cindex;
                    if (!factor_call_func(l, bc, name, true)) {
                        mstr_free(name);
                        return false;
                    }
                    bc_set_instr(bc, pc_skip, INSTR_JMP, ILLEGAL_PC);
                    bc_set_instr(bc, pc_guard, INSTR_NULLISH, pc_body);
                } else {
                    bc_gen_str(bc, INSTR_OPT_GET, name->cstr);
                }
                mstr_free(name);
            } else if (l->tk == '(' || l->tk == '[') {
                /* ES2020 optional call `base?.(args)` and optional index
                 * `base?.[key]`. The base is on the stack. A NULLISH guard keeps
                 * a non-nullish base and jumps into the call/index; a nullish
                 * base is popped and the whole link yields undefined WITHOUT
                 * evaluating the argument list or the key (spec short-circuit).
                 * Layout: [base] NULLISH->body | UNDEF | JMP->end | body... | end.
                 * Both paths leave exactly one value for the postfix loop. */
                bool is_call = (l->tk == '(');
                PC pc_guard = bc_reserve(bc); // NULLISH, patched to body
                bc_gen(bc, INSTR_UNDEF);      // nullish-base result
                PC pc_skip = bc_reserve(bc);  // JMP over the body, patched at end
                PC pc_body = bc->cindex;      // non-nullish entry: base on stack
                if (is_call) {
                    /* Capture a receiver kept by a just-compiled `v[key]` BEFORE
                     * arg compilation, as the plain `(` branch does. */
                    int recv = g_arrat_recv;
                    g_arrat_recv = 0;
                    bool has_spread = false;
                    int arg_num = call_func(l, bc, &has_spread);
                    if (arg_num < 0) {
                        return false;
                    }
                    if (has_spread) {
                        bc_gen_str(bc, recv ? INSTR_CALLXO_SPREAD : INSTR_CALLX_SPREAD, "");
                    } else {
                        mstr_t* s = mstr_new("");
                        gen_func_name("", arg_num, s);
                        bc_gen_str(bc, recv ? INSTR_CALLXO : INSTR_CALLX, s->cstr);
                        mstr_free(s);
                    }
                } else {
                    if (!lex_chkread(l, '[')) {
                        return false;
                    }
                    if (!base(l, bc)) {
                        return false;
                    }
                    if (!lex_chkread(l, ']')) {
                        return false;
                    }
                    bc_gen(bc, INSTR_ARRAY_AT);
                }
                bc_set_instr(bc, pc_skip, INSTR_JMP, ILLEGAL_PC);    // join point
                bc_set_instr(bc, pc_guard, INSTR_NULLISH, pc_body); // non-nullish
            } else {
                return false;
            }
        } else if (l->tk == '[') { // subscript on the value on the stack
            if (!lex_chkread(l, '[')) {
                return false;
            }
            if (!base(l, bc)) {
                return false;
            }
            if (!lex_chkread(l, ']')) {
                return false;
            }
            /* `v[key](...)`: keep the receiver so the following call binds `this`. */
            if (l->tk == '(') {
                bc_gen(bc, INSTR_ARRAY_AT_M);
                g_arrat_recv = 1;
            } else {
                bc_gen(bc, INSTR_ARRAY_AT);
            }
        } else if (l->tk == '(') {
            /* ES6 call on a value already on the stack: an IIFE
             * `(function(){...})()`, `(expr)(args)`, or a curried `f()()`.
             * call_func pushes the args above the callable value; CALLX picks
             * the value back off and invokes it with runtime/known arity. */
            /* Capture a receiver kept by a just-compiled `v[key]` BEFORE arg
             * compilation (nested calls would overwrite the flag). */
            int recv = g_arrat_recv;
            g_arrat_recv = 0;
            bool has_spread = false;
            int arg_num = call_func(l, bc, &has_spread);
            if (arg_num < 0) {
                return false;
            }
            if (has_spread) {
                bc_gen_str(bc, recv ? INSTR_CALLXO_SPREAD : INSTR_CALLX_SPREAD, "");
            } else {
                mstr_t* s = mstr_new("");
                gen_func_name("", arg_num, s);
                bc_gen_str(bc, recv ? INSTR_CALLXO : INSTR_CALLX, s->cstr);
                mstr_free(s);
            }
        } else if (l->tk == '`') {
            /* ES6 tagged template: the value on the stack is the tag callable. */
            if (!factor_tagged_template(l, bc)) {
                return false;
            }
        } else {
            break;
        }
    }

    return true;
}

bool unary(lex_t* l, bytecode_t* bc) {
    /* ES `await x`: compile the operand, then unwrap it with __await(). It
     * binds at unary level, so `await a + b` parses as `(await a) + b`. */
    if (l->tk == LEX_R_AWAIT) {
        if (!lex_chkread(l, LEX_R_AWAIT)) {
            return false;
        }
        if (!unary(l, bc)) {
            return false;
        }
        bc_gen_str(bc, INSTR_CALL, "__await$1");
        return true;
    }
    /* ES `delete ref`: compile the operand, then RETARGET its final reference
     * instruction to the matching delete opcode (member `.x` -> DELETE,
     * computed `[k]` -> DELETE_AT, bare name -> DELETE_VAR). Each retarget keeps
     * the operand instruction's stack arity, so the result is one bool on the
     * stack. Deleting a non-reference (a literal, or a call/paren result) is a
     * no-op that yields true: drop the value and push true. */
    if (l->tk == LEX_R_DELETE) {
        if (!lex_chkread(l, LEX_R_DELETE)) {
            return false;
        }
        if (!factor(l, bc, false)) {
            return false;
        }
        if (bc->cindex > 0) {
            PC last = bc->code_buf[bc->cindex - 1];
            opr_code_t op = OP(last);
            if (op == INSTR_GET) {
                bc->code_buf[bc->cindex - 1] = INS(INSTR_DELETE, OFF(last));
                return true;
            } else if (op == INSTR_ARRAY_AT) {
                bc->code_buf[bc->cindex - 1] = INS(INSTR_DELETE_AT, OFF(last));
                return true;
            } else if (op == INSTR_LOAD || op == INSTR_LOADV) {
                bc->code_buf[bc->cindex - 1] = INS(INSTR_DELETE_VAR, OFF(last));
                return true;
            }
        }
        bc_gen(bc, INSTR_POP);
        bc_gen(bc, INSTR_TRUE);
        return true;
    }
    /* Prefix ++/-- reached as a nested unary operand (e.g. `!--x`, `-++i`).
     * The additive-level expr() strips a leading ++/-- before term()->unary(),
     * so unary() only ever sees them here. Parse the operand, retarget a
     * member/subscript to its write variant (as expr() does), then pre-step. */
    if (l->tk == LEX_PLUSPLUS || l->tk == LEX_MINUSMINUS) {
        bool is_inc = (l->tk == LEX_PLUSPLUS);
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!unary(l, bc)) {
            return false;
        }
        if (bc->cindex > 0) {
            PC last = bc->code_buf[bc->cindex - 1];
            if (OP(last) == INSTR_ARRAY_AT)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_ARRAY_AT_W, OFF(last));
            else if (OP(last) == INSTR_GET)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_GETW, OFF(last));
            else if (OP(last) == INSTR_LOADV || OP(last) == INSTR_LOAD)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_LOADW, OFF(last));
        }
        bc_gen(bc, is_inc ? INSTR_PPLUS_PRE : INSTR_MMINUS_PRE);
        return true;
    }

    opr_code_t instr = INSTR_END;
    bool is_void = false;
    if (l->tk == '!') {
        if (!lex_chkread(l, '!')) {
            return false;
        }
        instr = INSTR_NOT;
    } else if (l->tk == LEX_R_TYPEOF) {
        if (!lex_chkread(l, LEX_R_TYPEOF)) {
            return false;
        }
        instr = INSTR_TYPEOF;
    } else if (l->tk == LEX_R_VOID) {
        /* `void expr` evaluates the operand, discards it, yields undefined. */
        if (!lex_chkread(l, LEX_R_VOID)) {
            return false;
        }
        is_void = true;
    } else if (l->tk == '-') { // unary minus, incl. as a right operand (a * -b)
        if (!lex_chkread(l, '-')) {
            return false;
        }
        instr = INSTR_NEG;
    } else if (l->tk == '+') { // unary plus: ToNumber(+x)
        if (!lex_chkread(l, '+')) {
            return false;
        }
        instr = INSTR_POS;
    } else if (l->tk == '~') { // unary bitwise NOT: ~x
        if (!lex_chkread(l, '~')) {
            return false;
        }
        instr = INSTR_BNOT;
    }

    if (instr != INSTR_END || is_void) {
        /* Prefix operators nest (`typeof void 0`, `!!x`): recurse at the
         * unary level for the operand instead of dropping to factor. */
        if (!unary(l, bc)) {
            return false;
        }
        /* `typeof x` on an UNDECLARED x yields "undefined", never throws,
         * even in strict code: retarget a bare-identifier operand's LOAD to
         * the non-throwing LOAD_SAFE. A member chain ends in GET, so
         * `typeof o.m` keeps the throwing LOAD for an undeclared `o`,
         * exactly like JS. */
        if (instr == INSTR_TYPEOF && bc->cindex > 0) {
            PC last = bc->code_buf[bc->cindex - 1];
            if (OP(last) == INSTR_LOAD || OP(last) == INSTR_LOADV)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_LOAD_SAFE, OFF(last));
        }
    } else {
        if (!factor(l, bc, false)) {
            return false;
        }
        /* Postfix `x++` / `x--`: binds tighter than the multiplicative level
         * (so `x++ % 3` parses as `(x++) % 3`) and looser than the member/call
         * chain inside factor. Same subscript/member write-retargeting as the
         * prefix form, so a proxy member yields its @@proxyslot sentinel. */
        if (l->tk == LEX_PLUSPLUS || l->tk == LEX_MINUSMINUS) {
            bool is_inc = (l->tk == LEX_PLUSPLUS);
            if (!lex_chkread(l, l->tk)) {
                return false;
            }
            if (bc->cindex > 0) {
                PC last = bc->code_buf[bc->cindex - 1];
                if (OP(last) == INSTR_ARRAY_AT)
                    bc->code_buf[bc->cindex - 1] = INS(INSTR_ARRAY_AT_W, OFF(last));
                else if (OP(last) == INSTR_GET)
                    bc->code_buf[bc->cindex - 1] = INS(INSTR_GETW, OFF(last));
                else if (OP(last) == INSTR_LOADV || OP(last) == INSTR_LOAD)
                    bc->code_buf[bc->cindex - 1] = INS(INSTR_LOADW, OFF(last));
            }
            bc_gen(bc, is_inc ? INSTR_PPLUS : INSTR_MMINUS);
        }
    }

    if (is_void) {
        bc_gen(bc, INSTR_POP);
        bc_gen(bc, INSTR_UNDEF);
    } else if (instr != INSTR_END) {
        bc_gen(bc, instr);
    }
    return true;
}

bool power(lex_t* l, bytecode_t* bc) {
    if (!unary(l, bc)) {
        return false;
    }
    // ES6 exponent '**' is right-associative and binds tighter than * / %.
    if (l->tk == LEX_POWER) {
        if (!lex_chkread(l, LEX_POWER)) {
            return false;
        }
        if (!power(l, bc)) {
            return false;
        }
        bc_gen(bc, INSTR_POW);
    }
    return true;
}

bool term(lex_t* l, bytecode_t* bc) {
    if (!power(l, bc)) {
        return false;
    }

    while (l->tk == '*' || l->tk == '/' || l->tk == '%') {
        LEX_TYPES op = (LEX_TYPES)l->tk;
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!power(l, bc)) {
            return false;
        }

        if (op == '*') {
            bc_gen(bc, INSTR_MULTI);
        } else if (op == '/') {
            bc_gen(bc, INSTR_DIV);
        } else {
            bc_gen(bc, INSTR_MOD);
        }
    }

    return true;
}

bool expr(lex_t* l, bytecode_t* bc) {
    LEX_TYPES pre = (LEX_TYPES)l->tk;

    if (l->tk == '-') {
        if (!lex_chkread(l, '-')) {
            return false;
        }
    } else if (l->tk == LEX_PLUSPLUS) {
        if (!lex_chkread(l, LEX_PLUSPLUS)) {
            return false;
        }
    } else if (l->tk == LEX_MINUSMINUS) {
        if (!lex_chkread(l, LEX_MINUSMINUS)) {
            return false;
        }
    }

    if (!term(l, bc)) {
        return false;
    }

    if (pre == '-') {
        bc_gen(bc, INSTR_NEG);
    } else if (pre == LEX_PLUSPLUS) {
        /* A prefix `++a[i]` / `--a[i]` steps through the binding node: retarget a
         * subscript operand to the write-variant so a TypedArray element yields a
         * synthetic @@taslot target, and a member operand (`.x`) to GETW so a proxy
         * yields its @@proxyslot sentinel and an accessor its [obj,node] pair
         * (normal arrays/objects are unaffected). */
        if (bc->cindex > 0) {
            PC last = bc->code_buf[bc->cindex - 1];
            if (OP(last) == INSTR_ARRAY_AT)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_ARRAY_AT_W, OFF(last));
            else if (OP(last) == INSTR_GET)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_GETW, OFF(last));
            else if (OP(last) == INSTR_LOADV || OP(last) == INSTR_LOAD)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_LOADW, OFF(last));
        }
        bc_gen(bc, INSTR_PPLUS_PRE);
    } else if (pre == LEX_MINUSMINUS) {
        if (bc->cindex > 0) {
            PC last = bc->code_buf[bc->cindex - 1];
            if (OP(last) == INSTR_ARRAY_AT)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_ARRAY_AT_W, OFF(last));
            else if (OP(last) == INSTR_GET)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_GETW, OFF(last));
            else if (OP(last) == INSTR_LOADV || OP(last) == INSTR_LOAD)
                bc->code_buf[bc->cindex - 1] = INS(INSTR_LOADW, OFF(last));
        }
        bc_gen(bc, INSTR_MMINUS_PRE);
    }

    while (l->tk == '+' || l->tk == '-') {
        /* Postfix ++/-- is handled at the unary level (right after factor),
         * so it binds tighter than the multiplicative operators. */
        int op = l->tk;
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!term(l, bc)) {
            return false;
        }
        if (op == '+') {
            bc_gen(bc, INSTR_PLUS);
        } else {
            bc_gen(bc, INSTR_MINUS);
        }
    }

    return true;
}

bool shift(lex_t* l, bytecode_t* bc) {
    if (!expr(l, bc)) {
        return false;
    }

    /* Shift operators are LEFT-associative and bind tighter than the bitwise
     * `& ^ |` and relational levels, so both operands are parsed at the additive
     * `expr()` level (NOT `base()`). Parsing the right operand with base() let a
     * following lower-precedence operator be swallowed into the shift's RHS:
     * `y<<4|digit` compiled as `y<<(4|digit)` (always 0 for the React Flight
     * row-ID accumulator) and `1<<2|3` gave 8 instead of 7. The `while` (not a
     * single `if`) keeps `a<<b<<c` left-associative. */
    while (l->tk == LEX_LSHIFT || l->tk == LEX_RSHIFT || l->tk == LEX_RSHIFTUNSIGNED) {
        int op = l->tk;
        if (!lex_chkread(l, op)) {
            return false;
        }
        if (!expr(l, bc)) {
            return false;
        }

        if (op == LEX_LSHIFT) {
            bc_gen(bc, INSTR_LSHIFT);
        } else if (op == LEX_RSHIFT) {
            bc_gen(bc, INSTR_RSHIFT);
        } else {
            bc_gen(bc, INSTR_URSHIFT);
        }
    }
    return true;
}

bool condition(lex_t* l, bytecode_t* bc) {
    if (!shift(l, bc)) {
        return false;
    }

    while (l->tk == LEX_EQUAL || l->tk == LEX_NEQUAL ||
           l->tk == LEX_TYPEEQUAL || l->tk == LEX_NTYPEQUAL ||
           l->tk == LEX_LEQUAL || l->tk == LEX_GEQUAL ||
           l->tk == LEX_R_INSTANCEOF || l->tk == LEX_R_IN ||
           l->tk == '<' || l->tk == '>') {
        int op = l->tk;
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!shift(l, bc)) {
            return false;
        }

        if (op == LEX_EQUAL) {
            bc_gen(bc, INSTR_EQ);
        } else if (op == LEX_NEQUAL) {
            bc_gen(bc, INSTR_NEQ);
        } else if (op == LEX_TYPEEQUAL) {
            bc_gen(bc, INSTR_TEQ);
        } else if (op == LEX_NTYPEQUAL) {
            bc_gen(bc, INSTR_NTEQ);
        } else if (op == LEX_LEQUAL) {
            bc_gen(bc, INSTR_LEQ);
        } else if (op == LEX_GEQUAL) {
            bc_gen(bc, INSTR_GEQ);
        } else if (op == LEX_R_INSTANCEOF) {
            bc_gen(bc, INSTR_INSTOF);
        } else if (op == LEX_R_IN) {
            bc_gen(bc, INSTR_IN);
        } else if (op == '>') {
            bc_gen(bc, INSTR_GRT);
        } else if (op == '<') {
            bc_gen(bc, INSTR_LES);
        }
    }

    return true;
}

/* Bitwise `& ^ |` occupy THREE distinct precedence levels in JS (`&` binds
 * tightest, then `^`, then `|`), each left-associative, sitting above the
 * relational `condition()` level and below the logical `&&`. Lumping them into a
 * single left-to-right loop mis-grouped mixed operands: `a | b & c` compiled as
 * `(a|b)&c` instead of `a|(b&c)`, and `a ^ b & c` as `(a^b)&c`. Split them so
 * each level's operand is the next-tighter level. */
static bool logic_bitand(lex_t* l, bytecode_t* bc) {
    if (!condition(l, bc)) {
        return false;
    }
    while (l->tk == '&') {
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!condition(l, bc)) {
            return false;
        }
        bc_gen(bc, INSTR_AND);
    }
    return true;
}

static bool logic_bitxor(lex_t* l, bytecode_t* bc) {
    if (!logic_bitand(l, bc)) {
        return false;
    }
    while (l->tk == '^') {
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!logic_bitand(l, bc)) {
            return false;
        }
        bc_gen(bc, INSTR_XOR);
    }
    return true;
}

static bool logic_bitor(lex_t* l, bytecode_t* bc) {
    if (!logic_bitxor(l, bc)) {
        return false;
    }
    while (l->tk == '|') {
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!logic_bitxor(l, bc)) {
            return false;
        }
        bc_gen(bc, INSTR_OR);
    }
    return true;
}

/* `&&` level: binds tighter than `||`, so each `||` operand is a whole
 * conjunction and a SCOR slot skips complete `&&` groups. */
static bool logic_and(lex_t* l, bytecode_t* bc) {
    if (!logic_bitor(l, bc)) {
        return false;
    }

    while (l->tk == LEX_ANDAND) {
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        /* Short-circuit `&&`: reserve a jump slot, compile the RHS, then
         * patch the slot. At runtime the LHS is already on the stack; if it
         * is falsy the jump skips the RHS keeping the LHS, otherwise the LHS
         * is popped and execution falls through into the RHS. Chained `&&`
         * patch each slot to the next slot (or the end), so `a&&b&&c`
         * short-circuits fully. */
        PC pc1 = bc_reserve(bc);
        if (!logic_bitor(l, bc)) {
            return false;
        }
        bc_set_instr(bc, pc1, INSTR_SCAND, ILLEGAL_PC);
    }
    return true;
}

bool logic(lex_t* l, bytecode_t* bc) {
    /* `||` level. Keeping `&&` on its own (tighter) level is what makes mixed
     * chains like `a&&b||c&&d` compile correctly: with a single flat loop a
     * truthy SCOR only skipped its own RHS primary and landed INSIDE the next
     * `&&` group, whose SCAND then popped the kept value and evaluated the
     * group's operands unconditionally (core-js' `typeof x&&x||...` global
     * detection then read a bare undeclared `global` and threw). */
    if (!logic_and(l, bc)) {
        return false;
    }

    while (l->tk == LEX_OROR) {
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        /* Same short-circuit scheme as `&&`: `||` truthy keeps the LHS and
         * jumps past the whole RHS conjunction, falsy pops and falls through
         * to evaluate it. Chained `||` hop slot to slot. */
        PC pc1 = bc_reserve(bc);
        if (!logic_and(l, bc)) {
            return false;
        }
        bc_set_instr(bc, pc1, INSTR_SCOR, ILLEGAL_PC);
    }
    return true;
}


bool ternary(lex_t* l, bytecode_t* bc) {
    if (!logic(l, bc)) {
        return false;
    }

    /* ES2020 `a ?? b`: binds looser than ||/&& (handled in logic) and tighter
     * than the conditional below. NULLISH short-circuits: if the LHS on the
     * stack is non-nullish it jumps past the RHS keeping the LHS as the result;
     * otherwise it pops the nullish LHS and falls through to evaluate the RHS. */
    while (l->tk == LEX_NULLISH) {
        if (!lex_chkread(l, LEX_NULLISH)) {
            return false;
        }
        PC pc1 = bc_reserve(bc); //keep for the short-circuit jump
        if (!base(l, bc)) {
            return false;
        }
        bc_set_instr(bc, pc1, INSTR_NULLISH, ILLEGAL_PC);
    }

    if (l->tk == '?') {
        PC pc1 = bc_reserve(bc); //keep for jump
        if (!lex_chkread(l, '?')) {
            return false;
        }
        if (!base(l, bc)) {
            return false;
        }
        PC pc2 = bc_reserve(bc); //keep for jump
        if (!lex_chkread(l, ':')) {
            return false;
        }
        bc_set_instr(bc, pc1, INSTR_NJMP, ILLEGAL_PC);
        if (!base(l, bc)) {
            return false;
        }
        bc_set_instr(bc, pc2, INSTR_JMP, ILLEGAL_PC);
    }
    return true;
}

bool base(lex_t* l, bytecode_t* bc) {
    if (!ternary(l, bc)) {
        return false;
    }

    if (l->tk == '=' || 
			l->tk == LEX_PLUSEQUAL ||
			l->tk == LEX_MULTIEQUAL ||
			l->tk == LEX_DIVEQUAL ||
			l->tk == LEX_MODEQUAL ||
			l->tk == LEX_POWEREQUAL ||
			l->tk == LEX_OREQUALOR ||
			l->tk == LEX_ANDEQUALAND ||
			l->tk == LEX_NULLISHEQUAL ||
			l->tk == LEX_ANDEQUAL ||
			l->tk == LEX_OREQUAL ||
			l->tk == LEX_XOREQUAL ||
			l->tk == LEX_LSHIFTEQUAL ||
			l->tk == LEX_RSHIFTEQUAL ||
			l->tk == LEX_RSHIFTUNSIGNEQUAL ||
	        l->tk == LEX_MINUSEQUAL) {
        LEX_TYPES op = (LEX_TYPES)l->tk;
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        /* For a plain assignment whose target ended with a member fetch (`.`),
         * retarget that fetch to the write-variant so a runtime setter is
         * invoked. This must run before the RHS is compiled, since the RHS
         * appends instructions after the target's final INSTR_GET.
         * The arithmetic compound assigns (`+= -= *= /= %= **=`) retarget the
         * member fetch too: a proxy target then yields its @@proxyslot write
         * sentinel (handle_math resolves it through the get trap and writes back
         * through the set trap) and an accessor yields its [obj,node] pair, while
         * a plain object member is unchanged (GETW == GET for it). The logical
         * assigns (`||= &&= ??=`) keep the read form because handle_logic_assign
         * installs through the binding node and does not model write sentinels.
         * Likewise a subscript target (`a[i] op= ..`) retargets INSTR_ARRAY_AT to
         * the write-variant INSTR_ARRAY_AT_W for EVERY assignment op: normal
         * arrays behave identically (the W handler delegates to the same push),
         * while a TypedArray receiver yields a synthetic @@taslot write target. */
        bool arith_compound = (op == LEX_PLUSEQUAL || op == LEX_MINUSEQUAL ||
                               op == LEX_MULTIEQUAL || op == LEX_DIVEQUAL ||
                               op == LEX_MODEQUAL || op == LEX_POWEREQUAL ||
                               op == LEX_ANDEQUAL || op == LEX_OREQUAL ||
                               op == LEX_XOREQUAL || op == LEX_LSHIFTEQUAL ||
                               op == LEX_RSHIFTEQUAL || op == LEX_RSHIFTUNSIGNEQUAL);
        bool deferred_wtarget = false;
        const char* wtarget_name = NULL;
        if (bc->cindex > 0) {
            PC last = bc->code_buf[bc->cindex - 1];
            if ((op == '=' || arith_compound) && OP(last) == INSTR_GET) {
                /* Deferred member write target: WANCHOR parks the base under a
                 * sentinel while the RHS runs, and WTARGET resolves the target
                 * afterwards (inserting it below the RHS value, where GETW would
                 * have left it). Resolving BEFORE the RHS - the old GETW rewrite -
                 * left the target node inside the RHS's operand window, so a
                 * fused member call in the RHS (`a.b = c.bind(null, a.b.bind(a))`,
                 * the webpack runtime tail) mis-picked it as its receiver. */
                bc->code_buf[bc->cindex - 1] = INS(INSTR_WANCHOR, OFF(last));
                deferred_wtarget = true;
                wtarget_name = bc_getstr(bc, OFF(last));
            } else if (OP(last) == INSTR_ARRAY_AT) {
                bc->code_buf[bc->cindex - 1] = INS(INSTR_ARRAY_AT_W, OFF(last));
            } else if (OP(last) == INSTR_LOADV || OP(last) == INSTR_LOAD) {
                /* Bare-name target (`x = v`, `x += v`, `x ||= v`): retarget the
                 * value-read (LOADV) - or a legacy LOAD - to LOADW, which keeps
                 * the raw binding node for ASIGN / compound-math / logical
                 * write-back and never invokes an accessor getter. */
                bc->code_buf[bc->cindex - 1] = INS(INSTR_LOADW, OFF(last));
            }
        }
        if (!base(l, bc)) {
            return false;
        }
        if (deferred_wtarget) {
            bc_gen_str(bc, INSTR_WTARGET, wtarget_name);
        }
        // sort out initialiser
        if (op == '=') {
            bc_gen(bc, INSTR_ASIGN);
        } else if (op == LEX_PLUSEQUAL) {
            bc_gen(bc, INSTR_PLUSEQ);
        } else if (op == LEX_MINUSEQUAL) {
            bc_gen(bc, INSTR_MINUSEQ);
        } else if (op == LEX_MULTIEQUAL) {
            bc_gen(bc, INSTR_MULTIEQ);
        } else if (op == LEX_DIVEQUAL) {
            bc_gen(bc, INSTR_DIVEQ);
        } else if (op == LEX_MODEQUAL) {
            bc_gen(bc, INSTR_MODEQ);
        } else if (op == LEX_POWEREQUAL) {
            bc_gen(bc, INSTR_POWEQ);
        } else if (op == LEX_OREQUALOR) {
            bc_gen(bc, INSTR_OREQ);
        } else if (op == LEX_ANDEQUALAND) {
            bc_gen(bc, INSTR_ANDEQ);
        } else if (op == LEX_NULLISHEQUAL) {
            bc_gen(bc, INSTR_NULLISHEQ);
        } else if (op == LEX_ANDEQUAL) {
            bc_gen(bc, INSTR_BITANDEQ);
        } else if (op == LEX_OREQUAL) {
            bc_gen(bc, INSTR_BITOREQ);
        } else if (op == LEX_XOREQUAL) {
            bc_gen(bc, INSTR_BITXOREQ);
        } else if (op == LEX_LSHIFTEQUAL) {
            bc_gen(bc, INSTR_LSHIFTEQ);
        } else if (op == LEX_RSHIFTEQUAL) {
            bc_gen(bc, INSTR_RSHIFTEQ);
        } else if (op == LEX_RSHIFTUNSIGNEQUAL) {
            bc_gen(bc, INSTR_URSHIFTEQ);
        }
		else {
			return false;
		}
    }
    return true;
}

static bool is_stmt_end(int tk) {
    /* '}' terminates the statement by ASI but is never consumed here. */
    return (tk == ';' || tk == '\n' || tk == '}' || tk == 0);
    //return (tk == ';');
}

/* The lexer treats '\n' as plain whitespace, so automatic semicolon
 * insertion is recovered from source positions: true when a line break sits
 * in the gap between the previous token and the current one. */
static bool lex_had_newline(lex_t* l) {
    int32_t from = l->tk_last_end;
    int32_t to = l->tk_start;
    if (to <= from) {
        return false;
    }
    if (to > l->data_end) {
        to = l->data_end;
    }
    for (int32_t i = from; i < to; i++) {
        if (l->data[i] == '\n') {
            return true;
        }
    }
    return false;
}

/* Tokens that may continue the current expression on the next line
 * (`var a = 1\n+2` is one expression in JS): a line break before anything
 * else ends the statement by ASI. */
static bool tk_continues_expr(int tk) {
    switch (tk) {
        case '+': case '-': case '*': case '/': case '%':
        case '(': case '[': case '.': case '?': case ':':
        case LEX_PLUSPLUS: case LEX_MINUSMINUS:
        case LEX_EQUAL: case LEX_NEQUAL: case LEX_TYPEEQUAL: case LEX_NTYPEQUAL:
        case LEX_LEQUAL: case LEX_GEQUAL: case LEX_ANDAND: case LEX_OROR:
        case '<': case '>': case '&': case '|': case '^':
        case LEX_R_INSTANCEOF: case LEX_R_IN:
            return true;
        default:
            return false;
    }
}

/** Comma (sequence) operator: `a, b, c` evaluates left to right and yields
 *  the last value. It lives ABOVE the assignment level, so it is only used
 *  where the grammar allows a full Expression (expression statements and the
 *  for-header clauses) - never for call arguments or literal elements, where
 *  ',' is a separator. */
static bool expr_seq(lex_t* l, bytecode_t* bc) {
    if (!base(l, bc)) {
        return false;
    }
    while (l->tk == ',') {
        bc_gen(bc, INSTR_POP); // discard the previous value, keep the last
        if (!lex_chkread(l, ',')) {
            return false;
        }
        lex_skip_empty(l);
        if (!base(l, bc)) {
            return false;
        }
    }
    return true;
}

/** ES6 destructuring in a declaration: `let [a, b] = expr` or
 *  `let {x, y: alias, z = def} = expr`. Supports array/object patterns,
 *  holes, renaming, per-element defaults and a trailing array rest element.
 *  The RHS is evaluated once into a hidden temp, then each target is bound. */
#define DESTR_MAX 32
typedef struct {
    int  index;             // array index; -1 for object patterns
    bool is_rest;           // array rest element (...name)
    bool is_obj_rest;       // object rest element (...name) in `{a, ...rest}`
    char key[64];           // object key name (empty for array patterns)
    char target[64];        // target variable name
    char def_expr[256];     // default-value source (empty if none)
} destr_entry_t;

/* Skip a balanced destructuring pattern (`{...}` or `[...]`) at the token
 * level. On entry l->tk is the opening brace/bracket; on return the opening
 * pattern has been consumed and l->tk is the token just after the matching
 * closer (usually '=' or ',' or ')'). */
static bool skip_balanced_pattern(lex_t* l) {
    int depth = 0;
    while (true) {
        if (l->tk == '{' || l->tk == '[') {
            depth++;
        } else if (l->tk == '}' || l->tk == ']') {
            depth--;
            if (depth <= 0) {
                lex_get_next_token(l);
                return true;
            }
        } else if (l->tk == LEX_EOF) {
            return false;
        }
        lex_get_next_token(l);
    }
}

/* Assignment-form destructuring may target any member expression:
 * `[this._a, o.b.value, arr[i]] = f()`. The leading identifier token has
 * already been read into l->tk_str; capture the rest of the target text up to
 * the element terminator (`,` / closing bracket / a single `=` starting the
 * default) at nesting depth 0. Returns false if the target is a plain
 * identifier (nothing captured). Strings and computed keys are carried over
 * verbatim so `a["x"]` / `a[i+1]` survive. */
static bool scan_member_target(lex_t* l, mstr_t* out) {
    if (l->curr_ch != '.' && l->curr_ch != '[' && !(l->curr_ch == '?' && l->next_ch == '.')) {
        return false;
    }
    mstr_cpy(out, l->tk_str->cstr);
    int depth = 0;
    while (l->curr_ch) {
        char c = l->curr_ch;
        if (c == '"' || c == '\'' || c == '`') {
            char q = c;
            mstr_add(out, c);
            lex_get_nextch(l);
            while (l->curr_ch && l->curr_ch != q) {
                if (l->curr_ch == '\\') {
                    mstr_add(out, l->curr_ch);
                    lex_get_nextch(l);
                }
                if (l->curr_ch) {
                    mstr_add(out, l->curr_ch);
                    lex_get_nextch(l);
                }
            }
            if (l->curr_ch == q) {
                mstr_add(out, l->curr_ch);
                lex_get_nextch(l);
            }
            continue;
        }
        if (c == '(' || c == '[' || c == '{') {
            depth++;
        } else if (c == ')' || c == ']' || c == '}') {
            if (depth == 0) {
                break;
            }
            depth--;
        } else if (depth == 0 && (c == ',' || (c == '=' && l->next_ch != '='))) {
            break;
        }
        mstr_add(out, c);
        lex_get_nextch(l);
    }
    lex_get_next_token(l); // l->tk becomes the terminator token
    return true;
}

/* Emit `<target_ex> = <rhs_src>` for a member-expression leaf target by
 * compiling the assignment text through base(), so the member write goes via
 * the regular WANCHOR/WTARGET (setter-aware) path instead of a raw GET+ASIGN,
 * which does not resolve a fetched member as a write target. */
static bool destr_assign_member(bytecode_t* bc, mstr_t* target_ex, const char* rhs_src) {
    mstr_t* asg = mstr_new(target_ex->cstr);
    mstr_append(asg, "=(");
    mstr_append(asg, rhs_src);
    mstr_append(asg, ")");
    bool ok = compile_captured_expr(asg->cstr, bc);
    mstr_free(asg);
    if (ok) {
        bc_gen(bc, INSTR_POP);
    }
    return ok;
}

/* Build the source-text accessor `<src>[idx]` / `<src>[<comp_var>]` /
 * `<src>["key"]` used as the RHS of a member-target binding. */
static void destr_rhs_text(mstr_t* out, const char* src, bool is_array, int idx,
                           const char* comp_var, const char* keyname) {
    mstr_cpy(out, src);
    mstr_add(out, '[');
    if (is_array) {
        mstr_append(out, mstr_from_int(idx, 10));
    } else if (comp_var[0]) {
        mstr_append(out, comp_var);
    } else {
        mstr_add(out, '"');
        const char* p;
        for (p = keyname; *p; p++) {
            if (*p == '"' || *p == '\\') {
                mstr_add(out, '\\');
            }
            mstr_add(out, *p);
        }
        mstr_add(out, '"');
    }
    mstr_add(out, ']');
}

/* Recursively parse a destructuring pattern and emit bindings that read from
 * the variable named `src` (already declared and holding the source value).
 * `decl_op` is the declaration instruction for leaf targets (INSTR_VAR /
 * INSTR_SAFE_VAR / INSTR_CONST), or 0 when targets are already declared. */
static bool destructure_pattern(lex_t* l, bytecode_t* bc, opr_code_t decl_op, const char* src) {
    bool is_array = (l->tk == '[');
    char close_ch = is_array ? ']' : '}';
    if (!lex_chkread(l, is_array ? '[' : '{')) {
        return false;
    }
    int idx = 0;
    char keys[DESTR_MAX][64];
    char key_vars[DESTR_MAX][32]; /* hidden var holding a computed key value */
    int nkeys = 0;

    lex_skip_empty(l);
    while (l->tk != close_ch) {
        lex_skip_empty(l);
        if (l->tk == close_ch) {
            break;
        }

        /* rest element: ...name (must be last) */
        if (l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
            lex_get_nextch(l);
            lex_get_nextch(l);
            lex_get_next_token(l);
            if (l->tk != LEX_ID) {
                return false;
            }
            char target[64];
            strncpy(target, l->tk_str->cstr, sizeof(target) - 1);
            target[sizeof(target) - 1] = 0;
            if (!lex_chkread(l, LEX_ID)) {
                return false;
            }
            if (decl_op) {
                bc_gen_str(bc, decl_op, target);
                if (g_vardecl_bc != NULL && decl_op == INSTR_VAR) {
                    bc_gen_str(g_vardecl_bc, INSTR_VAR, target); // ES5 var hoisting
                }
            }
            bc_gen_str(bc, INSTR_LOAD, target);
            bc_gen_str(bc, INSTR_LOAD, src);
            if (is_array) {
                bc_gen_int(bc, INSTR_INT, idx);
                bc_gen_str(bc, INSTR_CALLO, "slice$1");
            } else {
                bc_gen(bc, INSTR_ARRAY);
                int k;
                for (k = 0; k < nkeys; k++) {
                    if (key_vars[k][0]) {
                        bc_gen_str(bc, INSTR_LOAD, key_vars[k]);
                    } else {
                        bc_gen_str(bc, INSTR_STR, keys[k]);
                    }
                    bc_gen(bc, INSTR_MEMBER);
                }
                bc_gen(bc, INSTR_ARRAY_END);
                bc_gen_str(bc, INSTR_CALL, "__obj_rest$2");
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
            break;
        }

        char keyname[64];
        keyname[0] = 0;
        char comp_var[32]; /* hidden var holding a computed key value, if any */
        comp_var[0] = 0;
        int my_idx = idx;

        if (is_array) {
            if (l->tk == ',') { /* hole */
                idx++;
                if (!lex_chkread(l, ',')) {
                    return false;
                }
                continue;
            }
            idx++;
        } else {
            if (l->tk == LEX_ID) {
                strncpy(keyname, l->tk_str->cstr, sizeof(keyname) - 1);
                keyname[sizeof(keyname) - 1] = 0;
                if (!lex_chkread(l, LEX_ID)) {
                    return false;
                }
            } else if (l->tk == LEX_STR) {
                strncpy(keyname, l->tk_str->cstr, sizeof(keyname) - 1);
                keyname[sizeof(keyname) - 1] = 0;
                if (!lex_chkread(l, LEX_STR)) {
                    return false;
                }
            } else if (l->tk >= LEX_R_IF && l->tk < LEX_R_LIST_END) {
                /* A reserved word used as a destructuring key: `{ default: value }`,
                 * which bundlers emit to extract a default export. JS permits
                 * keywords as property names; a reserved-word token keeps its source
                 * text in tk_str (lex_get_reserved_word only retags tk), so capture
                 * the name and consume the token exactly like an identifier. */
                strncpy(keyname, l->tk_str->cstr, sizeof(keyname) - 1);
                keyname[sizeof(keyname) - 1] = 0;
                int tk = l->tk;
                if (!lex_chkread(l, tk)) {
                    return false;
                }
            } else if (l->tk == '[') {
                /* ES6 computed key: `{[expr]: target} = src`. Evaluate expr now
                 * into a hidden temp (it cannot reference src); the binding and
                 * any object-rest exclusion then read the key at runtime. */
                if (!lex_chkread(l, '[')) {
                    return false;
                }
                snprintf(comp_var, sizeof(comp_var), "__dk%d", g_destr_counter++);
                bc_gen_str(bc, INSTR_VAR, comp_var);
                bc_gen_str(bc, INSTR_LOAD, comp_var);
                lex_skip_empty(l);
                if (!base(l, bc)) {
                    return false;
                }
                lex_skip_empty(l);
                if (!lex_chkread(l, ']')) {
                    return false;
                }
                bc_gen(bc, INSTR_ASIGN);
                bc_gen(bc, INSTR_POP);
                lex_skip_empty(l);
                if (l->tk != ':') { /* computed keys are never shorthand */
                    return false;
                }
            } else {
                return false;
            }
            if (nkeys < DESTR_MAX) {
                strncpy(keys[nkeys], keyname, sizeof(keys[nkeys]) - 1);
                keys[nkeys][sizeof(keys[nkeys]) - 1] = 0;
                strncpy(key_vars[nkeys], comp_var, sizeof(key_vars[nkeys]) - 1);
                key_vars[nkeys][sizeof(key_vars[nkeys]) - 1] = 0;
                nkeys++;
            }
        }

        lex_skip_empty(l);
        bool nested = false;
        char target[64];
        target[0] = 0;
        mstr_t* target_ex = NULL; /* member-expression target (assignment form) */

        if (is_array) {
            /* the element itself is the target */
            if (l->tk == '{' || l->tk == '[') {
                nested = true;
            } else if (l->tk == LEX_ID) {
                strncpy(target, l->tk_str->cstr, sizeof(target) - 1);
                target[sizeof(target) - 1] = 0;
                if (decl_op == 0) {
                    target_ex = mstr_new("");
                    if (!scan_member_target(l, target_ex)) {
                        mstr_free(target_ex);
                        target_ex = NULL;
                    }
                }
                if (target_ex == NULL && !lex_chkread(l, LEX_ID)) {
                    return false;
                }
            } else {
                return false;
            }
        } else if (l->tk == ':') {
            /* key: <target> */
            if (!lex_chkread(l, ':')) {
                return false;
            }
            lex_skip_empty(l);
            if (l->tk == '{' || l->tk == '[') {
                nested = true;
            } else if (l->tk == LEX_ID) {
                strncpy(target, l->tk_str->cstr, sizeof(target) - 1);
                target[sizeof(target) - 1] = 0;
                if (decl_op == 0) {
                    target_ex = mstr_new("");
                    if (!scan_member_target(l, target_ex)) {
                        mstr_free(target_ex);
                        target_ex = NULL;
                    }
                }
                if (target_ex == NULL && !lex_chkread(l, LEX_ID)) {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            /* shorthand: the key name is itself the leaf target */
            strncpy(target, keyname, sizeof(target) - 1);
            target[sizeof(target) - 1] = 0;
        }

        if (nested) {
            /* nested pattern -> bind into a hidden temp, then recurse */
            char tmpn[32];
            snprintf(tmpn, sizeof(tmpn), "__dn%d", g_destr_counter++);
            bc_gen_str(bc, INSTR_VAR, tmpn);
            bc_gen_str(bc, INSTR_LOAD, tmpn);
            bc_gen_str(bc, INSTR_LOAD, src);
            if (is_array) {
                bc_gen_int(bc, INSTR_INT, my_idx);
                bc_gen(bc, INSTR_ARRAY_AT);
            } else if (comp_var[0]) {
                bc_gen_str(bc, INSTR_LOAD, comp_var);
                bc_gen(bc, INSTR_ARRAY_AT);
            } else {
                bc_gen_str(bc, INSTR_GET, keyname);
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
            /* Nested pattern default: `{a: {b} = {}} = src`. The `=` token only
             * appears AFTER the nested pattern text, but the guard must run
             * BEFORE the recursive bindings. Peek lexically: skip the balanced
             * pattern, capture a default expression if present, restore, then
             * emit `if (tmpn === undefined) tmpn = <default>` and recurse. */
            mstr_t* defex = NULL;
            bool had_def = false;
            {
                lex_t sv = *l;
                mstr_t* sv_tk = l->tk_str;
                l->tk_str = mstr_new("");
                mstr_cpy(l->tk_str, sv_tk->cstr);
                if (skip_balanced_pattern(l)) {
                    lex_skip_empty(l);
                    if (l->tk == '=') {
                        defex = mstr_new("");
                        if (scan_param_expr(l, defex) == 0) {
                            mstr_free(defex);
                            defex = NULL;
                        } else {
                            had_def = true;
                        }
                    }
                }
                mstr_free(l->tk_str);
                *l = sv;
                l->tk_str = sv_tk;
            }
            if (defex != NULL) {
                bc_gen_str(bc, INSTR_LOAD, tmpn);
                bc_gen(bc, INSTR_TYPEOF);
                bc_gen_str(bc, INSTR_STR, "undefined");
                bc_gen(bc, INSTR_TEQ);
                PC pj = bc_reserve(bc);
                bc_gen_str(bc, INSTR_LOAD, tmpn);
                if (!compile_captured_expr(defex->cstr, bc)) {
                    mstr_free(defex);
                    return false;
                }
                mstr_free(defex);
                bc_gen(bc, INSTR_ASIGN);
                bc_gen(bc, INSTR_POP);
                bc_set_instr(bc, pj, INSTR_NJMP, ILLEGAL_PC);
            }
            if (!destructure_pattern(l, bc, decl_op, tmpn)) {
                return false;
            }
            if (had_def) {
                /* the recursive parse stopped at the `=` of the default; skip
                 * its tokens (already compiled into the guard above) exactly
                 * like the leaf-default path does. */
                mstr_t* skipex = mstr_new("");
                if (scan_param_expr(l, skipex) == 0) {
                    mstr_free(skipex);
                    return false;
                }
                mstr_free(skipex);
                lex_get_next_token(l);
            }
        } else if (target_ex != NULL) {
            /* member-expression target (assignment form only) */
            mstr_t* rhs = mstr_new("");
            destr_rhs_text(rhs, src, is_array, my_idx, comp_var, keyname);
            bool ok = destr_assign_member(bc, target_ex, rhs->cstr);
            mstr_free(rhs);
            if (!ok) {
                mstr_free(target_ex);
                return false;
            }

            /* per-element default: <target> = <expr> */
            lex_skip_empty(l);
            if (l->tk == '=') {
                mstr_t* ex = mstr_new("");
                if (scan_param_expr(l, ex) == 0) {
                    mstr_free(ex);
                    mstr_free(target_ex);
                    return false;
                }
                lex_get_next_token(l);
                ok = compile_captured_expr(target_ex->cstr, bc);
                bc_gen(bc, INSTR_TYPEOF);
                bc_gen_str(bc, INSTR_STR, "undefined");
                bc_gen(bc, INSTR_TEQ);
                PC pj = bc_reserve(bc);
                ok = ok && destr_assign_member(bc, target_ex, ex->cstr);
                mstr_free(ex);
                if (!ok) {
                    mstr_free(target_ex);
                    return false;
                }
                bc_set_instr(bc, pj, INSTR_NJMP, ILLEGAL_PC);
            }
            mstr_free(target_ex);
            target_ex = NULL;
        } else {
            if (decl_op) {
                bc_gen_str(bc, decl_op, target);
                if (g_vardecl_bc != NULL && decl_op == INSTR_VAR) {
                    bc_gen_str(g_vardecl_bc, INSTR_VAR, target); // ES5 var hoisting
                }
            }
            bc_gen_str(bc, INSTR_LOAD, target);
            bc_gen_str(bc, INSTR_LOAD, src);
            if (is_array) {
                bc_gen_int(bc, INSTR_INT, my_idx);
                bc_gen(bc, INSTR_ARRAY_AT);
            } else if (comp_var[0]) {
                bc_gen_str(bc, INSTR_LOAD, comp_var);
                bc_gen(bc, INSTR_ARRAY_AT);
            } else {
                bc_gen_str(bc, INSTR_GET, keyname);
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);

            /* per-element default: name = <expr> */
            lex_skip_empty(l);
            if (l->tk == '=') {
                mstr_t* ex = mstr_new("");
                if (scan_param_expr(l, ex) == 0) {
                    mstr_free(ex);
                    return false;
                }
                lex_get_next_token(l);
                bc_gen_str(bc, INSTR_LOAD, target);
                bc_gen(bc, INSTR_TYPEOF);
                bc_gen_str(bc, INSTR_STR, "undefined");
                bc_gen(bc, INSTR_TEQ);
                PC pj = bc_reserve(bc);
                bc_gen_str(bc, INSTR_LOAD, target);
                if (!compile_captured_expr(ex->cstr, bc)) {
                    mstr_free(ex);
                    return false;
                }
                mstr_free(ex);
                bc_gen(bc, INSTR_ASIGN);
                bc_gen(bc, INSTR_POP);
                bc_set_instr(bc, pj, INSTR_NJMP, ILLEGAL_PC);
            }
        }

        lex_skip_empty(l);
        if (l->tk != close_ch) {
            if (!lex_chkread(l, ',')) {
                return false;
            }
        }
        lex_skip_empty(l);
    }
    if (!lex_chkread(l, close_ch)) {
        return false;
    }
    return true;
}

static bool destructure_assign_ex(lex_t* l, bytecode_t* bc, opr_code_t op, bool leave_value) {
    lex_t saved = *l;                 // pattern-start state (tk_str shared)
    mstr_t* saved_tk = l->tk_str;     // preserve caller token text

    /* Skip the pattern to reach `= RHS`, then evaluate RHS into a hidden temp. */
    if (!skip_balanced_pattern(l)) {
        return false;
    }
    lex_skip_empty(l);
    if (l->tk != '=') {
        return false;
    }
    if (!lex_chkread(l, '=')) {
        return false;
    }

    mstr_t* tmpm = mstr_new("__ds");
    mstr_append(tmpm, mstr_from_int(g_destr_counter++, 10));
    const char* tmp = tmpm->cstr;

    bc_gen_str(bc, INSTR_VAR, tmp);
    bc_gen_str(bc, INSTR_LOAD, tmp);
    if (!base(l, bc)) {
        mstr_free(tmpm);
        return false;
    }
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);

    /* Emit bindings by re-parsing the pattern on a private lexer copy. */
    lex_t pl = saved;
    pl.tk_str = mstr_new("");
    mstr_cpy(pl.tk_str, saved_tk->cstr);
    bool ok = destructure_pattern(&pl, bc, op, tmp);
    mstr_free(pl.tk_str);

    if (ok && leave_value) {
        /* expression form: the assignment's value is the RHS itself */
        bc_gen_str(bc, INSTR_LOAD, tmp);
    }
    mstr_free(tmpm);
    return ok;
}

static bool stmt_var_destructure(lex_t* l, bytecode_t* bc, opr_code_t op) {
    return destructure_assign_ex(l, bc, op, false);
}

/* Peek: does the `{`/`[` at the current token open a destructuring pattern that
 * is followed by a single `=` (i.e. a destructuring ASSIGNMENT rather than an
 * object/array literal)? The lexer is restored either way. */
static bool peek_is_destr_assign(lex_t* l) {
    lex_t saved = *l;
    mstr_t* saved_str = mstr_new(l->tk_str->cstr);
    bool is_destr = false;
    if (skip_balanced_pattern(l)) {
        lex_skip_empty(l);
        if (l->tk == '=') {
            is_destr = true;
        }
    }
    *l = saved;
    mstr_cpy(l->tk_str, saved_str->cstr);
    mstr_free(saved_str);
    return is_destr;
}

bool stmt_var(lex_t* l, bytecode_t* bc) {
    opr_code_t op;

    if (l->tk == LEX_R_VAR) {
        if (!lex_chkread(l, LEX_R_VAR)) {
            return false;
        }
        op = INSTR_VAR;
    } else if (l->tk == LEX_R_SAFE_VAR) {
        if (!lex_chkread(l, LEX_R_SAFE_VAR)) {
            return false;
        }
        op = INSTR_SAFE_VAR;
    } else {
        if (!lex_chkread(l, LEX_R_CONST)) {
            return false;
        }
        op = INSTR_CONST;
    }

    while (!is_stmt_end(l->tk)) {
        // ES6 destructuring declaration: let [..] = .. / let {..} = ..
        if (l->tk == '[' || l->tk == '{') {
            if (!stmt_var_destructure(l, bc, op)) {
                return false;
            }
            if (!is_stmt_end(l->tk)) {
                if (!lex_chkread(l, ',')) {
                    return false;
                }
            }
            continue;
        }
        mstr_t* vname = mstr_new(l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
			mstr_free(vname);
            return false;
        }
        bc_gen_str(bc, op, vname->cstr);
        if (g_vardecl_bc != NULL && op == INSTR_VAR) {
            /* ES5 var hoisting: the scan pass also declares the name at the
             * enclosing function/script top; the initializer stays in place. */
            bc_gen_str(g_vardecl_bc, INSTR_VAR, vname->cstr);
        }
        // sort out initialiser
        if (l->tk == '=') {
            if (!lex_chkread(l, '=')) {
				mstr_free(vname);
                return false;
            }
            bc_gen_str(bc, INSTR_LOAD, vname->cstr);
            if (!base(l, bc)) {
                mstr_free(vname);
                return false;
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
        }
        if (!is_stmt_end(l->tk)) {
            if (l->tk == ',') {
                if (!lex_chkread(l, ',')) {
                    mstr_free(vname);
                    return false;
                }
            } else if (lex_had_newline(l) && !tk_continues_expr(l->tk)) {
                /* ASI: `var x = 1\nfunction f(){}` - the line break ends the
                 * declaration list; leave the token to the next statement. */
                mstr_free(vname);
                return true;
            } else if (!lex_chkread(l, ',')) {
                mstr_free(vname);
                return false;
            }
        }
        mstr_free(vname);
    }
    return lex_chkread_stmt_end(l);
}

bool stmt_if(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_IF)) {
        return false;
    }
    if (!lex_chkread(l, '(')) {
        return false;
    }
    if (!expr_seq(l, bc)) {
        return false;
    } //condition
    if (!lex_chkread(l, ')')) {
        return false;
    }
    PC pc = bc_reserve(bc);
    lex_skip_empty(l);
    if (!statement(l, bc)) {
        return false;
    }
    lex_skip_empty(l);

    if (l->tk == LEX_R_ELSE) {
        if (!lex_chkread(l, LEX_R_ELSE)) {
            return false;
        }
        PC pc2 = bc_reserve(bc);
        bc_set_instr(bc, pc, INSTR_NJMP, ILLEGAL_PC);
        lex_skip_empty(l);
        if (!statement(l, bc)) {
            return false;
        }
        bc_set_instr(bc, pc2, INSTR_JMP, ILLEGAL_PC);
    } else {
        bc_set_instr(bc, pc, INSTR_NJMP, ILLEGAL_PC);
    }
    return true;
}

bool stmt_while(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_WHILE)) {
        return false;
    }
    bc_gen(bc, INSTR_LOOP);
    PC pc = bc_reserve(bc); //to init, nil for while statement.
    PC pc_condition = bc_add_instr(bc, pc, INSTR_JMP, pc + 2) - 1; //jmp to loop (for continue anchor).
    PC pc_break = bc_reserve(bc); //jump out of loop (for break anchor);

    if (!lex_chkread(l, '(')) {
        return false;
    }
    if (!expr_seq(l, bc)) {
        return false;
    } //condition
    if (!lex_chkread(l, ')')) {
        return false;
    }

    bc_add_instr(bc, pc_break, INSTR_NJMPB, ILLEGAL_PC); //not jump back to break anchor;

    /* ES6: a loop body is a block statement, so every iteration gets a FRESH
     * scope. Without it a body-level `let x` lives in the loop scope for the whole
     * loop: iteration 2 collides with iteration 1's binding, and since handle_const
     * keeps the existing binding instead of throwing, the body silently reuses the
     * previous round's value where real JS would see a fresh `undefined` - enough to
     * hang a work loop that breaks on an uninitialized flag (React's commit loop).
     * The block is pushed only when the condition passed, so every exit path is
     * balanced: fall-through runs the BLOCK_END below, `break`/`continue` pop scopes
     * down to the loop scope (vm_do_break/vm_do_continue), and the back edge plus the
     * condition-false NJMPB rebalance through vm_pop_scopes_to_loop. */
    bc_gen(bc, INSTR_BLOCK);

    if (!stmt_loop_block(l, bc)) {
        return false;
    }

    bc_gen(bc, INSTR_BLOCK_END);

    bc_add_instr(bc, pc_condition, INSTR_JMPB, ILLEGAL_PC); //coninue anchor;
    pc = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_break, INSTR_JMP, pc - 1); // end anchor;
    return true;
}

/* do { BODY } while (COND);  -- runs BODY once before the first COND test.
 * Mirrors stmt_while's scope anchors but puts the body ahead of the condition.
 * handle_block sets sc->pc_start = P+2 (continue) and sc->pc = P+3 (break)
 * when INSTR_LOOP sits at index P, so the layout is:
 *   [P]   LOOP
 *   [P+1] entry    JMP -> body_start  (fall-through executes BODY first)
 *   [P+2] continue JMP -> cond_start  (sc->pc_start)
 *   [P+3] break    JMP -> LOOP_END    (sc->pc; also NJMPB target when COND false)
 *   body_start: BODY
 *   cond_start: COND
 *             NJMPB -> P+3   (COND false -> break anchor -> end)
 *             JMPB  -> body_start (COND true -> loop back)
 *   LOOP_END */
bool stmt_do(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_DO)) {
        return false;
    }
    bc_gen(bc, INSTR_LOOP);
    PC pc_entry = bc_reserve(bc);    // P+1: entry -> body
    PC pc_continue = bc_reserve(bc); // P+2: continue anchor (sc->pc_start)
    PC pc_break = bc_reserve(bc);    // P+3: break anchor (sc->pc)

    lex_skip_empty(l);
    /* Per-iteration body scope, exactly like stmt_while. body_start must stay the
     * BLOCK slot: the condition-true back edge jumps here, and handle_jmpb pops any
     * scope left open above the loop before landing. */
    PC body_start = bc->cindex;
    bc_gen(bc, INSTR_BLOCK);
    if (!stmt_loop_block(l, bc)) {
        return false;
    }
    bc_gen(bc, INSTR_BLOCK_END);

    lex_skip_empty(l);
    if (!lex_chkread(l, LEX_R_WHILE)) {
        return false;
    }
    lex_skip_empty(l);
    if (!lex_chkread(l, '(')) {
        return false;
    }
    PC cond_start = bc->cindex;
    if (!expr_seq(l, bc)) {
        return false;
    }
    if (!lex_chkread(l, ')')) {
        return false;
    }
    lex_skip_empty(l);
    if (l->tk == ';') { // do-while ends with ';' (tolerate ASI when absent)
        lex_chkread(l, ';');
    }

    bc_add_instr(bc, pc_break, INSTR_NJMPB, ILLEGAL_PC);  // COND false -> break anchor
    bc_add_instr(bc, body_start, INSTR_JMPB, ILLEGAL_PC); // COND true  -> body

    PC end = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_entry, INSTR_JMP, body_start);
    bc_set_instr(bc, pc_continue, INSTR_JMP, cond_start);
    bc_set_instr(bc, pc_break, INSTR_JMP, end - 1);
    return true;
}

bool stmt_for_in(lex_t* l, bytecode_t* bc,
        PC pc_condition,
        PC pc_break,
        mstr_t* loop_var,
        opr_code_t var_op) {

    // For-in loop implementation using INSTR_ARRAY_AT
    
    // Store the object in a temporary variable
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_in_obj");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_obj");
    // Load the object to iterate over. The for-in RHS is a full Expression, so
    // `for (k in sideEffect(), obj)` is a comma expression yielding `obj`.
    if (!expr_seq(l, bc)) {
        return false;
    }
    if (!lex_chkread(l, ')')) {
        return false;
    }
    // The arr value is already on the stack from the base(l, bc) call above
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);

    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_in_keys");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_keys");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_obj");
    /* JS for-in semantics: arrays yield their indices as strings, objects yield
     * own enumerable keys. Handled by the native __enum_keys(o) helper. */
    bc_gen_str(bc, INSTR_CALL, "__enum_keys$1");
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);

    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_in_size");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_size");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_keys");
    bc_gen_str(bc, INSTR_CALLO, "length");
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);
    
    // Generate variable declaration bytecode for loop variable
    if (loop_var) {
        bc_gen_str(bc, var_op, loop_var->cstr);
        if (g_vardecl_bc != NULL && var_op == INSTR_VAR) {
            bc_gen_str(g_vardecl_bc, INSTR_VAR, loop_var->cstr); // ES5 var hoisting
        }
    }
    
    // Initialize index to 0
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_in_idx");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_idx");
    bc_gen_int(bc, INSTR_INT, 0);
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);
    
    // Condition: check if the current member is not empty or undefined
    PC cond_pc = bc->cindex;
    
    // Load the object and current index
    bc_gen_str(bc, INSTR_LOAD, "__for_in_idx");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_size");
    bc_gen(bc, INSTR_LES);
    
    // Jump out of loop if the member is undefined
    bc_add_instr(bc, pc_break, INSTR_NJMPB, ILLEGAL_PC);
    
    // Store the current index in the loop variable
    if (loop_var) {
        bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
        bc_gen_str(bc, INSTR_LOAD, "__for_in_keys");
        bc_gen_str(bc, INSTR_LOAD, "__for_in_idx");
        bc_gen(bc, INSTR_ARRAY_AT);
        bc_gen(bc, INSTR_ASIGN);
        bc_gen(bc, INSTR_POP);
    }
    
    /* Per-iteration body scope (see stmt_while). The increment below doubles as the
     * `continue` anchor and stays OUTSIDE the block; vm_do_continue pops the
     * iteration scope before jumping there, so the index still advances. */
    bc_gen(bc, INSTR_BLOCK);

    // Loop body
    if (!stmt_loop_block(l, bc)) {
        return false;
    }

    bc_gen(bc, INSTR_BLOCK_END);
    
    // Increment index  (also the `continue` target, so the index always advances)
    PC incr_pc = bc->cindex;
    bc_gen_str(bc, INSTR_LOAD, "__for_in_idx");
    bc_gen(bc, INSTR_PPLUS);
    bc_gen(bc, INSTR_POP);
    
    bc_add_instr(bc, cond_pc, INSTR_JMPB, ILLEGAL_PC); //after increment -> condition check
    
    PC pc = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_break, INSTR_JMP, pc - 1); // end anchor;
    bc_set_instr(bc, pc_condition, INSTR_JMP, incr_pc); // continue anchor -> increment
    
    if (loop_var) {
        mstr_free(loop_var);
    }
    return true;
}

/** ES6 for...of loop: iterate over the values of an array (or any object
 *  exposing length() and index access). The iterable expression has NOT been
 *  consumed yet when this is called. */
bool stmt_for_of(lex_t* l, bytecode_t* bc,
        PC pc_condition,
        PC pc_break,
        mstr_t* loop_var,
        opr_code_t var_op,
        lex_t* destr_pat,
        opr_code_t destr_op,
        bool is_for_await) {

    // __for_of_iter = GetIterator(<iterable>) via the iteration protocol
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_of_iter");
    bc_gen_str(bc, INSTR_LOAD, "__for_of_iter");
    // The for-of RHS is a full Expression: `for (x of a, b)` iterates b.
    if (!expr_seq(l, bc)) {
        return false;
    }
    if (!lex_chkread(l, ')')) {
        return false;
    }
    bc_gen(bc, INSTR_GET_ITER); // pop iterable, push its iterator
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);

    // __for_of_step holds the current { value, done } result from next()
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_of_step");

    // declare the loop variable
    if (loop_var) {
        bc_gen_str(bc, var_op, loop_var->cstr);
        if (g_vardecl_bc != NULL && var_op == INSTR_VAR) {
            bc_gen_str(g_vardecl_bc, INSTR_VAR, loop_var->cstr); // ES5 var hoisting
        }
    }

    /* Destructuring loop variable (`for (const [k,v] of ..)`): declare the
     * pattern's leaf variables once here (before the loop) by destructuring the
     * element holder, which is still undefined - the values are (re)assigned on
     * every iteration below. */
    if (destr_pat != NULL) {
        lex_t pl = *destr_pat;
        pl.tk_str = mstr_new("");
        bool dok = destructure_pattern(&pl, bc, destr_op, loop_var->cstr);
        mstr_free(pl.tk_str);
        if (!dok) {
            if (loop_var) mstr_free(loop_var);
            return false;
        }
    }

    /* condition anchor: fetch the next step, `__for_of_step = iter.next()`,
     * then break out when `step.done` is truthy. `continue` re-enters here. */
    PC cond_pc = bc->cindex;
    /* ES6 loop bodies are blocks: every iteration gets a FRESH scope, so a
     * body-level `const x` / `let x` re-binds instead of colliding with the
     * previous iteration's binding (the loop scope itself lives for the whole
     * loop; without a per-iteration block the second iteration throws
     * "let 'x' has already existed"). The block is pushed at the TOP of the
     * iteration so every exit path - fall-through, continue, break and the
     * natural done-jump - leaves it through exactly one BLOCK_END. */
    bc_gen(bc, INSTR_BLOCK);
    bc_gen_str(bc, INSTR_LOAD, "__for_of_step");
    bc_gen_str(bc, INSTR_LOAD, "__for_of_iter");
    bc_gen_str(bc, INSTR_CALLO, "next");
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);
    bc_gen_str(bc, INSTR_LOAD, "__for_of_step");
    bc_gen_str(bc, INSTR_GET, "done");
    bc_gen(bc, INSTR_NOT); // continue while !done
    /* done-jump: retargeted below (as a FORWARD NJMP) to the BLOCK_END
     * trampoline, because it leaves from INSIDE the iteration block (unlike
     * `break`, whose VM handler pops the block scopes itself before jumping).
     * Emitted as NJMPB for now only to reserve the slot; NJMPB jumps backward
     * and the trampoline lives ahead of it. */
    PC pc_njmpb = bc_add_instr(bc, pc_break, INSTR_NJMPB, ILLEGAL_PC) - 1;

    // loop_var = __for_of_step.value
    if (loop_var) {
        bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
        bc_gen_str(bc, INSTR_LOAD, "__for_of_step");
        bc_gen_str(bc, INSTR_GET, "value");
        bc_gen(bc, INSTR_ASIGN);
        bc_gen(bc, INSTR_POP);
    }

    /* Destructuring loop variable: (re)bind the pattern's leaves from the
     * current element on every iteration. decl_op is 0 since the leaves were
     * already declared once before the loop. */
    if (destr_pat != NULL) {
        lex_t pl = *destr_pat;
        pl.tk_str = mstr_new("");
        bool dok = destructure_pattern(&pl, bc, 0, loop_var->cstr);
        mstr_free(pl.tk_str);
        if (!dok) {
            if (loop_var) mstr_free(loop_var);
            return false;
        }
    }

    // loop body
    if (!stmt_loop_block(l, bc)) {
        return false;
    }

    bc_gen(bc, INSTR_BLOCK_END);   /* fall-through pops the iteration block */
    /* back edge -> cond_pc (pushes the next iteration's block) */
    PC pc_back = bc_add_instr(bc, cond_pc, INSTR_JMPB, ILLEGAL_PC) - 1;

    /* done trampoline: pop the iteration block, then leave. `break` needs no
     * trampoline - handle_break pops every scope above the loop scope itself
     * and lands straight on the loop's break slot. */
    PC pc_brk_trap = bc->cindex;
    bc_gen(bc, INSTR_BLOCK_END);
    PC pc_exit_jmp = bc_reserve(bc);            /* -> LOOP_END, patched below */

    PC pc = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_exit_jmp, INSTR_JMP, pc - 1);
    bc_set_instr(bc, pc_njmpb, INSTR_NJMP, pc_brk_trap);  /* NJMP: forward jump */
    bc_set_instr(bc, pc_break, INSTR_JMP, pc - 1);
    /* continue: the VM handler already popped the iteration block, so land on
     * the back edge (not on pc_bend's BLOCK_END, which would pop twice). */
    bc_set_instr(bc, pc_condition, INSTR_JMP, pc_back);

    if (loop_var) {
        mstr_free(loop_var);
    }
    return true;
}

bool stmt_for(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_FOR)) {
        return false;
    }
    /* ES2018 `for await (x of y)`: consume the optional `await`. The loop body
     * then awaits each value produced by the async iterator. */
    bool is_for_await = false;
    if (l->tk == LEX_R_AWAIT) {
        if (!lex_chkread(l, LEX_R_AWAIT)) {
            return false;
        }
        is_for_await = true;
        lex_skip_empty(l);
    }
    PC pc = bc_gen(bc, INSTR_LOOP);
    bc_add_instr(bc, pc, INSTR_JMP, pc + 3); //jmp to init.
    PC pc_condition = bc_reserve(bc); //jump to condition (for continue anchor);
    PC pc_break = bc_reserve(bc); //jump out of loop if not condition.

    if (!lex_chkread(l, '(')) {
        return false;
    }
    
    // Check if it's a for-in loop
    bool is_for_in = false;
    bool is_for_of = false;
    mstr_t* loop_var = NULL;
    opr_code_t var_op = INSTR_VAR;
    bool loop_destr = false;          // destructuring loop variable (for-of/for-in)
    lex_t pd_saved;                   // pattern-start lexer state when loop_destr
    opr_code_t destr_op = INSTR_VAR;  // declaration op for the pattern's leaves
    memset(&pd_saved, 0, sizeof(pd_saved));
    
    // Handle variable declaration for for-in loop specially
    if (l->tk == LEX_R_VAR || l->tk == LEX_R_SAFE_VAR || l->tk == LEX_R_CONST) {
        // Save the variable declaration type
        if (l->tk == LEX_R_VAR) {
            var_op = INSTR_VAR;
        } else if (l->tk == LEX_R_SAFE_VAR) {
            var_op = INSTR_SAFE_VAR;
        } else {
            var_op = INSTR_CONST;
        }
        
        // Consume the var/let/const keyword
        lex_chkread(l, l->tk);
        lex_skip_empty(l);
        
        // Read the loop variable: either a simple name or a destructuring
        // pattern (`[a, b]` / `{x, y}`). A pattern is staged through a hidden
        // temp element and its leaves are (re)bound inside stmt_for_of.
        if (l->tk == '[' || l->tk == '{') {
            loop_destr = true;
            destr_op = var_op;          // leaves keep the original const/let/var
            var_op = INSTR_SAFE_VAR;    // the element temp must stay mutable
            loop_var = mstr_new("__for_of_elem");
            pd_saved = *l;              // capture the pattern start for re-parse
            skip_balanced_pattern(l);   // consume the whole [...] / {...}
            lex_skip_empty(l);
        } else {
            if (l->tk != LEX_ID) {
                return false;
            }
            loop_var = mstr_new(l->tk_str->cstr);
            lex_chkread(l, LEX_ID);
            lex_skip_empty(l);
        }
        
        // Check if the next token is "in" (for-in loop). `in` is lexed as the
        // reserved word LEX_R_IN (it is also the binary `in` operator).
        if (l->tk == LEX_R_IN) {
            is_for_in = true;
            lex_chkread(l, LEX_R_IN); // consume "in"
            lex_skip_empty(l);
        } else if (l->tk == LEX_ID && strcmp(l->tk_str->cstr, "of") == 0) {
            is_for_of = true;
            lex_chkread(l, LEX_ID); // consume "of"
            lex_skip_empty(l);
        } else if (loop_destr) {
            /* C-style for with a destructuring FIRST declarator, e.g.
             *   for (var [r, d, c] = pair, o = 1, b = 0; ...; ...)
             * The in/of probe above skipped the pattern as a throwaway for-of
             * element temp; for a plain C-style loop that temp is wrong - the
             * pattern's leaf variables would never be bound (they read back as
             * undefined) and calling one as a function aborts the VM. Rewind
             * the lexer to the pattern start and emit real leaf bindings the
             * same way stmt_var() does, then finish any remaining declarators. */
            mstr_free(loop_var);
            loop_var = NULL;      // no element temp, no per-iteration binding
            var_op = INSTR_VAR;   // suppress the per-iteration block emitted below
            *l = pd_saved;        // rewind to the pattern's '[' / '{'
            if (!stmt_var_destructure(l, bc, destr_op)) {
                return false;
            }
            lex_skip_empty(l);
            /* Remaining comma-separated declarators (patterns or plain names). */
            while (l->tk == ',') {
                if (!lex_chkread(l, ',')) {
                    return false;
                }
                lex_skip_empty(l);
                if (l->tk == '[' || l->tk == '{') {
                    if (!stmt_var_destructure(l, bc, destr_op)) {
                        return false;
                    }
                    lex_skip_empty(l);
                    continue;
                }
                if (l->tk != LEX_ID) {
                    return false;
                }
                mstr_t* extra = mstr_new(l->tk_str->cstr);
                lex_chkread(l, LEX_ID);
                bc_gen_str(bc, destr_op, extra->cstr);
                if (g_vardecl_bc != NULL && destr_op == INSTR_VAR) {
                    bc_gen_str(g_vardecl_bc, INSTR_VAR, extra->cstr); // ES5 var hoisting
                }
                if (l->tk == '=') {
                    lex_chkread(l, '=');
                    bc_gen_str(bc, INSTR_LOAD, extra->cstr);
                    if (!base(l, bc)) {
                        mstr_free(extra);
                        return false;
                    }
                    bc_gen(bc, INSTR_ASIGN);
                    bc_gen(bc, INSTR_POP);
                }
                mstr_free(extra);
                lex_skip_empty(l);
            }
            if (l->tk != ';') {
                return false;
            }
            lex_chkread(l, ';');
            lex_skip_empty(l);
        } else {
            // Standard for loop variable initialization
            // Generate variable declaration bytecode
            if (loop_var) {
                bc_gen_str(bc, var_op, loop_var->cstr);
                if (g_vardecl_bc != NULL && var_op == INSTR_VAR) {
                    /* ES5 var hoisting: `for(var i=...)` declares i at the
                     * enclosing function/script top too. */
                    bc_gen_str(g_vardecl_bc, INSTR_VAR, loop_var->cstr);
                }
                /* ES6 for-let/const: declare the loop-scope temp that shuttles
                 * the loop variable in/out of the per-iteration block below. */
                if (var_op != INSTR_VAR) {
                    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_let_tmp");
                }
            }
            if (l->tk == '=') {
                lex_chkread(l, '=');
                bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
                if (!base(l, bc)) {
                    mstr_free(loop_var);
                    return false;
                }
                bc_gen(bc, INSTR_ASIGN);
                bc_gen(bc, INSTR_POP);
            }
            /* Comma-separated declaration list: `for (var a = 1, b = 2, c; ...)`. */
            while (l->tk == ',') {
                if (!lex_chkread(l, ',')) {
                    mstr_free(loop_var);
                    return false;
                }
                lex_skip_empty(l);
                if (l->tk != LEX_ID) {
                    mstr_free(loop_var);
                    return false;
                }
                mstr_t* extra = mstr_new(l->tk_str->cstr);
                lex_chkread(l, LEX_ID);
                bc_gen_str(bc, var_op, extra->cstr);
                if (g_vardecl_bc != NULL && var_op == INSTR_VAR) {
                    bc_gen_str(g_vardecl_bc, INSTR_VAR, extra->cstr); // ES5 var hoisting
                }
                if (l->tk == '=') {
                    lex_chkread(l, '=');
                    bc_gen_str(bc, INSTR_LOAD, extra->cstr);
                    if (!base(l, bc)) {
                        mstr_free(extra);
                        mstr_free(loop_var);
                        return false;
                    }
                    bc_gen(bc, INSTR_ASIGN);
                    bc_gen(bc, INSTR_POP);
                }
                mstr_free(extra);
                lex_skip_empty(l);
            }
            if (l->tk != ';') {
                mstr_free(loop_var);
                return false;
            }
            lex_chkread(l, ';');
            lex_skip_empty(l);
        }
    } else if (l->tk == ';') {
        // Empty init clause: `for(; cond; iter)`
        lex_chkread(l, ';');
        lex_skip_empty(l);
    } else {
        /* Bare-identifier for-in / for-of: `for (t in obj)` / `for (x of arr)`
         * where the loop variable is an existing binding (no var/let/const).
         * Peek past the identifier; if it is not followed by `in`/`of`, restore
         * the lexer fully and let the generic init path handle `for (i = 0; ...)`. */
        if (l->tk == LEX_ID) {
            lex_t saved = *l;                 // shallow copy: keeps original tk_str ptr
            mstr_t* saved_tk_str = l->tk_str; // do not let scanning clobber caller token
            l->tk_str = mstr_new("");
            mstr_cpy(l->tk_str, saved_tk_str->cstr);
            mstr_t* bare = mstr_new(saved_tk_str->cstr); // capture the loop-variable name
            lex_get_next_token(l);            // move past the identifier
            lex_skip_empty(l);
            bool bare_in = (l->tk == LEX_R_IN);
            bool bare_of = (!bare_in && l->tk == LEX_ID && strcmp(l->tk_str->cstr, "of") == 0);
            if (bare_in || bare_of) {
                if (bare_in) lex_chkread(l, LEX_R_IN);
                else         lex_chkread(l, LEX_ID);
                lex_skip_empty(l);
                /* l->tk is now the first token of the iterable, held in the temp
                 * buffer. Copy its text into the caller's buffer and keep the
                 * advanced position, so base() parses the iterable (not the stale
                 * loop-variable name). */
                mstr_cpy(saved_tk_str, l->tk_str->cstr);
                mstr_free(l->tk_str);       // drop the temp scan buffer
                l->tk_str = saved_tk_str;   // caller buffer now holds the live token
                /* The bare variable already exists (or is a global); declare it
                 * SAFE_VAR so the per-iteration reassignment is allowed. */
                if (bare_in) {
                    return stmt_for_in(l, bc, pc_condition, pc_break, bare, INSTR_SAFE_VAR);
                }
                return stmt_for_of(l, bc, pc_condition, pc_break, bare, INSTR_SAFE_VAR,
                        NULL, INSTR_SAFE_VAR, is_for_await);
            }
            mstr_free(bare);
            mstr_free(l->tk_str);
            *l = saved;                     // not for-in/of: full restore
        }
        // Standard for loop init statement. statement() consumes the init's
        // own ';' terminator, so the token that follows belongs to the
        // CONDITION clause; eating another ';' here would swallow the
        // condition separator of `for (i = 1;;)` and strand the parse.
        if (!statement(l, bc)) {
            return false;
        }
        lex_skip_empty(l);
    }
    
    /* A for-in/for-of loop variable is re-bound on every iteration, so it must
     * be mutable even when written `const` (JS gives each iteration a fresh
     * binding). Declare it - and any destructuring leaves - with SAFE_VAR so
     * reassignment on iteration 2+ is allowed. */
    if (is_for_in) {
        if (var_op == INSTR_CONST) var_op = INSTR_SAFE_VAR;
        return stmt_for_in(l, bc, pc_condition, pc_break, loop_var, var_op);
    }

    if (is_for_of) {
        if (var_op == INSTR_CONST) var_op = INSTR_SAFE_VAR;
        if (destr_op == INSTR_CONST) destr_op = INSTR_SAFE_VAR;
        return stmt_for_of(l, bc, pc_condition, pc_break, loop_var, var_op,
                loop_destr ? &pd_saved : NULL, destr_op, is_for_await);
    }

    // Standard for loop implementation
    PC cond_pc = bc->cindex; //condition-check anchor (init falls through here)
    if (l->tk == ';') {
        // Empty condition (`for(;;)`) is always true.
        bc_gen(bc, INSTR_TRUE);
    } else if (!expr_seq(l, bc)) { //condition
        if (loop_var) {
            mstr_free(loop_var);
        }
        return false;
    }
    if (!lex_chkread(l, ';')) {
        if (loop_var) {
            mstr_free(loop_var);
        }
        return false;
    }
    lex_skip_empty(l);
    bc_add_instr(bc, pc_break, INSTR_NJMPB, ILLEGAL_PC); //jump out of loop if not condition.
    PC pcl = bc_reserve(bc); //jump to loop .skip the iterrator

    PC pci = bc->cindex;  //iterator anchor;
    /* `continue` jumps to the loop scope's pc_start (== pc_condition). Point it
     * at the iterator so a continue advances the loop variable before re-testing
     * the condition; otherwise `for(..;..;..) { continue; }` never terminates. */
    bc_set_instr(bc, pc_condition, INSTR_JMP, pci);
    bool has_iter = (l->tk != ')'); // empty iterator clause: `for(a; b;)`
    if (has_iter) {
        if (!expr_seq(l, bc)) { //iterator statement
            if (loop_var) {
                mstr_free(loop_var);
            }
            return false;
        }
    }
    if (!lex_chkread(l, ')')) {
        if (loop_var) {
            mstr_free(loop_var);
        }
        return false;
    }
    if (has_iter) {
        bc_gen(bc, INSTR_POP); //pop the stack.
    }

    bc_add_instr(bc, cond_pc, INSTR_JMPB, ILLEGAL_PC); //after iterator -> condition check

    bc_set_instr(bc, pcl, INSTR_JMP, ILLEGAL_PC); // loop anchor;

    /* ES6 `for (let i ...)`: every iteration gets its OWN binding of the loop
     * variable, so closures made in the body capture that iteration's value.
     * The body is wrapped in a block that re-declares the variable and copies
     * its value in from (and back out to) the shared loop-scope binding via
     * the hidden __for_let_tmp; handle_func captures the block var. */
    bool per_iter = (loop_var != NULL && var_op != INSTR_VAR);
    if (per_iter) {
        // __for_let_tmp = i
        bc_gen_str(bc, INSTR_LOAD, "__for_let_tmp");
        bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
        bc_gen(bc, INSTR_ASIGN);
        bc_gen(bc, INSTR_POP);
        bc_gen(bc, INSTR_BLOCK);
        // let i(block) = __for_let_tmp
        bc_gen_str(bc, INSTR_SAFE_VAR, loop_var->cstr);
        bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
        bc_gen_str(bc, INSTR_LOAD, "__for_let_tmp");
        bc_gen(bc, INSTR_ASIGN);
        bc_gen(bc, INSTR_POP);
    }
    else {
        /* No loop-variable block was opened above, but the body still needs its own
         * per-iteration scope for body-level let/const (see stmt_while). */
        bc_gen(bc, INSTR_BLOCK);
    }

    // Loop body
    if (!stmt_loop_block(l, bc)) {
        if (loop_var) {
            mstr_free(loop_var);
        }
        return false;
    }

    if (per_iter) {
        /* Copy the (possibly body-modified) block binding back out so the step
         * expression sees it. Skipped for `const`: the binding cannot have
         * changed and re-assigning a const loop variable would throw. */
        if (var_op == INSTR_SAFE_VAR) {
            bc_gen_str(bc, INSTR_LOAD, "__for_let_tmp");
            bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
        }
        bc_gen(bc, INSTR_BLOCK_END);
        if (var_op == INSTR_SAFE_VAR) {
            bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
            bc_gen_str(bc, INSTR_LOAD, "__for_let_tmp");
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
        }
    }
    else {
        bc_gen(bc, INSTR_BLOCK_END);
    }

    bc_add_instr(bc, pci, INSTR_JMPB, ILLEGAL_PC); //jump to iterator anchor;
    pc = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_break, INSTR_JMP, pc - 1); // end anchor;
    
    if (loop_var) {
        mstr_free(loop_var);
    }
    return true;
}

bool stmt_break(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_BREAK)) {
        return false;
    }
    /* Labeled break: `break outer;`. This lexer folds newlines into ordinary
     * whitespace (there is no ASI newline token), so a following identifier is
     * taken as a label only when it is itself terminated by a statement end
     * (';' or '}'). That distinguishes `break label;` from an unlabeled `break`
     * that merely precedes an identifier statement such as `break` / `i++`. */
    char label[64];
    label[0] = 0;
    if (l->tk == LEX_ID) {
        lex_t saved = *l;
        mstr_t* saved_tk_str = l->tk_str;
        l->tk_str = mstr_new("");
        mstr_cpy(l->tk_str, saved_tk_str->cstr);
        strncpy(label, l->tk_str->cstr, sizeof(label) - 1);
        label[sizeof(label) - 1] = 0;
        lex_get_next_token(l); // token right after the identifier
        bool is_label = (l->tk == ';' || l->tk == '}' || l->tk == LEX_EOF);
        mstr_free(l->tk_str);
        *l = saved; // restore position and the original tk_str pointer
        if (!is_label) {
            label[0] = 0;
        }
    }
    if (label[0] != 0) {
        if (!lex_chkread(l, LEX_ID)) { // consume the label identifier
            return false;
        }
    }
    if (!lex_chkread_stmt_end(l)) {
        return false;
    }
    if (label[0] != 0) {
        bc_gen_str(bc, INSTR_BREAK, label);
    } else {
        bc_gen(bc, INSTR_BREAK);
    }
    return true;
}

bool stmt_continue(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_CONTINUE)) {
        return false;
    }
    /* Labeled continue: `continue outer;` targets the loop labeled `outer`, not
     * the innermost loop. Mirrors stmt_break's label detection - this lexer folds
     * newlines into whitespace (no ASI token), so a following identifier counts
     * as a label only when it is itself terminated by a statement end (';' or
     * '}'), distinguishing `continue label;` from an unlabeled `continue` that
     * merely precedes an identifier statement. */
    char label[64];
    label[0] = 0;
    if (l->tk == LEX_ID) {
        lex_t saved = *l;
        mstr_t* saved_tk_str = l->tk_str;
        l->tk_str = mstr_new("");
        mstr_cpy(l->tk_str, saved_tk_str->cstr);
        strncpy(label, l->tk_str->cstr, sizeof(label) - 1);
        label[sizeof(label) - 1] = 0;
        lex_get_next_token(l); // token right after the identifier
        bool is_label = (l->tk == ';' || l->tk == '}' || l->tk == LEX_EOF);
        mstr_free(l->tk_str);
        *l = saved; // restore position and the original tk_str pointer
        if (!is_label) {
            label[0] = 0;
        }
    }
    if (label[0] != 0) {
        if (!lex_chkread(l, LEX_ID)) { // consume the label identifier
            return false;
        }
    }
    if (!lex_chkread_stmt_end(l)) {
        return false;
    }
    if (label[0] != 0) {
        bc_gen_str(bc, INSTR_CONTINUE, label);
    } else {
        bc_gen(bc, INSTR_CONTINUE);
    }
    return true;
}

bool stmt_function(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_FUNCTION)) {
        return false;
    }
    mstr_t* fname = mstr_new("");
    factor_def_func(l, bc, fname);
    bc_gen_str(bc, INSTR_MEMBERN, fname->cstr);
    mstr_free(fname);
    return true;
}

bool stmt_include(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_INCLUDE)) {
        return false;
    }
    if (!base(l, bc)) {
        return false;
    }
    bc_gen(bc, INSTR_INCLUDE);
    return true;
}

/** ES6 modules: `import` / `export`.
 *
 * These compile down onto the existing single-global-scope VM using the module
 * registry in do_module() (mario.c). A module body runs once, its `export`
 * markers publish bindings into that module's namespace object, and an importer
 * reads them back out with ordinary GET + assignment. Bindings therefore land in
 * the global scope (the same model `include` uses); this is a pragmatic ESM,
 * not a fully isolated per-module scope, but it covers the common syntax: bare /
 * default / named / namespace / combined imports, and declaration / default /
 * list / re-export (`.. from`) / star exports. */

/* `from` and `as` are contextual keywords in module syntax: ordinary
 * identifiers the grammar gives meaning to. Match without consuming. */
static bool lex_is_word(lex_t* l, const char* word) {
    return (l->tk == LEX_ID && strcmp(l->tk_str->cstr, word) == 0);
}

/* Read a module binding name: an identifier, or a reserved word JS permits as
 * an import/export name (notably `default`). Captures the text and advances. */
static bool lex_read_binding_name(lex_t* l, mstr_t* out) {
    if (l->tk != LEX_ID && l->tk != LEX_R_DEFAULT) {
        if (!g_hoist_quiet) {
            mario_printf("import/export: expected a binding name! ");
            compile_error_pos(l, -1);
        }
        return false;
    }
    mstr_cpy(out, l->tk_str->cstr);
    lex_get_next_token(l);
    return true;
}

/* Consume the `from '<specifier>'` tail shared by import and re-export. */
static bool module_read_from(lex_t* l, mstr_t* spec) {
    if (!lex_is_word(l, "from")) {
        if (!g_hoist_quiet) {
            mario_printf("module syntax: expected 'from'! ");
            compile_error_pos(l, -1);
        }
        return false;
    }
    lex_get_next_token(l); // consume 'from'
    lex_skip_empty(l);
    if (l->tk != LEX_STR) {
        if (!g_hoist_quiet) {
            mario_printf("module syntax: expected a specifier string! ");
            compile_error_pos(l, -1);
        }
        return false;
    }
    mstr_cpy(spec, l->tk_str->cstr);
    lex_get_next_token(l); // consume the string
    return true;
}

/* Peek a class declaration's name without consuming any token, so the export
 * marker can reference it after factor_def_class compiles the class. */
static void export_peek_class_name(lex_t* l, mstr_t* out) {
    lex_t saved = *l;
    mstr_t* saved_str = mstr_new(l->tk_str->cstr);
    lex_get_next_token(l); // consume 'class' (l->tk is LEX_R_CLASS on entry)
    lex_skip_empty(l);
    if (l->tk == LEX_ID) {
        mstr_cpy(out, l->tk_str->cstr);
    }
    *l = saved;
    mstr_cpy(l->tk_str, saved_str->cstr);
    mstr_free(saved_str);
}

/* Emit one import binding: `dst = <ns>[src]` (src==NULL binds the whole
 * namespace object, for `import * as dst`). MODULE loads/evaluates the module
 * once (the registry caches it) and pushes its namespace; GET picks the exported
 * member; IMPORT_BIND writes the binding, bypassing the const guard so a name
 * that coincides with the source module's own top-level const is not an error. */
static void emit_import_binding(bytecode_t* bc, const char* spec, const char* src, const char* dst) {
    bc_gen_str(bc, INSTR_MODULE, spec);
    if (src != NULL) {
        bc_gen_str(bc, INSTR_GET, src);
    }
    bc_gen_str(bc, INSTR_IMPORT_BIND, dst);
}

/* True when the `import` token under the lexer starts an expression form
 * (`import(` or `import.`) rather than a declaration. Peeks one token on a
 * scratch copy and restores the lexer. */
static bool import_is_expression(lex_t* l) {
    lex_t saved = *l;
    mstr_t* saved_str = mstr_new(l->tk_str->cstr);
    lex_get_next_token(l); // consume 'import'
    bool expr = (l->tk == '(' || l->tk == '.');
    *l = saved;
    mstr_cpy(l->tk_str, saved_str->cstr);
    mstr_free(saved_str);
    return expr;
}

bool stmt_import(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_IMPORT)) {
        return false;
    }
    bc->chunk_is_module = true;   /* import syntax: this chunk is a module body */
    lex_skip_empty(l);

    /* import 'mod' : evaluate for side effects only. */
    if (l->tk == LEX_STR) {
        mstr_t* spec = mstr_new(l->tk_str->cstr);
        if (!lex_chkread(l, LEX_STR)) {
            mstr_free(spec);
            return false;
        }
        bc_gen_str(bc, INSTR_MODULE, spec->cstr);
        bc_gen(bc, INSTR_POP);
        mstr_free(spec);
        if (is_stmt_end(l->tk)) {
            lex_chkread_stmt_end(l);
        }
        return true;
    }

    /* Optional default binding: `import d from 'm'` / `import d, {..} from 'm'`. */
    mstr_t* def_name = mstr_new("");
    if (l->tk == LEX_ID) {
        mstr_cpy(def_name, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
            mstr_free(def_name);
            return false;
        }
        lex_skip_empty(l);
        if (l->tk == ',') {
            if (!lex_chkread(l, ',')) {
                mstr_free(def_name);
                return false;
            }
            lex_skip_empty(l);
        }
    }

    /* Optional namespace binding: `import * as ns from 'm'`. */
    mstr_t* ns_name = mstr_new("");
    if (l->tk == '*') {
        if (!lex_chkread(l, '*')) {
            mstr_free(def_name);
            mstr_free(ns_name);
            return false;
        }
        lex_skip_empty(l);
        if (!lex_is_word(l, "as")) {
            if (!g_hoist_quiet) {
                mario_printf("import: expected 'as' after '*'! ");
                compile_error_pos(l, -1);
            }
            mstr_free(def_name);
            mstr_free(ns_name);
            return false;
        }
        lex_get_next_token(l); // consume 'as'
        lex_skip_empty(l);
        if (l->tk != LEX_ID) {
            if (!g_hoist_quiet) {
                mario_printf("import: expected a namespace name! ");
                compile_error_pos(l, -1);
            }
            mstr_free(def_name);
            mstr_free(ns_name);
            return false;
        }
        mstr_cpy(ns_name, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
            mstr_free(def_name);
            mstr_free(ns_name);
            return false;
        }
        lex_skip_empty(l);
    }

    /* Named bindings: `import {a, b as c} from 'm'`. */
    m_array_t* srcs = array_new();
    m_array_t* dsts = array_new();
    bool ok = true;
    if (l->tk == '{') {
        if (!lex_chkread(l, '{')) {
            ok = false;
        }
        lex_skip_empty(l);
        while (ok && l->tk != '}') {
            mstr_t* s = mstr_new("");
            mstr_t* d = mstr_new("");
            if (!lex_read_binding_name(l, s)) {
                mstr_free(s);
                mstr_free(d);
                ok = false;
                break;
            }
            mstr_cpy(d, s->cstr);
            lex_skip_empty(l);
            if (lex_is_word(l, "as")) {
                lex_get_next_token(l); // consume 'as'
                lex_skip_empty(l);
                if (!lex_read_binding_name(l, d)) {
                    mstr_free(s);
                    mstr_free(d);
                    ok = false;
                    break;
                }
            }
            array_add(srcs, s);
            array_add(dsts, d);
            lex_skip_empty(l);
            if (l->tk == ',') {
                if (!lex_chkread(l, ',')) {
                    ok = false;
                    break;
                }
                lex_skip_empty(l);
            }
        }
        if (ok && !lex_chkread(l, '}')) {
            ok = false;
        }
        lex_skip_empty(l);
    }

    mstr_t* spec = mstr_new("");
    if (ok && !module_read_from(l, spec)) {
        ok = false;
    }

    if (ok) {
        if (def_name->len > 0) {
            emit_import_binding(bc, spec->cstr, "default", def_name->cstr);
        }
        if (ns_name->len > 0) {
            emit_import_binding(bc, spec->cstr, NULL, ns_name->cstr);
        }
        for (uint32_t i = 0; i < srcs->size; i++) {
            mstr_t* s = (mstr_t*)array_get(srcs, i);
            mstr_t* d = (mstr_t*)array_get(dsts, i);
            emit_import_binding(bc, spec->cstr, s->cstr, d->cstr);
        }
        if (is_stmt_end(l->tk)) {
            lex_chkread_stmt_end(l);
        }
    }

    mstr_free(spec);
    mstr_free(def_name);
    mstr_free(ns_name);
    array_free(srcs, (free_func_t)mstr_free);
    array_free(dsts, (free_func_t)mstr_free);
    return ok;
}

/* `export var/let/const x = .. [, y = ..]` : declare each binding then publish
 * it. Mirrors stmt_var's simple (non-destructuring) path with an EXPORT marker
 * appended per name. Emitted into `bc` only, so the hoist scan pass (bc==scratch)
 * discards it and the real pass emits it exactly once. */
static bool stmt_export_var(lex_t* l, bytecode_t* bc) {
    opr_code_t op;
    if (l->tk == LEX_R_VAR) {
        if (!lex_chkread(l, LEX_R_VAR)) return false;
        op = INSTR_VAR;
    } else if (l->tk == LEX_R_SAFE_VAR) {
        if (!lex_chkread(l, LEX_R_SAFE_VAR)) return false;
        op = INSTR_SAFE_VAR;
    } else {
        if (!lex_chkread(l, LEX_R_CONST)) return false;
        op = INSTR_CONST;
    }

    while (!is_stmt_end(l->tk)) {
        if (l->tk != LEX_ID) {
            if (!g_hoist_quiet) {
                mario_printf("export: expected a variable name! ");
                compile_error_pos(l, -1);
            }
            return false;
        }
        mstr_t* vname = mstr_new(l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
            mstr_free(vname);
            return false;
        }
        bc_gen_str(bc, op, vname->cstr);
        if (l->tk == '=') {
            if (!lex_chkread(l, '=')) {
                mstr_free(vname);
                return false;
            }
            bc_gen_str(bc, INSTR_LOAD, vname->cstr);
            if (!base(l, bc)) {
                mstr_free(vname);
                return false;
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
        }
        bc_gen_str(bc, INSTR_EXPORT, vname->cstr);
        mstr_free(vname);
        if (!is_stmt_end(l->tk)) {
            if (l->tk == ',') {
                if (!lex_chkread(l, ',')) return false;
            } else if (lex_had_newline(l) && !tk_continues_expr(l->tk)) {
                /* ASI, as in stmt_var: `export var f = function(){}\nexport ..`. */
                return true;
            } else {
                return false;
            }
        }
    }
    return lex_chkread_stmt_end(l);
}

bool stmt_export(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_EXPORT)) {
        return false;
    }
    bc->chunk_is_module = true;   /* export syntax: this chunk is a module body */
    lex_skip_empty(l);

    /* export default <assignment-expr> ; (function/class expressions included) */
    if (l->tk == LEX_R_DEFAULT) {
        if (!lex_chkread(l, LEX_R_DEFAULT)) return false;
        lex_skip_empty(l);
        if (!base(l, bc)) return false;
        bc_gen_str(bc, INSTR_EXPORT_VALUE, "default");
        if (is_stmt_end(l->tk)) lex_chkread_stmt_end(l);
        return true;
    }

    /* export function f(){} : the declaration is hoisted like a plain function
     * declaration (into g_funcdecl_bc during the scan pass), the EXPORT marker
     * rides `bc` so it runs after the hoisted definition. */
    if (l->tk == LEX_R_FUNCTION) {
        bytecode_t* dbc = (g_funcdecl_bc != NULL) ? g_funcdecl_bc : bc;
        if (!lex_chkread(l, LEX_R_FUNCTION)) return false;
        mstr_t* fname = mstr_new("");
        if (!factor_def_func(l, dbc, fname)) {
            mstr_free(fname);
            return false;
        }
        bc_gen_str(dbc, INSTR_MEMBERN, fname->cstr);
        bc_gen_str(bc, INSTR_EXPORT, fname->cstr);
        mstr_free(fname);
        return true;
    }

    /* export async function f(){} : same hoisting path as the plain form,
     * with g_async_pending marking the body async. */
    if (l->tk == LEX_R_ASYNC) {
        if (!lex_chkread(l, LEX_R_ASYNC)) return false;
        lex_skip_empty(l);
        if (l->tk != LEX_R_FUNCTION) {
            if (!g_hoist_quiet) {
                mario_printf("export: expected 'function' after 'async'! ");
                compile_error_pos(l, -1);
            }
            return false;
        }
        bytecode_t* dbc = (g_funcdecl_bc != NULL) ? g_funcdecl_bc : bc;
        if (!lex_chkread(l, LEX_R_FUNCTION)) return false;
        mstr_t* fname = mstr_new("");
        g_async_pending = 1;
        if (!factor_def_func(l, dbc, fname)) {
            g_async_pending = 0;
            mstr_free(fname);
            return false;
        }
        bc_gen_str(dbc, INSTR_MEMBERN, fname->cstr);
        bc_gen_str(bc, INSTR_EXPORT, fname->cstr);
        mstr_free(fname);
        return true;
    }

    /* export class C {} : factor_def_class binds C and pushes the class value;
     * discard it (like statement()) then publish the binding. */
    if (l->tk == LEX_R_CLASS) {
        mstr_t* cname = mstr_new("");
        export_peek_class_name(l, cname);
        if (!factor_def_class(l, bc)) {
            mstr_free(cname);
            return false;
        }
        bc_gen(bc, INSTR_POP);
        if (cname->len > 0) {
            bc_gen_str(bc, INSTR_EXPORT, cname->cstr);
        }
        mstr_free(cname);
        if (is_stmt_end(l->tk)) lex_chkread_stmt_end(l);
        return true;
    }

    /* export var/let/const .. */
    if (l->tk == LEX_R_VAR || l->tk == LEX_R_CONST || l->tk == LEX_R_SAFE_VAR) {
        return stmt_export_var(l, bc);
    }

    /* export * from 'm'  /  export * as ns from 'm' */
    if (l->tk == '*') {
        if (!lex_chkread(l, '*')) return false;
        lex_skip_empty(l);
        mstr_t* ns_name = mstr_new("");
        if (lex_is_word(l, "as")) {
            lex_get_next_token(l); // consume 'as'
            lex_skip_empty(l);
            if (l->tk != LEX_ID) {
                if (!g_hoist_quiet) {
                    mario_printf("export: expected a name after 'as'! ");
                    compile_error_pos(l, -1);
                }
                mstr_free(ns_name);
                return false;
            }
            mstr_cpy(ns_name, l->tk_str->cstr);
            if (!lex_chkread(l, LEX_ID)) {
                mstr_free(ns_name);
                return false;
            }
            lex_skip_empty(l);
        }
        mstr_t* spec = mstr_new("");
        if (!module_read_from(l, spec)) {
            mstr_free(ns_name);
            mstr_free(spec);
            return false;
        }
        bc_gen_str(bc, INSTR_MODULE, spec->cstr);
        if (ns_name->len > 0) {
            bc_gen_str(bc, INSTR_EXPORT_VALUE, ns_name->cstr);
        } else {
            bc_gen(bc, INSTR_EXPORT_STAR);
        }
        mstr_free(ns_name);
        mstr_free(spec);
        if (is_stmt_end(l->tk)) lex_chkread_stmt_end(l);
        return true;
    }

    /* export { a, b as c } [from 'm'] ; */
    if (l->tk == '{') {
        if (!lex_chkread(l, '{')) return false;
        lex_skip_empty(l);
        m_array_t* srcs = array_new();
        m_array_t* dsts = array_new();
        bool ok = true;
        while (ok && l->tk != '}') {
            mstr_t* s = mstr_new("");
            mstr_t* d = mstr_new("");
            if (!lex_read_binding_name(l, s)) {
                mstr_free(s);
                mstr_free(d);
                ok = false;
                break;
            }
            mstr_cpy(d, s->cstr);
            lex_skip_empty(l);
            if (lex_is_word(l, "as")) {
                lex_get_next_token(l); // consume 'as'
                lex_skip_empty(l);
                if (!lex_read_binding_name(l, d)) {
                    mstr_free(s);
                    mstr_free(d);
                    ok = false;
                    break;
                }
            }
            array_add(srcs, s);
            array_add(dsts, d);
            lex_skip_empty(l);
            if (l->tk == ',') {
                if (!lex_chkread(l, ',')) {
                    ok = false;
                    break;
                }
                lex_skip_empty(l);
            }
        }
        if (ok && !lex_chkread(l, '}')) {
            ok = false;
        }
        lex_skip_empty(l);

        if (ok && lex_is_word(l, "from")) {
            /* re-export: pull each name out of the source module's namespace. */
            mstr_t* spec = mstr_new("");
            if (!module_read_from(l, spec)) {
                mstr_free(spec);
                ok = false;
            } else {
                for (uint32_t i = 0; i < srcs->size; i++) {
                    mstr_t* s = (mstr_t*)array_get(srcs, i);
                    mstr_t* d = (mstr_t*)array_get(dsts, i);
                    bc_gen_str(bc, INSTR_MODULE, spec->cstr);
                    bc_gen_str(bc, INSTR_GET, s->cstr);
                    bc_gen_str(bc, INSTR_EXPORT_VALUE, d->cstr);
                }
                mstr_free(spec);
            }
        } else if (ok) {
            /* local export: publish existing bindings. */
            for (uint32_t i = 0; i < srcs->size; i++) {
                mstr_t* s = (mstr_t*)array_get(srcs, i);
                mstr_t* d = (mstr_t*)array_get(dsts, i);
                bc_gen_str(bc, INSTR_LOAD, s->cstr);
                bc_gen_str(bc, INSTR_EXPORT_VALUE, d->cstr);
            }
        }

        array_free(srcs, (free_func_t)mstr_free);
        array_free(dsts, (free_func_t)mstr_free);
        if (ok && is_stmt_end(l->tk)) lex_chkread_stmt_end(l);
        return ok;
    }

    if (!g_hoist_quiet) {
        mario_printf("export: unsupported form! ");
        compile_error_pos(l, -1);
    }
    return false;
}

bool stmt_return(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_RETURN)) {
        return false;
    }
    if (!is_stmt_end(l->tk)) {
        /* `return a, b` is a comma expression: evaluate a, discard, return b.
         * base() alone would stop at the ',' and strand the statement. */
        if (!expr_seq(l, bc)) {
            return false;
        }
        if (g_async_depth > 0) {
            /* async function: resolve the returned value into a Promise. */
            bc_gen_str(bc, INSTR_CALL, "__promise_resolve$1");
        }
        bc_gen(bc, INSTR_RETURNV);
    } else {
        if (g_async_depth > 0) {
            /* `return;` inside an async function -> resolve(undefined). */
            bc_gen(bc, INSTR_UNDEF);
            bc_gen_str(bc, INSTR_CALL, "__promise_resolve$1");
            bc_gen(bc, INSTR_RETURNV);
        } else {
            bc_gen(bc, INSTR_RETURN);
        }
    }
    return lex_chkread_stmt_end(l);
}

bool stmt_throw(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_THROW)) {
        return false;
    }
    /* `throw a, b` is a comma expression like `return a, b`: base() alone
     * would stop at the ',' and strand the statement (regenerator-transpiled
     * generators emit `throw y = !0, n;`). */
    if (!expr_seq(l, bc)) {
        return false;
    }
    if (!lex_chkread_stmt_end(l)) {
        return false;
    }
    bc_gen(bc, INSTR_THROW);
    return true;
}

bool stmt_try(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_TRY)) {
        return false;
    }
    PC pc = bc_gen(bc, INSTR_TRY);
    bc_add_instr(bc, pc, INSTR_JMP, pc + 2);
    /* Index of the INSTR_TRY word. Its (currently unused) operand is patched below
     * to the finally-block start PC so handle_block can record it on the scope and
     * a `return` leaving the body runs the finally (vm_finish_return). */
    PC pc_try = pc - 1;
    bool demoted = false;   // set when a try-finally with no catch becomes a plain block

    lex_skip_empty(l);
    PC pc_cache = bc_reserve(bc);
    if (!statement(l, bc)) {
        return false;
    }
    lex_skip_empty(l);
    PC pce = bc_reserve(bc); //jmp to finalize.

    if (l->tk == LEX_R_CATCH) {
        /* pc_cache is the throw anchor (sc->pc): on a throw the VM lands here
         * and jumps to the real catch handler emitted below. */
        bc_set_instr(bc, pc_cache, INSTR_JMP, ILLEGAL_PC);
        if (!lex_chkread(l, LEX_R_CATCH)) {
            return false;
        }

        lex_skip_empty(l);
        if (l->tk == '(') {
            if (!lex_chkread(l, '(')) {
                return false;
            }
            bc_gen_str(bc, INSTR_CATCH, l->tk_str->cstr);
            if (!lex_chkread(l, LEX_ID)) {
                return false;
            }
            if (!lex_chkread(l, ')')) {
                return false;
            }
        } else {
            /* ES2019 optional catch binding: `catch { ... }`. The thrown
             * value still has to be popped off the stack, so bind it to a
             * name no JS identifier can ever match. */
            bc_gen_str(bc, INSTR_CATCH, "");
        }
        lex_skip_empty(l);
        if (!statement(l, bc)) {
            return false;
        }
    } else {
        /* try-finally with no catch: the try provides no exception handling, so
         * demote INSTR_TRY to a plain BLOCK. The body runs normally and a throw
         * propagates straight to an enclosing handler (exactly like code with no
         * try at all) instead of routing through a synthetic rethrow — this
         * preserves the thrown value and never crashes when the try sits inside
         * a function that is called from another try. pc_cache stays a dead NIL
         * slot (the JMP right after the BLOCK skips it and no throw targets it).
         * The finally block below still runs on the normal (non-throwing) path. */
        bc->code_buf[pc - 1] = INS(INSTR_BLOCK, OFF(bc->code_buf[pc - 1]));
        demoted = true;
    }

    pc = bc_gen(bc, INSTR_TRY_END) - 1;
    bc_set_instr(bc, pce, INSTR_JMP, pc); // end anchor;
    /* The finally block (if any) is emitted sequentially right after TRY_END, so
     * this is where a deferred return/break/continue must jump to run it. */
    PC finally_start = pc + 1;

    /* Optional `finally { ... }`. `finally` is not a reserved word in this
     * lexer (it arrives as LEX_ID), so detect it by name and emit the block
     * sequentially after TRY_END. It runs on the normal path and on the
     * catch-completes-normally path, matching JS for the common cases. */
    lex_skip_empty(l);
    if (l->tk == LEX_ID && strcmp(l->tk_str->cstr, "finally") == 0) {
        if (!lex_chkread(l, LEX_ID)) {
            return false;
        }
        lex_skip_empty(l);
        if (!statement(l, bc)) {
            return false;
        }
        /* Terminate the finally block: on the normal/catch path pending_op is
         * FIN_NONE and it is a no-op fall-through; when a return was parked here it
         * resumes it. Then record the finally start PC on the try (or demoted
         * block) scope by patching the INSTR_TRY/INSTR_BLOCK operand. */
        bc_gen(bc, INSTR_FINALLY_END);
        bc_set_instr(bc, pc_try, demoted ? INSTR_BLOCK : INSTR_TRY, finally_start);
    }
    return true;
}

/* Is a '/' here a regex literal (not division)? Regex may start only where an
 * operand is expected: after an operator/keyword/punctuation, not after a value
 * token. prev is the preceding token type (0 = start, regex allowed). */
static bool skip_regex_allowed(uint32_t prev) {
    switch (prev) {
        case LEX_ID: case LEX_INT: case LEX_FLOAT: case LEX_STR:
        case LEX_BIGINT: case ')': case ']': case LEX_PLUSPLUS:
        case LEX_MINUSMINUS: case LEX_R_TRUE: case LEX_R_FALSE:
        case LEX_R_NULL: case LEX_R_UNDEFINED: return false;
        default: return true;
    }
}
/* Skip an entire template literal at the character level (no code emitted).
 * Assumes l->tk == '`': the opening backtick has already been consumed by the
 * lexer, so l->curr_ch points at the first content character. On return the
 * closing backtick is consumed and the next normal token has been read.
 * `${...}` substitutions (which may nest strings/regex/templates) are stepped
 * over by skip_template_subst, and backslash escapes by two chars, so the
 * content is never mistaken for the surrounding token stream. */
static bool skip_template_literal(lex_t* l) {
    while (true) {
        char c = l->curr_ch;
        if (c == 0) return false;            // unterminated template literal
        if (c == '`') {                       // closing backtick
            lex_get_nextch(l);
            break;
        }
        if (c == '$' && l->next_ch == '{') {  // embedded expression
            skip_template_subst(l);
            continue;
        }
        if (c == '\\') {                       // escape sequence
            lex_get_nextch(l);
            if (l->curr_ch) lex_get_nextch(l);
            continue;
        }
        lex_get_nextch(l);
    }
    lex_get_next_token(l);  // resume the normal token stream after the template
    return true;
}
/* Advance one token while skipping code, treating '/' as a regex literal when
 * the context allows it. Otherwise the generic lexer reads an embedded '//' as
 * a line comment and swallows the rest of a minified single-line file. */
static bool skip_advance(lex_t* l, uint32_t* prev) {
    /* A backtick starts a template literal: consume the WHOLE template at the
     * character level. The generic lexer only emits '`' as a single char token
     * and would then re-read the template CONTENT as ordinary tokens, so a
     * leading '/' (e.g. `/${x}`, extremely common in Next.js route helpers)
     * would be mis-scanned as a regex and swallow the rest of the minified
     * line - desyncing the two-pass switch compiler and failing the script. */
    if (l->tk == '`') {
        if (!skip_template_literal(l)) return false;
        *prev = LEX_STR;   // a template is a value: a following '/' is division
        return true;
    }
    if ((l->tk == '/' || l->tk == LEX_DIVEQUAL) && skip_regex_allowed(*prev)) {
        mstr_t* pat = mstr_new(""); mstr_t* flags = mstr_new("");
        bool ok = lex_scan_regex(l, pat, flags);
        mstr_free(pat); mstr_free(flags);
        if (!ok) return false;
        *prev = LEX_ID; return true;
    }
    *prev = l->tk; lex_get_next_token(l); return true;
}

/* Skip a switch clause body at the token level: advance until the next
 * `case`/`default` (at brace depth 0) or the switch's closing `}`. The boundary
 * token is NOT consumed. Used by the dispatch pass, which must step over bodies
 * without compiling them (they are compiled in the second pass). */
static bool skip_switch_body(lex_t* l) {
    int depth = 0;
    /* `case`/`default` are also legal property names after `.`/`?.` (e.g.
     * `f.default`, `obj.case`). Such a member access must NOT be mistaken for
     * the next clause boundary, or the skipper would return early and desync the
     * two-pass switch compiler. Track whether the previous token was a member
     * accessor so a following keyword is treated as a property name. */
    bool prev_dot = false;
    uint32_t prev = 0; // regex context; 0 => a leading '/' is a regex literal
    while (l->tk != LEX_EOF) {
        if (l->tk == '{') {
            depth++;
        } else if (l->tk == '}') {
            if (depth == 0) return true; // the switch's own closing brace
            depth--;
        } else if (depth == 0 && !prev_dot &&
                   (l->tk == LEX_R_CASE || l->tk == LEX_R_DEFAULT)) {
            return true;
        }
        prev_dot = (l->tk == '.' || l->tk == LEX_OPTCHAIN);
        if (!skip_advance(l, &prev)) return false;
    }
    return false;
}

/* Consume a `case <expr>:` or `default:` label at the token level (no code is
 * emitted). The case expression is skipped by scanning to the ':' that sits at
 * paren/bracket/brace depth 0 and ternary depth 0. */
static bool skip_case_label(lex_t* l) {
    if (l->tk == LEX_R_DEFAULT) {
        if (!lex_chkread(l, LEX_R_DEFAULT)) return false;
        lex_skip_empty(l);
        return lex_chkread(l, ':');
    }
    if (l->tk != LEX_R_CASE) return false;
    if (!lex_chkread(l, LEX_R_CASE)) return false;
    lex_skip_empty(l);
    int depth = 0, ternary = 0;
    uint32_t prev = 0; // regex context for the case expression
    while (l->tk != LEX_EOF) {
        if (l->tk == '(' || l->tk == '[' || l->tk == '{') depth++;
        else if (l->tk == ')' || l->tk == ']' || l->tk == '}') depth--;
        else if (l->tk == '?') ternary++;
        else if (l->tk == ':' && depth == 0) {
            if (ternary > 0) ternary--;
            else { lex_chkread(l, ':'); return true; }
        }
        if (!skip_advance(l, &prev)) return false;
    }
    return false;
}

/* Compile the statements of one switch clause body, stopping at the next
 * `case`/`default`/`}`. Fall-through to the following clause is implicit (the
 * bodies are emitted back-to-back). */
static bool compile_switch_body(lex_t* l, bytecode_t* bc) {
    while (l->tk != LEX_R_CASE && l->tk != LEX_R_DEFAULT &&
           l->tk != '}' && l->tk != LEX_EOF) {
        int32_t prev_pos = l->data_pos;
        uint32_t prev_tk = l->tk;
        if (!statement(l, bc)) return false;
        lex_skip_empty(l);
        if (l->data_pos == prev_pos && l->tk == prev_tk) return false; // no progress
    }
    return l->tk != LEX_EOF;
}

/* Generated code (e.g. a CSS named-colour table) routinely has a few hundred
 * cases in one switch; the anchor arrays are per-switch stack storage. */
#define SWITCH_MAX_CASES 1024

/* `switch (expr) { case E: ... default: ... }`.
 *
 * Emitted layout (two passes over the source, one shared bytecode buffer):
 *   <eval expr> -> __sw_val
 *   SWITCH                 ; push switch scope, sc->pc = break anchor (SWITCH+2)
 *   JMP  dispatch          ; normal flow skips the break anchor
 *   break_anchor: JMP end  ; `break` lands here (sc->pc), jumps to SWITCH_END
 * dispatch:
 *   for each case i:  LOAD __sw_val; <E_i>; TEQ; NJMP next_test_i; JMP body_i
 *   [JMP default_body]     ; only when a default clause exists
 * body_0: <stmts>          ; bodies in source order -> fall-through is implicit
 * body_1: <stmts>
 * ...
 * end: SWITCH_END          ; pop switch scope
 *
 * Pass 1 compiles the case expressions into the dispatch region and skips the
 * bodies (recording nothing but reserving jump anchors). Pass 2 restores the
 * lexer to the body-region start and compiles each body in source order, then
 * every reserved anchor is patched. Case expressions are compiled exactly once
 * (in pass 1), so their side effects run once, per spec. */
bool stmt_switch(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_SWITCH)) return false;
    if (!lex_chkread(l, '(')) return false;

    /* The discriminant temp must be unique per switch statement: an inner switch
     * compiled inside a case body would otherwise overwrite the outer temp and
     * the outer dispatch would then compare against the wrong value.
     *
     * It is declared with INSTR_VAR, not INSTR_SAFE_VAR: the temp is emitted
     * outside the switch scope, so a switch inside a loop re-runs the
     * declaration on every iteration and SAFE_VAR's redeclaration guard aborts
     * the whole script with "let '__sw_val_N' has already existed". `var`
     * semantics (idempotent, function-scoped) are what a compiler-internal temp
     * needs, and the ASIGN below refreshes it before every dispatch. */
    static uint32_t sw_seq = 0;
    char sw_tmp[32];
    snprintf(sw_tmp, sizeof(sw_tmp), "__sw_val_%u", sw_seq++);

    bc_gen_str(bc, INSTR_VAR, sw_tmp);
    bc_gen_str(bc, INSTR_LOAD, sw_tmp);
    if (!expr_seq(l, bc)) return false;   // discriminant
    if (!lex_chkread(l, ')')) return false;
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);
    lex_skip_empty(l);
    if (!lex_chkread(l, '{')) return false;
    lex_skip_empty(l);

    // Snapshot the body-region start; scan pass 1 with a private tk_str buffer so
    // the original token text survives for the pass-2 restore (see the shared
    // save/restore idiom in call_args_have_spread).
    lex_t saved = *l;
    mstr_t* saved_tk_str = l->tk_str;
    l->tk_str = mstr_new("");
    mstr_cpy(l->tk_str, saved_tk_str->cstr);

    PC pc_switch = bc_gen(bc, INSTR_SWITCH);
    bc_add_instr(bc, pc_switch, INSTR_JMP, pc_switch + 2); // normal flow -> dispatch
    PC pc_break = bc_reserve(bc);                          // == pc_switch+2 == sc->pc

    PC njmp_anchor[SWITCH_MAX_CASES];
    PC body_anchor[SWITCH_MAX_CASES];
    PC test_start[SWITCH_MAX_CASES];
    int n_cases = 0;
    bool has_default = false;
    bool ok = true;

    // ---- Pass 1: dispatch tests (bodies skipped) ----
    while (l->tk != '}') {
        if (l->tk == LEX_EOF) { ok = false; break; }
        if (l->tk == LEX_R_CASE) {
            if (n_cases >= SWITCH_MAX_CASES) { ok = false; break; }
            if (!lex_chkread(l, LEX_R_CASE)) { ok = false; break; }
            lex_skip_empty(l);
            test_start[n_cases] = bc->cindex;
            bc_gen_str(bc, INSTR_LOAD, sw_tmp);
            if (!base(l, bc)) { ok = false; break; }      // case expr, stops at ':'
            bc_gen(bc, INSTR_TEQ);
            njmp_anchor[n_cases] = bc_reserve(bc);        // NJMP -> next test
            body_anchor[n_cases] = bc_reserve(bc);        // JMP  -> body
            n_cases++;
            if (!lex_chkread(l, ':')) { ok = false; break; }
            lex_skip_empty(l);
            if (!skip_switch_body(l)) { ok = false; break; }
        } else if (l->tk == LEX_R_DEFAULT) {
            if (!lex_chkread(l, LEX_R_DEFAULT)) { ok = false; break; }
            lex_skip_empty(l);
            if (!lex_chkread(l, ':')) { ok = false; break; }
            lex_skip_empty(l);
            has_default = true;
            if (!skip_switch_body(l)) { ok = false; break; }
        } else {
            // Statements before the first label are dead code in JS; skip them.
            if (!skip_switch_body(l)) { ok = false; break; }
        }
    }
    if (ok && l->tk == '}') lex_chkread(l, '}');

    PC dispatch_end = bc->cindex;
    PC default_anchor = ILLEGAL_PC;
    PC no_match_anchor = ILLEGAL_PC;
    if (ok && has_default) default_anchor = bc_reserve(bc); // JMP -> default body
    /* Without a default clause nothing may sit at dispatch_end: pass 2 emits
     * body_0 right there, so a discriminant that matches no case would fall
     * straight into the first body. Reserve a slot that is patched to JMP
     * SWITCH_END once pc_end is known. */
    else if (ok) no_match_anchor = bc_reserve(bc);          // JMP -> SWITCH_END
    for (int i = 0; ok && i < n_cases; i++) {
        PC tgt;
        if (i + 1 < n_cases)          tgt = test_start[i + 1];
        else if (has_default)         tgt = dispatch_end;
        else                          tgt = no_match_anchor;
        bc_set_instr(bc, njmp_anchor[i], INSTR_NJMP, tgt);
    }

    // ---- Restore to the body-region start for pass 2 ----
    mstr_free(l->tk_str);
    *l = saved;
    if (!ok) return false;

    // ---- Pass 2: bodies in source order ----
    PC body_pos[SWITCH_MAX_CASES];
    int body_idx = 0;
    PC default_body_pos = ILLEGAL_PC;
    while (l->tk != '}') {
        if (l->tk == LEX_EOF) { ok = false; break; }
        if (l->tk == LEX_R_CASE) {
            if (!skip_case_label(l)) { ok = false; break; }
            lex_skip_empty(l);
            if (body_idx < SWITCH_MAX_CASES) body_pos[body_idx++] = bc->cindex;
            if (!compile_switch_body(l, bc)) { ok = false; break; }
        } else if (l->tk == LEX_R_DEFAULT) {
            if (!skip_case_label(l)) { ok = false; break; }
            lex_skip_empty(l);
            default_body_pos = bc->cindex;
            if (!compile_switch_body(l, bc)) { ok = false; break; }
        } else {
            if (!skip_switch_body(l)) { ok = false; break; }
        }
    }
    if (ok && l->tk == '}') lex_chkread(l, '}');
    if (!ok) return false;

    PC pc_end = bc_gen(bc, INSTR_SWITCH_END);
    /* bc_gen returns the post-emit cindex, so SWITCH_END itself sits at pc_end-1.
     * Every switch-exit jump (break, empty-case fallthrough, no-match, default
     * with no body) MUST land on SWITCH_END so handle_block_end pops the switch
     * scope before falling through to pc_end. Targeting pc_end (one past) skips
     * the pop and leaks the scope; a switch nested in a loop then leaves a stale
     * is_switch scope on the stack, so the next `break` re-targets this switch's
     * break anchor forever (JMP break-anchor -> SWITCH_END+1 == the enclosing
     * break -> back to the anchor): an infinite 2-instruction loop. This mirrors
     * the loop compiler, which anchors break at LOOP_END (pc-1), not past it. */
    PC pc_switch_end = pc_end - 1;
    bc_set_instr(bc, pc_break, INSTR_JMP, pc_switch_end);   // break -> SWITCH_END
    for (int i = 0; i < n_cases; i++) {
        if (i < body_idx) bc_set_instr(bc, body_anchor[i], INSTR_JMP, body_pos[i]);
        else              bc_set_instr(bc, body_anchor[i], INSTR_JMP, pc_switch_end);
    }
    if (has_default && default_anchor != ILLEGAL_PC) {
        PC dtgt = (default_body_pos != ILLEGAL_PC) ? default_body_pos : pc_switch_end;
        bc_set_instr(bc, default_anchor, INSTR_JMP, dtgt);
    }
    else if (no_match_anchor != ILLEGAL_PC) {
        bc_set_instr(bc, no_match_anchor, INSTR_JMP, pc_switch_end);
    }
    return true;
}

bool stmt_strict(lex_t* l, bytecode_t* bc) {
    if (l->tk != LEX_STR) {
        return false;
    }

    const char* s = l->tk_str->cstr;
    if (strcmp(s, "use strict") != 0) {
        return false;
    }

    if (!lex_chkread(l, LEX_STR)) {
        return false;
    }
    if (!lex_chkread_stmt_end(l)) {
        return false;
    }
    bc_gen(bc, INSTR_STRICT);
    return true;
}

/* True when the statement at the current position is a labeled statement, i.e.
 * an identifier immediately followed by ':'. At statement position nothing else
 * can produce `id :` (a ternary or object literal never starts a statement this
 * way), so this is unambiguous. Scans with a private tk_str buffer and fully
 * restores the lexer, mirroring paren_group_is_arrow. */
static bool statement_is_label(lex_t* l) {
    if (l->tk != LEX_ID) {
        return false;
    }
    lex_t saved = *l;
    mstr_t* saved_tk_str = l->tk_str;
    l->tk_str = mstr_new("");
    mstr_cpy(l->tk_str, saved_tk_str->cstr);
    lex_get_next_token(l); // move past the identifier
    bool is_label = (l->tk == ':');
    mstr_free(l->tk_str);
    *l = saved; // restore position and the original tk_str pointer
    return is_label;
}

/* ES labeled statement: `label: <statement>`. `break label` inside the statement
 * jumps to its end. Emitted layout (mirrors stmt_switch's break anchor):
 *   [P]   LABEL name        ; push a labeled scope, sc->pc = P+2 (break anchor)
 *   [P+1] JMP  body         ; normal flow skips the break anchor
 *   [P+2] break_anchor: JMP end ; a matching `break name` lands here
 *   body: <labeled statement>
 *   end:  LABEL_END         ; pop the labeled scope
 * handle_break unwinds inner scopes up to (but not including) the labeled scope,
 * then jumps to the break anchor; its JMP runs LABEL_END to pop that scope, so
 * the scope stack stays balanced on both the normal and the break path. */
static bool stmt_label(lex_t* l, bytecode_t* bc) {
    char name[64];
    strncpy(name, l->tk_str->cstr, sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    if (!lex_chkread(l, LEX_ID)) { // consume the label identifier
        return false;
    }
    if (!lex_chkread(l, ':')) {    // consume ':'
        return false;
    }
    lex_skip_empty(l);

    PC pc_label = bc_gen_str(bc, INSTR_LABEL, name);
    bc_add_instr(bc, pc_label, INSTR_JMP, pc_label + 2); // normal flow -> body
    PC pc_break = bc_reserve(bc);                        // == pc_label+2 == sc->pc
    if (!statement(l, bc)) {
        return false;
    }
    PC pc_end = bc_gen(bc, INSTR_LABEL_END);
    bc_set_instr(bc, pc_break, INSTR_JMP, pc_end - 1); // break anchor -> LABEL_END
    return true;
}

/* Legacy ES1 `with (obj) statement`: obj becomes the innermost scope for the
 * body, so bare names resolve against its members. `with` stays a plain
 * identifier in the lexer (TypedArray.prototype.with and friends use it as a
 * property name), detected here only in statement position, where a call to
 * a function named `with` would be illegal anyway. */
bool stmt_with(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_ID)) {
        return false;
    }
    lex_skip_empty(l);
    if (!lex_chkread(l, '(')) {
        return false;
    }
    if (!expr_seq(l, bc)) {
        return false;
    }
    if (!lex_chkread(l, ')')) {
        return false;
    }
    bc_gen(bc, INSTR_WITH);   /* pop the object, push it as the innermost scope */
    lex_skip_empty(l);
    if (!statement(l, bc)) {
        return false;
    }
    bc_gen(bc, INSTR_BLOCK_END);
    return true;
}

bool statement(lex_t* l, bytecode_t* bc) {
    bool pop = false;

    /* MARIO_SRCMAP: record where this statement's code starts. Only the main
     * source lexer counts - sub-lexers over captured text (default initialisers,
     * rewritten fragments) have offsets that mean nothing in the file. */
    if (bc->srcmap_on && l->data == g_srcmap_data && l->tk != '\n' && l->tk != ';') {
        bc_srcmap_add(bc, bc->cindex, (uint32_t)l->tk_start);
    }

    /* ES labeled statement: `label: <statement>` (checked first, since a label
     * starts with an identifier that would otherwise be an expression statement). */
    if (l->tk == LEX_ID && statement_is_label(l)) {
        return stmt_label(l, bc);
    }

    /* `with (obj) stmt` (see stmt_with): checked before the expression path,
     * which would otherwise compile it as a call to a function `with`. */
    if (l->tk == LEX_ID && strcmp(l->tk_str->cstr, "with") == 0) {
        return stmt_with(l, bc);
    }

    if (l->tk == '\n') {
        lex_skip_empty(l);
    } else if (l->tk == ';') { /* Empty statement */
        if (!lex_chkread(l, ';')) {
            return false;
        }
    } else if (l->tk == '{') { /* A block of code */
        if (!stmt_block(l, bc, false)) {
            return false;
        }
    } else if (l->tk == '[') {
        /* `[a, b] = rhs` is a destructuring *assignment* (targets already
         * declared); anything else starting with '[' is an array-literal
         * expression statement. Peek past the balanced pattern: a single '='
         * right after the closing ']' means destructuring. Restore the lexer
         * either way before compiling. */
        lex_t saved = *l;
        mstr_t* saved_str = mstr_new(l->tk_str->cstr);
        bool is_destr = false;
        if (skip_balanced_pattern(l)) {
            lex_skip_empty(l);
            if (l->tk == '=') {
                is_destr = true;
            }
        }
        *l = saved;
        mstr_cpy(l->tk_str, saved_str->cstr);
        mstr_free(saved_str);

        if (is_destr) {
            /* decl_op = 0: the leaves are existing bindings, not new ones. */
            if (!stmt_var_destructure(l, bc, 0)) {
                return false;
            }
            /* `[t,o]=f(o),e.push(t)`: the destructuring assignment may be the
             * first operand of a comma sequence (common as a one-line loop
             * body). Compile the remaining operands as an ordinary expression
             * sequence and discard its value. */
            if (l->tk == ',') {
                if (!lex_chkread(l, ',')) {
                    return false;
                }
                if (!expr_seq(l, bc)) {
                    return false;
                }
                pop = true;
            }
            if (is_stmt_end(l->tk)) {
                if (!lex_chkread_stmt_end(l)) {
                    return false;
                }
            }
        } else if (!stmt_strict(l, bc)) {
            if (!expr_seq(l, bc)) {
                return false;
            }
            if (is_stmt_end(l->tk)) {
                if (!lex_chkread_stmt_end(l)) {
                    return false;
                }
            }
            pop = true;
        }
    } else if (l->tk == LEX_STR || 
               l->tk == LEX_INT || l->tk == LEX_FLOAT ||
               l->tk == LEX_BIGINT ||
               l->tk == '`' ||
               l->tk == LEX_ID ||
               l->tk == LEX_PLUSPLUS ||
               l->tk == LEX_MINUSMINUS ||
               l->tk == '(' || l->tk == '!' || l->tk == LEX_R_NEW ||
               l->tk == '/' || l->tk == LEX_DIVEQUAL ||
               l->tk == LEX_R_AWAIT || l->tk == LEX_R_DELETE ||
               l->tk == LEX_R_NULL || l->tk == LEX_R_UNDEFINED ||
               l->tk == LEX_R_TRUE || l->tk == LEX_R_FALSE ||
               l->tk == LEX_R_TYPEOF || l->tk == LEX_R_VOID ||
               l->tk == '+' ||
               l->tk == '-' ||
               l->tk == '~') {
        if (!stmt_strict(l, bc)) {
            /* Execute a simple statement that only contains basic arithmetic... */
            if (!expr_seq(l, bc)) {
                return false;
            }
            if (is_stmt_end(l->tk)) {
                if (!lex_chkread_stmt_end(l)) {
                    return false;
                }
            }
        	pop = true;
        }
    } else if (l->tk == LEX_R_VAR || l->tk == LEX_R_CONST || l->tk == LEX_R_SAFE_VAR) {
        if (!stmt_var(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_CLASS) {
        factor_def_class(l, bc);
       	pop = true;
    } else if (l->tk == LEX_R_FUNCTION) {
        /* A declaration, never an expression (a statement cannot start with a
         * function expression). During hoisting this may be redirected into the
         * real bytecode (scan pass) or the scratch buffer (main pass). */
        if (!stmt_function(l, g_funcdecl_bc != NULL ? g_funcdecl_bc : bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_ASYNC) {
        /* `async function name() {...}` declaration, or an async-arrow
         * expression statement. */
        if (!lex_chkread(l, LEX_R_ASYNC)) {
            return false;
        }
        lex_skip_empty(l);
        if (l->tk == LEX_R_FUNCTION) {
            if (!lex_chkread(l, LEX_R_FUNCTION)) {
                return false;
            }
            mstr_t* fname = mstr_new("");
            g_async_pending = 1;
            bytecode_t* dbc = (g_funcdecl_bc != NULL) ? g_funcdecl_bc : bc;
            factor_def_func(l, dbc, fname);
            bc_gen_str(dbc, INSTR_MEMBERN, fname->cstr);
            mstr_free(fname);
        } else {
            g_async_pending = 1;
            if (!base(l, bc)) {
                g_async_pending = 0;
                return false;
            }
            if (is_stmt_end(l->tk)) {
                lex_chkread_stmt_end(l);
            }
            pop = true;
        }
    } else if (l->tk == LEX_R_INCLUDE) {
        if (!stmt_include(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_IMPORT) {
        if (import_is_expression(l)) {
            /* `import(spec).then(..)` / `import.meta.x` as an expression
             * statement: factor() handles the import form. */
            if (!expr_seq(l, bc)) {
                return false;
            }
            if (is_stmt_end(l->tk)) {
                if (!lex_chkread_stmt_end(l)) {
                    return false;
                }
            }
            pop = true;
        } else if (!stmt_import(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_EXPORT) {
        if (!stmt_export(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_RETURN) {
        if (!stmt_return(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_IF) {
        if (!stmt_if(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_WHILE) {
        if (!stmt_while(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_DO) {
        if (!stmt_do(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_FOR) {
        if (!stmt_for(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_BREAK) {
        if (!stmt_break(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_CONTINUE) {
        if (!stmt_continue(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_THROW) {
        if (!stmt_throw(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_TRY) {
        if (!stmt_try(l, bc)) {
            return false;
        }
    } else if (l->tk == LEX_R_SWITCH) {
        if (!stmt_switch(l, bc)) {
            return false;
        }
    }

    if (pop) {
        bc_gen(bc, INSTR_POP);
    }

    return true;
}

bool js_compile(bytecode_t *bc, const char* input) {
    lex_t lex;
    lex_init(&lex, input);
    lex_get_next_token(&lex);
    const char* saved_srcmap_data = g_srcmap_data;
    g_srcmap_data = input;
    bc_srcmap_begin(bc, input);

    /* Hoist top-level function declarations (ES5 semantics). */
    bytecode_t scratch;
    bytecode_t* saved_redirect;
    bytecode_t* saved_vardecl;
    hoist_begin(&lex, bc, &scratch, false, false, &saved_redirect, &saved_vardecl);

    bool ret = true;
    while (lex.tk != LEX_EOF && ret) {
        int32_t prev_pos = lex.data_pos;
        uint32_t prev_tk = lex.tk;
        ret = statement(&lex, bc);
        lex_skip_empty(&lex);
        /* Safety net: if a statement consumed no input at all (an unhandled
         * leading token), bail out with a compile error rather than spinning
         * forever in this loop. */
        if (ret && lex.tk != LEX_EOF &&
            lex.data_pos == prev_pos && lex.tk == prev_tk) {
            if (!g_hoist_quiet)
                mario_printf("compile error: unexpected token, made no progress! ");
            ret = false;
        }
    }
    hoist_end(&scratch, saved_redirect, saved_vardecl);
    g_srcmap_data = saved_srcmap_data;
    
    if (ret) {
        bc_gen(bc, INSTR_END);
    }
	else {
        compile_error_pos(&lex, -1);
	}
    
    lex_release(&lex);
    return ret;
}
