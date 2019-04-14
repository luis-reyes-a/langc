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
    Declaration_EnumFlags,     //enum {enum_member; enum_member}  
    Declaration_EnumMember, //ident = 4;
    Declaration_Procedure,  //internal u32 main()
    Declaration_ProcedureArgs, //u32 argc, char **argv
}declaration_type;

typedef struct declaration
{
    declaration_type type;
    char *identifier; //NOTE anonymous structs have this null
    struct declaration *next; //TODO remove this...store same way as compound expression type
    union
    {
        struct
        {
            type_specifier *typespec; //var, procargs, and typedefs
            expression *initializer; //var and procargs
        };
        struct //struct, union, enums
        {
            struct declaration *members;
            bool32 struct_needs_constructor;
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



typedef struct
{
    declaration *decl;
} decl_fixup_info;

internal declaration *ParseDeclaration(lexer_state *lexer);

inline declaration *NewDeclaration(declaration_type type);
internal declaration *ParseTypedef(lexer_state *lexer);
internal type_specifier *ParseTypeSpecifier(lexer_state *lexer);
internal declaration *ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type);
internal declaration *ParseProcedure(lexer_state *lexer, u32 qualifier);
internal declaration *ParseEnumMember(lexer_state *lexer, declaration *last_enum_member, int enum_flags);
internal declaration *ParseEnum(lexer_state *lexer);
internal declaration *ParseEnumFlags(lexer_state *lexer);
internal declaration *ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type);
inline bool32 ValidMemberDeclaration(declaration *decl, type_specifier *struct_typespec);
internal bool32 AreMembersInitializedToSomething(declaration *decl);
internal declaration *ParseStructUnion(lexer_state *lexer);
internal declaration *ParseDeclaration(lexer_state *lexer);
internal void PrintDeclaration(declaration *decl, u32 indent);
inline void PrintAST(declaration *ast);
#endif