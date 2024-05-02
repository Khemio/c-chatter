#include <stdlib.h>

#include "queue.h"

struct queue* initQ(void) {
    struct queue *q;

    q = malloc(sizeof(struct queue));

    q->head = 0;
    q->tail = 0;

    return q;
}

void enqueue(struct queue *q, int value) {
    if (q->tail == q->head - 1) {
        return;
    }

    q->items[q->tail] = value;
    q->tail++;

    if (q->tail > QUEUE_SIZE - 1) {
        q->tail = 0;
    } else {
        q->tail++;
    }
}

int dequeue(struct queue *q) {
    if (q-> head == q->tail) {
        return NULL;
    }

    int value = q->items[q->head];
    q->head++;

    return value;
}
