#include "ast.h"

internal void
RemoveAllInitializersFromStruct(declaration *decl)
{
    //TODO memory leaks here!!!
    Assert(decl->type == Declaration_Struct ||
           decl->type == Declaration_Union);
    for(declaration *member = decl->members;
        member;
        member = member->next)
    {
        if(member->type == Declaration_Variable)
        {
            member->initializer = 0;
        }
        else if (member->type == Declaration_Struct ||
                 member->type == Declaration_Union)
        {
            RemoveAllInitializersFromStruct(member);
        }
    }
}

internal expression *
AssignExpressionForMember(declaration *member, expression *lval_struct_expr)
{
    expression *result = 0;
    if(member->type == Declaration_Variable &&
       member->initializer)
    {
        expression *lval = NewExpression(Expression_MemberAccess);
        lval->struct_expr = lval_struct_expr;
        lval->struct_member_identifier = member->identifier;
        expression *rval = member->initializer;
        result = NewBinaryExpression('=', lval, rval);
        
        member->initializer = 0;
    }
    else if(member->type == Declaration_Union)
    {
        for(declaration *inner_member = member->members;
            inner_member;
            inner_member = inner_member->next)
        {
            result = AssignExpressionForMember(inner_member, lval_struct_expr);
            if(result)
            {
                break;
            }
        }
        RemoveAllInitializersFromStruct(member);
    }
    else if(member->type == Declaration_Struct)
    {
        expression **tail = &result;
        for(declaration *inner_member = member->members;
            inner_member;
            inner_member = inner_member->next)
        {
            expression *init_expr = AssignExpressionForMember(inner_member, lval_struct_expr);
            if(init_expr)
            {
                if(!result)
                {
                    result = init_expr;
                }
                else
                {
                    AppendExpressionToTail(tail, init_expr);
                    tail = &((*tail)->compound_next);
                }
            }
            
        }
    }
    else Assert(member->type == Declaration_Variable); //unint var decl!
    return result;
}

internal void
FixupEnumFlags(declaration *enum_flags)
{
    enum_flags->type = Declaration_Enum;
    //why not just store this as an enum throughout?
}




internal void
AstFixupC(declaration_list *ast)//TODO
{
    //create constructros for all structs and unions
    for(u32 i = 0; i < fixup_table.fixup_count; ++i)
    {
        after_parse_fixup *fixup = fixup_table.fixups + i;
        if(fixup->type == Fixup_DeclBeforeDecl)
        {
            //NOTE I can probably pull this out into a safe function!
            
            declaration *prev_back = fixup->first_decl->back;
            fixup->first_decl->back = fixup->decl_after_decl;
            fixup->decl_after_decl->back = prev_back;
            if(prev_back->next) //in case of it being last decl in entire ast!
            {
                prev_back->next = fixup->decl_after_decl;
            }
            
            
            
            declaration *rest_decls = fixup->decl_after_decl->next;
            fixup->decl_after_decl->next = fixup->first_decl;
            fixup->first_decl->next = rest_decls;
            if(rest_decls) //in case of last one in list
            {
                rest_decls->back = fixup->first_decl;
            }
            
            
        }
        else if (fixup->type == Fixup_StructNeedsConstructor)
        {
            Assert(fixup->struct_with_no_constructor);
            declaration *dstruct = fixup->struct_with_no_constructor;
            declaration *constructor = NewDeclaration(Declaration_Procedure);
            constructor->identifier = ConcatCStringsIntern(dstruct->identifier, "__Constructor");
            constructor->proc_keyword = Keyword_Inline;
            constructor->proc_return_type = FindBasicType(dstruct->identifier);
            constructor->proc_body = NewStatement(Statement_Compound);
            statement *decl_result_stmt = NewStatement(Statement_Declaration);
            decl_result_stmt->decl = NewDeclaration(Declaration_Variable);
            decl_result_stmt->decl->identifier = StringInternLiteral("result");
            decl_result_stmt->decl->typespec = constructor->proc_return_type;
            constructor->proc_body->compound_stmts = decl_result_stmt;
            constructor->proc_body->compound_tail = constructor->proc_body->compound_stmts;
            constructor->proc_body->decl_list.decls = decl_result_stmt->decl;
            constructor->proc_body->decl_list.above = ast;
            
            expression *result_expr =  NewExpressionIdentifier("result");
            if(dstruct->type == Declaration_Struct)
            {
                for(declaration *member = dstruct->members;
                    member;
                    member = member->next)
                {
                    expression *assign_expr = AssignExpressionForMember(member, result_expr);
                    if(assign_expr)
                    {
                        statement *stmt = NewStatement(Statement_Expression);
                        stmt->expr = assign_expr;
                        AppendStatement(constructor->proc_body, stmt);
                    }
                }
            }
            else
            {
                for(declaration *member = dstruct->members;
                    member;
                    member = member->next)
                {
                    expression *assign_expr = AssignExpressionForMember(member, result_expr);
                    if(assign_expr)
                    {
                        statement *stmt = NewStatement(Statement_Expression);
                        stmt->expr = assign_expr;
                        AppendStatement(constructor->proc_body, stmt);
                        break;
                    }
                }
                RemoveAllInitializersFromStruct(dstruct);
                
            }
            
            
            statement *return_result = NewStatement(Statement_Return);
            return_result->expr = result_expr;
            AppendStatement(constructor->proc_body, return_result);
            
            declaration *remaining = dstruct->next;
            dstruct->next = constructor;
            constructor->next = remaining;
            constructor->back = dstruct;
            remaining->back = constructor;
        }
        else if(fixup->type == Fixup_VarMayNeedConstructor)
        {
            if(fixup->decl->typespec->type == TypeSpec_UnDeclared)
            {
                Panic("Should've been fixed beforehand when first parsed typespecifier!");
            }
            else if (fixup->decl->typespec->type == TypeSpec_StructUnion)
            {
                if(fixup->decl->typespec->needs_constructor)
                {
                    Assert(fixup->decl->initializer);
                    fixup->decl->initializer->type = Expression_Call;
                    char *constructor_name = ConcatCStringsIntern(fixup->decl->typespec->identifier, "__Constructor");
                    fixup->decl->initializer->call_expr = NewExpressionIdentifier(constructor_name);
                    fixup->decl->initializer->call_args = 0;
                }
                else
                {
                    fixup->decl->initializer = expr_zero_struct;
                }
                
            }
            else if (fixup->decl->typespec->type == TypeSpec_Array)
            {
                fixup->decl->initializer = expr_zero_struct;
            }
            else
            {
                fixup->decl->initializer = expr_zero;
            }
        }
        else if(fixup->type == Fixup_ProcedureOptionalArgs)
        {
            Assert(fixup->decl->type == Declaration_Procedure);
            statement *body = fixup->decl->proc_body;
            statement opts = {0};
            opts.type = Statement_Compound;
            Assert(body->compound_stmts);
            for(declaration *arg = fixup->decl->proc_args;
                arg;
                arg = arg->next)
            {
                Assert(arg->type == Declaration_ProcedureArgs);
                if(arg->initializer)
                {
                    statement *init_stmt = NewStatement(Statement_Expression);
                    expression *lval = NewExpressionIdentifier(arg->identifier);
                    init_stmt->expr = NewBinaryExpression('=', lval, arg->initializer);
                    arg->initializer = 0;
                    
                    AppendStatement(&opts, init_stmt);
                    
                }
            }
            
            
            if(opts.compound_stmts)
            {
                statement *rest_body = body->compound_stmts;
                body->compound_stmts = opts.compound_stmts;
                opts.compound_tail->next = rest_body;
            }
        }
        else
        {
            Panic();
        }
    }
    
    
    
    
#if 0
    for(declaration *decl = ast->decls;
        decl;
        decl = decl->next)
    {
        switch(decl->type)
        {
            case Declaration_Struct: case Declaration_Union:
            declaration *new_nodes = FixupStructUnion(decl);
            if(new_nodes)
            {
                declaration *remaining = decl->next;
                decl->next = new_nodes;
                new_nodes->next = remaining;
                decl = new_nodes; //NOTE run this through again?
            }
            break;
            
            case Declaration_Variable:
            FixupVariable(decl);
            break;
            
            case Declaration_Procedure:
            FixupProcedure(decl);
            if(decl->proc_body->type == Statement_Compound)
            {
                AstFixupC(&decl->proc_body->decl_list);
            }
            
            //TODO what about single line statements?
#if 0
            for(stmt = ;
                stmt;
                stmt = stmt->next)
            {
                if(stmt->type == Statement_Declaration)
                {
                    AstFixupC(stmt->decl);
                }
            }
            if(stmt && stmt->type == Statement_Declaration)
                AstFixupC(stmt->decl);
#endif
            
            break;
            
            case Declaration_EnumFlags:
            FixupEnumFlags(decl);
            break;
            
            
        }
    }
#endif
}

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
                default: Panic();
            } 
        }break;
        
        case TypeSpec_StructUnion: case TypeSpec_Enum: 
        {
            printf("%s", typespec->identifier);
        }break;
        
        case TypeSpec_Ptr: 
        {
            printf("%s *%d", typespec->ptr_base_type->identifier, typespec->star_count);
        }break;
        
        case TypeSpec_Array: 
        { 
            PrintTypeSpecifier(typespec->array_base_type);
            printf("[%llu]", typespec->array_size);
        }break;
        
        case TypeSpec_Proc: 
        {
            Panic(); //TODO
        }break;
        default: Panic();
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
                PrintExpression(expr->array_index_expr);
                printf("]");
            }break;
            case Expression_Compound: { 
                printf("(");
                expression *compound = expr;
                while(compound->type == Expression_Compound)
                {
                    PrintExpression(compound->compound_expr);
                    printf(",");
                    compound = compound->compound_next;
                }
                if(compound)
                {
                    PrintExpression(compound);
                }
                printf(")");
            }break;
            case Expression_ToBeDefined: { 
                printf("TBD!!!");
            }break;
            default: Panic();
        }
        
    }
}

#undef CasePrint

internal void
PrintStatement(statement *stmt, u32 ident)
{
    PrintTabs(ident);
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
        Assert(stmt->cond_expr_as_const);
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
        
        default: Panic();
    }
    
}


internal void //newlines, except procargs
PrintDeclaration(declaration *decl,  u32 indent)
{
    PrintTabs(indent);
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
        Panic();
        break;
        
        case Declaration_EnumMember: 
        printf("%s ", decl->identifier);
        PrintExpression(decl->expr_as_const);
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
                printf("}\n");
            }
            else
            {
                printf(";\n");
            }
        }break;
        
        case Declaration_Procedure: 
        if(decl->proc_keyword == Keyword_Inline)  printf("inline ");
        else if(decl->proc_keyword == Keyword_NoInline)  printf("no_inline ");
        else if(decl->proc_keyword == Keyword_Internal)  printf("internal ");
        else if(decl->proc_keyword == Keyword_External)  printf("external ");
        PrintTypeSpecifier(decl->proc_return_type);
        printf("\n");
        PrintTabs(indent);
        printf("%s(", decl->identifier);
        for(declaration *arg = decl->proc_args;
            arg;
            arg = arg->next)
        {
            PrintDeclaration(arg, 0);
            if(arg->next)
            {
                printf(", ");
            }
            
        }
        printf(")");
        if(decl->proc_body)
        {
            PrintStatement(decl->proc_body, indent);
        }
        else
        {
            printf(";\n");
        }
        
        break;
        
        case Declaration_Include:
        printf("#include ");
        PrintExpression(decl->actual_expr);
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
        
        default: Panic();
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