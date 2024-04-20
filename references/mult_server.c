#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MAXPENDING 5

void DieWithError(char *errorMessage);
void handleTCPClient(int clntSocket);
int CreateTCPServerSocket(unsigned short servPort);
int AcceptTCPConnection(int servSock);
void handleTCPClient(int clntSocket);

int main(int argc, char *argv[]) {
    int *servSock;                  /* Socket descriptors for server */
    int maxDescriptor;              /* Maximum socket descriptor value */
    fd_set socketSet;               /* Set of socket descriptors for select() */
    long timeout;                   /* Timeout value given on command-line */
    struct timeval selTimeout;      /* Timeout for select() */
    int running = 1;                /* 1 if server should be running; 0 otherwise */
    int noPorts;                    /* Number of port specified on command-line */
    int port;                       /* Looping variable for ports */
    unsigned short portNo;          /* Actual port number */

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <Timeout (secs.)> <Port 1> ...\n", argv[0]);
        exit(1);
    }

    timeout = atoi(argv[1]);         /* First arg: Timeout */
    noPorts = argc - 2;                    /* Number of ports is argument count minus 2 */

    servSock = (int *) malloc(noPorts * sizeof(int)); /* Allocate list of sockets for incoming connections */
    maxDescriptor = -1;                                     /* Initialize maxDescriptor for use by select() */


    for (port = 0; port < noPorts; port++) {                /* Create list of ports and sockets to handle ports */
        portNo = atoi(argv[port + 2]);                 /* Add port to port list. Skip first two arguments */

        servSock[port] = CreateTCPServerSocket(portNo);      /* Create port socket */

        if (servSock[port] > maxDescriptor) {                          /* Determine if new descriptor is the largest */
            maxDescriptor = servSock[port];
        }
    }

    printf("Starting server: Hit return to shutdown\n");
    while (running) {
        /* Zero socket descriptor vector and set for server sockets */
        /* This must be reset every time select() is called */
        FD_ZERO(&socketSet);
        FD_SET(STDIN_FILENO, &socketSet);           /* Add keyboard to descriptor vector */
        for (port = 0; port < noPorts; port++) FD_SET(servSock[port], &socketSet);

        /* Timeout specification */
        /* This must be reset every time select() is called */
        selTimeout.tv_sec = timeout;        /* timeout (secs.) */
        selTimeout.tv_usec = 0;             /* 0 microseconds */

        /* Suspend program until descriptor is ready or timeout */
        if (select(maxDescriptor + 1, &socketSet, NULL, NULL, &selTimeout) == 0) {
            printf("No echo requests for %ld secs... Server still alive\n", timeout);
        } else {
            if (FD_ISSET(0, &socketSet)) {          /* Check keyboard */
                printf("Shutting down server\n");
                getchar();
                running = 0;
            }

            for (port = 0; port < noPorts; port++) {
                if (FD_ISSET(servSock[port], &socketSet)) {
                    printf("Request on port %d: ", port);
                    handleTCPClient(servSock[port]);
                }
            }
        }
    }
        for (port = 0; port < noPorts; port ++) close(servSock[port]);
        free(servSock);
        exit(0);
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
