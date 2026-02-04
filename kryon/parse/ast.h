#ifndef KRYON_AST_H
#define KRYON_AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Widget types */
typedef enum {
    WIDGET_APP,
    WIDGET_WINDOW,
    WIDGET_CONTAINER,
    WIDGET_BUTTON,
    WIDGET_TEXT,
    WIDGET_INPUT,
    WIDGET_COLUMN,
    WIDGET_ROW,
    WIDGET_CENTER
} WidgetType;

/* Value types */
typedef enum {
    VALUE_STRING,
    VALUE_NUMBER,
    VALUE_COLOR,
    VALUE_IDENTIFIER,
    VALUE_ARRAY
} ValueType;

/* Code block types */
typedef enum {
    CODE_LIMBO,
    CODE_TCL,
    CODE_LUA
} CodeType;

/* Forward declarations */
struct Value;
struct Property;
struct Widget;
struct CodeBlock;
struct VarDecl;
struct ComponentDef;
struct Param;
struct AppDecl;
struct Program;

/* Value node */
typedef struct Value {
    ValueType type;
    union {
        char *string_val;
        long number_val;
        char *color_val;
        char *ident_val;
        struct {
            struct Value **items;
            int count;
        } array_val;
    } v;
} Value;

/* Property node */
typedef struct Property {
    char *name;
    Value *value;
    struct Property *next;
} Property;

/* Widget node */
typedef struct Widget {
    WidgetType type;
    char *id;
    Property *props;
    struct Widget *children;
    struct Widget *next;
    int is_wrapper;  /* 1 if synthetic wrapper, not a real widget */
} Widget;

/* Code block node */
typedef struct CodeBlock {
    CodeType type;
    char *code;
    struct CodeBlock *next;
} CodeBlock;

/* Variable declaration node */
typedef struct VarDecl {
    char *name;
    char *type;
    Value *init_value;
    struct VarDecl *next;
} VarDecl;

/* Parameter node */
typedef struct Param {
    char *name;
    char *type;
    char *default_value;
    struct Param *next;
} Param;

/* Component definition node */
typedef struct ComponentDef {
    char *name;
    Param *params;
    VarDecl *vars;
    CodeBlock *handlers;
    Widget *body;
    struct ComponentDef *next;
} ComponentDef;

/* App declaration node */
typedef struct AppDecl {
    char *title;
    Property *props;
    Widget *body;
} AppDecl;

/* Program node (root) */
typedef struct Program {
    VarDecl *vars;
    CodeBlock *code_blocks;
    ComponentDef *components;
    AppDecl *app;
} Program;

/* AST construction functions */
Program *ast_program_create(void);
VarDecl *ast_var_decl_create(const char *name, const char *type, Value *init);
CodeBlock *ast_code_block_create(CodeType type, const char *code);
ComponentDef *ast_component_create(const char *name);
AppDecl *ast_app_decl_create(void);
Widget *ast_widget_create(WidgetType type);
Property *ast_property_create(const char *name);
Value *ast_value_create(ValueType type);
Param *ast_param_create(const char *name, const char *type, const char *default_val);

/* Utility functions */
void ast_value_set_string(Value *v, const char *s);
void ast_value_set_number(Value *v, long n);
void ast_value_set_color(Value *v, const char *c);
void ast_value_set_ident(Value *v, const char *id);
void ast_value_set_array(Value *v, Value **items, int count);
void ast_property_set_value(Property *p, Value *v);

/* Widget linking functions */
void ast_widget_add_child(Widget *parent, Widget *child);
void ast_widget_add_property(Widget *w, Property *prop);
void ast_program_add_var(Program *prog, VarDecl *var);
void ast_program_add_code_block(Program *prog, CodeBlock *code);
void ast_program_add_component(Program *prog, ComponentDef *comp);
void ast_program_set_app(Program *prog, AppDecl *app);

/* List building functions */
Property *ast_property_list_add(Property *list, Property *item);
Widget *ast_widget_list_add(Widget *list, Widget *item);
VarDecl *ast_var_list_add(VarDecl *list, VarDecl *item);
CodeBlock *ast_code_block_list_add(CodeBlock *list, CodeBlock *item);
ComponentDef *ast_component_list_add(ComponentDef *list, ComponentDef *item);
Param *ast_param_list_add(Param *list, Param *item);

/* AST destruction functions */
void ast_free_program(Program *prog);
void ast_free_widget(Widget *w);
void ast_free_property(Property *p);
void ast_free_value(Value *v);
void ast_free_code_block(CodeBlock *cb);
void ast_free_var_decl(VarDecl *vd);
void ast_free_component(ComponentDef *cd);
void ast_free_app_decl(AppDecl *ad);
void ast_free_param(Param *p);

#endif /* KRYON_AST_H */
