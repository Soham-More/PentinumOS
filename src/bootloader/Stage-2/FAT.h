#pragma once

#include "includes.h"
#include "File.h"

bool initialiseFAT(uint8_t drive);
bool searchFile(const char* filename);

// Open a file
FILE* fopen(const char* filename);

// Get a charecter from a file
char fgetc(FILE* file);

// get if current postion is EOF
bool isEOF(FILE* file);

// size for buffer larger than file will stop reading before buffer size limit is reached
// can handle buffers after 1 MiB limit
bool fread(FILE* file, char* buffer, uint32_t size);

// get file size
uint32_t fsize(FILE* file);

// seek in file
bool fseek(FILE* file, uint32_t value, uint8_t seek_mode);

// Close a opened file
void fclose(FILE* file);
