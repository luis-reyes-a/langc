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

internal void
OutputC(declaration_list *ast_, char *filename) //TODO get rid of stdio.h
{
    
    HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        char large_buffer[40] = {0};
        
        writeable_text_buffer buffer = {0};
        u32 test = Kilobytes(512);
        buffer.max_size = Kilobytes(512);
        buffer.size_written = 0;
        buffer.ptr = platform_virtual_alloc(buffer.max_size);
        buffer.at = buffer.ptr; //this is overwriting the main ast decl nodes!
        buffer.format_char_width = 80;
        buffer.tried_to_break_already = false;
        
        
        for(declaration *decl = ast_->decls;
            decl;
            decl = decl->next)
        {
            OutputDeclC(&buffer, decl);
            OutputChar(&buffer, '\n', 0); //top level decls only
            DWORD bytes_written;
            if(WriteFile(file_handle, buffer.ptr, buffer.size_written, &bytes_written, 0))
            {
                if(bytes_written != buffer.size_written)
                {
                    panic();
                }
            }
            else
            {
                panic();
            }
            ResetTextBuffer(&buffer);
        }
        
        CloseHandle(file_handle);
    }
    else
    {
        panic();
    }
    
    
    
    
#if 0
    //FILE *file = fopen(filename, "w");
    if(file)
    {
        AstFixupC(ast_);
        for(declaration *decl = ast_->decls;
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
#endif
}


internal bool32
ExpressionResolvesToPointer(expression *expr)//Put in expression.h
{
    return 0;
}

#endif