#include "cout.h"

inline void
ResetTextBuffer(writeable_text_buffer *buffer)
{
    buffer->at = buffer->ptr;
    buffer->size_written = 0;
    buffer->tried_to_break_already = false;
}

#define WrapperSwitchOpen(wrapper)\
switch(wrapper)\
{\
    case 0: case ' ':\
    break;\
    \
    case '(':\
    OutputChar(buffer, '(', 0);\
    break;\
    \
    case '{':\
    OutputChar(buffer, '{', 0);\
    break;\
    \
    case '[':\
    OutputChar(buffer, '[', 0);\
    break;\
    \
    case '\"':\
    OutputChar(buffer, '\"', 0);\
    break;\
    \
    case '\'':\
    OutputChar(buffer, '\'', 0);\
    break;\
    \
    default: Panic();\
}

#define WrapperSwitchClose(wrapper)\
switch(wrapper)\
{\
    case 0:\
    break;\
    \
    case ' ':\
    OutputChar(buffer, ' ', 0);\
    break;\
    \
    case '(':\
    OutputChar(buffer, ')', 0);\
    break;\
    \
    case '{':\
    OutputChar(buffer, '}', 0);\
    break;\
    \
    case '[':\
    OutputChar(buffer, ']', 0);\
    break;\
    \
    case '\"':\
    OutputChar(buffer, '\"', 0);\
    break;\
    \
    case '\'':\
    OutputChar(buffer, '\'', 0);\
    break;\
    \
    default: Panic();\
}


internal void
OutputChar(writeable_text_buffer *buffer, char c, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    if(buffer->size_written < buffer->max_size)
    {
        ++buffer->size_written;
        if(c == '\n')
            buffer->chars_in_line = 0;
        else
            ++buffer->chars_in_line;
        *buffer->at++ = c;
    }
    else Panic(); //TODO dynamic memory allocation
    WrapperSwitchClose(wrapper);
}

internal void
OutputCString(writeable_text_buffer *buffer, char *string, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    Assert(string);
    if(buffer->chars_in_line > buffer->format_char_width &&
       !buffer->tried_to_break_already)
    {
        buffer->tried_to_break_already = true;
        OutputChar(buffer, '\n', 0);
        OutputCString(buffer, string, 0);
    }
    else
    {
        if(buffer->tried_to_break_already)   buffer->tried_to_break_already = false;
        for(char c = *string; c; c = *++string)
        {
            OutputChar(buffer, c, 0);
        }
    }
    WrapperSwitchClose(wrapper);
}

internal void
OutputInteger64(writeable_text_buffer *buffer, u64 integer)
{
    if(integer == 0)  OutputChar(buffer, '0', 0);
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
            OutputChar(buffer, (char)('0' + value), 0);
            copy -= (value * power);
        }
    }
}

internal void
OutputFloat64(writeable_text_buffer *buffer, double value)
{
    
}

internal void
OutputTypeSpecifier(writeable_text_buffer *buffer, type_specifier *typespec, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    switch(typespec->type)
    {
        case TypeSpec_Internal:   
        switch(typespec->internal_type) //soft typechecking...
        {//NOTE ensure to include "stdint" or check compiler types for int long and short...
            case Internal_S8:      OutputCString(buffer, "int8_t",0); break; 
            case Internal_S16:     OutputCString(buffer, "int16_t",0); break;
            case Internal_S32:     OutputCString(buffer, "int32_t",0); break;
            case Internal_S64:     OutputCString(buffer, "int64_t",0); break;
            case Internal_U8:      OutputCString(buffer, "uint8_t",0); break;
            case Internal_U16:     OutputCString(buffer, "uint16_t",0); break;
            case Internal_U32:     OutputCString(buffer, "uint32_t",0); break;
            case Internal_U64:     OutputCString(buffer, "uint64_t",0); break;
            case Internal_Float:   OutputCString(buffer, "float",0); break;
            case Internal_Double:  OutputCString(buffer, "double",0); break;
            default: Panic();
        }
        break;
        
        case TypeSpec_StructUnion: case TypeSpec_Enum:   
        if(typespec->identifier)
            OutputCString(buffer, typespec->identifier, 0);
        break;
        
        case TypeSpec_Ptr:   
        OutputTypeSpecifier(buffer, typespec->ptr_base_type, ' ');
        for(u32 i = 0; i < typespec->star_count; ++i)
        {
            OutputChar(buffer, '*', 0);
        }
        break;
        
        
        case TypeSpec_Array:   
        {
            OutputTypeSpecifier(buffer, typespec->array_base_type ,0); 
            //NOTE this will only output the base type, will not type any []'s...
        }
        
        break;
        
        case TypeSpec_Proc:   
        Panic(); //TODO
        break;
        
        default: Panic();
    }
    WrapperSwitchClose(wrapper);
}

internal void
OutputExpression(writeable_text_buffer *buffer, expression *expr, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    switch(expr->type)
    {
        case Expression_Identifier:   
        OutputCString(buffer, expr->identifier, 0);
        break; 
        
        case Expression_StringLiteral:   
        OutputCString(buffer, expr->string_literal, '\"');
        break;
        
        case Expression_CharLiteral:   
        OutputChar(buffer, expr->char_literal, '\'');
        break;
        
        case Expression_Integer:   
        OutputInteger64(buffer, expr->integer);
        break;
        
        case Expression_RealNumber:   
        OutputFloat64(buffer, expr->real);
        break;
        
        case Expression_Binary:   
        OutputExpression(buffer, expr->binary_left, 0);
        switch(expr->binary_type)
        {
            case '%': OutputCString(buffer, " % ", 0); break;
            
            case '+': OutputCString(buffer, " + ", 0); break; 
            
            case '-': OutputCString(buffer, " - ", 0); break;
            
            case '*': OutputCString(buffer, " * ", 0); break;
            
            case '/': OutputCString(buffer, " / ", 0); break;
            
            case '>': OutputCString(buffer, " > ", 0); break;
            
            case '<': OutputCString(buffer, " < ", 0); break;
            
            case Binary_GreaterThanEquals: OutputCString(buffer, " >= ", 0); break;
            
            case Binary_LessThanEquals: OutputCString(buffer, " <= ", 0); break;
            
            case Binary_Equals: OutputCString(buffer, " == ", 0); break;
            
            case Binary_NotEquals: OutputCString(buffer, " != ", 0); break;
            
            case Binary_And: OutputCString(buffer, " && ", 0); break; 
            
            case Binary_Or: OutputCString(buffer, " || ", 0); break;
            
            case '=': OutputCString(buffer, " = ", 0); break; 
            
            case Binary_AndAssign: OutputCString(buffer, " &= ", 0);break;  
            
            case Binary_OrAssign: OutputCString(buffer, " |= ", 0); break;
            
            case Binary_XorAssign: OutputCString(buffer, " ^= ", 0); break;  //bitwise
            
            case Binary_AddAssign: OutputCString(buffer, " += ", 0); break;
            
            case Binary_SubAssign: OutputCString(buffer, " -= ", 0); break;
            
            case Binary_MulAssign: OutputCString(buffer, " *= ", 0); break;
            
            case Binary_DivAssign: OutputCString(buffer, " /= ", 0); break;
            
            case Binary_ModAssign: OutputCString(buffer, " %= ", 0); break;
            
            case Binary_LeftShift: OutputCString(buffer, " << ", 0); break;
            
            case Binary_RightShift: OutputCString(buffer, " >> ", 0);break;
            
            case Binary_None: default: Panic();
        }
        OutputExpression(buffer, expr->binary_right, 0);
        break;
        
        case Expression_Unary:   
        if(expr->unary_type == Unary_PostIncrement)
        {
            OutputExpression(buffer, expr->unary_expr, 0);
            OutputCString(buffer, "++", 0);
        }
        else if(expr->unary_type == Unary_PostDecrement)
        {
            OutputExpression(buffer, expr->unary_expr, 0);
            OutputCString(buffer, "--", 0);
        }
        else
        {
            switch(expr->unary_type)
            {
                case '-': 
                OutputChar(buffer, '-', 0);
                break;
                
                case Unary_PreIncrement:
                OutputCString(buffer, "++", 0);
                break; 
                
                case Unary_PreDecrement:  
                OutputCString(buffer, "--", 0);
                break;
                
                case '!': 
                OutputChar(buffer, '!', 0);
                break; 
                
                case'*':  
                OutputChar(buffer, '*', 0);
                break; 
                
                case'&':  
                OutputChar(buffer, '&', 0);
                break;
                
                default: Panic();
            }
            OutputExpression(buffer, expr->unary_expr, 0);
        }
        break;
        
        case Expression_MemberAccess: 
        OutputExpression(buffer, expr->struct_expr, 0);
        if(ExpressionResolvesToPointer(expr->struct_expr))
        {
            OutputCString(buffer, "->", 0);
        }
        else
        {
            OutputChar(buffer, '.', 0);
        }
        OutputCString(buffer, expr->struct_member_identifier, 0);
        break;
        
        case Expression_Compound:   
        {
            OutputChar(buffer, '(', 0);
            expression *outer;
            for(outer = expr;
                outer && outer->type == Expression_Compound;
                outer = outer->compound_next)
            { //NOTE allow null expression pointers?
                Assert(outer->compound_expr);
                OutputExpression(buffer, outer->compound_expr, 0);
                OutputCString(buffer, ", ", 0);
                
            }
            Assert(outer);
            OutputExpression(buffer, outer, 0);
            OutputChar(buffer, ')', 0);
        }
        break;
        
        case Expression_CompoundInitializer:
        {
            OutputChar(buffer, '{', 0);
            expression *outer;
            for(outer = expr;
                outer && outer->type == Expression_CompoundInitializer;
                outer = outer->compound_next)
            {
                Assert(outer->compound_expr);
                OutputExpression(buffer, outer->compound_expr, 0);
                if(outer->compound_next)
                {
                    OutputCString(buffer, ", ", 0);
                }
                
            }
            if(outer) //for single element initializers!
            {
                OutputExpression(buffer, outer->compound_expr, 0);
            }
            OutputChar(buffer, '}', 0);
        }break;
        
        case Expression_Ternary:   
        OutputExpression(buffer, expr->tern_cond, '(');
        OutputCString(buffer, " ? ", 0);
        OutputExpression(buffer, expr->tern_true_expr, '(');
        OutputCString(buffer, " : ", 0);
        OutputExpression(buffer, expr->tern_false_expr, '(');
        break;
        
        case Expression_Cast:  
        OutputTypeSpecifier(buffer, expr->casting_to, '(');
        OutputExpression(buffer, expr->cast_expression, '(');
        break;
        
        case Expression_Call:   
        OutputExpression(buffer, expr->call_expr, 0); //NOTE some expr types should be in parens...
        if(expr->call_args)
            OutputExpression(buffer, expr->call_args, 0); //NOTE compound expr!
        else OutputCString(buffer, "( )", 0);
        break;
        
        case Expression_ArraySubscript:   
        OutputExpression(buffer, expr->array_expr, 0);
        OutputExpression(buffer, expr->array_index_expr, '[');
        break; 
        
        
        
        case Expression_None: 
        case Expression_ToBeDefined:
        default: 
        Panic();
    }
    WrapperSwitchClose(wrapper);
}





internal void
OutputStatement(writeable_text_buffer *buffer, statement *stmt, char wrapper)
{
    WrapperSwitchOpen(wrapper);
    switch(stmt->type)
    {
        case Statement_Declaration: 
        for(declaration *decl = stmt->decl;
            decl;
            decl = decl->next)
        {
            OutputDeclC(buffer, decl);
        }
        break;
        
        case Statement_Expression:  
        OutputExpression(buffer, stmt->expr, 0);
        OutputCString(buffer, ";\n", 0);
        break;
        
        case Statement_If:  
        OutputCString(buffer, "if", 0);
        OutputExpression(buffer, stmt->if_expr, '(');
        OutputStatement(buffer, stmt->if_stmt, 0);
        break;
        
        case Statement_Else:  
        OutputCString(buffer, "else", 0);
        OutputStatement(buffer, stmt->else_stmt, 0);
        break;
        
        case Statement_Switch:  
        OutputCString(buffer, "switch", 0);
        OutputExpression(buffer, stmt->switch_expr, '(');
        OutputStatement(buffer, stmt->switch_cases, 0);
        break; 
        
        case Statement_SwitchCase:  
        if(stmt->case_label, 0)
        {
            OutputCString(buffer, "case ", 0);
            OutputExpression(buffer, stmt->case_label, '(');
        }
        else
        {
            OutputCString(buffer, "default", 0);
        }
        
        OutputChar(buffer, ':', 0);
        OutputStatement(buffer, stmt->case_statement, 0);
        OutputCString(buffer, "break;\n", 0);
        break; 
        
        case Statement_While:  
        OutputCString(buffer, "while", 0);
        OutputExpression(buffer, stmt->while_expr, '(');
        OutputStatement(buffer, stmt->while_stmt, 0);
        break;
        
        case Statement_For:  
        OutputCString(buffer, "for", 0);
        OutputChar(buffer, '(', 0);
        OutputStatement(buffer, stmt->for_init_stmt, 0);
        OutputExpression(buffer, stmt->for_expr2, 0);
        OutputCString(buffer, "; ", 0);
        OutputExpression(buffer, stmt->for_expr3, 0);
        OutputCString(buffer, ", 0)\n", 0);
        OutputStatement(buffer, stmt->for_stmt, 0);
        break; 
        
        case Statement_Defer:  
        OutputStatement(buffer, stmt->defer_stmt, 0);
        break;
        
        case Statement_Goto:  
        Panic();
        break;
        
        case Statement_Break:  
        OutputCString(buffer, "break;\n", 0);
        break;
        
        case Statement_Continue:  
        OutputCString(buffer, "continue;\n", 0);
        break;
        
        case Statement_Return:  
        OutputCString(buffer, "return ", 0);
        OutputExpression(buffer, stmt->return_expr, 0);
        OutputCString(buffer, ";\n", 0);
        break;
        
        case Statement_Compound:  
        {
            OutputCString(buffer, "{\n", 0);
            for(statement *inner = stmt->compound_stmts;
                inner;
                inner = inner->next)
            {
                OutputStatement(buffer, inner, 0);
            }
            OutputCString(buffer, "}\n", 0);
        }
        
        break;
        
        case Statement_Null: 
        OutputChar(buffer, ';', 0);
        break;
    }
    WrapperSwitchClose(wrapper);
}

internal void
PrefixVariablesWithStructIfNecessary(writeable_text_buffer *buffer, declaration *decl, type_specifier *ref_type)
{
    if(decl->type == Declaration_Variable)
    {
        if(decl->typespec->type == TypeSpec_StructUnion && decl->typespec == ref_type)
        {
            Panic(); //Infinte nesting
        }
        else if(decl->typespec->type == TypeSpec_Array && 
                decl->typespec->array_base_type == ref_type)
        {
            Panic(); //Infinte nesting
        }
        else if (decl->typespec->type == TypeSpec_Ptr && decl->typespec->ptr_base_type == ref_type)
        {
            OutputCString(buffer, "struct ", 0);
        }
    }
    else if (decl->type == Declaration_Struct || decl->type == Declaration_Union)
    {
        for(declaration *member = decl->members;
            member;
            member = member->next)
        {
            PrefixVariablesWithStructIfNecessary(buffer, member, ref_type);
        }
    }
    else Panic(); //check with valid member for structs/unions
}

internal void
OutputDeclC(writeable_text_buffer *buffer, declaration *decl)
{
    Assert(decl);
    switch(decl->type)
    {
        case Declaration_Variable: 
        case Declaration_ProcedureArgs:   
        {
            OutputTypeSpecifier(buffer, decl->typespec, ' ');//NOTE array [] after var 
            OutputCString(buffer, decl->identifier, 0);
            //u32 num_array_subscripts = TypeSpecifierNumArrays(decl->typespec);
            for(type_specifier *top = decl->typespec; //NOTE output array subscripts
                top->type == TypeSpec_Array;
                top = top->array_type)
            {//TODO is this correct order in C?
                OutputChar(buffer, '[', 0);
                OutputInteger64(buffer, top->array_size); //TODO expression
                OutputChar(buffer, ']', 0);
            }
            
            if(decl->type == Declaration_Variable)
            {
                if(decl->initializer)
                {
                    OutputCString(buffer, " = ", 0);
                    OutputExpression(buffer, decl->initializer, 0);
                }
                OutputCString(buffer, ";\n", 0); 
            }
            else if(decl->type == Declaration_ProcedureArgs && decl->initializer) Panic();
        }
        break;
        
        case Declaration_Typedef:   
        Panic();
        //TODO can you typedef arrays?
        
        break;//semicolon
        
        case Declaration_Struct: case Declaration_Union: case Declaration_Enum:  
        //NOTE structs can be nested into functions!!!
        if(decl->identifier)
        {
            OutputCString(buffer, "typedef ", 0);
        }
        
        
        if(decl->type == Declaration_Struct) 
        {
            OutputCString(buffer, "struct ", 0);
            if(decl->identifier) OutputCString(buffer, decl->identifier, 0);
        }
        else if(decl->type == Declaration_Union) 
        {
            OutputCString(buffer, "union ", 0);
            if(decl->identifier) OutputCString(buffer, decl->identifier, 0);
        }
        else if(decl->type == Declaration_Enum)
        {
            
            OutputCString(buffer, "enum ", 0);
        }
        
        OutputCString(buffer, "\n{\n", 0);
        
        type_specifier *this_type_spec = 0;
        if(decl->identifier)
        {
            this_type_spec = FindBasicType(decl->identifier);
        }
        for(declaration *member = decl->members; //NOTE hack enum_members and members overlap in memory
            member;
            member = member->next)
        {
            if(decl->type != Declaration_Enum && this_type_spec)
            {
                PrefixVariablesWithStructIfNecessary(buffer, member, this_type_spec);
            }
            OutputDeclC(buffer, member);
            
        }
        OutputChar(buffer, '}', 0);
        if(decl->identifier) OutputCString(buffer, decl->identifier, 0);
        OutputCString(buffer, ";\n\n", 0);
        break;
        
        case Declaration_EnumMember:   
        OutputCString(buffer, decl->identifier, 0);
        if(decl->const_expr)
        {
            OutputCString(buffer, " = ", 0);
            OutputExpression(buffer, decl->const_expr, 0);
        }
        OutputCString(buffer, ",\n", 0);
        break;
        
        case Declaration_Procedure:   
        if(decl->proc_keyword == Keyword_Internal)  
            OutputCString(buffer, "static ", 0);
        else if(decl->proc_keyword == Keyword_External)  
            OutputCString(buffer, "extern ", 0);
        else if(decl->proc_keyword == Keyword_Inline)  
            OutputCString(buffer, "inline ", 0); //TODO compiler specific force inline
        else if(decl->proc_keyword == Keyword_NoInline)  ;
        //TODO compiler specific force no_inline
        else Panic();
        OutputTypeSpecifier(buffer, decl->proc_return_type, 0);
        OutputChar(buffer, '\n', 0);
        OutputCString(buffer, decl->identifier, 0); //TODO function_overloading!!!
        OutputChar(buffer, '(', 0);
        for(declaration *arg = decl->proc_args;
            arg;
            arg = arg->next)
        {
            Assert(arg->type == Declaration_ProcedureArgs);
            OutputDeclC(buffer, arg); //TODO optional param!
            if(!arg->next == 0)
                OutputCString(buffer, ", ", 0);
        }
        OutputCString(buffer, ")\n", 0);
        OutputStatement(buffer, decl->proc_body, 0);
        OutputChar(buffer, '\n', 0);
        break;
        
        default: Panic();
    }
}

