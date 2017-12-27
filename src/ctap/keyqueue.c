#include <stdlib.h>
#include <stdio.h>
#include "keyqueue.h"
#include "sha3.h"


#define TRUE  1
#define FALSE	0


Queue *ConstructQueue(int limit);
void DestructQueue(Queue *queue);
int Enqueue(Queue *pQueue, KEY *item);
KEY *Dequeue(Queue *pQueue);
int isEmpty(Queue* pQueue);

Queue *ConstructQueue(int limit) {
    Queue *queue = (Queue*) malloc(sizeof (Queue));
    if (queue == NULL) {
        return NULL;
    }
    if (limit <= 0) {
        limit = 65535;
    }
    queue->limit = limit;
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;

    return queue;
}

KEY *CopyKey(KEY *k){
    KEY *key = (KEY*) malloc(sizeof (KEY));
    if (key == NULL) {
        return NULL;
    }
    memcpy(&k, &key, sizeof (KEY));    
    return key;
}

KEY *ConstructKey(uint8_t *buf) {    
    KEY *key = (KEY*) malloc(sizeof (KEY));
    if (key == NULL) {
        return NULL;
    }
    key->usage=0;
    int i=0;
    for(i=0;i<32;i++){
        key->key[i]=buf[i];
    }
    sha3(key->key,32,key->sha,32);
    return key;
}

void DestructQueue(Queue *queue) {
    KEY *pN;
    while (!isEmpty(queue)) {
        pN = Dequeue(queue);
        free(pN);
    }
    free(queue);
}

int Enqueue(Queue *pQueue, KEY *item) {
    /* Bad parameter */
    if ((pQueue == NULL) || (item == NULL)) {
        return FALSE;
    }
    // if(pQueue->limit != 0)
    if (pQueue->size >= pQueue->limit) {
        return FALSE;
    }
    /*the queue is empty*/
    item->prev = NULL;
    if (pQueue->size == 0) {
        pQueue->head = item;
        pQueue->tail = item;

    } else {
        /*adding item to the end of the queue*/
        pQueue->tail->prev = item;
        pQueue->tail = item;
    }
    pQueue->size++;
    return TRUE;
}

KEY * Dequeue(Queue *pQueue) {
    /*the queue is empty or bad param*/
    KEY *item;
    if (isEmpty(pQueue))
        return NULL;
    item = pQueue->head;
    pQueue->head = (pQueue->head)->prev;
    pQueue->size--;
    return item;
}

int isEmpty(Queue* pQueue) {
    if (pQueue == NULL) {
        return FALSE;
    }
    if (pQueue->size == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}
