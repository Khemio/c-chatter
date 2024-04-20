#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
 
int main(void)
{
  int sockfd = 0,n = 0;
  char msg[100];
  char recvBuff[1024] = {0};
  char sendBuff[1025] = {0};
  struct sockaddr_in serv_addr;

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
      printf("\n Error : Could not create socket \n");
      return 1;
    }
 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(5000);
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
      printf("\n Error : Connect Failed \n");
      return 1;
    }
 
    strcpy(sendBuff, "Hello");
    send(sockfd, sendBuff, strlen(sendBuff), 0);

  do {
    memset(recvBuff, 0 ,sizeof(recvBuff));
    memset(sendBuff, 0 ,sizeof(sendBuff));

    read(sockfd, recvBuff, sizeof(recvBuff)-1);
    printf("%s\n", recvBuff);

    scanf("%s", msg);

    strcpy(sendBuff, msg);
    send(sockfd, sendBuff, strlen(sendBuff), 0);
  } while ((int)*msg != EOF);

  close(sockfd);
  return 0;
}