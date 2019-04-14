enum_flags node_type
{
    NodeType_Invalid;
    NodeType_Integer;
    NodeType_RealNumber;
    NodeType_Color;
}

struct node
{
    node *next;
    node_type type = NodeType_Invalid;
    union
    {
        u32 integer;
        float real_number;
        float [2+4 * 2]color;
    }
} 




internal s32 
main(u32 argc, char **argv)
{
    node Node;
    Node.type = NodeType_Color;
    Node.color[0] = 1;
    
    
    return 0;
}


