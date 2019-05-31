typedef struct
{
    declaration_list *scope; //current scope within list we are in
    memory_arena *arena;
    declaration_list list; //resolved top level symbols, final output
    
} resolve_state;

inline int
typespec_is_ref(type_specifier *typespec)
{
    return typespec->type == TypeSpec_Ptr || typespec->type == TypeSpec_Array;
}

inline int
typspec_is_user_defined(type_specifier *typespec)
{
    return typespec->type == TypeSpec_Struct || typespec->type == TypeSpec_Union ||
        typespec->type == TypeSpec_Enum || typespec->type == TypeSpec_EnumFlags;
}

internal void
check_typespec(resolve_state *resolver, declaration_list *scope, type_specifier *typespec)
{
    assert(resolver->list.above == 0);
    if(typespec->type != TypeSpec_Internal) //userdefined type
    {
        
        type_specifier *base = typespec_is_ref(typespec) ? typespec->base_type : typespec;
        
        if(base->flags & TypeSpecFlags_TypeDeclaredAfterUse ||
           !find_decl_in_scope(&resolver->list, base->identifier))
        {
            declaration *typedecl = find_decl(scope, base->identifier);
            assert(typedecl);
            append_resolved_decl(resolver, scope, typedecl);
            //TODO set this to zero regardless of not set before
            typespec->flags ^= TypeSpecFlags_TypeDeclaredAfterUse;
        }
    }
}

typedef enum
{
    MetaExpr_Undefined,
    
    MetaExpr_Constant,
    MetaExpr_lvalue, 
    MetaExpr_rvalue, 
    
    MetaExpr_TempVariable, //Will I need this
    
#if 0
    MetaExpr_Struct,
    MetaExpr_Union,
    MetaExpr_Enum,
    MetaExpr_EnumFlags,
    MetaExpr_EnumMember, 
    MetaExpr_Procedure, 
    
    
    MetaExpr_Include,
    MetaExpr_Insert, 
#endif
    
    
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



internal meta_expression //NOTE this function can be very misleading
resolve_expr(resolve_state *resolver, declaration_list *scope, expression *expr)
{
    meta_expression result = {0};
    if(ExpressionIsConstant(expr))
    {
        result.type = MetaExpr_Constant;
        result.cexpr = *expr;
        //TODO check values to determine appropraite type
        if(expr->type == Expression_Sizeof)
        {
            check_typespec(resolver, scope, expr->typespec);
            //NOTE should be a size_t here
            result.typespec = internal_type_u32;
        } 
        else if(expr->type == Expression_CharLiteral)
        {
            result.typespec = internal_type_s8;
        }
        else if(expr->type == Expression_StringLiteral)
        {
            result.typespec = internal_type_u64; //pointer? or index into strtab?
        }
        else if(expr->type == Expression_Integer)
        {
            result.typespec = internal_type_s32;
        }
        else if(expr->type == Expression_RealNumber)
        {
            result.typespec = internal_type_float;
        }
    }
    else if(expr->type == Expression_Identifier)
    {
        //TODO think about this, this will lead to some wierd behavior
        if(!expr->decl) 
        {
            expr->decl = find_decl(scope);
            if(!expr->decl) panic("identifier never declared");
            
            //NOTE here the scope and declaration don't line up
            //expr->decl may not be directly within the scope
            resolve_and_append(resolver, scope, expr->decl);
        }
    }
    
    
    
    else if(expr->type == Expression_MemberAccess)  {
        meta_expression mexpr = resolve_expr(resolver, scope, expr->struct_expr);
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
            else panic("Member not found in struct!");
        }
        else panic(); //TODO type cast e.g. ((struct*)0)->member => offsetof(struct, member)
    }
    else if(expr->type == Expression_Ternary) 
    {
        meta_expression cond_expr = resolve_expr(out, scope,  expr->tern_cond);
        meta_expression true_expr = resolve_expr(out, scope, expr->tern_true_expr);
        meta_expression false_expr = resolve_expr(out, scope, expr->tern_false_expr);
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
            resolve_expr(out, scope, outer->compound_expr);
        }
        assert(outer);
        result = resolve_expr(out, scope, outer);
    }
    else if(expr->type == Expression_Cast) 
    {
#if 0
        result = resolve_expr(expr->cast_expression);
        if(result.type == MetaExpr_lvalue)
        {
            result.typespec = expr->casting_to;
        }
        //TODO
#endif
        panic();
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
            assert(arg & arg->type == Declaration_ProcedureArgs);
            meta_expression marg = resolve_expr(outer->compound_expr);
            if(marg.type == MetaExpr_rvalue || marg->type == MetaExpr_lvalue)
            {
                
            }
            else if(marg.type == MetaExpr_Constant)
            {
                
            }
            else panic();
        }
        if(outer) resolve_expr(outer);
        
        result = (meta_expression){MetaExpr_rvalue, proc->proc_return_type};
#else
        panic();
#endif
    }
    else if(expr->type == Expression_ArraySubscript) 
    {
        meta_expression array_expr = resolve_expr(out, scope, expr->array_expr);
        if(array_expr.type == MetaExpr_lvalue &&
           array_expr.typespec->type == TypeSpec_Array ||
           array_expr.typespec->type == TypeSpec_Ptr)
        {
            assert(expr->array_actual_index_expr);
            if(!expr->array_index_expr_as_const)
            {
                meta_expression index_expr = resolve_expr(out, scope, expr->array_actual_index_expr);
                if(index_expr.type == MetaExpr_Constant)
                {
                    
                }
                else panic();
            }
        }
        else panic();
        
    }
    else if(expr->type == Expression_CompoundInitializer) 
    {
        //NOTE should this always be constant?
        expression *outer;
        for(outer = expr->call_args;
            outer && outer->type == Expression_Compound;
            outer = outer->compound_next)
        {
            resolve_expr(out, scope, outer->compound_expr);
        }
        if(outer) resolve_expr(out, scope, outer);
    }
    
    else if (expr->type == Expression_Binary)
    {
        result.type = MetaExpr_rvalue;
        meta_expression left = resolve_expr(out, scope, expr->binary_left);
        meta_expression right = resolve_expr(out, scope, expr->binary_right);
        if(MetaReadValue(&left) && MetaReadValue(&right))
        {
            if(expr->binary_type == '%') 
            {
                assert(left.typespec && right.typespec);
                if(TypeIsIntegral(left.typespec) && TypeIsIntegral(right.typespec))
                {
                    result.type = MetaExpr_rvalue;
                    result.typespec = internal_type_u32; //TODO find better one
                }
                else panic();
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
                else panic();
                
                if(expr->binary_type == '/' && right.type == MetaExpr_Constant)
                {
                    if(right.cexpr.type == Expression_Integer &&
                       right.cexpr.integer == 0)
                    {
                        panic("Division by zero");
                    }
                    if(right.cexpr.type == Expression_RealNumber &&
                       right.cexpr.integer == 0)
                    {
                        panic("Division by zero");
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
                else panic();
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
                else panic();
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
                else panic();
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
                    else panic();
                }
                else panic();
            }
        }
        else panic();
    }
    else if (expr->type == Expression_Unary)
    {
        meta_expression mexpr = resolve_expr(out, scope, expr->unary_expr);
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
                else panic();
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
                else panic();
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
                else panic();
            }
            else panic();
        }
        else panic();
    }
    
    else if(expr->type == Expression_ToBeDefined)
    {
        panic();
    }
    
    else panic();
    
    
    return result;
}


inline void
resolver_begin_scope(resolve_state *resolver)
{
    assert(!list->decls && !list->above);
    declaration *list = &resolver->decl->decl_list;
    list->above = resolver->scope;
    resolver->scope = list;
}

inline void
resolver_end_scope(resolver_state *resolver)
{
    resolver->scope = resolve->scope->above;
}

inline declaration *
get_decl(declaration *decl)
{
    assert(decl->resolve == ResolveState_Resolving);
    declaration *result = push_type(resolver->arena, declaration);
    *result = (declaration){0};
    result->type = decl->type;
    result->identifier = decl->identifier;
    result->resolve = decl->resolve;
    return result;
}

inline meta_expression
resolve_initializer(resolve_state *resolver, declaration_list *scope, declaration *decl)
{
    meta_expression result;
    assert(decl->initializer);
    if(decl->initializer_as_const)
    {
        if(decl->initializer_as_const->type == Expression_ToBeDefined)
        {
            result = resolve_expr(resolver scope, decl->initializer);
            if(result.type == MetaExpr_Constant)
            {
                *decl->initializer_as_const = result.cexpr;
            }
            else panic("Not a constant initializer!!!"); 
        }
        else
        {
            result = resolve_expr(resolver scope, decl->initializer_as_const);
        }
    }
    else
    {
        result = resolve_expr(resolver scope, decl->initializer);
        if(result.type == MetaExpr_Constant) //just store it?
        {
            decl->initializer_as_const = NewExpression(0);
            *decl->initializer_as_const = mexpr.cexpr;
        }
    }
    return result;
}

inline void
resolver_append(resolve_state *resolver, declaration *decl)
{
    assert(decl->resolve == ResolveState_Resolved);
    
    decl_list_append(&resolver->list, resolved);
}

inline void
resolver_append_scope(resolve_state *resolver, declaration *decl)
{
    assert(decl->resolve == ResolveState_Resolved);
    declaration *resolved = push_type(resolver->arena, declaration);
    *resolved = *decl;
    decl_list_append(resolver->scope, resolved);
}

internal void //NOTE out may or may not be the top scope
resolve_and_append(resolve_state *resolver, declaration_list *scope, declaration *decl)
{
    assert(out && scope && decl);
    
    if(decl->resolve == ResolveState_Resolving)
    {
        panic("cyclical dependency");
    }
    else if(decl->resolve == ResolveState_Resolved)
    {
        return;
    }
    decl->resolve = ResolveState_Resolving;
    declaration *result = get_decl(resolver, decl);
    
    
    switch(decl->type)
    {
        case Declaration_Struct:
        case Declaration_Union:
        {
            resolver_begin_scope(resolver, &result->list);
            for(declaration *member = decl->members;
                member;
                member = member->next)
            {
                resolve_and_append(resolver, &decl->list, member);
            }
            resolver_end_scope(resolver);
            
            decl->resolve = ResolveState_Resolved;
            result->resolve = ResolveState_Resolved; //NOTE this should really be left unresolved if we use packages
            if(decl->identifier)
                resolver_append(resolver, result);
            else
                resolver_append_scope(resolver, result); //anonymous structs
        }break;
        
        case Declaration_Enum:
        case Declaration_EnumFlags:
        {
            resolver_begin_scope(resolver, &result->list);
            for(declaration *member = decl->members, *prev = 0;
                member;
                prev = member, member = member->next)
            {
                assert(member->type == Declaration_EnumMember);
                resolve_and_append(resolver, member);
            }
            resolver_end_scope(resolver);
            
            decl->resolve = ResolveState_Resolved;
            resolver_append(resolver, decl);
        }break;
        
        case Declaration_EnumMember:
        {
            assert(decl->initializer && member->initializer_as_const);
            type_specifier *enum_member_type = internal_type_s32; //TODO
            if(decl->initializer_as_const->type == Expression_ToBeDefined)
            {
                meta_expression mexpr = resolve_expr(resolver, scope, decl->initializer);
                if(mexpr.type == MetaExpr_Constant)
                {
                    *decl->initializer_as_const = mexpr.cexpr;
                }
                else panic("Member initializers need to be constant!");
            }
            assert(!decl->inferred_type);
            decl->inferred_type = enum_member_type;
            
            decl->resolve = ResolveState_Resolved;
            *result = *decl;
            resolver_append_scope(resolver, result);
        }break;
        
        case Declaration_Variable:
        {
            check_typespec(resolver, scope, decl->typespec);
            if(decl->initializer)
            {
                meta_expression mexpr = resolve_initializer(resolver, scope, decl);
                if(!TypesMatch(decl->typespec, mexpr.typespec))
                {
                    panic();
                }
            }
            
            decl->resolve = ResolveState_Resolved;
            *result = *decl;
            resolver_append_scope(resolver, result);
        }break;
        
        
        case Declaration_Include:
        case Declaration_Insert:
        case Declaration_Constant:
        {
            assert(decl->initializer && decl->initializer_as_const);
            meta_expression mexpr = resolve_const_init(resolver, scope, decl);
            assert(!decl->inferred_type);
            decl->inferred_type = mexpr.typespec;
            if(decl->type == Declaration_Include ||
               decl->type == Declaration_Insert)
            {
                assert(decl->inferred_type == internal_type_u64); //pointer
            }
            
            
            decl->resolve = ResolveState_Resolved;
            *result = *decl;
            resolver_append(resolver, result);
        }break;
        
        case Declaration_ProcedureFowardGroup:
        {
            for(declaration *sign = decl->members;
                sign;
                sign = sign->next)
            {
                //TODO check return expresion matches with decl!
                check_typespec(resolver, scope, sign->return_type);
                for(declaration *arg = sign->args;
                    arg;
                    arg = arg->next)
                {
                    assert(arg->type == Declaration_ProcedureArgs);
                    check_typespec(resolver, scope, arg->typespec);
                    
                    if(arg->initializer)
                    {
                        meta_expression mexpr = resolve_initializer(resolver, scope, arg);
                        if(!TypesMatch(arg->typespec, mexpr.typespec))
                        {
                            panic();
                        }
                    }
                }
            }
            
            resolver_append(resolver, result);
        }break;
        
        case Declaration_Procedure:
        {
            statement *body = decl->proc_body;
            ResolveStatement(out, scope, body);
            resolver_append(resolver, result);
        }break;
    }
}



internal declaration_list
FixedAST(declaration_list *ast, memory_arena *arena)
{
    resolve_state resolver = {0};
    resolver->arena = arena;
    resolver->scope = &resolver->list;
    
    for(declaration *decl = ast->decls;
        decl;
        decl = decl->next)
    {
        resolve_and_append(&resolver, ast, decl);
    }
    return resolver->list;
}