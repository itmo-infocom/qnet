#ifndef ANB_PRIVATE_DEFS_H
#define ANB_PRIVATE_DEFS_H

#include "AnBDefs.h"

#define ANB_DEVICE_NAME "AnB"
#define ANB_VERSION     0xabcdef05

enum AnBCommands {
    Version = 0x7fab0015,
    RegisterWrite,          //unsafe
    RegisterRead,
    SetDeviceMode,
    SetDumpSource,
    SetDestTable,
    DMAEnable,
    DMADisable,
    DMASetBufferSize,
    DMARollBuffer,
    TableSetSize,
    TableSetMode,
    
    AnBCommand_Illegal
};

enum AnBModes {
    ModeRNG,
    ModeDAC,
    ModeCAL,
    
    AnBMode_Illegal
};

union AnBCommandArg {
    struct AnBRegInfo* reg_info;
    
    enum AnBDevices    device_type;
    enum AnBModes      device_mode;
    enum DumpSources   dump_source;
    enum DestTables    dest_table;
    enum AnBTableModes table_mode;
    
    unsigned int buffer_size;
    unsigned int table_size;

    unsigned int version;
    
    unsigned long raw;
};

#endif 