#include "expression.h"


memory_arena expression_arena; //NOTE should I use this?

inline expression *
NewExpression(expression_type type)
{
    expression *expr = PushType(&ast_arena, expression);
    *expr = (expression) { 0 };
    expr->type = type;
    return expr;
}

inline expression *
NewUnaryExpression(expression_unary_type type, expression *expr)
{
    Assert(expr);
    expression *result = NewExpression(Expression_Unary);
    result->unary_type = type;
    result->unary_expr = expr;
    return result;
}

inline expression *
NewBinaryExpression(expression_binary_type type, expression *left, expression *right)
{
    Assert(left && right);
    expression *result = NewExpression(Expression_Binary);
    result->binary_type = type;
    result->binary_left = left;
    result->binary_right = right;
    return result;
}


inline expression_binary_type
BinaryTypeFromEatenOperator(lexer_state *lexer, lexer_token_type *types, u32 count)
{
    for(u32 i = 0; i < count; ++i)
    {
        if(WillEatTokenType(lexer, types[i]))
        {
            return (expression_binary_type)types[i];
        }
    }
    return Binary_None;
}

inline expression *ParseExpression(lexer_state *lexer);

internal expression *
TryParseParentheticalExpression(lexer_state *lexer)
{
    expression *result = 0;
    if(WillEatTokenType(lexer, '('))
    {
        result = ParseExpression(lexer);
        ExpectToken(lexer, ')');
    }
    return result;
}

internal expression *
TryParseTopExpression(lexer_state *lexer)
{
    expression *result = 0;
    lexer_token token = PeekToken(lexer);
    if(TokenTypeInteger(&token))
    {
        EatToken(lexer);
        result = NewExpression(Expression_Integer);
        result->integer_value = token.integer;
    }
    else if (token.type == TokenType_Float64 || token.type == TokenType_Float)
    {
        EatToken(lexer);
        result = NewExpression(Expression_RealNumber);
        result->real_value = token.floating;
    }
    else if (token.type == TokenType_StringLiteral)
    {
        EatToken(lexer);
        result = NewExpression(Expression_StringLiteral);
        result->string_literal = token.string;
    }
    else if (token.type == TokenType_CharacterLiteral)
    {
        EatToken(lexer);
        result = NewExpression(Expression_CharLiteral);
        result->char_literal = token.character;
    }
    else if (token.type == TokenType_PlusPlus || token.type == TokenType_MinusMinus)
    {
        Panic(); //TODO
    }
    else if(token.type == '(')
    {
        result = TryParseParentheticalExpression(lexer);
        //NOTE should this be on top? it really doesn't matter in this case
        //but it feels like it should be moved up since it has a higher precedence
    }
    else if(token.type == TokenType_Identifier)
    {
        EatToken(lexer);
        result = NewExpression(Expression_Identifier);
        result->identifier = token.identifier;
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "..."); //TODO more cases I'm not thinking about?
    }
    
    token = PeekToken(lexer);
    while(token.type == '(' /*CALL*/ ||
          token.type == '[' /*ARRAY*/ ||
          token.type == '.' /*ACCESS*/)
    {
        expression *left = result;
        if(left->type == Expression_Integer || left->type == Expression_RealNumber)
        { //TODO more checking that left is a valid expression for this expression types!!!
            SyntaxError(lexer->file, lexer->line_at, "Check for more invalid expressions...");
        }
        if(WillEatTokenType(lexer,'(')) //call
        {
            result = NewExpression(Expression_Call);
            result->call_expr = left;
            result->call_args = ParseExpression(lexer); 
            ExpectToken(lexer, ')');
        }
        else if(WillEatTokenType(lexer,'[')) //array subscript
        {
            result = NewExpression(Expression_ArraySubscript);
            result->array_expr = left;
            result->call_args = ParseExpression(lexer); 
            ExpectToken(lexer, ']');
        }
        else if(WillEatTokenType(lexer,'.')) //member access(through ptr also)
        {
            result = NewExpression(Expression_MemberAccess);
            result->struct_expr = left;
            token = PeekToken(lexer);
            if(token.type == TokenType_Identifier)
            {
                EatToken(lexer);
                result->struct_member_identifier = token.identifier;
            }
            else
            {
                //TODO
                SyntaxError(token.file, token.line, "Expected member identifier");
            }
        }
        token = PeekToken(lexer);
    }
    return result;
}

//TODO increment and decrement operators???

internal expression * //NOTE right to left
TryParseUnaryExpression(lexer_state *lexer)
{
    expression *result = 0;
    if(WillEatTokenType(lexer, '!'))
    {
        expression *expr = TryParseUnaryExpression(lexer);
        result = NewUnaryExpression('!', expr);
    }
    else if(WillEatTokenType(lexer, '*'))
    {
        expression *expr = TryParseUnaryExpression(lexer);
        result = NewUnaryExpression('*', expr);
    }
    else if(WillEatTokenType(lexer, '&'))
    {
        expression *expr = TryParseUnaryExpression(lexer);
        result = NewUnaryExpression('&', expr);
    }
    else if(WillEatTokenType(lexer, '-'))
    {
        expression *expr = TryParseUnaryExpression(lexer);
        result = NewUnaryExpression('-', expr);
    }
    else result = TryParseTopExpression(lexer);
    return result;
}

internal expression *
TryParseProductExpression(lexer_state *lexer)
{
    expression *left = TryParseUnaryExpression(lexer);
    expression *result = left;
    
    lexer_token_type ops[] = {'*', '/', '%'};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops)))
    {
        expression *right = TryParseUnaryExpression(lexer);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}

internal expression *
TryParseSumExpression(lexer_state *lexer)
{
    expression *left = TryParseProductExpression(lexer);
    expression *result = left;
    
    lexer_token_type ops[] = {'+', '-'};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops)))
    {
        expression *right = TryParseProductExpression(lexer);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}



//#define BinaryParsingFunctionLeftToRight(name, prev_func, __VA_ARGS__) //with operators there...

internal expression *
TryParseShiftExpression(lexer_state *lexer)
{
    expression *left = TryParseSumExpression(lexer);
    expression *result = left;
    
    lexer_token_type ops[] = {TokenType_LeftShift, TokenType_RightShift};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops)))
    {
        expression *right = TryParseSumExpression(lexer);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}

internal expression *
TryParseRelationalExpression(lexer_state *lexer)
{
    expression *left = TryParseShiftExpression(lexer);
    expression *result = left;
    
    lexer_token_type ops[] = {TokenType_Equals, TokenType_NotEquals,
        TokenType_LessThanEquals, TokenType_GreaterThanEquals, '<', '>'};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, ArrayCount(ops)))
    {
        expression *right = TryParseShiftExpression(lexer);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}




internal expression *
TryParseAndExpression(lexer_state *lexer)
{
    expression *left = TryParseRelationalExpression(lexer);
    expression *result = left;
    while(WillEatTokenType(lexer, TokenType_And))
    {
        expression *right = TryParseRelationalExpression(lexer);
        result = NewBinaryExpression(Binary_And, left, right);
        left = result;
    }
    
    return result;
}
internal expression *
TryParseOrExpression(lexer_state *lexer)
{
    expression *left = TryParseAndExpression(lexer);
    expression *result = left;
    while(WillEatTokenType(lexer, TokenType_Or))
    {
        expression *right = TryParseAndExpression(lexer);
        result = NewBinaryExpression(Binary_Or, left, right);
        left = result;
    }
    
    return result;
}


internal expression *
TryParseTernaryExpression(lexer_state *lexer)
{
    expression *result = 0;
    if(WillEatKeyword(lexer, Keyword_If))
    {
        expression *cond = TryParseOrExpression(lexer);
        WillEatKeyword(lexer, Keyword_Then);
        
        expression *expr1 = TryParseTernaryExpression(lexer);
        ExpectKeyword(lexer, Keyword_Else); //TODO maybe ':'...
        
        result = NewExpression(Expression_Ternary);
        result->tern_cond = cond;
        result->tern_true_expr = expr1;
        result->tern_false_expr = TryParseTernaryExpression(lexer);
    }
    else result = TryParseOrExpression(lexer);
    return result;
}


internal expression * //NOTE right to left
TryParseAssignmentExpression(lexer_state *lexer)
{
    expression *left = TryParseTernaryExpression(lexer); 
    expression *result = left;
    if(WillEatTokenType(lexer, '='))
    {
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_Assign, left, right);
    }
    else if(WillEatTokenType(lexer, TokenType_AddAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_AddAssign, left, right);
    }
    else if(WillEatTokenType(lexer, TokenType_SubtractAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_SubAssign, left, right);
    }
    else if(WillEatTokenType(lexer, TokenType_MultiplyAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_MulAssign, left, right);
    }
    else if(WillEatTokenType(lexer, TokenType_DivideAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_DivAssign, left, right);
    }
    else if(WillEatTokenType(lexer, TokenType_ModAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_ModAssign, left, right);
    }
    else if(WillEatTokenType(lexer, TokenType_BitwiseAndAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_AndAssign, left, right);
    } 
    else if(WillEatTokenType(lexer, TokenType_BitwiseOrAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_OrAssign, left, right);
    }
    else if(WillEatTokenType(lexer, TokenType_BitwiseXorAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer);
        result = NewBinaryExpression(Binary_XorAssign, left, right);
    }
    return result;
}



internal expression * //NOTE parses RIGHT to LEFT!!!!
TryParseCompoundExpression(lexer_state *lexer)
{
    expression *first = TryParseAssignmentExpression(lexer);
    expression *result = first;
    if(WillEatTokenType(lexer, ','))
    {//NOTE this is parsing right to left...but it shouldn't matter as long as I know that...
        result = NewExpression(Expression_Compound);
        result->compound_expr = first;
        result->compound_next = TryParseCompoundExpression(lexer);
    }
    return result;
}

inline expression *
ParseExpression(lexer_state *lexer)
{
    return TryParseCompoundExpression(lexer);
}




#define CasePrint(_case) case _case: printf("%s", #_case); break

#define CasePrintChar(_case) case _case: printf("%c", _case); break;

inline void
PrintBinaryOperator(expression_binary_type type)
{
    switch(type)
    {
        CasePrintChar(Binary_Mod);
        CasePrintChar(Binary_Add);
        CasePrintChar(Binary_Sub);
        CasePrintChar(Binary_Mul);
        CasePrintChar(Binary_Div);
        CasePrintChar(Binary_GreaterThan);
        CasePrintChar(Binary_LessThan);
        case Binary_GreaterThanEquals: printf(">="); break;
        case Binary_LessThanEquals: printf("<="); break;
        case Binary_Equals: printf("=="); break;
        case Binary_NotEquals: printf("!="); break;
        case Binary_And: printf("&&"); break;
        case Binary_Or: printf("||"); break;
        CasePrintChar(Binary_BitwiseAnd) ;
        CasePrintChar(Binary_BitwiseOr);
        CasePrintChar(Binary_BitwiseXor);
        CasePrintChar(Binary_Assign) ;
        case Binary_AndAssign: printf("&="); break;
        case Binary_OrAssign: printf("|="); break;
        case Binary_XorAssign: printf("^="); break;
        case Binary_AddAssign: printf("+="); break;
        case Binary_SubAssign: printf("-="); break;
        case Binary_MulAssign: printf("*="); break;
        case Binary_DivAssign: printf("/="); break;
        case Binary_ModAssign: printf("%%="); break;
        case Binary_LeftShift: printf("<<"); break;
        case Binary_RightShift: printf(">>"); break;
        default: Panic();
    }
}

inline void
PrintUnaryOperator(expression_unary_type type)
{
    switch(type)
    {
        //CasePrintChar(Unary_Plus);
        CasePrintChar(Unary_Minus);
        case Unary_PreIncrement: case Unary_PostIncrement: printf("++"); break;
        case Unary_PreDecrement: case Unary_PostDecrement: printf("--"); break;
        //CasePrintChar(Unary_BitwiseNot);
        CasePrintChar(Unary_Not);
        CasePrintChar(Unary_Dereference);
        CasePrintChar(Unary_AddressOf);
        default: Panic();
    }
}


internal void
PrintExpression(expression *expr, u32 indent)
{
    u32 tab = 1;
    if(expr)
    {
        PrintTabs(indent);
        //printf("Expression:");
        
        switch(expr->type)
        {
            case Expression_Identifier: { 
                printf("id.%s", expr->identifier);
            }break; //variable
            case Expression_StringLiteral: { 
                printf("\"%s\"", expr->string_literal);
            }break;
            case Expression_CharLiteral: { 
                printf("\'%c\'", expr->char_literal);
            }break;
            case Expression_Integer: { 
                printf("%llu", expr->integer_value);
            }break;
            case Expression_RealNumber: { 
                printf("%ff", (float)expr->real_value);
            }break;
            case Expression_Binary: { 
                printf("(");
                PrintExpression(expr->binary_left, indent);
                printf(" ");
                PrintBinaryOperator(expr->binary_type); 
                printf(" ");
                PrintExpression(expr->binary_right, indent);
                printf(")");
            }break;
            case Expression_Unary: { 
                if(expr->unary_type == Unary_PostDecrement ||
                   expr->unary_type == Unary_PostIncrement)
                {
                    PrintExpression(expr->unary_expr, indent);
                    PrintUnaryOperator(expr->unary_type);
                }
                else
                {
                    PrintUnaryOperator(expr->unary_type);
                    PrintExpression(expr->unary_expr, indent);
                }
            }break;
            case Expression_MemberAccess: { 
                PrintExpression(expr->struct_expr, indent);
                printf(".%s", expr->struct_member_identifier);
            }break; 
            case Expression_Ternary: { 
                printf("(");
                PrintExpression(expr->tern_cond, indent);
                printf("?");
                PrintExpression(expr->tern_true_expr, indent);
                printf(":");
                PrintExpression(expr->tern_false_expr, indent);
                printf(")");
            }break;
            case Expression_Cast: { 
                printf("cast(");
                PrintTypeSpecifier(expr->casting_to);
                printf(",");
                PrintExpression(expr->cast_expression, indent);
                printf(")");
            }break;
            case Expression_Call: { 
                //printf("Call...\n");
                PrintExpression(expr->call_expr, indent);
                PrintExpression(expr->call_args, indent);
            }break;
            case Expression_ArraySubscript: { 
                //printf("Array Index...\n");
                PrintExpression(expr->array_expr, indent );
                printf("[");
                PrintExpression(expr->array_index_expr, indent);
                printf("]");
            }break;
            case Expression_Compound: { 
                printf("(");
                expression *compound = expr;
                while(compound->type == Expression_Compound)
                {
                    PrintExpression(compound->compound_expr, indent );
                    printf(",");
                    compound = compound->compound_next;
                }
                if(compound)
                {
                    printf(",");
                    PrintExpression(compound, indent );
                }
                printf(")");
            }break;
            default: Panic();
        }
        
    }
}

#undef CasePrint






















#if 0
//NOTE doing it like c, but maybe && and || should have same precedence
internal expression *
TryParseAndExpression(lexer_state *lexer)
{
    expression *left = TryParseConditionalExpression(lexer);
    expression *last = left;
    if(WillEatTokenType(lexer, TokenType_And))
    {
        do
        {
            expression *right = TryParseConditionalExpression(lexer);
            expression *result = NewBinaryExpression(Expression_And, last, right);
            last = result;
        } while(WillEatKeyword(lexer, TokenType_And));
        return result;
    }
    else return left;
}

internal expression *
TryParseOrExpression(lexer_state *lexer)
{
    expression *left = TryParseAndExpression(lexer);
    expression *last = left;
    if(WillEatTokenType(lexer, TokenType_Or))
    {
        do
        {
            expression *right = TryParseAndExpression(lexer);
            expression *result = NewBinaryExpression(Expression_Or, last, right);
            last = result;
        } while(WillEatKeyword(lexer, TokenType_Or));
        return result;
    }
    else return left;
}











#endif