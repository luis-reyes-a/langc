#include "declaration.h"
#include "statement.h"

inline declaration *
NewDeclaration(declaration_type type)
{
    declaration *decl = PushType(&ast_arena, declaration);
    *decl = (declaration){0};
    decl->type = type;
    return decl;
}

internal declaration * //TODO
ParseTypedef(lexer_state *lexer)
{
    Panic();
    return 0;
}

internal int
DeclarationCollidesInList(declaration_list *list, declaration *decl)
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
DeclarationListAppend(declaration_list *list, declaration *decl)
{ 
    //check back poniters are set
    for(declaration *check = decl, *prev = 0;
        check;
        (prev = check, check = check->next))
    {
        if(DeclarationCollidesInList(list, check))
        {
            printf("Declaration name collision");
            Panic();
        }
        if(check != decl && !check->back)
        {
            check->back = prev;
        }
    }
    
    
    if(list->decls)
    {
        Assert(decl);
        Assert(list->decls->back->next == 0);
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


//NOTE if it can't find a type 
//it marks it as Undeclared and points fixup to a fixup error
//the type also stores a refernce to this fixup
internal type_specifier *
ParseTypeSpecifier(lexer_state *lexer, after_parse_fixup **fixup)
{ 
    type_specifier *result = 0;
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        type_specifier *base_type = FindBasicType(lexer->eaten.identifier);
        if(!base_type)
        {
            base_type = AddUnDeclaredType(lexer->eaten.identifier);
            Assert(!*fixup);
            *fixup = NewFixup(Fixup_DeclBeforeDecl);
            base_type->fixup = *fixup;
        }
        result = base_type;
        Assert(result);
        
        u32 num_stars = 0;
        while(WillEatTokenType(lexer, '*'))   ++num_stars;
        if(num_stars)
        {
            type_specifier *ptr_type = FindPointerType(result, num_stars);
            if(!ptr_type)
            {
                ptr_type = AddPointerType(result, num_stars);
            }
            result = ptr_type;
        }
        
        while(WillEatTokenType(lexer, '['))
        {
            if(WillEatTokenType(lexer, ']'))
            {
                SyntaxError(lexer->file, lexer->line_at, "Empty Array!");
            }
            expression *array_size_expr = ParseExpression(lexer);
            expression const_expr = ResolvedExpression(array_size_expr);
            if(const_expr.type == Expression_Integer &&
               const_expr.negative == false)
            {
                type_specifier *array_type = FindArrayType(result, const_expr.integer);
                if(!array_type)
                {
                    array_type = AddArrayType(base_type, result, const_expr.integer);
                }
                array_type->array_size_expr = array_size_expr;
                result = array_type;
                ExpectToken(lexer, ']');
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Array size is not constant!");
            }
        }
        
        if(PeekToken(lexer).type == '*')
        {
            SyntaxError(lexer->file, lexer->line_at, "Can't declare pointers to fixed arrays");
        }
    }
    Assert(result);
    return result;
}


internal declaration *
ParseConstant(lexer_state *lexer, int skip_keyword)
{
    declaration *result = 0;
    
    if(!skip_keyword)
    {
        ExpectKeyword(lexer, Keyword_Let);
    }
    
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result = NewDeclaration(Declaration_Constant);
        result->identifier = lexer->eaten.identifier;
        ExpectToken(lexer, '=');
        result->actual_expr = TryParseTernaryExpression(lexer);
        expression const_expr = ResolvedExpression(result->actual_expr);
        if(const_expr.type) 
        {
            result->expr_as_const = NewExpression(0);
            *result->expr_as_const = const_expr;
        }
        else
        {
            SyntaxError(lexer->file, lexer->line_at, "Constants must be constant!");
        }
    }
    
    declaration **next = &result->next;
    if(PeekToken(lexer).type == ',')
    {
        while(WillEatTokenType(lexer, ','))
        {
            Assert(*next == 0);
            *next = ParseConstant(lexer, 1);
            while(*next) next = &(*next)->next;
        }
        
    }
    else
    {
        ExpectToken(lexer, ';');
    }
    
    return result;
}

internal declaration *
ParseProcedure(lexer_state *lexer, u32 qualifier, declaration_list *scope)
{
    declaration *result = 0;
    
    if(WillEatTokenType(lexer, TokenType_Keyword))
    {
        result = NewDeclaration(Declaration_Procedure);
        result->proc_keyword = lexer->eaten.keyword;
        Assert(result->proc_keyword == Keyword_Internal ||
               result->proc_keyword == Keyword_External ||
               result->proc_keyword == Keyword_Inline ||
               result->proc_keyword == Keyword_NoInline);
        result->proc_return_type = ParseTypeSpecifier(lexer, 0);
        result->identifier = ExpectIdentifier(lexer);
        ExpectToken(lexer, '(');
        result->proc_args = ParseVariable(lexer, 0, Declaration_ProcedureArgs);
        ExpectToken(lexer, ')');
        result->proc_body = ParseStatement(lexer, scope, result);
        
        for(declaration *arg = result->proc_args;
            arg;
            arg = arg->next)
        {
            if(arg->initializer)
            {
                after_parse_fixup *fixup = NewFixup(Fixup_ProcedureOptionalArgs);
                fixup->decl = result;
                break;
            }
        }
        
        //
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Unknown proc return type");
    }
    return result;
}

internal declaration *
ParseInclude(lexer_state *lexer, keyword_type keyword)
{
    declaration *result = 0;
    if(!keyword) 
    {
        ExpectToken(lexer, TokenType_Keyword);
        keyword = lexer->eaten.keyword;
    }
    
    if(keyword == Keyword_Include)
    {
        result = NewDeclaration(Declaration_Include);
    }
    else if(keyword == Keyword_Insert)
    {
        result = NewDeclaration(Declaration_Insert);
    }
    if(result)
    {
        result->actual_expr = TryParseTernaryExpression(lexer);
        expression cexpr = ResolvedExpression(result->actual_expr);
        if(cexpr.type)
        {
            result->expr_as_const = NewExpression(0);
            *result->expr_as_const = cexpr;
        }
        else
        {
            SyntaxError(lexer->file, lexer->line_at, "Include expr must be constant!");
        }
        
        declaration **next = &result->next;
        if(PeekToken(lexer).type == ',')
        {
            while(WillEatTokenType(lexer, ','))
            {
                Assert(*next == 0);
                *next = ParseInclude(lexer, keyword);
                while(*next) next = &(*next)->next;
            }
        }
        else
        {
            ExpectToken(lexer, ';');
        }
    }
    else Panic();
    return result;
}


internal declaration * //NOTE doesn't move pass the comma!
ParseEnumMember(lexer_state *lexer, declaration *last_enum_member, int enum_flags)
{
    declaration *result = 0;
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result = NewDeclaration(Declaration_EnumMember);
        result->identifier = lexer->eaten.identifier;
        if(WillEatTokenType(lexer, '='))
        {
            result->actual_expr = ParseExpression(lexer);
            expression cexpr = ResolvedExpression(result->actual_expr);
            if(cexpr.type)
            {
                result->expr_as_const = NewExpression(0);
                *result->expr_as_const = cexpr;
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Member initializer not constant!");
            }
            
        }
        else
        {
            if(last_enum_member)
            {
                Assert(last_enum_member->expr_as_const->type == Expression_Integer);
                if(enum_flags)
                {
                    //TODO check value is a power of 2
                    result->actual_expr = NewExpressionInteger(last_enum_member->expr_as_const->integer << 1);
                    result->expr_as_const = result->actual_expr;
                }
                else
                {
                    result->actual_expr = NewExpressionInteger(last_enum_member->expr_as_const->integer + 1);
                    result->expr_as_const = result->actual_expr;
                }
                
            }
            else
            {
                if(enum_flags)
                {
                    result->actual_expr = NewExpressionInteger(1);
                    result->expr_as_const = result->actual_expr; //NOTE is this safe?
                }
                else
                {
                    result->actual_expr = expr_zero;
                    result->expr_as_const = expr_zero;
                }
                
            }
        }
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Unnamed Enum Member!");
    }
    return result;
}


internal declaration *
ParseEnum(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    
    ExpectKeyword(lexer, Keyword_Enum);
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result = NewDeclaration(Declaration_Enum); 
        result->identifier = lexer->eaten.identifier;
        result->list.above = scope;
        AddNewEnumType(result->identifier, result);
    }
    else
    {
        //TODO allow untyped enums?
        SyntaxError(lexer->file, lexer->line_at, "Untyped enum!");
    }
    ExpectToken(lexer, '{');
    DeclarationListAppend(&result->list, ParseEnumMember(lexer, 0, false));
    //declaration *current = result->members;
    while(WillEatTokenType(lexer, ';') &&
          PeekToken(lexer).type != '}')
    {
        DeclarationListAppend(&result->list, 
                              ParseEnumMember(lexer, result->list.decls->back, false));
        //current = current->next;
    }
    ExpectToken(lexer, '}');
    return result;
}

internal declaration *
ParseEnumFlags(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    ExpectKeyword(lexer, Keyword_EnumFlags);
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result = NewDeclaration(Declaration_EnumFlags);
        result->identifier = lexer->eaten.identifier;
        result->list.above = scope;
        AddNewEnumType(result->identifier, result);
        
        ExpectToken(lexer, '{');
        DeclarationListAppend(&result->list, ParseEnumMember(lexer, 0, true));
        if(result->list.decls)
        {
            //declaration *current = result->members;
            while(WillEatTokenType(lexer, ';') &&
                  PeekToken(lexer).type != '}')
            {
                DeclarationListAppend(&result->list, ParseEnumMember(lexer, result->list.decls->back, true));
            }
        }
        else
        {
            SyntaxError(lexer->file, lexer->line_at, "Empty enum flags!");
        }
        
        ExpectToken(lexer, '}');
    }
    else
    {
        //TODO allow untyped enums?
        SyntaxError(lexer->file, lexer->line_at, "Untyped enum flags!");
    }
    return result;
}


//This can parse normal var decls but also proc args as specified by decl_type
//Pass in the type to skip parsing the type (if omitted)
internal declaration * 
ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type)
{
    //TODO type inferenced
    declaration *result = NewDeclaration(decl_type);
    after_parse_fixup *fixup = 0;
    if(type)  result->typespec = type;
    else      result->typespec = ParseTypeSpecifier(lexer, &fixup);
    if(!result->typespec)   Panic();
    
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result->identifier = lexer->eaten.identifier;
        
        if(WillEatTokenType(lexer, '='))
        {
            if(PeekToken(lexer).type == '{')
            {
                result->initializer = ParseCompoundInitializerExpression(lexer);
                Assert(result->initializer);
            }
            else if(PeekToken(lexer).type == '#' &&
                    NextToken(lexer).type == '#') //### unint variable!
            {
                EatToken(lexer), EatToken(lexer);
                ExpectToken(lexer, '#');
                result->initializer = 0;
            }
            else //then it's an expression
            {
                //result->initializer = ParseExpression(lexer);
                //NOTE should I just require compound exprs to always be wrapped? (?)
                result->initializer = TryParseAssignmentExpression(lexer);
                Assert(result->initializer);
            }
        }
        else if(result->type == Declaration_Variable) 
        {
            
            if(result->typespec->type == TypeSpec_StructUnion)
            {
                //need to find decl!
                if(result->typespec->needs_constructor)
                {
                    result->initializer = NewExpression(Expression_Call);
                    char *constructor_name = ConcatCStringsIntern(result->typespec->identifier, "__Constructor");
                    result->initializer->call_expr = NewExpressionIdentifier(constructor_name);
                    result->initializer->call_args = 0;
                }
                else result->initializer = expr_zero_struct;
            }
            else if (result->typespec->type == TypeSpec_Array)
            {
                result->initializer = expr_zero_struct; //NOTE default to zero init!
            }
            else if(result->typespec->type == TypeSpec_UnDeclared)
            {
                //...what do we do...add a fixup to this?
                after_parse_fixup *fixup_default_init = NewFixup(Fixup_VarMayNeedConstructor);
                fixup_default_init->decl = result;
                result->initializer = NewExpression(Expression_ToBeDefined);
            }
            else
            {
                result->initializer = expr_zero; //NOTE default to zero init!
            }
        }
        if(WillEatTokenType(lexer, ',')) 
        {
            if(PeekToken(lexer).type == TokenType_Identifier &&
               (NextToken(lexer).type == ',' ||
                NextToken(lexer).type == ';' ||
                NextToken(lexer).type == '=' ||
                NextToken(lexer).type == ')'))
            {
                result->next = ParseVariable(lexer, result->typespec, decl_type);
            }
            else
            {
                
                result->next = ParseVariable(lexer, 0, decl_type);
            }
        }
        else if(decl_type == Declaration_Variable)
        {
            ExpectToken(lexer, ';');
        }
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Unnamed variable!");
    }
    
    if(fixup)
    {
        fixup->first_decl = result;
        fixup->decl_after_decl = 0; 
    }
    return result;
}

inline declaration *
ParseStructMember(lexer_state *lexer, declaration *struct_decl)
{
    declaration *member = 0;
    
    if(PeekToken(lexer).type == TokenType_Keyword)
    {
        switch(lexer->peek.keyword)
        {
            case Keyword_Struct:
            case Keyword_Union:
            member = ParseStructUnion(lexer, &struct_decl->list); //think about this
            Assert(member->identifier == 0);
            break;
            case Keyword_Let:
            member = ParseConstant(lexer, 0);
            break;
            default:
            Panic();
            
            //TODO more fixups over here?
        }
    }
    else if(lexer->peek.type == TokenType_Identifier)
    {
        member = ParseVariable(lexer, 0, Declaration_Variable);
        if(member->typespec->type == TypeSpec_UnDeclared)
        {
            member->typespec->fixup->first_decl = struct_decl;
        }
    }
    else
    {
        Panic();
    }
    
    return member;
}

internal bool32
StructNeedsConstructor(declaration *decl)
{
    Assert(decl->type == Declaration_Struct ||
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
            if(StructNeedsConstructor(member))
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
ParseStructUnion(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    if(WillEatTokenType(lexer, TokenType_Keyword) &&
       (lexer->eaten.keyword == Keyword_Struct ||
        lexer->eaten.keyword == Keyword_Union))
    {
        switch(lexer->eaten.keyword){
            case Keyword_Struct: result = NewDeclaration(Declaration_Struct); break;
            case Keyword_Union : result = NewDeclaration(Declaration_Union); break;
            default: Panic();
        }
        
        //Struct name, add it to the type table
        type_specifier *this_type = 0;
        if(WillEatTokenType(lexer, TokenType_Identifier))
        {
            result->identifier = lexer->eaten.identifier;
            this_type = AddNewStructUnionType(result->identifier, result);
        }
        result->list.above = scope;
        //else anonymous struct/union
        
        ExpectToken(lexer, '{');
        while(PeekToken(lexer).type != '}')
        {
            DeclarationListAppend(&result->list, ParseStructMember(lexer, result));
        }
        ExpectToken(lexer, '}');
        
        if(!result->list.decls)
        {
            SyntaxError(lexer->file, lexer->line_at, "Empty Struct/Union!");
        }
        
        
        //TODO better way to store information like this
        //NOTE I could maybe make some sort of separate data structure that just stores fix up information
        //instead of embedding that info in every struct, it also makes it cleaner!
        if(StructNeedsConstructor(result))
        {
            //return if all set to zero so that 
            after_parse_fixup *fixup = NewFixup(Fixup_StructNeedsConstructor);
            fixup->struct_with_no_constructor = result;
#if 1
            Assert(this_type);
            {
                this_type->needs_constructor = true;
            }
#endif
            
        }
        
        
        //check for struct instances and parse them
        if(PeekToken(lexer).type == TokenType_Identifier &&
           (NextToken(lexer).type == ',' || NextToken(lexer).type == ';'))
        {
            Assert(this_type); 
            result->next = ParseVariable(lexer, this_type, Declaration_Variable);
        }
        
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Incorrect struct syntax!");
    }
    
    return result;
}

internal declaration *
ParseForeignDeclaration(lexer_state *lexer)
{
    declaration *result = 0;
    if(WillEatKeyword(lexer, Keyword_Struct))
    {
        result = NewDeclaration(Declaration_Struct);
        result->identifier = ExpectIdentifier(lexer);
    }
    else if(WillEatKeyword(lexer, Keyword_Union))
    {
        result = NewDeclaration(Declaration_Union);
        result->identifier = ExpectIdentifier(lexer);
    }
    else if(WillEatKeyword(lexer, Keyword_Enum))
    {
        result = NewDeclaration(Declaration_Enum);
        result->identifier = ExpectIdentifier(lexer);
    }
    else if(WillEatKeyword(lexer, Keyword_Define))
    {
        char *identifier = ExpectIdentifier(lexer);
        if(WillEatTokenType(lexer, '('))
        {
            //NOTE ignore args for now
            while(EatToken(lexer).type != ')');
            result = NewDeclaration(Declaration_MacroArgs);
            result->identifier = identifier;
        }
        else
        {
            result = NewDeclaration(Declaration_Macro);
            result->identifier = identifier;
        }
    }
    else //it's a procedure or variable
    {
        type_specifier *type = ParseTypeSpecifier(lexer, 0);
        char *identifier = ExpectIdentifier(lexer);
        if(WillEatTokenType(lexer, '('))
        {
            result = NewDeclaration(Declaration_Procedure);
            result->proc_return_type = type;
            result->identifier = identifier;
            result->proc_keyword = Keyword_Internal;
            result->proc_body = 0;
            result->proc_args = ParseVariable(lexer, 0, Declaration_ProcedureArgs);
            ExpectToken(lexer, ')');
        }
        else
        {
            result = NewDeclaration(Declaration_Variable);
            result->typespec = type;
            result->identifier = identifier;
        }
    }
    ExpectToken(lexer, ';');
    return result;
}

internal declaration *
ParseForeignBlock(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = NewDeclaration(Declaration_ForeignBlock);
    ExpectKeyword(lexer, Keyword_Foreign);
    result->list.above = scope;
    ExpectToken(lexer, '{');
    DeclarationListAppend(&result->list, ParseForeignDeclaration(lexer));
    while(PeekToken(lexer).type != '}')
    {
        DeclarationListAppend(&result->list, ParseForeignDeclaration(lexer));
    }
    EatToken(lexer);
    return result;
}


//
//NOTE if it's convenient this may pass multiple decls through the *next pointer!!!
//
internal declaration *
ParseDeclaration(lexer_state *lexer, declaration_list *scope)
{
    declaration *result = 0;
    lexer_token token = PeekToken(lexer);
    if(token.type == TokenType_Keyword)
    {
        switch(token.keyword)
        {
            case Keyword_Typedef:
            Panic();
            result = ParseTypedef(lexer);
            break;
            case Keyword_Struct: case Keyword_Union: 
            result = ParseStructUnion(lexer, scope);
            break;
            case Keyword_Enum:
            result = ParseEnum(lexer, scope);
            break;
            case Keyword_EnumFlags:
            result = ParseEnumFlags(lexer, scope);
            break;
            case Keyword_Let:
            result = ParseConstant(lexer, 0);
            break;
            case Keyword_Include: case Keyword_Insert:
            result = ParseInclude(lexer, 0);
            break;
            case Keyword_Foreign:
            result = ParseForeignBlock(lexer, scope);
            break;
            case Keyword_Internal: case Keyword_External:
            case Keyword_Inline: case Keyword_NoInline:
            result = ParseProcedure(lexer, token.keyword, scope);
            break;
            default: Panic();
        }
    }
    else if(token.type == TokenType_Identifier)
    {
        result = ParseVariable(lexer, 0, Declaration_Variable);
    }
    else if (token.type == TokenType_EndOfStream)
    {
        result = 0;
    }
    else
    {
        Assert(token.type != TokenType_Invalid);
        SyntaxError(lexer->file, lexer->line_at, "Can't parse this!");
    }
    return result;
}








