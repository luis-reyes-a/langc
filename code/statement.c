#include "statement.h"

inline statement *
NewStatement(statement_type type)
{
    statement *stmt = PushType(&ast_arena, statement);
    *stmt = (statement){ 0 };
    stmt->type = type;
    return stmt;
}


//NOTE ptr 0 and Null Statmenmt shouldn't be the same thing!
//NOTE this parses just a single statement!!!
internal statement *
ParseStatement(lexer_state *lexer)
{
    statement *result = 0;
    if(WillEatKeyword(lexer, Keyword_If)) //if or 'switch'
    {
        expression *expr = ParseExpression(lexer);
        if(WillEatTokenType(lexer, TokenType_And)) //switch
        {
            result = NewStatement(Statement_Switch);
            result->switch_cases = ParseStatement(lexer);
            //NOTE require opening and closing braces?
        }
        else //regular if
        {
            result = NewStatement(Statement_If); 
            result->if_expr = expr;
            WillEatKeyword(lexer, Keyword_Then);
            result->if_stmt = ParseStatement(lexer);
        }
    }
    else if(WillEatKeyword(lexer, Keyword_Else))
    {
        result = NewStatement(Statement_Else);
        result->else_stmt = ParseStatement(lexer);
    }
    
    else if(WillEatKeyword(lexer, Keyword_While))
    {
        result = NewStatement(Statement_While);
        result->while_expr = ParseExpression(lexer);
        result->while_stmt = ParseStatement(lexer);
    }
    else if(WillEatKeyword(lexer, Keyword_For)) //TODO think about this one!
    {
        result = NewStatement(Statement_For);
        result->for_init_stmt = ParseStatement(lexer);
        result->for_expr2 = ParseExpression(lexer);
        ExpectToken(lexer, ';');
        result->for_expr3 = ParseExpression(lexer);
        result->for_stmt = ParseStatement(lexer);
    }
    else if(WillEatKeyword(lexer, Keyword_Defer))
    {
        result = NewStatement(Statement_Defer);
        result->defer_stmt = ParseStatement(lexer);
    }
    else if(WillEatKeyword(lexer, Keyword_Goto))
    {
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
        
        
    }
    else if(WillEatTokenType(lexer, '{')) //compound
    {
        result = NewStatement(Statement_Compound);
        result->compound_stmt = ParseStatement(lexer);
        statement *current = result;
        while(PeekToken(lexer).type != '}')
        {
            Assert(current->type == Statement_Compound);
            current->compound_next = ParseStatement(lexer);
            if(PeekToken(lexer).type != '}')
            {
                statement *temp = current->compound_next;
                current->compound_next = NewStatement(Statement_Compound);
                current->compound_next->compound_stmt = temp;
            }
            current = current->compound_next;
        }
        
    }
    else if(WillEatTokenType(lexer, TokenType_And)) //switch case statement
    {
        result = NewStatement(Statement_SwitchCase);
        if(PeekToken(lexer).type == '{') //default case
        {
            result->case_label = 0;
        }
        else
        {
            result->case_label = ParseExpression(lexer);
        }
        result->case_statement = ParseStatement(lexer);
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
        result->return_expr = ParseExpression(lexer);
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
            result->decl = ParseDeclaration(lexer);
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
                result->decl = ParseDeclaration(lexer);
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