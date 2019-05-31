#include "declaration.h"
#include "statement.h"

inline declaration *
new_decl(declaration_type type)
{
    declaration *decl = PushType(&ast_arena, declaration);
    *decl = (declaration){0};
    decl->type = type;
    return decl;
}

internal declaration * //TODO
parse_typedef(lexer_state *lexer)
{
    panic();
    return 0;
}

internal int
decl_collides_in_list(declaration_list *list, declaration *decl)
{
    int result = false;
    for(declaration *check = list->decls;
        check;
        check = check->next)
    { 
        if(decl != check &&
           decl->identifier == check->identifier)
        {
            result = true;
        }
    }
    return result;
}

internal void //Appends to the end of the list, checks for collision and back pointers set
decl_list_append(declaration_list *list, declaration *decl)
{ 
    //check back poniters are set if the passed in decl is a linked list
    if(decl->next)
    {
        for(declaration *check = decl, *prev = 0;
            check;
            (prev = check, check = check->next))
        {
            if(check->identifier &&
               decl_collides_in_list(list, check))
            {
                panic();
            }
            if(check != decl && !check->back)
            {
                check->back = prev;
            }
        }
    }
    
    
    //NOTE should we check that there isn't a copy in the list?
    if(list->decls)
    {
        assert(decl);
        assert(list->decls->back->next == 0);
        declaration *prev_last = list->decls->back;
        prev_last->next = decl;
        decl->back = prev_last;
        while(decl->next) //NOTE this finds actual last in decl pointer!
        {
            decl = decl->next;
        }
        list->decls->back = decl;
    }
    else
    {
        list->decls = decl;
        while(decl->next)
        {
            decl = decl->next;
        }
        list->decls->back = decl;
    }
}


internal type_specifier * //searches in typetable. if fails, returns 'undeclared' type
parse_typespec(lexer_state *lexer, declaration_list *scope)
{ 
    type_specifier *result = 0;
    if(eats_token(lexer, TokenType_Identifier))
    {
        result = find_type(lexer->eaten.identifier);
        if(!result) result = make_undecl_type(lexer->eaten.identifier);
        
        for(u32 count = 1; eats_token(lexer, '*'); ++count)
        {
            type_specifier *ptrtype = find_ptr_type(result, count);
            if(!ptrtype) ptrtype = make_ptr_type(result, count);
            result = ptrtype;
        }
        
        while(eats_token(lexer, '['))
        {
            if(eats_token(lexer, ']'))
            {
                SyntaxError(lexer->file, lexer->line_at, "Empty Array!");
            }
            expression *array_size_expr = parse_expr(lexer, scope);
            expression const_expr = TryResolveExpression(array_size_expr);
            if(const_expr.type == Expression_Integer &&
               const_expr.negative == false)
            {
                type_specifier *arrtype = find_array_type(result, const_expr.integer);
                if (!arrtype) arrtype = make_array_type(result, const_expr.integer, array_size_expr);
                result = arrtype;
                expect_token(lexer, ']');
            }
            else
            {
                //TODO allow this
                SyntaxError(lexer->file, lexer->line_at, "Array size is not constant!");
            }
        }
        
        if(PeekToken(lexer).type == '*')
        {
            SyntaxError(lexer->file, lexer->line_at, "Can't declare pointers to fixed arrays");
        }
    }
    assert(result);
    return result;
}




internal declaration * //looks just within scope, doesn't look above
find_decl_in_scope(declaration_list *list, char *identifier)
{
    assert(list && identifier);
    declaration *result = 0;
    assert(list && identifier);
    for(declaration *decl = list->decls;
        decl;
        decl = decl->next)
    {
        if(decl->identifier == identifier)
        {
            result = decl;
            break;
        }
    }
    return result;
}

internal declaration * //searches everywhere
find_decl(declaration_list *list, char *identifier)
{
    declaration *result = 0;
    for(declaration_list *scope = list;
        scope;
        scope = scope->above)
    {
        result = find_decl_in_scope(scope, identifier);
        if(result) break;
    }
    return result;
}

inline declaration *
find_proc_group(declaration_list *list, char *identifier)
{
    declaration *result = find_decl(list, identifier);
    if(result)
    {
        assert(result->type == Declaration_ProcedureGroup);
    }
    return result;
}

internal declaration * //NOTE this makes a 'unique' name for each overload
parse_proc_signature(lexer_state *lexer, declaration_list *scope, char **identifier)
{
    declaration *result = 0;
    if(eats_token(lexer, TokenType_Keyword) &&
       (lexer->eaten.keyword == Keyword_Internal ||
        lexer->eaten.keyword == Keyword_External ||
        lexer->eaten.keyword == Keyword_Inline ||
        lexer->eaten.keyword == Keyword_NoInline))
    {
        result = new_decl(Declaration_ProcedureSignature);
        result->keyword = lexer->eaten.keyword;
        result->return_type = parse_typespec(lexer, 0);
        result->identifier = expect_identifier(lexer);
        assert(identifier);
        *identifier = result->identifier;
        
        expect_token(lexer, '(');
        result->args = parse_proc_args(lexer, 0, scope);
        expect_token(lexer, ')');
        
        for(declaration *arg = result->args;
            arg;
            arg = arg->next)
        {
            assert(arg->type == Declaration_ProcedureArgs);
            if(arg->typespec->type == TypeSpec_Ptr)
            {
                result->identifier = ConcatCStringsIntern(result->identifier, arg->typespec->base_type->identifier);
                for(u32 i = 0; i < arg->typespec->size; ++i)
                {
                    result->identifier = ConcatCStringsIntern(result->identifier, "p");
                }
            }
            else if(arg->typespec->type == TypeSpec_Array)
            {
                result->identifier = ConcatCStringsIntern(result->identifier, arg->typespec->base_type->identifier);
                panic("TODO");
                for(u32 i = 0; i < arg->typespec->size; ++i)
                {
                    //NOTE but with arrays bigger than 255 will have a bug
                    //result->identifier = ConcatCStringsIntern(result->identifier, );
                }
            }
            else
            {
                result->identifier = ConcatCStringsIntern(result->identifier, arg->typespec->identifier);
            }
            ++result->num_args;
        }
    } else panic();
    return result;
}


internal declaration *
parse_proc(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    char *identifier;
    declaration *signature = parse_proc_signature(lexer, scope, &identifier);
    if(signature)
    {
        result = new_decl(Declaration_Procedure);
        result->identifier = signature->identifier; 
        result->signature = signature;
        result->proc_body = parse_compound_stmt(lexer, scope);
        
        declaration *group = find_proc_group(scope, identifier);
        if(!group)
        {
            group = new_decl(Declaration_ProcedureGroup);
            group->identifier = identifier;
            group->above = scope;
            assert(!scope->above);
            decl_list_append(scope, group);
        }
        
        //NOTE this will clash if names collide
        decl_list_append(&group->list, signature);
    }
    return result;
}



internal expression *
get_const_initializer(expression *initializer)
{
    assert(initializer);
    expression *result = 0;
    expression const_expr = TryResolveExpression(initializer);
    if(const_expr.type) 
    {
        result = DuplicateExpression(&const_expr);
    }
    else //NOTE
    {
        result = NewExpression(Expression_ToBeDefined);
    }
    return result;
}

internal declaration *
parse_constant(lexer_state *lexer, int skip_keyword, declaration_list *scope)
{
    declaration *result = 0;
    if(!skip_keyword)  expect_keyword(lexer, Keyword_Let);
    
    result = new_decl(Declaration_Constant);
    result->identifier = expect_identifier(lexer);
    expect_token(lexer, '=');
    result->initializer = TryParseTernaryExpression(lexer, scope);
    result->initializer_as_const = get_const_initializer(result->initializer);
    
    if(PeekToken(lexer).type == ',')
    {
        result->next = parse_constant(lexer, 1, scope);
    }
    else
    {
        expect_token(lexer, ';');
    }
    
    return result;
}

internal declaration *
parse_include(lexer_state *lexer, keyword_type keyword, declaration_list *scope)
{
    declaration *result = 0;
    if(!keyword) 
    {
        expect_token(lexer, TokenType_Keyword);
        keyword = lexer->eaten.keyword;
    }
    
    switch(keyword) {
        case Keyword_Include: result = new_decl(Declaration_Include); break;
        case Keyword_Insert: result = new_decl(Declaration_Insert); break;
        default: panic();
    }
    
    
    if(result)
    {
        result->initializer = TryParseTernaryExpression(lexer, scope);
        result->initializer_as_const = get_const_initializer(result->initializer);
        
        if(PeekToken(lexer).type == ',')
        {
            result->next = parse_include(lexer, keyword, scope);
        }
        else
        {
            expect_token(lexer, ';');
        }
    }
    else panic();
    return result;
}

internal declaration *
parse_flag_member(lexer_state *lexer, declaration *prev, declaration_list *scope)
{
    declaration *result = 0;
    if(eats_token(lexer, TokenType_Identifier))
    {
        result = new_decl(Declaration_EnumMember);
        result->identifier = lexer->eaten.identifier;
        if(eats_token(lexer, '='))
        {
            result->initializer = parse_expr(lexer, scope);
            result->initializer_as_const = get_const_initializer(result->initializer);
        }
        else
        {
            if(prev && prev->initializer_as_const->type == Expression_ToBeDefined)
            {
                result->initializer = NewBinaryExpression(Binary_LeftShift, 
                                                          prev->initializer, 
                                                          NewExpressionInteger(1));
                result->initializer_as_const = NewExpression(Expression_ToBeDefined);
            }
            else if(prev) //can compute value
            {
                result->initializer = 
                    NewExpressionInteger(prev->initializer_as_const->integer << 1);
                result->initializer_as_const = result->initializer;
            }
            else //first in enum
            {
                result->initializer_as_const = NewExpressionInteger(1);
                result->initializer = result->initializer_as_const;
            }
            
        }
    }
    else panic();
    return result;
}

internal declaration *
parse_enum_member(lexer_state *lexer, declaration *prev, declaration_list *scope)
{
    declaration *result = 0;
    if(eats_token(lexer, TokenType_Identifier))
    {
        result = new_decl(Declaration_EnumMember);
        result->identifier = lexer->eaten.identifier;
        if(eats_token(lexer, '='))
        {
            result->initializer = parse_expr(lexer, scope);
            result->initializer_as_const = get_const_initializer(result->initializer);
        }
        else
        {
            if(prev && prev->initializer_as_const->type == Expression_ToBeDefined)
            {
                result->initializer = NewBinaryExpression(Binary_Add, 
                                                          prev->initializer, 
                                                          NewExpressionInteger(1));
                result->initializer_as_const = NewExpression(Expression_ToBeDefined);
            }
            else if(prev)
            {
                result->initializer = 
                    NewExpressionInteger(prev->initializer_as_const->integer + 1);
                result->initializer_as_const = result->initializer;
            }
            else
            {
                result->initializer_as_const = NewExpressionInteger(0);
                result->initializer = result->initializer_as_const;
            }
        }
    }
    else panic();
    return result;
}

internal declaration *
parse_enum(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    
    expect_keyword(lexer, Keyword_Enum);
    result = new_decl(Declaration_Enum); 
    result->list.above = scope;
    result->identifier = expect_identifier(lexer);
    
    expect_token(lexer, '{');
    decl_list_append(&result->list, parse_enum_member(lexer, 0, scope));
    declaration *last = asserted(result->members);
    while(eats_token(lexer, ';') &&
          PeekToken(lexer).type != '}')
    {
        decl_list_append(&result->list, parse_enum_member(lexer, last, scope));
        last = last->next;
    }
    expect_token(lexer, '}');
    return result;
}

internal declaration *
parse_enum_flags(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    
    expect_keyword(lexer, Keyword_EnumFlags);
    result = new_decl(Declaration_EnumFlags); 
    result->list.above = scope;
    result->identifier = expect_identifier(lexer);
    
    expect_token(lexer, '{');
    decl_list_append(&result->list, parse_flag_member(lexer, 0, scope));
    declaration *last = asserted(result->members);
    while(eats_token(lexer, ';') &&
          PeekToken(lexer).type != '}')
    {
        decl_list_append(&result->list, parse_flag_member(lexer, last, scope));
        last = last->next;
    }
    expect_token(lexer, '}');
    return result;
}

internal declaration * 
parse_var(lexer_state *lexer, type_specifier *typespec, declaration_list *scope)
{
    declaration *result = 0;
    if(!typespec) typespec = parse_typespec(lexer, scope);
    if(typespec)
    {
        result = new_decl(Declaration_Variable);
        result->identifier = expect_identifier(lexer);
        result->typespec = typespec;
        
        if(eats_token(lexer, '='))
        {
            if(eats_token(lexer, TokenType_UnInitialized))
            {
                result->initializer = 0;
            }
            else
            {
                result->initializer = parse_expr(lexer, scope);
            }
        }
        else //default init values
        {
            if(result->typespec->type == TypeSpec_Struct ||
               result->typespec->type == TypeSpec_Union)
            {
                result->initializer = NewExpression(Expression_ToBeDefined);
                result->initializer_as_const = result->initializer;
            }
            else if (result->typespec->type == TypeSpec_Array)
            {
                result->initializer = expr_zero_struct; 
            }
            else if(result->typespec->type == TypeSpec_UnDeclared)
            {
                result->initializer = NewExpression(Expression_ToBeDefined);
            }
            else
            {
                result->initializer = expr_zero; //NOTE default to zero init!
                result->initializer_as_const = expr_zero;
            }
        }
        
        if(eats_token(lexer, ',')) 
        {
            if(PeekToken(lexer).type == TokenType_Identifier &&
               (NextToken(lexer).type == ',' ||
                NextToken(lexer).type == ';' ||
                NextToken(lexer).type == '='))
            {
                result->next = parse_var(lexer, result->typespec, scope);
            }
            else
            {
                result->next = parse_var(lexer, 0, scope);
            }
        }
        else
        {
            expect_token(lexer, ';');
        }
    }
    return result;
}


internal declaration *
parse_proc_args(lexer_state *lexer, type_specifier *typespec, declaration_list *scope)
{
    declaration *result = 0;
    if(!typespec) typespec = parse_typespec(lexer, scope);
    if(typespec)
    {
        result = new_decl(Declaration_ProcedureArgs);
        result->identifier = expect_identifier(lexer);
        result->typespec = typespec;
        
        if(eats_token(lexer, '='))
        {
            if(eats_token(lexer, TokenType_UnInitialized))
            {
                panic("Why would you want this?");
            }
            result->initializer = parse_expr(lexer, scope);
            result->initializer_as_const = get_const_initializer(result->initializer);
        }
        if(eats_token(lexer, ','))
        {
            if(PeekToken(lexer).type == TokenType_Identifier &&
               (NextToken(lexer).type == ',' ||
                NextToken(lexer).type == ';' ||
                NextToken(lexer).type == '=' ||
                NextToken(lexer).type == ')'))
            {
                result->next = parse_proc_args(lexer, typespec, scope);
            }
            else
            {
                result->next = parse_proc_args(lexer, 0, scope);
            }
        }
    }
    else panic();
    return result;
}

inline declaration *
parse_struct_member(lexer_state *lexer, declaration_list *scope)
{
    declaration *member = parse_decl(lexer, scope);
    if(member->type == Declaration_Struct ||
       member->type == Declaration_Union)
    {
        assert(!member->identifier);
    }
    else if(member->type == Declaration_Variable)
    {
        for(declaration *mdecl = member;
            mdecl;
            mdecl = mdecl->next)
        {
            if(mdecl->initializer && !mdecl->initializer_as_const)
            {
                mdecl->initializer_as_const = get_const_initializer(mdecl->initializer);
            }
        }
    }
    else if(member->type == Declaration_Constant) /*okay*/;
    else
    {
        panic("you can't declare that inside a struct!");
    }
    return member;
}

internal bool32
struct_needs_constructor(declaration *decl)
{
    assert(decl->type == Declaration_Struct ||
           decl->type == Declaration_Union);
    bool32 result = false;
    for(declaration *member = decl->list.decls;
        member;
        member = member->next)
    {
        if(member->type == Declaration_Variable &&
           member->initializer)
        {
            result = true;
            break;
        }
        else if (member->type == Declaration_Struct ||
                 member->type == Declaration_Union)
        {
            if(struct_needs_constructor(member))
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

//do I still need the list
//NOTE the list is only for functions that need pointer to global decls



internal declaration *
parse_struct_union(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    if(eats_token(lexer, TokenType_Keyword) &&
       (lexer->eaten.keyword == Keyword_Struct ||
        lexer->eaten.keyword == Keyword_Union))
    {
        switch(lexer->eaten.keyword){
            case Keyword_Struct: result = new_decl(Declaration_Struct); break;
            case Keyword_Union : result = new_decl(Declaration_Union); break;
            default: panic();
        }
        result->list.above = scope;
        
        type_specifier *type = 0;
        if(eats_token(lexer, TokenType_Identifier))
        {
            result->identifier = lexer->eaten.identifier;
            type = add_new_type(result->identifier, result->type);
        }
        
        expect_token(lexer, '{');
        while(PeekToken(lexer).type != '}')
        {
            decl_list_append(&result->list, parse_struct_member(lexer, &result->list));
        }
        expect_token(lexer, '}');
        
        if(!result->list.decls)
        {
            SyntaxError(lexer->file, lexer->line_at, "Empty Struct/Union!");
        }
        
        if (type) type->needs_constructor = struct_needs_constructor(result);
        
        if(PeekToken(lexer).type == TokenType_Identifier &&
           (NextToken(lexer).type == ',' || NextToken(lexer).type == ';'))
        {
            assert(type); //TODO allow this 
            result->next = parse_var(lexer, type, scope);
        }
        
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Incorrect struct syntax!");
    }
    
    return result;
}

internal declaration *
parse_foreign_decl(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    if(eats_keyword(lexer, Keyword_Struct))
    {
        result = new_decl(Declaration_Struct);
        result->identifier = expect_identifier(lexer);
    }
    else if(eats_keyword(lexer, Keyword_Union))
    {
        result = new_decl(Declaration_Union);
        result->identifier = expect_identifier(lexer);
    }
    else if(eats_keyword(lexer, Keyword_Enum))
    {
        result = new_decl(Declaration_Enum);
        result->identifier = expect_identifier(lexer);
    }
    else if(eats_keyword(lexer, Keyword_Define))
    {
        char *identifier = expect_identifier(lexer);
        if(eats_token(lexer, '('))
        {
            //NOTE ignore args for now
            while(EatToken(lexer).type != ')');
            result = new_decl(Declaration_MacroArgs);
            result->identifier = identifier;
        }
        else
        {
            result = new_decl(Declaration_Macro);
            result->identifier = identifier;
        }
    }
    else //it's a procedure or variable
    {
        
        //TODO make this more robust
        result = parse_proc_args(lexer, 0, scope);
        if(!result)
        {
            char *dumb;
            result = parse_proc_signature(lexer, scope, &dumb);
        }
        assert(result);
    }
    expect_token(lexer, ';');
    return result;
}

internal declaration *
parse_foreign_block(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = new_decl(Declaration_ForeignBlock);
    expect_keyword(lexer, Keyword_Foreign);
    result->list.above = scope;
    expect_token(lexer, '{');
    decl_list_append(&result->list, parse_foreign_decl(lexer, scope));
    while(PeekToken(lexer).type != '}')
    {
        decl_list_append(&result->list, parse_foreign_decl(lexer, scope));
    }
    EatToken(lexer);
    return result;
}


//
//NOTE if it's convenient this may pass multiple decls through the *next pointer!!!
//
internal declaration *
parse_decl(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    lexer_token token = PeekToken(lexer);
    if(token.type == TokenType_Keyword)
    {
        switch(token.keyword)
        {
            case Keyword_Typedef:
            panic();
            result = parse_typedef(lexer);
            break;
            case Keyword_Struct: case Keyword_Union: 
            result = parse_struct_union(lexer, scope);
            break;
            case Keyword_Enum:
            result = parse_enum(lexer, scope);
            break;
            case Keyword_EnumFlags:
            result = parse_enum_flags(lexer, scope);
            break;
            case Keyword_Let:
            result = parse_constant(lexer, 0, scope);
            break;
            case Keyword_Include: case Keyword_Insert:
            result = parse_include(lexer, 0, scope);
            break;
            case Keyword_Foreign:
            result = parse_foreign_block(lexer, scope);
            break;
            case Keyword_Internal: case Keyword_External:
            case Keyword_Inline: case Keyword_NoInline:
            //NOTE this means don't append, we did that ourselves
            result = parse_proc(lexer, scope);
            break;
            default: panic();
        }
    }
    else if(token.type == TokenType_Identifier)
    {
        result = parse_var(lexer, 0, scope);
    }
    else if (token.type == TokenType_EndOfStream)
    {
        result = 0;
    }
    else
    {
        assert(token.type != TokenType_Invalid);
        SyntaxError(lexer->file, lexer->line_at, "Can't parse this!");
    }
    return result;
}








#if 0
inline declaration *
FowardDeclarationForProcedure(declaration *proc)
{
    assert(proc->type == Declaration_Procedure);
    declaration *result = new_decl(Declaration_ProcedureFoward);
    
    //TODO how should i uniquely name these guys
    result->identifier = proc->identifier;
    result->foward_return_type = proc->proc_return_type;
    result->foward_keyword = proc->proc_keyword;
    result->foward_args = PROC_ARGS(proc);
    return result;
    
    internal declaration *
        ActuallyParseProcedure(lexer_state *lexer, declaration_list *scope_declared_in)
    {
        declaration *result = 0;
        expect_token(lexer, TokenType_Keyword);
        if(lexer->eaten.keyword == Keyword_Internal || lexer->eaten.keyword == Keyword_External ||
           lexer->eaten.keyword == Keyword_Inline || lexer->eaten.keyword == Keyword_NoInline)
        {
            result = new_decl(Declaration_Procedure);
            result->proc_keyword = lexer->eaten.keyword;
            result->proc_return_type = parse_typespec(lexer, 0);
            result->identifier = expect_identifier(lexer);
            expect_token(lexer, '(');
            declaration *args = ParseVariable(lexer, 0, Declaration_ProcedureArgs, scope_declared_in);
            expect_token(lexer, ')');
            
            result->proc_body = NewStatement(Statement_Compound);
            DeclarationListAppend(&result->proc_body->decl_list, args);
            
            //we used to pass in result but we need to pass in the header function
            //TODO think about this
            ParseCompoundStatement(lexer, result->proc_body, scope_declared_in, result);
            
            for(declaration *arg = PROC_ARGS(result);
                arg && arg->type == Declaration_ProcedureArgs;
                arg = arg->next)
            {
                if(arg->initializer)
                {
                    after_parse_fixup *fixup = NewFixup(Fixup_ProcedureOptionalArgs);
                    fixup->decl = result;
                    break;
                }
            }
        }
        else panic();
        return result;
    }
}

internal declaration *
parse_proc(lexer_state *lexer, declaration_list *scope)
{
    declaration *proc = 0;
    if(PeekToken(lexer).type == TokenType_Keyword)
    {
        u32 keyword = lexer->peek.keyword;
        if(keyword == Keyword_Internal || keyword == Keyword_External ||
           keyword == Keyword_Inline || keyword == Keyword_NoInline)
        {
            proc = ActuallyParseProcedure(lexer, scope);
            declaration *group = FindProcedureHeader(scope, proc->identifier);
            if(!group)
            {
                group = new_decl(Declaration_ProcedureFowardGroup);
                group->identifier = ConcatCStringsIntern(proc->identifier,"__Header");
                //group->overloaded_list = proc;
                group->overloaded_list = FowardDeclarationForProcedure(proc);
                group->last_overloaded_list = group->overloaded_list;
                
                DeclarationListAppend(scope, group);
            }
            else
            {
                assert(group->last_overloaded_list);
                //group->last_overloaded_list->next = proc;
                group->last_overloaded_list->next = FowardDeclarationForProcedure(proc);
                group->last_overloaded_list->next->back = group->last_overloaded_list;
                group->last_overloaded_list = group->last_overloaded_list->next;
                assert(!group->last_overloaded_list->next);
            }
        }
        else panic();
    }
    else panic();
    return proc;
}

#endif