#pragma once

#include "includes.h"
#include "FAT.h"

bool loadELF(char* elfFilePath, void** entryPoint, void** kernelMap);
