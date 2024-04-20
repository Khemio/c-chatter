// #include <netinet/in.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXPENDING 5

void DieWithError(char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    exit(1);
}
void handleTCPClient(int clntSocket);

int main(int argc, char *argv[]) {
    int servSock;
    int clntSock;
    struct sockaddr_in echoServAddr;
    struct sockaddr_in echoClntAddr;
    unsigned short echoServPort;
    unsigned int clntLen;                   /* Lenghth of the client address data structure */

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        DieWithError("socket() failed");
    }

    /* Construct local addres structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));             // Zero out the structure
    echoServAddr.sin_family = AF_INET;                                 // Internet address family
    echoServAddr.sin_port = htons(echoServPort);            // Local port
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY)  ;      // Any incoming interface

    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        DieWithError("bind() failed");
    }

    if (listen(servSock, MAXPENDING) < 0) {
        DieWithError("listen() failed");
    }

    for (;;) {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);

        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0) {
            DieWithError("accept() failed");
        }

        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        handleTCPClient(clntSock);
    }
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
