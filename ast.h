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
typedef struct type_specifier type_specifier;
//typedef struct lexer_state lexer_state;

internal declaration *find_decl(declaration_list *list, char *identifier);

#include "tokens.h"
#include "declaration.h"


#include "tokens.c"
#include "type_checker.c"

internal type_specifier *parse_typespec(lexer_state *lexer, declaration_list *scope);

#include "expression.c"
#include "declaration.c"
#include "statement.c"



internal void
ParseAST(declaration_list *ast, lexer_state *lexer)
{
    declaration *decl = parse_decl(lexer, ast);
    while(decl)
    {
        decl_list_append(ast, decl);
        decl = parse_decl(lexer, ast);
    }
}






typedef struct symbol symbol;



typedef struct symbol_list
{
    symbol *symbols;
    u32 count; 
    u32 cap;
    struct symbol_list *above;
} symbol_list;

typedef enum
{
    SymbolType_NonAggregate,
    SymbolType_Aggregate,
} symbol_type;

struct symbol
{
    symbol_type type;
    declaration *decl;
    union{
        symbol_list list; //to rearrange members in the struct
    };
} ;

inline symbol_list
MakeSymbolList(u32 cap, symbol_list *above)
{
    symbol_list sym_list = {0};
    sym_list.cap = cap;
    sym_list.above = above;
    sym_list.symbols = calloc(sym_list.cap, sizeof(symbol));
    return sym_list;
}


inline symbol *
NewSymbol(symbol_list *list)
{
    assert(list);
    if(list->count >= list->cap)
    {
        list->cap = list->cap + 256;
        list->symbols = realloc(list->symbols, list->cap * sizeof(symbol));
    }
    assert(list->count < list->cap);
    symbol *sym = list->symbols + list->count++;
    *sym = (symbol){0};
    return sym;
}

inline void
PushSymbol(symbol_list *list, declaration *decl)
{
    assert(list && decl);
    symbol *sym = NewSymbol(list);
    sym->decl = decl;
}


//internal void CheckType(declaration_list *scope, type_specifier *type, declaration *top); //ensures decl before use
//internal void FixAST(declaration_list *ast);

//internal void CheckTypeSpecifier(symbol_list *out, declaration_list *scope, type_specifier *type);
//internal void
//CheckTypeSpecifier(declaration_list *out, memory_arena *arena, 
//declaration_list *scope, type_specifier *typespec);
//internal void AST_SymbolList(declaration_list *ast, symbol_list *out);
internal void append_resolved_decl(declaration_list *out, memory_arena *arena, 
                                   declaration_list *scope, declaration *decl);


//internal declaration * FixupVariable(declaration * var);
//internal declaration * FixupProcedure(declaration * proc);
//internal void RemoveAllInitializersFromStruct(declaration * decl);
//internal expression *InitExpressionForDeclaration(declaration * member, expression * lval_struct_expr);
//internal declaration * FixupStructUnion(declaration * decl);
//internal void FixupEnumFlags(declaration * enum_flags);
//internal void AstFixupC(declaration_list * ast_);

#endif