#include "ast.h"
//this is fixup code to make the ast more 'c-compatible'

typedef struct
{
    declaration *start;
    declaration *one_after_last;
} declaration_range;

internal declaration_range
InsertSecondDeclBeforeFirst(declaration *first, declaration *second)
{
    declaration_range range = {0};
    if(first->next != second)
    {
        range.start = first->next; //whatif this is second
        range.one_after_last = second->next;
    }
    
    
#if 1
    second->back->next = second->next;
    second->next->back = second->back;
    second->next = 0;
    second->back = 0;
    
    declaration *prev_back = first->back;
    first->back = second;
    second->back = prev_back;
    
    Assert(prev_back->next && prev_back->next == first);
    prev_back->next = second;
    second->next = first;
    
    
#else
    declaration *prev_back = first->back;
    first->back = second;
    second->back = prev_back;
    Assert(prev_back->next);
    prev_back->next = second;
    //NOTE if prev_back is first in entire list, it's back pointer has to be changed
    //in case the last decl in the list is changed!
    
    declaration *rest_decls = second->next;
    second->next = first;
    first->next = rest_decls; //NOTE what about stuff in between! This isn't right
    if(rest_decls) //in case of last one in list
    {
        rest_decls->back = first;
    }
#endif
    
    return range;
}

typedef enum
{
    MetaExpr_Undefined,
    
    //NOTE collapse these two?
    MetaExpr_Constant,
    MetaExpr_ConstantVar, 
    
    MetaExpr_Variable, 
    
    MetaExpr_Struct,
    MetaExpr_Union,
    MetaExpr_Enum,
    MetaExpr_EnumFlags,
    MetaExpr_EnumMember, 
    MetaExpr_Procedure, 
    
    
    MetaExpr_Include,
    MetaExpr_Insert, 
    
    MetaExpr_TempVariable, //Will I need this
} meta_expression_type;

typedef struct
{
    meta_expression_type type;
    union
    {
        expression cexpr;
        declaration *decl;
    };
} meta_expression;

inline meta_expression
MetaExpressionForDeclaration(declaration *decl)
{
    meta_expression result = {0};
    Assert(decl);
    if(decl->type == Declaration_Variable)
        result.type = MetaExpr_Variable;
    else if(decl->type == Declaration_Struct)
        result.type = MetaExpr_Struct;
    else if(decl->type == Declaration_Constant)
        result.type = MetaExpr_ConstantVar;
    else if(decl->type == Declaration_Union)
        result.type = MetaExpr_Union;
    else if(decl->type == Declaration_Enum)
        result.type = MetaExpr_Enum;
    else if(decl->type == Declaration_EnumFlags)
        result.type = MetaExpr_EnumFlags;
    else if(decl->type == Declaration_EnumMember) 
        result.type = MetaExpr_EnumMember;
    else if(decl->type == Declaration_Procedure) 
        result.type = MetaExpr_Procedure;
    else if(decl->type == Declaration_Include)
        result.type = MetaExpr_Include;
    else if(decl->type == Declaration_Insert) 
        result.type = MetaExpr_Insert;
    else Panic();
    result.decl = decl;
    return result;
}


internal meta_expression
ResolveExpression(declaration_list *scope, expression *expr, declaration *topmostdecl)
{
    //{ ...scope
    //    decls...  
    //    topdecl...containing the expr
    //}
    
    meta_expression result = {0};
    if(ExpressionIsConstant(expr))
    {
        result.type = MetaExpr_Constant;
        result.cexpr = *expr;
    }
    else if(expr->type == Expression_Identifier)
    {
        find_declaration_result find = FindDeclarationAndMarkTop(scope, topdecl, expr->identifier);
        if(find.decl)
        {
            result = MetaExpressionForDeclaration(find.decl);
            if(find.mark)
            {
                //identifier declared but after topdecl where expr was found!
                InsertSecondDeclBeforeFirst(topdecl, find.result);
            }
        }
        else
        {
            Panic("identifier never declared");
        }
    }
    
    if(expr->type == Expression_MemberAccess)  {
        meta_expression mexpr = DecayExpression(expr->struct_expr);
        if(mexpr.type == DecayType_Variable)
        {
            declaration *sdecl = mexpr.decl;
            if(sdecl->typespec->type == TypeSpec_StructUnion ||
               (sdecl->typespec->type == TypeSpec_Pointer &&
                sdecl->typespec->ptr_base_type == TypeSpec_StructUnion))
            {
                find_declaration_result find = FindDeclarationTop(&sdecl->list, expr->struct_member_identifier);
                if(find.decl)
                {
                    result = MetaExpressionForDeclaration(find.decl);
                }
                else Panic("Member not found in struct!");
            }
            else Panic();
        }
        else Panic();
    }
    if(expr->type == Expression_Ternary) 
    {
        meta_expression cond_decay = DecayExpression(scope, topdecl, expr->tern_cond);
        meta_expression true_decay = DecayExpression(scope, topdecl, expr->tern_true_expr);
        meta_expression false_decay = DecayExpression(scope, topdecl,expr->tern_false_expr);
        if(cond_decay.type != MetaExpr_Constant)
        {
            result.type = MetaExpr_Undefined;
        }
        else
        {
            if(cond_decay.cexpr.integer)
            {
                result = true_decay;
            }
            else
            {
                result = false_decay;
            }
        }
    }
    if(expr->type == Expression_Compound) 
    {
        //wtf do I do here? do we only care if the whole thing is constant
        
    }
    if(expr->type == Expression_Cast) 
    {
        Panic();
    }
    if(expr->type == Expression_Call) 
    {
        //NOTE right now only doing identifiers not function pointers!
        result = {MetaExpr_Undefined};
#if 0
        meta_expression mexpr = DecayExpression(scope, topdecl, expr->call_expr);
        if(mexpr.type == MetaExpr_Procedure)
        {
            //check if it returns anything
        }
#endif
    }
    if(expr->type == Expression_ArraySubscript) 
    {
        
    }
    if(expr->type == Expression_CompoundInitializer) 
    {
        
    }
    if(expr->type == Expression_SizeOf) 
    {
        
    }
    
    
    return result;
}

internal void
FixStructUnionDeclaration(declaration_list *ast, declaration *decl
                          declaration *topdecl, declaration *tompostdecl);

internal int
AnyMemberVariableTypeMatchesDeclaration(declaration *decl, declaration *against)
{
    Assert(decl && (decl->type == Declaration_Struct || decl->type == Declaration_Union))
        int match = false;
    for(declaration *member = decl->types;
        member;
        member = member->next)
    {
        if(member->type == Declaration_Variable)
        {
            if(member->typespec->identifier == against->identifier)
            {
                //NOTE should I actually try to find the typespecifier for check?
                match = true;
                break;
            }
        }
        else if(member->type == Declaration_Struct ||
                member->type == Declaration_Union)
        {
            CheckMemberTypesAgainst(member, against);
        }
        else if(member->type == Declaration_Constant)
        {
            Panic(); //TODO
        }
        else Panic("Can't declare that in a struct!")
    }
    return match;
}

inline int
DeclarationIsUserDefinedType(declaration *decl)
{
    //TODO TYPEDEFS!
    if(decl->type == Declaration_Struct ||
       decl->type == Declaration_Union  ||
       decl->type == Declaration_Enum   ||
       decl->type == Declaration_EnumFlags) return true;
    else return false;
}

internal void
InsertTypeDeclarationBeforeDeclaration(declaration *decl, declaration *typedecl)
{
    declaration_range range = InsertSecondDeclBeforeFirst(decl, typedecl);
    
    if(typedecl->type == Declaration_Struct || typedecl->type == Declaration_Union)
    {
        if(AnyMemberVariableTypeMatchesDeclaration(typedecl->members, decl))
        {
            Panic("Circular dependency here!");
        }
        
        for(declaration *check = range.start;
            check != range.one_after_last;
            check = check->next)
        {
            if(DeclarationIsUserDefinedType(check))
            {
                if (AnyMemberVariableTypeMatchesDeclaration((typedecl, check)))
                {
                    //rearrange them again!
                    InsertTypeDeclarationBeforeDeclaration(typedecl, check);
                }
            }
        }
    }
}

inline void
InsertTypespecBeforeDeclaration(declaration_list *ast, 
                                declaration *decl, type_specifier *typespec)
{
    declaration *typedecl = 0;
    typedecl = FindDeclaration(ast, typespec->identifier);
    Assert(typedecl && DeclarationIsUserDefinedType(typedecl););
    
    InsertTypeDeclarationBeforeDeclaration(decl, typedecl);
}



internal void
FixStatement(declaration_list *ast, statement *stmt,
             declaration *topdecl, declaration *topmostdecl)
{
    
}



internal void //NOTE topmostdecl is optional
CheckVariableType(declaration *decl, declaration *topmostdecl)
{
    Assert(decl->type == Declaration_Variable || decl->type == Declaration_ProcedureArgs);
    if(decl->typespec->flags & TypeSpecFlags_TypeDeclaredAfterUse)
    {
        Assert(topmostdecl);
        InsertUserTypeBeforeDeclaration(topmostdecl, decl->typespec);
        decl->typespec->flags ^= TypeSpecFlags_TypeDeclaredAfterUse;
    }
}

internal expression *
ResolveConstantExpression(declaration_list *scope, expression *initializer, declaration *topmostdecl)
{
    expression *result = 0;
    meta_expression mexpr = ResolveExpression(scope, initializer, topmostdecl);
    if(mexpr.type == MetaExpr_Constant)
        //TODO collapse contantVar to that!
    {
        result = DuplicateExpression(&mexpr.cexpr);
    }
    return result;
}

inline void
FixConstant(declaration_list *scope, declaration *decl, declaration *topmostdecl)
{
    Assert(decl->type == Declaration_Constant && decl->actual_expr);
    if(!decl->expr_as_const)
    {
        decl->expr_as_const = ResolveConstantExpression(scope, decl->actual_expr, topmostdecl);
    }
}

internal void //NOTE all this does is go through all the variable decls and check for out of order types
FixStructUnionDeclaration(declaration *decl, declaration *topmostdecl)
{
    //NOTE this is why we need better support for tagged unions in the language, I write this everywhere!
    Assert(decl->type == Declaration_Struct || decl->type == Declaration_Union);
    for(declaration *member = decl->members;
        member;
        member = member->next)
    {
        if(member->type == Declaration_Variable)
        {
            CheckVariableType(member, topmostdecl);
            if(member->initializer && !member->initializer_as_const)
            {
                member->initializer_as_const = ResolveConstantExpression(&decl->list, member->initializer, topmostdecl);
                Assert(member->initializer_as_const);
            }
            
        }
        else if(member->type == Declaration_Struct ||
                member->type == Declaration_Union)
        {
            Assert(!member->identifier);
            FixStructUnionDeclaration(member, topmostdecl);
        }
        else if(member->type == Declaration_Constant)
        {
            FixConstant(&decl->list, member, topmostdecl);
        }
        else Panic("Can't declare that in a struct!")
    }
    
    //generate constructor for this guy
}

internal void
FixAST(declaration_list *ast)
{
    for(declaration *decl = ast->decls;
        decl;
        decl = decl->next)
    {
        if(decl->type == Declaration_Struct ||
           decl->type == Declaration_Union)
        {
            FixStructUnionDeclaration(decl, decl);
        }
        else if(decl->type == Declaration_Variable, 0)
        {
            CheckVariableType(decl, decl);
            if(decl->initializer && !decl->initializer_as_const)
            {
                decl->initializer_as_const = 
                    ResolveConstantExpression(ast, decl->initializer, decl);
                Assert(decl->initializer_as_const);
            }
        }
        else if(decl->type == Declaration_Constant)
        {
            FixConstant(ast, decl, decl);
        }
        else if(decl->type == Declaration_Enum ||
                decl->type == Declaration_EnumFlags)
        {
            for(declaration *member = decl->members, *prev = 0;
                member;
                prev = member, member = member->next)
            {
                Assert(member->type == Declaration_EnumMember);
                Assert(member->actual_expr);
                if(!member->expr_as_const)
                {
                    member->expr_as_const = 
                        ResolveConstantExpression(&decl->list, member->actual_expr, decl);
                }
            }
        }
        else if(decl->type == Declaration_ProcedureHeader)
        {
            for(declaration *proc = decl->overloaded_list;
                proc;
                proc = proc->next)
            {
                if(decl->proc_return_type->flags & TypeSpecFlags_TypeDeclaredAfterUse)
                {
                    InsertUserTypeBeforeDeclaration(decl, decl->proc_return_type);
                    decl->proc_return_type->flags ^= TypeSpecFlags_TypeDeclaredAfterUse;
                }
                //check each statement
                
                statement *body = decl->proc_body;
                for(statement *stmt = body->compound_stmts;
                    stmt;
                    stmt = stmt->next)
                {
                    switch(stmt->type)
                    {
                        
                    }
                }
            }
        }
        else if(decl->type == Declaration_Procedure)
        {
            //NOTE shouldn't be a top level until end
        }
    }
}



#if 0
internal void
CheckStatement(statement *stmt, declaration_list *scope)
{
    if(stmt->type == Statement_Expression ||
       stmt->type == Statement_Return)
    {
        CheckExpression(stmt->expr, scope);
    }
    
    
    else if(stmt->type == Statement_If ||
            stmt->type == Statement_Switch ||
            stmt->type == Statement_While)
    {
        CheckExpression(stmt->cond_expr, scope);
        CheckStatement(stmt->cond_stmt, scope);
    }
    else if(stmt->type == Statement_Else)
    {
        Assert(!stmt->cond_expr);
        CheckStatement(stmt->cond_stmt, scope);
    }
    else if(stmt->type == Statement_SwitchCase) 
    {
        if(!stmt->cond_expr_as_const)
        {
            printf("case statement doesn't have constant value!");
        }
        CheckStatement(stmt->cond_stmt, scope);
    }
    
    else if(stmt->type == Statement_For) 
    {
        CheckStatement(stmt->for_init_stmt, scope);
        CheckExpression(stmt->for_expr2, scope);
        CheckExpression(stmt->for_expr3, scope);
        CheckStatement(stmt->for_stmt, scope);
    }
    else if(stmt->type == Statement_Defer)
    {
        CheckStatement(stmt->defer_stmt, scope);
    }
    else if(stmt->type == Statement_Compound)
    {
        CheckDeclarationList(&stmt->decl_list);
        for(statement *compound = stmt->compound_stmts;
            compound;
            compound = compound->next)
        {
            CheckStatement(compound, scope);
        }
        
    }
    else if(stmt->type == Statement_Break ||
            stmt->type == Statement_Continue ||
            stmt->type == Statement_Null ||
            stmt->type == Statement_Declaration) //this should always be checked already
    
    {
        //ignore
    }
    else Panic();
}

internal void 
CheckDeclaration(declaration *decl, declaration_list *scope)
{
    //NOTE we don't have to check for name collisions because always done on DeclListAppend()
    switch(decl->type)
    {
        case Declaration_Struct: case Declaration_Union:
        case Declaration_Enum: case Declaration_EnumFlags:
        Assert(decl->members);
        Assert(decl->list.above == scope);
        for(declaration *member = decl->members;
            member;
            member = member->next)
        {
            CheckDeclaration(member, &decl->list);
        }
        break;
        
        case Declaration_Procedure:
        CheckStatement(decl->proc_body, scope);
        break;
        
        case Declaration_Variable: 
        if(decl->initializer)
        {
            CheckExpression(decl->initializer, scope);
        }
        break;
        
        case Declaration_Typedef: 
        Panic();
        break;
        
        case Declaration_EnumMember:
        case Declaration_Constant:
        case Declaration_Include: 
        case Declaration_Insert:
        if(!decl->expr_as_const)
        {
            //TODO store error output info within decls
            printf("Enum member %s doesn't have a constant value", decl->identifier);
        }
        break;
        
        case Declaration_ForeignBlock:
        // do nothing
        break;
        
        case Declaration_ProcedureArgs:
        default: Panic();
    }
}


internal void
CheckDeclarationList(declaration_list *ast)
{
    for(declaration *decl = ast->decls;
        decl;
        decl = decl->next)
    {
        CheckDeclaration(decl, ast);
    }
}
#endif