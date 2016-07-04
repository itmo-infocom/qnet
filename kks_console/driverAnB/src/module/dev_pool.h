#ifndef DEV_POOL_H
#define DEV_POOL_H

#include "AnBPrivateDefs.h"

#define NAME ANB_DEVICE_NAME "_00"

struct AnBDevice;

struct vm_area_struct;
struct miscdevice;
struct dma_struct;

struct semaphore;

#pragma pack(1)
struct file_entry {   
    struct miscdevice* device;       
    struct vm_area_struct* map_ref;
    struct dma_struct* dma;         //the structure that is describing DMA 

    struct semaphore* sem;

    unsigned int misc_registered;
    
    enum DumpSources dump_source;   //the source that provides data for read operations (the default value is BAR0)
    enum DestTables  dest_table;    //the destination that consumes data from write operations (the default value is TableRNG)
    
    char name[sizeof(NAME) + 1];    //the device name in the /dev file system
};
#pragma pack()

inline void deviceRemove(struct AnBDevice* device_ref);
inline int  deviceAdd(struct AnBDevice* device_ref);

inline struct AnBDevice* deviceGetMap(struct vm_area_struct* map_ref);
inline struct AnBDevice* deviceGet(struct miscdevice* misc_device_ref);

inline int getDeviceNumber(struct AnBDevice* device_ref);

#endif