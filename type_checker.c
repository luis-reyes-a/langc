#include "type_checker.h"

//Finds structs\unions\enums
internal type_specifier *
find_type(char *identifier)
{
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        type_specifier *typespec = type_table.types + i;
        if(typespec->type == TypeSpec_Internal || typespec->type == TypeSpec_UnDeclared ||
           typespec->type == TypeSpec_Struct || typespec->type == TypeSpec_Union ||
           typespec->type == TypeSpec_Enum || typespec->type == TypeSpec_EnumFlags)
        
        {
            if(typespec->identifier == identifier)
            {
                result = typespec;
                break;
            }
        }
    }
    return result;
}

internal type_specifier *
find_ptr_type(type_specifier *base, u32 star_count)
{
    assert(star_count);
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        type_specifier *typespec = type_table.types + i;
        if(typespec->type == TypeSpec_Ptr && 
           typespec->base_type == base &&
           typespec->size == star_count)
        {
            result = typespec;
            break;
        }
    }
    return result;
}

internal type_specifier *
find_array_type(type_specifier *last_type, u64 size)
{
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        type_specifier *typespec = type_table.types + i;
        if(typespec->type == TypeSpec_Array && 
           typespec->last_type == last_type &&
           typespec->size == size)
        {
            result = typespec;
            break;
        }
    }
    return result;
}

inline type_specifier *
new_typetable_entry(type_specifier_type type)
{
    type_specifier *result = 0;
    if(type_table.num_types < array_count(type_table.types))
    {
        result = type_table.types + type_table.num_types++;
        *result = (type_specifier){0};
    } else panic();
    result->type = type;
    return result;
}

//maybe user only used the find_type procedures bc we always
//need to check type is not there before the add
inline type_specifier *
make_internal_type(char *identifier, internal_typespec_type type) 
{
    type_specifier *typespec = new_typetable_entry(TypeSpec_Internal);
    typespec->identifier = identifier;
    typespec->internal_type = type;
    return typespec;
}

inline type_specifier *
make_type(char *identifier, type_specifier_type type)
{
    type_specifier *result = new_typetable_entry(type);
    result->identifier = identifier;
    return result;
}


inline type_specifier *
make_ptr_type(type_specifier *last_type, u64 star_count)
{
    type_specifier *result = new_typetable_entry(TypeSpec_Ptr);
    result->last_type = last_type;
    result->size = star_count;
    if(last_type->type != TypeSpec_Ptr)
    {
        result->base_type = last_type;
    }
    else
    {
        result->base_type = last_type->base_type;
    }
    return result;
}

inline type_specifier *
make_array_type(type_specifier *last_type, u64 size, void *size_expr)
{
    type_specifier *result = new_typetable_entry(TypeSpec_Array);
    result->last_type = last_type;
    result->size = size;
    result->size_expr = size_expr;
    if(last_type->type != TypeSpec_Ptr && last_type->type != TypeSpec_Array)
    {
        result->base_type = last_type;
    }
    else
    {
        result->base_type = last_type->base_type;
    }
    return result;
}

inline type_specifier *
make_undecl_type(char *identifier)
{
    type_specifier *result = new_typetable_entry(TypeSpec_UnDeclared);
    result->identifier = identifier;
    return result;
}

internal type_specifier *
add_new_type(char *identifier, declaration_type decl_type)
{
    type_specifier *result = 0;
    type_specifier_type type = 0;
    switch(decl_type)
    {
        case Declaration_Struct:
        type = TypeSpec_Struct;break;
        case Declaration_Union:
        type = TypeSpec_Union;break;
        case Declaration_Enum:
        type = TypeSpec_Enum;break;
        case Declaration_EnumFlags:
        type = TypeSpec_EnumFlags;break;
        default: panic();
    }
    type_specifier *check = find_type(identifier);
    if(check)
    {
        if(check->type == TypeSpec_UnDeclared)
        {
            check->type = type;
            check->flags |= TypeSpecFlags_TypeDeclaredAfterUse;
            result = check;
        }
        else
        {
            panic("type redefinition!");
        }
    }
    else
    {
        result = make_type(identifier, type);
    }
    return result;
}




#if 0
internal type_specifier *
make_type(char *identifier, void *struct_decl)
{
    type_specifier *result = 0;
    type_specifier *check = FindBasicType(identifier);
    if(!check)
    {
        result = NewTypeTableEntry();
        result->type = TypeSpec_StructUnion;
        result->identifier = identifier;
    }
    else if(check->type == TypeSpec_UnDeclared)
    {
        
        check->type = TypeSpec_StructUnion;
        check->flags |= TypeSpecFlags_TypeDeclaredAfterUse;
        check->identifier = identifier;
        result = check;
    }
    else
    {
        Panic("Redefinition");
    }
    return result;
}

internal type_specifier *
add_type(char *identifier, type_specifier_type type)
{
    type_specifier *result = 0;
    type_specifier *check = find_type(identifier);
    if(check)
    {
        if(check->type == TypeSpec_UnDeclared)
        {
            check->type type;
            checl->flags |= TypeSpecFlags_TypeDeclaredAfterUse;
            result = check;
        }
        else
        {
            panic("type redefinition");
        }
    }
    else
    {
        result = 
            //make new type
    }
}

internal type_specifier *
AddNewEnumType(char *identifier, void *enum_decl)
{
    type_specifier *result = 0;
    type_specifier *check = FindBasicType(identifier);
    if(!check)
    {
        result = NewTypeTableEntry();
        result->type = TypeSpec_Enum;
        result->identifier = identifier;
    }
    else if(check->type == TypeSpec_UnDeclared)
    {
        Assert(check->fixup->decl && !check->fixup->decl2);
        check->fixup->decl2 = enum_decl;
        check->fixup = 0;
        
        check->type = TypeSpec_Enum;
        check->flags |= TypeSpecFlags_TypeDeclaredAfterUse;
        check->identifier = identifier;
        result = check;
    }
    else
    {
        Panic("Redefinition");
    }
    return result;
}

internal type_specifier *
AddPointerType(type_specifier *base, u32 star_count)
{
    type_specifier *typespec = NewTypeTableEntry();
    Assert(base->type != TypeSpec_Ptr);
    Assert(star_count);
    typespec->type = TypeSpec_Ptr;
    typespec->ptr_base_type = base;
    typespec->star_count = star_count;
    return typespec;
}

internal type_specifier *
AddArrayType(type_specifier *base, type_specifier *type, u64 size)
{
    type_specifier *typespec = NewTypeTableEntry();
    Assert(size);
    typespec->type = TypeSpec_Array;
    typespec->array_base_type = base;
    typespec->array_type = type;
    typespec->array_size = size;
    
    //char *[4]strings;
    
    return typespec;
}

internal type_specifier *
AddProcedureType(char *identifier)
{
    Panic();
    return 0;
}
#endif







#if 0
inline int
TypeIsIntegral(type_specifier *type)
{
    int result = false;
    if(type->type == TypeSpec_Internal)
    {
        switch(type->internal_type)
        {
            case Internal_S8: result = true; break; 
            case Internal_S16: result = true; break;
            case Internal_S32: result = true; break;
            case Internal_S64: result = true; break;
            case Internal_U8: result = true; break;
            case Internal_U16: result = true; break;
            case Internal_U32: result = true; break;
            case Internal_U64: result = true; break;
        }
    }
    return result;
}

inline int
TypeIsFloat(type_specifier *type)
{
    int result = false;
    if(type->type == TypeSpec_Internal)
    {
        switch(type->internal_type)
        {
            case Internal_Float: result = true; break;
            case Internal_Double: result = true; break;
        }
    }
    return result;
}

#endif