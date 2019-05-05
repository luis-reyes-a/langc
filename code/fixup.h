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


#if 0

internal void
RemoveAllInitializersFromStruct(declaration *decl)
{
    //TODO memory leaks here!!!
    Assert(decl->type == Declaration_Struct ||
           decl->type == Declaration_Union);
    for(declaration *member = decl->members;
        member;
        member = member->next)
    {
        if(member->type == Declaration_Variable)
        {
            member->initializer = 0;
        }
        else if (member->type == Declaration_Struct ||
                 member->type == Declaration_Union)
        {
            RemoveAllInitializersFromStruct(member);
        }
    }
}

internal expression *
AssignExpressionForMember(declaration *member, expression *lval_struct_expr)
{
    expression *result = 0;
    if(member->type == Declaration_Variable &&
       member->initializer)
    {
        expression *lval = NewExpression(Expression_MemberAccess);
        lval->struct_expr = lval_struct_expr;
        lval->struct_member_identifier = member->identifier;
        expression *rval = member->initializer;
        result = NewBinaryExpression('=', lval, rval);
        
        member->initializer = 0;
    }
    else if(member->type == Declaration_Union)
    {
        for(declaration *inner_member = member->members;
            inner_member;
            inner_member = inner_member->next)
        {
            result = AssignExpressionForMember(inner_member, lval_struct_expr);
            if(result)
            {
                break;
            }
        }
        RemoveAllInitializersFromStruct(member);
    }
    else if(member->type == Declaration_Struct)
    {
        expression **tail = &result;
        for(declaration *inner_member = member->members;
            inner_member;
            inner_member = inner_member->next)
        {
            expression *init_expr = AssignExpressionForMember(inner_member, lval_struct_expr);
            if(init_expr)
            {
                if(!result)
                {
                    result = init_expr;
                }
                else
                {
                    AppendExpressionToTail(tail, init_expr);
                    tail = &((*tail)->compound_next);
                }
            }
            
        }
    }
    else Assert(member->type == Declaration_Variable); //unint var decl!
    return result;
}

internal void
FixupEnumFlags(declaration *enum_flags)
{
    enum_flags->type = Declaration_Enum;
    //why not just store this as an enum throughout?
}


internal void
AstFixupC(declaration_list *ast)//TODO
{
    //create constructros for all structs and unions
    for(u32 i = 0; i < fixup_table.fixup_count; ++i)
    {
        after_parse_fixup *fixup = fixup_table.fixups + i;
        if(fixup->type == Fixup_DeclBeforeDecl)
        {
            InsertSecondDeclBeforeFirst(fixup->decl, fixup->decl2);
        }
        else if(fixup->type == Fixup_DeclBeforeIdentifier)
        {
            declaration *decl = FindDeclaration(ast, fixup->identifier);
            Assert(decl);
            InsertSecondDeclBeforeFirst(decl, fixup->decl2);
        }
        else if (fixup->type == Fixup_StructNeedsConstructor)
        {
            Assert(fixup->decl);
            declaration *dstruct = fixup->decl;
            declaration *constructor = NewDeclaration(Declaration_Procedure);
            constructor->identifier = ConcatCStringsIntern(dstruct->identifier, "__Constructor");
            constructor->proc_keyword = Keyword_Inline;
            constructor->proc_return_type = FindBasicType(dstruct->identifier);
            constructor->proc_body = NewStatement(Statement_Compound);
            statement *decl_result_stmt = NewStatement(Statement_Declaration);
            decl_result_stmt->decl = NewDeclaration(Declaration_Variable);
            decl_result_stmt->decl->identifier = StringInternLiteral("result");
            decl_result_stmt->decl->typespec = constructor->proc_return_type;
            constructor->proc_body->compound_stmts = decl_result_stmt;
            constructor->proc_body->compound_tail = constructor->proc_body->compound_stmts;
            constructor->proc_body->decl_list.decls = decl_result_stmt->decl;
            constructor->proc_body->decl_list.above = ast;
            
            expression *result_expr =  NewExpressionIdentifier("result");
            if(dstruct->type == Declaration_Struct)
            {
                for(declaration *member = dstruct->members;
                    member;
                    member = member->next)
                {
                    expression *assign_expr = AssignExpressionForMember(member, result_expr);
                    if(assign_expr)
                    {
                        statement *stmt = NewStatement(Statement_Expression);
                        stmt->expr = assign_expr;
                        AppendStatement(constructor->proc_body, stmt);
                    }
                }
            }
            else
            {
                for(declaration *member = dstruct->members;
                    member;
                    member = member->next)
                {
                    expression *assign_expr = AssignExpressionForMember(member, result_expr);
                    if(assign_expr)
                    {
                        statement *stmt = NewStatement(Statement_Expression);
                        stmt->expr = assign_expr;
                        AppendStatement(constructor->proc_body, stmt);
                        break;
                    }
                }
                RemoveAllInitializersFromStruct(dstruct);
                
            }
            
            
            statement *return_result = NewStatement(Statement_Return);
            return_result->expr = result_expr;
            AppendStatement(constructor->proc_body, return_result);
            
            declaration *remaining = dstruct->next;
            dstruct->next = constructor;
            constructor->next = remaining;
            constructor->back = dstruct;
            remaining->back = constructor;
        }
        else if(fixup->type == Fixup_VarMayNeedConstructor)
        {
            if(fixup->decl->typespec->type == TypeSpec_UnDeclared)
            {
                Panic("Should've been fixed beforehand when first parsed typespecifier!");
            }
            else if (fixup->decl->typespec->type == TypeSpec_StructUnion)
            {
                if(fixup->decl->typespec->needs_constructor)
                {
                    Assert(fixup->decl->initializer);
                    fixup->decl->initializer->type = Expression_Call;
                    char *constructor_name = ConcatCStringsIntern(fixup->decl->typespec->identifier, "__Constructor");
                    fixup->decl->initializer->call_expr = NewExpressionIdentifier(constructor_name);
                    fixup->decl->initializer->call_args = 0;
                }
                else
                {
                    fixup->decl->initializer = expr_zero_struct;
                }
                
            }
            else if (fixup->decl->typespec->type == TypeSpec_Array)
            {
                fixup->decl->initializer = expr_zero_struct;
            }
            else
            {
                fixup->decl->initializer = expr_zero;
            }
        }
        else if(fixup->type == Fixup_ProcedureOptionalArgs)
        {
            Assert(fixup->decl->type == Declaration_Procedure);
            statement *body = fixup->decl->proc_body;
            statement opts = {0};
            opts.type = Statement_Compound;
            Assert(body->compound_stmts);
            for(declaration *arg = PROC_ARGS(fixup->decl);
                arg && arg->type == Declaration_ProcedureArgs;
                arg = arg->next)
            {
                Assert(arg->type == Declaration_ProcedureArgs);
                if(arg->initializer)
                {
                    statement *init_stmt = NewStatement(Statement_Expression);
                    expression *lval = NewExpressionIdentifier(arg->identifier);
                    init_stmt->expr = NewBinaryExpression('=', lval, arg->initializer);
                    arg->initializer = 0;
                    
                    AppendStatement(&opts, init_stmt);
                    
                }
            }
            
            
            if(opts.compound_stmts)
            {
                statement *rest_body = body->compound_stmts;
                body->compound_stmts = opts.compound_stmts;
                opts.compound_tail->next = rest_body;
            }
        }
        else if(fixup->type == Fixup_EnumListNeedsConstExpressions)
        {
            Assert(fixup->decl->type == Declaration_Enum ||
                   fixup->decl->type == Declaration_EnumFlags);
            declaration *prev = fixup->decl->members;
            for(declaration *member = fixup->decl->members;
                member;
                prev = member, member = member->next)
            {
                if(!member->expr_as_const)
                {
                    if(member->actual_expr)
                    {
                        expression cexpr = ResolvedExpression(member->actual_expr);
                        if(cexpr.type)
                        {
                            member->expr_as_const = NewExpression(0);
                            *member->expr_as_const = cexpr;
                        }
                        else Panic();
                    }
                    else
                    {
                        Assert(prev != member && prev->expr_as_const);
                        Assert(prev->expr_as_const->type == Expression_Integer);
                        if(fixup->decl->type == Declaration_Enum)
                        {
                            member->expr_as_const = NewExpressionInteger(prev->expr_as_const->integer + 1);
                            member->actual_expr = member->expr_as_const;
                        }
                        else
                        {
                            member->expr_as_const = NewExpressionInteger(prev->expr_as_const->integer << 1);
                            member->actual_expr = member->expr_as_const;
                        }
                    }
                }
            }
        }
        else if (fixup->type == Fixup_AppendAllOverloadedProceduresToEnd)
        {
            declaration *header = fixup->decl;
            Assert(header->type == Declaration_ProcedureHeader);
            for(declaration *proc = header->overloaded_list;
                proc;
                proc = proc->next)
            {
                Assert(proc->type == Declaration_Procedure);
                DeclarationListAppend(ast, proc);
            }
        }
        else
        {
            Panic();
        }
    }
}

#endif

#endif