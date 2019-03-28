typedef struct v2
{
    float x;
    float y;
    
}v2;


typedef union v3
{
    struct 
    {
        float x;
        float y;
        float z;
        
    };
    
    struct 
    {
        float r;
        float g;
        float b;
        
    };
    
    float E[3];
    
}v3;


typedef struct v26
{
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
    float g;
    float h;
    float i;
    float j;
    float k;
    float l;
    float m;
    float n;
    float o;
    float p;
    float q;
    float r;
    float s;
    float t;
    float u;
    float v;
    float w;
    float x;
    float y;
    float z;
    
}v26;


v26 vector;

float f;

float f;

typedef enum 
{
    Node_First,
    Node_Second,
    Node_Third,
    Node_Fourth,
    
}node_type;


typedef struct node
{
    node * next;
    node_type type;
    union 
    {
        float a;
        uint32_t b;
        uint8_t c;
        uint8_t * str;
        
    };
    
    
}node;


static uint32_t
main(uint32_t argc, uint8_t ** argv)
{
    float r;
    float g;
    float b;
    float r;
    float g;
    float b;
    return 0;
    
}


