# Lexer module for Kryon compiler

Lexer: module
{
    # Token type constants
    TOKEN_EOF: con 0;
    TOKEN_STRING: con 1;
    TOKEN_NUMBER: con 2;
    TOKEN_COLOR: con 3;
    TOKEN_IDENTIFIER: con 4;
    TOKEN_LIMBO: con 5;
    TOKEN_TCL: con 6;
    TOKEN_LUA: con 7;
    TOKEN_WINDOW: con 8;
    TOKEN_CONTAINER: con 9;
    TOKEN_BUTTON: con 10;
    TOKEN_TEXT: con 11;
    TOKEN_INPUT: con 12;
    TOKEN_COLUMN: con 13;
    TOKEN_ROW: con 14;
    TOKEN_CENTER: con 15;
    TOKEN_END: con 16;

    # Token ADT - users need to access this
    Token: adt {
        toktype: int;
        string_val: string;
        number_val: big;
        lineno: int;
    };

    # Lexer ADT - internal structure
    LexerObj: adt {
        src: string;
        src_data: string;
        pos: int;
        lineno: int;
        column: int;
        in_code_block: int;
        code_type: int;
    };

    # Public interface - module-level functions
    create: fn(src: string, data: string): ref LexerObj;
    lex: fn(l: ref LexerObj): ref Token;
    peek_token: fn(l: ref LexerObj): ref Token;
    get_lineno: fn(l: ref LexerObj): int;
    get_column: fn(l: ref LexerObj): int;
};
