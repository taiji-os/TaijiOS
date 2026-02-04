#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    char *copy = malloc(strlen(s) + 1);
    if (copy) strcpy(copy, s);
    return copy;
}

/* Program creation */
Program *ast_program_create(void) {
    Program *prog = calloc(1, sizeof(Program));
    return prog;
}

/* Widget creation */
Widget *ast_widget_create(WidgetType type) {
    Widget *w = calloc(1, sizeof(Widget));
    if (w) w->type = type;
    return w;
}

/* Property creation */
Property *ast_property_create(const char *name) {
    Property *p = calloc(1, sizeof(Property));
    if (p) p->name = strdup_safe(name);
    return p;
}

/* Value creation */
Value *ast_value_create(ValueType type) {
    Value *v = calloc(1, sizeof(Value));
    if (v) v->type = type;
    return v;
}

/* Parameter creation */
Param *ast_param_create(const char *name, const char *type, const char *default_val) {
    Param *p = calloc(1, sizeof(Param));
    if (p) {
        p->name = strdup_safe(name);
        p->type = strdup_safe(type);
        p->default_value = strdup_safe(default_val);
    }
    return p;
}

/* VarDecl creation */
VarDecl *ast_var_decl_create(const char *name, const char *type, Value *init) {
    VarDecl *vd = calloc(1, sizeof(VarDecl));
    if (vd) {
        vd->name = strdup_safe(name);
        vd->type = strdup_safe(type);
        vd->init_value = init;
    }
    return vd;
}

/* CodeBlock creation */
CodeBlock *ast_code_block_create(CodeType type, const char *code) {
    CodeBlock *cb = calloc(1, sizeof(CodeBlock));
    if (cb) {
        cb->type = type;
        cb->code = strdup_safe(code);
    }
    return cb;
}

/* Component creation */
ComponentDef *ast_component_create(const char *name) {
    ComponentDef *cd = calloc(1, sizeof(ComponentDef));
    if (cd) cd->name = strdup_safe(name);
    return cd;
}

/* AppDecl creation */
AppDecl *ast_app_decl_create(void) {
    return calloc(1, sizeof(AppDecl));
}

/* Value setters */
void ast_value_set_string(Value *v, const char *s) {
    if (!v) return;
    v->type = VALUE_STRING;
    v->v.string_val = strdup_safe(s);
}

void ast_value_set_number(Value *v, long n) {
    if (!v) return;
    v->type = VALUE_NUMBER;
    v->v.number_val = n;
}

void ast_value_set_color(Value *v, const char *c) {
    if (!v) return;
    v->type = VALUE_COLOR;
    v->v.color_val = strdup_safe(c);
}

void ast_value_set_ident(Value *v, const char *id) {
    if (!v) return;
    v->type = VALUE_IDENTIFIER;
    v->v.ident_val = strdup_safe(id);
}

void ast_value_set_array(Value *v, Value **items, int count) {
    if (!v) return;
    v->type = VALUE_ARRAY;
    v->v.array_val.count = count;
    if (count > 0 && items) {
        v->v.array_val.items = calloc(count, sizeof(Value*));
        for (int i = 0; i < count; i++) {
            v->v.array_val.items[i] = items[i];
        }
    }
}

void ast_property_set_value(Property *p, Value *v) {
    if (!p) return;
    p->value = v;
}

/* List building functions */
Property *ast_property_list_add(Property *list, Property *item) {
    if (!list) return item;
    Property *p = list;
    while (p->next) p = p->next;
    p->next = item;
    return list;
}

Widget *ast_widget_list_add(Widget *list, Widget *item) {
    if (!list) return item;
    Widget *w = list;
    while (w->next) w = w->next;
    w->next = item;
    return list;
}

VarDecl *ast_var_list_add(VarDecl *list, VarDecl *item) {
    if (!list) return item;
    VarDecl *v = list;
    while (v->next) v = v->next;
    v->next = item;
    return list;
}

CodeBlock *ast_code_block_list_add(CodeBlock *list, CodeBlock *item) {
    if (!list) return item;
    CodeBlock *cb = list;
    while (cb->next) cb = cb->next;
    cb->next = item;
    return list;
}

ComponentDef *ast_component_list_add(ComponentDef *list, ComponentDef *item) {
    if (!list) return item;
    ComponentDef *cd = list;
    while (cd->next) cd = cd->next;
    cd->next = item;
    return list;
}

Param *ast_param_list_add(Param *list, Param *item) {
    if (!list) return item;
    Param *p = list;
    while (p->next) p = p->next;
    p->next = item;
    return list;
}

/* Widget functions */
void ast_widget_add_child(Widget *parent, Widget *child) {
    if (!parent || !child) return;
    if (!parent->children) {
        parent->children = child;
    } else {
        Widget *w = parent->children;
        while (w->next) w = w->next;
        w->next = child;
    }
}

void ast_widget_add_property(Widget *w, Property *prop) {
    if (!w || !prop) return;
    if (!w->props) {
        w->props = prop;
    } else {
        Property *p = w->props;
        while (p->next) p = p->next;
        p->next = prop;
    }
}

/* Program functions */
void ast_program_add_var(Program *prog, VarDecl *var) {
    if (!prog || !var) return;
    prog->vars = ast_var_list_add(prog->vars, var);
}

void ast_program_add_code_block(Program *prog, CodeBlock *code) {
    if (!prog || !code) return;
    prog->code_blocks = ast_code_block_list_add(prog->code_blocks, code);
}

void ast_program_add_component(Program *prog, ComponentDef *comp) {
    if (!prog || !comp) return;
    prog->components = ast_component_list_add(prog->components, comp);
}

void ast_program_set_app(Program *prog, AppDecl *app) {
    if (!prog) return;
    prog->app = app;
}

/* Free functions */
void ast_free_value(Value *v) {
    if (!v) return;
    switch (v->type) {
        case VALUE_STRING:
            free(v->v.string_val);
            break;
        case VALUE_COLOR:
            free(v->v.color_val);
            break;
        case VALUE_IDENTIFIER:
            free(v->v.ident_val);
            break;
        case VALUE_ARRAY:
            if (v->v.array_val.items) {
                for (int i = 0; i < v->v.array_val.count; i++) {
                    ast_free_value(v->v.array_val.items[i]);
                }
                free(v->v.array_val.items);
            }
            break;
        default:
            break;
    }
    free(v);
}

void ast_free_property(Property *p) {
    if (!p) return;
    free(p->name);
    ast_free_value(p->value);
    /* Don't free next - it's part of a list */
}

void ast_free_widget(Widget *w) {
    if (!w) return;
    free(w->id);
    Property *p = w->props;
    while (p) {
        Property *next = p->next;
        ast_free_property(p);
        free(p);
        p = next;
    }
    Widget *child = w->children;
    while (child) {
        Widget *next = child->next;
        ast_free_widget(child);
        child = next;
    }
    /* Don't free next - it's part of a list */
}

void ast_free_param(Param *p) {
    if (!p) return;
    free(p->name);
    free(p->type);
    free(p->default_value);
    /* Don't free next - it's part of a list */
}

void ast_free_var_decl(VarDecl *vd) {
    if (!vd) return;
    free(vd->name);
    free(vd->type);
    ast_free_value(vd->init_value);
    /* Don't free next - it's part of a list */
}

void ast_free_code_block(CodeBlock *cb) {
    if (!cb) return;
    free(cb->code);
    /* Don't free next - it's part of a list */
}

void ast_free_component(ComponentDef *cd) {
    if (!cd) return;
    free(cd->name);
    Param *p = cd->params;
    while (p) {
        Param *next = p->next;
        ast_free_param(p);
        free(p);
        p = next;
    }
    VarDecl *v = cd->vars;
    while (v) {
        VarDecl *next = v->next;
        ast_free_var_decl(v);
        free(v);
        v = next;
    }
    CodeBlock *cb = cd->handlers;
    while (cb) {
        CodeBlock *next = cb->next;
        ast_free_code_block(cb);
        free(cb);
        cb = next;
    }
    ast_free_widget(cd->body);
    /* Don't free next - it's part of a list */
}

void ast_free_app_decl(AppDecl *ad) {
    if (!ad) return;
    free(ad->title);
    Property *p = ad->props;
    while (p) {
        Property *next = p->next;
        ast_free_property(p);
        free(p);
        p = next;
    }
    ast_free_widget(ad->body);
}

void ast_free_program(Program *prog) {
    if (!prog) return;
    VarDecl *v = prog->vars;
    while (v) {
        VarDecl *next = v->next;
        ast_free_var_decl(v);
        free(v);
        v = next;
    }
    CodeBlock *cb = prog->code_blocks;
    while (cb) {
        CodeBlock *next = cb->next;
        ast_free_code_block(cb);
        free(cb);
        cb = next;
    }
    ComponentDef *cd = prog->components;
    while (cd) {
        ComponentDef *next = cd->next;
        ast_free_component(cd);
        free(cd);
        cd = next;
    }
    ast_free_app_decl(prog->app);
    free(prog);
}
