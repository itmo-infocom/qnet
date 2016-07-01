#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <asm/uaccess.h>

#include "pci_device.h"

#include "dev_entry.h"
#include "dev_pool.h"

#include "dma.h"

static int          open(struct inode* inode, struct file* filp);
static ssize_t      read(struct file* filp, char __user* buffer, size_t size, loff_t* offset);
static ssize_t     write(struct file* filp, const char __user* buffer, size_t size, loff_t* offset);
static long        ioctl(struct file* filp, enum AnBCommands cmd, union AnBCommandArg arg);
static int          mmap(struct file* filp, struct vm_area_struct* vm_user);
static unsigned int poll(struct file *filp, poll_table* wait);

static void map_close(struct vm_area_struct *vma);

static struct file_operations dev_fops =
{
    .owner          = THIS_MODULE,
    .open           = open,
    .mmap           = mmap,
    .read           = read,
    .write          = write,
    .unlocked_ioctl = (long (*) (struct file *, unsigned int, unsigned long))ioctl,
    .poll           = poll
};

static struct vm_operations_struct dev_map_ops = {
    .close = map_close,
};

int AllocateEntry(struct AnBDevice* private_data)
{
    int retval = 0;

    unsigned char c0, c1;

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating entry in the device struct located at: 0x%016lx\n", (unsigned long)private_data);
    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating a memory region for structure file_entry...\n");

    private_data->file = kzalloc(sizeof(struct file_entry), GFP_KERNEL);

    if(private_data->file == NULL)
    {
        retval = -ENOMEM;
        goto exit;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel logical memory: 0x%016lx\n", (unsigned long)private_data->file);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                 0x%016lx\n", (unsigned long)private_data->file + sizeof(struct file_entry));
    printk(KERN_INFO ANB_DEVICE_PREFIX "                    memory size: %ld bytes\n", sizeof(struct file_entry));

    memcpy(private_data->file->name, NAME, sizeof(NAME));
    
    c0 = (private_data->device_id & 0x00f) >> 0;
    c1 = (private_data->device_id & 0x0f0) >> 4;
    
    private_data->file->name[sizeof(NAME) - 2] = (c0 >= 0 && c0 <= 9) ? c0 + 0x30 : c0 + 0x51; 
    private_data->file->name[sizeof(NAME) - 3] = (c1 >= 0 && c1 <= 9) ? c1 + 0x30 : c1 + 0x51;

    printk(KERN_INFO ANB_DEVICE_PREFIX "Device name: %.*s\n", (int)sizeof(NAME), private_data->file->name);

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating entry in the struct located at: 0x%016lx\n", (unsigned long)private_data->file);
    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating a memory region for structure miscdevice...\n");

    private_data->file->device = kzalloc(sizeof(struct miscdevice), GFP_KERNEL);

    if(private_data->file->device == NULL)
    {
        retval = -ENOMEM;
        goto fall_back_dealloc_file;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel logical memory: 0x%016lx\n", (unsigned long)private_data->file->device);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                 0x%016lx\n", (unsigned long)private_data->file->device + sizeof(struct miscdevice));
    printk(KERN_INFO ANB_DEVICE_PREFIX "                    memory size: %ld bytes\n", sizeof(struct miscdevice));

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating a memory region for structure semaphore...\n");

    private_data->file->sem = kzalloc(sizeof(struct semaphore), GFP_KERNEL);

    if(private_data->file->sem == NULL)
    {
        retval = -ENOMEM;
        goto fall_back_dealloc_device;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel logical memory: 0x%016lx\n", (unsigned long)private_data->file->sem);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                 0x%016lx\n", (unsigned long)private_data->file->sem + sizeof(struct semaphore));
    printk(KERN_INFO ANB_DEVICE_PREFIX "                    memory size: %ld bytes\n", sizeof(struct semaphore));

    sema_init(private_data->file->sem, 1);


    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating a memory region for structure dma_struct...\n");

    private_data->file->dma = kzalloc(sizeof(struct dma_struct), GFP_KERNEL);

    if(private_data->file->dma == NULL)
    {
        retval = -ENOMEM;
        goto fall_back_dealloc_sem;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocated kernel logical memory: 0x%016lx\n", (unsigned long)private_data->file->dma);
    printk(KERN_INFO ANB_DEVICE_PREFIX "                                 0x%016lx\n", (unsigned long)private_data->file->dma + sizeof(struct dma_struct));
    printk(KERN_INFO ANB_DEVICE_PREFIX "                    memory size: %ld bytes\n", sizeof(struct dma_struct));

    printk(KERN_INFO ANB_DEVICE_PREFIX "Registering the DMA service...\n");

    retval = register_dma(private_data);

    if(private_data->file->dma == NULL)
    {
        retval = -ENOMEM;
        goto fall_back_dealloc_dma;
    }

    printk(KERN_INFO ANB_DEVICE_PREFIX "Adding device to the pool...\n");

    retval = deviceAdd(private_data);

    if(retval)
        goto fall_back_unregister_dma;

    private_data->file->device->minor = MISC_DYNAMIC_MINOR;
    private_data->file->device->name  = private_data->file->name;
    private_data->file->device->fops  = &dev_fops;
    private_data->file->device->mode  = 0666;

    printk(KERN_INFO ANB_DEVICE_PREFIX "Registering a misc device...\n");

    retval = misc_register(private_data->file->device);

    if(retval)
        goto fall_back_remove_pool;

    private_data->file->misc_registered = 1;

    goto exit;

fall_back_remove_pool:
    deviceRemove(private_data);    

fall_back_unregister_dma:
    unregister_dma(private_data);

fall_back_dealloc_dma:
    kfree(private_data->file->dma);
    private_data->file->dma = NULL;

fall_back_dealloc_sem:
    kfree(private_data->file->sem);
    private_data->file->sem = NULL;

fall_back_dealloc_device:
    kfree(private_data->file->device);
    private_data->file->device = NULL;    

fall_back_dealloc_file:
    kfree(private_data->file);
    private_data->file = NULL;    

exit:
    printk(KERN_INFO ANB_DEVICE_PREFIX "Allocating retval: %d\n", retval); 

    return retval;
}

void DeallocateEntry(struct AnBDevice* private_data)
{
    if(private_data->file)
    {
        if(private_data->file->misc_registered)
            misc_deregister(private_data->file->device); 

        deviceRemove(private_data);

        unregister_dma(private_data);

        if(private_data->file->dma)
        {
            kfree(private_data->file->dma);
            private_data->file->dma = NULL;
        }

        if(private_data->file->sem)
        {    
            kfree(private_data->file->sem);
            private_data->file->sem = NULL;
        }

        if(private_data->file->device)
        {
            kfree(private_data->file->device);
            private_data->file->device = NULL; 
        }

        kfree(private_data->file);
        private_data->file = NULL;    
    }
}

int open(struct inode *inode, struct file *filp)
{
    struct AnBDevice* private_data = deviceGet(filp->private_data);

    printk(KERN_INFO ANB_DEVICE_PREFIX "Private data: 0x%016lx\n", (unsigned long)filp->private_data);

    if(!private_data)
        return -ENODEV;
        
    filp->private_data = private_data;  
    
    return 0;
}

ssize_t read(struct file* filp, char __user* buffer, size_t size, loff_t* offset)
{
	union AnBReg reg_old;
    union AnBReg reg;

    unsigned int restore = 0;

    ssize_t retval = 0;
       
    void (*source_method)(const struct AnBDevice* , void*, unsigned int, unsigned int) = NULL;
    
    char*        kern_buffer = NULL;
    unsigned int source_size = 0;
    
    struct AnBDevice* info = filp->private_data;
    
    if(!info)
        return -ENODEV;
        
    if(!info->file)
        return -ENODEV;    
    
    if(down_interruptible(info->file->sem))
         return -ERESTARTSYS;

    if(info->file->dma->dma_started)
    {
        retval = -EBUSY;
            
        goto done;
    }

    switch(info->file->dump_source)        
    {
    case BAR0:
        source_size   = info->bars[ANB_DEVICE_REGISTERS_BAR].length;
        source_method = DevRegDump;
        break;
    case BAR1_RNG:
    case BAR1_DAC:
    case BAR1_CAL:
        restore = 1;
        
        source_size   = AnBTableMemorySize[info->file->dump_source];
        source_method = DevMemoryRead;
        
        DevRegRead(info, RegMode, &reg_old);
        reg.raw = reg_old.raw;
        reg.mode.table = info->file->dump_source;
        DevRegWrite(info, RegMode, reg);
        break;
    default:
        retval = -EINVAL;
        goto done;

        break;
    }
   
    if(*offset > source_size)
        goto done;
    else if (*offset + size > source_size)
        size = source_size - *offset;
        
    if(!size)
        goto done;
        
    if(!source_method)
    {
        retval = -ENODEV;
        goto done;
    }
    
    up(info->file->sem);

    kern_buffer = kmalloc(size, GFP_KERNEL);

    if(down_interruptible(info->file->sem))
         return -ERESTARTSYS;
    
    if(!kern_buffer)
    {
        retval = -ENOMEM;
        goto done;
    }
        
    source_method(info, kern_buffer, size, *offset);        
    
    retval = copy_to_user(buffer, kern_buffer, size);
        
    kfree(kern_buffer);                            
    
    if(retval)
        goto done;
        
    *offset = size;     
    retval = size;
    
done:    
    if(restore)
        DevRegWrite(info, RegMode, reg_old);   
        
    up(info->file->sem);

    return retval;
}

ssize_t write(struct file* filp, const char __user* buffer, size_t size, loff_t* offset)
{
    union AnBReg reg_old;
    union AnBReg reg;

    ssize_t retval = 0;
       
    char*        kern_buffer = NULL;
    unsigned int source_size = 0;
    
    struct AnBDevice* info = filp->private_data;
    
    if(!info)
        return -ENODEV;
        
    if(!info->file)
        return -ENODEV;    
    
    if(down_interruptible(info->file->sem))
         return -ERESTARTSYS;
    
    if(info->file->dma->dma_started)
    {
        retval = -EBUSY;
            
        goto done;
    }
    
    source_size = AnBTableMemorySize[info->file->dest_table];

    DevRegRead(info, RegMode, &reg_old);
    
    reg.raw = reg_old.raw;
    reg.mode.table = info->file->dest_table;

    DevRegWrite(info, RegMode, reg);
    
    if(*offset > source_size)
        goto done;
    else if (*offset + size > source_size)
        size = source_size - *offset;
        
    if(!size)
        goto done;
        
    up(info->file->sem);

    kern_buffer = kmalloc(size, GFP_KERNEL);

    if(down_interruptible(info->file->sem))
         return -ERESTARTSYS;
    
    if(!kern_buffer)
    {
        retval = -ENOMEM;
        goto done;
    }
        
    retval = copy_from_user(kern_buffer, buffer, size);
    
    if(retval)
        goto done;
    
    DevMemoryWrite(info, kern_buffer, size, *offset);        
    
    *offset = size;     
    retval = size;
    
done:    
    DevRegWrite(info, RegMode, reg_old);   

    kfree(kern_buffer);                            
        
    up(info->file->sem);

    return retval;
}

long ioctl(struct file *filp, enum AnBCommands cmd, union AnBCommandArg arg)
{
    int retval = 0;
    
    struct AnBDevice* info = filp->private_data;

    struct AnBRegInfo reg_info;
    
    if(down_interruptible(info->file->sem))
     	return -ERESTARTSYS;
    
    switch(cmd)
    {
    case Version:
        if(arg.version != ANB_VERSION)
            retval = -EINVAL;
        break;
    case RegisterWrite:
        if(info->file->dma->dma_started)
            retval = -EBUSY;
        else if(!access_ok(VERIFY_READ, (void __user *)arg.reg_info, sizeof(struct AnBRegInfo)))
            retval = -EFAULT;
        else if(copy_from_user(&reg_info, arg.reg_info, sizeof(struct AnBRegInfo)))
            retval =  -EFAULT;
        else if(reg_info.address >= ANB_REG_MIN && reg_info.address <= ANB_REG_MAX && reg_info.address % 4 == 0)
            DevRegWrite(info, reg_info.address, reg_info.value);
        else
        	retval = -EINVAL;
        break;
    case RegisterRead:    
        if(info->file->dma->dma_started)
            retval = -EBUSY;
        else if(!access_ok(VERIFY_WRITE, (void __user *)arg.reg_info, sizeof(struct AnBRegInfo)))
            retval = -EFAULT;
        else if(!access_ok(VERIFY_READ, (void __user *)arg.reg_info, sizeof(struct AnBRegInfo)))
        	retval = -EFAULT;
        else if(copy_from_user(&reg_info, arg.reg_info, sizeof(struct AnBRegInfo)))
        	retval = -EFAULT;
        else if(reg_info.address >= ANB_REG_MIN && reg_info.address <= ANB_REG_MAX && reg_info.address % 4 == 0)
        {
            DevRegRead(info, reg_info.address, &reg_info.value);
        
            if(copy_to_user(arg.reg_info, &reg_info, sizeof(struct AnBRegInfo)))
                retval = -EFAULT;
        }
        else 
        	retval = -EINVAL;
        break;
    case SetDeviceMode:
        if(info->file->dma->dma_started)
            retval = -EBUSY;
        else if(arg.device_type >= AnBDevice_Illegal)
            retval = -EINVAL;
        else
        {
            reg_info.address = RegMode;

            DevRegRead(info, reg_info.address, &reg_info.value);  

            reg_info.value.mode.mode = arg.device_type;

            DevRegWrite(info, reg_info.address, reg_info.value);  
        }

        break;        
    case SetDumpSource:
        if(arg.dump_source >= DumpSource_Illegal)
        	retval = -EINVAL;
        else                        
            info->file->dump_source = arg.dump_source;
        break;
    case SetDestTable:
        if(arg.dest_table >= DestTable_Illegal)
        	retval = -EINVAL;
        else
            info->file->dest_table = arg.dest_table;    
        break;
    case DMAEnable:
        retval = enable_dma(info);
        break;
    case DMADisable:
        printk(KERN_INFO ANB_DEVICE_PREFIX "Disabling dma\n");

        retval = disable_dma(info);
        break;
    case DMASetBufferSize:
        if(info->file->map_ref != NULL)
            retval = -EBUSY;
        
        retval = change_buffer(info, arg.buffer_size);
    	break;
    case DMARollBuffer:
        while (info->file->dma->active_read == info->file->dma->active_write) 
        {
            up(info->file->sem);

            if (filp->f_flags & O_NONBLOCK)
                return -EAGAIN;
            
            if (wait_event_interruptible(info->file->dma->read_q, (info->file->dma->active_read != info->file->dma->active_write)))
                return -ERESTARTSYS; 

            if (down_interruptible(info->file->sem))
                return -ERESTARTSYS;
        }    
        
        if(info->file->dma->active_read == info->file->dma->buffers - 1)
            info->file->dma->active_read = 0;
        else
            info->file->dma->active_read += 1;     

        if(info->file->dma->suspended)
            info->file->dma->suspended = 0;

        break;
    case TableSetSize:
        if(info->file->dma->dma_started)
            retval = -EBUSY;
        else if(arg.table_size > AnBTableMaxSize)
            retval = -EINVAL;
        else
        {
            reg_info.address = RegTable;

            DevRegRead(info, reg_info.address, &reg_info.value);  

            reg_info.value.table.size = arg.table_size;

            DevRegWrite(info, reg_info.address, reg_info.value);  
        }
        break;
    case TableSetMode:
        if(info->file->dma->dma_started)
            retval = -EBUSY;
        else if(arg.table_mode >= AnBTableMode_Illegal)
            retval = -EINVAL;
        else
        {
            reg_info.address = RegTable;

            DevRegRead(info, reg_info.address, &reg_info.value);  

            reg_info.value.table.mode = arg.table_mode;

            DevRegWrite(info, reg_info.address, reg_info.value);  
        }
        break;
    default: 
        retval = -ENOTTY;
    }

    up(info->file->sem);
    
    return retval;
}

int mmap(struct file* filp, struct vm_area_struct* vm_user)
{   
	int retval = 0;

    struct AnBDevice* info = filp->private_data;
    struct page*      page = NULL;
    
    unsigned long pfn = 0;
    
    if(down_interruptible(info->file->sem))
    	return -ERESTARTSYS;

    if(info->file->dma->dma_started)
    {
        retval = -EBUSY;
            
        goto exit;
    }
    
    if(!virt_addr_valid(info->file->dma->address_kernel))
    {
        retval = -EIO;
        
        goto exit;
    }
        
    page = virt_to_page(info->file->dma->address_kernel);
    
    if(!page)
    {
        retval = -EIO;
        
        goto exit;
    }
        
    pfn = page_to_pfn(page);

    vm_user->vm_ops = &dev_map_ops;

    if (remap_pfn_range(vm_user,
                        vm_user->vm_start,
                        pfn,
                        PAGE_ALIGN(vm_user->vm_end - vm_user->vm_start),
                        vm_user->vm_page_prot)) 
    {
        retval = -EAGAIN;
        
        goto exit;
    }
       
    vm_user->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP; 
    
    info->file->map_ref = vm_user;

    printk(KERN_INFO ANB_DEVICE_PREFIX "Memory maped: 0x%016lx\n", (unsigned long)vm_user);

exit:    
    up(info->file->sem);

    return retval; 
}

unsigned int poll(struct file *filp, poll_table* wait)
{
    struct AnBDevice* info = filp->private_data;
	
    unsigned int mask = 0;

	down(info->file->sem);

	poll_wait(filp, &info->file->dma->read_q, wait);
	
	if(info->file->dma->active_read != info->file->dma->active_write)
		mask |= POLLIN | POLLRDNORM;	
	
	up(info->file->sem);
	
	return mask;
}

void map_close(struct vm_area_struct *vma)
{
    struct AnBDevice* info = deviceGetMap(vma);

    if(info != NULL)
    {
        info->file->map_ref = NULL;
        
        printk(KERN_INFO ANB_DEVICE_PREFIX "Memory closed: 0x%016lx\n", (unsigned long)vma);    
    }
}