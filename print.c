internal void PrintDeclaration(declaration *decl, u32 indent);

internal void
PrintTypeSpecifier(type_specifier *typespec)
{
    switch(typespec->type)
    {
        case TypeSpec_Internal: 
        {
            switch(typespec->internal_type)
            {
                case Internal_S8: printf("s8");break;
                case Internal_S16: printf("s16");break;
                case Internal_S32: printf("s32");break;
                case Internal_S64: printf("s64");break;
                case Internal_U8: printf("u8");break;
                case Internal_U16: printf("u16");break;
                case Internal_U32: printf("u32");break;
                case Internal_U64: printf("u64");break;
                case Internal_Float: printf("float");break;
                case Internal_Double: printf("double");break;
                case Internal_Void: printf("void");break;
                default: panic();
            } 
        }break;
        
        case TypeSpec_Struct:case TypeSpec_Union:
        case TypeSpec_Enum: case TypeSpec_EnumFlags: 
        {
            printf("%s", typespec->identifier);
        }break;
        
        case TypeSpec_Ptr: 
        {
            printf("%s *%llu", typespec->base_type->identifier, typespec->size);
        }break;
        
        case TypeSpec_Array: 
        { 
            PrintTypeSpecifier(typespec->base_type);
            printf("[%llu]", typespec->size);
        }break;
        
        
        default: panic();
    }
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
        default: panic();
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
        default: panic();
    }
}


internal void
PrintExpression(expression *expr)
{
    if(expr)
    {
        
        switch(expr->type)
        {
            case Expression_Identifier: { 
                printf("%s", expr->identifier);
            }break; //variable
            case Expression_StringLiteral: { 
                printf("\"%s\"", expr->string_literal);
            }break;
            case Expression_CharLiteral: { 
                printf("\'%c\'", expr->char_literal);
            }break;
            case Expression_Integer: { 
                printf("%llu", expr->integer);
            }break;
            case Expression_RealNumber: { 
                printf("%ff", (float)expr->real);
            }break;
            case Expression_Binary: { 
                printf("(");
                PrintExpression(expr->binary_left);
                printf(" ");
                PrintBinaryOperator(expr->binary_type); 
                printf(" ");
                PrintExpression(expr->binary_right);
                printf(")");
            }break;
            case Expression_Unary: { 
                if(expr->unary_type == Unary_PostDecrement ||
                   expr->unary_type == Unary_PostIncrement)
                {
                    PrintExpression(expr->unary_expr);
                    PrintUnaryOperator(expr->unary_type);
                }
                else
                {
                    PrintUnaryOperator(expr->unary_type);
                    PrintExpression(expr->unary_expr);
                }
            }break;
            case Expression_MemberAccess: { 
                PrintExpression(expr->struct_expr);
                printf(".%s", expr->struct_member_identifier);
            }break; 
            case Expression_Ternary: { 
                printf("(");
                PrintExpression(expr->tern_cond);
                printf("?");
                PrintExpression(expr->tern_true_expr);
                printf(":");
                PrintExpression(expr->tern_false_expr);
                printf(")");
            }break;
            case Expression_Cast: { 
                printf("cast(");
                PrintTypeSpecifier(expr->casting_to);
                printf(",");
                PrintExpression(expr->cast_expression);
                printf(")");
            }break;
            case Expression_Call: { 
                //printf("Call...\n");
                PrintExpression(expr->call_expr);
                if(expr->call_args)
                {
                    if(expr->type != Expression_Compound)
                    {
                        printf("(");
                        PrintExpression(expr->call_args);
                        printf(")");
                    }
                    else
                    {
                        PrintExpression(expr->call_args);
                    }
                }
                else
                {
                    printf("()");
                }
                
            }break;
            case Expression_ArraySubscript: { 
                //printf("Array Index...\n");
                PrintExpression(expr->array_expr);
                printf("[");
                PrintExpression(expr->array_index_expr_as_const);
                printf("]");
            }break;
            case Expression_Compound: { 
                printf("(");
                assert(expr->members);
                for(expression *member = expr->members;
                    member;
                    member = member->member_next)
                {
                    assert(member->type == Expression_Member);
                    PrintExpression(member->member_expression);
                    if(member->member_next)
                    {
                        printf(",");
                    }
                }
                printf(")");
            }break;
            
            
            case Expression_CompoundInitializer:
            {
                printf("{");
                for(expression *member = expr->members;
                    member;
                    member = member->member_next)
                {
                    assert(member->type == Expression_Member);
                    PrintExpression(member->member_expression);
                    if(member->member_next)
                    {
                        printf(",");
                    }
                }
                printf("}");
            }break;
            case Expression_ToBeDefined: { 
                printf("TBD!!!");
            }break;
            default: panic();
        }
        
    }
}

#undef CasePrint

internal void
PrintStatement(statement *stmt, u32 ident)
{
    print_tabs(ident);
    switch(stmt->type)
    {
        case Statement_Declaration:
        PrintDeclaration(stmt->decl, 0);
        break;
        
        case Statement_Expression:
        PrintExpression(stmt->expr);
        printf(";\n");
        break;
        
        case Statement_If:
        case Statement_Switch:
        case Statement_While:
        if(stmt->type == Statement_If) printf("if");
        else if(stmt->type == Statement_Switch) printf("switch");
        else if(stmt->type == Statement_While) printf("while");
        PrintExpression(stmt->cond_expr);
        PrintStatement(stmt->cond_stmt, ident);
        break;
        
        case Statement_Else:
        printf("else");
        PrintStatement(stmt->cond_stmt, ident);
        break;
        
        case Statement_SwitchCase:
        printf("case ");
        assert(stmt->cond_expr_as_const);
        PrintExpression(stmt->cond_expr_as_const);
        PrintStatement(stmt->cond_stmt, ident);
        break; 
        
        case Statement_For:
        printf("for(");
        PrintStatement(stmt->for_init_stmt, 0);
        PrintExpression(stmt->for_expr2);
        printf("; ");
        PrintExpression(stmt->for_expr3);
        printf(";)");
        PrintStatement(stmt->for_stmt, ident);
        break;
        
        case Statement_Defer:
        printf("defer ");
        PrintStatement(stmt->defer_stmt, ident);
        break;
        
        
        case Statement_Break:
        printf("break;\n");
        break;
        case Statement_Continue:
        printf("continue;\n");
        break;
        case Statement_Return:
        printf("return ");
        if(stmt->expr)
        {
            PrintExpression(stmt->expr);
        }
        printf(";\n");
        break;
        
        case Statement_Compound:
        printf("{\n");
        for(statement *inner = stmt->compound_stmts;
            inner;
            inner = inner->next)
        {
            PrintStatement(inner, ident + 1);
        }
        printf("}\n");
        break;
        
        case Statement_Null:
        printf(";\n");
        break;
        
        default: panic();
    }
    
}


internal void //newlines, except procargs
PrintDeclaration(declaration *decl,  u32 indent)
{
    print_tabs(indent);
    switch(decl->type)
    {
        case Declaration_Variable: 
        case Declaration_ProcedureArgs: 
        PrintTypeSpecifier(decl->typespec);
        printf(" %s", decl->identifier);
        if(decl->initializer)
        {
            printf(" = ");
            PrintExpression(decl->initializer);
        }
        if(decl->type == Declaration_Variable)
        {
            printf(";\n");
        }
        break;
        
        
        case Declaration_Typedef: 
        //printf("Decl Typedef:%s...alias:%s\n", decl->identifier, decl->typespec->identifier);
        panic();
        break;
        
        case Declaration_EnumMember: 
        printf("%s ", decl->identifier);
        PrintExpression(decl->initializer_as_const);
        printf(";\n");
        break;
        
        case Declaration_Struct: 
        case Declaration_Union: 
        case Declaration_Enum: 
        case Declaration_EnumFlags: 
        {
            if(decl->type == Declaration_Struct)   printf("struct %s", decl->identifier);
            else if(decl->type == Declaration_Union)   printf("union %s", decl->identifier);
            else if(decl->type == Declaration_Enum)   printf("enum %s", decl->identifier);
            else if(decl->type == Declaration_EnumFlags)   printf("enum_flags %s", decl->identifier);
            if(decl->members)
            {
                printf("{\n");
                for(declaration *member = decl->members; 
                    member;
                    member = member->next)
                {
                    PrintDeclaration(member, indent + 1);
                }
                
                print_tabs(indent);
                printf("}\n");
            }
            else
            {
                printf(";\n");
            }
        }break;
        
        case Declaration_ProcedureGroup:
        {
            assert(decl->members);
            for(declaration *sign = decl->members;
                sign;
                sign= sign->next)
            {
                PrintDeclaration(sign, indent);
            }
        }break;
        
        case Declaration_ProcedureSignature:
        {
            print_tabs(indent);
            if(decl->keyword == Keyword_Inline)         printf("inline ");
            else if(decl->keyword == Keyword_NoInline)  printf("no_inline ");
            else if(decl->keyword == Keyword_Internal)  printf("internal ");
            else if(decl->keyword == Keyword_External)  printf("external ");
            else panic();
            PrintTypeSpecifier(decl->return_type);
            printf("\n%s(", decl->identifier);
            
            for(declaration *arg = decl->args;
                arg;
                arg = arg->next)
            {
                PrintDeclaration(arg, 0);
                if(arg->next)
                {
                    printf(", ");
                }
                
            }
            printf(")\n");
        }break;
        
        case Declaration_Procedure: 
        PrintDeclaration(decl->signature, indent);
        PrintStatement(decl->proc_body, indent);
        break;
        
        case Declaration_Include:
        printf("#include ");
        PrintExpression(decl->initializer_as_const);
        break; 
        
        case Declaration_Constant:
        printf("let %s = ", decl->identifier);
        PrintExpression(decl->initializer_as_const);
        printf(";\n");
        break;
        
        case Declaration_ForeignBlock:
        printf("foreign {\n");
        for(declaration *member = decl->members;
            member;
            member = member->next)
        {
            PrintDeclaration(member, indent + 1);
        }
        printf("}\n");
        break;
        
        case Declaration_Macro:
        printf("define %s;\n", decl->identifier);
        break;
        
        
        
        case Declaration_MacroArgs:
        printf("define %s(...);\n", decl->identifier);
        break;
        
        default: panic();
    }
}


inline void
PrintAST(declaration_list *ast)
{
    for(declaration *decl = ast->decls;
        decl;
        decl = decl->next)
    {
        PrintDeclaration(decl, 0);
        printf("\n\n");
    }
}

#if 0
inline void
PrintSymbolList(symbol_list *list)
{
    for(u32 i = 0; i < list->count; ++i)
    {
        symbol *sym = list->symbols + i;
        PrintDeclaration(sym->decl, 0);
        printf("\n\n");
    }
    
}
#endif


internal void
print_type_table()
{
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        PrintTypeSpecifier(type_table.types + i);
    }
}