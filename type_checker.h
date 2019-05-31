#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

typedef enum
{
    TypeSpec_Internal,
    TypeSpec_Struct,
    TypeSpec_Union,
    TypeSpec_Enum, 
    TypeSpec_EnumFlags, 
    TypeSpec_UnDeclared,
    
    TypeSpec_Ptr,
    TypeSpec_Array,
    //TypeSpec_Proc,
}type_specifier_type;

typedef enum
{
    Internal_S8, 
    Internal_S16,
    Internal_S32,
    Internal_S64,
    Internal_U8,
    Internal_U16,
    Internal_U32,
    Internal_U64,
    Internal_Float,
    Internal_Double,
    Internal_Void,
} internal_typespec_type;

#include "fixup.h"

typedef enum{
    TypeSpecFlags_TypeDeclaredAfterUse = 1 << 0,
} type_specifier_flags;

typedef struct type_specifier
{
    type_specifier_type type;
    type_specifier_flags flags;
    union
    {
        struct
        {
            char *identifier; 
            internal_typespec_type internal_type;
            bool32 needs_constructor;
        };
        struct
        {
            struct type_specifier *base_type;
            struct type_specifier *last_type;
            void *size_expr; 
            u64 size; //for ptrs this is num_asterisks
        };
    };
}type_specifier;





//TODO hash table
struct
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



internal type_specifier *find_type(char *identifier);//basic or struct types
internal type_specifier *find_ptr_type(type_specifier *base, u32 star_count);
internal type_specifier *find_array_type(type_specifier *base, u64 array_size);


type_specifier *internal_type_s8; 
type_specifier *internal_type_s16;
type_specifier *internal_type_s32;
type_specifier *internal_type_s64;
type_specifier *internal_type_u8;
type_specifier *internal_type_u16;
type_specifier *internal_type_u32;
type_specifier *internal_type_u64;
type_specifier *internal_type_float;
type_specifier *internal_type_double;
type_specifier *internal_type_char;
type_specifier *internal_type_void;


#endif