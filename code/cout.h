#ifndef COUT_H
#define COUT_H

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



inline void ResetTextBuffer(writeable_text_buffer *buffer);

internal void OutputChar(writeable_text_buffer *buffer, char c, char wrapper);
internal void OutputCString(writeable_text_buffer *buffer, char *string, char wrapper);

internal void OutputItoa(writeable_text_buffer *buffer, u64 integer);
internal void OutputFtoa64(writeable_text_buffer *buffer, double value); //TODO
internal void OutputTypeSpecifier(writeable_text_buffer *buffer, type_specifier *typespec, char wrapper);


internal void OutputExpression(writeable_text_buffer *buffer, expression *expr, char wrapper);

internal void OutputStatement(writeable_text_buffer *buffer, statement *stmt, char wrapper);
internal void OutputDeclC(writeable_text_buffer *buffer, declaration *decl);



//if statment is not compound, make it one and add to it's next pointer
//if it's a chain of compounds, go to the very last and do the same thing again
#if 0
internal void //TODO speed up, doing this for clarity right now
CompoundAppendStatement(statement **first_statement, statement *new_stmt)
{
    if((*first_statement)->type != Statement_Compound)
    {
        statement *temp = *first_statement;
        *first_statement = NewStatement(Statement_Compound);
        (*first_statement)->compound_stmt = temp;
        (*first_statement)->compound_next = new_stmt;
    }
    else
    {
        //Find next non compound statement;
        statement *second_to_last = *first_statement;
        statement *end = second_to_last->compound_next;
        while(end->type == Statement_Compound)
        {
            second_to_last = end;
            end = second_to_last->compound_next;
        }
        second_to_last->compound_next = NewStatement(Statement_Compound);
        second_to_last->compound_next->compound_stmt = end;
        second_to_last->compound_next->compound_next = new_stmt;
        u32 vs_sucks = 0;
        ++vs_sucks;
    }
}


inline void
AppendStatementToTail(statement **tail, statement *new_stmt)
{
    statement *tail_stmt = *tail;
    *tail = NewStatement(Statement_Compound);
    (*tail)->compound_stmt = tail_stmt;
    (*tail)->compound_next = new_stmt;
}

#endif

internal declaration *
FixupVariable(declaration *var)
{
    Assert(var->type == Declaration_Variable);
    declaration *nodes = 0;
    if(var->typespec->type == TypeSpec_StructUnion && 
       var->initializer && var->initializer->type == Expression_ToBeDefined)
    {
        var->initializer->type = Expression_Call;
        char *constructor_name = ConcatCStringsIntern(var->typespec->identifier, "__Constructor");
        var->initializer->call_expr = NewExpressionIdentifier(constructor_name);
        var->initializer->call_args = 0;
    }
    
    
    return nodes;
}

internal declaration *
FixupProcedure(declaration *proc)
{
    Assert(proc->type == Declaration_Procedure);
    declaration *nodes = 0;
    //optional args
    statement *top_proc_body = proc->proc_body;
    statement *opt_initializer_stmts = stmt_null;
    statement *current = opt_initializer_stmts;
    for(declaration *arg = proc->proc_args;
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
            
            if(opt_initializer_stmts == stmt_null)
            {
                opt_initializer_stmts = init_stmt;
                current = init_stmt;
            }
            else
            {
                Assert(current->next == 0);
                current->next = init_stmt;
                current = current->next;
            }
        }
    }
    if(opt_initializer_stmts != stmt_null)
    {
        Assert(current->next == 0);
        current->next = top_proc_body->compound_stmts;
        proc->proc_body->compound_stmts = opt_initializer_stmts;
    }
    
    return nodes;
}


internal void
RemoveAllInitializersFromStruct(declaration *decl)
{
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
InitExpressionForDeclaration(declaration *member, expression *lval_struct_expr)
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
            result = InitExpressionForDeclaration(inner_member, lval_struct_expr);
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
            expression *init_expr = InitExpressionForDeclaration(inner_member, lval_struct_expr);
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





//NOTE this will generate some *decls, that have to be moved before and after struct decl
internal declaration *
FixupStructUnion(declaration *decl)
{
    Assert(decl->type == Declaration_Struct || decl->type == Declaration_Union);
    declaration *new_nodes = 0;
    
    if(decl->struct_needs_constructor)
    {
        declaration *constructor = NewDeclaration(Declaration_Procedure);
        constructor->identifier = ConcatCStringsIntern(decl->identifier, "__Constructor");
        constructor->proc_keyword = Keyword_Inline;
        constructor->proc_return_type = FindBasicType(decl->identifier);
        constructor->proc_body = NewStatement(Statement_Compound);
        statement *decl_result_stmt = NewStatement(Statement_Declaration);
        decl_result_stmt->decl = NewDeclaration(Declaration_Variable);
        decl_result_stmt->decl->identifier = StringInternLiteral("result");
        decl_result_stmt->decl->typespec = constructor->proc_return_type;
        constructor->proc_body->compound_stmts = decl_result_stmt;
        constructor->proc_body->compound_tail = constructor->proc_body->compound_stmts;
        
        
        expression *struct_expr =  NewExpressionIdentifier("result");
        if(decl->type == Declaration_Struct)
        {
            for(declaration *member = decl->members;
                member;
                member = member->next)
            {
                expression *assign_expr = InitExpressionForDeclaration(member, struct_expr);
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
            for(declaration *member = decl->members;
                member;
                member = member->next)
            {
                expression *assign_expr = InitExpressionForDeclaration(member, struct_expr);
                if(assign_expr)
                {
                    statement *stmt = NewStatement(Statement_Expression);
                    stmt->expr = assign_expr;
                    AppendStatement(constructor->proc_body, stmt);
                    break;
                }
            }
            RemoveAllInitializersFromStruct(decl);
            
        }
        
        
        statement *return_result = NewStatement(Statement_Return);
        return_result->return_expr = struct_expr;
        AppendStatement(constructor->proc_body, return_result);
        new_nodes = constructor;
    }
    return new_nodes;
}

internal void
FixupEnumFlags(declaration *enum_flags)
{
    enum_flags->type = Declaration_Enum;
}

internal void
AstFixupC(declaration *ast)//TODO
{
    //create constructros for all structs and unions
    for(declaration *decl = ast;
        decl;
        decl = decl->next)
    {
        switch(decl->type)
        {
            case Declaration_Struct: case Declaration_Union:
            declaration *new_nodes = FixupStructUnion(decl);
            declaration *remaining = decl->next;
            decl->next = new_nodes;
            new_nodes->next = remaining;
            decl = new_nodes; //NOTE run this through again?
            break;
            
            case Declaration_Variable:
            FixupVariable(decl);
            break;
            
            case Declaration_Procedure:
            FixupProcedure(decl);
            statement *stmt;
            for(stmt = decl->proc_body->compound_stmts;
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
            break;
            
            case Declaration_EnumFlags:
            FixupEnumFlags(decl);
            break;
            
            
        }
    }
}

internal void
OutputC(declaration *ast, char *filename) //TODO get rid of stdio.h
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
        AstFixupC(ast);
        for(declaration *decl = ast;
            decl;
            decl = decl->next)
        {
            OutputDeclC(&buffer, decl);
            OutputChar(&buffer, '\n', 0); //top level decls only
            fwrite(buffer.ptr, buffer.size_written, 1, file);
            ResetTextBuffer(&buffer);
        }
        
        fclose(file);
    }
}


internal bool32
ExpressionResolvesToPointer(expression *expr)//Put in expression.h
{
    return 0;
}

#endif