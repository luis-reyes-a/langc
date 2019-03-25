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


#define PushType(arena, type)         (type *)PushMemory(arena, sizeof(type))
#define PushArray(arena, type, count) (type *)PushMemory(arena, sizeof(type)*count)

inline void *
PushMemory(memory_arena *arena, u32 size_to_push)
{
    void *result = 0;
    if(arena->used + size_to_push <= arena->size)
    {
        result = arena->base + arena->used;
        arena->used += size_to_push;
    }
    else
    {
        Panic();
    }
    return result;
}

inline memory_arena
PushMemoryArena(memory_arena *arena, u32 size)
{
    memory_arena result;
    result.base = PushMemory(arena, size);
    result.size = size;
    result.used = 0;
    return result;
}



#endif