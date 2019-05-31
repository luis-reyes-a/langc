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
DuplicateExpression(expression *expr)
{
    expression *result = NewExpression(0);
    *result = *expr;
    return result;
}

inline expression *
NewUnaryExpression(expression_unary_type type, expression *expr)
{
    assert(expr);
    expression *result = NewExpression(Expression_Unary);
    result->unary_type = type;
    result->unary_expr = expr;
    return result;
}

inline expression *
NewBinaryExpression(expression_binary_type type, expression *left, expression *right)
{
    assert(left && right);
    expression *result = NewExpression(Expression_Binary);
    result->binary_type = type;
    result->binary_left = left;
    result->binary_right = right;
    return result;
}

inline expression *
NewExpressionIdentifier(char *identifier)
{
    assert(identifier);
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
        if(eats_token(lexer, types[i]))
        {
            return (expression_binary_type)types[i];
        }
    }
    return Binary_None;
}
#endif

inline expression *parse_expr(lexer_state *lexer, declaration_list *scope);

internal expression *
TryParseParentheticalExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *result = 0;
    if(eats_token(lexer, '('))
    {
        result = parse_expr(lexer, scope);
        expect_token(lexer, ')');
    }
    return result;
}


//NOTE TODO should I do interning for top level expressions
//like always store a unique copy of identifier exprs and  numbers and stuff like that???
internal expression *
TryParseTopExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *result = 0;
    if(eats_token(lexer, TokenType_Integer))
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
    else if (eats_token(lexer, TokenType_Float))
    {
        result = NewExpression(Expression_RealNumber);
        result->real = lexer->eaten.floating;
    }
    else if (eats_token(lexer, TokenType_StringLiteral))
    {
        result = NewExpression(Expression_StringLiteral);
        result->string_literal = lexer->eaten.string;
    }
    else if (eats_token(lexer, TokenType_CharacterLiteral))
    {
        result = NewExpression(Expression_CharLiteral);
        result->char_literal = lexer->eaten.character;
    }
    else if(eats_keyword(lexer, Keyword_Sizeof)){
        result = NewExpression(Expression_Sizeof);
        expect_token(lexer, '(');
        //TODO make this work with variables!
        result->typespec = parse_typespec(lexer, scope);
        expect_token(lexer, ')');
    }
    else if (eats_token(lexer, TokenType_PlusPlus) || eats_token(lexer, TokenType_MinusMinus))
    {
        panic(); //TODO
    }
    else if(PeekToken(lexer).type == '(' ||
            lexer->peek.type == TokenType_Identifier)
    {
        if(lexer->peek.type == '(')
        {
            result = TryParseParentheticalExpression(lexer, scope);
        }
        else //we have an identifier
        {
            EatToken(lexer);
            result = NewExpression(Expression_Identifier);
            result->identifier = lexer->eaten.identifier;
            result->decl = find_decl(scope, result->identifier);
        }
        else panic();
        
        
        while(PeekToken(lexer).type == '(' /*CALL*/ ||
              lexer->peek.type == '['      /*ARRAY*/ ||
              lexer->peek.type == '.'      /*ACCESS*/)
        {
            expression *left = result;
            //TODO check left is valid expression here for each of the cases
            
            if(eats_token(lexer,'(')) //call
            {
                result = NewExpression(Expression_Call);
                result->call_expr = left;
                //NOTE temp as I figure out how to handle function pointers
                assert(result->call_expr->type == Expression_Identifier);
                result->call_args = parse_expr(lexer, scope); 
                expect_token(lexer, ')');
            }
            else if(eats_token(lexer,'[')) //array subscript
            {
                result = NewExpression(Expression_ArraySubscript);
                result->array_expr = left;
                result->array_actual_index_expr = parse_expr(lexer, scope); 
                expression cexpr = TryResolveExpression(result->array_actual_index_expr);
                if(cexpr.type)
                {
                    result->array_index_expr_as_const = NewExpression(0);
                    *result->array_index_expr_as_const = cexpr;
                }
                expect_token(lexer, ']');
            }
            else if(eats_token(lexer,'.')) //member access(through ptr also)
            {
                result = NewExpression(Expression_MemberAccess);
                result->struct_expr = left;
                if(eats_token(lexer, TokenType_Identifier))
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
TryParseUnaryExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *result = 0;
    if(eats_token(lexer, '!'))
    {
        expression *expr = TryParseUnaryExpression(lexer, scope);
        result = NewUnaryExpression('!', expr);
    }
    else if(eats_token(lexer, '*'))
    {
        expression *expr = TryParseUnaryExpression(lexer, scope);
        result = NewUnaryExpression('*', expr);
    }
    else if(eats_token(lexer, '&'))
    {
        expression *expr = TryParseUnaryExpression(lexer, scope);
        result = NewUnaryExpression('&', expr);
    }
    else if(eats_token(lexer, '-'))
    {
        expression *expr = TryParseUnaryExpression(lexer, scope);
        result = NewUnaryExpression('-', expr);
    }
    else result = TryParseTopExpression(lexer, scope);
    return result;
}

internal expression *
TryParseProductExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *left = TryParseUnaryExpression(lexer, scope);
    expression *result = left;
    
    lexer_token_type ops[] = {'*', '/', '%'};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops)))
    {
        expression *right = TryParseUnaryExpression(lexer, scope);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}

internal expression *
TryParseSumExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *left = TryParseProductExpression(lexer, scope);
    expression *result = left;
    
    lexer_token_type ops[] = {'+', '-'};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops)))
    {
        expression *right = TryParseProductExpression(lexer, scope);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}



//#define BinaryParsingFunctionLeftToRight(name, prev_func, __VA_ARGS__) //with operators there...

internal expression *
TryParseShiftExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *left = TryParseSumExpression(lexer, scope);
    expression *result = left;
    
    lexer_token_type ops[] = {TokenType_LeftShift, TokenType_RightShift};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops)))
    {
        expression *right = TryParseSumExpression(lexer, scope);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}

internal expression *
TryParseRelationalExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *left = TryParseShiftExpression(lexer, scope);
    expression *result = left;
    
    //NOTE may think this is an if 'switch' statement
    if(PeekToken(lexer).type == TokenType_Equals &&
       NextToken(lexer).type == '{')
    {
        return result; //
    }
    
    lexer_token_type ops[] = {TokenType_Equals, TokenType_NotEquals,
        TokenType_LessThanEquals, TokenType_GreaterThanEquals, '<', '>'};
    for(expression_binary_type type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops));
        type;
        type = BinaryTypeFromEatenOperator(lexer, ops, array_count(ops)))
    {
        expression *right = TryParseShiftExpression(lexer, scope);
        result = NewBinaryExpression(type, left, right);
        left = result;
    }
    return result;
}




internal expression *
TryParseAndExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *left = TryParseRelationalExpression(lexer, scope);
    expression *result = left;
    while(eats_token(lexer, TokenType_And))
    {
        expression *right = TryParseRelationalExpression(lexer, scope);
        result = NewBinaryExpression(Binary_And, left, right);
        left = result;
    }
    
    return result;
}
internal expression *
TryParseOrExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *left = TryParseAndExpression(lexer, scope);
    expression *result = left;
    while(eats_token(lexer, TokenType_Or))
    {
        expression *right = TryParseAndExpression(lexer, scope);
        result = NewBinaryExpression(Binary_Or, left, right);
        left = result;
    }
    
    return result;
}


internal expression *
TryParseTernaryExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *result = 0;
    if(eats_keyword(lexer, Keyword_If))
    {
        expression *cond = TryParseOrExpression(lexer, scope);
        eats_keyword(lexer, Keyword_Then);
        
        expression *expr1 = TryParseTernaryExpression(lexer, scope);
        expect_keyword(lexer, Keyword_Else); //TODO maybe ':'...
        
        result = NewExpression(Expression_Ternary);
        result->tern_cond = cond;
        result->tern_true_expr = expr1;
        result->tern_false_expr = TryParseTernaryExpression(lexer, scope);
    }
    else result = TryParseOrExpression(lexer, scope);
    return result;
}


internal expression * //NOTE right to left
TryParseAssignmentExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *left = TryParseTernaryExpression(lexer, scope); 
    expression *result = left;
    if(eats_token(lexer, '='))
    {
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_Assign, left, right);
    }
    else if(eats_token(lexer, TokenType_AddAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_AddAssign, left, right);
    }
    else if(eats_token(lexer, TokenType_SubtractAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_SubAssign, left, right);
    }
    else if(eats_token(lexer, TokenType_MultiplyAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_MulAssign, left, right);
    }
    else if(eats_token(lexer, TokenType_DivideAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_DivAssign, left, right);
    }
    else if(eats_token(lexer, TokenType_ModAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_ModAssign, left, right);
    }
    else if(eats_token(lexer, TokenType_BitwiseAndAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_AndAssign, left, right);
    } 
    else if(eats_token(lexer, TokenType_BitwiseOrAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_OrAssign, left, right);
    }
    else if(eats_token(lexer, TokenType_BitwiseXorAssign)) { 
        expression *right = TryParseAssignmentExpression(lexer, scope);
        result = NewBinaryExpression(Binary_XorAssign, left, right);
    }
    return result;
}

internal expression * 
TryParseCompoundExpression(lexer_state *lexer, declaration_list *scope)
{
    expression *result = TryParseAssignmentExpression(lexer, scope);
    
    //NOTE this is stupid, i should just make a linked list
    //you have to allocate more nodes this way
    //but with linked list all expressions increase by 64bits!!!
    //i don't know compound expressions aren't that common
    //but compound initializers kindof are....
    
    //NOTE to iterate just do this
#if 0
    for(expression *member = compound->members;
        member;
        member = member->next)
    {
        assert(member->type == Expression_Member);
        //and remember to use member->member_expression not actual member!!!
    }
    
#endif
    
    if(PeekToken(lexer).type == ',')
    {
        expression *expr = result;
        result = NewExpression(Expression_Compound);
        result->members = NewExpression(Expression_Member);
        result->members->member_expression = expr;
        
        expression *last = result->members;
        while(eats_token(lexer, ','))
        {
            expr = TryParseAssignmentExpression(lexer, scope);
            assert(expr && last && last->member_next == 0);
            last->member_next = NewExpression(Expression_Member);
            last->member_next->member_expression = expr;
            last = last->member_next;
        }
    }
    return result;
}



internal expression * //NOTE
ParseCompoundInitializerExpression(lexer_state *lexer, declaration_list *scope)
{
    expect_token(lexer, '{');
    expression *result = NewExpression(Expression_CompoundInitializer);
    expression *expr = TryParseAssignmentExpression(lexer, scope);
    result->members = NewExpression(Expression_Member);
    result->members->member_expression = expr;
    
    if(PeekToken(lexer).type == ',')
    {
        expression *last = result->members;
        while(eats_token(lexer, ','))
        {
            expr = TryParseAssignmentExpression(lexer, scope);
            assert(expr && last && last->member_next == 0);
            last->member_next = NewExpression(Expression_Member);
            last->member_next->member_expression = expr;
            last = last->member_next;
        }
        result->last_member = last;
    }
    else result->last_member = result->members;
    expect_token(lexer, '}');
    return result;
}

inline expression * //scope used to lookup symbol
parse_expr(lexer_state *lexer, declaration_list *scope)
{
    if(eats_token(lexer, '{'))
    {
        return ParseCompoundInitializerExpression(lexer, scope);
    }
    else
    {
        return TryParseCompoundExpression(lexer, scope);
    }
}

inline expression *
ParseConstantExpression(lexer_state *lexer)
{
    return parse_expr(lexer, 0);
}

inline int
ExpressionIsConstant(expression *expr)
{
    if(expr->type == Expression_CharLiteral ||
       expr->type == Expression_StringLiteral ||
       expr->type == Expression_Integer ||
       expr->type == Expression_RealNumber ||
       expr->type == Expression_Sizeof ||
       expr->type == Expression_CallIdentifier || //TODO get rid of this here!!!
       expr == expr_zero_struct) //NOTE constant initializer?
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
        panic("Type mismatch!");\
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
        else panic();\
    }\
}

#define case_comparison(_left, _right, _op, _result)\
if(_left.type == Expression_Integer &&\
_right.type == Expression_Integer)\
{\
    _result.type = Expression_Integer;\
    _result.integer = _left.integer _op _right.integer;\
}

#if 0
inline int
CanResolveExpression(expression *expr)
{
    expression test = ResolvedExpression(expr);
    int result = ExpressionIsConstant(&test);
    return result;
}
#endif

//This is similar to ResolveExpression but can't look
//up decls for identifiers to solve expressions. Just done as a prepass
internal expression //returns a constant expression
TryResolveExpression(expression *expr)
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
            expression  left = TryResolveExpression(expr->binary_left);
            expression  right = TryResolveExpression(expr->binary_right);
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
                        panic();//TODO output message, can only mod on integers!
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
                            panic(); //TODO div by zero error!
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
                    panic("Not const expression");
                }
                default: panic();
            }
        }break;
        case Expression_Unary:
        {
            switch(expr->unary_type)
            {
                case Unary_Minus:
                {
                    expression unary_expr = TryResolveExpression(expr->unary_expr);
                    if(ExpressionIsNumerical(&unary_expr))
                    {
                        result = unary_expr;
                        result.negative = !result.negative;
                    }
                }break;
                
                case Unary_Not:
                {
                    expression unary_expr = TryResolveExpression(expr->unary_expr);
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
                    panic("Not a constant expression!");
                }break;
                default: panic();
            }
        }break;
        case Expression_MemberAccess:
        {
            //TODO in C this wouldn't be true but what if I scoped globals to structs?
            //TODO struct-scoped constants
            panic();
        }break;
        case Expression_Compound:
        {
            //get la
            result = TryResolveExpression(expr->last_member);
        }
        case Expression_Ternary:
        {
            expression const_cond = TryResolveExpression(expr->tern_cond);
            expression const_true = TryResolveExpression(expr->tern_true_expr);
            expression const_false = TryResolveExpression(expr->tern_false_expr);
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
                else panic(); //TODO
            }
        }break;
        
        case Expression_CompoundInitializer:
        {
#if 0
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
                result = TryResolveExpression(outer);
            }
#endif
        }break;
        
        case Expression_Cast:case Expression_Call:
        case Expression_ArraySubscript: case Expression_ToBeDefined:
        break;
        
        
        default: panic();
    }
    return result;
}

