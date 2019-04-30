#ifndef DECLARATION_H
#define DECLARATION_H


typedef enum
{
    Declaration_Variable, //u32 i; this can be a field or statement decl;
    Declaration_Typedef,  //TODO typedef uint32_t u32;
    Declaration_Struct,   //struct ident {...}
    Declaration_Union,    //union {...}
    Declaration_Enum,     //enum {...}  
    Declaration_EnumFlags,//enum {...}  
    Declaration_EnumMember, 
    Declaration_Procedure,  //internal u32 main()
    Declaration_ProcedureArgs, //u32 argc, char **argv
    Declaration_Constant,
    Declaration_Include, //for c files only
    Declaration_Insert, //for any file
    
    Declaration_ForeignBlock, 
    Declaration_Macro,
    Declaration_MacroArgs,
}declaration_type;

typedef struct declaration_list
{
    declaration *decls;
    struct declaration_list *above;
}declaration_list; 


typedef struct declaration
{
    declaration_type type;
    char *identifier; //NOTE anonymous structs have this null
    struct declaration *next; 
    struct declaration *back; 
    
    union
    {
        struct
        {
            type_specifier *typespec; //var, procargs, and typedefs
            expression *initializer; //var and procargs
        };
        struct //constants, includes, enum_members
        {
            expression *actual_expr;
            expression *expr_as_const; //0 if not resolved to constant
        };
        union//struct, union, enums, enum_flags
        {
            declaration_list list; //NOTE this is how to implement using like in JAI
            struct
            {
                declaration *members;
                declaration_list *above;
            };
        };
        struct //proc
        {
            type_specifier *proc_return_type;
            declaration *proc_args;
            keyword_type proc_keyword;
            statement *proc_body;
        };
    };
} declaration;




internal declaration *ParseDeclaration(lexer_state *lexer, declaration_list *scope);

inline declaration *NewDeclaration(declaration_type type);
internal declaration *ParseTypedef(lexer_state *lexer);
internal type_specifier *ParseTypeSpecifier(lexer_state *lexer, after_parse_fixup **fixup);
internal declaration *ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type);
internal declaration *ParseProcedure(lexer_state *lexer, u32 qualifier, declaration_list *list);
internal declaration *ParseEnumMember(lexer_state *lexer, declaration *last_enum_member, int enum_flags);
internal declaration *ParseEnum(lexer_state *lexer, declaration_list *scope);
internal declaration *ParseEnumFlags(lexer_state *lexer, declaration_list *scope);
internal declaration *ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type);
internal declaration *ParseStructUnion(lexer_state *lexer, declaration_list *scope);
#endif