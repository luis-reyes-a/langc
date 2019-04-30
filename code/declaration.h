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
    Declaration_ProcedureHeader,
    Declaration_Procedure,  //internal u32 main()
    Declaration_ProcedureArgs, //u32 argc, char **argv
    Declaration_Constant,
    Declaration_Include, //for c files only
    Declaration_Insert, //for any file
    
    Declaration_ForeignBlock, 
    Declaration_Macro,
    Declaration_MacroArgs,
    
    //Declaration_Sentinel,
}declaration_type;

typedef struct declaration_list
{
    declaration *decls;
    struct declaration_list *above;
}declaration_list; 

#define CAN_APPEND_DECL(decl) (((u64)decl > 1) ? 1 : 0)



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