#include "String.h"
#include "IO.h"

uint32_t strlen(const char* string, const char termination_char)
{
    uint32_t length = 0;

    while (*(string + length) != termination_char)
    {
        length++;
    }
    
    return length;
}

bool strcmp(const char* a, const char* b, uint32_t length)
{
    for(uint32_t i = 0; i < length; i++)
    {
        if(a[i] != b[i])
        {
            return false;
        }
    }

    return true;
}

uint32_t count_char_occurrence(const char* string, char c)
{
    uint32_t length = 0;
    uint32_t occurrence = 0;

    while (*(string + length) != null)
    {
        length++;

        if(*(string + length) == c) occurrence++;
    }
    
    return occurrence;
}

uint32_t find_char(char* string, char c)
{
    uint32_t length = 0;

    while (*(string + length) != null)
    {
        if(*(string + length) == c) return length;

        length++;
    }
    
    return FIND_FAIL;
}

char* getSplitString(SplitString* splitString, uint32_t index)
{
    if(index >= splitString->split_count)
    {
        printf("ERROR: Invalid Index\n");
        return nullptr;
    }

    return splitString->string_array + (splitString->split_string_size * index);
}

void split_wrt_char(const char* string, char c, SplitString* splitString)
{
    uint32_t length = 0;
    uint32_t current_string_offset = 0;
    uint32_t split_string_index = 0;

    char currentChar = *string;

    while (currentChar != null)
    {
        if(split_string_index >= splitString->split_string_size)
        {
            printf("ERROR: cannot split string, space between splits is higher than %u", splitString->split_string_size);
            return;
        }

        if(currentChar == c)
        {
            splitString->string_array[current_string_offset + split_string_index] = null;
            current_string_offset += splitString->split_string_size;
            split_string_index = 0;
            length++;
            currentChar = string[length];
            continue;
        }

        splitString->string_array[current_string_offset + split_string_index] = currentChar;

        length++;
        split_string_index++;
        currentChar = *(string + length);
    }
}

char toUpper(char c)
{
    if(c >= 'a' && c <= 'z')
    {
        return c + 'A' - 'a';
    }

    return c;
}
