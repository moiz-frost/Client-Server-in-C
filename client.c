#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <strings.h>


 void *recieveOutputFromServer(void* fd)
 {
	int* sock = fd;
	int serverOutputCount;
	char serverOutput[5000];
	serverOutputCount = read(*sock, serverOutput, sizeof(serverOutput));
	write(STDOUT_FILENO, serverOutput, serverOutputCount);
	pthread_exit(0);
 }

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in server;
	struct hostent *hp;
    char terminalInput[50];
    int terminalInputCount;
	pthread_t serverOutputThread;
    
	// Create socket

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Opening stream socket");
		exit(1);
	}

	// Connect socket using name specified by command line
	server.sin_family = AF_INET;

	hp = gethostbyname(argv[1]);

	if (hp <= 0)
    {
		fprintf(stderr, "%s: Unknown Host\n", argv[1]);
		exit(1);
	}

	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port = htons(atoi(argv[2]));

	if (connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) 
    {
		perror("Client Connecting Error");
		exit(1);
	}

	write(STDOUT_FILENO, "``Client Started``\n", sizeof("``Client Started``\n"));
	while(1) 
    {
        terminalInputCount = read(STDIN_FILENO, terminalInput, sizeof(terminalInput));
        write(sock, terminalInput, terminalInputCount);
		pthread_create(&serverOutputThread, NULL, recieveOutputFromServer, &sock);
    }

	pthread_join(serverOutputThread, NULL);

	close(sock);
    
}