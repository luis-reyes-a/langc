include "stdint.h";
include "stdio.h"; //NOTE have to includes in top always!

union Vec3
{
    float [3]E;
    struct
    {
        float x, y, z;
    }
    struct
    {
        Vec2 xy;
        float z;
    }
}

struct Vec2
{
    float x, y;
}



enum_flags flags
{
    SET;
    OFF;
    USE;
    DONTUSE;
    OTHERSTUFF;
}

enum my_enum
{
    GREEN;
    BLUE;
    RED;
    YELLOW = 8;
    NEON;
}

internal void main(u32 argc, char **string)
{
    Vec3 vector;
    Vec3 [10 + 4]vectors;
    float result = vector.x + vectors[1].y;
    return result;
}

internal void main(u32 argc, u32 argc2)
{
    return 0;
}

internal void main(char *a, char **b)
{
    return 4 + 4 * 3;
}

internal void main(char**a, char **b)
{
    return 1 << 25;
}








