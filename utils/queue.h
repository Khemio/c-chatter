#define QUEUE_SIZE 10
// Might need to think abou the size of the queue
struct queue {
    int head;
    int tail;
    int items[QUEUE_SIZE];
};

struct queue* initQ(void);

void enqueue(struct queue *q, int value);

int dequeue(struct queue *q);