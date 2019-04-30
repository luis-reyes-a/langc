#ifndef STRINGS_H
#define STRINGS_H

typedef struct string_intern_entry
{
    struct string_intern_entry *next;
    u32 length;
#if DEBUG_BUILD
    char *string;
#endif
} string_intern_entry;

struct
{
    string_intern_entry *entries;
    memory_arena arena;
}string_intern_table;



internal char *ConcatCStringsIntern(char * string1, char * string2);
internal char * StringIntern(char * string, u32 length);

inline  u32 StringLength(char * string);
internal u64 StringLength64(char * string);
inline bool32 StringsMatchLength(char * a, char * b, u32 length);

internal void MovePassWhitespace(char * * at);

inline int IsWhiteSpace(char c);
inline bool32 IsDigit(char c);
inline bool32 IsUppercase(char c);
inline bool32 IsLowercase(char c);
inline bool32 IsAlpha(char c);
inline bool32 IsAlphaNumeric(char c);

//TODO
//inline u64 AtoiOctal(char * string, u32 length);
//inline u64 AtoiHexadecimal(char * string, u32 length);
//internal atoi_result AtoiUnsigned(char * string, u32 length);


typedef struct 
{
    u32 index;
    u32 length;
} string_array_search_result;

internal string_array_search_result IndexInStringArray(char * at, 
                                                       char **array, u32 array_count);


inline char * StringFromInternEntry(string_intern_entry * entry);


#endif