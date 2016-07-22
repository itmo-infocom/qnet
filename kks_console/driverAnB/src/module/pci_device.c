#include <linux/semaphore.h>

#include "pci_device.h"

#define ANB_DEVICE_LATENCY 0x20
#define ANB_DEVICE_COMMAND (PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER)

#define ANB_DEVICE_LIMIT   256

static unsigned int devices_count = 0;

DEFINE_SEMAPHORE(sem);

static void print_bar_info(struct pci_dev* device)
{
    unsigned int flags = 0;
    unsigned int i     = 0;
    
    for(i = 0; i < 6; i++)
    {
        flags = pci_resource_flags(device, i);
        
        if(!flags)
            printk(KERN_INFO ANB_DEVICE_PREFIX "Device BAR %d: not in use\n", i);        
        else
        {
            printk(KERN_INFO ANB_DEVICE_PREFIX "Device BAR %d: %10d bytes (0x%08x ~ 0x%08x) Type: %3s P  %c RO %c C  %c RL%c SH%c\n",                  
                             i, (unsigned int)pci_resource_len(device, i),  (unsigned int)pci_resource_start(device, i), (unsigned int)pci_resource_end(device, i),
                             ((flags & IORESOURCE_IO)  == IORESOURCE_IO)  ? "IO"  :
                             ((flags & IORESOURCE_MEM) == IORESOURCE_MEM) ? "MEM" : "---",
                                                                                       
                             ((flags & IORESOURCE_PREFETCH)      == IORESOURCE_PREFETCH)      ? '+' : '-',
                             ((flags & IORESOURCE_READONLY)      == IORESOURCE_READONLY)      ? '+' : '-',
                             ((flags & IORESOURCE_CACHEABLE)     == IORESOURCE_CACHEABLE)     ? '+' : '-',
                             ((flags & IORESOURCE_RANGELENGTH)   == IORESOURCE_RANGELENGTH)   ? '+' : '-',
                             ((flags & IORESOURCE_SHADOWABLE)    == IORESOURCE_SHADOWABLE)    ? '+' : '-');                             
            printk(KERN_INFO ANB_DEVICE_PREFIX "                                                                   ASz%c ASt%c M64%c W %c M %c\n",
                             ((flags & IORESOURCE_SIZEALIGN)     == IORESOURCE_SIZEALIGN)     ? '+' : '-',
                             ((flags & IORESOURCE_STARTALIGN)    == IORESOURCE_STARTALIGN)    ? '+' : '-',
                             ((flags & IORESOURCE_MEM_64)        == IORESOURCE_MEM_64)        ? '+' : '-',
                             ((flags & IORESOURCE_WINDOW)        == IORESOURCE_WINDOW)        ? '+' : '-', 
                             ((flags & IORESOURCE_MUXED)         == IORESOURCE_MUXED)         ? '+' : '-');                                                                                      
            printk(KERN_INFO ANB_DEVICE_PREFIX "                                                                   Ex %c Dis%c U  %c A %c B %c\n",
                             ((flags & IORESOURCE_EXCLUSIVE)     == IORESOURCE_EXCLUSIVE)     ? '+' : '-',
                             ((flags & IORESOURCE_DISABLED)      == IORESOURCE_DISABLED)      ? '+' : '-',
                             ((flags & IORESOURCE_UNSET)         == IORESOURCE_UNSET)         ? '+' : '-', 
                             ((flags & IORESOURCE_AUTO)          == IORESOURCE_AUTO)          ? '+' : '-', 
                             ((flags & IORESOURCE_BUSY)          == IORESOURCE_BUSY)          ? '+' : '-');            
        }
     }
}

int device_probe (struct pci_dev* device, const struct pci_device_id* device_id)
{    
    struct AnBDevice* private_data = NULL;
    
    unsigned int flags = 0;
    int retval         = 0;

    down(&sem);

    printk(KERN_INFO ANB_DEVICE_PREFIX "----------------------------------------probing---------------------------------------\n");
    
    if(devices_count >= ANB_DEVICE_LIMIT)
    {
        printk(KERN_INFO ANB_DEVICE_PREFIX "The limit of devices has been reached\n");        
    
        return -EINVAL;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Device number: %d\n", devices_count);
    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating a memory...\n");
    
    private_data = kzalloc(sizeof(*private_data), GFP_KERNEL);

    if (!private_data)
    {
        retval = -ENOMEM;
        
        goto probing_done;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel logical memory: 0x%016lx\n", (unsigned long)private_data);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                 0x%016lx\n", (unsigned long)private_data + sizeof(*private_data));
    printk(KERN_INFO ANB_DEVICE_PREFIX "                    memory size: %ld bytes\n", sizeof(*private_data));        
    
    printk(KERN_INFO ANB_DEVICE_PREFIX "Enabling a device...\n");
    
    retval = pci_enable_device(device);
    
    if (retval)
        goto probing_fb_free;
            
    printk(KERN_INFO ANB_DEVICE_PREFIX "Requesting memory regions...\n");
    
    retval = pci_request_regions(device, ANB_DEVICE_NAME);
    
    if (retval)
        goto probing_fb_disable;       
    
    printk(KERN_INFO ANB_DEVICE_PREFIX "Checking BARs configuration...\n");
    
    print_bar_info(device);
    
    flags                                               = pci_resource_flags(device, ANB_DEVICE_REGISTERS_BAR);
    private_data->bars[ANB_DEVICE_REGISTERS_BAR].offset = pci_resource_start(device, ANB_DEVICE_REGISTERS_BAR);
    private_data->bars[ANB_DEVICE_REGISTERS_BAR].length = pci_resource_len(device,   ANB_DEVICE_REGISTERS_BAR);

    if((flags & IORESOURCE_IO) != IORESOURCE_IO)    
    {
        printk(KERN_WARNING ANB_DEVICE_PREFIX "Unexpected BAR type (device registers)\n");
        
        retval = -EINVAL;
        
        goto probing_fb_regions;
    }
    
    flags                                            = pci_resource_flags(device, ANB_DEVICE_MEMORY_BAR);
    private_data->bars[ANB_DEVICE_MEMORY_BAR].offset = pci_resource_start(device, ANB_DEVICE_MEMORY_BAR);
    private_data->bars[ANB_DEVICE_MEMORY_BAR].length = pci_resource_len(device,   ANB_DEVICE_MEMORY_BAR);

    if((flags & IORESOURCE_MEM) != IORESOURCE_MEM)    
    {
        printk(KERN_WARNING ANB_DEVICE_PREFIX "Unexpected BAR type (device memory)\n");
        
        retval = -EINVAL;
        
        goto probing_fb_regions;
    }  
    
    printk(KERN_INFO ANB_DEVICE_PREFIX "Setting the device up...\n");        
        
    pci_write_config_dword(device, PCI_BASE_ADDRESS_0 + 4 * ANB_DEVICE_REGISTERS_BAR, private_data->bars[ANB_DEVICE_REGISTERS_BAR].offset);
    pci_write_config_dword(device, PCI_BASE_ADDRESS_0 + 4 * ANB_DEVICE_MEMORY_BAR,    private_data->bars[ANB_DEVICE_MEMORY_BAR].offset);
    
    pci_write_config_word(device, PCI_COMMAND,       ANB_DEVICE_COMMAND);
    pci_write_config_byte(device, PCI_LATENCY_TIMER, ANB_DEVICE_LATENCY);
    
    printk(KERN_INFO ANB_DEVICE_PREFIX "Mapping device registers into the memory...\n");
    
    private_data->bars[ANB_DEVICE_REGISTERS_BAR].virt_address = pci_iomap(device, ANB_DEVICE_REGISTERS_BAR, private_data->bars[ANB_DEVICE_REGISTERS_BAR].length);
                                               
    if(!private_data->bars[ANB_DEVICE_REGISTERS_BAR].virt_address)
    {
        printk(KERN_WARNING ANB_DEVICE_PREFIX "Can't map BAR into the memory (device registers)\n");

        retval = -ENOMEM;
        
        goto probing_fb_regions;
    }                  
   
    private_data->bars[ANB_DEVICE_MEMORY_BAR].virt_address = pci_iomap(device, ANB_DEVICE_MEMORY_BAR, private_data->bars[ANB_DEVICE_MEMORY_BAR].length);
                                               
    if(!private_data->bars[ANB_DEVICE_MEMORY_BAR].virt_address)
    {
        printk(KERN_WARNING ANB_DEVICE_PREFIX "Can't map BAR into the memory (device memory)\n");

        retval = -ENOMEM;
        
        goto probing_fb_io_map;
    }                  
    
    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating entry in /dev...\n");    
    
    pci_set_drvdata(device, private_data);

    private_data->device_ref = device;
    private_data->device_id  = devices_count;
     
    retval = AllocateEntry(private_data);
    
    if(retval)
        goto probing_fb_mem_map;
    
    devices_count += 1;

    goto probing_done;

probing_fb_mem_map:
    pci_iounmap(device, private_data->bars[ANB_DEVICE_MEMORY_BAR].virt_address);
    private_data->bars[ANB_DEVICE_MEMORY_BAR].virt_address = NULL;

probing_fb_io_map:
    pci_iounmap(device, private_data->bars[ANB_DEVICE_REGISTERS_BAR].virt_address);
    private_data->bars[ANB_DEVICE_REGISTERS_BAR].virt_address = NULL;

probing_fb_regions:
    pci_release_regions(device);

probing_fb_disable:
    pci_disable_device(device);
    
probing_fb_free:    
    kfree(private_data);
    private_data = NULL;
    
probing_done:    
    printk(KERN_INFO ANB_DEVICE_PREFIX "Probing retval: %d\n", retval); 
    
    up(&sem);

    return retval;    
}

void device_remove (struct pci_dev* device)
{
    struct AnBDevice* private_data = NULL;

    down(&sem);

    private_data = pci_get_drvdata(device);
    
    if(private_data)
    {
        DeallocateEntry(private_data);

        if(private_data->bars[ANB_DEVICE_REGISTERS_BAR].virt_address)
        {
            pci_iounmap(device, private_data->bars[ANB_DEVICE_REGISTERS_BAR].virt_address);
            private_data->bars[ANB_DEVICE_REGISTERS_BAR].virt_address = NULL;
        }
            
        if(private_data->bars[ANB_DEVICE_MEMORY_BAR].virt_address)       
        {
            pci_iounmap(device, private_data->bars[ANB_DEVICE_MEMORY_BAR].virt_address);
            private_data->bars[ANB_DEVICE_MEMORY_BAR].virt_address = NULL;
        }
     
        pci_release_regions(device);
        
        pci_disable_device(device);
        
        kfree(private_data);

        devices_count--;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "---------------------------------------removing---------------------------------------\n");

    up(&sem);
}

int device_suspend(struct pci_dev *device, u32 state)
{
    return 0;
}

int device_resume (struct pci_dev *device)
{
    return 0;
}
