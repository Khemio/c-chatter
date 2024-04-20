#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAXPENDING 5
#define THREAD_POOL_SIZE 4

pthread_t threadPool[THREAD_POOL_SIZE];
pthread_mutex_t qmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

struct queue {
    int head;
    int tail;
    int items[THREAD_POOL_SIZE];
};

struct queue* initQ(void);
void enqueue(struct queue *q, int value);
int dequeue(struct queue *q);

void DieWithError(char *errorMessage);
void handleTCPClient(int clntSocket);
int CreateTCPServerSocket(unsigned short servPort);
int AcceptTCPConnection(int servSock);
void *ThreadMain(void *threadArgs);

struct ThreadArgs {
    // Shold be a queue
    // int clntSock;
    struct queue *sockq;
};

int main(int argc, char *argv[]) {
    int servSock;
    int clntSock;
    unsigned short echoServPort;

    struct ThreadArgs *threadArgs;
    if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL) DieWithError("...");
    threadArgs->sockq = initQ();

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

void DieWithError(char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    exit(1);
}

#define RCVBUFSIZE 32

void handleTCPClient(int clntSocket) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;

    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0) {
        DieWithError("recv() failed");
    }

    // Testing
    // printf("%s\n", echoBuffer);

    // if (send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize) {
    //         DieWithError("send() failed");
    //     }
    // close(clntSocket);


    // Echo
    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0) {
        if (send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize) {
            DieWithError("send() failed");
        }
        close(clntSocket);

        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0) {
            DieWithError("recv() failed");
        }
    }

    close(clntSocket);
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

        if (clntSock) handleTCPClient(clntSock);
    }

    // return (NULL);
}