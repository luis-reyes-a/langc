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
    assert(compound->type == Statement_Compound);
    if(compound->compound_stmts)
    {
        assert(compound->compound_tail->next == 0);
        compound->compound_tail->next = stmt;
        compound->compound_tail = stmt;
    }
    else
    {
        compound->compound_stmts = stmt;
        compound->compound_tail = stmt;
    }
}


internal statement *ParseStatement(lexer_state *lexer, declaration_list *list);


internal statement *
parse_compound_stmt(lexer_state *lexer, declaration_list *scope)
{
    expect_token(lexer, '{');
    statement *result = NewStatement(Statement_Compound);
    
    result->decl_list.above = scope;
    statement *stmt = ParseStatement(lexer, &result->decl_list);
    result->compound_stmts = stmt;
    result->compound_tail = result->compound_stmts;
    assert(result->compound_stmts);
    if(stmt->type == Statement_Declaration)
    {
        decl_list_append(&result->decl_list, stmt->decl);
    }
    while(PeekToken(lexer).type != '}')
    {
        stmt = ParseStatement(lexer, &result->decl_list);
        if(stmt->type == Statement_Declaration)
        {
            decl_list_append(&result->decl_list, stmt->decl);
        }
        AppendStatement(result, stmt);
    }
    expect_token(lexer, '}');
    
    
    
    
#if 0 //wtf is this for?
    for(declaration *decl = result->decl_list.decls;
        decl;
        decl = decl->next)
    {
        if(decl->type != Declaration_ProcedureArgs)
        {
            //can't we have undeclared types in the ProcedureArgs?
            if(decl->typespec->type == TypeSpec_UnDeclared)
            {
                if(top_decl->type == Declaration_Procedure)
                {
                    char *header_identifier = ConcatCStringsIntern(top_decl->identifier, "__Header");
                }
                else
                {
                    panic(); //when will this happen?
                }
                
            }
        }
    }
#endif
    return result;
}


internal statement *
ParseStatement(lexer_state *lexer, declaration_list *scope)
{
    statement *result = 0;
    if(eats_keyword(lexer, Keyword_If)) //if or 'switch'
    {
        expression *expr = parse_expr(lexer, scope);
        if(eats_token(lexer, TokenType_Equals)) //switch
        {
            result = NewStatement(Statement_Switch);
            result->cond_expr = expr;
            result->cond_stmt = ParseStatement(lexer, scope);
            //NOTE require opening and closing braces?
        }
        else //regular if
        {
            result = NewStatement(Statement_If); 
            result->cond_expr = expr;
            eats_keyword(lexer, Keyword_Then);
            result->cond_stmt = ParseStatement(lexer, scope);
        }
    }
    else if(eats_keyword(lexer, Keyword_Else))
    {
        result = NewStatement(Statement_Else);
        result->cond_stmt = ParseStatement(lexer, scope);
    }
    
    else if(eats_keyword(lexer, Keyword_While))
    {
        result = NewStatement(Statement_While);
        result->cond_expr = parse_expr(lexer, scope);
        result->cond_stmt = ParseStatement(lexer, scope);
    }
    else if(eats_keyword(lexer, Keyword_For)) 
    {
        result = NewStatement(Statement_For);
        result->for_init_stmt = ParseStatement(lexer, scope); //TODO think about this
        result->for_expr2 = parse_expr(lexer, scope);
        expect_token(lexer, ';');
        result->for_expr3 = parse_expr(lexer, scope);
        result->for_stmt = ParseStatement(lexer, scope);
    }
    else if(eats_keyword(lexer, Keyword_Defer))
    {
        result = NewStatement(Statement_Defer);
        result->defer_stmt = ParseStatement(lexer, scope);
    }
    else if(eats_keyword(lexer, Keyword_Goto))
    {
        panic();
#if 0
        result = NewStatement(Statement_Goto);
        lexer_token token = EatToken(lexer);
        if(token.type == TokenType_Identifier)
        {
            result->goto_handle = token.identifier;
            expect_token(lexer, ';');
        }
        else
        {
            SyntaxError(lexer->file, lexer->line_at, "Expected goto handle");//TODO
        }
#endif
        
    }
    else if(PeekToken(lexer).type == '{') //compound
    {
        result = parse_compound_stmt(lexer, scope);
    }
    else if(eats_token(lexer, TokenType_Equals)) //switch case statement
    {
        result = NewStatement(Statement_SwitchCase);
        if(PeekToken(lexer).type == '{') //default case
        {
            result->cond_expr = 0;
        }
        else
        {
            result->cond_expr = parse_expr(lexer, scope);
            expression cexpr = TryResolveExpression(result->cond_expr);
            if(cexpr.type)
            {
                result->cond_expr_as_const = DuplicateExpression(&cexpr);
            }
            else
            {
                SyntaxError(lexer->file, lexer->line_at, "Case label is not consant!");
            }
        }
        result->cond_stmt = ParseStatement(lexer, scope);
    }
    else if(eats_keyword(lexer, Keyword_Break))
    {
        result = NewStatement(Statement_Break);
        expect_token(lexer, ';');
    }
    else if(eats_keyword(lexer, Keyword_Continue))
    {
        result = NewStatement(Statement_Continue);
        expect_token(lexer, ';');
    }
    else if(eats_keyword(lexer, Keyword_Return))
    {
        result = NewStatement(Statement_Return);
        result->expr = parse_expr(lexer, scope);
        expect_token(lexer, ';');
    }
    else if(eats_token(lexer, ';'))
    {
        result = NewStatement(Statement_Null);
    }
    else //Try to parse an expression or decl
    {
        if(PeekToken(lexer).type == TokenType_Keyword)//struct, union, enum, etc...
        {
            result = NewStatement(Statement_Declaration);
            result->decl = parse_decl(lexer, scope);
            
            //TODO add it to local scope
            assert(result->decl->type != Declaration_Procedure);
            if(result->decl->type == Declaration_Typedef)
            {
                expect_token(lexer, ';');
            }
        }
        else
        {//NOTE better way to figure this out???
            if(PeekToken(lexer).type == TokenType_Identifier && //var decl
               (NextToken(lexer).type == TokenType_Identifier ||
                lexer->next.type == '*' ||
                lexer->next.type == '['))
            {
                result = NewStatement(Statement_Declaration);
                result->decl = parse_decl(lexer, scope);
            }
            else
            {
                result = NewStatement(Statement_Expression);
                result->expr = parse_expr(lexer, scope);
                expect_token(lexer, ';');
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