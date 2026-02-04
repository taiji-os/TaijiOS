# Code generator module for Kryon compiler

Codegen: module
{
    include "ast.m";

    # Generate Limbo/Tk code from AST
    generate: fn(output: string, prog: ref Ast->Program, module_name: string): string;
};
