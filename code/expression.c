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

inline expression *
NewExpressionIdentifier(char *identifier)
{
    Assert(identifier);
    expression *result = NewExpression(Expression_Identifier);;
    result->identifier = StringIntern(identifier, StringLength(identifier));
    return result;
}

inline expression *
NewExpressionInteger(u64 integer)
{
    if(integer == 0)   return expr_zero;
    expression *result = NewExpression(Expression_Integer);
    result->integer = integer;
    return result;
}


#if 1
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
#endif

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
    if(WillEatTokenType(lexer, TokenType_Integer))
    {
        if(lexer->eaten.integer == 0)
        {
            result = expr_zero;
        }
        else
        {
            result = NewExpression(Expression_Integer);
            result->integer = lexer->eaten.integer;
        }
    }
    else if (WillEatTokenType(lexer, TokenType_Float))
    {
        result = NewExpression(Expression_RealNumber);
        result->real = lexer->eaten.floating;
    }
    else if (WillEatTokenType(lexer, TokenType_StringLiteral))
    {
        result = NewExpression(Expression_StringLiteral);
        result->string_literal = lexer->eaten.string;
    }
    else if (WillEatTokenType(lexer, TokenType_CharacterLiteral))
    {
        result = NewExpression(Expression_CharLiteral);
        result->char_literal = lexer->eaten.character;
    }
    else if (WillEatTokenType(lexer, TokenType_PlusPlus) || WillEatTokenType(lexer, TokenType_MinusMinus))
    {
        Panic(); //TODO
    }
    else if(PeekToken(lexer).type == '(' ||
            lexer->peek.type == TokenType_Identifier)
    {
        if(lexer->peek.type == '(')
        {
            result = TryParseParentheticalExpression(lexer);
        }
        else
        {
            EatToken(lexer);
            result = NewExpression(Expression_Identifier);
            result->identifier = lexer->eaten.identifier;
        }
        
        
        while(PeekToken(lexer).type == '(' /*CALL*/ ||
              lexer->peek.type == '['      /*ARRAY*/ ||
              lexer->peek.type == '.'      /*ACCESS*/)
        {
            expression *left = result;
            //TODO check left is valid expression here for each of the cases
            
            if(WillEatTokenType(lexer,'(')) //call
            {
                result = NewExpression(Expression_Call);
                result->call_expr = left;
                //NOTE temp as I figure out how to handle function pointers
                Assert(result->call_expr->type == Expression_Identifier);
                result->call_args = ParseExpression(lexer); 
                ExpectToken(lexer, ')');
            }
            else if(WillEatTokenType(lexer,'[')) //array subscript
            {
                result = NewExpression(Expression_ArraySubscript);
                result->array_expr = left;
                result->array_actual_index_expr = ParseExpression(lexer); 
                expression cexpr = ResolvedExpression(result->array_actual_index_expr);
                if(cexpr.type)
                {
                    result->array_index_expr_as_const = NewExpression(0);
                    *result->array_index_expr_as_const = cexpr;
                }
                else
                {
                    after_parse_fixup *fixup = NewFixup(Fixup_ExprNeedsConstExpression);
                    fixup->expr = result;
                }
                ExpectToken(lexer, ']');
            }
            else if(WillEatTokenType(lexer,'.')) //member access(through ptr also)
            {
                result = NewExpression(Expression_MemberAccess);
                result->struct_expr = left;
                if(WillEatTokenType(lexer, TokenType_Identifier))
                {
                    result->struct_member_identifier = lexer->eaten.identifier;
                }
                else
                {
                    SyntaxError(lexer->file, lexer->line_at, "Expected member identifier");
                }
            }
        }
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Can't parse top prec expression"); 
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
    
    //NOTE may think this is an if 'switch' statement
    if(PeekToken(lexer).type == TokenType_Equals &&
       NextToken(lexer).type == '{')
    {
        return result; //
    }
    
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

internal void
AppendExpressionToTail(expression **tail, expression *new_expr)
{
    expression *tail_expr = *tail;
    *tail = NewExpression(Expression_Compound);
    (*tail)->compound_expr = tail_expr;
    (*tail)->compound_next = new_expr;
}

internal expression * //NOTE parses RIGHT to LEFT!!!!
TryParseCompoundExpression(lexer_state *lexer)
{
    expression *result = TryParseAssignmentExpression(lexer);
    expression **tail = &result;
    
    if(WillEatTokenType(lexer, ',')) //
    {//NOTE this is parsing right to left...but it shouldn't matter as long as I know that...
        //NOTE this can be rewritten without AppendToTail() 
        //this may overbloat the stack if it gets very recursively nested!
        AppendExpressionToTail(tail, TryParseCompoundExpression(lexer));
        //*tail = (*tail)->compound_next;
    }
    return result;
}



internal expression * //NOTE
ParseCompoundInitializerExpression(lexer_state *lexer)
{
    ExpectToken(lexer, '{');
    expression *result = TryParseOrExpression(lexer);
    expression **tail = &result;
    while(WillEatTokenType(lexer, ','))
    {
        AppendExpressionToTail(tail, TryParseOrExpression(lexer));
        (*tail)->type = Expression_CompoundInitializer;
        tail = &((*tail)->compound_next);
    }
    if(result->type != Expression_CompoundInitializer)
    {
        if(result == expr_zero)
        {
            result = expr_zero_struct;
        }
        else
        {
            expression *last = result;
            result = NewExpression(Expression_CompoundInitializer);
            result->compound_expr = last;
        }
    }
    ExpectToken(lexer, '}');
    
    return result;
}

inline expression *
ParseExpression(lexer_state *lexer)
{
    return TryParseCompoundExpression(lexer);
}

inline int
ExpressionIsConstant(expression *expr)
{
    if(expr->type == Expression_CharLiteral ||
       expr->type == Expression_StringLiteral ||
       expr->type == Expression_Integer ||
       expr->type == Expression_RealNumber ||
       expr == expr_zero_struct)
    {
        return true;
    }
    else return false;
}

inline int
ExpressionIsNumerical(expression *expr)
{
    if(expr->type == Expression_Integer ||
       expr->type == Expression_RealNumber ||
       expr == expr_zero)
        return true;
    else return false;
}

#define COMMENT(...)

#define case_relational(_left, _right, _op, _result)\
if(ExpressionIsNumerical(&_left) && \
ExpressionIsNumerical(&_right))\
{\
    if(_left.type == Expression_Integer &&\
    _right.type == Expression_Integer)\
    {\
        _result.type = Expression_Integer;\
        _result.integer = _left.integer _op _right.integer;\
        \
    }\
    else if(_left.type == Expression_RealNumber &&\
    _right.type == Expression_RealNumber)\
    {\
        _result.type = Expression_Integer;\
        _result.integer = _left.integer _op _right.integer;\
        \
    }\
    else\
    {\
        Panic("Type mismatch!");\
    }\
}

#define case_arithmatic(_left, _right, _op, _result)\
if(ExpressionIsNumerical(&_left) && \
ExpressionIsNumerical(&_right))\
{\
    if(_left.type == Expression_Integer &&\
    _right.type == Expression_Integer)\
    {\
        _result.type = Expression_Integer;\
        _result.integer = _left.integer _op _right.integer;\
    }\
    else\
    {\
        COMMENT("What are C's integer promotion rules?")\
        _result.type = Expression_RealNumber;\
        if(_left.type == Expression_RealNumber &&\
        _right.type == Expression_RealNumber)\
        {\
            _result.real = _left.real _op _right.real;\
        }\
        else if(_left.type == Expression_RealNumber)\
        {\
            _result.real = _left.real _op _right.integer;\
        }\
        else if(_right.type == Expression_RealNumber)\
        {\
            _result.real = _left.integer _op  _right.real;\
        }\
        else Panic();\
    }\
}

#define case_comparison(_left, _right, _op, _result)\
if(_left.type == Expression_Integer &&\
_right.type == Expression_Integer)\
{\
    _result.type = Expression_Integer;\
    _result.integer = _left.integer _op _right.integer;\
}

inline int
CanResolveExpression(expression *expr)
{
    expression test = ResolvedExpression(expr);
    int result = ExpressionIsConstant(&test);
    return result;
}


//NOTE this creates a new constant expr, if it already was, then it dups it
internal expression //returns a constant expression
ResolvedExpression(expression *expr)
{
    expression result = {0};
    if(ExpressionIsConstant(expr)) //NOTE I think I will make copies
    {
        result = *expr;
        return result;
    }
    switch(expr->type)
    {
        case Expression_Identifier:
        
        break;
        case Expression_Binary:
        {
            expression  left = ResolvedExpression(expr->binary_left);
            expression  right = ResolvedExpression(expr->binary_right);
            switch(expr->binary_type)
            {
                case Binary_Mod:
                {
                    if(left.type == Expression_Integer &&
                       right.type == Expression_Integer)
                    {
                        result.type = Expression_Integer;
                        result.integer = left.integer + right.integer;
                    }
                    else
                    {
                        Panic();//TODO output message, can only mod on integers!
                    }
                }break;
                case Binary_Add:
                {
                    case_arithmatic(left, right, +, result);
                }break;
                case Binary_Sub:
                {
                    case_arithmatic(left, right, -, result)
                }break;
                case Binary_Mul:
                {
                    case_arithmatic(left, right, *, result);
                }break;
                case Binary_Div:
                {
                    if(ExpressionIsNumerical(&left) && 
                       ExpressionIsNumerical(&right))
                    {
                        if(right.integer == 0)
                        {
                            Panic(); //TODO div by zero error!
                        }
                        else
                        {
                            case_arithmatic(left, right, /, result);
                        }
                    }
                }break;
                
                case Binary_GreaterThan:
                {
                    case_relational(left, right, >, result);
                }break;
                case Binary_LessThan:
                {
                    case_relational(left, right, <, result);
                }break;
                case Binary_GreaterThanEquals:
                {
                    case_relational(left, right, >=, result);
                }break;
                case Binary_LessThanEquals:
                {
                    case_relational(left, right, <=, result);
                }break;
                case Binary_Equals:
                {
                    case_relational(left, right, ==, result);
                }break;
                case Binary_NotEquals:
                {
                    case_relational(left, right, !=, result);
                }break;
                
                case Binary_And:
                {
                    case_comparison(left, right, &&, result);
                }break;
                case Binary_Or:
                {
                    case_comparison(left, right, ||, result);
                }break;
                
                case Binary_BitwiseAnd:
                {
                    case_comparison(left, right, &, result);
                }break;
                case Binary_BitwiseOr:
                {
                    case_comparison(left, right, |, result);
                }break;
                case Binary_BitwiseXor:
                {
                    case_comparison(left, right, ^, result);
                }break;
                //NOTE not comparison but I want the same desired effect
                case Binary_LeftShift:
                {
                    case_comparison(left, right, <<, result);
                }break;
                case Binary_RightShift:
                {
                    case_comparison(left, right, >>, result);
                }break;
                
                case Binary_Assign:case Binary_AndAssign:case Binary_OrAssign:
                case Binary_XorAssign:case Binary_AddAssign:case Binary_SubAssign:
                case Binary_MulAssign:case Binary_DivAssign:case Binary_ModAssign:
                {
                    Panic("Not const expression");
                }
                default: Panic();
            }
        }break;
        case Expression_Unary:
        {
            case Unary_Minus:
            {
                expression unary_expr = ResolvedExpression(expr->unary_expr);
                if(ExpressionIsNumerical(&unary_expr))
                {
                    result = unary_expr;
                    result.negative = !result.negative;
                }
            }break;
            
            case Unary_Not:
            {
                expression unary_expr = ResolvedExpression(expr->unary_expr);
                if(ExpressionIsNumerical(&unary_expr))
                {
                    if(unary_expr.type == Expression_Integer)
                    {
                        unary_expr.integer = !unary_expr.integer;
                    }
                    else
                    {
                        unary_expr.real = !unary_expr.real;
                    }
                }
            }break;
            
            case Unary_Dereference: case Unary_AddressOf:
            case Unary_PreIncrement: case Unary_PreDecrement:
            case Unary_PostIncrement: case Unary_PostDecrement:
            {
                Panic("Not a constant expression!");
            }break;
            default: Panic();
        }break;
        case Expression_MemberAccess:
        {
            //TODO in C this wouldn't be true but what if I scoped globals to structs?
            //TODO struct-scoped constants
            Panic();
        }break;
        case Expression_Compound:
        {
            result = ResolvedExpression(expr->compound_tail);
        }
        case Expression_Ternary:
        {
            expression const_cond = ResolvedExpression(expr->tern_cond);
            expression const_true = ResolvedExpression(expr->tern_true_expr);
            expression const_false = ResolvedExpression(expr->tern_false_expr);
            if(ExpressionIsConstant(&const_cond) &&
               ExpressionIsConstant(&const_true) &&
               ExpressionIsConstant(&const_false))
            {
                if(const_cond.type == Expression_Integer)
                {
                    if(const_cond.integer == true)
                    {
                        result = const_true;
                    }
                    else
                    {
                        result = const_false;
                    }
                }
                else Panic(); //TODO
            }
        }break;
        
        case Expression_CompoundInitializer:
        {
            bool32 constant = true;
            expression *outer;
            for(outer = expr;
                outer->type == Expression_Compound;
                outer = outer->compound_next)
            {
                if(!CanResolveExpression(expr->compound_expr))
                {
                    constant = false;
                    break;
                }
            }
            if(constant && outer)
            {
                //last 
                result = ResolvedExpression(outer);
            }
        }break;
        
        case Expression_Cast:case Expression_Call:
        case Expression_ArraySubscript: case Expression_ToBeDefined:
        {
            Panic("Not const expression!");
        }break;
        
        //default: Panic();
    }
    return result;
}

















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