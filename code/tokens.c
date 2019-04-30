#include "tokens.h"

#define case2(c1, c2, t2) case c1: \
{ \
    if(lexer->at[1] == c2)  \
    { \
        result.type = t2;  \
        lexer->at += 2;  \
    }  \
    else \
    {  \
        result.type = c1;  \
        ++lexer->at;  \
    }  \
}

#define case3(c1, c2, t2, c3, t3) case c1:\
{\
    if(lexer->at[1] == c2)\
    {\
        result.type = t2;\
        lexer->at += 2;\
    }\
    else if(lexer->at[1] == c3)\
    {\
        result.type = t3;\
        lexer->at += 2;\
    }\
    else\
    {\
        result.type = c1;\
        ++lexer->at;\
    }\
}


#define case_digit case'0':case'1':case'2':case'3': \
case'4':case'5':case'6':case'7':case'8':case'9'



//TODO file and location
internal lexer_token //MOVE to next token and returns it
EatToken(lexer_state *lexer)
{
    lexer_token error = {0};
    lexer_token result = error;
    //TODO move pass whitespace
    
    if(u64(lexer->at - lexer->start) > lexer->num_characters)
    {
        
        return error;
    }
    
    //NOTE do I need more...
    while(*lexer->at == ' ' || *lexer->at == '\n' || *lexer->at == '\t' ||
          *lexer->at == '\v' || *lexer->at == '\f' || *lexer->at == '\r')
    {
        if(*lexer->at == '\n') //also use \r????
        {
            ++lexer->line_at;
        }
        ++lexer->at;
    }
    
    result.line = lexer->line_at;
    result.file = 0; //TODO
    
    
#if 0 //TODO preprocessor
    if(StringMatchesLiteral(lexer->at, "define ")) 
    {
        
    }
    if(StringMatchesLiteral(lexer->at, "include ")) 
    {
        
    }
    else if(StringMatchesLiteral(lexer->at, "else") ||
            StringMatchesLiteral(lexer->at, "ELSE"))
    {
        
    }
    else if(StringMatchesLiteral(lexer->at, "else") ||
            StringMatchesLiteral(lexer->at, "ELSE"))
    {
        
    }
    else if(StringMatchesLiteral(lexer->at, "if ") ||
            StringMatchesLiteral(lexer->at, "IF ")) 
    {
        
    }
    else if(StringMatchesLiteral(lexer->at, "else") ||
            StringMatchesLiteral(lexer->at, "ELSE"))
    {
        
    }
#endif
    
    switch(*lexer->at)
    {
        //Single character case
        case '#': case '(':case ')':case '{':case '}':case '[':case ']':case ',':case ';':
        case ':':case '~': result.type = *lexer->at; ++lexer->at; break;
        case2('!', '=', TokenType_NotEquals)break;
        case2('^', '=', TokenType_BitwiseXorAssign)break;
        case2('*', '=', TokenType_MultiplyAssign)break;
        case2('%', '=', TokenType_ModAssign)break;
        case2('=', '=', TokenType_Equals)break;
        case3('>', '>', TokenType_RightShift,'=', TokenType_GreaterThanEquals)break;
        case3('<', '<', TokenType_LeftShift, '=', TokenType_LessThanEquals)break;
        case3('&', '&', TokenType_And, '=', TokenType_BitwiseAndAssign)break;
        case3('|', '|', TokenType_Or, '=', TokenType_BitwiseOrAssign)break;
        case3('+', '+', TokenType_PlusPlus,'=', TokenType_AddAssign)break;
        case '-':{
            if(lexer->at[1] == '=')
            {
                result.type = TokenType_SubtractAssign;
                lexer->at += 2;
            }
            /*
            else if(lexer->at[1] == '>')
            {
                result.type = TokenType_PointerAccess;
                lexer->at += 2;
            }
            */
            else if(lexer->at[1] == '-')
            {
                result.type = TokenType_MinusMinus;
                lexer->at += 2;
            }
            
            else
            {
                result.type = '-';
                ++lexer->at;
            }
            
        }break;
        case '.': {
            if(lexer->at[1] == '.' && lexer->at[2] =='.')
            {
                result.type = TokenType_Ellipsis;
                lexer->at += 3;
            }
            else
            {
                result.type = '.';
                ++lexer->at;
            }
        }break; 
        case '/':{
            if(lexer->at[1] == '=')
            {
                result.type = TokenType_DivideAssign;
                lexer->at += 2;
            }
            
            else if(lexer->at[1] == '/') //c++ comment
            {
                while(*lexer->at != '\n') ++lexer->at;
                result = EatToken(lexer);
            }
            else if(lexer->at[1] == '*') //c comment 
            {
                for(u32 balance = 0 ;;)
                {
                    if(lexer->at[0] == '/' && lexer->at[1] == '*')
                    {
                        ++balance;
                    }
                    else if(lexer->at[0] == '*' && lexer->at[1] == '/')
                    {
                        --balance;
                    }
                    
                    if(balance == 0)
                    {
                        break;
                    }
                    ++lexer->at;
                }
                lexer->at += 2;
                result = EatToken(lexer);
            }
            else 
            {
                result.type = '/';
                ++lexer->at;
            }
            
            
        }break;
        case '\"': //string literal
        { //TODO TODO handle escape sequences!
            ++lexer->at;
            char *begin = lexer->at;
            u32 length = 0;
            while(*lexer->at != '\"') {  ++length; ++lexer->at;}
            result.type = TokenType_StringLiteral;
            result.string = StringIntern(begin, length);
            ++lexer->at;
        }break;
        case '\'':
        {
            ++lexer->at;
            result.type = TokenType_CharacterLiteral;
            if(*lexer->at == '\\')
            {
                ++lexer->at;
                switch(*lexer->at)
                {
                    case 'n': result.character = '\n';break;
                    case 'r': result.character = '\r';break;
                    case 't': result.character = '\t';break;
                    case 'b': result.character = '\b';break;
                    case 'v': result.character = '\v';break;
                    case 'a': result.character = '\a';break;
                    case 'f': result.character = '\f';break;
                    case '\\': result.character = '\\';break;
                    case '\?': result.character = '\?';break;
                    case '\'': result.character = '\'';break;
                    case '\"': result.character = '\"';break;
                    default: Panic(); //TODO logging!
                }
                ++lexer->at;
            }
            else
            {
                result.character = *lexer->at;
                ++lexer->at;
            }
            ++lexer->at;
        }break;
        
        /*case '#': //must be qualifer, preproccesor done up top!
        {
            ++lexer->at;
            string_array_search_result search = IndexInStringArray(lexer->at, global_qualifiers, ArrayCount(global_qualifiers));
            if(search.length)
            {
                result.type = TokenType_Qualifier;
                //result.qualifier = global_qualifiers[search.index];
                result.qualifier = search.index;
                lexer->at += search.length;
            }
            else
            {
                //TODO syntax error unrecognized hash command
                Panic();
            }
            
        }break;*/
        case_digit:{
            if(*lexer->at == '0')
            {
                if(IsDigit(lexer->at[1])) //TODO parse octal number
                {
                    ++lexer->at;
                    u32 length = 0;
                    char *start = lexer->at;
                    while(IsDigit(*lexer->at)) { ++lexer->at; ++length;}
                    result.type = TokenType_Integer;
                    result.integer = AtoiOctal(start, length);
                    lexer->at += length;
                }
                else if(lexer->at[1] == 'x')//TODO parse hex number
                {
                    lexer->at += 2;
                    u32 length = 0;
                    char *start = lexer->at;
                    while(IsDigit(*lexer->at) || (*lexer->at >= 'a' && *lexer->at <= 'f') || 
                          (*lexer->at >= 'A' && *lexer->at <= 'F')) 
                    { 
                        ++lexer->at; ++length;
                    }
                    result.type = TokenType_Integer;
                    result.integer = AtoiOctal(start, length);
                    lexer->at += length;
                }
                else
                {
                    //NOTE should be zero but should check for errors...
                    //E.g. 0a should throw an error
                    result.type = TokenType_Integer;
                    result.integer = 0;
                    ++lexer->at;
                }
            }
            else
            {
                
                bool32 decimal_number = false;
                result.type = TokenType_Integer;
                result.integer = 0;
                u32 decimal_place = 10;
                while(IsDigit(*lexer->at) || *lexer->at == '.')
                {
                    if(*lexer->at == '.')
                    {
                        if(decimal_number)
                        {
                            while(IsDigit(*lexer->at) || *lexer->at == '.') ++lexer->at;
                            result = error;
                            break;
                        }
                        else
                        {
                            decimal_number = true;
                            result.type = TokenType_Float;
                            result.floating = (float)result.integer;
                        }
                        
                    }
                    else
                    {
                        if(!decimal_number)
                        {
                            result.integer *= 10;
                            result.integer += (*lexer->at - '0');
                        }
                        else
                        {
                            result.floating += ((*lexer->at - '0') / float(decimal_place));
                            decimal_place *= 10;
                        }
                    }
                    ++lexer->at;
                }
                
                if(!decimal_number)
                {
                    if(*lexer->at == 'u' || *lexer->at == 'U')
                    {
                        result.type += 3; //NOTE HACK!!!
                        ++lexer->at;
                    }
                    if(*lexer->at == 'l' || *lexer->at == 'L')
                    {
                        ++result.type;
                        ++lexer->at;
                    }
                    if(*lexer->at == 'l' || *lexer->at == 'L')
                    {
                        ++result.type;
                        ++lexer->at;
                    }
                }
                else //NOTE maybe the default is float, or use type inference to see if float or double
                {
                    if(*lexer->at == 'f' || *lexer->at == 'F')
                    {
                        result.type = TokenType_Float;
                    }
                }
                
            }
        }break;
        case 0:
        {
            result.type = TokenType_EndOfStream;
            ++lexer->at;
        }break;
        
        default: //either identifier or keyword
        {
            string_array_search_result search = IndexInStringArray(lexer->at, global_keywords, ArrayCount(global_keywords));
            if(search.length)
            {
                result.type = TokenType_Keyword;
                result.keyword = search.index;
                lexer->at += search.length;
                break;
            }
            else
            {
                result.type = TokenType_Identifier;
                char *start = lexer->at;
                
                if(*lexer->at == 'h')
                {
                    u32 stop_here = 0;
                    stop_here++;
                }
                u32 length = 0;
                while(IsAlphaNumeric(*lexer->at) || *lexer->at == '_' || 
                      (lexer->at[0] == '-' && IsAlphaNumeric(lexer->at[1]))) 
                {
                    ++length; ++lexer->at;
                }
                
                result.string = StringIntern(start, length);
            }
        }break;
        
        
    }
    lexer->eaten = result;
    return result;
}
#undef case2
#undef case3
#undef case_digit

inline lexer_token
PeekToken(lexer_state *lexer)
{
    lexer_state temp = *lexer;
    lexer_token result = EatToken(lexer);
    *lexer = temp;
    lexer->peek = result;
    return result;
}

inline lexer_token
NextToken(lexer_state *lexer) 
{
    lexer_state temp = *lexer;
    EatToken(lexer);
    lexer_token result = PeekToken(lexer); 
    *lexer = temp;
    lexer->next = result;
    return result;
}

internal void
PrintToken(lexer_token *token)
{
    switch(token->type)
    {
        case TokenType_Identifier:  
        printf("Identifier:%s\n", token->identifier);
        break;
        case TokenType_Keyword:  
        printf("Keyword:");
        switch(token->keyword)
        {
            //case Keyword_Static: printf("static\n"); break;
            case Keyword_If: printf("if\n"); break;
            case Keyword_Else: printf("else\n"); break;
            case Keyword_Struct: printf("struct\n"); break;
            case Keyword_Union: printf("union\n"); break;
            case Keyword_Enum: printf("enum\n"); break;
            case Keyword_Typedef: printf("typedef\n"); break;
            case Keyword_Sizeof: printf("sizeof\n"); break;
            case Keyword_Offsetof: printf("offsetof\n"); break;
            //case Keyword_Const: printf("const\n"); break;
            case Keyword_Let: printf("let\n"); break;
            case Keyword_Goto: printf("goto\n"); break;
            case Keyword_Inline: printf("inline\n"); break;
            case Keyword_NoInline: printf("no_inline\n"); break;
            case Keyword_Internal: printf("internal\n"); break;
            case Keyword_External: printf("external\n"); break;
            default: Panic();
        }
        break;
        case TokenType_Qualifier: 
        printf("Qualifier:");
        switch(token->qualifier)
        {
            case Qualifier_Null: printf("null\n"); break;
            case Qualifier_Empty: printf("empty\n"); break;
            default: Panic();
        }
        break;
        case TokenType_StringLiteral: 
        printf("String Literal:%s\n", token->string);
        break;
        case TokenType_CharacterLiteral: 
        printf("Char Literal:%c\n", token->character);
        break;
        case TokenType_Integer: 
        printf("Integer:%d\n", (int)token->integer);
        break;
        case TokenType_Float:  
        printf("Float:%f\n", token->floating);
        break;
        case TokenType_And:  
        printf("&&\n");
        break;
        case TokenType_Or:
        printf("||\n");
        break;
        case TokenType_NotEquals:  
        printf("!=\n");
        break;
        case TokenType_MultiplyAssign:  
        printf("*=\n");
        break;
        case TokenType_DivideAssign:  
        printf("/=\n");
        break;
        case TokenType_AddAssign:  
        printf("+=\n");
        break;
        case TokenType_SubtractAssign:  
        printf("-=\n");
        break;
        case TokenType_PlusPlus:  
        printf("++\n");
        break;
        case TokenType_MinusMinus:  
        printf("--\n");
        break;
        case TokenType_Equals:  
        printf("==\n");
        break;
        case TokenType_GreaterThanEquals:  
        printf(">=\n");
        break;
        case TokenType_LessThanEquals:  
        printf("<=\n");
        break;
        case TokenType_BitwiseXorAssign:  
        printf("^=\n");
        break;
        case TokenType_BitwiseAndAssign:  
        printf("&=\n");
        break;
        case TokenType_BitwiseOrAssign:  
        printf("|=\n");
        break;
        case TokenType_RightShift:  
        printf(">>\n");
        break;
        case TokenType_LeftShift:  
        printf("<<\n");
        break;
        /*
        case TokenType_PointerAccess:  
        printf("->\n");
        break;*/
        case TokenType_Ellipsis:  
        printf("...\n");
        break;
        default: 
        {
            if(token->type >= TokenType_AsciiBegin &&
               token->type <= TokenType_AsciiEnd)
            {
                printf("%c\n", token->type);
            }
            else
            {
                Panic();
            }
        } break;
    }
}


internal int
WillEatTokenType(lexer_state *lexer, lexer_token_type type)
{
    Assert(type != TokenType_Invalid);
    if(PeekToken(lexer).type == type)
    {
        EatToken(lexer);
        return true;
    }
    else return false;
}


internal int
WillEatKeyword(lexer_state *lexer, keyword_type keyword)
{
    Assert(keyword != Keyword_Invalid);
    if(PeekToken(lexer).type == TokenType_Keyword &&
       lexer->peek.keyword == keyword)
    {
        EatToken(lexer);
        return true;
    }
    else return false;
}

internal void
ExpectToken(lexer_state *lexer, lexer_token_type type)
{
    if(!WillEatTokenType(lexer, type))
    {
        SyntaxError(lexer->file, lexer->line_at, "Was expecting token %c", (char)type);
    }
}

internal void
ExpectKeyword(lexer_state *lexer, keyword_type keyword)
{
    if(!WillEatKeyword(lexer, keyword))
    {
        SyntaxError(lexer->file, lexer->line_at, "Was expecting keyword %s", global_keywords[keyword]);
    }
}

internal char *
ExpectIdentifier(lexer_state *lexer)
{
    char *identifier = 0;
    if(WillEatTokenType(lexer, TokenType_Identifier))
    {
        identifier = lexer->eaten.identifier;
        Assert(identifier);
    }
    else
    {
        SyntaxError(lexer->file, lexer->line_at, "Was expecting an identifier");
    }
    return identifier;
}

