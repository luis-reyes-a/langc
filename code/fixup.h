#ifndef FIXUP_H
#define FIXUP_H
//
//NOTE fixups are designed to transform the ast into a "c-compatible" ast
//so orders are fixed, stuff is directly initialized if needed
//optinal args are set
//

typedef enum
{
    Fixup_None,
    Fixup_DeclBeforeIdentifier,
    Fixup_DeclBeforeDecl, //move decl2 before decl
    Fixup_DeclBeforeStmt, //move decl2 before stmt
    Fixup_StructNeedsConstructor, //make constructor for decl
    Fixup_VarMayNeedConstructor,  //set initializer to constructor for decl
    Fixup_ProcedureOptionalArgs,  //remove optional args, embed in decl->proc_body
    Fixup_ExprNeedsConstExpression, //resolve expression to const expr, couldn't before
    Fixup_DeclNeedsConstExpression, //resolve expression to const expr, couldn't before
    Fixup_EnumListNeedsConstExpressions, //enum initializer wasn't resolved before, fix now to const expr
    Fixup_AppendAllOverloadedProceduresToEnd, //
} after_parse_fixup_type;



typedef struct
{
    after_parse_fixup_type type;
    union
    {
        declaration *decl;
        statement *stmt;
        expression *expr;
        char *identifier;
        struct
        {
            void *_ignored_;
            union
            {
                declaration *decl2;
                statement *stmt2;
                expression *expr2;
            };
        };
    };
    
    
} after_parse_fixup;

struct
{
    u32 fixup_count;
    after_parse_fixup fixups[512];
} fixup_table;


inline after_parse_fixup *
NewFixup(after_parse_fixup_type type)
{
    after_parse_fixup *result = 0;
    if(fixup_table.fixup_count < ArrayCount(fixup_table.fixups))
    {
        result = fixup_table.fixups + fixup_table.fixup_count++;
        result->type = type;
    }
    return result;
}

#endif