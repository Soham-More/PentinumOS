#pragma once

#include "includes.h"
#include "File.h"
#include "Disk.h"

bool FAT12_initialise(DISK bootdisk, uint32_t partition_offset);
bool FAT12_search(const char* filename);

void FAT12_pretty_print_path(const char* filepath);

// Open a file
FILE* FAT12_open(const char* filename);

// Get a charecter from a file
char FAT12_getc(FILE* file);

// get if current postion is EOF
bool FAT12_isEOF(FILE* file);

// size for buffer larger than file will stop reading before buffer size limit is reached
bool FAT12_read(FILE* file, char* buffer, uint32_t size);

// get file size
uint32_t FAT12_fsize(FILE* file);

bool FAT12_seek(FILE* file, uint32_t value, uint8_t seek_mode);

// Close a opened file
void FAT12_close(FILE* file);

void FAT12_table_hexdump();
