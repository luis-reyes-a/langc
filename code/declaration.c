#include "declaration.h"


inline declaration * //TODO
NewDeclaration(declaration_type type)
{
    declaration *decl = PushType(&ast_arena, declaration);
    *decl = (declaration){0};
    decl->type = type;
    return decl;
}



internal declaration *ParseDeclaration(lexer_state *lexer);



internal declaration *
ParseTypedef(lexer_state *lexer)
{
    return 0;
}

//types of style char *[4] StringTable;
internal type_specifier *
ParseTypeSpecifier(lexer_state *lexer)
{ 
    type_specifier *result = 0;
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result = FindBasicType(lexer->eaten.identifier);
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
            else if(WillEatTokenType(lexer, TokenType_Integer)) //TODO parse expression!
            {
                type_specifier *array_type = FindArrayType(result, lexer->eaten.integer);
                if(!array_type)
                {
                    array_type = AddArrayType(result, lexer->eaten.integer);
                }
                result = array_type;
                ExpectToken(lexer, ']');
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Invalid Array integral expression!");
            }
        }
    }
    return result;
}

internal declaration *ParseProcedureArgs(lexer_state *lexer, type_specifier *type);


internal declaration *
ParseProcedure(lexer_state *lexer, u32 qualifier)
{
    declaration *result = 0;
    
    if(WillEatTokenType(lexer, TokenType_Keyword))
    {
        result = NewDeclaration(Declaration_Procedure);
        result->proc_keyword = lexer->eaten.keyword;
        result->proc_return_type = ParseTypeSpecifier(lexer);
        result->identifier = ExpectIdentifier(lexer);
        ExpectToken(lexer, '(');
        result->proc_args = ParseProcedureArgs(lexer, 0);
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
ParseEnumMember(lexer_state *lexer)
{
    declaration *result = NewDeclaration(Declaration_EnumMember);
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result->identifier = lexer->eaten.identifier;
        if(WillEatTokenType(lexer, '='))
        {
            result->const_expr = ParseExpression(lexer);
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
    declaration *result = NewDeclaration(Declaration_Enum);
    if(!WillEatKeyword(lexer, Keyword_Enum))   
        SyntaxError(lexer->file, lexer->line_at, "Forgot enum keyword!");
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result->identifier = lexer->eaten.identifier;
        if(!FindBasicType(result->identifier))
        {
            AddEnumType(result->identifier);
        }
        else
        {
            //TODO redef...
        }
    }
    ExpectToken(lexer, '{');
    result->enum_members = ParseEnumMember(lexer);
    declaration *current = result->enum_members;
    while(PeekToken(lexer).type == ';')
    {
        current->next = ParseEnumMember(lexer);
        current = current->next;
    }
    ExpectToken(lexer, '}');
    return result;
}


internal declaration * //NOTE parse_var only adds to the type_table
ParseProcedureArgs(lexer_state *lexer, type_specifier *type)
{
    declaration *result = NewDeclaration(Declaration_ProcedureArgs);
    if(type)  result->typespec = type;
    else result->typespec = ParseTypeSpecifier(lexer);
    if(!result->typespec)   Panic();
    
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        result->identifier = lexer->eaten.identifier;
        
        if(WillEatTokenType(lexer, '='))
        {
            result->initializer = ParseExpression(lexer);
        }
        if(WillEatTokenType(lexer, ',')) 
        {
            if(PeekToken(lexer).type == TokenType_Identifier &&
               (NextToken(lexer).type == ',' ||
                NextToken(lexer).type == ';' ||
                NextToken(lexer).type == ')'))
            {
                result->next = ParseProcedureArgs(lexer, result->typespec);
            }
            else
            {
                
                result->next = ParseProcedureArgs(lexer, 0);
            }
        }
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Unnamed variable!");
    }
    return result;
}

inline declaration *
ParseVariable(lexer_state *lexer, type_specifier *type)
{
    declaration *result = ParseProcedureArgs(lexer, type);
    for(declaration *decl = result;
        decl;
        decl = decl->next)
    {
        decl->type = Declaration_Variable;
    }
    ExpectToken(lexer, ';');
    return result;
}

inline bool32
ValidMemberDeclaration(declaration *decl, type_specifier *struct_typespec)
{
    bool32 valid_member_decl = false;
    if(decl->type == Declaration_Struct || decl->type == Declaration_Union)
    {
        if(!decl->identifier)
        {//anonymouse structs/unions okay
            valid_member_decl = true;
        }
#if 0 //Typed structs shouldn't be allowed...
        else if(decl->identifier != result->identifier)
        {//
            valid_member_decl = true;
        }
#endif
    }
    else if(decl->type == Declaration_Variable)
    {
        if(decl->typespec->type != TypeSpec_StructUnion)
        {
            valid_member_decl = true;
        }
        else if(!struct_typespec) //anonymous struct is 
        {
            valid_member_decl = true;
        }
        else if(struct_typespec != decl->typespec)
        {
            valid_member_decl = true;
        }
        
    }
    return valid_member_decl;
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
        ExpectToken(lexer, '{');
        result->members = ParseDeclaration(lexer);
        if(result->members)
        {
            declaration *last_member = result->members;
            while(PeekToken(lexer).type != '}')
            {
                declaration *member = ParseDeclaration(lexer);
                if(ValidMemberDeclaration(member, this_type))
                {
                    last_member->next = member;
                    while(last_member->next) last_member = last_member->next;
                }
                else
                {
                    SyntaxError(lexer->file, lexer->line_at, "Invalid Struct/Union Member Declaration!");
                }
            }
        }
        else
        {
            SyntaxError(lexer->file, lexer->line_at, "Empty Struct/Union!");
        }
        ExpectToken(lexer, '}');
        
        if(PeekToken(lexer).type == TokenType_Identifier &&
           (NextToken(lexer).type == ',' || NextToken(lexer).type == ';'))
        {
            Assert(this_type); 
            result->next = ParseVariable(lexer, this_type);
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
            case Keyword_Internal: case Keyword_External:
            case Keyword_Inline: case Keyword_NoInline:
            result = ParseProcedure(lexer, token.keyword);
            break;
            default: Panic();
        }
    }
    else if(token.type == TokenType_Identifier)
    {
        result = ParseVariable(lexer, 0);
    }
    else if (token.type == TokenType_EndOfStream)
    {
        result = 0;
    }
    else if (token.type == TokenType_Invalid)
    {
        Panic();
    }
    else
    {
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