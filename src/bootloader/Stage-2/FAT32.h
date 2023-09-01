#pragma once

#include "includes.h"
#include "File.h"
#include "Disk.h"

bool FAT32_initialise(DISK bootdisk, uint32_t partition_offset);
bool FAT32_search(const char* filename);

void FAT32_pretty_print_path(const char* filepath);

// Open a file
FILE* FAT32_open(const char* filename);

// Get a charecter from a file
char FAT32_getc(FILE* file);

// get if current postion is EOF
bool FAT32_isEOF(FILE* file);

// size for buffer larger than file will stop reading before buffer size limit is reached
bool FAT32_read(FILE* file, char* buffer, uint32_t size);

// get file size
uint32_t FAT32_fsize(FILE* file);

bool FAT32_seek(FILE* file, uint32_t value, uint8_t seek_mode);

// Close a opened file
void FAT32_close(FILE* file);

void FAT32_table_hexdump();
