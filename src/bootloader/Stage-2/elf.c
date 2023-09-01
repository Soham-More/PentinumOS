#include "elf.h"

#include "IO.h"
#include "String.h"
#include "memdefs.h"
#include "Memory.h"

#define ELF_MAGIC_VALUE "\x7F" "ELF"

#define callf(x) \
    if(!x) \
    { \
        printf("ELF Error: Failed to load elf file %s\n\tFunction: %s", elfFilePath, #x); \
        fclose(elfFile); \
        return false; \
    }

enum TargetBits
{
    ELF_32BIT = 1,
    ELF_64BIT = 2,
};

enum EndianType
{
    ELF_LITTLE_ENDIAN = 1,
    ELF_BIG_ENDIAN = 2,
};

enum ELFType
{
    ELF_Relocatable = 1,
    ELF_Executable = 2,
    ELF_Shared = 3,
    ELF_Core = 4,
};

enum ELFProgramType
{
    // Program header table entry unused.
    ELF_PROGRAM_TYPE_NULL           = 0,

    // Loadable segment.
    ELF_PROGRAM_TYPE_LOAD           = 1,

    // Dynamic linking information.
    ELF_PROGRAM_TYPE_DYNAMIC        = 2,

    // Interpreter information.
    ELF_PROGRAM_TYPE_INTERP         = 3,

    // Auxiliary information.
    ELF_PROGRAM_TYPE_NOTE           = 4,

    // Reserved
    ELF_PROGRAM_TYPE_SHLIB          = 5,

    // Segment containing program header table itself.
    ELF_PROGRAM_TYPE_PHDR           = 6,

    // Thread-Local Storage template.
    ELF_PROGRAM_TYPE_TLS            = 7,

    // Reserved inclusive range. Operating system specific.
    ELF_PROGRAM_TYPE_LOOS           = 0x60000000,
    ELF_PROGRAM_TYPE_HIOS           = 0x6FFFFFFF,

    // Reserved inclusive range. Processor specific.
    ELF_PROGRAM_TYPE_LOPROC         = 0x70000000,
    ELF_PROGRAM_TYPE_HIPROC         = 0x7FFFFFFF,
};

enum ELFInstructionSet
{
    ELF_INSTRUCTION_SET_NONE    = 0,
    ELF_INSTRUCTION_SET_X86     = 3,
    ELF_INSTRUCTION_SET_ARM     = 0x28,
    ELF_INSTRUCTION_SET_X64     = 0x3E,
    ELF_INSTRUCTION_SET_ARM64   = 0xB7,
    ELF_INSTRUCTION_SET_RISCV   = 0xF3,
};

typedef struct ELFHeader
{
    char        magic[4];                       // 0x0
    uint8_t     targetBits;                     // 0x4
    uint8_t     endianType;                     // 0x5
    uint8_t     headerVersion;                  // 0x6
    uint8_t     ABI;                            // 0x7
    uint8_t     _unused[8];                     // 0x8
    uint16_t    type;                           // 0x10
    uint16_t    instructionSet;                 // 0x12
    uint32_t    elfVersion;
    uint32_t    programEntryPoint;
    uint32_t    programHeaderTablePosition;
    uint32_t    sectionHeaderPosition;
    uint32_t    flags;
    uint16_t    headerSize;
    uint16_t    programHeaderTableEntrySize;
    uint16_t    programHeaderTableEntryCount;
    uint16_t    sectionHeaderTableEntrySize;
    uint16_t    sectionHeaderTableEntryCount;
    uint16_t    sectionNamesIndex;
} _packed ELFHeader;

typedef struct ELFProgramHeader
{
    uint32_t segmentType;
    uint32_t segmentOffset;
    uint32_t segmentVirtualaddress;
    uint32_t _undefinedABI;
    uint32_t segmentFileSize;
    uint32_t segmentMemorySize;
    uint32_t flags;
    uint32_t requiredAlign;
} _packed ELFProgramHeader;

typedef struct KernelMapEntry
{
    uint32_t sectionBegin;
    uint32_t sectionSize;
}_packed KernelMapEntry;

typedef struct KernelMap
{
    uint32_t entryCount;
    KernelMapEntry* entries;
}_packed KernelMap;

bool loadELF(char* elfFilePath, void** entryPoint, void** kernelMap)
{
    uint8_t* headerBuffer = (uint8_t*)MEMORY_ELF_ADDR;
    uint8_t* loadBuffer = (uint8_t*)KERNEL_LOAD_ADDR;

    FILE* elfFile = fopen(elfFilePath);

    if(elfFile == nullptr)
    {
        printf("ELF Error: Failed to load elf file %s\n", elfFilePath);
        return false;
    }

    // load the program header
    ELFHeader elf_header;

    if(!fread(elfFile, (char*)&elf_header, sizeof(elf_header)))
    {
        printf("ELF Error: Failed to read elf file %s\n", elfFilePath);
        fclose(elfFile);
        return false;
    }

    // validate header
    bool ok = true;
    ok = ok && (strcmp(elf_header.magic, ELF_MAGIC_VALUE, 4) != 0);
    ok = ok && (elf_header.targetBits == ELF_32BIT);
    ok = ok && (elf_header.endianType == ELF_LITTLE_ENDIAN);
    ok = ok && (elf_header.headerVersion == 1);
    ok = ok && (elf_header.elfVersion == 1);
    ok = ok && (elf_header.type == ELF_Executable);
    ok = ok && (elf_header.instructionSet == ELF_INSTRUCTION_SET_X86);

    if(!ok)
    {
        printf("ELF Error: Invalid elf file %s\n", elfFilePath);
        return false;
    }

    *entryPoint = (void*)elf_header.programEntryPoint;

    // load program header
    uint32_t phOffset = elf_header.programHeaderTablePosition;
    uint32_t phSize = elf_header.programHeaderTableEntrySize * elf_header.programHeaderTableEntryCount;
    uint32_t phTableEntrySize = elf_header.programHeaderTableEntrySize;
    uint32_t phTableEntryCount = elf_header.programHeaderTableEntryCount;

    // allocate sectors for kernel memory map
    *kernelMap = alloc(1);

    KernelMap* kMap = *kernelMap;
    kMap->entryCount = phTableEntryCount;
    kMap->entries = alloc(bytesToSectors(phTableEntryCount * sizeof(KernelMapEntry)));

    // seek to pheaders
    callf(fseek(elfFile, phOffset, FAT_SEEK_BEGINNING))

    callf(fread(elfFile, (char*)headerBuffer, phSize));

    ELFProgramHeader* pHeaders = (ELFProgramHeader*)headerBuffer;

    for(uint32_t i = 0; i < phTableEntryCount; i++)
    {
        //printf("Loading Segment: type=%d, foffset=%d, vaddr=%x, fsize=%d, msize=%d\n", pHeaders[i].segmentType, pHeaders[i].segmentOffset, pHeaders[i].segmentVirtualaddress, pHeaders[i].segmentFileSize, pHeaders[i].segmentMemorySize);

        if(pHeaders[i].segmentType == ELF_PROGRAM_TYPE_LOAD)
        {
            uint8_t* vaddr = (uint8_t*)pHeaders[i].segmentVirtualaddress;

            kMap->entries[i].sectionBegin = (uint32_t)vaddr;
            kMap->entries[i].sectionSize = pHeaders[i].segmentMemorySize;

            // fill buffer with 0
            memset(vaddr, 0, pHeaders[i].segmentMemorySize);

            // seek to segment
            callf(fseek(elfFile, pHeaders[i].segmentOffset, FAT_SEEK_BEGINNING));

            // read the segment
            callf(fread(elfFile, (char*)vaddr, pHeaders[i].segmentFileSize));
        }
    }

    fclose(elfFile);

    return true;
}
