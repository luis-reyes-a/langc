#include "ast.h"
internal int
TypesMatch(type_specifier *a, type_specifier *b)
{ 
    Assert(a && b); //NOTE will this suffice?
    return  a == b;
}


typedef enum
{
    MetaExpr_Undefined,
    MetaExpr_Constant,
    
    MetaExpr_lvalue, 
    MetaExpr_rvalue, 
    
    
    
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
ResolveExpression(symbol_list *out, declaration_list *scope, expression *expr)
{
    meta_expression result = {0};
    Assert(expr);
    if(ExpressionIsConstant(expr))
    {//NOTE this handles callidentifier
        result.type = MetaExpr_Constant;
        result.cexpr = *expr;
        //TODO check values to determine appropraite type
        if(expr->type == Expression_Sizeof)
        {
            CheckTypeSpecifier(out, scope, expr->typespec);
            result.typespec = internal_type_u32;
        } 
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
        //TODO think about this, this will lead to some wierd behavior
        if(!expr->decl) 
        {
            declaration *decl = 0;
            symbol_list *current_out = out;
            for(declaration_list *current = scope;
                !decl && current;
                current = current->above)
            {
                decl = FindDeclarationScope(current, expr->identifier);
                if(decl)
                {
                    AppendResolvedDeclaration(current_out, current, decl);
                    break;
                }
                else
                {
                    current_out = current_out->above;
                }
            }
            if(!decl)
            {
                Panic("identifier never declared!");
            }
            expr->decl = decl;
        }
    }
    
    
    
    else if(expr->type == Expression_UndeclaredIdentifier){
        result.type = MetaExpr_Undefined;
    }
    else if(expr->type == Expression_MemberAccess)  {
        meta_expression mexpr = ResolveExpression(out, scope, expr->struct_expr);
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
        meta_expression cond_expr = ResolveExpression(out, scope,  expr->tern_cond);
        meta_expression true_expr = ResolveExpression(out, scope, expr->tern_true_expr);
        meta_expression false_expr = ResolveExpression(out, scope, expr->tern_false_expr);
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
            ResolveExpression(out, scope, outer->compound_expr);
        }
        Assert(outer);
        result = ResolveExpression(out, scope, outer);
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
            meta_expression marg = ResolveExpression(outer->compound_expr);
            if(marg.type == MetaExpr_rvalue || marg->type == MetaExpr_lvalue)
            {
                
            }
            else if(marg.type == MetaExpr_Constant)
            {
                
            }
            else Panic();
        }
        if(outer) ResolveExpression(outer);
        
        result = (meta_expression){MetaExpr_rvalue, proc->proc_return_type};
#else
        Panic();
#endif
    }
    else if(expr->type == Expression_ArraySubscript) 
    {
        meta_expression array_expr = ResolveExpression(out, scope, expr->array_expr);
        if(array_expr.type == MetaExpr_lvalue &&
           array_expr.typespec->type == TypeSpec_Array ||
           array_expr.typespec->type == TypeSpec_Ptr)
        {
            Assert(expr->array_actual_index_expr);
            if(!expr->array_index_expr_as_const)
            {
                meta_expression index_expr = ResolveExpression(out, scope, expr->array_actual_index_expr);
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
            ResolveExpression(out, scope, outer->compound_expr);
        }
        if(outer) ResolveExpression(out, scope, outer);
    }
    
    else if (expr->type == Expression_Binary)
    {
        result.type = MetaExpr_rvalue;
        meta_expression left = ResolveExpression(out, scope, expr->binary_left);
        meta_expression right = ResolveExpression(out, scope, expr->binary_right);
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
        meta_expression mexpr = ResolveExpression(out, scope, expr->unary_expr);
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
        
    }
    
    else Panic();
    
    
    return result;
}





inline meta_expression
ResolveExpressionExpectingType(symbol_list *out, declaration_list *scope, expression *expr, type_specifier *typespec)
{
    meta_expression result = ResolveExpression(out, scope, expr);
    if(result.type == MetaExpr_Constant &&
       result.cexpr.integer == 0) 
    {
        if((typespec->type == TypeSpec_Internal && typespec->internal_type != Internal_Void) ||
           typespec->type == TypeSpec_Ptr)
        {
            result.typespec = typespec;
        }
        else
        {
            Panic("Can't set something of that type to zero!");
        }
    }
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





//every time you "fixup"/"check" an ast node you have to duplicate it again and update pointer


inline declaration *
FindDeclarationInSymbolList(symbol_list *list, char *identifier)
{
    declaration *result = 0;
    for(u32 i = 0; i < list->count; ++i)
    {
        symbol *sym = list->symbols + i;
        if(sym->decl->identifier == identifier)
        {
            result = sym->decl;
        }
    }
    return result;
}


internal void
CheckTypeSpecifier(symbol_list *out, declaration_list *scope, type_specifier *typespec)
{
    symbol_list *top;
    for(top = out; //NOTE should always append typedecls to the top of the sym_list 
        top->above;
        top = top->above);
    
    if(typespec->type != TypeSpec_Internal)
    {
        if(typespec->flags & TypeSpecFlags_TypeDeclaredAfterUse ||
           !FindDeclarationInSymbolList(top, typespec->identifier))
        {
            declaration *typedecl = FindDeclaration(scope, typespec->identifier);
            AppendResolvedDeclaration(top, scope, typedecl);
            typespec->flags ^= TypeSpecFlags_TypeDeclaredAfterUse;
        }
    }
}

internal void
ResolveStatement(symbol_list *out, declaration_list *scope, statement *stmt)
{
    //some declaration need to go to out, other
    switch(stmt->type)
    {
        case Statement_Compound:
        {
#if 0
            for(statement *stmt = body->compound_stmts;
                stmt;
                stmt = stmt->next)
            {
                ResolveStatement();
            }
#endif 
        }break;
    }
}

internal void
AppendResolvedDeclaration(symbol_list *out, declaration_list *scope, declaration *decl)
{
    Assert(out && decl);
    symbol_list new_list = {0};
    if(decl->resolve == ResolveState_Resolving)
    {
        Panic("Cyclical dependency");
    }
    
    
    if(decl->resolve == ResolveState_NotResolved)
    {
        decl->resolve = ResolveState_Resolving;
        switch(decl->type)
        {
            case Declaration_Struct:
            case Declaration_Union:
            {
                //sym->type == SymbolType_Aggregate;
                int needs_constructor = false;
                //Assert(decl->list.above == scope);
                //TODO in decls store how many members there are to make this allocation tight!
                new_list = MakeSymbolList(16, out);
                for(declaration *member = decl->members;
                    member;
                    member = member->next)
                {
                    //symbol_list is mirroring decl_list
                    //similar to how symbol is mirroring decl
                    AppendResolvedDeclaration(&new_list, &decl->list, member);
                }
                
                if(needs_constructor)
                {
                    
                }
            }break;
            case Declaration_Enum:
            case Declaration_EnumFlags:
            {
                //Assert(decl->list.above == scope);
                new_list = MakeSymbolList(16, out);
                
                //TODO let user specify this as qualifier in enum decl
                type_specifier *enum_member_type = internal_type_s32;
                for(declaration *member = decl->members, *prev = 0;
                    member;
                    prev = member, member = member->next)
                {
                    Assert(member->type == Declaration_EnumMember);
                    Assert(member->actual_expr && member->expr_as_const);
                    if(member->expr_as_const->type == Expression_ToBeDefined)
                    {
                        meta_expression mexpr = ResolveExpression(&new_list, &decl->list, decl->actual_expr);
                        if(mexpr.type == MetaExpr_Constant)
                        {
                            *member->expr_as_const = mexpr.cexpr;
                        }
                        else Panic("Member initializers need to be constant!");
                    }
                    Assert(!member->inferred_type);
                    member->inferred_type = enum_member_type;
                }
            }break;
            
            case Declaration_Variable:
            {
                CheckTypeSpecifier(out, scope, decl->typespec);
                if(decl->initializer)
                {
                    //meta_expression mexpr = ResolveExpression(out, scope, decl->initializer);
                    meta_expression mexpr = ResolveExpressionExpectingType(out, scope, decl->initializer, decl->typespec);
                    if(decl->initializer_as_const && 
                       decl->initializer_as_const->type == Expression_ToBeDefined)
                    {
                        if(mexpr.type == MetaExpr_Constant)
                        {
                            *decl->initializer_as_const = mexpr.cexpr;
                        }
                        else Panic("Expression needs to be constant!");
                    }
                    if(!TypesMatch(decl->typespec, mexpr.typespec))
                    {
                        Panic();
                    }
                }
            }break;
            
            case Declaration_Constant:
            {
                Assert(decl->actual_expr && decl->expr_as_const);
                if(decl->expr_as_const->type == Expression_ToBeDefined)
                {
                    meta_expression mexpr = ResolveExpression(out, scope, decl->actual_expr);
                    if(mexpr.type == MetaExpr_Constant)
                    {
                        Assert(!decl->inferred_type);
                        decl->inferred_type = mexpr.typespec;
                        *decl->expr_as_const = mexpr.cexpr;
                    }
                    else Panic("Expression needs to be constant!");
                }
                if(!decl->inferred_type)
                {
                    meta_expression mexpr = ResolveExpression(out, scope, decl->expr_as_const);
                    decl->inferred_type = mexpr.typespec;
                }
            }break;
            
            
            
            case Declaration_Include:
            case Declaration_Insert:
            {
                Assert(decl->actual_expr && decl->expr_as_const);
                if(decl->expr_as_const->type == Expression_ToBeDefined)
                {
                    meta_expression mexpr = ResolveExpression(out, scope, decl->actual_expr);
                    if(mexpr.type == MetaExpr_Constant)
                    {
                        Assert(mexpr.cexpr.type == Expression_StringLiteral);
                        *decl->expr_as_const = mexpr.cexpr;
                    }
                    else Panic("Member initializers need to be constant!");
                }
            }break;
            
            case Declaration_ProcedureFowardGroup:
            {
                for(declaration *foward = decl->overloaded_list;
                    foward;
                    foward = foward->next)
                {
                    CheckTypeSpecifier(out, scope, foward->foward_return_type);
                    
                    for(declaration *arg = foward->foward_args;
                        arg && arg->type == Declaration_ProcedureArgs; //NOTE this is really confusing
                        arg = arg->next)
                    {
                        CheckTypeSpecifier(out, scope, arg->typespec);
                        
                        if(arg->initializer)
                        {
                            Assert(arg->initializer_as_const);
                            if(arg->initializer_as_const->type == Expression_ToBeDefined)
                            {
                                meta_expression mexpr = ResolveExpression(out, scope, arg->initializer);
                                if(mexpr.type == MetaExpr_Constant)
                                {
                                    *arg->initializer_as_const = mexpr.cexpr;
                                }
                                else Panic(); //optional args need to be constant!
                            }
                        }
                    }
                }
            }break;
            
            case Declaration_Procedure:
            {
                //NOTE don't have to check return types, or arg types, just check the statement
                
                statement *body = decl->proc_body;
                ResolveStatement(out, scope, body);
                
            }break;
            
            //TODO deal with optional args here
        }
        
        
        decl->resolve = ResolveState_Resolved;
        symbol *sym = NewSymbol(out);
        sym->decl = decl;
        if(new_list.symbols) //we resolved an aggregate type with their own symbol list
        {
            sym->type = SymbolType_Aggregate;
            sym->list = new_list;
        }
    }
}






internal void
AST_SymbolList(declaration_list *ast, symbol_list *out)
{
    declaration_list result = {0};
    for(declaration *decl = ast->decls;
        decl;
        decl = decl->next)
    {
        AppendResolvedDeclaration(out, ast, decl);
    }
}


