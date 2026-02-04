#ifndef KRYON_LEXER_H
#define KRYON_LEXER_H

#include <stdio.h>
#include "y.tab.h"

/* Initialize lexer */
void lexer_init(FILE *in);

/* Main lexer function */
int yylex(void);

/* Get line number for error reporting */
int lexer_get_lineno(void);

/* Get column number for error reporting */
int lexer_get_column(void);

#endif /* KRYON_LEXER_H */
