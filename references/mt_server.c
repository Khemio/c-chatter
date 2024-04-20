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

void DieWithError(char *errorMessage);
void handleTCPClient(int clntSocket);
int CreateTCPServerSocket(unsigned short servPort);
int AcceptTCPConnection(int servSock);
void *ThreadMain(void *threadArgs);

struct ThreadArgs {
    int clntSock;
};

int main(int argc, char *argv[]) {
    int servSock;
    int clntSock;
    unsigned short echoServPort;

    pthread_t threadID;
    struct ThreadArgs *threadArgs;
    

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);

    servSock = CreateTCPServerSocket(echoServPort);

    for (;;) {
        clntSock = AcceptTCPConnection(servSock);

        // Create separate memory for client argument
        if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL) DieWithError("...");
        threadArgs->clntSock = clntSock;

        // Create client thread
        if (pthread_create(&threadID, NULL, ThreadMain, (void *) threadArgs) != 0) DieWithError("...");
           
    }
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

    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0) {
        // printf("%s\n", echoBuffer);
        if (send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize) {
            DieWithError("send() failed");
        }

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

void *ThreadMain(void *threadArgs) {
    int clntSock;

    pthread_detach(pthread_self()); /* Guarantees that thread resources are deallocated upon return */
    
    /* Extract socket file descriptor from argument */
    clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock;
    free(threadArgs);

    handleTCPClient(clntSock);

    return (NULL);
}