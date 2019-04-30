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
    Fixup_DeclBeforeDecl,
    Fixup_DeclBeforeStmt,
    Fixup_StructNeedsConstructor,
    Fixup_VarMayNeedConstructor,
    Fixup_ProcedureOptionalArgs,
} after_parse_fixup_type;



typedef struct
{
    after_parse_fixup_type type;
    union
    {
        struct 
        {
            union {declaration *first_decl, *decl;};
            declaration *decl_after_decl;
        };
        struct
        {
            declaration *first_statement;
            statement *decl_after_stmt;
        };
        struct
        {
            declaration *struct_with_no_constructor;
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