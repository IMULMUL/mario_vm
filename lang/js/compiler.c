/**
 * very tiny js script compiler.
 */

#include "lex/mario_lex.h"
#include <stdlib.h>
#include <stdio.h>

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
    LEX_R_INCLUDE,
    LEX_R_THROW,
    LEX_R_TRY,
    LEX_R_CATCH,
    LEX_R_INSTANCEOF,
    LEX_R_ASYNC,
    LEX_R_AWAIT,
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
        } else if (lex->curr_ch == '>') { // >>>
            lex->tk = LEX_RSHIFTUNSIGNED;
            lex_get_nextch(lex);
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
            switch (lex->curr_ch) {
                case 'n':
                    mstr_add(lex->tk_str, '\n');
                    break;
                case 'a':
                    mstr_add(lex->tk_str, '\a');
                    break;
                case 'r':
                    mstr_add(lex->tk_str, '\r');
                    break;
                case 't':
                    mstr_add(lex->tk_str, '\t');
                    break;
                case '\'':
                    mstr_add(lex->tk_str, '\'');
                    break;
                case '\\':
                    mstr_add(lex->tk_str, '\\');
                    break;
                case 'x': {
                    // hex digits
                    char buf[3] = "??";
                    lex_get_nextch(lex);
                    buf[0] = lex->curr_ch;
                    lex_get_nextch(lex);
                    buf[1] = lex->curr_ch;
                    mstr_add(lex->tk_str, (char)strtol(buf, 0, 16));
                }
                break;
                default:
                    if (lex->curr_ch >= '0' && lex->curr_ch <= '7') {
                        // octal digits
                        char buf[4] = "???";
                        buf[0] = lex->curr_ch;
                        lex_get_nextch(lex);
                        buf[1] = lex->curr_ch;
                        lex_get_nextch(lex);
                        buf[2] = lex->curr_ch;
                        mstr_add(lex->tk_str, (char)strtol(buf, 0, 8));
                    } else {
                        mstr_add(lex->tk_str, lex->curr_ch);
                    }
            }
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
    } else if (strcmp(lex->tk_str->cstr, "throw") == 0) {
        lex->tk = LEX_R_THROW;
    } else if (strcmp(lex->tk_str->cstr, "try") == 0) {
        lex->tk = LEX_R_TRY;
    } else if (strcmp(lex->tk_str->cstr, "catch") == 0) {
        lex->tk = LEX_R_CATCH;
    } else if (strcmp(lex->tk_str->cstr, "instanceof") == 0) {
        lex->tk = LEX_R_INSTANCEOF;
    } else if (strcmp(lex->tk_str->cstr, "async") == 0) {
        lex->tk = LEX_R_ASYNC;
    } else if (strcmp(lex->tk_str->cstr, "await") == 0) {
        lex->tk = LEX_R_AWAIT;
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
    }
    return "?[UNKNOW]";
}

void compile_error_pos(lex_t* l, int pos) {
    int line = 1;
    int col;

    lex_get_pos(l, &line, &col, pos);
    mario_printf("compile error at (line: %d, col: %d)\n", line, col);
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

        mario_printf("lex got '%s' expected '%s'! ", stk, sexp);
        compile_error_pos(lex, -1);
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
                break;
            }
        } else if (depth == 1 && l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
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
            if (!statement(l, bc)) {
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

    while (l->tk && l->tk != '}') {
        if (!statement(l, bc)) {
            return false;
        }
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

/** Non-zero while compiling the body of an `async` function. Used by
 *  stmt_return / func_params_and_body to wrap the returned value in a
 *  resolved Promise (Promise.resolve). Entering any function body resets it
 *  to that function's own async-ness, so nested functions are unaffected. */
static int g_async_depth = 0;

/* Set to 1 immediately before defining an `async` function/arrow. It is
 * consumed by factor_def_func / factor_def_afunc to establish g_async_depth
 * for that body and then cleared, so nested definitions default to sync. */
static int g_async_pending = 0;

/** Parse a function's parameter list (starting at '(') and body, emitting the
 *  argument-name instructions expected by func_def plus the body bytecode.
 *  Supports ES6 default parameters and a trailing rest parameter. The caller
 *  must already have emitted INSTR_FUNC (or a variant). */
bool func_params_and_body(lex_t* l, bytecode_t* bc) {
    lex_skip_empty(l);
    //do arguments
    if (!lex_chkread(l, '(')) {
        return false;
    }
    lex_skip_empty(l);

    m_array_t defaults;      // param_default_t* entries
    array_init(&defaults);
    m_array_t pdestrs;       // param_destr_t* entries (destructuring params)
    array_init(&pdestrs);
    mstr_t* rest_name = NULL; // ES6 rest parameter (...rest)
    int positional = 0;       // number of positional (non-rest) parameters

    while (l->tk != ')') {
        // ES6 rest parameter: ...name (must be the last parameter)
        if (l->tk == '.' && l->curr_ch == '.' && l->next_ch == '.') {
            lex_get_nextch(l); // -> 3rd '.'
            lex_get_nextch(l); // -> first char of the rest name
            lex_get_next_token(l);
            if (l->tk != LEX_ID) {
                break;
            }
            rest_name = mstr_new(l->tk_str->cstr);
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
            positional++;
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
            array_add(&pdestrs, pd);
            if (l->tk != ')') {
                if (!lex_chkread(l, ',')) {
                    break;
                }
                lex_skip_empty(l);
            }
            continue;
        }

        if (l->tk != LEX_ID) {
            break;
        }
        mstr_t* pname = mstr_new(l->tk_str->cstr);
        bc_gen_str(bc, INSTR_LOAD, pname->cstr); // argument name for func_def
        positional++;
        if (!lex_chkread(l, LEX_ID)) {
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
            array_add(&defaults, pd);
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
        array_clean(&defaults, free_param_default);
        array_clean(&pdestrs, free_param_destr);
        if (rest_name != NULL) mstr_free(rest_name);
        return false;
    }
    lex_skip_empty(l);
    PC pc = bc_reserve(bc);

    // Emit ES6 default-parameter initialisers at the start of the body:
    //   if (typeof name === "undefined") name = <expr>;
    uint32_t di;
    for (di = 0; di < defaults.size; di++) {
        param_default_t* pd = (param_default_t*)array_get(&defaults, di);
        bc_gen_str(bc, INSTR_LOAD, pd->name->cstr);
        bc_gen(bc, INSTR_TYPEOF);
        bc_gen_str(bc, INSTR_STR, "undefined");
        bc_gen(bc, INSTR_TEQ);
        PC pj = bc_reserve(bc);
        bc_gen_str(bc, INSTR_LOAD, pd->name->cstr); // assignment target
        if (!compile_captured_expr(pd->expr->cstr, bc)) {
            array_clean(&defaults, free_param_default);
            array_clean(&pdestrs, free_param_destr);
            if (rest_name != NULL) mstr_free(rest_name);
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
    for (pi = 0; pi < pdestrs.size; pi++) {
        param_destr_t* pd = (param_destr_t*)array_get(&pdestrs, pi);
        if (pd->def != NULL) {
            bc_gen_str(bc, INSTR_LOAD, pd->temp);
            bc_gen(bc, INSTR_TYPEOF);
            bc_gen_str(bc, INSTR_STR, "undefined");
            bc_gen(bc, INSTR_TEQ);
            PC pj = bc_reserve(bc);
            bc_gen_str(bc, INSTR_LOAD, pd->temp); // assignment target
            if (!compile_captured_expr(pd->def->cstr, bc)) {
                array_clean(&defaults, free_param_default);
                array_clean(&pdestrs, free_param_destr);
                if (rest_name != NULL) mstr_free(rest_name);
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
            array_clean(&defaults, free_param_default);
            array_clean(&pdestrs, free_param_destr);
            if (rest_name != NULL) mstr_free(rest_name);
            return false;
        }
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
    bool is_static = false;
    lex_skip_empty(l);

    if (l->tk == LEX_R_STATIC) {
        if (!lex_chkread(l, LEX_R_STATIC)) {
            return false;
        }
        is_static = true;
    }

    /* ES6 generator: `function*` or a `*method()` shorthand. The star precedes
     * the (optional) name, so detect it here before reading the name. */
    bool is_gen = false;
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
    }
    if (l->tk == LEX_ID) { //class get/set token
        if (strcmp(name->cstr, "get") == 0) {
            mstr_cpy(name, l->tk_str->cstr);
            if (!lex_chkread(l, LEX_ID)) {
                return false;
            }
            bc_gen(bc, INSTR_FUNC_GET);
        }
        if (strcmp(name->cstr, "set") == 0) {
            mstr_cpy(name, l->tk_str->cstr);
            if (!lex_chkread(l, LEX_ID)) {
                return false;
            }
            bc_gen(bc, INSTR_FUNC_SET);
        }
    } else {
        bc_gen(bc, is_gen ? INSTR_FUNC_GEN : (is_static ? INSTR_FUNC_STC : INSTR_FUNC));
    }
    bool ok = func_params_and_body(l, bc);
    g_async_depth = saved_async;
    return ok;
}

bool factor_def_afunc(lex_t* l, bytecode_t* bc) {
    int saved_async = g_async_depth;
    g_async_depth = g_async_pending;
    g_async_pending = 0;
    lex_skip_empty(l);
    PC pc = bc_reserve(bc);

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

static bool lex_chkread_stmt_end(lex_t* l) {
    if (l->tk == 0) {
        return true;
    }

    if (l->tk == ';') {
        return lex_chkread(l, ';');
    } else if (l->tk == '\n') {
        return lex_chkread(l, '\n');
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
        mstr_cpy(name, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_ID)) {
            mstr_free(name);
            return false;
        }
        bc_gen_str(bc, INSTR_EXTENDS, name->cstr);
    }

    lex_skip_empty(l);
    if (!lex_chkread(l, '{')) {
        mstr_free(name);
        return false;
    }
    lex_skip_empty(l);
    while (l->tk != '}') {
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
            /* ES async class method: `async foo() {...}`. */
            if (l->tk == LEX_R_ASYNC) {
                if (!lex_chkread(l, LEX_R_ASYNC)) {
                    mstr_free(name);
                    return false;
                }
                lex_skip_empty(l);
                g_async_pending = 1;
            }
            if (!factor_def_func(l, bc, name)) {
                g_async_pending = 0;
                mstr_free(name);
                return false;
            }
            lex_skip_empty(l);
            bc_gen_str(bc, INSTR_MEMBERN, name->cstr);
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
    mstr_t* class_name = mstr_new("");
    mstr_cpy(class_name, l->tk_str->cstr);

    if (!lex_chkread(l, LEX_ID)) {
        mstr_free(class_name);
        return false;
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

        // ES2017 async method: {async name(params){body}}. `async` is a
        // reserved word, so this token can only begin an async method here.
        bool is_async_method = false;
        if (l->tk == LEX_R_ASYNC) {
            lex_chkread(l, LEX_R_ASYNC);
            lex_skip_empty(l);
            is_async_method = true;
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
        } else {
            mstr_free(id);
            return false;
        }
        lex_skip_empty(l);

        /* ES6 accessor in an object literal: `get name() {...}` /
         * `set name(v) {...}`. Told apart from a method or property literally
         * named get/set by requiring a property-name token (ID or string) to
         * follow the keyword. */
        if ((strcmp(id->cstr, "get") == 0 || strcmp(id->cstr, "set") == 0) &&
            (l->tk == LEX_ID || l->tk == LEX_STR)) {
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
    bc_gen(bc, INSTR_ARRAY_AT);
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

/* Skip a `${...}` substitution at the character level (no compilation),
 * tracking brace nesting and string/template literals. Assumes l->curr_ch is
 * the '$'. On return l->curr_ch is the char just past the matching '}'. */
static void skip_template_subst(lex_t* l) {
    lex_get_nextch(l); // consume '$' -> curr_ch == '{'
    lex_get_nextch(l); // consume '{' -> curr_ch == first char inside
    int depth = 1;
    while (l->curr_ch && depth > 0) {
        char c = l->curr_ch;
        if (c == '"' || c == '\'' || c == '`') {
            char q = c;
            lex_get_nextch(l);
            while (l->curr_ch && l->curr_ch != q) {
                if (l->curr_ch == '\\') {
                    lex_get_nextch(l);
                    if (l->curr_ch) {
                        lex_get_nextch(l);
                    }
                    continue;
                }
                if (q == '`' && l->curr_ch == '$' && l->next_ch == '{') {
                    skip_template_subst(l);
                    continue;
                }
                lex_get_nextch(l);
            }
            if (l->curr_ch == q) {
                lex_get_nextch(l);
            }
            continue;
        }
        if (c == '{') {
            depth++;
        } else if (c == '}') {
            depth--;
            if (depth <= 0) {
                lex_get_nextch(l); // consume the closing '}'
                return;
            }
        }
        lex_get_nextch(l);
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
        if (!lex_chkread(l, '(')) {
            return false;
        }

		lex_skip_empty(l);
		if(l->tk != ')') {
			if (!base(l, bc)) {
				return false;
			}
			lex_skip_empty(l);

			while (l->tk == ',') {
				if (!lex_chkread(l, ',')) {
					return false;
				}
				if (!base(l, bc)) {
					return false;
				}
				lex_skip_empty(l);
			}
		}
		if (!lex_chkread(l, ')')) {
			return false;
		}

        if (l->tk == LEX_R_AFUNCTION) {
            if (!lex_chkread(l, LEX_R_AFUNCTION)) {
                return false;
            }
            bc_set_instr(bc, pc, INSTR_FUNC, 0);
            factor_def_afunc(l, bc);
        } else {
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
    } else if (l->tk == LEX_STR) {
        bc_gen_str(bc, INSTR_STR, l->tk_str->cstr);
        if (!lex_chkread(l, LEX_STR)) {
            return false;
        }
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
            factor_def_func(l, bc, fname);
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
        factor_def_func(l, bc, fname);
        mstr_free(fname);
    } else if (l->tk == LEX_R_CLASS) { //define class
        factor_def_class(l, bc);
    } else if (l->tk == LEX_R_NEW) { //new object
        factor_new(l, bc);
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
                bc_gen(bc, INSTR_FUNC);
                bc_gen_str(bc, INSTR_LOAD, name->cstr);
                factor_def_afunc(l, bc);
            } else {
                bc_gen_str(bc, INSTR_LOAD, name->cstr);
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
            bc_gen(bc, INSTR_ARRAY_AT);
        } else if (l->tk == '(') {
            /* ES6 call on a value already on the stack: an IIFE
             * `(function(){...})()`, `(expr)(args)`, or a curried `f()()`.
             * call_func pushes the args above the callable value; CALLX picks
             * the value back off and invokes it with runtime/known arity. */
            bool has_spread = false;
            int arg_num = call_func(l, bc, &has_spread);
            if (arg_num < 0) {
                return false;
            }
            if (has_spread) {
                bc_gen_str(bc, INSTR_CALLX_SPREAD, "");
            } else {
                mstr_t* s = mstr_new("");
                gen_func_name("", arg_num, s);
                bc_gen_str(bc, INSTR_CALLX, s->cstr);
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
    opr_code_t instr = INSTR_END;
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
    }

    if (!factor(l, bc, false)) {
        return false;
    }

    if (instr != INSTR_END) {
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
        bc_gen(bc, INSTR_PPLUS_PRE);
    } else if (pre == LEX_MINUSMINUS) {
        bc_gen(bc, INSTR_MMINUS_PRE);
    }

    while (l->tk == '+' || l->tk == '-' ||
           l->tk == LEX_PLUSPLUS || l->tk == LEX_MINUSMINUS) {
        int op = l->tk;
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (op == LEX_PLUSPLUS) {
            bc_gen(bc, INSTR_PPLUS);
        } else if (op == LEX_MINUSMINUS) {
            bc_gen(bc, INSTR_MMINUS);
        } else {
            if (!term(l, bc)) {
                return false;
            }
            if (op == '+') {
                bc_gen(bc, INSTR_PLUS);
            } else if (op == '-') {
                bc_gen(bc, INSTR_MINUS);
            }
        }
    }

    return true;
}

bool shift(lex_t* l, bytecode_t* bc) {
    if (!expr(l, bc)) {
        return false;
    }

    if (l->tk == LEX_LSHIFT || l->tk == LEX_RSHIFT || l->tk == LEX_RSHIFTUNSIGNED) {
        int op = l->tk;
        if (!lex_chkread(l, op)) {
            return false;
        }
        if (!base(l, bc)) {
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
           l->tk == LEX_R_INSTANCEOF ||
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
        } else if (op == '>') {
            bc_gen(bc, INSTR_GRT);
        } else if (op == '<') {
            bc_gen(bc, INSTR_LES);
        }
    }

    return true;
}

bool logic(lex_t* l, bytecode_t* bc) {
    if (!condition(l, bc)) {
        return false;
    }

    while (l->tk == '&' || l->tk == '|' || l->tk == '^' || l->tk == LEX_ANDAND || l->tk == LEX_OROR) {
        int op = l->tk;
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        if (!condition(l, bc)) {
            return false;
        }

        if (op == LEX_ANDAND) {
            bc_gen(bc, INSTR_AAND);
        } else if (op == LEX_OROR) {
            bc_gen(bc, INSTR_OOR);
        } else if (op == '|') {
            bc_gen(bc, INSTR_OR);
        } else if (op == '&') {
            bc_gen(bc, INSTR_AND);
        } else if (op == '^') {
            bc_gen(bc, INSTR_XOR);
        }
    }
    return true;
}


bool ternary(lex_t* l, bytecode_t* bc) {
    if (!logic(l, bc)) {
        return false;
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
	        l->tk == LEX_MINUSEQUAL) {
        LEX_TYPES op = (LEX_TYPES)l->tk;
        if (!lex_chkread(l, l->tk)) {
            return false;
        }
        /* For a plain assignment whose target ended with a member fetch (`.`),
         * retarget that fetch to the write-variant so a runtime setter is
         * invoked. This must run before the RHS is compiled, since the RHS
         * appends instructions after the target's final INSTR_GET. */
        if (op == '=' && bc->cindex > 0) {
            PC last = bc->code_buf[bc->cindex - 1];
            if (OP(last) == INSTR_GET) {
                bc->code_buf[bc->cindex - 1] = INS(INSTR_GETW, OFF(last));
            }
        }
        if (!base(l, bc)) {
            return false;
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
        }
		else {
			return false;
		}
    }
    return true;
}

static bool is_stmt_end(int tk) {
    return (tk == ';' || tk == '\n' || tk == 0);
    //return (tk == ';');
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
                    bc_gen_str(bc, INSTR_STR, keys[k]);
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
            } else {
                return false;
            }
            if (nkeys < DESTR_MAX) {
                strncpy(keys[nkeys], keyname, sizeof(keys[nkeys]) - 1);
                keys[nkeys][sizeof(keys[nkeys]) - 1] = 0;
                nkeys++;
            }
        }

        lex_skip_empty(l);
        bool nested = false;
        char target[64];
        target[0] = 0;

        if (is_array) {
            /* the element itself is the target */
            if (l->tk == '{' || l->tk == '[') {
                nested = true;
            } else if (l->tk == LEX_ID) {
                strncpy(target, l->tk_str->cstr, sizeof(target) - 1);
                target[sizeof(target) - 1] = 0;
                if (!lex_chkread(l, LEX_ID)) {
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
                if (!lex_chkread(l, LEX_ID)) {
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
            } else {
                bc_gen_str(bc, INSTR_GET, keyname);
            }
            bc_gen(bc, INSTR_ASIGN);
            bc_gen(bc, INSTR_POP);
            if (!destructure_pattern(l, bc, decl_op, tmpn)) {
                return false;
            }
        } else {
            if (decl_op) {
                bc_gen_str(bc, decl_op, target);
            }
            bc_gen_str(bc, INSTR_LOAD, target);
            bc_gen_str(bc, INSTR_LOAD, src);
            if (is_array) {
                bc_gen_int(bc, INSTR_INT, my_idx);
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

static bool stmt_var_destructure(lex_t* l, bytecode_t* bc, opr_code_t op) {
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

    mstr_free(tmpm);
    return ok;
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
            if (!lex_chkread(l, ',')) {
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
    if (!base(l, bc)) {
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
    if (!base(l, bc)) {
        return false;
    } //condition
    if (!lex_chkread(l, ')')) {
        return false;
    }

    bc_add_instr(bc, pc_break, INSTR_NJMPB, ILLEGAL_PC); //not jump back to break anchor;

    if (!stmt_loop_block(l, bc)) {
        return false;
    }

    bc_add_instr(bc, pc_condition, INSTR_JMPB, ILLEGAL_PC); //coninue anchor;
    pc = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_break, INSTR_JMP, pc - 1); // end anchor;
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
    // Load the object to iterate over
    if (!base(l, bc)) {
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
    bc_gen_str(bc, INSTR_CALLO, "keys");
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
    }
    
    // Initialize index to 0
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_in_idx");
    bc_gen_str(bc, INSTR_LOAD, "__for_in_idx");
    bc_gen_int(bc, INSTR_INT, 0);
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);
    
    // Condition: check if the current member is not empty or undefined
    bc_set_instr(bc, pc_condition, INSTR_JMP, bc->cindex);
    
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
    
    // Loop body
    if (!stmt_loop_block(l, bc)) {
        return false;
    }
    
    // Increment index
    bc_gen_str(bc, INSTR_LOAD, "__for_in_idx");
    bc_gen(bc, INSTR_PPLUS);
    bc_gen(bc, INSTR_POP);
    
    bc_add_instr(bc, pc_condition, INSTR_JMPB, ILLEGAL_PC); //jump to continue anchor;
    
    PC pc = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_break, INSTR_JMP, pc - 1); // end anchor;
    
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
        opr_code_t destr_op) {

    // store the iterable into a temporary variable
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_of_obj");
    bc_gen_str(bc, INSTR_LOAD, "__for_of_obj");
    if (!base(l, bc)) {
        return false;
    }
    if (!lex_chkread(l, ')')) {
        return false;
    }
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);

    // __for_of_size = __for_of_obj.length()
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_of_size");
    bc_gen_str(bc, INSTR_LOAD, "__for_of_size");
    bc_gen_str(bc, INSTR_LOAD, "__for_of_obj");
    bc_gen_str(bc, INSTR_CALLO, "length");
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);

    // declare the loop variable
    if (loop_var) {
        bc_gen_str(bc, var_op, loop_var->cstr);
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

    // __for_of_idx = 0
    bc_gen_str(bc, INSTR_SAFE_VAR, "__for_of_idx");
    bc_gen_str(bc, INSTR_LOAD, "__for_of_idx");
    bc_gen_int(bc, INSTR_INT, 0);
    bc_gen(bc, INSTR_ASIGN);
    bc_gen(bc, INSTR_POP);

    // condition anchor: idx < size
    bc_set_instr(bc, pc_condition, INSTR_JMP, bc->cindex);
    bc_gen_str(bc, INSTR_LOAD, "__for_of_idx");
    bc_gen_str(bc, INSTR_LOAD, "__for_of_size");
    bc_gen(bc, INSTR_LES);
    bc_add_instr(bc, pc_break, INSTR_NJMPB, ILLEGAL_PC);

    // loop_var = __for_of_obj[idx]
    if (loop_var) {
        bc_gen_str(bc, INSTR_LOAD, loop_var->cstr);
        bc_gen_str(bc, INSTR_LOAD, "__for_of_obj");
        bc_gen_str(bc, INSTR_LOAD, "__for_of_idx");
        bc_gen(bc, INSTR_ARRAY_AT);
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

    // idx++
    bc_gen_str(bc, INSTR_LOAD, "__for_of_idx");
    bc_gen(bc, INSTR_PPLUS);
    bc_gen(bc, INSTR_POP);

    bc_add_instr(bc, pc_condition, INSTR_JMPB, ILLEGAL_PC); //continue anchor;

    PC pc = bc_gen(bc, INSTR_LOOP_END);
    bc_set_instr(bc, pc_break, INSTR_JMP, pc - 1); // end anchor;

    if (loop_var) {
        mstr_free(loop_var);
    }
    return true;
}

bool stmt_for(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_FOR)) {
        return false;
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
        
        // Check if the next token is "in" (for-in loop)
        if (l->tk == LEX_ID && strcmp(l->tk_str->cstr, "in") == 0) {
            is_for_in = true;
            lex_chkread(l, LEX_ID); // consume "in"
            lex_skip_empty(l);
        } else if (l->tk == LEX_ID && strcmp(l->tk_str->cstr, "of") == 0) {
            is_for_of = true;
            lex_chkread(l, LEX_ID); // consume "of"
            lex_skip_empty(l);
        } else {
            // Standard for loop variable initialization
            // Generate variable declaration bytecode
            if (loop_var) {
                bc_gen_str(bc, var_op, loop_var->cstr);
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
            if (l->tk != ';') {
                mstr_free(loop_var);
                return false;
            }
            lex_chkread(l, ';');
            lex_skip_empty(l);
        }
    } else {
        // Standard for loop init statement
        if (!statement(l, bc)) {
            return false;
        }
        lex_skip_empty(l);
        lex_chkread(l, ';');
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
                loop_destr ? &pd_saved : NULL, destr_op);
    }

    // Standard for loop implementation
    bc_set_instr(bc, pc_condition, INSTR_JMP, bc->cindex);
    if (!base(l, bc)) { //condition
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
    if (!base(l, bc)) { //iterator statement
        if (loop_var) {
            mstr_free(loop_var);
        }
        return false;
    }
    if (!lex_chkread(l, ')')) {
        if (loop_var) {
            mstr_free(loop_var);
        }
        return false;
    }
    bc_gen(bc, INSTR_POP); //pop the stack.

    bc_add_instr(bc, pc_condition, INSTR_JMPB, ILLEGAL_PC); //jump to coninue anchor;

    bc_set_instr(bc, pcl, INSTR_JMP, ILLEGAL_PC); // loop anchor;

    // Loop body
    if (!stmt_loop_block(l, bc)) {
        if (loop_var) {
            mstr_free(loop_var);
        }
        return false;
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
    if (!lex_chkread_stmt_end(l)) {
        return false;
    }
    bc_gen(bc, INSTR_BREAK);
    return true;
}

bool stmt_continue(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_CONTINUE)) {
        return false;
    }
    if (!lex_chkread_stmt_end(l)) {
        return false;
    }
    bc_gen(bc, INSTR_CONTINUE);
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

bool stmt_return(lex_t* l, bytecode_t* bc) {
    if (!lex_chkread(l, LEX_R_RETURN)) {
        return false;
    }
    if (!is_stmt_end(l->tk)) {
        if (!base(l, bc)) {
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
    if (!base(l, bc)) {
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

    lex_skip_empty(l);
    PC pc_cache = bc_reserve(bc);
    if (!statement(l, bc)) {
        return false;
    }
    lex_skip_empty(l);
    PC pce = bc_reserve(bc); //jmp to finalize.

    bc_set_instr(bc, pc_cache, INSTR_JMP, ILLEGAL_PC);
    if (!lex_chkread(l, LEX_R_CATCH)) {
        return false;
    }

    lex_skip_empty(l);
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
    lex_skip_empty(l);
    if (!statement(l, bc)) {
        return false;
    }

    pc = bc_gen(bc, INSTR_TRY_END) - 1;
    bc_set_instr(bc, pce, INSTR_JMP, pc); // end anchor;
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

bool statement(lex_t* l, bytecode_t* bc) {
    bool pop = false;

    if (l->tk == '\n') {
        lex_skip_empty(l);
    } else if (l->tk == '{') { /* A block of code */
        if (!stmt_block(l, bc, false)) {
            return false;
        }
    } else if (l->tk == LEX_STR || 
               l->tk == LEX_INT || l->tk == LEX_FLOAT ||
               l->tk == '[' || l->tk == '`' ||
               l->tk == LEX_ID ||
               l->tk == LEX_PLUSPLUS ||
               l->tk == LEX_MINUSMINUS ||
               l->tk == '(' || l->tk == '!' || l->tk == LEX_R_NEW ||
               l->tk == '-') {
        if (!stmt_strict(l, bc)) {
            /* Execute a simple statement that only contains basic arithmetic... */
            if (!base(l, bc)) {
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
        if (!stmt_function(l, bc)) {
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
            factor_def_func(l, bc, fname);
            bc_gen_str(bc, INSTR_MEMBERN, fname->cstr);
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
            mario_printf("compile error: unexpected token, made no progress! ");
            ret = false;
        }
    }
    
    if (ret) {
        bc_gen(bc, INSTR_END);
    }
	else {
        compile_error_pos(&lex, -1);
	}
    
    lex_release(&lex);
    return ret;
}
