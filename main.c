
#include "stdint.h"

#define internal static
#define true 1
#define false 0
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
//typedef u32 bool32;
#define u32_max UINT32_MAX
#define u32(cast) (u32)(cast)
#define u64(cast) (u64)(cast)
#define float(cast) (float)(cast)
#define array_count(array) (sizeof(array) / sizeof(array[0]))

#define crash (*( (int*)0 ) = 1)
//#define crash do{ int *ptr = 0; *ptr = 1; }while(0)
#define panic(...) crash
#define assert(expr) if(!(expr)) panic("Assert fired!")
#define asserted(expr) (expr) ? (expr) : (panic("Assert fired"), 0)

#include "stdio.h" //TODO get rid of printf
#include "stdlib.h" //TODO get rid of malloc


#define DEBUG_BUILD 1

inline void 
print_tabs(u32 tabs)
{
    while(tabs--)  printf("\t");
}


#include "memory_arena.h"
#include "strings.c"
#include "ast.h"

inline u64
pow64(u64 base, u64 times)
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
platform_virtual_alloc(u64 size)
{
    return VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

//need thisNOTE
#include "cout.c"


//#include "ast.c"
//#include "ast2.c"

//need thisNOTE
#include "ast3.c"
#include "print.c"


typedef struct debug_read_file_result
{
    u32 size;
    void *data;
} platform_file;

inline u32
SafeTruncateUInt64(u64 Value)
{
    assert(Value <= 0xFFFFFFFF);
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
                    panic();
                }
            }
            else
            {
                panic();
            }
        }
        else
        {
            panic();
        }
        
        CloseHandle(FileHandle);
    }
    else
    {
        panic();
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
    big_arena.base = platform_virtual_alloc(big_arena.size);
    
    big_arena.used = 0;
    
    ast_arena = push_memory_arena(&big_arena, ast_arena_size);
    string_intern_table.entries = 0;
    string_intern_table.arena = push_memory_arena(&big_arena, Megabytes(512));
    
    
    expr_zero = NewExpression(Expression_Integer);
    expr_zero->integer = 0;
    expr_zero_struct = NewExpression(Expression_CompoundInitializer);
    expr_zero_struct->members = NewExpression(Expression_Member);
    expr_zero_struct->members->member_expression = expr_zero;
    
    
    stmt_null = NewStatement(Statement_Null);
    
    internal_type_s8 = make_internal_type(StringInternLiteral("s8"), Internal_S8); 
    internal_type_s16 = make_internal_type(StringInternLiteral("s16"), Internal_S16);
    internal_type_s32 = make_internal_type(StringInternLiteral("s32"), Internal_S32);
    internal_type_s64 = make_internal_type(StringInternLiteral("s64"), Internal_S64);
    internal_type_u8 = make_internal_type(StringInternLiteral("u8"), Internal_U8);
    internal_type_u16 = make_internal_type(StringInternLiteral("u16"), Internal_U16);
    internal_type_u32 = make_internal_type(StringInternLiteral("u32"), Internal_U32);
    internal_type_u64 = make_internal_type(StringInternLiteral("u64"), Internal_U64);
    internal_type_float = make_internal_type(StringInternLiteral("float"), Internal_Float);
    internal_type_double = make_internal_type(StringInternLiteral("double"), Internal_Double);
    internal_type_char = make_internal_type(StringInternLiteral("char"), Internal_U8);
    internal_type_void = make_internal_type(StringInternLiteral("void"), Internal_Void);
    
    
    
    
    
    
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
        declaration *pad = new_decl(Declaration_Constant);
        pad->identifier = StringInternLiteral("__padding__"); //HACK to make DeclListAppend always workd
        pad->initializer_as_const = expr_zero;
        pad->initializer = expr_zero;
        decl_list_append(&ast, pad); //prob don't need this anymore NOTE
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
        DebugBreak();
        //symbol_list sym_list = MakeSymbolList(512, 0);
        //AST_SymbolList(&ast, &sym_list);
        //DebugBreak();
        //PrintSymbolList(&sym_list);
        OutputC(&ast, output_filepath);
        
#endif
    }
    return 0;
}
