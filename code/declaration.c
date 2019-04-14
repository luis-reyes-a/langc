#include "declaration.h"

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

//types of style char *[4] StringTable;
internal type_specifier *
ParseTypeSpecifier(lexer_state *lexer)
{ 
    type_specifier *result = 0;
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        type_specifier *base_type = FindBasicType(lexer->eaten.identifier);
        result = base_type;
        if(!result)   Panic();
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
#if 1
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
#else
            else if(WillEatTokenType(lexer, TokenType_Integer)) //TODO parse expression!
            {
                type_specifier *array_type = FindArrayType(result, lexer->eaten.integer);
                if(!array_type)
                {
                    array_type = AddArrayType(base_type, result, lexer->eaten.integer);
                }
                result = array_type;
                ExpectToken(lexer, ']');
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Invalid Array integral expression!");
            }
#endif
        }
        
        if(PeekToken(lexer).type == '*')
        {
            //
            SyntaxError(lexer->file, lexer->line_at, "Can't declare pointers to fixed arrays");
        }
    }
    return result;
}

internal declaration *
ParseProcedure(lexer_state *lexer, u32 qualifier)
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
        result->proc_return_type = ParseTypeSpecifier(lexer);
        result->identifier = ExpectIdentifier(lexer);
        ExpectToken(lexer, '(');
        result->proc_args = ParseVariable(lexer, 0, Declaration_ProcedureArgs);
        ExpectToken(lexer, ')');
        result->proc_body = ParseStatement(lexer);
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Unknown proc return type");
    }
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
            result->const_expr = ParseExpression(lexer);
        }
        else
        {
            if(last_enum_member)
            {
                Assert(last_enum_member->const_expr->type == Expression_Integer);
                if(enum_flags)
                {
                    //TODO check value is a power of 2
                    result->const_expr = NewExpressionInteger(last_enum_member->const_expr->integer << 1);
                }
                else
                {
                    result->const_expr = NewExpressionInteger(last_enum_member->const_expr->integer + 1);
                }
                
            }
            else
            {
                if(enum_flags)
                {
                    result->const_expr = NewExpressionInteger(1);
                }
                else
                {
                    result->const_expr = expr_zero;
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
ParseEnum(lexer_state *lexer)
{
    declaration *result = 0;
    
    if(!WillEatKeyword(lexer, Keyword_Enum))   
        SyntaxError(lexer->file, lexer->line_at, "Forgot enum keyword!");
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result = NewDeclaration(Declaration_Enum); 
        result->identifier = lexer->eaten.identifier;
        if(!FindBasicType(result->identifier))
        {
            AddEnumType(result->identifier);
        }
        else
        {
            SyntaxError(lexer->file, lexer->line_at, "Enum redefinition");
        }
    }
    else
    {
        //TODO allow untyped enums?
        SyntaxError(lexer->file, lexer->line_at, "Untyped enum!");
    }
    ExpectToken(lexer, '{');
    result->members = ParseEnumMember(lexer, 0, false);
    declaration *current = result->members;
    while(WillEatTokenType(lexer, ';') &&
          PeekToken(lexer).type != '}')
    {
        current->next = ParseEnumMember(lexer, current, false);
        current = current->next;
    }
    ExpectToken(lexer, '}');
    return result;
}

internal declaration *
ParseEnumFlags(lexer_state *lexer)
{
    declaration *result = 0;
    ExpectKeyword(lexer, Keyword_EnumFlags);
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result = NewDeclaration(Declaration_EnumFlags);
        result->identifier = lexer->eaten.identifier;
        if(!FindBasicType(result->identifier))
        {
            AddEnumType(result->identifier);
        }
        ExpectToken(lexer, '{');
        result->members = ParseEnumMember(lexer, 0, true);
        if(result->members)
        {
            declaration *current = result->members;
            while(WillEatTokenType(lexer, ';') &&
                  PeekToken(lexer).type != '}')
            {
                current->next = ParseEnumMember(lexer, current, true);
                current = current->next;
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

internal declaration * //NOTE parse_var only adds to the type_table
ParseVariable(lexer_state *lexer, type_specifier *type, declaration_type decl_type)
{
    //TODO type inference
    declaration *result = NewDeclaration(decl_type);
    if(type)  result->typespec = type;
    else      result->typespec = ParseTypeSpecifier(lexer);
    if(!result->typespec)   Panic();
    
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result->identifier = lexer->eaten.identifier;
        
        if(WillEatTokenType(lexer, '='))
        {
            if(PeekToken(lexer).type == '{')
            {
                result->initializer = ParseCompoundInitializerExpression(lexer);
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
                result->initializer = ParseExpression(lexer);
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
                    result->initializer = NewExpression(Expression_ToBeDefined); //needs constructor
                }
                else result->initializer = 0;
            }
            else if (result->typespec->type == TypeSpec_Array)
            {
                result->initializer = expr_zero_struct; //NOTE default to zero init!
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
                NextToken(lexer).type == ')'))
            {
                result->next = ParseVariable(lexer, result->typespec, decl_type);
            }
            else
            {
                
                result->next = ParseVariable(lexer, 0, decl_type);
            }
        }
        
        if(decl_type == Declaration_Variable)
        {
            ExpectToken(lexer, ';');
        }
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Unnamed variable!");
    }
    return result;
}


inline bool32
ValidMemberDeclaration(declaration *member, type_specifier *struct_typespec)
{
    bool32 valid_member_decl = false;
    if(member->type == Declaration_Struct || member->type == Declaration_Union)
    {
        Assert(member->members);
        if(!member->identifier) //only anonymous structs allowed!
        {
            valid_member_decl = true;
        }
    }
    else if(member->type == Declaration_Variable)
    {
        if(member->typespec->type != TypeSpec_StructUnion)
        {
            valid_member_decl = true;
        }
        else if(!struct_typespec) //anonymous struct is 
        {
            valid_member_decl = true;
        }
        else if(struct_typespec != member->typespec)
        {
            valid_member_decl = true;
        }
        
    }
    return valid_member_decl;
}


internal bool32
AreMembersInitializedToSomething(declaration *decl)
{
    Assert(decl->type == Declaration_Struct ||
           decl->type == Declaration_Union);
    bool32 result = false;
    for(declaration *member = decl->members;
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
            if(AreMembersInitializedToSomething(member))
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

internal declaration *
ParseStructUnion(lexer_state *lexer)
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
            if(!FindBasicType(result->identifier))
            {
                this_type = AddStructUnionType(lexer->eaten.identifier);
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Struct already defined!");
            }
        }
        
        //Parse all of it's members
        ExpectToken(lexer, '{');
        declaration **current = &result->members;
        while(PeekToken(lexer).type != '}')
        {
            declaration *member = ParseDeclaration(lexer);
            if(ValidMemberDeclaration(member, this_type))
            {
                *current = member;
                while(*current) current = &(*current)->next;
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Invalid Struct/Union Member Declaration!");
            }
        }
        ExpectToken(lexer, '}');
        if(!result->members)
        {
            SyntaxError(lexer->file, lexer->line_at, "Empty Struct/Union!");
        }
        
        
        //TODO better way to store information like this
        //NOTE I could maybe make some sort of separate data structure that just stores fix up information
        //instead of embedding that info in every struct, it also makes it cleaner!
        if(AreMembersInitializedToSomething(result))
        {
            result->struct_needs_constructor = true;
            if(this_type)
            {
                this_type->needs_constructor = result->struct_needs_constructor; //TODO think about this
            }
            
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


//
//NOTE if it's convenient this may pass multiple decls through the *next pointer!!!
//
internal declaration *
ParseDeclaration(lexer_state *lexer)
{
    declaration *result = 0;
    lexer_token token = PeekToken(lexer);
    if(token.type == TokenType_Keyword)
    {
        switch(token.keyword)
        {
            case Keyword_Typedef:
            result = ParseTypedef(lexer);
            break;
            case Keyword_Struct: case Keyword_Union: 
            result = ParseStructUnion(lexer);
            break;
            case Keyword_Enum:
            result = ParseEnum(lexer);
            break;
            case Keyword_EnumFlags:
            result = ParseEnumFlags(lexer);
            break;
            case Keyword_Internal: case Keyword_External:
            case Keyword_Inline: case Keyword_NoInline:
            result = ParseProcedure(lexer, token.keyword);
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









internal void
PrintDeclaration(declaration *decl, u32 indent)
{
    u32 scope_ident = 1;
    PrintTabs(indent);
    switch(decl->type)
    {
        case Declaration_Variable: 
        printf("Decl Variable:%s...", decl->identifier);
        PrintTypeSpecifier(decl->typespec);
        if(decl->initializer)
        {
            PrintExpression(decl->initializer, indent);
        }
        printf("\n");
        break;
        
        
        case Declaration_Typedef: 
        printf("Decl Typedef:%s...alias:%s\n", decl->identifier, decl->typespec->identifier);
        break;
        
        
        case Declaration_Struct: 
        printf("Decl Struct:%s with members...\n", decl->identifier);
        for(declaration *member = decl->members; 
            member;
            member = member->next)
        {
            PrintDeclaration(member, indent + scope_ident);
        }
        
        
        break;
        case Declaration_Union: 
        printf("Decl Union:%s with members...\n", decl->identifier);
        for(declaration *member = decl->members; 
            member;
            member = member->next)
        {
            PrintDeclaration(member, indent + scope_ident);
        }
        break;
        
        
        case Declaration_Enum: 
        printf("Decl Enum:%s with members...\n", decl->identifier);
        for(declaration *member = decl->members; 
            member;
            member= member->next)
        {
            PrintDeclaration(member, indent + scope_ident);
        }
        break;
        
        
        case Declaration_EnumMember: 
        printf("Decl EnumMember:%s\n", decl->identifier);
        if(decl->const_expr)
        {
            PrintExpression(decl->const_expr, indent);
        }
        break;
        
        
        case Declaration_Procedure: 
        printf("Decl Procedure:%s\n", decl->identifier);
        Panic();
        //printf("Returns:", decl->identifier);
        //PrintTypeSpecifier(decl->proc_return_type, indent);
        //printf("\n");
        //printf("Args:", decl->identifier);
        //for(declaration *arg = decl->proc_args;
        //arg;
        //arg = arg->next)
        //{
        //PrintDeclaration(arg, indent);
        //}
        //PrintStatement(decl->proc_body);
        break;
    }
}

inline void
PrintAST(declaration *ast)
{
    for(declaration *decl = ast;
        decl;
        decl = decl->next)
    {
        PrintDeclaration(decl, 0);
        printf("\n");
    }
}