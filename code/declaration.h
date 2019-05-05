#ifndef DECLARATION_H
#define DECLARATION_H



typedef enum
{
    Declaration_Variable, 
    Declaration_Constant,
    //user defined types
    Declaration_Typedef,  
    Declaration_Struct,   
    Declaration_Union,    
    Declaration_Enum,     
    Declaration_EnumFlags,
    
    Declaration_EnumMember, 
    
    Declaration_ProcedureHeader,
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
}declaration_list; 

//HACK
#define CAN_APPEND_DECL(decl) (((u64)decl > 1) ? 1 : 0)



typedef struct declaration
{
    declaration_type type;
    char *identifier; //NOTE anonymous structs have this null
    struct declaration *next; 
    struct declaration *back; 
    
    union
    {
        struct //variables, procedure_args, 
        {
            type_specifier *typespec;
            expression *initializer; 
            expression *initializer_as_const; //for struct members and proc_args
        };
        struct //constants, includes, enum_members
        {
            expression *actual_expr;
            expression *expr_as_const; //0 if not resolved to constant
        };
        union//struct, union, enums, enum_flags
        {
            declaration_list list; 
            struct
            {
                declaration *members;
                declaration_list *above;
            };
        };
        struct //proc_header
        {
            declaration *overloaded_list;
            declaration *last_overloaded_list;
        };
        struct //proc
        {
            type_specifier *proc_return_type;
            keyword_type proc_keyword;
            statement *proc_body; 
        };
    };
} declaration;







//static declaration *sentinel_decl;

//NOTE check decl_list full before this?
#define PROC_ARGS(_decl) \
((_decl->proc_body->decl_list.decls->type == Declaration_ProcedureArgs) ? \
_decl->proc_body->decl_list.decls : 0)








internal declaration *ParseDeclaration(lexer_state *lexer, declaration_list *scope);

inline declaration *NewDeclaration(declaration_type type);
internal void ParseTopLevelProcedure(lexer_state *lexer,  declaration_list *list);
internal declaration *ParseLocalProcedure(lexer_state *lexer, declaration_list *list);
internal declaration *ParseTypedef(lexer_state *lexer);
internal type_specifier *ParseTypeSpecifier(lexer_state *lexer, after_parse_fixup **fixup);
internal declaration *ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type);


internal declaration *ParseEnumMember(lexer_state *lexer, declaration *last_enum_member, int enum_flags, after_parse_fixup **fixup);
internal declaration *ParseEnum(lexer_state *lexer, declaration_list *scope);
internal declaration *ParseEnumFlags(lexer_state *lexer, declaration_list *scope);
internal declaration *ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type);
internal declaration *ParseStructUnion(lexer_state *lexer, declaration_list *scope);
#endif