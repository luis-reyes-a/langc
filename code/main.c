
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


#define DEBUG_BUILD 1

inline void 
PrintTabs(u32 tabs)
{
    while(tabs--)  printf("\t");
}


#include "memory_arena.h"
#include "strings.c"
#include "ast.h"

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

#include "windows.h"

inline void *
PlatformAllocateMemory(u64 size)
{
    return VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

#include "cout.c"
#include "ast.c"


typedef struct debug_read_file_result
{
    u32 size;
    void *data;
} platform_file;

inline u32
SafeTruncateUInt64(u64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return(Result);
}

internal platform_file //taken from handmade hero
PlatformReadFile(char *filename)
{
    platform_file result = { 0 };
    
    HANDLE FileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            result.data = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(result.data)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, result.data, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // NOTE(casey): File read successfully
                    result.size = FileSize32;
                }
                else
                {              
                    Panic();
                }
            }
            else
            {
                Panic();
            }
        }
        else
        {
            Panic();
        }
        
        CloseHandle(FileHandle);
    }
    else
    {
        Panic();
    }
    
    return result;
}


internal int
FileHasExtension(char *filename, char *extension, u32 extension_length)
{
    int has_extension = false;
    for(char *at = filename; *at; ++at)
    {
        if(at[0] == '.' && at[1] == '.')
        {
            at += 2; //skips \\ as well
        }
        else if(*at == '.')
        {
            has_extension = true;
            ++at;
            for(u32 i = 0; i < extension_length; ++i)
            {
                if(at[i] && at[i] == extension[i])
                {
                    //Okay
                }
                else
                {
                    has_extension = false;
                    break;
                }
            }
            break;
        }
    }
    return has_extension;
}

#if 1
internal void CheckDeclarationList(declaration_list *list);

internal int
ExpressionResolvesToSturctType(expression *expr)
{
    Panic();
    return 0;
}

typedef struct
{
    expression_type type;
    union
    {
        type_specifier *typespec; //if identifier
    };
} resolved_expr_info;

internal resolved_expr_info
ResolvedExpressionType(expression *expr)
{
    resolved_expr_info result = {0};
    switch(expr->type)
    {
        case Expression_StringLiteral:
        case Expression_CharLiteral:
        case Expression_Integer:
        case Expression_RealNumber:
        result.type = expr->type;
        break;
        
        case Expression_MemberAccess:
        //will want type info!
        result.type = Expression_Identifier;
        break;
        
        case Expression_Call:
        //check return value
        break;
        case Expression_ArraySubscript:
        break; 
    }
    
}

internal declaration *
ExpressionResolveDeclaration(expression *expr)
{
    return 0;
}

internal void
CheckExpression(expression *expr, declaration_list *scope)
{
    switch(expr->type)
    {
        case Expression_Identifier:
        declaration *decl = FindDeclaration(scope, expr->identifier);
        if(!decl)
        {
            SyntaxError(expr->file, expr->line, "Identifier never declared!");
        }
        break;
        
        case Expression_StringLiteral:
        case Expression_CharLiteral:
        case Expression_Integer:
        case Expression_RealNumber:
        //... nothing
        break;
        
        case Expression_Binary:
        break;
        case Expression_Unary:
        break;
        
        case Expression_MemberAccess:
        {
            declaration *resolved_decl = ExpressionResolveDeclaration(expr->struct_expr);
            if(resolved_decl  &&
               (resolved_decl->type == Declaration_Struct ||
                resolved_decl->type == Declaration_Union))
            {
                //find this in that decl
                int found = false;
                for(declaration *member = resolved_decl->members;
                    member;
                    member = member->next)
                {
                    if(member->identifier = expr->struct_member_identifier)
                    {
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    printf("Didn't find member in struct by that identifier!");
                }
            }
            else
            {
                printf("Left of . didn't resolve to a struct or union declaration");
            }
        }break;
        
        
        case Expression_Call:
        {
            declaration *resolved_decl = ExpressionResolveDeclaration(expr->call_expr);
            if(resolved_decl)
            {
                if(resolved_decl->type == Declaration_Procedure)
                {
#if 0
                    //do type checking here
                    for(expression *call_arg = expr->call_args;
                        call_arg;
                        call_arg = call_arg->next)
                    {
                        for(declaration *decl_arg = PROC_ARGS(resolved_decl);
                            decl_arg && decl_arg->type == Declaration_ProcedureArgs;
                            decl_arg = decl_arg->next)
                        {
                            
                        }
                    }
#endif
                }
                else printf("Proc call expr wasn't resolved to a procedure!");
            }
            else
            {
                printf("Proc call expr wasn't resolved to a declaration");
            }
        }break;
        
        
        case Expression_ArraySubscript:
        declaration *resolved_decl = ExpressionResolveDeclaration(expr->array_expr);
        if(resolved_decl)
        {
            if(resolved_decl->type == TypeSpec_Ptr ||
               resolved_decl->type == TypeSpec_Array)
            {
                Assert(expr->array_index_expr_as_const);
            }
            else
            {
                printf("Array expression didn't resolve to a pointer or array declaration!");
            }
        }
        else
        {
            printf("array identifer wasn't resolved to a declaration!");
        }
        break; 
        
        
        case Expression_SizeOf:
        break;
        
        case Expression_Compound:
        break;
        case Expression_CompoundInitializer:
        break;
        case Expression_Ternary:
        break;
        case Expression_Cast:
        break;
        
        case Expression_ToBeDefined:
        default: Panic();
    }
}

//NOTE all exprs that should be constant should be resolved in the fixup pass

internal void
CheckStatement(statement *stmt, declaration_list *scope)
{
    if(stmt->type == Statement_Expression ||
       stmt->type == Statement_Return)
    {
        CheckExpression(stmt->expr, scope);
    }
    
    
    else if(stmt->type == Statement_If ||
            stmt->type == Statement_Switch ||
            stmt->type == Statement_While)
    {
        CheckExpression(stmt->cond_expr, scope);
        CheckStatement(stmt->cond_stmt, scope);
    }
    else if(stmt->type == Statement_Else)
    {
        Assert(!stmt->cond_expr);
        CheckStatement(stmt->cond_stmt, scope);
    }
    else if(stmt->type == Statement_SwitchCase) 
    {
        if(!stmt->cond_expr_as_const)
        {
            printf("case statement doesn't have constant value!");
        }
        CheckStatement(stmt->cond_stmt, scope);
    }
    
    else if(stmt->type == Statement_For) 
    {
        CheckStatement(stmt->for_init_stmt, scope);
        CheckExpression(stmt->for_expr2, scope);
        CheckExpression(stmt->for_expr3, scope);
        CheckStatement(stmt->for_stmt, scope);
    }
    else if(stmt->type == Statement_Defer)
    {
        CheckStatement(stmt->defer_stmt, scope);
    }
    else if(stmt->type == Statement_Compound)
    {
        CheckDeclarationList(&stmt->decl_list);
        for(statement *compound = stmt->compound_stmts;
            compound;
            compound = compound->next)
        {
            CheckStatement(compound, scope);
        }
        
    }
    else if(stmt->type == Statement_Break ||
            stmt->type == Statement_Continue ||
            stmt->type == Statement_Null ||
            stmt->type == Statement_Declaration) //this should always be checked already
    
    {
        //ignore
    }
    else Panic();
}

internal void 
CheckDeclaration(declaration *decl, declaration_list *scope)
{
    //NOTE we don't have to check for name collisions because always done on DeclListAppend()
    switch(decl->type)
    {
        case Declaration_Struct: case Declaration_Union:
        case Declaration_Enum: case Declaration_EnumFlags:
        Assert(decl->members);
        Assert(decl->list.above == scope);
        for(declaration *member = decl->members;
            member;
            member = member->next)
        {
            CheckDeclaration(member, &decl->list);
        }
        break;
        
        case Declaration_Procedure:
        CheckStatement(decl->proc_body, scope);
        break;
        
        case Declaration_Variable: 
        if(decl->initializer)
        {
            CheckExpression(decl->initializer, scope);
        }
        break;
        
        case Declaration_Typedef: 
        Panic();
        break;
        
        case Declaration_EnumMember:
        case Declaration_Constant:
        case Declaration_Include: 
        case Declaration_Insert:
        if(!decl->expr_as_const)
        {
            //TODO store error output info within decls
            printf("Enum member %s doesn't have a constant value", decl->identifier);
        }
        break;
        
        case Declaration_ForeignBlock:
        // do nothing
        break;
        
        case Declaration_ProcedureArgs:
        default: Panic();
    }
}


internal void
CheckDeclarationList(declaration_list *ast)
{
    for(declaration *decl = ast->decls;
        decl;
        decl = decl->next)
    {
        CheckDeclaration(decl, ast);
    }
}
#endif

internal void
PrintTypeTable()
{
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        PrintTypeSpecifier(type_table.types + i);
    }
}

int main(int argc, char **argv)
{
    char *working_directory = 0;
    char *filename = 0;
    char *filepath = 0;
    char *output_filename = 0;
    char *output_filepath = 0;
    if(argc > 1)
    {
        filename = argv[1];
        if(!FileHasExtension(filename, "cc", 2))
        {
            printf("Please give me a .cc file.\n");
            return 0;
        }
    }
    else
    {
        printf("Give me a file to parse please!\n");
        return 0;
    }
    
    
    
    memory_arena big_arena;
    big_arena.size = Gigabytes(2);
    //big_arena.base = malloc(big_arena.size);
    //big_arena.base = VirtualAlloc(0, big_arena.size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    big_arena.base = PlatformAllocateMemory(big_arena.size);
    
    big_arena.used = 0;
    
    ast_arena = PushMemoryArena(&big_arena, ast_arena_size);
    string_intern_table.entries = 0;
    string_intern_table.arena = PushMemoryArena(&big_arena, Megabytes(512));
    
    
    expr_zero = NewExpression(Expression_Integer);
    expr_zero->integer = 0;
    expr_zero_struct = NewExpression(Expression_CompoundInitializer);
    expr_zero_struct->compound_expr = expr_zero;
    
    
    stmt_null = NewStatement(Statement_Null);
    
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
    AddInternalType(StringInternLiteral("void"), Internal_Void);
    
    
    
    
    
    
    u32 working_directory_length;
    
    {
        char temp_working_directory[MAX_PATH];
        working_directory_length = GetCurrentDirectory(MAX_PATH, temp_working_directory);
        working_directory = StringIntern(temp_working_directory, working_directory_length + 1);
        working_directory[working_directory_length] = '\\';
    }
    printf("Working directory:%s\n", working_directory);
    filepath = ConcatCStringsIntern(working_directory, filename);
    printf("Filepath:%s\n", filepath);
    
    for(u32 i = 2; i < (u32)argc; ++i)
    {
        char *option = argv[i];
        if(StringMatchLiteral(option, "/out"))
        {
            output_filename = argv[++i];
            if(!output_filename)
            {
                printf("Specify output file name!\n");
            }
        }
        //printf("%s\n", option);
    }
    
    
    if(!output_filename)
    {
        output_filepath = StringIntern(filename, StringLength(filepath) - 1);
        for(char *at = output_filepath; *at; at++)
        {
            if(*at == '.')
            {
                *++at = 'c';
                break;
            }
        }
    }
    
    if(!FileHasExtension(output_filename, "c", 1))
    {
        printf("Output file must end with .c!");
        return 0;
    }
    else
    {
        output_filepath = ConcatCStringsIntern(working_directory, output_filename);
    }
    
    declaration_list ast = {0};
    
    platform_file file = PlatformReadFile(filepath);
    if(file.data)
    {
        printf("Working directory %s\n", working_directory);
        printf("Parsing %s\n", filename);
        printf("Outputing to %s\n\n", output_filename);
        
        
        lexer_state lexer;
        lexer.start = file.data;
        lexer.at = lexer.start;
        lexer.line_at = 0;
        lexer.num_characters = file.size;
        lexer.file = filename;
        //lexer.past_file = lexer.file;
        
        ParseAST(&ast, &lexer);
        declaration *pad = NewDeclaration(Declaration_Constant);
        pad->identifier = StringInternLiteral("__padding__"); //HACK to make DeclListAppend always workd
        pad->expr_as_const = expr_zero;
        pad->actual_expr = expr_zero;
        DeclarationListAppend(&ast, pad);
    }
    else
    {
        printf("Couldn't open the file to parse!");
        return 0;
    }
    
    
#if 0
    printf("String intern table entries...\n");
    for(string_intern_entry *entry = string_intern_table.entries;
        entry;
        entry = entry->next)
    {
        printf("%s\n", entry->string);
    }
    printf("\n\n");
#endif
    
    
    
    if(ast.decls)
    {
        
#if DEBUG_BUILD
        PrintAST(&ast);
        
#endif
        
        AstFixupC(&ast); 
        PrintAST(&ast);
        
        //Does type checking and expression checking
        //CheckDeclarationList(&ast);
        Panic("Stop!!!");
        
        
        
        //debug_decl_lookup_list = &ast;
        //OutputC(&ast, output_filepath);
    }
    return 0;
}
