#ifndef DECLARATION_H
#define DECLARATION_H

typedef enum
{
    Declaration_Variable, 
    Declaration_Constant,
    
    //These add entrires to the type table
    Declaration_Typedef,  
    Declaration_Struct,   
    Declaration_Union,    
    Declaration_Enum,     
    Declaration_EnumFlags,
    
    Declaration_EnumMember, 
    
    Declaration_ProcedureGroup,
    Declaration_ProcedureSignature,
    Declaration_Procedure,
    Declaration_ProcedureArgs, 
    
    
    Declaration_Include, 
    Declaration_Insert, 
    
    Declaration_ForeignBlock, 
    Declaration_Macro,
    Declaration_MacroArgs,
}declaration_type;


typedef struct declaration_list
{
    declaration *decls;
    struct declaration_list *above;
    //u32 alist; //decls that can be allowed in
}declaration_list; 


typedef enum
{
    ResolveState_NotResolved = 0,
    ResolveState_Resolving,
    ResolveState_Resolved,
} resolution_state; //not a good name


typedef struct declaration
{
    declaration_type type;
    char *identifier; //NOTE anonymous structs have this null
    struct declaration *next; 
    struct declaration *back; 
    resolution_state resolve;
    
    union
    {
        struct 
        {
            type_specifier *typespec;
            expression *initializer; 
            expression *initializer_as_const; 
        };
        union
        {
            declaration_list list; 
            struct
            {
                declaration *members;
                declaration_list *above;
            };
        };
        struct //proc foward decls
        {
            type_specifier *return_type;
            declaration *args;
            struct
            {
                u8 keyword;
                u8 num_args;
                u16 pad1;
            };
        };
        struct //proc
        {
            declaration *signature;
            statement *proc_body; //args in within the statement 
        };
    };
} declaration;








internal declaration *parse_decl(lexer_state *lexer, declaration_list *scope);

internal declaration *parse_typedef(lexer_state *lexer);
internal type_specifier *parse_typespec(lexer_state *lexer, declaration_list *scope);
internal declaration *parse_var(lexer_state *lexer, type_specifier *type,  declaration_list *scope);
internal declaration *parse_proc_args(lexer_state *lexer, type_specifier *type , declaration_list *scope);

internal declaration *parse_genum_member(lexer_state *lexer, declaration *last_enum_member, declaration_list *scope);
internal declaration *parse_enum(lexer_state *lexer, declaration_list *scope);
internal declaration *parse_enum_flags(lexer_state *lexer, declaration_list *scope);

internal declaration *parse_struct_union(lexer_state *lexer, declaration_list *scope);
#endif