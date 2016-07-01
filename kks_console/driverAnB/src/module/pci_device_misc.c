#include "pci_device.h"

inline void mem_read(unsigned int* address, unsigned int* buffer, unsigned int size, unsigned int offset)
{
    unsigned int i    = 0;
    unsigned int temp = 0;

    unsigned int size_q   = size / 4;
    unsigned int size_r   = size % 4;
    unsigned int offset_q = offset / 4;
    unsigned int offset_r = offset % 4;
      
    if(offset_r != 0)
    {
        temp = ioread32(address);
        
        memcpy(buffer, ((char*)&temp) + offset_r, 4 - offset_r);
                
        offset_q += 1;
    }
    
    address = (void*)(((char*)address) + offset);
    buffer  = (void*)(((char*)buffer)  + offset_r);
    
    for(i = 0; i < size_q; i ++)
        buffer[i] = ioread32(address + i); 
    
    if(size_r != 0)
    {
        temp = ioread32(address + offset + size_q);
        
        memcpy(&buffer[size_q], (char*)&temp, size_r);
    }
}

inline void mem_write(unsigned int* address, const unsigned int* buffer, unsigned int size, unsigned int offset)
{
    unsigned int i    = 0;
    unsigned int temp = 0;

    unsigned int size_q   = size / 4;
    unsigned int size_r   = size % 4;
    unsigned int offset_q = offset / 4;
    unsigned int offset_r = offset % 4;
      
    if(offset_r != 0)
    {
        temp = ioread32(address);
        
        memcpy(((char*)&temp) + offset_r, buffer, 4 - offset_r);

        iowrite32(temp, address);
                
        offset_q += 1;
    }
    
    address = (void*)(((char*)address) + offset);
    buffer  = (void*)(((char*)buffer)  + offset_r);
    
    for(i = 0; i < size_q; i ++)
        iowrite32(buffer[i], address + i); 
    
    if(size_r != 0)
    {
        temp = ioread32(address + offset + size_q);
        
        memcpy((char*)&temp, &buffer[size_q], size_r);

        iowrite32(temp, address + offset + size_q);
    }
}

inline void DevRegWrite(const struct AnBDevice* device, enum AnBRegs reg, union AnBReg value)
{        
    iowrite32(value.raw, ((char*)device->bars[ANB_DEVICE_REGISTERS_BAR].virt_address) + reg);
}

inline void DevRegRead(const struct AnBDevice* device, enum AnBRegs reg, union AnBReg* value)
{
    if(value)
        value->raw = ioread32(((char*)device->bars[ANB_DEVICE_REGISTERS_BAR].virt_address) + reg);    
}

inline void DevRegDump(const struct AnBDevice* device, void* buffer, unsigned int size, unsigned int offset)
{
    mem_read(device->bars[ANB_DEVICE_REGISTERS_BAR].virt_address, buffer, size, offset);
}

inline void DevMemoryWrite(const struct AnBDevice* device, const void* buffer, unsigned int size, unsigned int offset)
{
    mem_write(device->bars[ANB_DEVICE_MEMORY_BAR].virt_address, buffer, size, offset);
}

inline void DevMemoryRead(const struct AnBDevice* device, void* buffer, unsigned int size, unsigned int offset)
{
    mem_read(device->bars[ANB_DEVICE_MEMORY_BAR].virt_address, buffer, size, offset);    
}