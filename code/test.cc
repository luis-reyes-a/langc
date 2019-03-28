struct v2
{
    float x, y;
}

union v3
{
    struct
    {
        float x; float y, z;
    }
    struct
    {
        float r, g; float b;
    }
    float [3]E;
}

struct v26
{
    float a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z;
} vector, float f;

float f;

enum node_type
{
    Node_First;
    Node_Second;
    Node_Third;
    Node_Fourth;
}

struct node
{
    node *next;
    node_type type;
    union
    {
        float a;
        u32 b;
        char c;
        char *str;
    }
}


internal u32
main(u32 argc, char **argv)
{
    float r, g; 
    float b;
    float r, g; float b;
    return 0;
}