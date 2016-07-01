#ifndef PCI_DEVICE_H
#define PCI_DEVICE_H

#include <linux/pci.h>

#include "AnBPrivateDefs.h"

#define ANB_DEVICE_PREFIX ANB_DEVICE_NAME ": "

#define ANB_DEVICE_BARS 2

#define ANB_DEVICE_REGISTERS_BAR 0
#define ANB_DEVICE_MEMORY_BAR    1

struct file_entry;

struct AnBDevice {
    struct {
        void __iomem* virt_address;

        unsigned int offset;
        unsigned int length;
    } bars[ANB_DEVICE_BARS];
    
    struct pci_dev* device_ref;    

    struct file_entry* file;

    unsigned int device_id;
};

extern int  AllocateEntry(struct AnBDevice* private_data);
extern void DeallocateEntry(struct AnBDevice* private_data); 

inline void DevRegWrite(const struct AnBDevice* device, enum AnBRegs reg, union AnBReg value);
inline void DevRegRead(const struct AnBDevice* device, enum AnBRegs reg, union AnBReg* value);
inline void DevRegDump(const struct AnBDevice* device, void* buffer, unsigned int size, unsigned int offset);

inline void DevMemoryWrite(const struct AnBDevice* device, const void* buffer, unsigned int size, unsigned int offset);
inline void DevMemoryRead(const struct AnBDevice* device, void* buffer, unsigned int size, unsigned int offset);

int device_suspend(struct pci_dev *device, u32 state);
int device_resume(struct pci_dev *device);
int device_probe(struct pci_dev* device, const struct pci_device_id* device_id);

void device_remove(struct pci_dev* device);

#endif

