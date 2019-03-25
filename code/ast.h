#ifndef AST_H
#define AST_H

#include "memory_arena.h"
static memory_arena ast_arena;
#define ast_arena_size (Megabytes(256))



//#define SyntaxError(_file, _line, _msg, ...)
#define SyntaxError(_file, _line, _msg, ...) \
printf("Syntax Error in %s at line %llu\n" _msg "\n", _file, _line, __VA_ARGS__)

#include "tokens.c"
#include "type_checker.c"
#include "expression.c"
#include "declaration.c"
#include "statement.c"

internal declaration *
ParseAST(lexer_state *lexer)
{
    declaration *first = ParseDeclaration(lexer);
    declaration *current = first;
    while(current->next) current = current->next;
    for(declaration *next = ParseDeclaration(lexer);
        next;
        next = ParseDeclaration(lexer))
    {
        current->next = next;
        while (current->next)  current = current->next;
    }
    return first;
}

#endif