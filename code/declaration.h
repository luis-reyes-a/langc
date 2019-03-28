#ifndef DECLARATION_H
#define DECLARATION_H

#include "statement.h"

typedef enum
{
    Declaration_Variable, //u32 i;
    Declaration_Typedef,  //typedef uint32_t u32;
    Declaration_Struct,   //struct ident {...decls}
    Declaration_Union,    //union {decls...}
    Declaration_Enum,     //enum {enum_member; enum_member}  
    Declaration_EnumMember, //ident = 4;
    Declaration_Procedure,  //internal u32 main()
    Declaration_ProcedureArgs, //u32 argc, char **argv
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