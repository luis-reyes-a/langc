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
typedef struct expression expression;


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
        if(CAN_APPEND_DECL(decl)) //These should be zero not by error but by parsing Top Procedures
        {
            DeclarationListAppend(ast, decl);
        }
        
        //decls may be chained together but back pointers are prob not set!
        decl = ParseDeclaration(lexer, ast);
    }
}

internal declaration * FixupVariable(declaration * var);
internal declaration * FixupProcedure(declaration * proc);
internal void RemoveAllInitializersFromStruct(declaration * decl);
internal expression *InitExpressionForDeclaration(declaration * member, expression * lval_struct_expr);
internal declaration * FixupStructUnion(declaration * decl);
internal void FixupEnumFlags(declaration * enum_flags);
internal void AstFixupC(declaration_list * ast_);

#endif