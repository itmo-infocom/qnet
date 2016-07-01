#ifndef ANB_DMA_H
#define ANB_DMA_H

#include <linux/fs.h>

struct AnBDevice;

struct dma_struct {
    void* address_kernel;
    unsigned int size;
    
    dma_addr_t* address_dma;
    unsigned int buffers;
    
    char irq;
    int active_write;
    int active_read;

    unsigned int dma_suspended;
    unsigned int dma_started;
    
    wait_queue_head_t read_q, outq; 
    
    unsigned int handled;
    unsigned int suspended;
};

int enable_dma(struct AnBDevice* device);
int disable_dma(struct AnBDevice* device);

int register_dma(struct AnBDevice* device);
void unregister_dma(struct AnBDevice* device);

int change_buffer(struct AnBDevice* device, unsigned int size);

#endif
