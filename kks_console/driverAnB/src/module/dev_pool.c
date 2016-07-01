#include <linux/semaphore.h>

#include "pci_device.h"

#include "dev_pool.h"

DEFINE_SEMAPHORE(dev_pool_sem);

static struct AnBDevice** devices = NULL;
static unsigned int devices_count = 0;

inline int getDeviceNumber(struct AnBDevice* device_ref)
{
    int retval = -1;
    int i      =  0;

    if(devices == NULL)
        goto exit;

    for(i = 0; i < devices_count; i++)
    {
        if(devices[i] == device_ref)
        {
            retval = i;
            break;
        }
    }

exit:
    return retval;
}

inline void deviceRemove(struct AnBDevice* device_ref)
{
    int index = -1;

    struct AnBDevice** temp = NULL;

    down(&dev_pool_sem);

    index = getDeviceNumber(device_ref);

    if(index >= 0)
    {
        if(devices_count == 1)
        {
            kfree(devices);

            devices = NULL;
        }
        else
        {
            temp = kzalloc(sizeof(*temp) * (devices_count - 1), GFP_KERNEL);

            if(!temp)
                goto exit;
                
            if(index - 1 >= 0)
                memcpy(temp, &devices[index - 1], (index - 1) * sizeof(*devices));

            if(index + 1 < devices_count)
                memcpy(&temp[index], &devices[index + 1], (devices_count - index - 1) * sizeof(*devices));

            kfree(devices);
            devices = temp;
        }
        
        devices_count -= 1;

        printk(KERN_INFO ANB_DEVICE_PREFIX "Device has been removed from the pool\n");
    }

exit:
    up(&dev_pool_sem);
}

inline int deviceAdd(struct AnBDevice* device_ref)
{
    int retval = -1; 
    int index  = -1;

    struct AnBDevice** temp = NULL;

    down(&dev_pool_sem);

    index = getDeviceNumber(device_ref);

    if(index < 0)
    {
        printk(KERN_INFO ANB_DEVICE_PREFIX "Reallocating the pool...\n");

        temp = kzalloc(sizeof(*temp) * (devices_count + 1), GFP_KERNEL);

        if(temp == NULL)
        {
            retval = -ENOMEM;

            goto exit;
        }

        printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel memory for the pool at: 0x%016lx\n", (unsigned long)temp);
        printk(KERN_INFO ANB_DEVICE_PREFIX "                                         0x%016lx\n", (unsigned long)temp + sizeof(*temp) * (devices_count + 1));
        printk(KERN_INFO ANB_DEVICE_PREFIX "                            memory size: %ld bytes\n", sizeof(*temp) * (devices_count + 1));
        printk(KERN_INFO ANB_DEVICE_PREFIX "Devices are in the pool: %d\n", devices_count + 1);
        printk(KERN_INFO ANB_DEVICE_PREFIX "Last device added:       0x%016lx", (unsigned long)device_ref);

        if(devices)
        {
            memcpy(temp, devices, sizeof(*temp) * (devices_count));
            kfree(devices);
        }

        devices = temp;

        devices[devices_count++] = device_ref;

        retval = 0;
    }
    else 
        printk(KERN_INFO ANB_DEVICE_PREFIX "Device is already presented in the pool\n");

exit:
    up(&dev_pool_sem);

    return retval;
}

inline struct AnBDevice* deviceGetMap(struct vm_area_struct* map_ref)
{
    struct AnBDevice* retval = NULL;
    
    int i =  0;

    down(&dev_pool_sem);

    if(devices == NULL || map_ref == NULL)
        goto exit;

    for(i = 0; i < devices_count; i++)
    {
        if(devices[i]->file->map_ref == map_ref)
        {
            retval = devices[i];
            break;
        }
    }

exit:
    up(&dev_pool_sem);

    return retval;    
}

inline struct AnBDevice* deviceGet(struct miscdevice* misc_device_ref)
{
    struct AnBDevice* retval = NULL;
    
    int i =  0;

    down(&dev_pool_sem);

    if(devices == NULL || misc_device_ref == NULL)
        goto exit;

    for(i = 0; i < devices_count; i++)
    {
        if(devices[i]->file->device == misc_device_ref)
        {
            retval = devices[i];
            break;
        }
    }

exit:
    up(&dev_pool_sem);

    return retval;
}

