#include <unistd.h>
#include <pthread.h>

#include "utils/includes.h"

void DieWithError(char *errorMessage);
void handleConnection(int clntSocket);
void *listenToConn(void * servSock);
int CreateTCPSocket(char *ip_addr, unsigned short servPort);

int main(void)
{
    pthread_t threadId;
    int chat;
    int n = 0;
    char *buffer;
    size_t bufSize = 256;
    char recvBuff[1024] = {0};
    char sendBuff[1025] = {0};

    buffer = malloc(sizeof(char) * bufSize);
  
    chat = CreateTCPSocket("127.0.0.1", 5001);

    printf("Please enter your screen name: ");
    // scanf("%s", msg);
    getline(&buffer, &bufSize, stdin);

    strcpy(sendBuff, buffer);
    // strcpy(sendBuff, "test message");
    send(chat, sendBuff, strlen(sendBuff), 0);

    pthread_create(&threadId, NULL, listenToConn, &chat);

    while (true) {
        // memset(recvBuff, 0 ,sizeof(recvBuff));
        memset(sendBuff, 0 ,sizeof(sendBuff));
        memset(buffer, 0 ,sizeof(&buffer));

        // Recv on separate thread

        // recv(chat, recvBuff, sizeof(recvBuff), 0);
        // printf("%s\n", recvBuff);

        getline(&buffer, &bufSize, stdin);
        if (strcmp(buffer, "exit") == 0) {
            return 0;
        }

        strcpy(sendBuff, buffer);
        send(chat, sendBuff, strlen(sendBuff), 0);

    }

    close(chat);
    return 0;
}

int CreateTCPSocket(char *ip_addr, unsigned short servPort) {
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
            printf("\n Error : Could not create socket \n");
            return 1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(servPort);
    serv_addr.sin_addr.s_addr = inet_addr(ip_addr); // strcp or something
 
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
            printf("\n Error : Connect Failed \n");
            return 1;
    }

    return sockfd;
}

void *listenToConn(void *servSock) {
    int chat = *(int *) servSock;
    char recvBuff[1024] = {0};

    while (true) {
        memset(recvBuff, 0 ,sizeof(recvBuff));

        if (recv(chat, recvBuff, sizeof(recvBuff), 0) > 0) {

            printf("%s\n", recvBuff);
        }
    }

}