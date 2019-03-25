#ifndef TOKENS_H
#define TOKENS_H
typedef enum{
    TokenType_Invalid = 0,
    TokenType_Identifier,
    TokenType_Keyword,
    TokenType_Qualifier,
    TokenType_StringLiteral,
    TokenType_CharacterLiteral,
    
    TokenType_Integer,
    TokenType_IntegerLong,
    TokenType_IntegerLongLong,
    
    TokenType_UnsignedInteger,
    TokenType_UnsignedIntegerLong,
    TokenType_UnsignedIntegerLongLong,
    
    TokenType_Float,
    TokenType_Float64,
    
    //Operator tokens...
    TokenType_AsciiBegin = '!',
    TokenType_AsciiEnd = '~',
    //^, &, |
    TokenType_And,
    TokenType_Or,
    
    TokenType_PlusPlus,
    TokenType_MinusMinus,
    TokenType_Equals,
    TokenType_NotEquals,
    TokenType_GreaterThanEquals,
    TokenType_LessThanEquals,
    
    TokenType_AddAssign,
    TokenType_SubtractAssign,
    TokenType_MultiplyAssign,
    TokenType_DivideAssign,
    TokenType_ModAssign,
    
    TokenType_BitwiseAndAssign,
    TokenType_BitwiseOrAssign,
    TokenType_BitwiseXorAssign,
    
    TokenType_RightShift,
    TokenType_LeftShift,
    //end operator
    
    //TokenType_PointerAccess,
    TokenType_Ellipsis,
    
    TokenType_EndOfStream,
}lexer_token_type;

typedef enum
{
    Keyword_Invalid = 0,
    Keyword_If,
    Keyword_Then,
    Keyword_Else,
    Keyword_While,
    Keyword_For,
    Keyword_Break,
    Keyword_Continue,
    Keyword_Return,
    Keyword_Defer,
    Keyword_Struct,
    Keyword_Union,
    Keyword_Enum,
    Keyword_Typedef,
    Keyword_Sizeof,
    Keyword_Offsetof,
    Keyword_Const,
    Keyword_Goto,
    Keyword_Inline,
    Keyword_NoInline,
    Keyword_Internal,
    Keyword_External,
    //Keyword_Static,
    
    
    
    
} keyword_type;

//TODO macronize this

static char *global_keywords[] = {"___reserved___", "if", "then", "else",
    "while", "for", "break", "continue", "return", "defer",
    "struct", "union", "enum", "typedef", "sizeof", "offsetof",
    "const", "goto", "inline", "no_inline", "internal", "external"};




typedef struct
{
    union
    {
        lexer_token_type type;
        char type_char;
    };
    char *file;
    u64 line;
    union
    {
        char *identifier, *string,  character;
        u32 qualifier;
        keyword_type keyword;
        u64 integer;
        double floating;
    };
} lexer_token;

typedef struct
{
    char *start;
    char *at;
    u64 line_at;
    u64 num_characters;
    //TODO
    char *file; //NOTE lexer parses contigious chars in memory, but I can annotate which file comes from which
    char *past_file;
    u64 past_line_at;
    
    lexer_token eaten;
    lexer_token peek;
}lexer_state;

//NOTE only EatToken advances token stream, Peek looks at current token, next at next token
internal lexer_token EatToken(lexer_state *lexer);
inline lexer_token PeekToken(lexer_state *lexer);
inline lexer_token NextToken(lexer_state *lexer);


internal lexer_token_type WillEatTokenType(lexer_state *lexer, lexer_token_type type);
internal keyword_type WillEatKeyword(lexer_state *lexer, keyword_type keyword);
internal void ExpectToken(lexer_state *lexer, lexer_token_type type);
internal void ExpectKeyword(lexer_state *lexer, keyword_type keyword);
inline bool32 TokenTypeInteger(lexer_token *token);
internal void PrintToken(lexer_token *token);





#define InvalidStringIndex ArrayCount(global_keywords)

//same must be followed by a #
static char *global_qualifiers[] = {"null", "empty"};

enum
{
    Qualifier_Null,
    Qualifier_Empty,
    Qualifier_Invalid,
};





#endif