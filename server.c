#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAXPENDING 5
#define THREAD_POOL_SIZE 5

pthread_t threadPool[THREAD_POOL_SIZE];
pthread_mutex_t qmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

//TODO: Handle disconnects
//TODO: Handle memory clean up
//TODO: Add graceful shutdown
//TODO: Extract utils
//TODO: Fix warnings

//Issue 1: If every body disconnects server fails to brodcas and shuts down
//Issue 2: After reconnecting to the server client can recive messages but not send


struct queue {
    int head;
    int tail;
    int items[THREAD_POOL_SIZE * 2];
};

struct Client {
    int sockfd;
    char *name;
};

struct commArgs {
        struct Client *sender;
        struct list *clients;
        char msg[1024];
};

struct node {
    struct node *next;
    struct node *prev;
    struct Client *client;
};

struct list {
    struct node *head;
    struct node *tail;
};

struct queue* initQ(void);
void enqueue(struct queue *q, int value);
int dequeue(struct queue *q);

struct list* initL(void);
void append(struct list *l, struct Client *cl);
void delete(struct list *l, struct Client *cl);

void DieWithError(char *errorMessage);

int CreateTCPServerSocket(unsigned short servPort);
int AcceptTCPConnection(int servSock);

void handleTCPClient(struct Client *client, struct list *clients);
void *handleBrodcast(void *args);

void *ThreadMain(void *threadArgs);

struct ThreadArgs {
    // Shold be a queue
    // int clntSock;
    struct queue *sockq;
    struct list *clntL;
};

int main(int argc, char *argv[]) {
    int servSock;
    int clntSock;
    unsigned short echoServPort;

    struct ThreadArgs *threadArgs;
    if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL) DieWithError("...");
    threadArgs->sockq = initQ();
    threadArgs->clntL = initL();

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);

    servSock = CreateTCPServerSocket(echoServPort);

    // Create a thread pool with client queue
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&threadPool[i], NULL, ThreadMain, threadArgs) != 0) DieWithError("...");
    }

    for (;;) {
        clntSock = AcceptTCPConnection(servSock);

        pthread_mutex_lock(&qmutex);
        enqueue(threadArgs->sockq, clntSock);
        pthread_cond_signal(&cond_var);
        pthread_mutex_unlock(&qmutex);
           
    }
}

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

    if (q->tail > THREAD_POOL_SIZE - 1) {
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

struct list* initL(void) {
    struct list *l;
    l = malloc(sizeof(struct list));

    l->head = NULL;
    l->tail = NULL;

    return l;
}

void append(struct list *l, struct Client *cl) {
    // printf("append()\n");
    struct node *n;
    n = malloc(sizeof(struct node));

    n->client = cl;
    n->next = NULL;
    n->prev = NULL;
    // printf("node -> client -> socket: %d\n", n->client.sockfd);

    if (l->head == NULL) {
        l->head = l->tail = n;
    } else {
        n->prev = l->tail;
        l->tail->next = n;
        l->tail = n;
    }
}

void delete(struct list *l, struct Client *cl) {
    // Do error handling and messaging
    struct node *curr = l->head;
    while (curr && curr->client->sockfd != cl->sockfd) {
        // if (curr->next == NULL) return;

        curr = curr->next;     
    }

    if (curr == NULL) return;

    if (l->head == curr) {
        l->head = curr->next;
        curr->next->prev = NULL;
    }

    if (l->tail == curr) {
        l->tail = curr->prev;
        curr->prev->next = NULL;
    }

    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;

    free(curr);
}

// void destroyL(struct list l) {

// }

void DieWithError(char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    exit(1);
}

#define RCVBUFSIZE 128

void handleTCPClient(struct Client *client, struct list *clients) {
    pthread_t threadId;
    char recvMsg[RCVBUFSIZE] = {0};
    char *sendMsg;
    int recvMsgSize;

    struct commArgs *args;
    args = malloc(sizeof(struct commArgs));

    args->sender = client;
    args->clients = clients;


    if ((recvMsgSize = recv(client->sockfd, recvMsg, RCVBUFSIZE, 0)) < 0) {
        DieWithError("initial recv() failed");
    }

    client->name = malloc(sizeof(char) * strlen(recvMsg));
    int nameSize = sprintf(client->name, "%s",recvMsg);

    memset(recvMsg, 0, RCVBUFSIZE);

    char *greetings;
    greetings = malloc(sizeof("Hello ") + nameSize);
    sprintf(greetings, "Hello %s", client->name);

    if (send(client->sockfd, greetings, strlen(greetings), 0) < 0) {
            DieWithError("initial send() failed");
    }

    /* Spawn a thread to handle listening */
    while (recvMsgSize > 0) {
        memset(recvMsg, 0, RCVBUFSIZE);
        if ((recvMsgSize = recv(client->sockfd, recvMsg, RCVBUFSIZE, 0)) < 0) {
            DieWithError("recv() failed");
        }

        char *temp;
        temp = malloc(nameSize - 1);
        memcpy(temp, args->sender->name, nameSize-1);

        int msgSize = sprintf(args->msg, "%s: %s",temp, recvMsg);

        free(temp);
        temp = NULL;
 
        printf("%s\n", args->msg);

        struct node *curr = args->clients->head;

        while (curr != NULL) {
            if (curr->client->sockfd != args->sender->sockfd) {

                if (send(curr->client->sockfd, args->msg, msgSize, 0) < 0) {
                    DieWithError("brodcast send() failed");
                }
            }

            curr = curr->next;
        } 
    }

    close(client->sockfd);
}

int CreateTCPServerSocket(unsigned short servPort) {
    int servSock;
    struct sockaddr_in echoServAddr;

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        DieWithError("socket() failed");
    }

    /* Construct local addres structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));             // Zero out the structure
    echoServAddr.sin_family = AF_INET;                                 // Internet address family
    echoServAddr.sin_port = htons(servPort);            // Local port
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY)  ;      // Any incoming interface

    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        DieWithError("bind() failed");
    }

    if (listen(servSock, MAXPENDING) < 0) {
        DieWithError("listen() failed");
    }

    return servSock;
}

int AcceptTCPConnection(int servSock) {
    int clntSock;
    struct sockaddr_in echoClntAddr;
    unsigned int clntLen;                   /* Lenghth of the client address data structure */

    /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);

        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0) {
            DieWithError("accept() failed");
        }

    printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

    return clntSock;
}

// Run forever and take clients from queue
void *ThreadMain(void *threadArgs) {
    while (1) {
        int clntSock;

        // pthread_detach(pthread_self()); /* Guarantees that thread resources are deallocated upon return */
        
        /* Extract socket file descriptor from argument */
        pthread_mutex_lock(&qmutex);
        if ((clntSock = dequeue(((struct ThreadArgs *) threadArgs) -> sockq)) == NULL) {
            pthread_cond_wait(&cond_var, &qmutex);
            clntSock = dequeue(((struct ThreadArgs *) threadArgs) -> sockq);
        }
        pthread_mutex_unlock(&qmutex);
        // free(threadArgs);

        if (clntSock) {
            struct Client *client;
            client = malloc(sizeof(struct Client));
            client->sockfd = clntSock;

            append(((struct ThreadArgs *) threadArgs) -> clntL, client);

            handleTCPClient(client, ((struct ThreadArgs *) threadArgs) -> clntL);
        }
    }

    // return (NULL);
}