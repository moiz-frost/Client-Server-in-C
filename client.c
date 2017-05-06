#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define LINE_READ_SIZE 5000

int clientFD;
int isClientActive = 0; // 1 == clientFD active, 0 = STDOUT_FILENO active

pthread_t serverOutputThread;
pthread_t serverInputThread;

 int initializeClientFD(char *host, char *port)
 {
	int sock;
	struct sockaddr_in server;
	struct hostent *hp;

	// Create socket

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Opening stream socket");
		exit(1);
	}

	// Connect socket using name specified by command line
	server.sin_family = AF_INET;

	hp = gethostbyname(host);

	if (hp <= 0)
    {
		fprintf(stderr, "%s: Unknown Host\n", host);
		exit(1);
	}

	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port = htons(atoi(port));

	if (connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) 
    {
		perror("Client Connecting Error");
		exit(1);
	}

	write(STDOUT_FILENO, "``Client Initialized``\n", sizeof("``Client Initialized``\n"));

	return sock;
 }

 void disconnect(int fd)
 {
	 write(STDOUT_FILENO, "``Client Closed``\n", sizeof("``Client Closed``\n"));	 
	 close(fd);	 
	//  pthread_cancel(serverOutputThread);
	//  pthread_cancel(serverInputThread);
 }


 void parser(int fd, char terminalInput[], int terminalInputCount)
{
	const char s[2] = " "; // strdelimiter
	char *token;
	char *tokenTwo;
    
    if((terminalInputCount == 1))
    {
		if(isClientActive == 0)
		{
			write(STDOUT_FILENO, "No command entered\n", sizeof("No command entered\n"));
			return;
		}
		if(isClientActive == 1)
		{
			return;
		}
    }

	terminalInput[terminalInputCount - 1] = '\0';
	
	token = strtok(terminalInput, s);
	
	// If exit
    if(strcmp(token, "exit") == 0)
    {
		exit(0);   
    }
	return;

	if(isClientActive == 0)
	{
		if(strcmp(token, "connect") == 0)
		{
			token = strtok(NULL, s);
			tokenTwo = strtok(NULL, s);
			if(tokenTwo != NULL)
			{
				clientFD = initializeClientFD(token, tokenTwo);
				isClientActive = 1;
			} else
			{
				write(STDOUT_FILENO, "Incomplete Command\n", sizeof("Incomplete Command\n"));
			}
		} else
		{
			write(STDOUT_FILENO, "Invalid Command\n", sizeof("Invalid Command\n"));
		}
		return;
	}


	if(isClientActive == 1)
	{
		if(strcmp(token, "disconnect") == 0)
		{
			if(fd > 2)
			{
				isClientActive = 0;
				disconnect(fd);
			} else
			{
				write(STDOUT_FILENO, "Invalid File Descriptor\n", sizeof("Invalid File Descriptor\n"));
			}
			return;
		}
	}


}

 // LISTENER FOR OUTPUTS FROM THE SERVER
 void *recieveOutputFromServer()
 {
	 int serverOutputCount;
	 char serverOutput[5000];
	 while(1)
	 {
		 while(isClientActive == 0)
		 {
			 // lalala..
		 }
		 while(isClientActive == 1)
		 {
			 serverOutputCount = read(clientFD, serverOutput, sizeof(serverOutput));
			 write(STDOUT_FILENO, serverOutput, serverOutputCount);
		 }
	 }

	 pthread_exit(0);
 }

// LISTENER FOR INPUTS FROM THE CLIENT
void *recieveInputFromClient()
{
	char clientInput[5000];
	int clientInputCount;
	char parserInput[5000];
	int parserInputCount;

	while(1)
	{
		while(isClientActive == 0)
		{
			clientInputCount = read(STDOUT_FILENO, clientInput, sizeof(clientInput));
			parser(STDOUT_FILENO, clientInput, clientInputCount);
		}
		while(isClientActive == 1)
		{
			memset(clientInput, '\0', sizeof(clientInput)); // Clearing the buffer	

			clientInputCount = read(STDOUT_FILENO, clientInput, sizeof(clientInput));
			parserInputCount = sprintf(parserInput, "%s", clientInput);

			write(clientFD, clientInput, clientInputCount);
			parser(clientFD, parserInput, parserInputCount);
		}
	}
	
	pthread_exit(0);

 }



int main(int argc, char *argv[])
{
	pthread_create(&serverOutputThread, NULL, recieveOutputFromServer, NULL);
	pthread_create(&serverInputThread, NULL, recieveInputFromClient, NULL);

	pthread_join(serverOutputThread, NULL);

    
}