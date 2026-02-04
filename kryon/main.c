#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parse/lexer.h"
#include "parse/ast.h"
#include "gen/codegen.h"

extern int yyparse(void);
extern Program *ast_root;

static void show_usage(const char *progname) {
    printf("Usage: %s [-o output] input.kry\n", progname);
    printf("\nOptions:\n");
    printf("  -o <output>  Specify output file (default: input.b)\n");
    printf("  -h           Show this help message\n");
}

static char *derive_module_name(const char *input_file) {
    if (!input_file) return strdup("Module");

    /* Find basename */
    const char *slash = strrchr(input_file, '/');
    const char *basename = slash ? slash + 1 : input_file;

    /* Copy and remove extension */
    char *module_name = strdup(basename);
    if (!module_name) return NULL;

    char *dot = strrchr(module_name, '.');
    if (dot) *dot = '\0';

    /* Capitalize first letter */
    if (module_name[0]) {
        module_name[0] = toupper((unsigned char)module_name[0]);
    }

    return module_name;
}

static char *derive_output_file(const char *input_file) {
    if (!input_file) return NULL;

    char *output = strdup(input_file);
    if (!output) return NULL;

    char *dot = strrchr(output, '.');
    if (dot) {
        strcpy(dot, ".b");
    } else {
        strcat(output, ".b");
    }

    return output;
}

int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            show_usage(argv[0]);
            return 1;
        } else {
            if (input_file) {
                fprintf(stderr, "Error: Multiple input files specified\n");
                return 1;
            }
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        show_usage(argv[0]);
        return 1;
    }

    /* Open input file */
    FILE *in = fopen(input_file, "r");
    if (!in) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        return 1;
    }

    /* Initialize lexer and parse */
    lexer_init(in);
    int parse_result = yyparse();
    fclose(in);

    if (parse_result != 0 || ast_root == NULL) {
        fprintf(stderr, "Error: Failed to parse input file\n");
        ast_free_program(ast_root);
        return 1;
    }

    /* Generate output filename if not specified */
    char *output_path = NULL;
    if (!output_file) {
        output_path = derive_output_file(input_file);
        output_file = output_path;
    }

    /* Derive module name from input file */
    char *module_name = derive_module_name(input_file);
    if (!module_name) {
        fprintf(stderr, "Error: Failed to generate module name\n");
        ast_free_program(ast_root);
        free(output_path);
        return 1;
    }

    /* Open output file */
    FILE *out = fopen(output_file, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
        ast_free_program(ast_root);
        free(module_name);
        free(output_path);
        return 1;
    }

    printf("Parsing %s...\n", input_file);
    printf("Generating %s from %s...\n", output_file, input_file);

    /* Generate code */
    if (codegen_generate(out, ast_root, module_name) != 0) {
        fprintf(stderr, "Error: Code generation failed\n");
        fclose(out);
        ast_free_program(ast_root);
        free(module_name);
        free(output_path);
        return 1;
    }

    fclose(out);
    printf("Successfully generated %s\n", output_file);

    /* Cleanup */
    ast_free_program(ast_root);
    free(module_name);
    free(output_path);

    return 0;
}
