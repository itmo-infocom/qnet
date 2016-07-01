#ifndef DEV_ENTRY_H
#define DEV_ENTRY_H

#include <linux/fs.h>
#include <linux/poll.h>

struct AnBDevice;

int  AllocateEntry(struct AnBDevice* private_data);
void DeallocateEntry(struct AnBDevice* private_data); 

#endif
