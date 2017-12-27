#ifndef KEYQUEUE_H
#define KEYQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    uint8_t sha[32];
    uint8_t key[32];
    uint8_t usage;
    struct KEY *prev;
} KEY;

typedef struct Queue {
    KEY *head;
    KEY *tail;
    int size;
    int limit;
} Queue;

KEY *ConstructKey(uint8_t *buf);
Queue *ConstructQueue(int limit);
void DestructQueue(Queue *queue);
int Enqueue(Queue *pQueue, KEY *item);
KEY *Dequeue(Queue *pQueue);
KEY *CopyKey(KEY *k);
int isEmpty(Queue* pQueue);

#ifdef __cplusplus
}
#endif

#endif

