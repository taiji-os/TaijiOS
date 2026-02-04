#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *escape_tk_string(const char *s) {
    if (!s) return strdup("");

    size_t len = strlen(s);
    char *result = malloc(len * 2 + 1);
    if (!result) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '"' || s[i] == '\\' || s[i] == '$' || s[i] == '[' || s[i] == ']') {
            result[j++] = '\\';
        }
        result[j++] = s[i];
    }
    result[j] = '\0';
    return result;
}

static const char *widget_type_to_tk(WidgetType type) {
    switch (type) {
        case WIDGET_BUTTON: return "button";
        case WIDGET_TEXT: return "label";
        case WIDGET_INPUT: return "entry";
        case WIDGET_WINDOW: return "toplevel";
        case WIDGET_CENTER:
        case WIDGET_COLUMN:
        case WIDGET_ROW:
        case WIDGET_CONTAINER: return "frame";
        default: return "frame";
    }
}

static const char *widget_type_to_name(WidgetType type) {
    switch (type) {
        case WIDGET_APP: return "App";
        case WIDGET_WINDOW: return "Window";
        case WIDGET_CONTAINER: return "Container";
        case WIDGET_BUTTON: return "Button";
        case WIDGET_TEXT: return "Text";
        case WIDGET_INPUT: return "Input";
        case WIDGET_COLUMN: return "Column";
        case WIDGET_ROW: return "Row";
        case WIDGET_CENTER: return "Center";
        default: return "Widget";
    }
}

static void process_widget_list(CodeGen *cg, Widget *w, const char *parent, int is_root);

/* Generate code for a single widget */
static void codegen_widget(CodeGen *cg, Widget *w, const char *parent, int is_root) {
    if (!w) return;

    /* Skip wrapper widgets */
    if (w->is_wrapper) {
        process_widget_list(cg, w->children, parent, is_root);
        return;
    }

    char widget_path[256];
    if (is_root) {
        snprintf(widget_path, sizeof(widget_path), ".");
    } else {
        snprintf(widget_path, sizeof(widget_path), "%s.w%d", parent, cg->widget_counter);
        cg->widget_counter++;
    }

    /* Generate widget creation command */
    fprintf(cg->out, "    tk->cmd(toplevel, \"%s",
            is_root ? "" : widget_path + 1); /* Skip leading dot for non-root */

    const char *tk_type = widget_type_to_tk(w->type);
    fprintf(cg->out, " %s\"", tk_type);

    /* Generate properties */
    Property *prop = w->props;
    while (prop) {
        if (prop->value) {
            switch (prop->value->type) {
                case VALUE_STRING: {
                    char *escaped = escape_tk_string(prop->value->v.string_val);
                    fprintf(cg->out, " -%s \"%s\"", prop->name, escaped);
                    free(escaped);
                    break;
                }
                case VALUE_NUMBER:
                    fprintf(cg->out, " -%s %ld", prop->name, prop->value->v.number_val);
                    break;
                case VALUE_COLOR: {
                    char *escaped = escape_tk_string(prop->value->v.color_val);
                    fprintf(cg->out, " -%s #%s", prop->name, escaped);
                    free(escaped);
                    break;
                }
                case VALUE_IDENTIFIER:
                    fprintf(cg->out, " -%s %s", prop->name, prop->value->v.ident_val);
                    break;
                default:
                    break;
            }
        }
        prop = prop->next;
    }

    /* Special handling for layout containers */
    if (w->type == WIDGET_COLUMN || w->type == WIDGET_ROW) {
        fprintf(cg->out, "\" :=");  /* Print output for reference */
    }

    fprintf(cg->out, ");\n");

    /* Process children */
    if (w->children) {
        process_widget_list(cg, w->children, widget_path, 0);
    }
}

/* Process widget list, flattening wrappers */
static void process_widget_list(CodeGen *cg, Widget *w, const char *parent, int is_root) {
    while (w) {
        if (w->is_wrapper) {
            /* Process children of wrapper with same parent */
            Widget *child = w->children;
            while (child) {
                if (child->is_wrapper) {
                    /* Nested wrapper - recurse */
                    process_widget_list(cg, child->children, parent, is_root);
                } else {
                    codegen_widget(cg, child, parent, is_root);
                }
                child = child->next;
            }
        } else {
            codegen_widget(cg, w, parent, is_root);
        }
        w = w->next;
    }
}

/* Generate prologue */
static void codegen_prologue(CodeGen *cg, Program *prog) {
    fprintf(cg->out, "implement %s;\n\n", cg->module_name);

    fprintf(cg->out, "include \"sys.m\";\n");
    fprintf(cg->out, "include \"draw.m\";\n");
    fprintf(cg->out, "include \"tk.m\";\n");
    fprintf(cg->out, "include \"tkclient.m\";\n\n");

    fprintf(cg->out, "sys: Sys;\n");
    fprintf(cg->out, "tk: Tk;\n");
    fprintf(cg->out, "tkclient: Tkclient;\n\n");
}

/* Generate code blocks (Limbo functions) */
static void codegen_code_blocks(CodeGen *cg, Program *prog) {
    CodeBlock *cb = prog->code_blocks;
    while (cb) {
        if (cb->type == CODE_LIMBO && cb->code) {
            fprintf(cg->out, "%s\n", cb->code);
        }
        cb = cb->next;
    }
}

/* Generate init function */
static void codegen_init(CodeGen *cg, Program *prog) {
    fprintf(cg->out, "\n");
    fprintf(cg->out, "init(nil: ref Draw->Context, nil: list of string)\n");
    fprintf(cg->out, "{\n");
    fprintf(cg->out, "    sys = load Sys Sys->PATH;\n");
    fprintf(cg->out, "    tk = load Tk Tk->PATH;\n");
    fprintf(cg->out, "    tkclient = load Tkclient Tkclient->PATH;\n\n");

    /* Get app properties */
    const char *title = "Application";
    int width = 400;
    int height = 300;
    const char *bg = "#191919";

    if (prog->app && prog->app->props) {
        Property *prop = prog->app->props;
        while (prop) {
            if (strcmp(prop->name, "title") == 0 && prop->value &&
                prop->value->type == VALUE_STRING) {
                title = prop->value->v.string_val;
            } else if (strcmp(prop->name, "width") == 0 && prop->value &&
                       prop->value->type == VALUE_NUMBER) {
                width = prop->value->v.number_val;
            } else if (strcmp(prop->name, "height") == 0 && prop->value &&
                       prop->value->type == VALUE_NUMBER) {
                height = prop->value->v.number_val;
            } else if (strcmp(prop->name, "background") == 0 && prop->value &&
                       prop->value->type == VALUE_COLOR) {
                bg = prop->value->v.color_val;
            } else if (strcmp(prop->name, "backgroundColor") == 0 && prop->value &&
                       prop->value->type == VALUE_COLOR) {
                bg = prop->value->v.color_val;
            }
            prop = prop->next;
        }
    }

    char *escaped_title = escape_tk_string(title);
    fprintf(cg->out, "    (toplevel, nil) := tkclient->toplevel"
            "(tkclient->plain, nil, \"%s\", Tkclient->Appl);\n\n", escaped_title);
    free(escaped_title);

    fprintf(cg->out, "    # Configure main window\n");
    fprintf(cg->out, "    tk->cmd(toplevel, \"configure -width %d -height %d -bg {%s}\");\n\n",
            width, height, bg);

    /* Build UI */
    fprintf(cg->out, "    # Build UI\n");
    cg->widget_counter = 0;

    if (prog->app && prog->app->body) {
        process_widget_list(cg, prog->app->body, ".", 1);
    }

    /* Event loop */
    fprintf(cg->out, "\n");
    fprintf(cg->out, "    # Event loop\n");
    fprintf(cg->out, "    tk->cmd(toplevel, \"pack . -expand 1 -fill both\");\n");
    fprintf(cg->out, "    tk->cmd(toplevel, \"update\");\n\n");

    fprintf(cg->out, "    for(;;) {\n");
    fprintf(cg->out, "        alt {\n");
    fprintf(cg->out, "        c := <-toplevel.ctxt =>\n");
    fprintf(cg->out, "            if (c == nil) {\n");
    fprintf(cg->out, "                return;\n");
    fprintf(cg->out, "            }\n");
    fprintf(cg->out, "            tkclient->wmctl(toplevel, c);\n");
    fprintf(cg->out, "        s := <-toplevel.ctxt.kbd =>\n");
    fprintf(cg->out, "            tk->keyboard(toplevel, s);\n");
    fprintf(cg->out, "        <-toplevel.wreq ||\n");
    fprintf(cg->out, "        <-toplevel.ctl ||\n");
    fprintf(cg->out, "        *toplevel.ctxt.ptr =>\n");
    fprintf(cg->out, "            ;\n");
    fprintf(cg->out, "        }\n");
    fprintf(cg->out, "    }\n");
    fprintf(cg->out, "}\n");
}

int codegen_generate(FILE *out, Program *prog, const char *module_name) {
    if (!out || !prog || !module_name) {
        return -1;
    }

    CodeGen cg;
    memset(&cg, 0, sizeof(CodeGen));
    cg.out = out;
    cg.module_name = module_name;
    cg.widget_counter = 0;
    cg.handler_counter = 0;

    /* Check for code block types */
    CodeBlock *cb = prog->code_blocks;
    while (cb) {
        if (cb->type == CODE_TCL) cg.has_tcl = 1;
        if (cb->type == CODE_LUA) cg.has_lua = 1;
        cb = cb->next;
    }

    /* Generate code */
    codegen_prologue(&cg, prog);
    codegen_code_blocks(&cg, prog);
    codegen_init(&cg, prog);

    return 0;
}
