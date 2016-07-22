#include <linux/interrupt.h>
#include <linux/sched.h>

#include "pci_device.h"
#include "dev_pool.h"
#include "dma.h"

static int map_active(struct AnBDevice* device, int active);

irqreturn_t DeviceHandler(int irq, struct AnBDevice* device)
{
    union AnBReg reg_status;
    union AnBReg reg_dma;

    int device_num;
        
    if((device_num = getDeviceNumber(device)) < 0)
        return IRQ_HANDLED;

    DevRegRead(device, RegIRQ, &reg_status);
    DevRegRead(device, RegDMA, &reg_dma);
    
    if(reg_status.irq_status.dma && device->file->dma->dma_started)
    {
        device->file->dma->handled += 1;

        if(!device->file->dma->dma_suspended)
        {
            reg_status.irq_status.dma = 1;
            reg_dma.dma.enabled = 0;
                    
            if(device->file->dma->active_write == device->file->dma->buffers - 1)
                device->file->dma->active_write = 0;
            else
                device->file->dma->active_write += 1;
                    
            if(device->file->dma->active_write == device->file->dma->active_read)
                device->file->dma->dma_suspended = 1;
            else
            {    
                map_active(device, device->file->dma->active_write);   
            
                DevRegWrite(device, RegIRQ, reg_status);
                DevRegWrite(device, RegDMA, reg_dma);        
                
                reg_dma.dma.enabled = 1;

                DevRegWrite(device, RegDMA, reg_dma);    
                
                wake_up_interruptible(&device->file->dma->read_q);
            }
        }
        else
            device->file->dma->suspended += 1;
    }

    return IRQ_HANDLED;
}

static void unset_buffer(struct AnBDevice* device)
{
    if(device->file->dma->address_kernel)
    {
        kfree(device->file->dma->address_kernel);
    
        device->file->dma->address_kernel = NULL;
        device->file->dma->size = 0;
    }

    if(device->file->dma->address_dma)
    {
        kfree(device->file->dma->address_dma);
    
        device->file->dma->address_dma = NULL;
        device->file->dma->buffers = 0;
    }
}

static void unmap_all(struct AnBDevice* device)
{
    int i = 0;
    
    for(i = 0; i < device->file->dma->buffers; i++)
    {
        if(device->file->dma->address_dma[i])
        {
            pci_unmap_single(device->device_ref, device->file->dma->address_dma[i], device->file->dma->size, PCI_DMA_FROMDEVICE);
            device->file->dma->address_dma[i] = 0;
        }    
    }
}

int map_active(struct AnBDevice* device, int active)
{
    union AnBReg reg;

    if(active >= 0 && active < device->file->dma->buffers)
    {
        unmap_all(device);
            
        device->file->dma->active_write = active;
        device->file->dma->address_dma[active] = pci_map_single(device->device_ref, ((char*)device->file->dma->address_kernel) + device->file->dma->size * active, device->file->dma->size, PCI_DMA_FROMDEVICE);
        
        if(!device->file->dma->address_dma[active])
            return -ENOMEM;
                        
        pci_set_dma_mask(device->device_ref, device->file->dma->address_dma[active]);
      
        reg.DMAAddressLow = device->file->dma->address_dma[active] & 0x0ffffffff;
        
        DevRegWrite(device, RegAddr_l, reg);
        
        reg.DMAAddressHigh = (device->file->dma->address_dma[active] & 0xffffffff00000000) >> 32;
        
        DevRegWrite(device, RegAddr_h, reg);
        
        reg.DMASize = device->file->dma->size;
        
        DevRegWrite(device, RegSize, reg);
    }
    else
        return -EINVAL;    

    return 0;
}

static int set_buffer(struct AnBDevice* device, unsigned int buffer_size, unsigned int buffer_num)
{
    int retval = 0;

    void* kernel_temp = NULL;
    void* dma_temp    = NULL; 
    
    if(device->file->dma->dma_started)
    {
    	retval = -EBUSY;
    	
    	goto done;
    }

    if(buffer_size == 0 || buffer_num < 2)
    {
        retval = -EINVAL;

        goto done;
    }
    
    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating DMA buffers...\n");

    kernel_temp = kzalloc(buffer_size * buffer_num, GFP_KERNEL | GFP_DMA);
    
    if(!kernel_temp)
    {
        retval = -ENOMEM;
        
        goto done;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel memory for the DMA buffer at: 0x%016lx\n", (unsigned long)kernel_temp);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                               0x%016lx\n", (unsigned long)kernel_temp + buffer_size * buffer_num);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                  memory size: %d bytes\n", buffer_size * buffer_num);
            
    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating DMA addresses array...\n");

    dma_temp = kzalloc(sizeof(*device->file->dma->address_dma) * buffer_num, GFP_KERNEL);

    if(!dma_temp)
    {
        retval = -ENOMEM;
        
        goto fail_fb_buffer_kernel;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel memory for the DMA addresses array at: 0x%016lx\n", (unsigned long)dma_temp);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                                        0x%016lx\n", (unsigned long)dma_temp + sizeof(*device->file->dma->address_dma) * buffer_num);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                           memory size: %ld bytes\n", sizeof(*device->file->dma->address_dma) * buffer_num);
             
    printk(KERN_INFO ANB_DEVICE_PREFIX "Setting a DMA region up...\n");

    unset_buffer(device);

    device->file->dma->address_kernel = kernel_temp;
    device->file->dma->address_dma    = dma_temp;
    device->file->dma->size           = buffer_size;
    device->file->dma->buffers        = buffer_num;

    retval = map_active(device, 0);

    if(retval)
        goto fail_fb_buffer;

    goto done;
        
fail_fb_buffer:
    unset_buffer(device);
    goto done;

fail_fb_buffer_kernel:
    kfree(kernel_temp);
        
done:
    return retval;
}

int enable_dma(struct AnBDevice* device)
{
    union AnBReg reg_info;
    union AnBReg reg_status;

    int retval = 0;
        
    if(device->file->dma->dma_started)
    {
    	retval = -EINVAL;
    	goto exit;
    }

    device->file->dma->active_read = 0;

    DevRegRead(device, RegDMA, &reg_info);
    DevRegRead(device, RegIRQ, &reg_status);
        
    reg_info.dma.enabled      = 0;  
    reg_status.irq_status.dma = 1;           

    DevRegWrite(device, RegIRQ, reg_status);
    DevRegWrite(device, RegDMA, reg_info);  

    map_active(device, 0); 

    reg_info.dma.enabled = 1;         
        
    DevRegWrite(device, RegDMA, reg_info);
    
    device->file->dma->dma_started = 1;
              
exit: 
    return retval;
}

int disable_dma(struct AnBDevice* device)
{
    union AnBReg reg_info;

    int retval = 0;
    
    if(!device->file->dma->dma_started)
    {
    	retval = -EINVAL;
    	goto exit;
    }

    DevRegRead(device, RegIRQ, &reg_info);
    DevRegRead(device, RegDMA, &reg_info);

    reg_info.dma.enabled = 0;         
        
    DevRegWrite(device, RegDMA, reg_info);
    
    device->file->dma->dma_started = 0;
 
exit:                 
    return retval;
}

int register_dma(struct AnBDevice* device)
{
    int retval = 0;

    union AnBReg reg_info;

    device->file->dma->irq = -1;
                
    retval = set_buffer(device, AnBBufferMinSize, AnBBuffers);
    
    if(retval)
        goto done;

    printk(KERN_INFO ANB_DEVICE_PREFIX "Registering an interrupt handler...\n");

    device->file->dma->irq = device->device_ref->irq;

    printk(KERN_INFO ANB_DEVICE_PREFIX "IRQ: %d\n", device->file->dma->irq);
    
    retval = request_irq(device->file->dma->irq, (irq_handler_t)DeviceHandler, IRQF_SHARED, ANB_DEVICE_NAME, device);
    
    if(retval)
    {
        device->file->dma->irq = -1;

        goto fall_back_buffer;
    }

    init_waitqueue_head(&device->file->dma->read_q);
    init_waitqueue_head(&device->file->dma->outq);    

    DevRegRead(device, RegDMA, &reg_info);

    reg_info.dma.enabled = 0;         
        
    DevRegWrite(device, RegDMA, reg_info);

    goto done;

fall_back_buffer:
    unmap_all(device);
    unset_buffer(device);
    
done:
	printk(KERN_INFO ANB_DEVICE_PREFIX "DMA retval: %d\n", retval); 

    return retval;
}

void unregister_dma(struct AnBDevice* device)
{
    disable_dma(device);

    if(device->file->dma->irq >= 0)
    {
        free_irq(device->file->dma->irq, device);
       
        device->file->dma->irq = -1;
    }
    
    unmap_all(device);
    unset_buffer(device);
                
    printk(KERN_INFO ANB_DEVICE_PREFIX "Interrupts were handled: %d\n", device->file->dma->handled);
    printk(KERN_INFO ANB_DEVICE_PREFIX "Interrupts were ignored: %d\n", device->file->dma->suspended);
    
    device->file->dma->handled = 0;
}

int change_buffer(struct AnBDevice* device, unsigned int size)
{
    return set_buffer(device, AnBBufferMinSize, size);
}
