#ifndef ANBDEFS_H
#define ANBDEFS_H

#define ANB_REG_MAX 0xfc
#define ANB_REG_MIN 0x00

#define AnBTableMaxSize  4096
#define AnBBufferMinSize 16384 * 4   //Specified in the BUFFER_SIZE constant in the VHDL code (the BUFFER_SIZE constant actually specifies the number of 4-byte words)

#define AnBBuffers 16

enum AnBDevices {
    Bob,
    Alice,
    
    AnBDevice_Illegal
};

enum DumpSources {
    BAR1_RNG,
    BAR1_DAC,
    BAR1_CAL,

    BAR0,
    
    DumpSource_Illegal
};

enum DestTables {
    TableRNG,
    TableDAC,
    TableCAL,
    
    DestTable_Illegal
};

enum AnBTableModes {
    Mode1 = 0,
    Mode2 = 1,    
    Mode3 = 2,    
    Mode4 = 3,    
    Mode5 = 4,    
    Mode6 = 5, 
    
    AnBTableMode_Illegal
};

static const unsigned int AnBTableMemorySize[DestTable_Illegal] = 
{
    2048,
    4096 * 4,
    128  * 4
};

enum AnBRegs {
    RegPattern = 0x00,            //Address in register bank, which always contains "PCIe" in ASCII
    RegMode    = 0x08,          
    RegDMA     = 0x0C,
    RegAddr_l  = 0x10,
    RegAddr_h  = 0x14,
    RegSize    = 0x18,
    RegTable   = 0x24,
    RegIRQ     = 0x38,
};

union AnBReg {
#ifdef __cplusplus
public:
    AnBReg() : pattern({'0', '0', '0', '0'}) {}
#endif
    struct {
        const char pattern[4];
    };
    
    struct {
       unsigned int mode     : 1;  //enum AnBDevices
       unsigned int reserved : 23;
       unsigned int table    : 2;  //enum AnBModes
    } mode;
    
    struct {
       unsigned int enabled : 1;
    } dma;   
    
    unsigned int DMAAddressLow;
    unsigned int DMAAddressHigh;
    unsigned int DMASize;
    
    struct {
        unsigned int size : 16;     //from 0 to AnBTableMaxValue
        unsigned int mode : 4;      //enum AnBTableModes
    } table;
    
    struct {
        unsigned int frame       : 1;
        unsigned int dma         : 1;
        unsigned int calibration : 1;
            
    } irq_status;
      
    unsigned int raw;
};

struct AnBRegInfo {
    union AnBReg value;
    enum AnBRegs address;
};

#endif
