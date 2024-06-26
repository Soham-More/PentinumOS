#pragma once
#include "includes.h"

#define FIND_FAIL UINT16_MAX

typedef struct _SplitString
{
    uint32_t split_count;
    uint32_t split_string_size;
    char* string_array;
}SplitString;

uint32_t strlen(const char* string, const char termination_char);
bool strcmp(const char* a, const char* b, uint32_t length);

uint32_t count_char_occurrence(const char* string, char c);
uint32_t find_char(char* string, char c);

char* getSplitString(SplitString* splitString, uint32_t index);
void split_wrt_char(const char* string, char c, SplitString* splitString);

char toUpper(char c);
