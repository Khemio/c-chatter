#define MAXPENDING 5

void DieWithError(char *errorMessage);

int CreateTCPServerSocket(unsigned short servPort);
int AcceptTCPConnection(int servSock);