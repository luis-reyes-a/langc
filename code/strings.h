#ifndef STRINGS_H
#define STRINGS_H

typedef struct string_intern_entry
{
    struct string_intern_entry *next;
    u32 length;
} string_intern_entry;

inline char *
StringFromInternEntry(string_intern_entry *entry)
{
    return (char *)(entry + 1);
}


struct
{
    string_intern_entry *entries;
    memory_arena arena;
}string_intern_table;

inline int
IsWhiteSpace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' ||
       c == '\r' || c == '\b')
    {
        return true;
    }
    else return false;
}

inline bool32 //NOTE does not compare the null terminator!
StringsMatchLength(char *a, char *b, u32 length)
{
    for(u32 i = 0; i < length; ++i)
    {
        if(a[i] == 0 || b[i] == 0)
        {
            return false; //NOTE passing in length presumes you know length of one at least
            //so if it reaches here, one of them is obviously different
        }
        if(a[i] != b[i])
        {
            return false;
        }
    }
    
    if((a[length] == 0 || IsWhiteSpace(a[length])) &&
       (b[length] == 0 || IsWhiteSpace(b[length])))
    {
        return true;
    }
    else return false;
}

#define StringMatchesLiteral(string, literal) StringsMatchLength(string, literal, sizeof(literal) - 1)

#define StringInternLiteral(literal) StringIntern(literal, sizeof(literal) - 1)

internal char *
StringIntern(char *string, u32 length) //NOTE length doesn't include terminator
{
    for(string_intern_entry *entry = string_intern_table.entries;
        entry;
        entry = entry->next)
    {
        if(entry->length == length)
        {
            char *intern_string = StringFromInternEntry(entry);
            if(StringsMatchLength(intern_string, string, length))
            {
                return intern_string;
            }
        }
    }
    //didn't find the string in the table, add it
    u32 size = sizeof(string_intern_entry) + length + 1;
    string_intern_entry *new_entry = PushMemory(&string_intern_table.arena, size);
    new_entry->length = length;
    char *new_string = StringFromInternEntry(new_entry);
    for(u32 i = 0; i < length; ++i)
    {
        new_string[i] = string[i];
    }
    new_string[length] = 0;
    //add entry into table at head end
    new_entry->next = string_intern_table.entries;
    string_intern_table.entries = new_entry;
    return new_string;
}

inline bool32
IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}
inline bool32
IsUppercase(char c)
{
    return (c >= 'A'  && c <= 'Z');
}
inline bool32
IsLowercase(char c)
{
    return (c >= 'a' && c <= 'z');
}
inline bool32
IsAlpha(char c)
{
    return (IsUppercase(c) || IsLowercase(c));
}
inline bool32
IsAlphaNumeric(char c)
{
    return (IsAlpha(c) || IsDigit(c));
}

inline u64
AtoiOctal(char *string, u32 length)
{
    Panic();
    return 0;
}
inline u64
AtoiHexadecimal(char *string, u32 length)
{
    Panic();
    return 0;
}

typedef struct
{
    bool32 is_integer;
    u32 parse_length;
    union
    {
        u64 integer_value;
        double floating_value;
    };
}atoi_result;

internal atoi_result // NOTE parse length of 0 indicates unsuccessful
AtoiUnsigned(char *string, u32 length)
{
    atoi_result result;
    result.is_integer = true;
    result.parse_length = 0;
    result.integer_value = 0;
    
    while(IsDigit(*string) || *string == '.' || *string == 'f')
    {
        if(*string  == '.')
        {
            if(result.is_integer)
            {
                result.is_integer = false;
            }
            else
            { //error
                result.parse_length = 0;
                break;
            }
        }
        
        if(result.is_integer)
        {
            result.integer_value;
        }
        else
        {
            
        }
        
        ++string;
        ++result.parse_length;
    }
    
    return result;
}


internal u64 //NOTE doesn't count null terminator
StringLength64(char *string)
{
    u32 length = 0;
    while(*string)
    {
        ++length, ++string;
    }
    return length;
}


inline u32
StringLength(char *string)
{
    u64 result = StringLength64(string);
    if(result <= u32_max)
    {
        return u32(result);
    }
    else
    {
        Panic(); //TODO remove this
        return 0;
    }
}

//memory_arena string_builder_arena;

internal char *
ConcatCStringsIntern(char *string1, char *string2)
{
    u32 len1 = StringLength(string1);
    u32 len2 = StringLength(string2);
    
    char temp[512];
    Assert(len1 + len2 <= ArrayCount(temp));
    char *at = temp;
    while(*string1)
    {
        *at++ = *string1++;
    }
    while(*string2)
    {
        *at++ = *string2++;
    }
    
    return StringIntern(temp, len1 + len2);
}

//TODO think about what I need register?


typedef struct 
{
    u32 index;
    u32 length;
} string_array_search_result;


static string_array_search_result
IndexInStringArray(char *at, char **array, u32 array_count)
{
    string_array_search_result result = {0};
    for(u32 i = 0; i < array_count; ++i)
    {
        char *test = array[i];
        u32 length = StringLength(test); //TODO speed up
        if(StringsMatchLength(at, test, length))
        {
            result.index = i;
            result.length = length;
            break;
        }
    }
    return result;
}



static void
MovePassWhitespace(char **at)
{
    while(IsWhiteSpace(**at))//TODO more
    {
        ++*at;
    }
}


#endif