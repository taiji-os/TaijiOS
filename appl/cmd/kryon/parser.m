# Parser module for Kryon compiler

Parser: module
{
    # Parse a complete program from the lexer
    parse_program: fn(lexer: ref Lexer->LexerObj): (ref Ast->Program, string);

    # Internal parsing functions (exported for testing)
    parse_code_blocks: fn(lexer: ref Lexer->LexerObj): (ref Ast->CodeBlock, string);
    parse_app_decl: fn(lexer: ref Lexer->LexerObj): (ref Ast->AppDecl, string);
    parse_widget: fn(lexer: ref Lexer->LexerObj): (ref Ast->Widget, string);
    parse_property: fn(lexer: ref Lexer->LexerObj): (ref Ast->Property, string);
    parse_value: fn(lexer: ref Lexer->LexerObj): (ref Ast->Value, string);
};
