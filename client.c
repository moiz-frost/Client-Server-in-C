#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>


/*
 * This program creates a socket and initiates a connection with the socket
 * given in the command line.  One message is sent over the connection and
 * then the socket is closed, ending the connection. The form of the command
 * line is streamwrite hostname portnumber 
 */

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in server;
	struct hostent *hp;
    char terminalInput[50];
    int terminalInputCount;
    int pipe[2];
	int serverOutputCount;
	char serverOutput[5000];
    
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

	while(write(STDOUT_FILENO, "> ", sizeof("> "))) 
    {
        terminalInputCount = read(STDIN_FILENO, terminalInput, sizeof(terminalInput));
        write(sock, terminalInput, terminalInputCount);
		serverOutputCount = read(sock, serverOutput, sizeof(serverOutput));
		write(STDOUT_FILENO, serverOutput, serverOutputCount);
    }

	close(sock);
    
}