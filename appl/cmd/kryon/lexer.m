# Lexer module for Kryon compiler

Lexer: module
{
    # Token type constants
    TOKEN_ENDINPUT: con 0;
    TOKEN_STRING: con 1;
    TOKEN_NUMBER: con 2;
    TOKEN_REAL: con 3;
    TOKEN_COLOR: con 4;
    TOKEN_IDENTIFIER: con 5;
    TOKEN_AT: con 6;
    TOKEN_VAR: con 7;
    TOKEN_FN: con 8;
    TOKEN_WINDOW: con 9;
    TOKEN_FRAME: con 10;
    TOKEN_BUTTON: con 11;
    TOKEN_LABEL: con 12;
    TOKEN_ENTRY: con 13;
    TOKEN_CHECKBUTTON: con 14;
    TOKEN_RADIOBUTTON: con 15;
    TOKEN_LISTBOX: con 16;
    TOKEN_CANVAS: con 17;
    TOKEN_SCALE: con 18;
    TOKEN_MENUBUTTON: con 19;
    TOKEN_MESSAGE: con 20;
    TOKEN_COLUMN: con 21;
    TOKEN_ROW: con 22;
    TOKEN_CENTER: con 23;
    TOKEN_ARROW: con 24;
    TOKEN_IMG: con 25;
    TOKEN_TYPE: con 26;    # type keyword
    TOKEN_STRUCT: con 27;  # struct keyword
    TOKEN_CHAN: con 28;    # chan keyword
    TOKEN_SPAWN: con 29;   # spawn keyword
    TOKEN_OF: con 30;      # of keyword
    TOKEN_ARRAY: con 31;   # array keyword
    TOKEN_CONST: con 32;   # const keyword
    TOKEN_IF: con 33;      # if keyword
    TOKEN_ELSE: con 34;    # else keyword
    TOKEN_FOR: con 35;     # for keyword
    TOKEN_WHILE: con 36;   # while keyword
    TOKEN_RETURN: con 37;  # return keyword
    TOKEN_IN: con 38;      # in keyword (for-each loops)

    # Token ADT - users need to access this
    Token: adt {
        toktype: int;
        string_val: string;
        number_val: big;
        real_val: real;
        lineno: int;
    };

    # Lexer ADT - internal structure
    LexerObj: adt {
        src: string;
        src_data: string;
        pos: int;
        lineno: int;
        column: int;
    };

    # Public interface - module-level functions
    create: fn(src: string, data: string): ref LexerObj;
    lex: fn(l: ref LexerObj): ref Token;
    peek_token: fn(l: ref LexerObj): ref Token;
    get_lineno: fn(l: ref LexerObj): int;
    get_column: fn(l: ref LexerObj): int;
};
