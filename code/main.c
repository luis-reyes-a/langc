#include "stdint.h"

#define internal static
#define true 1
#define false 0
typedef uint32_t u32;
typedef uint64_t u64;
typedef u32 bool32;
#define u32_max UINT32_MAX
#define u32(cast) (u32)(cast)
#define u64(cast) (u64)(cast)
#define float(cast) (float)(cast)
#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

#define Crash do{ int *ptr = 0; *ptr = 1; }while(0)
#define Panic(...) Crash
#define Assert(expr) if(!(expr)) Panic("Assert fired!")

#include "stdio.h" //TODO get rid of printf
#include "stdlib.h" //TODO get rid of malloc

inline void 
PrintTabs(u32 tabs)
{
    while(tabs--)  printf("\n");
}


#include "memory_arena.h"
#include "strings.h"
#include "ast.h"

typedef struct
{
    char *ptr;
    char *at; //NOTE don't need this tbh
    u32 max_size;
    u32 size_written;
    u32 format_char_width;
    u32 chars_in_line;
    bool32 tried_to_break_already;
} writeable_text_buffer;

inline void
ResetTextBuffer(writeable_text_buffer *buffer)
{
    buffer->at = buffer->ptr;
    buffer->size_written = 0;
    buffer->tried_to_break_already = false;
}

internal u32
TypeSpecifierNumArrays(type_specifier *typespec)
{
    u32 result = 0;
    
    return result;
}



internal void
OutputChar(writeable_text_buffer *buffer, char c)
{
    if(buffer->size_written < buffer->max_size)
    {
        ++buffer->size_written;
        if(c == '\n')
            buffer->chars_in_line = 0;
        else
            ++buffer->chars_in_line;
        *buffer->at++ = c;
    }
    else Panic();
}

internal void
OutputCString(writeable_text_buffer *buffer, char *string)
{
    Assert(string);
    if(buffer->chars_in_line > buffer->format_char_width &&
       !buffer->tried_to_break_already)
    {
        buffer->tried_to_break_already = true;
        OutputChar(buffer, '\n');
        OutputCString(buffer, string);
    }
    else
    {
        if(buffer->tried_to_break_already)   buffer->tried_to_break_already = false;
        for(char c = *string; c; c = *++string)
        {
            OutputChar(buffer, c);
        }
    }
}

#define WrapperSwitchOpen(wrapper)\
switch(wrapper)\
{\
    case '(':\
    OutputChar(buffer, '(');\
    break;\
    \
    case '{':\
    OutputChar(buffer, '{');\
    break;\
    \
    case '[':\
    OutputChar(buffer, '[');\
    break;\
    \
    case '\"':\
    OutputChar(buffer, '\"');\
    break;\
    \
    case '\'':\
    OutputChar(buffer, '\'');\
    break;\
    \
    default: Panic();\
}

#define WrapperSwitchClose(wrapper)\
switch(wrapper)\
{\
    case '(':\
    OutputChar(buffer, ')');\
    break;\
    \
    case '{':\
    OutputChar(buffer, '}');\
    break;\
    \
    case '[':\
    OutputChar(buffer, ']');\
    break;\
    \
    case '\"':\
    OutputChar(buffer, '\"');\
    break;\
    \
    case '\'':\
    OutputChar(buffer, '\'');\
    break;\
    \
    default: Panic();\
}

inline void
OutputCStringW(writeable_text_buffer *buffer, char *string, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    OutputCString(buffer, string);
    WrapperSwitchClose(wrapper);
}

inline void
OutputCharW(writeable_text_buffer *buffer, char c, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    OutputChar(buffer, c);
    WrapperSwitchClose(wrapper);
}


inline u64
Pow64(u64 base, u64 times)
{
    if(times)
    {
        u64 result = 1;
        while(times--)
        {
            result *= base;
        }
        return result;
    }
    else return 1;
}

internal void
OutputItoa(writeable_text_buffer *buffer, u64 integer)
{
    if(integer == 0)  OutputChar(buffer, '0');
    else
    {
        u32 num_places = 0;
        for(u64 num = integer; num >= 1; num /= 10)  ++num_places;
        
        u64 copy = integer;
        for(u32 place = num_places; place; --place)
        {
            u64 power = Pow64(10, place - 1);
            u32 value = u32(copy / power);
            Assert(value >= 0 && value < 10);
            OutputChar(buffer, (char)('0' + value));
            copy -= (value * power);
        }
    }
}

internal void
OutputFtoa64(writeable_text_buffer *buffer, double value)
{
    
}

internal void
OutputTypeSpecifier(writeable_text_buffer *buffer, type_specifier *typespec)
{
    switch(typespec->type)
    {
        case TypeSpec_Internal:   
        switch(typespec->internal_type) //soft typechecking...
        {//NOTE ensure to include "stdint" or check compiler types for int long and short...
            case Internal_S8:      OutputCString(buffer, "int8_t"); break; 
            case Internal_S16:     OutputCString(buffer, "int16_t"); break;
            case Internal_S32:     OutputCString(buffer, "int32_t"); break;
            case Internal_S64:     OutputCString(buffer, "int64_t"); break;
            case Internal_U8:      OutputCString(buffer, "uint8_t"); break;
            case Internal_U16:     OutputCString(buffer, "uint16_t"); break;
            case Internal_U32:     OutputCString(buffer, "uint32_t"); break;
            case Internal_U64:     OutputCString(buffer, "uint64_t"); break;
            case Internal_Float:   OutputCString(buffer, "float"); break;
            case Internal_Double:  OutputCString(buffer, "double"); break;
            default: Panic();
        }
        break;
        
        case TypeSpec_StructUnion: case TypeSpec_Enum:   
        if(typespec->identifier)
            OutputCString(buffer, typespec->identifier);
        break;
        
        case TypeSpec_Ptr:   
        OutputTypeSpecifier(buffer, typespec->base_type);
        OutputChar(buffer, ' ');
        for(u32 i = 0; i < typespec->star_count; ++i)
        {
            OutputChar(buffer, '*');
        }
        break;
        
        
        case TypeSpec_Array:   
        {
            type_specifier *base = typespec->array_base;
            for(base; base->type == TypeSpec_Array; base = base->array_base);
            OutputTypeSpecifier(buffer, base); 
            //NOTE this will only output the base type, will not type any []'s...
        }
        
        break;
        
        case TypeSpec_Proc:   
        Panic(); //TODO
        break;
        
        default: Panic();
    }
}




internal bool32
ExpressionResolvesToPointer(expression *expr)
{
    return 0;
}

internal void OutputExpression(writeable_text_buffer *buffer, expression *expr);

internal void
OutputExpressionW(writeable_text_buffer *buffer, expression *expr, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    OutputExpression(buffer, expr);
    WrapperSwitchClose(wrapper);
}

internal void
OutputExpression(writeable_text_buffer *buffer, expression *expr)
{
    switch(expr->type)
    {
        case Expression_Identifier:   
        OutputCString(buffer, expr->identifier);
        break; 
        
        case Expression_StringLiteral:   
        OutputCStringW(buffer, expr->string_literal, '\"');
        break;
        
        case Expression_CharLiteral:   
        OutputCharW(buffer, expr->char_literal, '\'');
        break;
        
        case Expression_Integer:   
        OutputItoa(buffer, expr->integer_value);
        break;
        
        case Expression_RealNumber:   
        OutputFtoa64(buffer, expr->real_value);
        break;
        
        case Expression_Binary:   
        OutputExpression(buffer, expr->binary_left);
        switch(expr->binary_type)
        {
            case '%':
            OutputCString(buffer, " % ");
            break;
            
            case '+':   
            OutputCString(buffer, " + "); 
            break; 
            
            case '-':   
            OutputCString(buffer, " - "); 
            break;
            
            case '*':
            OutputCString(buffer, " * "); 
            break;
            
            case '/':   
            OutputCString(buffer, " / "); 
            break;
            
            case '>':
            OutputCString(buffer, " > "); 
            break;
            
            case '<':   
            OutputCString(buffer, " < "); 
            break;
            
            case Binary_GreaterThanEquals:   
            OutputCString(buffer, " >= "); 
            break;
            
            case Binary_LessThanEquals:   
            OutputCString(buffer, " <= "); 
            break;
            
            case Binary_Equals:   
            OutputCString(buffer, " == "); 
            break;
            
            case Binary_NotEquals:   
            OutputCString(buffer, " != "); 
            break;
            
            case Binary_And:   
            OutputCString(buffer, " && "); 
            break; 
            
            case Binary_Or:
            OutputCString(buffer, " || "); 
            break;
            
            //case '&':   OutputCString(buffer, ""); break; //TODO not supporting this!
            //case '|':   OutputCString(buffer, ""); break;
            //case '^':   OutputCString(buffer, ""); break;
            
            case '=':
            OutputCString(buffer, " = ");
            break; 
            
            case Binary_AndAssign:   
            OutputCString(buffer, " &= ");
            break;  
            
            case Binary_OrAssign:
            OutputCString(buffer, " |= "); 
            break;
            
            case Binary_XorAssign:
            OutputCString(buffer, " ^= "); 
            break;  //bitwise
            
            case Binary_AddAssign:
            OutputCString(buffer, " += "); 
            break;
            
            case Binary_SubAssign:
            OutputCString(buffer, " -= "); 
            break;
            
            case Binary_MulAssign:
            OutputCString(buffer, " *= "); 
            break;
            
            case Binary_DivAssign:
            OutputCString(buffer, " /= "); 
            break;
            
            case Binary_ModAssign:
            OutputCString(buffer, " %= "); 
            break;
            
            case Binary_LeftShift:
            OutputCString(buffer, " << "); 
            break;
            
            case Binary_RightShift:
            OutputCString(buffer, " >> "); 
            break;
            
            case Binary_None: default: Panic();
        }
        OutputExpression(buffer, expr->binary_right);
        break;
        
        case Expression_Unary:   
        if(expr->unary_type == Unary_PostIncrement)
        {
            OutputExpression(buffer, expr->unary_expr);
            OutputCString(buffer, "++");
        }
        else if(expr->unary_type == Unary_PostDecrement)
        {
            OutputExpression(buffer, expr->unary_expr);
            OutputCString(buffer, "--");
        }
        else
        {
            switch(expr->unary_type)
            {
                case '-': 
                OutputChar(buffer, '-');
                break;
                
                case Unary_PreIncrement:
                OutputCString(buffer, "++");
                break;
                
                case Unary_PreDecrement:  
                OutputCString(buffer, "--");
                break;
                
                case '!': 
                OutputChar(buffer, '!');
                break; 
                
                case'*':  
                OutputChar(buffer, '*');
                break; 
                
                case'&':  
                OutputChar(buffer, '&');
                break;
                
                default: Panic();
            }
            OutputExpression(buffer, expr->unary_expr);
        }
        break;
        
        case Expression_MemberAccess: 
        OutputExpression(buffer, expr->struct_expr);
        if(ExpressionResolvesToPointer(expr->struct_expr))
        {
            OutputCString(buffer, "->");
        }
        else
        {
            OutputChar(buffer, '.');
        }
        OutputCString(buffer, expr->struct_member_identifier);
        break;
        
        case Expression_Compound:   
        {
            OutputChar(buffer, '(');
            expression *outer = 0;
            for(outer = expr;
                outer && outer->type == Expression_Compound;
                outer = outer->compound_next)
            {
                OutputExpression(buffer, outer->compound_expr);
                OutputCString(buffer, ", ");
            }
            Assert(outer);
            OutputExpression(buffer, outer);
            OutputChar(buffer, ')');
        }
        
        break;
        
        case Expression_Ternary:   
        OutputExpressionW(buffer, expr->tern_cond, '(');
        OutputCString(buffer, " ? ");
        OutputExpressionW(buffer, expr->tern_true_expr, '(');
        OutputCString(buffer, " : ");
        OutputExpressionW(buffer, expr->tern_false_expr, '(');
        break;
        
        case Expression_Cast:  
        OutputChar(buffer, '(');
        OutputTypeSpecifier(buffer, expr->casting_to);
        OutputChar(buffer, ')');
        OutputExpressionW(buffer, expr->cast_expression, '(');
        break;
        
        case Expression_Call:   
        OutputExpression(buffer, expr->call_expr); //NOTE some expr types should be in parens...
        OutputExpression(buffer, expr->call_args); //NOTE compound expr!
        break;
        
        case Expression_ArraySubscript:   
        OutputExpression(buffer, expr->array_expr);
        OutputExpressionW(buffer, expr->array_index_expr, '[');
        break; 
        
        case Expression_None: default: Panic();
    }
}

internal void OutputDeclC(writeable_text_buffer *buffer, declaration *decl);



internal void
OutputStatement(writeable_text_buffer *buffer, statement *stmt)
{
    switch(stmt->type)
    {
        case Statement_Declaration: 
        for(declaration *decl = stmt->decl;
            decl;
            decl = decl->next)
        {
            OutputDeclC(buffer, decl);
        }
        
        //OutputChar(buffer, ';'); //TODO one here?
        break;
        
        case Statement_Expression:  
        OutputExpression(buffer, stmt->expr);
        OutputChar(buffer, ';');
        break;
        
        case Statement_If:  
        OutputCString(buffer, "if");
        OutputExpressionW(buffer, stmt->if_expr, '(');
        OutputStatement(buffer, stmt->if_stmt);
        break;
        
        case Statement_Else:  
        OutputCString(buffer, "else");
        OutputStatement(buffer, stmt->else_stmt);
        break;
        
        case Statement_Switch:  
        OutputCString(buffer, "switch");
        OutputExpressionW(buffer, stmt->switch_expr, '(');
        OutputStatement(buffer, stmt->else_stmt);
        break; 
        
        case Statement_SwitchCase:  
        OutputCString(buffer, "case ");
        OutputExpressionW(buffer, stmt->case_label, '(');
        OutputStatement(buffer, stmt->case_statement);
        break; 
        
        case Statement_While:  
        OutputCString(buffer, "while");
        OutputExpressionW(buffer, stmt->while_expr, '(');
        OutputStatement(buffer, stmt->while_stmt);
        break;
        
        case Statement_For:  
        OutputCString(buffer, "for");
        OutputChar(buffer, '(');
        OutputStatement(buffer, stmt->for_init_stmt);
        OutputExpression(buffer, stmt->for_expr2);
        OutputCString(buffer, "; ");
        OutputExpression(buffer, stmt->for_expr3);
        OutputCString(buffer, ")\n");
        OutputStatement(buffer, stmt->for_stmt);
        break; 
        
        case Statement_Defer:  
        OutputStatement(buffer, stmt->defer_stmt);
        break;
        
        case Statement_Goto:  
        Panic();
        break;
        
        case Statement_Break:  
        OutputCString(buffer, "break;\n");
        break;
        
        case Statement_Continue:  
        OutputCString(buffer, "continue;\n");
        break;
        
        case Statement_Return:  
        OutputCString(buffer, "return ");
        OutputExpression(buffer, stmt->return_expr);
        OutputCString(buffer, ";\n");
        break;
        
        case Statement_Compound:  
        {
            OutputCString(buffer, "{\n");
            statement *outer;
            for(outer = stmt;
                outer && outer->type == Statement_Compound;
                outer = outer->compound_next)
            {
                OutputStatement(buffer, outer->compound_stmt);
                //OutputCString(buffer, ";\n");
            }
            Assert(outer);
            OutputStatement(buffer, outer);
            OutputCString(buffer, "\n}\n");
        }
        
        break;
        
        case Statement_Null: 
        OutputChar(buffer, ';');
        break;
    }
}

internal void
OutputDeclC(writeable_text_buffer *buffer, declaration *decl)
{
    Assert(decl);
    switch(decl->type)
    {
        
        
        case Declaration_Variable: case Declaration_ProcedureArgs:   
        OutputTypeSpecifier(buffer, decl->typespec);//NOTE array [] after var 
        OutputChar(buffer, ' ');
        OutputCString(buffer, decl->identifier);
        //u32 num_array_subscripts = TypeSpecifierNumArrays(decl->typespec);
        for(type_specifier *top = decl->typespec; //NOTE output array subscripts
            top->type == TypeSpec_Array;
            top = top->array_base)
        {//TODO is this correct order in C?
            OutputChar(buffer, '[');
            OutputItoa(buffer, top->array_size);
            OutputChar(buffer, ']');
        }
        if(decl->type == Declaration_Variable) OutputCString(buffer, ";\n");
        
        break;//semicolon
        
        case Declaration_Typedef:   
        Panic();
        //TODO can you typedef arrays?
        break;//semicolon
        
        case Declaration_Struct: case Declaration_Union: case Declaration_Enum:  
        //NOTE structs can be nested into functions!!!
        if(decl->identifier)
        {
            OutputCString(buffer, "typedef ");
        }
        
        
        if(decl->type == Declaration_Struct) 
        {
            OutputCString(buffer, "struct ");
            if(decl->identifier) OutputCString(buffer, decl->identifier);
        }
        else if(decl->type == Declaration_Union) 
        {
            OutputCString(buffer, "union ");
            if(decl->identifier) OutputCString(buffer, decl->identifier);
        }
        else if(decl->type == Declaration_Enum)
        {
            OutputCString(buffer, "enum ");
        }
        
        OutputCString(buffer, "\n{\n");
        for(declaration *member = decl->members; //NOTE hack enum_members and members overlap in memory
            member;
            member = member->next)
        {
            OutputDeclC(buffer, member);
        }
        OutputCString(buffer, "\n}");
        if(decl->identifier) OutputCString(buffer, decl->identifier);
        OutputCString(buffer, ";\n\n");
        break;
        
        case Declaration_EnumMember:   
        OutputCString(buffer, decl->identifier);
        if(decl->const_expr)
        {
            OutputCString(buffer, " = ");
            OutputExpression(buffer, decl->const_expr);
        }
        OutputCString(buffer, ",\n");
        break;
        
        case Declaration_Procedure:   
        if(decl->proc_keyword == Keyword_Internal)  
            OutputCString(buffer, "static ");
        else if(decl->proc_keyword == Keyword_External)  
            OutputCString(buffer, "extern ");
        else if(decl->proc_keyword == Keyword_Inline)  
            OutputCString(buffer, "inline "); //TODO compiler specific force inline
        else if(decl->proc_keyword == Keyword_NoInline)  ;
        //TODO compiler specific force no_inline
        else Panic();
        OutputTypeSpecifier(buffer, decl->proc_return_type);
        OutputChar(buffer, '\n');
        OutputCString(buffer, decl->identifier); //TODO function_overloading!!!
        OutputChar(buffer, '(');
        for(declaration *arg = decl->proc_args;
            arg;
            arg = arg->next)
        {
            Assert(arg->type == Declaration_ProcedureArgs);
            //arg->type = Declaration_Variable; //NOTE hack!
            OutputDeclC(buffer, arg); //TODO optional param!
            //arg->type = Declaration_ProcedureArgs;
            if(!arg->next == 0)
                OutputCString(buffer, ", ");
        }
        OutputCString(buffer, ")\n");
        OutputStatement(buffer, decl->proc_body);
        OutputChar(buffer, '\n');
        break;
        
        
        
        /* NOTE should i keep this?
        case Declaration_ProcedureArgs:   
        decl->type == Declaration_Variable;
        break;
        */
        
        default: Panic();
    }
}

internal void
OutputC(declaration *ast, char *filename)
{
    FILE *file = fopen(filename, "w");
    writeable_text_buffer buffer;
    buffer.max_size = Kilobytes(512);
    buffer.size_written = 0;
    buffer.ptr = malloc(buffer.max_size);
    buffer.at = buffer.ptr;
    buffer.format_char_width = 80;
    buffer.tried_to_break_already = false;
    
    if(file)
    {
        for(declaration *decl = ast;
            decl;
            decl = decl->next)
        {
            OutputDeclC(&buffer, decl);
            OutputChar(&buffer, '\n'); //top level decls only
            fwrite(buffer.ptr, buffer.size_written, 1, file);
            ResetTextBuffer(&buffer);
        }
        
        fclose(file);
    }
}



int main(int argc, char **argv)
{
    memory_arena big_arena;
    big_arena.size = Gigabytes(2);
    big_arena.base = malloc(big_arena.size);
    big_arena.used = 0;
    
    ast_arena = PushMemoryArena(&big_arena, ast_arena_size);
    string_intern_table.entries = 0;
    string_intern_table.arena = PushMemoryArena(&big_arena, Megabytes(512));
    
    
    AddInternalType(StringInternLiteral("s8"), Internal_S8); 
    AddInternalType(StringInternLiteral("s16"), Internal_S16);
    AddInternalType(StringInternLiteral("s32"), Internal_S32);
    AddInternalType(StringInternLiteral("s64"), Internal_S64);
    AddInternalType(StringInternLiteral("u8"), Internal_U8);
    AddInternalType(StringInternLiteral("u16"), Internal_U16);
    AddInternalType(StringInternLiteral("u32"), Internal_U32);
    AddInternalType(StringInternLiteral("u64"), Internal_U64);
    AddInternalType(StringInternLiteral("float"), Internal_Float);
    AddInternalType(StringInternLiteral("double"), Internal_Double);
    AddInternalType(StringInternLiteral("char"), Internal_U8);
    
    FILE *file = fopen("../code/test.cc", "r");
    declaration *ast = 0;
    if(file)
    {
        fseek(file, 0, SEEK_END);
        u32 file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        lexer_state lexer;
        lexer.start = malloc(file_size);
        lexer.at = lexer.start;
        lexer.line_at = 0;
        lexer.num_characters = file_size;
        lexer.file = "test.cc";
        lexer.past_file = lexer.file;
        
        fread(lexer.start, file_size, 1, file);
        
        ast = ParseAST(&lexer);
        
        fclose(file);
    }
    if(ast)
    {
        OutputC(ast, "../code/RESULT.c");
    }
    return 0;
}
