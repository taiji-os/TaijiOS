#ifndef KRYON_CODEGEN_H
#define KRYON_CODEGEN_H

#include "../parse/ast.h"

typedef struct CodeGen {
    FILE *out;
    const char *module_name;
    int widget_counter;
    int handler_counter;
    int has_tcl;
    int has_lua;
} CodeGen;

int codegen_generate(FILE *out, Program *prog, const char *module_name);
char *escape_tk_string(const char *s);

#endif /* KRYON_CODEGEN_H */
