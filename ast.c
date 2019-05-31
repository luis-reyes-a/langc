#include "ast.h"
//this is fixup code to make the ast more 'c-compatible'

typedef struct
{
    declaration *start;
    declaration *one_after_last;
} declaration_range;

internal int
TypesMatch(type_specifier *a, type_specifier *b)
{ //for now...
    Assert(a && b);
    return  a == b;
}

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

internal int
AnyMemberVariableTypeMatchesDeclaration(declaration *decl, declaration *against)
{
    Assert(decl && (decl->type == Declaration_Struct || decl->type == Declaration_Union));
    int match = false;
    for(declaration *member = decl->members;
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
            AnyMemberVariableTypeMatchesDeclaration(member, against);
        }
        else if(member->type == Declaration_Constant)
        {
            
        }
        else Panic("Can't declare that in a struct!");
    }
    return match;
}

internal void
InsertTypeDeclarationBeforeDeclaration(declaration *decl, declaration *typedecl)
{
    declaration_range range = InsertSecondDeclBeforeFirst(decl, typedecl);
    
    if(typedecl->type == Declaration_Struct || typedecl->type == Declaration_Union)
    {
        if(AnyMemberVariableTypeMatchesDeclaration(typedecl, decl))
        {
            Panic("Circular dependency here!");
        }
        
        for(declaration *check = range.start;
            check != range.one_after_last;
            check = check->next)
        {
            if(DeclarationIsUserDefinedType(check))
            {
                if (AnyMemberVariableTypeMatchesDeclaration(typedecl, check))
                {
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
    Assert(typedecl && DeclarationIsUserDefinedType(typedecl));
    
    InsertTypeDeclarationBeforeDeclaration(decl, typedecl);
}


typedef enum
{
    MetaExpr_Undefined,
    MetaExpr_Constant,
    
    
    MetaExpr_lvalue, //'persistent' object in memory
    MetaExpr_rvalue, //'immediate' value in memory
    
    
    //i kind of what to get rid of these
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
    type_specifier *typespec;
    union
    {
        expression cexpr;
        //declaration *decl;
    };
} meta_expression;

inline int
MetaReadValue(meta_expression *expr)
{
    if(expr->type == MetaExpr_Constant ||
       expr->type == MetaExpr_lvalue ||
       expr->type == MetaExpr_rvalue) return true;
    else return false;
}

inline meta_expression
MetaExpressionForDeclaration(declaration *decl)
{
    meta_expression result = {0};
    Assert(decl);
    if(decl->type == Declaration_Constant)
    {
        result.type = MetaExpr_Constant; 
        Assert(decl->expr_as_const);
        result.cexpr = *decl->expr_as_const;
    }
    else if(decl->type == Declaration_Variable)
    {
        result.typespec = decl->typespec;
    }
    else
    {
        if(decl->type == Declaration_Struct)
            result.type = MetaExpr_Struct;
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
    }
    
    return result;
}

#define boolean_expr_result_type internal_type_u32 


internal meta_expression //NOTE this function can be very misleading
ResolveExpression(declaration_list *scope, expression *expr, declaration *topmostdecl)
{
    meta_expression result = {0};
    if(ExpressionIsConstant(expr))
    {//NOTE this handles callidentifier
        result.type = MetaExpr_Constant;
        result.cexpr = *expr;
        if(expr->type == Expression_Sizeof &&
           expr->typespec->flags & TypeSpecFlags_TypeDeclaredAfterUse)
        {
            InsertTypespecBeforeDeclaration(scope, topmostdecl, expr->typespec); //only reason to have topmost is here
            expr->typespec->flags ^= TypeSpecFlags_TypeDeclaredAfterUse;
            result.typespec = internal_type_u32;
        } //TODO check values to determine appropraite type
        else if(expr->type == Expression_CharLiteral)
        {
            result.typespec = internal_type_s8;
        }
        else if(expr->type == Expression_StringLiteral)
        {
            result.typespec = internal_type_u64; //pointer?
        }
        else if(expr->type == Expression_Integer)
        {
            result.typespec = internal_type_s32;
        }
        else if(expr->type == Expression_RealNumber)
        {
            result.typespec = internal_type_float;
        }
        else if(expr->type == Expression_CallIdentifier) 
        {
            result.typespec = 0; //todo
        }
    }
    else if(expr->type == Expression_Identifier)
    {
        Assert(expr->decl);
        result = MetaExpressionForDeclaration(expr->decl);
    }
    
    else if(expr->type == Expression_UndeclaredIdentifier){
        result.type = MetaExpr_Undefined;
    }
    else if(expr->type == Expression_MemberAccess)  {
        meta_expression mexpr = ResolveExpression(scope, expr->struct_expr, topmostdecl);
        if(mexpr.type == MetaExpr_lvalue)
        {
            declaration_list *members = 0;
            if(mexpr.typespec->type == TypeSpec_StructUnion)
            {
                members = &FindDeclaration(scope, mexpr.typespec->identifier)->list;
            }
            else if(mexpr.typespec->type == TypeSpec_Ptr &&
                    mexpr.typespec->ptr_base_type->type == TypeSpec_StructUnion)
            {
                members = &FindDeclaration(scope, mexpr.typespec->ptr_base_type->identifier)->list;
            }
            declaration *mdecl = FindDeclarationScope(members, expr->struct_member_identifier);
            if(mdecl)
            {
                result = MetaExpressionForDeclaration(mdecl);
            }
            else Panic("Member not found in struct!");
        }
        else Panic(); //TODO type cast e.g. ((struct*)0)->member => offsetof(struct, member)
    }
    else if(expr->type == Expression_Ternary) 
    {
        meta_expression cond_expr = ResolveExpression(scope,  expr->tern_cond, topmostdecl);
        meta_expression true_expr = ResolveExpression(scope, expr->tern_true_expr, topmostdecl);
        meta_expression false_expr = ResolveExpression(scope, expr->tern_false_expr, topmostdecl);
        if(cond_expr.type != MetaExpr_Constant)
        {
            result.type = MetaExpr_Undefined;
        }
        else
        {
            if(cond_expr.cexpr.integer)
            {
                result = true_expr;
            }
            else
            {
                result = false_expr;
            }
        }
    }
    else if(expr->type == Expression_Compound) 
    {
        //wtf do I do here? do we only care if the whole thing is constant
        expression *outer;
        for(outer = expr;
            outer && outer->type == Expression_Compound;
            outer = outer->compound_next)
        {
            ResolveExpression(scope, outer->compound_expr, topmostdecl);
        }
        Assert(outer);
        result = ResolveExpression(scope, outer, topmostdecl);
    }
    else if(expr->type == Expression_Cast) 
    {
#if 0
        result = ResolveExpression(expr->cast_expression);
        if(result.type == MetaExpr_lvalue)
        {
            result.typespec = expr->casting_to;
        }
        //TODO
#endif
        Panic();
    }
    else if(expr->type == Expression_Call) 
    {
        //TODO how do I handle this
#if 0
        declaration *proc = FindDeclaration(scope, expr->identifier);
        declaration *arg = PROC_ARGS(proc);
        for(outer = expr->call_args, arg;
            outer && outer->type == Expression_Compound;
            arg = arg->next, outer = outer->compound_next)
        {
            Assert(arg & arg->type == Declaration_ProcedureArgs);
            meta_expression marg = ResolveExpression(outer->compound_expr, topmostdecl);
            if(marg.type == MetaExpr_rvalue || marg->type == MetaExpr_lvalue)
            {
                
            }
            else if(marg.type == MetaExpr_Constant)
            {
                
            }
            else Panic();
        }
        if(outer) ResolveExpression(outer, topmostdecl);
        
        result = (meta_expression){MetaExpr_rvalue, proc->proc_return_type};
#else
        Panic();
#endif
    }
    else if(expr->type == Expression_ArraySubscript) 
    {
        meta_expression array_expr = ResolveExpression(scope, expr->array_expr, topmostdecl);
        if(array_expr.type == MetaExpr_lvalue &&
           array_expr.typespec->type == TypeSpec_Array ||
           array_expr.typespec->type == TypeSpec_Ptr)
        {
            Assert(expr->array_actual_index_expr);
            if(!expr->array_index_expr_as_const)
            {
                meta_expression index_expr = ResolveExpression(scope, expr->array_actual_index_expr, topmostdecl);
                if(index_expr.type == MetaExpr_Constant)
                {
                    
                }
                else Panic();
            }
        }
        else Panic();
        
    }
    else if(expr->type == Expression_CompoundInitializer) 
    {
        //NOTE should this always be constant?
        expression *outer;
        for(outer = expr->call_args;
            outer && outer->type == Expression_Compound;
            outer = outer->compound_next)
        {
            ResolveExpression(scope, outer->compound_expr, topmostdecl);
        }
        if(outer) ResolveExpression(scope, outer, topmostdecl);
    }
    
    else if (expr->type == Expression_Binary)
    {
        result.type = MetaExpr_rvalue;
        meta_expression left = ResolveExpression(scope, expr->binary_left, topmostdecl);
        meta_expression right = ResolveExpression(scope, expr->binary_right, topmostdecl);
        if(MetaReadValue(&left) && MetaReadValue(&right))
        {
            if(expr->binary_type == '%') 
            {
                Assert(left.typespec && right.typespec);
                if(TypeIsIntegral(left.typespec) && TypeIsIntegral(right.typespec))
                {
                    result.type = MetaExpr_rvalue;
                    result.typespec = internal_type_u32; //TODO find better one
                }
                else Panic();
            }
            else if(expr->binary_type == '+' ||
                    expr->binary_type == '-' ||
                    expr->binary_type == '*' ||
                    expr->binary_type == '/') 
            {
                if(TypeIsIntegral(left.typespec) && TypeIsIntegral(right.typespec))
                {
                    result.typespec = internal_type_s64;
                }
                else if((TypeIsIntegral(left.typespec) && TypeIsFloat(right.typespec)) ||
                        (TypeIsFloat(left.typespec) && TypeIsIntegral(right.typespec)) ||
                        (TypeIsFloat(left.typespec) && TypeIsFloat(right.typespec)))
                {
                    
                    result.typespec = internal_type_float;
                }
                else Panic();
                
                if(expr->binary_type == '/' && right.type == MetaExpr_Constant)
                {
                    if(right.cexpr.type == Expression_Integer &&
                       right.cexpr.integer == 0)
                    {
                        Panic("Division by zero");
                    }
                    if(right.cexpr.type == Expression_RealNumber &&
                       right.cexpr.integer == 0)
                    {
                        Panic("Division by zero");
                    }
                    
                }
            }
            else if(expr->binary_type == '>' ||
                    expr->binary_type == '<' ||
                    expr->binary_type == Binary_GreaterThanEquals ||
                    expr->binary_type == Binary_LessThanEquals ||
                    expr->binary_type == Binary_Equals ||
                    expr->binary_type == Binary_NotEquals)
            {
                if(TypesMatch(left.typespec, right.typespec))
                {
                    result.type = MetaExpr_rvalue;
                    result.typespec = left.typespec;
                }
                else Panic();
            }
            else if(expr->binary_type == Binary_And ||
                    expr->binary_type == Binary_Or)
            {
                if((TypeIsIntegral(left.typespec) || left.typespec->type == TypeSpec_Ptr)  &&
                   (TypeIsIntegral(right.typespec) || right.typespec->type == TypeSpec_Ptr))
                {
                    result.type = MetaExpr_rvalue;
                    result.typespec = boolean_expr_result_type;
                }
                else Panic();
            }
            else if(expr->binary_type == Binary_BitwiseAnd ||
                    expr->binary_type == Binary_BitwiseOr  ||
                    expr->binary_type == Binary_BitwiseXor ||
                    expr->binary_type == Binary_LeftShift ||
                    expr->binary_type == Binary_RightShift)
            {
                if(TypeIsIntegral(left.typespec) && TypeIsIntegral(right.typespec))
                {
                    result.type = MetaExpr_rvalue;
                    result.typespec = left.typespec;
                }
                else Panic();
            }
            else if (expr->binary_type == Binary_Assign ||
                     expr->binary_type == Binary_AndAssign ||
                     expr->binary_type == Binary_OrAssign  ||
                     expr->binary_type == Binary_XorAssign ||
                     expr->binary_type == Binary_AddAssign ||
                     expr->binary_type == Binary_SubAssign ||
                     expr->binary_type == Binary_MulAssign ||
                     expr->binary_type == Binary_DivAssign ||
                     expr->binary_type == Binary_ModAssign)
            {
                if(left.type == MetaExpr_lvalue)
                {
                    if(TypesMatch(left.typespec, right.typespec))
                    {
                        result.type = MetaExpr_lvalue;
                        result.typespec =  left.typespec;
                        //TODO integral promotion type stuff
                    }
                    else Panic();
                }
                else Panic();
            }
        }
        else Panic();
    }
    else if (expr->type == Expression_Unary)
    {
        meta_expression mexpr = ResolveExpression(scope, expr->unary_expr, topmostdecl);
        if(MetaReadValue(&mexpr))
        {
            if(expr->unary_type == Unary_PreIncrement || 
               expr->unary_type == Unary_PreDecrement  ||
               expr->unary_type == Unary_PostIncrement ||
               expr->unary_type == Unary_PostDecrement)
            {
                if(mexpr.type == MetaExpr_lvalue)
                {
                    result.type = MetaExpr_lvalue;
                    result.typespec = mexpr.typespec;
                }
                else Panic();
            }
            else if(expr->unary_type == Unary_Minus)
            {
                if(TypeIsIntegral(mexpr.typespec) ||
                   TypeIsFloat(mexpr.typespec))
                {
                    result.type = MetaExpr_rvalue;
                    result.typespec = mexpr.typespec;
                }
            }
            else if (expr->unary_type == Unary_Not)
            {
                if(TypeIsIntegral(mexpr.typespec) ||
                   mexpr.typespec->type == TypeSpec_Ptr)
                {
                    result.type = MetaExpr_rvalue;
                    result.typespec = boolean_expr_result_type;
                }
            }
            else if(expr->unary_type == '*')
            {
                if(mexpr.typespec->type == TypeSpec_Ptr)
                {
                    result.type = MetaExpr_lvalue;
                    //not the base type but the base type below
                    //TODO this won't work
                    result.typespec = mexpr.typespec->ptr_base_type;
                }
                else Panic();
            }
            else if(expr->unary_type == '&')
            {
                if(mexpr.type == MetaExpr_lvalue)
                {
                    result.type = MetaExpr_lvalue;
                    type_specifier *ptr_type;
                    u32 star_count;
                    if(mexpr.typespec->type == TypeSpec_Ptr)
                    {
                        star_count = mexpr.typespec->star_count + 1;
                    }
                    else
                    {
                        star_count = 1;
                    }
                    ptr_type = FindPointerType(mexpr.typespec, star_count);
                    if(!ptr_type)
                    {
                        ptr_type = AddPointerType(mexpr.typespec, star_count);
                    }
                    result.typespec = ptr_type;
                }
                else Panic();
            }
            else Panic();
        }
        else Panic();
    }
    
    else if(expr->type == Expression_ToBeDefined)
    {
        Panic("TODO");
    }
    
    else Panic();
    
    
    return result;
}

internal void
FixStructUnionDeclaration(declaration *decl, declaration *tompostdecl);



internal void
FixStatement(declaration_list *ast, statement *stmt,
             declaration *topdecl, declaration *topmostdecl)
{
    return;
}

internal void
CheckType(declaration_list *scope, type_specifier *typespec, declaration *topmostdecl)
{
    if(typespec->flags & TypeSpecFlags_TypeDeclaredAfterUse)
    {
        Assert(topmostdecl);
        InsertTypespecBeforeDeclaration(scope, topmostdecl, typespec);
        typespec->flags ^= TypeSpecFlags_TypeDeclaredAfterUse;
    }
}

inline void
FixConstant(declaration_list *scope, declaration *decl, declaration *topmostdecl)
{
    Assert(decl->type == Declaration_Constant && decl->actual_expr);
    if(!decl->expr_as_const)
    {
        meta_expression mexpr = ResolveExpression(scope, decl->actual_expr, topmostdecl);
        if(mexpr.type == MetaExpr_Constant)
        {
            Assert(!decl->inferred_type);
            decl->inferred_type = mexpr.typespec;
            decl->expr_as_const = DuplicateExpression(&mexpr.cexpr);
        }
        else Panic("Member initializers need to be constant!");
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
            //TODO I should go to the top level here... SCOPE_TOP(&decl->list)
            CheckType(&decl->list, member->typespec, topmostdecl);
            if(!member->initializer_as_const)
            {
                Assert(member->initializer);
                meta_expression mexpr = ResolveExpression(&decl->list, member->initializer, topmostdecl);
                if(mexpr.type == MetaExpr_Constant)
                {
                    //TODO set initializer_as_const to something...
                    //or go ahead and just make the contructor right here?
                    if(mexpr.cexpr.integer == 0)
                    {
                        //it's okay
                    }
                    else
                    {
                        if(!TypesMatch(mexpr.typespec, member->typespec))
                        {
                            Panic("You can't assign that to variable to this type");
                        }
                    }
                    
                }
                else Panic("Member initializers need to be constant!");
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
        else Panic("Can't declare that in a struct!");
    }
    
    //TODO generate constructor for this guy
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
        else if(decl->type == Declaration_Variable)
        {
            CheckType(ast, decl->typespec, decl);
            
            if(decl->initializer && !decl->initializer_as_const)
            {
                meta_expression mexpr = ResolveExpression(ast, decl->initializer, decl);
                if(mexpr.type == MetaExpr_Constant)
                {
                    if(!TypesMatch(mexpr.typespec, decl->typespec))
                    {
                        Panic("You can't assign that to variable to this type");
                    }
                }
                else Panic("Expression needs to be constant");
            }
            else if(!decl->initializer)
            {
                decl->initializer = expr_zero;
                decl->initializer_as_const = expr_zero;
            }
        }
        else if(decl->type == Declaration_Constant)
        {
            FixConstant(ast, decl, decl);
        }
        else if(decl->type == Declaration_Include ||
                decl->type == Declaration_Insert)
        {
            Assert(decl->actual_expr);
            if(!decl->expr_as_const)
            {
                meta_expression mexpr = ResolveExpression(ast, decl->actual_expr, decl);
                if(mexpr.type == MetaExpr_Constant)
                {
                    Assert(mexpr.cexpr.type == Expression_StringLiteral);
                    decl->expr_as_const = DuplicateExpression(&mexpr.cexpr);
                }
                else Panic("Member initializers need to be constant!");
            }
        }
        else if(decl->type == Declaration_Enum ||
                decl->type == Declaration_EnumFlags)
        {
            //TODO let the user specify this
            type_specifier *enum_member_type = internal_type_s32;
            for(declaration *member = decl->members, *prev = 0;
                member;
                prev = member, member = member->next)
            {
                Assert(member->type == Declaration_EnumMember);
                Assert(member->actual_expr);
                if(!decl->expr_as_const)
                {
                    meta_expression mexpr = ResolveExpression(&decl->list, decl->actual_expr, decl);
                    if(mexpr.type == MetaExpr_Constant)
                    {
                        //inferred type is same for all of them
                    }
                    else Panic("Member initializers need to be constant!");
                }
                Assert(!member->inferred_type);
                member->inferred_type = enum_member_type;
            }
        }
        
        else if(decl->type == Declaration_ProcedureFowardGroup)
        {
            for(declaration *proc = decl->overloaded_list;
                proc;
                proc = proc->next)
            {
                CheckType(ast, proc->proc_return_type, proc);
                
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
        else Panic();
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