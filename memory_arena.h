#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#define Kilobytes(count) (1024LL * count)
#define Megabytes(count) (1024LL * Kilobytes(count))
#define Gigabytes(count) (1024LL * Megabytes(count))


typedef struct
{
    char *base;
    u32 used;
    u32 size;
}memory_arena;


#define PushType(arena, type)         push_memory(arena, sizeof(type))
#define PushArray(arena, type, count) push_memory(arena, sizeof(type)*count)

inline void *
push_memory(memory_arena *arena, u32 size_to_push)
{
    void *result = 0;
    if(arena->used + size_to_push <= arena->size)
    {
        result = arena->base + arena->used;
        arena->used += size_to_push;
    }
    else
    {
        
        panic();
    }
    return result;
}


inline memory_arena
push_memory_arena(memory_arena *arena, u32 size)
{
    memory_arena result;
    result.base = push_memory(arena, size);
    result.size = size;
    result.used = 0;
    return result;
}





#endif