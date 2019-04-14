
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

#include "cout.c"





int main(int argc, char **argv)
{
    memory_arena big_arena;
    big_arena.size = Gigabytes(2);
    big_arena.base = malloc(big_arena.size);
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
    
    FILE *file = fopen("../code/test1.cc", "r");
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
