implement Parser;

include "sys.m";
    sys: Sys;
include "ast.m";
    ast: Ast;
include "lexer.m";
    lexer: Lexer;
include "parser.m";


# Helper: format error message with line number
fmt_error(l: ref Lexer->LexerObj, msg: string): string
{
    return sys->sprint("line %d: %s", l.get_lineno(), msg);
}

# Helper: expect a specific token type or return error
expect_token(l: ref Lexer->LexerObj, expected_typ: int): string
{
    tok := l.lex();
    if (tok.toktype != expected_typ) {
        return sys->sprint("expected token type %d, got %d", expected_typ, tok.toktype);
    }
    return nil;
}

# Helper: expect a specific character or return error
expect_char(l: ref Lexer->LexerObj, expected: int): string
{
    tok := l.lex();
    if (tok.toktype != expected) {
        return sys->sprint("expected '%c', got '%c'", expected, tok.toktype);
    }
    return nil;
}

# Parse a value (STRING, NUMBER, COLOR, IDENTIFIER)
parse_value(l: ref Lexer->LexerObj): (ref Ast->Value, string)
{
    tok := l.lex();

    case tok.toktype {
    Lexer->TOKEN_STRING =>
        return (ast.value_create_string(tok.string_val), nil);

    Lexer->TOKEN_NUMBER =>
        return (ast.value_create_number(tok.number_val), nil);

    Lexer->TOKEN_COLOR =>
        return (ast.value_create_color(tok.string_val), nil);

    Lexer->TOKEN_IDENTIFIER =>
        return (ast.value_create_ident(tok.string_val), nil);

    * =>
        return (nil, fmt_error(l, "expected value (string, number, color, or identifier)"));
    }
}

# Parse a property (name = value)
parse_property(l: ref Lexer->LexerObj): (ref Ast->Property, string)
{
    tok := l.lex();

    if (tok.toktype != Lexer->TOKEN_IDENTIFIER) {
        return (nil, fmt_error(l, "expected property name"));
    }

    name := tok.string_val;

    # Expect '='
    err := expect_char(l, '=');
    if (err != nil) {
        return (nil, err);
    }

    # Parse value
    (val, err) := parse_value(l);
    if (err != nil) {
        return (nil, err);
    }

    prop := ast.property_create(name);
    prop.value = val;

    return (prop, nil);
}

# Parse widget body content (properties and children)
parse_widget_body_content(l: ref Lexer->LexerObj): (ref Ast->Widget, ref Ast->Widget, string)
{
    props: ref Ast->Property = nil;
    children: ref Ast->Widget = nil;

    while (1) {
        tok := l.peek_token();

        # Check for end of body
        if (tok.toktype == '}' || tok.toktype == Lexer->TOKEN_EOF) {
            break;
        }

        # Property: identifier = value
        if (tok.toktype == Lexer->TOKEN_IDENTIFIER) {
            # Peek ahead to see if next token is '='
            (prop, err) := parse_property(l);
            if (err != nil) {
                return (nil, nil, err);
            }

            if (props == nil) {
                props = prop;
            } else {
                ast.property_list_add(props, prop);
            }
        }
        # Widget: Button { ... }
        else if (tok.toktype >= Lexer->TOKEN_APP && tok.toktype <= Lexer->TOKEN_CENTER) {
            (child, err) := parse_widget(l);
            if (err != nil) {
                return (nil, nil, err);
            }

            if (children == nil) {
                children = child;
            } else {
                ast.widget_list_add(children, child);
            }
        } else {
            return (nil, nil, fmt_error(l, "expected property or widget in body"));
        }
    }

    return (props, children, nil);
}

# Parse widget body: { ... }
parse_widget_body(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    # Expect '{'
    err := expect_char(l, '{');
    if (err != nil) {
        return (nil, err);
    }

    (props, children, err) := parse_widget_body_content(l);
    if (err != nil) {
        return (nil, err);
    }

    # Expect '}'
    err = expect_char(l, '}');
    if (err != nil) {
        return (nil, err);
    }

    # Create a wrapper widget to hold props and children
    w := ast.widget_create(Ast->WIDGET_CONTAINER);
    w.props = props;
    w.children = children;
    w.is_wrapper = 1;

    return (w, nil);
}

# Parse specific widget types
parse_window(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_WINDOW);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

parse_container(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_CONTAINER);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

parse_button(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_BUTTON);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

parse_text(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_TEXT);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

parse_input(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_INPUT);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

parse_column(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_COLUMN);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

parse_row(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_ROW);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

parse_center(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    (body, err) := parse_widget_body(l);
    if (err != nil) {
        return (nil, err);
    }

    w := ast.widget_create(Ast->WIDGET_CENTER);
    w.props = body.props;
    w.children = body.children;

    return (w, nil);
}

# Parse a widget (dispatch based on type)
parse_widget(l: ref Lexer->LexerObj): (ref Ast->Widget, string)
{
    tok := l.lex();

    case tok.toktype {
    Lexer->TOKEN_WINDOW =>
        return parse_window(l);

    Lexer->TOKEN_APP =>
        # App is treated as Window
        return parse_window(l);

    Lexer->TOKEN_CONTAINER =>
        return parse_container(l);

    Lexer->TOKEN_BUTTON =>
        return parse_button(l);

    Lexer->TOKEN_TEXT =>
        return parse_text(l);

    Lexer->TOKEN_INPUT =>
        return parse_input(l);

    Lexer->TOKEN_COLUMN =>
        return parse_column(l);

    Lexer->TOKEN_ROW =>
        return parse_row(l);

    Lexer->TOKEN_CENTER =>
        return parse_center(l);

    * =>
        return (nil, fmt_error(l, sys->sprint("unknown widget type token: %d", tok.toktype)));
    }
}

# Parse a single code block
parse_code_block(l: ref Lexer->LexerObj): (ref Ast->CodeBlock, string)
{
    tok := l.lex();

    code := "";
    typ := 0;

    case tok.toktype {
    Lexer->TOKEN_LIMBO =>
        typ = Ast->CODE_LIMBO;
        code = tok.string_val;

    Lexer->TOKEN_TCL =>
        typ = Ast->CODE_TCL;
        code = tok.string_val;

    Lexer->TOKEN_LUA =>
        typ = Ast->CODE_LUA;
        code = tok.string_val;

    * =>
        return (nil, fmt_error(l, "expected code block (@limbo, @tcl, or @lua)"));
    }

    return (ast.code_block_create(typ, code), nil);
}

# Parse multiple code blocks
parse_code_blocks(l: ref Lexer->LexerObj): (ref Ast->CodeBlock, string)
{
    first: ref Ast->CodeBlock = nil;
    last: ref Ast->CodeBlock = nil;

    while (1) {
        tok := l.peek_token();

        if (tok.toktype != Lexer->TOKEN_LIMBO &&
            tok.toktype != Lexer->TOKEN_TCL &&
            tok.toktype != Lexer->TOKEN_LUA) {
            break;
        }

        (cb, err) := parse_code_block(l);
        if (err != nil) {
            return (nil, err);
        }

        if (first == nil) {
            first = cb;
            last = cb;
        } else {
            last.next = cb;
            last = cb;
        }
    }

    return (first, nil);
}

# Parse app declaration
parse_app_decl(l: ref Lexer->LexerObj): (ref Ast->AppDecl, string)
{
    tok := l.lex();

    is_app := 0;

    case tok.toktype {
    Lexer->TOKEN_WINDOW =>
        # OK, continue
    Lexer->TOKEN_APP =>
        is_app = 1;
    * =>
        return (nil, fmt_error(l, "expected Window or App declaration"));
    }

    # Expect '{'
    err := expect_char(l, '{');
    if (err != nil) {
        return (nil, err);
    }

    (props, children, err) := parse_widget_body_content(l);
    if (err != nil) {
        return (nil, err);
    }

    # Expect '}'
    err = expect_char(l, '}');
    if (err != nil) {
        return (nil, err);
    }

    app := ast.app_decl_create();
    app.props = props;
    app.body = children;

    return (app, nil);
}

# Parse a complete program
parse_program(lexer: ref Lexer->LexerObj): (ref Ast->Program, string)
{
    prog := ast.program_create();

    # Parse optional code blocks
    tok := lexer.peek_token();
    if (tok.toktype == Lexer->TOKEN_LIMBO ||
        tok.toktype == Lexer->TOKEN_TCL ||
        tok.toktype == Lexer->TOKEN_LUA) {

        (code_blocks, err) := parse_code_blocks(lexer);
        if (err != nil) {
            return (nil, err);
        }
        prog.code_blocks = code_blocks;
    }

    # Parse app declaration
    (app, err) := parse_app_decl(lexer);
    if (err != nil) {
        return (nil, err);
    }
    prog.app = app;

    return (prog, nil);
}
