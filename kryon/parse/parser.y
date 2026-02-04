%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

Program *ast_root = NULL;

void yyerror(const char *s) {
    extern int lexer_get_lineno(void);
    fprintf(stderr, "Parse error at line %d: %s\n", lexer_get_lineno(), s);
}

extern int yylex(void);

%}

%union {
    char *string;
    long number;
    Value *value;
    Property *property;
    Widget *widget;
    CodeBlock *codeblock;
    AppDecl *appdecl;
    Program *program;
}

%token <string> TOKEN_STRING TOKEN_IDENTIFIER TOKEN_COLOR
%token <number> TOKEN_NUMBER
%token <string> TOKEN_LIMBO TOKEN_TCL TOKEN_LUA
%token TOKEN_APP TOKEN_WINDOW TOKEN_CONTAINER TOKEN_BUTTON
%token TOKEN_TEXT TOKEN_INPUT TOKEN_COLUMN TOKEN_ROW TOKEN_CENTER
%token TOKEN_END TOKEN_EOF

%type <value> value
%type <property> property
%type <widget> widget widget_body_content widget_body_item_list widget_body_item
%type <codeblock> code_block code_blocks
%type <appdecl> app_decl
%type <program> program

%%

program:
    code_blocks app_decl
    {
        $$ = ast_program_create();
        $$->code_blocks = $1;
        $$->app = $2;
        ast_root = $$;
    }
    | app_decl
    {
        $$ = ast_program_create();
        $$->app = $1;
        ast_root = $$;
    }
    ;

code_blocks:
    code_block
    {
        $$ = $1;
    }
    | code_blocks code_block
    {
        $$ = $1;
        $1->next = $2;
    }
    ;

code_block:
    TOKEN_LIMBO
    {
        extern YYSTYPE yylval;
        $$ = ast_code_block_create(CODE_LIMBO, yylval.string);
    }
    | TOKEN_TCL
    {
        extern YYSTYPE yylval;
        $$ = ast_code_block_create(CODE_TCL, yylval.string);
    }
    | TOKEN_LUA
    {
        extern YYSTYPE yylval;
        $$ = ast_code_block_create(CODE_LUA, yylval.string);
    }
    ;

app_decl:
    TOKEN_WINDOW '{' widget_body_content '}'
    {
        $$ = ast_app_decl_create();
        if ($3) {
            $$->props = $3->props;
            $$->body = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_APP '{' widget_body_content '}'
    {
        /* For backward compatibility */
        $$ = ast_app_decl_create();
        if ($3) {
            $$->props = $3->props;
            $$->body = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    ;

widget_body_content:
    /* empty */
    {
        $$ = ast_widget_create(WIDGET_CONTAINER);
        $$->is_wrapper = 1;
    }
    | widget_body_item_list
    {
        $$ = ast_widget_create(WIDGET_CONTAINER);
        $$->props = $1->props;
        $$->children = $1->children;
        $1->props = NULL;
        $1->children = NULL;
        free($1);
        $$->is_wrapper = 1;
    }
    ;

widget_body_item_list:
    widget_body_item
    {
        $$ = $1;
    }
    | widget_body_item_list widget_body_item
    {
        if ($2->props) {
            Property *p = $1->props;
            if (p) {
                while (p->next) p = p->next;
                p->next = $2->props;
            } else {
                $1->props = $2->props;
            }
            $2->props = NULL;
        }
        if ($2->children) {
            Widget *w = $1->children;
            if (w) {
                while (w->next) w = w->next;
                w->next = $2->children;
            } else {
                $1->children = $2->children;
            }
            $2->children = NULL;
        }
        free($2);
        $$ = $1;
    }
    ;

widget_body_item:
    property
    {
        $$ = calloc(1, sizeof(*$$));
        $$->props = $1;
        $$->children = NULL;
    }
    | widget
    {
        $$ = calloc(1, sizeof(*$$));
        $$->props = NULL;
        $$->children = $1;
    }
    ;

property:
    TOKEN_IDENTIFIER '=' value
    {
        $$ = ast_property_create($1);
        $$->value = $3;
        free($1);
    }
    ;

value:
    TOKEN_STRING
    {
        $$ = ast_value_create(VALUE_STRING);
        $$->v.string_val = $1;
    }
    | TOKEN_NUMBER
    {
        $$ = ast_value_create(VALUE_NUMBER);
        $$->v.number_val = $1;
    }
    | TOKEN_COLOR
    {
        $$ = ast_value_create(VALUE_COLOR);
        $$->v.color_val = $1;
    }
    | TOKEN_IDENTIFIER
    {
        $$ = ast_value_create(VALUE_IDENTIFIER);
        $$->v.ident_val = $1;
    }
    ;

widget:
    TOKEN_CENTER '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_CENTER);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_COLUMN '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_COLUMN);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_ROW '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_ROW);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_CONTAINER '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_CONTAINER);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_BUTTON '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_BUTTON);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_TEXT '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_TEXT);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_INPUT '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_INPUT);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    | TOKEN_WINDOW '{' widget_body_content '}'
    {
        $$ = ast_widget_create(WIDGET_WINDOW);
        if ($3) {
            $$->props = $3->props;
            $$->children = $3->children;
            $3->props = NULL;
            $3->children = NULL;
        }
    }
    ;

%%

int yywrap(void) {
    return 1;
}
