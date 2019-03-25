#include "type_checker.h"

internal type_specifier *
FindBasicType(char *identifier)
{
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        type_specifier *typespec = type_table.types + i;
        if(typespec->type != TypeSpec_Ptr && typespec->type != TypeSpec_Array)
        {
            if(typespec->identifier == identifier)
            {
                result = typespec;
            }
        }
    }
    return result;
}

internal type_specifier *
FindPointerType(type_specifier *base, u32 star_count)
{
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        type_specifier *typespec = type_table.types + i;
        if(typespec->type == TypeSpec_Ptr && typespec->base_type == base)
        {
            if(typespec->star_count == star_count)
            {
                result = typespec;
            }
        }
    }
    return result;
}

internal type_specifier *
FindArrayType(type_specifier *base, u64 array_size)
{
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        type_specifier *typespec = type_table.types + i;
        if(typespec->type == TypeSpec_Array && typespec->array_base == base)
        {
            if(typespec->array_size == array_size)
            {
                result = typespec;
            }
        }
    }
    return result;
}

inline type_specifier *
NewTypeTableEntry()
{
    if(type_table.num_types < ArrayCount(type_table.types))
    {
        return type_table.types + type_table.num_types++;
    }
    else return 0;
}

inline type_specifier *
AddInternalType(char *identifier, internal_typespec_type type)
{
    type_specifier *typespec = NewTypeTableEntry();
    typespec->type = TypeSpec_Internal;
    typespec->internal_identifier = identifier;
    typespec->internal_type = type;
    return typespec;
}

internal type_specifier *
AddStructUnionType(char *identifier)
{
    type_specifier *typespec = NewTypeTableEntry();
    typespec->type = TypeSpec_StructUnion;
    typespec->identifier = identifier;
    return typespec;
}

internal type_specifier *
AddEnumType(char *identifier)
{
    type_specifier *typespec = NewTypeTableEntry();
    typespec->type = TypeSpec_Enum;
    typespec->internal_identifier = identifier;
    return typespec;
}

internal type_specifier *
AddPointerType(type_specifier *base, u32 star_count)
{
    type_specifier *typespec = NewTypeTableEntry();
    Assert(base->type != TypeSpec_Ptr);
    Assert(star_count);
    typespec->type = TypeSpec_Ptr;
    typespec->base_type = base;
    typespec->star_count = star_count;
    return typespec;
}

internal type_specifier *
AddArrayType(type_specifier *base, u64 size)
{
    type_specifier *typespec = NewTypeTableEntry();
    Assert(size);
    typespec->type = TypeSpec_Array;
    typespec->array_base = base;
    typespec->array_size = size;
    return typespec;
}

internal type_specifier *
AddProcedureType(char *identifier)
{
    Panic();
    return 0;
}

internal void
PrintTypeSpecifier(type_specifier *typespec)
{
    switch(typespec->type)
    {
        case TypeSpec_Internal: 
        {
            switch(typespec->internal_type)
            {
                case Internal_S8: printf("Internal_S8");break;
                case Internal_S16: printf("Internal_S16");break;
                case Internal_S32: printf("Internal_S32");break;
                case Internal_S64: printf("Internal_S64");break;
                case Internal_U8: printf("Internal_U8");break;
                case Internal_U16: printf("Internal_U16");break;
                case Internal_U32: printf("Internal_U32");break;
                case Internal_U64: printf("Internal_U64");break;
                case Internal_Float: printf("Internal_Float");break;
                case Internal_Double: printf("Internal_Double");break;
                default: Panic();
            } 
        }break;
        
        case TypeSpec_StructUnion: case TypeSpec_Enum: 
        {
            printf("%s", typespec->identifier);
        }break;
        
        case TypeSpec_Ptr: 
        {
            printf("%s *%d", typespec->base_type->identifier, typespec->star_count);
        }break;
        
        case TypeSpec_Array: 
        { 
            PrintTypeSpecifier(typespec->array_base);
            printf("[%llu]", typespec->array_size);
        }break;
        
        case TypeSpec_Proc: 
        {
            Panic(); //TODO
        }break;
        default: Panic();
    }
}


internal void
PrintTypeTable()
{
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        PrintTypeSpecifier(type_table.types + i);
    }
}
