#ifndef DECLARATION_H
#define DECLARATION_H

#include "statement.h"

typedef enum
{
    Declaration_Variable,//semicolon
    Declaration_Typedef,//semicolon
    Declaration_Struct,
    Declaration_Union,
    Declaration_Enum,
    Declaration_EnumMember,
    Declaration_Procedure,
    Declaration_ProcedureArgs,
}declaration_type;



//NOTE:
//decls won't eat semicolons cuz elsewhere you may want to reuse this code
//and not want semicolons to mark 'decls'

typedef struct declaration
{
    declaration_type type;
    char *identifier;
    struct declaration *next; //TODO remove this...store same way as compound expression type
    union
    {
        struct
        {
            type_specifier *typespec; //var and typedefs
            expression *initializer; //var
        };
        struct //struct or union
        {
            struct declaration *members;
        };
        struct//enum
        {
            struct declaration *enum_members;
        };
        struct//enum member
        {
            expression *const_expr;
        };
        struct //proc
        {
            type_specifier *proc_return_type;
            declaration *proc_args;
            keyword_type proc_keyword; //TODO check approp keyword!
            statement *proc_body;
        };
    };
} declaration;

internal declaration *ParseDeclaration(lexer_state *lexer);


#endif