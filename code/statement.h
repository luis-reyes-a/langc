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
    Statement_Defer,//TODO def do this one!
    Statement_Goto,//TODO think about this...
    Statement_Break,
    Statement_Continue,
    Statement_Return,
    Statement_Compound,
    Statement_Null,
} statement_type;



typedef struct declaration declaration;

#include "expression.h"

typedef struct statement
{
    statement_type type;
    struct statement *next;
    union
    {
        declaration *decl;
        expression *expr;
        struct 
        {
            expression *if_expr;
            struct statement *if_stmt;
        };
        struct
        {
            struct statement *else_stmt;
        };
        struct
        {
            expression *switch_expr;
            struct statement *switch_cases;
        };
        struct
        {
            expression *case_label;
            struct statement *case_statement;
        };
        struct
        {
            expression *while_expr;
            struct statement *while_stmt;
        };
        struct
        {//NOTE implement some more types of for loops
            struct statement *for_init_stmt;
            expression *for_expr2;
            expression *for_expr3;
            struct statement *for_stmt;
        };
        struct
        {
            expression *return_expr;
        };
        struct
        {
            struct statement *defer_stmt;
        };
        struct
        {
            struct statement *compound_stmts;
            struct statement *compound_tail;
        };
        char *goto_handle;
    };
    
} statement;


internal statement *ParseStatement(lexer_state *lexer);

static statement *stmt_null;

#endif