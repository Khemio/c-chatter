#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "utils/queue.h"
#include "utils/list.h"
#include "utils/servUtils.h"
#include "utils/includes.h"

// #define MAXPENDING 5
#define THREAD_POOL_SIZE 5

pthread_t threadPool[THREAD_POOL_SIZE];
pthread_mutex_t qmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

//TODO: Handle disconnects
//TODO: Handle memory clean up
//TODO: Add graceful shutdown
//TODO: Extract utils
//TODO: Fix warnings

//Issue 1: Total (not simultaneous) number of connections cannot exceed THREAD_POOL_SIZE 

struct commArgs {
        struct Client *sender;
        struct list *clients;
        char msg[1024];
};


void handleTCPClient(struct Client *client, struct list *clients);
void *handleBrodcast(void *args);

void *ThreadMain(void *threadArgs);

struct ThreadArgs {
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
            if (curr->client->sockfd != args->sender->sockfd && recvMsgSize > 0) {

                if (send(curr->client->sockfd, args->msg, msgSize, 0) < 0) {
                    DieWithError("brodcast send() failed");
                }
            }

            curr = curr->next;
        } 
    }
    // printf("out of loop");

    deleteI(clients, client);
    close(client->sockfd);
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