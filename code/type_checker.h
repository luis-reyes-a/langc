#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

typedef enum
{
    TypeSpec_Internal,
    TypeSpec_StructUnion,
    TypeSpec_Enum,
    TypeSpec_Ptr,
    TypeSpec_Array,
    TypeSpec_Proc,
}type_specifier_type;

typedef enum
{
    Internal_S8, //NOTE think about this for a second
    Internal_S16,
    Internal_S32,
    Internal_S64,
    Internal_U8,
    Internal_U16,
    Internal_U32,
    Internal_U64,
    Internal_Float,
    Internal_Double,
} internal_typespec_type;


typedef struct type_specifier
{
    type_specifier_type type;
    union
    {
        struct
        {
            char *identifier; //structs/unions/enums
            //declaration *user_decl;
            bool32 needs_constructor;
        };
        
        struct //internal types
        {
            char *internal_identifier;
            internal_typespec_type internal_type;
        };
        struct
        {
            struct type_specifier *ptr_base_type; //NOTE think about this
            u32 star_count;
        };
        struct
        {
            struct type_specifier *array_type; //NOTE think about this
            struct type_specifier *array_base_type;
            //I'm tired of mixing up the order of header files for this!
            void *array_size_expr; //should be expression *
            u64 array_size;
            
            //TODO expression array_size;
            
        };
        
        struct//TODO think about this
        {
            struct type_specifier *proc_return_types;
            struct type_specifier *proc_arg_types;
            u32 proc_arg_count;
            u32 proc_return_count;
        };
    };
}type_specifier;


struct //NOTE all types are stored in a type table that can be looked up for type checking
{
    type_specifier types[1024];
    u32 num_types;
} type_table;

inline type_specifier *AddInternalType(char *identifier, internal_typespec_type type);
internal type_specifier *AddStructUnionType(char *identifier);
internal type_specifier *AddEnumType(char *identifier);
internal type_specifier *AddPointerType(type_specifier *base, u32 star_count);
internal type_specifier *AddArrayType(type_specifier *base, type_specifier *type, u64 size);
internal type_specifier *AddProcedureType(char *identifier);



internal type_specifier *FindBasicType(char *identifier);//basic or struct types
internal type_specifier *FindPointerType(type_specifier *base, u32 star_count);
internal type_specifier *FindArrayType(type_specifier *base, u64 array_size);


#endif