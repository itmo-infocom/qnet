#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#include "pci_device.h"

static const struct pci_device_id device_id[] = 
{
    {
        .class       = 0x040000,
        .class_mask  = 0x040000,
        .vendor      = 0x1172,
        .device      = 0x5201,
        .subvendor   = 0x1556,
        .subdevice   = 0x5555,
        .driver_data = 0 /* Arbitrary data, hence zero */
    },
    { }
};

static struct pci_driver anb_driver = 
{
    .name     = ANB_DEVICE_NAME,
    .id_table = device_id,
    .probe    = device_probe,
    .remove   = device_remove,
};

static int __init pcie_init(void)
{
    printk(KERN_INFO ANB_DEVICE_PREFIX "-----------------------------------------init-----------------------------------------\n");
	
	return pci_register_driver(&anb_driver);
}

static void __exit pcie_exit(void)
{
    pci_unregister_driver(&anb_driver);

    printk(KERN_INFO ANB_DEVICE_PREFIX "-----------------------------------------exit-----------------------------------------\n");
}

module_init(pcie_init);
module_exit(pcie_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Baevskih <aleks.bae@gmail.com>");
MODULE_DESCRIPTION("A PCI-e driver for a specialized application");

MODULE_DEVICE_TABLE(pci, device_id);
