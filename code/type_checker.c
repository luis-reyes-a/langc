#include "type_checker.h"


//Finds structs\unions\enums
internal type_specifier *
FindBasicType(char *identifier)
{
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        type_specifier *typespec = type_table.types + i;
        if(typespec->type == TypeSpec_Internal ||
           typespec->type == TypeSpec_StructUnion ||
           typespec->type == TypeSpec_Enum ||
           typespec->type == TypeSpec_UnDeclared)
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
        if(typespec->type == TypeSpec_Ptr && typespec->ptr_base_type == base)
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
FindArrayType(type_specifier *type, u64 array_size)
{
    type_specifier *result = 0;
    for(u32 i = 0; i < type_table.num_types; ++i)
    {
        //NOTE the base type is not used here at all! Do I really need it?
        type_specifier *typespec = type_table.types + i;
        if(typespec->type == TypeSpec_Array && typespec->array_type == type)
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
    type_specifier *result = 0;
    if(type_table.num_types < ArrayCount(type_table.types))
    {
        result = type_table.types + type_table.num_types++;
        *result = (type_specifier){0};
    } else Panic();
    return result;
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

inline type_specifier *
AddUnDeclaredType(char *identifier)
{
    type_specifier *typespec = NewTypeTableEntry();
    typespec->type = TypeSpec_UnDeclared;
    typespec->undeclared_identifier = identifier;
    return typespec;
}



internal type_specifier *
AddNewStructUnionType(char *identifier, void *struct_decl)
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
        
        Assert(check->fixup->decl && !check->fixup->decl2);
        check->fixup->decl2 = struct_decl;
        check->fixup = 0;
        
        check->type = TypeSpec_StructUnion;
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



