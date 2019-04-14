typedef enum 
{
NodeType_Invalid = 1,
NodeType_Integer = 2,
NodeType_RealNumber = 4,
NodeType_Color = 8,
}node_type;


typedef struct node
{
struct node * next;
node_type type;
union 
{
uint32_t integer;
float real_number;
float color[10];
};

}node;


inline node
node__Constructor()
{
node result;
result.next = 0;
result.type = NodeType_Invalid;
result.integer = 0;
return result;
}


static int32_t
main(uint32_t argc, uint8_t ** argv)
{
node Node = node__Constructor( );
Node.type = NodeType_Color;
Node.color[0] = 1;
return 0;
}


