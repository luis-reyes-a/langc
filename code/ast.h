#ifndef AST_H
#define AST_H

#include "memory_arena.h"
static memory_arena ast_arena;
#define ast_arena_size (Megabytes(256))


//TODO make this more proper and easy to use
#define SyntaxError(_file, _line, _msg, ...) \
printf("Syntax Error in %s at line %llu\n" _msg "\n", _file, _line, __VA_ARGS__)

typedef struct declaration declaration;
typedef struct declaration_list declaration_list;
typedef struct statement  statement;


#include "tokens.c"
#include "type_checker.c"
#include "expression.c"
#include "declaration.c"
#include "statement.c"



internal void
ParseAST(declaration_list *ast, lexer_state *lexer)
{
    declaration *decl = ParseDeclaration(lexer, ast);
    while(decl)
    {
        DeclarationListAppend(ast, decl);
        //decls may be chained together but back pointers are prob not set!
        decl = ParseDeclaration(lexer, ast);
    }
}


//NOTE I may be able to allow the user to declare decls of the same identifier but of separate type
internal declaration *
FindTopLevelDeclaration(declaration *ast, char *identifier /*Type too?, allow that?*/)
{
    declaration *result = 0;
    for(declaration *decl = ast;
        decl;
        decl = decl->next)
    {
        if(decl->identifier == identifier)
        {
            result = decl;
        }
    }
    return result;
}

internal declaration * FixupVariable(declaration * var);
internal declaration * FixupProcedure(declaration * proc);
internal void RemoveAllInitializersFromStruct(declaration * decl);
internal expression *InitExpressionForDeclaration(declaration * member, expression * lval_struct_expr);
internal declaration * FixupStructUnion(declaration * decl);
internal void FixupEnumFlags(declaration * enum_flags);
internal void AstFixupC(declaration_list * ast_);

#endif