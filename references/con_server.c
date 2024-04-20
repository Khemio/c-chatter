#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXPENDING 5

void DieWithError(char *errorMessage);
void handleTCPClient(int clntSocket);
int CreateTCPServerSocket(unsigned short servPort);
int AcceptTCPConnection(int servSock);
void ProcessMain(int servSock);

int main(int argc, char *argv[]) {
    int servSock;
    int clntSock;
    unsigned short echoServPort;

    pid_t processID;
    unsigned int processLimit;
    unsigned int processCt;
    

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server Port> <FORK LIMIT>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);
    processLimit = atoi(argv[2]);

    servSock = CreateTCPServerSocket(echoServPort);

    for (processCt = 0; processCt < processLimit; processCt++) {
        if ((processID = fork()) < 0) DieWithError("fork() failed");
        else if (processID == 0) ProcessMain(servSock);
    }
    exit(0);    /* The children will carry on */
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

void ProcessMain(int servSock) {
    int clntSock;

    for (;;) {
        clntSock = AcceptTCPConnection(servSock);
        printf("with child process: %d\n", (unsigned int) getpid());
        handleTCPClient(clntSock);
    }
}