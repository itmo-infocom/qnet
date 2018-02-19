#ifndef KEYQUEUE_H
#define KEYQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

    typedef struct KEY {
        uint8_t sha[32];
        uint8_t key[32];
        uint16_t usage;
        struct KEY *prev;
    } KEY;

    typedef struct Queue {
        KEY *head;
        KEY *tail;
        int size;
        int limit;
    } Queue;

    KEY *ConstructKey(uint8_t *buf);
    KEY *ConstructKeyUsage(uint8_t *buf, uint16_t usage);
    KEY *ConstructKeySha(uint8_t *buf, uint8_t *sha);
    KEY *ConstructKeyShaUsage(uint8_t *buf, uint8_t *sha, uint16_t usage);
    Queue *ConstructQueue(int limit);
    void DestructQueue(Queue *queue);
    int Enqueue(Queue *pQueue, KEY *item);
    KEY *Dequeue(Queue *pQueue);
    KEY *CopyKey(KEY *k);
    void PrintKey(KEY *k);
    int isEmpty(Queue* pQueue);

#ifdef __cplusplus
}
#endif

#endif

