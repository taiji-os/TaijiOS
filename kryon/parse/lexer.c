#include "lexer.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    FILE *in;
    int lineno;
    int column;
    int current_char;
    int primed;
    int at_eof;
    int in_code_block;
    CodeType code_type;
    char *code_buffer;
    size_t code_buffer_size;
    size_t code_buffer_len;
} LexerState;

static LexerState lexer;

void lexer_init(FILE *in) {
    memset(&lexer, 0, sizeof(LexerState));
    lexer.in = in;
    lexer.lineno = 1;
    lexer.column = 0;
    lexer.current_char = EOF;
    lexer.primed = 0;
    lexer.at_eof = 0;
    lexer.in_code_block = 0;
    lexer.code_buffer = NULL;
    lexer.code_buffer_size = 0;
    lexer.code_buffer_len = 0;
}

int lexer_get_lineno(void) {
    return lexer.lineno;
}

int lexer_get_column(void) {
    return lexer.column;
}

static int grow_buffer(void) {
    size_t new_size = lexer.code_buffer_size ? lexer.code_buffer_size * 2 : 4096;
    char *new_buf = realloc(lexer.code_buffer, new_size);
    if (!new_buf) return 0;
    lexer.code_buffer = new_buf;
    lexer.code_buffer_size = new_size;
    return 1;
}

static void append_to_buffer(int c) {
    if (lexer.code_buffer_len + 1 >= lexer.code_buffer_size) {
        if (!grow_buffer()) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
    }
    lexer.code_buffer[lexer.code_buffer_len++] = c;
}

static int next_char(void) {
    if (lexer.at_eof) return EOF;

    int c;
    if (!lexer.primed) {
        c = fgetc(lexer.in);
        if (c == EOF) {
            lexer.at_eof = 1;
            return EOF;
        }
    } else {
        c = lexer.current_char;
    }

    int next = fgetc(lexer.in);
    if (next == EOF) {
        lexer.at_eof = 1;
    }
    lexer.current_char = next;
    lexer.primed = 1;

    if (c == '\n') {
        lexer.lineno++;
        lexer.column = 0;
    } else {
        lexer.column++;
    }

    return c;
}

static int peek_char(void) {
    if (!lexer.primed) {
        int c = fgetc(lexer.in);
        if (c == EOF) {
            lexer.at_eof = 1;
            return EOF;
        }
        lexer.current_char = c;
        lexer.primed = 1;
    }
    return lexer.current_char;
}

static void skip_whitespace(void) {
    int c;
    while (1) {
        c = peek_char();
        if (c == EOF) break;

        /* Handle // comments - peek ahead */
        if (c == '/') {
            next_char(); /* consume first / */
            c = peek_char();
            if (c == '/') {
                /* Skip to end of line */
                next_char(); /* consume second / */
                while ((c = peek_char()) != EOF && c != '\n') {
                    next_char();
                }
                /* Continue to check for more whitespace/comments */
                continue;
            } else {
                /* Not a comment, the / will be handled by the main lexer */
                /* We can't unget, so return and let the main loop handle it */
                /* The primed flag will handle this */
                lexer.primed = 1;
                lexer.current_char = c;
                return;
            }
        }

        if (isspace(c)) {
            if (c == '\n' && lexer.in_code_block) {
                /* In code blocks, we preserve newlines */
                return;
            }
            next_char();
        } else {
            break;
        }
    }
}

static char *read_string_literal(void) {
    size_t capacity = 256;
    size_t len = 0;
    char *str = malloc(capacity);

    next_char(); /* Skip opening quote */

    while (1) {
        int c = peek_char();
        if (c == EOF) {
            fprintf(stderr, "Unterminated string at line %d\n", lexer.lineno);
            free(str);
            return NULL;
        }
        if (c == '"') {
            next_char();
            break;
        }
        if (c == '\\') {
            next_char();
            c = peek_char();
            if (c == 'n') {
                c = '\n';
            } else if (c == 't') {
                c = '\t';
            } else if (c == 'r') {
                c = '\r';
            }
        }
        next_char();
        if (len + 1 >= capacity) {
            capacity *= 2;
            char *new_str = realloc(str, capacity);
            if (!new_str) {
                fprintf(stderr, "Out of memory\n");
                free(str);
                return NULL;
            }
            str = new_str;
        }
        str[len++] = c;
    }
    str[len] = '\0';
    return str;
}

static int read_number(long *out_val) {
    long val = 0;
    int c = peek_char();

    while (c != EOF && isdigit(c)) {
        val = val * 10 + (c - '0');
        next_char();
        c = peek_char();
    }

    *out_val = val;
    return 1;
}

static char *read_identifier(void) {
    size_t capacity = 64;
    size_t len = 0;
    char *str = malloc(capacity);
    int c = peek_char();

    if (!isalpha(c) && c != '_') {
        free(str);
        return NULL;
    }

    while (c != EOF && (isalnum(c) || c == '_')) {
        next_char();
        if (len + 1 >= capacity) {
            capacity *= 2;
            char *new_str = realloc(str, capacity);
            if (!new_str) {
                fprintf(stderr, "Out of memory\n");
                free(str);
                return NULL;
            }
            str = new_str;
        }
        str[len++] = c;
        c = peek_char();
    }
    str[len] = '\0';
    return str;
}

static char *read_color_literal(void) {
    size_t len = 0;
    char str[16];

    next_char(); /* Skip # */

    int c = peek_char();
    while (c != EOF && (isxdigit(c) || c == '#' || c == '(' || c == ')' ||
                        c == ',' || c == '.' || c == '%' || c == ' ')) {
        if (len >= 15) break;
        next_char();
        str[len++] = c;
        c = peek_char();
    }
    str[len] = '\0';

    return strdup(str);
}

static int check_keyword(const char *ident) {
    if (strcmp(ident, "limbo") == 0) return TOKEN_LIMBO;
    if (strcmp(ident, "tcl") == 0) return TOKEN_TCL;
    if (strcmp(ident, "lua") == 0) return TOKEN_LUA;
    if (strcmp(ident, "end") == 0) return TOKEN_END;
    if (strcmp(ident, "App") == 0) return TOKEN_APP;
    if (strcmp(ident, "Window") == 0) return TOKEN_WINDOW;
    if (strcmp(ident, "Container") == 0) return TOKEN_CONTAINER;
    if (strcmp(ident, "Button") == 0) return TOKEN_BUTTON;
    if (strcmp(ident, "Text") == 0) return TOKEN_TEXT;
    if (strcmp(ident, "Input") == 0) return TOKEN_INPUT;
    if (strcmp(ident, "Column") == 0) return TOKEN_COLUMN;
    if (strcmp(ident, "Row") == 0) return TOKEN_ROW;
    if (strcmp(ident, "Center") == 0) return TOKEN_CENTER;
    return TOKEN_IDENTIFIER;
}

static int read_code_block(void) {
    /* We've already seen @ and consumed it */
    /* Read the code block until @end */

    lexer.code_buffer_len = 0;

    while (1) {
        int c = peek_char();

        if (c == EOF) {
            fprintf(stderr, "Unterminated code block at line %d\n", lexer.lineno);
            return 0;
        }

        /* Check for @end */
        if (c == '@') {
            next_char(); /* consume @ */
            c = peek_char();
            if (c == 'e' || c == 'E') {
                /* Possible 'end' */
                char buf[4];
                buf[0] = c;
                next_char();
                buf[1] = peek_char();
                next_char();
                buf[2] = peek_char();
                next_char();
                buf[3] = '\0';

                if (strcasecmp(buf, "end") == 0) {
                    /* Skip rest of line */
                    while ((c = peek_char()) != EOF && c != '\n') {
                        next_char();
                    }
                    lexer.in_code_block = 0;
                    break;
                }
                /* Not 'end', put back characters */
                for (int i = 2; i >= 0; i--) {
                    /* We can't really unget, so just continue */
                    /* This is a limitation - we'll add what we consumed */
                    append_to_buffer('@');
                    for (int j = 0; j <= i; j++) {
                        append_to_buffer(buf[j]);
                    }
                }
            } else {
                append_to_buffer('@');
            }
        }

        next_char();
        append_to_buffer(c);

        if (c == '\n') {
            /* Check next line for @end */
            int saved_line = lexer.lineno;
            skip_whitespace();
            c = peek_char();
            if (c == '@') {
                next_char(); /* consume @ */
                c = peek_char();
                if (c == 'e' || c == 'E') {
                    next_char();
                    if ((c = peek_char()) == 'n' || c == 'N') {
                        next_char();
                        if ((c = peek_char()) == 'd' || c == 'D') {
                            next_char();
                            /* Found @end */
                            lexer.in_code_block = 0;
                            break;
                        }
                        /* Not 'end', continue */
                        append_to_buffer('\n');
                        append_to_buffer('@');
                        append_to_buffer(tolower(c));
                    } else {
                        append_to_buffer('\n');
                        append_to_buffer('@');
                        append_to_buffer(tolower(c));
                    }
                } else {
                    append_to_buffer('\n');
                    append_to_buffer('@');
                    append_to_buffer(tolower(c));
                }
            } else {
                append_to_buffer('\n');
            }
        }
    }

    /* Null terminate and set yylval */
    if (lexer.code_buffer_len + 1 >= lexer.code_buffer_size) {
        grow_buffer();
    }
    lexer.code_buffer[lexer.code_buffer_len] = '\0';

    /* Set yylval.string to the code buffer */
    yylval.string = lexer.code_buffer;
    /* Create a new buffer for next time */
    lexer.code_buffer = NULL;
    lexer.code_buffer_size = 0;
    lexer.code_buffer_len = 0;

    return lexer.code_type == CODE_LIMBO ? TOKEN_LIMBO :
           lexer.code_type == CODE_TCL ? TOKEN_TCL : TOKEN_LUA;
}

int yylex(void) {
    YYSTYPE yylval_local;
    int token;
    char *str;

    skip_whitespace();

    int c = peek_char();

    if (c == EOF) {
        return 0;  /* yacc expects 0 for EOF, not TOKEN_EOF */
    }

    /* Check for @ keyword/code block start */
    if (c == '@') {
        next_char(); /* consume @ */
        c = peek_char();

        if (c == 'l' || c == 'L') {
            next_char(); /* consume l */
            c = peek_char();
            if (c == 'i' || c == 'I') {
                next_char(); /* consume i */
                c = peek_char();
                if (c == 'm' || c == 'M') {
                    next_char(); /* consume m */
                    c = peek_char();
                    if (c == 'b' || c == 'B') {
                        next_char(); /* consume b */
                        c = peek_char();
                        if (c == 'o' || c == 'O') {
                            next_char(); /* consume o */
                            /* Skip to newline */
                            while ((c = peek_char()) != EOF && c != '\n') {
                                next_char();
                            }
                            lexer.in_code_block = 1;
                            lexer.code_type = CODE_LIMBO;
                            return read_code_block();
                        }
                    }
                }
            }
        } else if (c == 't' || c == 'T') {
            next_char(); /* consume t */
            c = peek_char();
            if (c == 'c' || c == 'C') {
                next_char(); /* consume c */
                c = peek_char();
                if (c == 'l' || c == 'L') {
                    next_char(); /* consume l */
                    /* Skip to newline */
                    while ((c = peek_char()) != EOF && c != '\n') {
                        next_char();
                    }
                    lexer.in_code_block = 1;
                    lexer.code_type = CODE_TCL;
                    return read_code_block();
                }
            } else if (c == 'c' || c == 'L') { /* lua */
                next_char(); /* consume second letter */
                c = peek_char();
                if (c == 'a' || c == 'A') {
                    next_char(); /* consume a */
                    /* Skip to newline */
                    while ((c = peek_char()) != EOF && c != '\n') {
                        next_char();
                    }
                    lexer.in_code_block = 1;
                    lexer.code_type = CODE_LUA;
                    return read_code_block();
                }
            }
        } else if (c == 'e' || c == 'E') {
            /* @end */
            next_char(); /* consume e */
            c = peek_char();
            if (c == 'n' || c == 'N') {
                next_char(); /* consume n */
                c = peek_char();
                if (c == 'd' || c == 'D') {
                    next_char(); /* consume d */
                    /* Skip rest of line */
                    while ((c = peek_char()) != EOF && c != '\n') {
                        next_char();
                    }
                    return TOKEN_END;
                }
            }
        }

        /* Just an @ by itself - return @ */
        return '@';
    }

    /* String literal */
    if (c == '"') {
        str = read_string_literal();
        if (!str) return TOKEN_EOF;
        yylval.string = str;
        return TOKEN_STRING;
    }

    /* Color literal */
    if (c == '#') {
        str = read_color_literal();
        if (!str) return TOKEN_EOF;
        yylval.string = str;
        return TOKEN_COLOR;
    }

    /* Number */
    if (isdigit(c)) {
        long val;
        read_number(&val);
        yylval.number = val;
        return TOKEN_NUMBER;
    }

    /* Identifier or keyword */
    if (isalpha(c) || c == '_') {
        str = read_identifier();
        if (!str) return TOKEN_EOF;

        token = check_keyword(str);

        if (token == TOKEN_IDENTIFIER) {
            yylval.string = str;
        } else {
            free(str);
        }

        return token;
    }

    /* Single character tokens - return the character itself */
    next_char();

    switch (c) {
        case '{': return '{';
        case '}': return '}';
        case '(': return '(';
        case ')': return ')';
        case '[': return '[';
        case ']': return ']';
        case ',': return ',';
        case '.': return '.';
        case ':': return ':';
        case ';': return ';';
        case '=': return '=';
        case '+': return '+';
        case '-': return '-';
        case '*': return '*';
        case '/': return '/';
        case '@': return '@';
        default:
            fprintf(stderr, "Unknown character '%c' (0x%02x) at line %d\n",
                    isprint(c) ? c : '?', c, lexer.lineno);
            return 0;
    }
}
