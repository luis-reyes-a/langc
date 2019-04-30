#include "statement.h"

inline statement *
NewStatement(statement_type type)
{
    statement *stmt = PushType(&ast_arena, statement);
    *stmt = (statement){ 0 };
    stmt->type = type;
    return stmt;
}

//gotta already have a list started...this is kind of confusing
inline void
AppendStatement(statement *compound, statement *stmt)
{
    Assert(compound->type == Statement_Compound);
    if(compound->compound_stmts)
    {
        Assert(compound->compound_tail->next == 0);
        compound->compound_tail->next = stmt;
        compound->compound_tail = stmt;
    }
    else
    {
        compound->compound_stmts = stmt;
        compound->compound_tail = stmt;
    }
    
}





internal declaration *
FindDeclaration(declaration_list *list, char *identifier)
{
    declaration *result = 0;
    Assert(list && identifier);
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
    if(!result && list->above)
    {
        result = FindDeclaration(list->above, identifier);
    }
    return result;
}


//NOTE ptr 0 and Null Statmenmt shouldn't be the same thing!
//NOTE this parses just a single statement!!!
internal statement *
ParseStatement(lexer_state *lexer, declaration_list *above, declaration *top_decl)
{
    statement *result = 0;
    if(WillEatKeyword(lexer, Keyword_If)) //if or 'switch'
    {
        expression *expr = ParseExpression(lexer);
        if(WillEatTokenType(lexer, TokenType_Equals)) //switch
        {
            result = NewStatement(Statement_Switch);
            result->cond_expr = expr;
            result->cond_stmt = ParseStatement(lexer, above, top_decl);
            //NOTE require opening and closing braces?
        }
        else //regular if
        {
            result = NewStatement(Statement_If); 
            result->cond_expr = expr;
            WillEatKeyword(lexer, Keyword_Then);
            result->cond_stmt = ParseStatement(lexer, above, top_decl);
        }
    }
    else if(WillEatKeyword(lexer, Keyword_Else))
    {
        result = NewStatement(Statement_Else);
        result->cond_stmt = ParseStatement(lexer, above, top_decl);
    }
    
    else if(WillEatKeyword(lexer, Keyword_While))
    {
        result = NewStatement(Statement_While);
        result->cond_expr = ParseExpression(lexer);
        result->cond_stmt = ParseStatement(lexer, above, top_decl);
    }
    else if(WillEatKeyword(lexer, Keyword_For)) 
    {
        result = NewStatement(Statement_For);
        result->for_init_stmt = ParseStatement(lexer, above, top_decl); //TODO think about this
        result->for_expr2 = ParseExpression(lexer);
        ExpectToken(lexer, ';');
        result->for_expr3 = ParseExpression(lexer);
        result->for_stmt = ParseStatement(lexer, above, top_decl);
    }
    else if(WillEatKeyword(lexer, Keyword_Defer))
    {
        result = NewStatement(Statement_Defer);
        result->defer_stmt = ParseStatement(lexer, above, top_decl);
    }
    else if(WillEatKeyword(lexer, Keyword_Goto))
    {
        Panic();
#if 0
        result = NewStatement(Statement_Goto);
        lexer_token token = EatToken(lexer);
        if(token.type == TokenType_Identifier)
        {
            result->goto_handle = token.identifier;
            ExpectToken(lexer, ';');
        }
        else
        {
            SyntaxError(lexer->file, lexer->line_at, "Expected goto handle");//TODO
        }
#endif
        
    }
    else if(WillEatTokenType(lexer, '{')) //compound
    {
        result = NewStatement(Statement_Compound);
        result->decl_list.above = above;
        statement *stmt = ParseStatement(lexer, &result->decl_list, top_decl);
        result->compound_stmts = stmt;
        result->compound_tail = result->compound_stmts;
        Assert(result->compound_stmts);
        //declaration_list_entry **next_entry = &result->decl_list;
        if(stmt->type == Statement_Declaration)
        {
            DeclarationListAppend(&result->decl_list, stmt->decl);
            //Assert(*next_entry == 0);
            //*next_entry = NewDeclarationListEntry(stmt->decl);
            //while(*next_entry) next_entry = &(*next_entry)->next;
        }
        while(PeekToken(lexer).type != '}')
        {
            stmt = ParseStatement(lexer, &result->decl_list, top_decl);
            if(stmt->type == Statement_Declaration)
            {
                DeclarationListAppend(&result->decl_list, stmt->decl);
                //Assert(*next_entry == 0);
                //*next_entry = NewDeclarationListEntry(stmt->decl);
                //while(*next_entry) next_entry = &(*next_entry)->next;
            }
            
            AppendStatement(result, stmt);
#if 0
            Assert(current->type == Statement_Compound);
            current->compound_next = ParseStatement(lexer);
            if(PeekToken(lexer).type != '}')
            {
                statement *temp = current->compound_next;
                current->compound_next = NewStatement(Statement_Compound);
                current->compound_next->compound_stmt = temp;
            }
            current = current->compound_next;
#endif
        }
        ExpectToken(lexer, '}');
        
        for(declaration *decl = result->decl_list.decls;
            decl;
            decl = decl->next)
        {
            if(decl->typespec->type == TypeSpec_UnDeclared)
            {
                decl->typespec->fixup->first_decl = top_decl;
            }
        }
        
    }
    else if(WillEatTokenType(lexer, TokenType_Equals)) //switch case statement
    {
        result = NewStatement(Statement_SwitchCase);
        if(PeekToken(lexer).type == '{') //default case
        {
            result->cond_expr = 0;
        }
        else
        {
            result->cond_expr = ParseExpression(lexer);
            expression cexpr = ResolvedExpression(result->cond_expr);
            if(cexpr.type)
            {
                result->cond_expr_as_const = NewExpression(0);
                *result->cond_expr_as_const = cexpr;
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Case label is not consant!");
            }
        }
        result->cond_stmt = ParseStatement(lexer, above, top_decl);
    }
    else if(WillEatKeyword(lexer, Keyword_Break))
    {
        result = NewStatement(Statement_Break);
        ExpectToken(lexer, ';');
    }
    else if(WillEatKeyword(lexer, Keyword_Continue))
    {
        result = NewStatement(Statement_Continue);
        ExpectToken(lexer, ';');
    }
    else if(WillEatKeyword(lexer, Keyword_Return))
    {
        result = NewStatement(Statement_Return);
        result->expr = ParseExpression(lexer);
        ExpectToken(lexer, ';');
    }
    else if(WillEatTokenType(lexer, ';'))
    {
        result = NewStatement(Statement_Null);
    }
    else //Try to parse an expression or decl
    {
        if(WillEatTokenType(lexer, TokenType_Keyword))//struct, union, enum, etc...
        {
            result = NewStatement(Statement_Declaration);
            result->decl = ParseDeclaration(lexer, above);
            if(result->decl->type == Declaration_Typedef)
            {
                ExpectToken(lexer, ';');
            }
        }
        else
        {//NOTE better way to figure this out???
            if(PeekToken(lexer).type == TokenType_Identifier && //var decl
               NextToken(lexer).type == TokenType_Identifier)
            {
                result = NewStatement(Statement_Declaration);
                result->decl = ParseDeclaration(lexer, above);
            }
            else
            {
                result = NewStatement(Statement_Expression);
                result->expr = ParseExpression(lexer);
                ExpectToken(lexer, ';');
            }
            
        }
    }
    return result;
}



/*
internal void
PrintStatement(statement *stmt)
{
    switch(stmt->type)
    {
        case Statement_Declaration:   
        {
            printf("Decl Stmt...");
            printf("\n");
            PrintDeclaration(stmt->decl);
            printf("\n");
        }break;
        case Statement_Expression:   
        {
            printf("Expr Stmt...");
            printf("\n");
            PrintExpression(stmt->expr);
            printf("\n");
        }break;
        case Statement_If:   
        {
            printf("If Stmt...\n");
            printf("Expr:");
            PrintExpression(stmt->if_expr);
            printf("\n");
            printf("Stmt:");
            PrintStatement(stmt->if_stmt);
            printf("\n");
        }break;
        
        case Statement_Else:   
        {
            printf("Else Stmt...\n");
            printf("Stmt:");
            PrintStatement(stmt->if_stmt);
            printf("\n");
        }break;
        case Statement_Switch:   
        {
            printf("Switch Stmt...\n");
            printf("Expr:");
            PrintExpression(stmt->if_expr);
            printf("\n");
            printf("Stmt:");
            PrintStatement(stmt->if_stmt);
            printf("\n");
        }break; 
        case Statement_SwitchCase:
        {
        
        }break; 
        case Statement_While:   break;
        case Statement_For:   break; //TODO may have more than one type of for loop!
        case Statement_Defer:   break;//TODO def do this one!
        case Statement_Goto:   break;//TODO think about this...
        case Statement_Break:   break;
        case Statement_Continue:   break;
        case Statement_Return:   break;
        case Statement_Null:   break;
        
        //TODO
        case Statement_Compound:   break;
        default: panic();
    }
}
*/