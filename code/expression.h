#ifndef EXPRESSION_H
#define EXPRESSION_H

typedef enum
{
    Expression_None = 0,
    
    Expression_Identifier, //variable
    Expression_StringLiteral,
    Expression_CharLiteral,
    Expression_Integer,
    Expression_RealNumber,
    
    
    Expression_Binary,
    Expression_Unary,
    
    Expression_MemberAccess,
    Expression_Compound,
    Expression_Ternary,
    Expression_Cast,
    Expression_Call,
    Expression_ArraySubscript, 
    Expression_CompoundInitializer,
    Expression_SizeOf, //TODO
    Expression_ToBeDefined, //NOTE indicates that we expect an expression later in the parsing
} expression_type;

typedef enum
{
    Unary_None = 0,
    Unary_Minus = '-',
    Unary_PreIncrement = TokenType_PlusPlus,
    Unary_PreDecrement = TokenType_MinusMinus,
    Unary_PostIncrement,//TODO
    Unary_PostDecrement,//TODO
    Unary_Not = '!', 
    Unary_Dereference = '*', 
    Unary_AddressOf = '&',
} expression_unary_type;

typedef enum
{ 
    Binary_None = 0,
    Binary_Mod = '%',
    Binary_Add = '+', //integers, floats, vars of those types, and pointers and array ids.
    Binary_Sub = '-',
    Binary_Mul = '*',
    Binary_Div = '/',
    
    Binary_GreaterThan = '>',
    Binary_LessThan = '<',
    Binary_GreaterThanEquals = TokenType_GreaterThanEquals,
    Binary_LessThanEquals = TokenType_LessThanEquals,
    Binary_Equals = TokenType_Equals,
    Binary_NotEquals = TokenType_NotEquals,
    
    Binary_And = TokenType_And, 
    Binary_Or = TokenType_Or,
    
    Binary_BitwiseAnd = '&', 
    Binary_BitwiseOr = '|',
    Binary_BitwiseXor = '^',
    
    Binary_Assign = '=', 
    Binary_AndAssign = TokenType_BitwiseAndAssign,  //bitwise
    Binary_OrAssign  = TokenType_BitwiseOrAssign,   //bitwise
    Binary_XorAssign = TokenType_BitwiseXorAssign,  //bitwise
    Binary_AddAssign = TokenType_AddAssign,
    Binary_SubAssign = TokenType_SubtractAssign,
    Binary_MulAssign = TokenType_MultiplyAssign,
    Binary_DivAssign = TokenType_DivideAssign,
    Binary_ModAssign = TokenType_ModAssign,
    
    Binary_LeftShift = TokenType_LeftShift,
    Binary_RightShift = TokenType_RightShift,
    
    
    
} expression_binary_type;



typedef struct expression
{
    expression_type type;
    char *file;
    u64 line;
    union
    {
        struct
        {
            union
            {
                u64 integer;
                double real;
            };
            bool32 negative;
        };
        char *identifier, *string_literal, char_literal;
        struct //unary
        {
            expression_unary_type unary_type;
            struct expression *unary_expr;
        };
        struct //binary
        {
            expression_binary_type binary_type;
            struct expression *binary_left;
            struct expression *binary_right;
        };
        struct //ternary
        {
            struct expression *tern_cond;
            struct expression *tern_true_expr;
            struct expression *tern_false_expr;
        };
        struct //compound
        { 
            //NOTE should I just use a linked list here?
            //it's would simplify a lot of code but it will make structs bigger
            //compound expressions arn't that common, maybe it's best to leave it as is
            struct expression *compound_expr;
            struct expression *compound_next; //NOTE second gets resolved before first!
            struct expression *compound_tail;
        };
        struct //member access for structs, unions, enums
        {
            struct expression *struct_expr;
            char *struct_member_identifier; 
        };
        struct //call
        {
            struct expression *call_expr;
            struct expression *call_args; //NOTE compound expression
        };
        struct //array
        {
            struct expression *array_expr; //
            struct expression *array_index_expr;
        };
        struct //cast
        {
            type_specifier *casting_to;
            struct expression *cast_expression;
        };
        struct //NOTE should the parser just appropriately parse without this expression type
        {
            struct expression *parenthetical_expression;
        };
        
    };
} expression;

internal expression ResolvedExpression(expression *expr);


expression *expr_zero;
expression *expr_zero_struct;

#endif