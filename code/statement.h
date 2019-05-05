#ifndef STATEMENT_H
#define STATEMENT_H

typedef enum
{
    Statement_Declaration,
    Statement_Expression,
    Statement_If,
    Statement_Else,
    Statement_Switch, 
    Statement_SwitchCase, 
    Statement_While,
    Statement_For, //TODO may have more than one type of for loop!
    Statement_Defer,
    //Statement_Goto, do I need this?
    Statement_Break,
    Statement_Continue,
    Statement_Return,
    Statement_Compound,
    Statement_Null,
} statement_type;

#include "expression.h"

typedef struct statement
{
    statement_type type;
    struct statement *next;
    union
    {
        
        declaration *decl; //NOTE this can only be a single decl for decl_list to work
        expression *expr;
        struct //if, else, switch, cases, while, 
        {
            expression *cond_expr;
            expression *cond_expr_as_const;
            struct statement *cond_stmt;
        };
        struct //defer
        {
            struct statement *defer_stmt;
        };
        struct //for
        {
            struct statement *for_init_stmt;
            expression *for_expr2;
            expression *for_expr3;
            struct statement *for_stmt;
        };
        
        struct //compound statement
        {
            struct statement *compound_stmts;
            struct statement *compound_tail;
            declaration_list decl_list;
        };
    };
    
} statement;


inline statement *NewStatement(statement_type type);
internal void ParseCompoundStatement(lexer_state *lexer, statement *result, declaration_list *scope, declaration *top_decl);


static statement *stmt_null;

#endif