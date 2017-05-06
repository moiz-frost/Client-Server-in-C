#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>


#define LINE_READ_SIZE 5000
#define MAX_PROCESS 50
#define MAX_CLIENTS 50
#define DATE_TIME_LENGTH 150


// ALL GLOBAL VARIABLES
char currentSystemDateTime[DATE_TIME_LENGTH]; // current system dateTime  
const char s[2] = " "; // strdelimiter
int pid = 0; // kill PID from user input
char name[MAX_PROCESS]; // kill name from user input
int cli; // clientFD
int clientWantsToDisconnect = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// STRUCT FOR PROCESS LIST
typedef struct
{
    char name[50];
    pid_t pid;
    char startTime[DATE_TIME_LENGTH];
    char stopTime[DATE_TIME_LENGTH];
    char status[10];
} process;

// STRUCT FOR CLIENT LIST
typedef struct
{
    char ip[50];
	int port;
	int clientFD;
    char status[20];
} clientInfo;

// STRUCT FOR PASSING MULTIPLE ARGUMENTS IN THREAD
struct arguments
{
    int fd;
    char *token;
};


// PROCESS COUNTER
int processCounter = 0;

// CLIENT COUNTER
int clientCounter = 0;

// MAX PROCESS STORED CAN BE 50
process p[MAX_PROCESS];

// MAX CLIENTS STORED CAN BE 50
clientInfo clients[MAX_CLIENTS];


void listClients()
{
    char string[500]; // storage for sprintf
    int stringCount; // Count value for sprintf

    if(clientCounter == 0)
    {
        write(STDOUT_FILENO, "Client List Empty\n", sizeof("Client List Empty\n"));
    }

    for (int i = 0; i < clientCounter; i++)
    {
        if((strcmp(clients[i].status, "Active")) == 0)
        {
            stringCount = sprintf(string, "-------------------------------------------------------\n"
                                            "IP : %s\n"
                                            "Port : %d\n"
                                            "Client FD : %d\n"
                                            "Status : %s\n"
                                          "-------------------------------------------------------\n",
                                    clients[i].ip,
                                    clients[i].port,
                                    clients[i].clientFD,
                                    clients[i].status
            );

            string[0] = '\0'; // For avoiding stack smashing

            write(STDOUT_FILENO, string, stringCount);
        }
    }
}

void appendClient(int fd, struct sockaddr_in client)
{
    sprintf(clients[clientCounter].ip,"%s", inet_ntoa(client.sin_addr));
    clients[clientCounter].port = ntohs(client.sin_port);
    clients[clientCounter].clientFD = fd;
    sprintf(clients[clientCounter].status, "%s", "Active");
    clientCounter++;
}

void deactivateClientFromList(int fd)
{
    for (int i = 0; i < clientCounter; i++)
    {
        if(clients[i].clientFD == fd)
        {
            if((strcmp(clients[i].status, "Active")) == 0)
            {
                sprintf(clients[i].status, "%s", "Inactive");
            }
        }
    }
}

// RETURNS THE LIST OF ALL ACTIVE AND ALL KILLED PROCESS
void listAll(int fd)
{
    if(processCounter == 0)
    {
        write(fd, "Process List is Empty\n", sizeof("Process List is Empty\n"));
        return;
    }

    char string[500]; // storage for sprintf
    int stringCount; // Count value for sprintf

    for (int i = 0; i < processCounter; i++)
    {
        stringCount = sprintf(string, "-------------------------------------------------------\n"
                                        "Name : %s\n"
                                        "PID : %d\n"
                                        "Start Time : %s"
                                        "Stop Time : %s"
                                        "Status : %s\n"
                                      "-------------------------------------------------------\n",
                                p[i].name,
                                p[i].pid,
                                p[i].startTime,
                                p[i].stopTime,
                                p[i].status);

        string[0] = '\0'; // For avoiding stack smashing

        write(fd, string, stringCount);
    }
}



// RETURNS THE LIST OF ALL ACTIVE PROCESS
void listActive(int fd)
{
    int isListEmpty = 1;

    if(processCounter == 0)
    {
        write(fd, "Process List is Empty\n", sizeof("Process List is Empty\n"));
        return;
    }

    char string[500]; // Storage for sprintf
    int stringCount; // Count value for sprintf

    for (int i = 0; i < processCounter; i++)
    {
        if(strcmp(p[i].status, "Active") == 0)
        {
            isListEmpty = 0;
            stringCount = sprintf(string, "-------------------------------------------------------\n"
                                            "Name : %s\n"
                                            "PID : %d\n"
                                            "Start Time : %s"
                                            "Status : %s\n"
                                          "-------------------------------------------------------\n",
                                          p[i].name,
                                          p[i].pid,
                                          p[i].startTime,
                                          p[i].status);

            write(fd, string, stringCount);
        }
    }

    if(isListEmpty == 1)
    {
        write(fd, "No active processes\n", sizeof("No active processes\n"));
    }


    return;
}

// REGISTER SIGNAL HANDLER FOR INVALID PROGRAM
static void sigusr1_handler (int signo)
{
    if(signo == SIGUSR1)
    {
        processCounter--;
        write(STDOUT_FILENO, "Program not found!\n", sizeof("Program not found!\n"));
    }
    return;
}

// REGISTER SIGNAL HANDLER FOR UPDATING CLIENT LIST
static void sigusr2_handler (int signo)
{
    if(signo == SIGUSR2)
    {
        deactivateClientFromList(cli);        
        // write(STDOUT_FILENO, "DISCONNECT\n", sizeof("DISCONNECT\n"));
    }
    return;
}



// REGISTER SIGNAL HANDLER FOR PRINTING PROCESS LIST
static void sigquit_handler (int signo)
{
    if(signo == SIGQUIT)
    {
        listAll(1);        
        // write(STDOUT_FILENO, "DISCONNECT\n", sizeof("DISCONNECT\n"));
    }
    return;
}

static void sigchld_handler (int signo)
{
    if(signo == SIGCHLD)
    {
        // write(STDOUT_FILENO, "SIGCHLD Caught\n", sizeof("SIGCHLD Caught\n"));
    }
    return;
}

// GETS CURRENT SYSTEM DATE AND TIME AND STORES IN currentSystemDateTime VARIABLE
char setCurrentSystemDateTime()
{
    time_t curtime;
    struct tm *loc_time;
    
    // Getting current time of system
    curtime = time (NULL);
    
    // Converting current time to local time
    loc_time = localtime (&curtime);

    // Storing dateTime as string   
    sprintf(currentSystemDateTime, "%s", asctime(loc_time));
    // printf("%s", currentDateOutput);    
}

// KILL WITH PROCESS ID
void killWithPID(int pid, int fd)
{
    char message[50];
    int count;
    int doesNotExist = 1;
    for (int i = 0; i < processCounter; i++)
    {
        if(p[i].pid == pid)
        {
            if((strcmp(p[i].status, "Active") == 0))
            {
                kill(pid, SIGTERM);
                sprintf(p[i].stopTime, "%s", currentSystemDateTime);
                sprintf(p[i].status, "%s", "Inactive");
                count = sprintf(message, "~~~Killed Process~~~\n    PID : %d\n", pid);
                write(fd, message, count);
            }
            else
            {
                write(fd, "\nProcess already Inactive\n\n", sizeof("\nProcess already Inactive\n\n"));
            }
            break;
        }
    }

    if(doesNotExist == 1)
    {
        write(fd, "No process with specified PID exists\n", sizeof("No process with specified PID exists\n"));
    }
}

// KILL WITH PROCESS Name
void killWithName(char *name, int fd)
{
    char processName[50];
    char message[50]; // Message to print on screen
    int count; // Stores sprintf
    int inactiveCounter; // 
    int doesNotExist = 1;

    count = sprintf(processName, "%s\n", name);

    for (int i = 0; i < processCounter; i++)
    {
        if(strcmp(p[i].name, processName))
        {
            doesNotExist = 0;
            if((strcmp(p[i].status, "Active") == 0))
            {
                pid = p[i].pid;
                kill(pid, SIGTERM);
                sprintf(p[i].stopTime, "%s", currentSystemDateTime);
                sprintf(p[i].status, "%s", "Inactive");
                count = sprintf(message, "~~~Killed Process~~~\n   Name : %s    PID : %d\n", processName, pid);
                write(fd, message, count);
                break;
            }
            if((strcmp(p[i].status, "Inactive") == 0))
            {
                inactiveCounter++;
            }
            count = sprintf(message, "\n%d Processe(s) are already inactive\n\n", inactiveCounter);
            write(fd, message, count);
        }
    }
    if(doesNotExist == 1)
    {
        write(fd, "No process with specified name exists\n", sizeof("No process with specified name exists\n"));
    }
}



void killAll()
{
    for (int i = 0; i < processCounter; i++)
    {
        pid = p[i].pid;
        kill(pid, SIGTERM);
    }
}

//KILL ALL PROCESS WITH SAME NAME
void killAllWithName(char *name, int fd)
{
    char processName[50];
    char message[100];
    int count;
    int doesNotExist = 1;
    count = sprintf(processName, "%s\n", name);
    for (int i = 0; i < processCounter; i++)
    {
        doesNotExist = 0;
        if(strcmp(p[i].name, processName)) 
        {
            if((strcmp(p[i].status, "Active") == 0))
            {
                pid = p[i].pid;
                kill(pid, SIGTERM);
                sprintf(p[i].stopTime, "%s", currentSystemDateTime);
                sprintf(p[i].status, "%s", "Inactive");
            }
        }
    }
    if(doesNotExist == 1)
    {
        write(fd, "No process with specified name exists\n", sizeof("No process with specified name exists\n"));
    } else 
    {
        count = sprintf(message, "~~~Killed all instances of Process~~~\n   Name : %s\n", processName);        
    }
    write(fd, message, count);
}

// RUNS A PROGRAM
void run(char program[], int fd)
{
    int execStatus = 0; // exec status of execlp in child
    int status = 0; // exit status
    char stringStatus[10];
    int count = 0; // buffer count
    pid_t pid = 0; // pid

    signal(SIGUSR1, sigusr1_handler);
    
    switch (pid = fork())
    {

        case 0:
        execStatus = execlp(program, program, (char *)0);
        // If invalid program, then resort to the code below, which will basically generate a signal
        write(fd, "Invalid Program!\n", sizeof("Invalid Program!\n"));
        kill(getppid(), SIGUSR1);
        exit(2);

        default:
        setCurrentSystemDateTime();
        sprintf(p[processCounter].name, "%s", program);
        p[processCounter].pid = pid;
        sprintf(p[processCounter].startTime, "%s", currentSystemDateTime);
        sprintf(p[processCounter].stopTime, "%s", "Not Stopped\n");
        sprintf(p[processCounter].status, "%s", "Active");
        processCounter++;

        waitpid(pid, &status, WNOHANG);

        signal(SIGCHLD, sigchld_handler);

            
    }
}


void* add(void* argumentList)
{
    pthread_mutex_lock (&mutex);
    
    sleep(3);
    struct arguments *args = (struct arguments*)argumentList;
    int sock = args ->fd;
    char* token = args ->token;
    char *ptr; // int pointer for add/mult
    int result; // stores result for add/mult
    int buff_size; // storing size from sprintf
    int err = 0; // stores error for add/mult
    char res[LINE_READ_SIZE]; // stores result in string
    ptr = '\0';
    result = 0;
    token = strtok(NULL, s);
    while(token != NULL)
    {
        result = result + strtod(token, &ptr);
        token = strtok(NULL, s);   
    }
    if(result != 0)
    {
        buff_size = sprintf(res, "%d\n", result);
        err = write(STDOUT_FILENO, res, buff_size);
        err = write(sock, res, buff_size);
    }

    pthread_mutex_unlock(&mutex);
    pthread_exit(0);
}

void* subtract(void* argumentList)
{
    struct arguments *args = (struct arguments*)argumentList;
    int sock = args ->fd;
    char* token = args ->token;
    char *ptr; // int pointer for add/mult
    int result; // stores result for add/mult
    int buff_size; // storing size from sprintf
    int err = 0; // stores error for add/mult
    char res[LINE_READ_SIZE]; // stores result in string
    ptr = '\0';
    result = 0;
    token = strtok(NULL, s);
    while(token != NULL)
    {
        result = result - strtod(token, &ptr);
        token = strtok(NULL, s);   
    }
    if(result != 0)
    {
        buff_size = sprintf(res, "%d\n", result);
        err = write(STDOUT_FILENO, res, buff_size);
        err = write(sock, res, buff_size);
    }
    pthread_exit(0);
}

void* multiply(void* argumentList)
{
    struct arguments *args = (struct arguments*)argumentList;
    int sock = args ->fd;
    char* token = args ->token;
    char *ptr; // int pointer for add/mult
    int result; // stores result for add/mult
    int buff_size; // storing size from sprintf
    int err = 0; // stores error for add/mult
    char res[LINE_READ_SIZE]; // stores result in string
    ptr = '\0';
    result = 1;
    token = strtok(NULL, s);
    while(token != NULL)
    {
        result = result * strtod(token, &ptr);
        token = strtok(NULL, s);
    }
    if(result != 0)
    {
        buff_size = sprintf(res, "%d\n", result);
        err = write(STDOUT_FILENO, res, buff_size);
        err = write(sock, res, buff_size);
    }
    pthread_exit(0);
}

void parseKill(int fd, char* token)
{
    char *ptr;
    char* tokenTwo;
    token = strtok(NULL, s);
    tokenTwo = strtok(NULL, s);
    
    if(token != NULL)
    {
        pid = strtod(token, &ptr);
        if(tokenTwo != NULL)
        {
            if(strcmp(tokenTwo, "all") == 0)
            {
                
                killAllWithName(ptr, fd);
            }
        }
        else if(pid > 0)
        {
            killWithPID(pid, fd);
        }
        else if(pid == 0)
        {
            killWithName(ptr, fd);
        }
    }
    else
    {
        char killOutput[] = "Usage:"
                            "kill <pid>\n"
                            "kill <process name>\n"
                            "kill <process name> all\n";

        write(fd, killOutput, sizeof(killOutput));
    }
}

void parseList(int fd, char *token)
{
    token = strtok(NULL, s);
    if(token != NULL)
    {            
        if(strcmp(token, "all") == 0)
        {
            listAll(fd);
        }
    }
    else
    {
        listActive(fd);
    }
    return;
}

// INITIALIZES AND RETURNS A SOCKET FD
int initServer()
{
    int sock, serverLength;
	struct sockaddr_in server;
    char port[30];

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0); // AF_INET (IPv4), SOCK_STREAM(TCP protocol)
	if (sock < 0) 
    {
		perror("Opening Socket Error");
		exit(1);
	}

	/* Name socket using wildcards */
	server.sin_family = AF_INET; // IPv4 internet protocols
	server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = 0; // server port
	if (bind(sock, (struct sockaddr *) &server, sizeof(server))) // assigns an address space for the socket specidifed by 2nd argument
    {
		perror("Server Bind Error");
		exit(1);
	}

    serverLength = sizeof(server);
    if (getsockname(sock, (struct sockaddr *) &server, (socklen_t*) &serverLength)) 
    {
		perror("Getting Socket Name");
		exit(1);
	}
    int portCount = sprintf(port, "Server Port number is : %d\n", ntohs(server.sin_port));
    write(STDOUT_FILENO, port, portCount);


    if((listen(sock, 5)) == -1)
    {
        perror("Server Listen Error");
        exit(-1);
    }


    return sock;
}


void serverParser(char terminalInput[], int terminalInputCount)
{
	const char s[2] = " "; // strdelimiter
	char *token;
	char *tokenTwo;
    char *tokenThree;
    char *tokenFour;
    
    if((terminalInputCount == 1))
    {
        write(STDOUT_FILENO, "No command entered\n", sizeof("No command entered\n"));
        return;
    }

	terminalInput[terminalInputCount - 1] = '\0';
	
	token = strtok(terminalInput, s);


    if(strcmp(token, "exit") == 0)
    {
        exit(0);
    }

    if(strcmp(token, "list") == 0)
    {
        tokenTwo = strtok(NULL, s);
        if(tokenTwo == NULL)
        {
            listClients();
            return;
        }
        if(strcmp(tokenTwo, "all") == 0)
        {
            killpg(0, SIGQUIT);
        }
        return;
    }

    if(strcmp(token, "message") == 0)
    {
        char *ptr = "\0";
        if(token != NULL)
        {
            tokenTwo = strtok(NULL, s);
            if(tokenTwo != NULL)
            {
                int targetFD = strtod(tokenTwo, &ptr);
                tokenThree = strtok(NULL, "");
                write(targetFD, tokenThree, strlen(tokenThree));
            }
        }
        return;
    }


    // If help
    if(strcmp(line, "help") == 0)
    {
        char helpOutput[] = "----------------------------------------------------------------------------------------\n"
                            "lisr                                       -- to display list of connected clients\n"
                            "list all                                   -- to display list of all currently running processes\n"
                            "message <fd of client> <your message>      -- to send a message to a desired client\n"
                            "exit                                       -- to quit now!\n"
                            "disconnect                                 -- to disconnect client\n"
                            "----------------------------------------------------------------------------------------\n";

        write(STDOUT_FILENO, helpOutput, sizeof(helpOutput));
        return 0;
    }




    write(STDOUT_FILENO, "Invalid Command\n", sizeof("Invalid Command\n"));
	return;
}


int parser(int fd, char line[], int lineCount)
{
    pthread_t addThread;
    pthread_t subThread;
    pthread_t multThread;

    char* token; // strtoken
    
    if(lineCount == 1) // a workaround to prevent segmentation fault
    {
        write(fd, "No command entered\n", sizeof("No command entered\n"));
        return 0;
    }
    
    line[lineCount - 1] = '\0'; // add a null character at the end
    token = strtok(line, s); // initial tokenization

    // If run
    if(strcmp(token, "run") == 0)
    {
        token = strtok(NULL, s);
        if(token != NULL)
        {
            run(token, fd);
        }
        else
        {
            write(fd, "run <process name>\n", sizeof("run <process name>\n"));
        }
        return 0;
    }


    // If add
    if(strcmp(token, "add") == 0)
    {
        struct arguments args;
        args.fd = fd;
        args.token = token;
        pthread_create(&addThread, NULL, add, (void *)&args);
        pthread_join(addThread, NULL);
        return 0;
    }

    // If sub
    if(strcmp(token, "sub") == 0)
    {
        struct arguments args;
        args.fd = fd;
        args.token = token;
        pthread_create(&subThread, NULL, subtract, (void *)&args);
        pthread_join(subThread, NULL);
        return 0;
    }

    // If mult
    if(strcmp(token, "mult") == 0)
    {
        struct arguments args;
        args.fd = fd;
        args.token = token;
        pthread_create(&multThread, NULL, multiply, (void *)&args);
        pthread_join(multThread, NULL);
        return 0;
    }

    // If list all/list
    if(strcmp(token, "list") == 0)
    {
        parseList(fd, token);
        return 0;
    }

    // If kill
    if(strcmp(token, "kill") == 0)
    {
        parseKill(fd, token);
        return 0;
    }

    // If disconnect
    if(strcmp(token, "disconnect") == 0)
    {
        killAll();
        kill(getppid(), SIGUSR2);
        write(fd, "Disconnected From Server\n", sizeof("Disconnected From Server\n"));
        return fd;
    }


    // If help
    if(strcmp(line, "help") == 0)
    {
        char helpOutput[] = "----------------------------------------------------------------------------------------\n"
                            "run <process name>                         -- to execute a program\n"
                            "kill <process PID>                         -- to kill a process by PID\n"
                            "kill <process name>                        -- to kill a process by name\n"
                            "kill <process name> all                    -- to kill all process by the same name\n"
                            "add <integer1> <integer2> ... <integerN>   -- to add multiple numbers\n"
                            "sub <integer1> <integer2> ... <integerN>   -- to subtract multiple numbers\n"
                            "mult <integer1> <integer2> ... <integerN>  -- to multiply multiple numbers\n"
                            "exit                                       -- to quit now!\n"
                            "disconnect                                 -- to disconnect client\n"
                            "----------------------------------------------------------------------------------------\n";

        write(fd, helpOutput, sizeof(helpOutput));
        return 0;
    }

    write(fd, "Invalid Command\n", sizeof("Invalid Command\n")); // if all else fails
    return 0;

}

// THREAD FOR READING FROM SERVER INPUT
void* mainServerTerminalReader()
{
    write(STDOUT_FILENO, "``Server Started``\n", sizeof("``Server Started``\n"));
    while(1)
    {
        char terminalLineBuffer[LINE_READ_SIZE]; // for reading from terminal
        int terminalLineCount = read(STDIN_FILENO, &terminalLineBuffer, LINE_READ_SIZE);  

        if(terminalLineCount < 0)
        {
            perror("Server Terminal Read Error");
            exit(0);
        }

        serverParser(terminalLineBuffer, terminalLineCount);
        // write(STDOUT_FILENO, terminalLineBuffer, terminalLineCount);
    }
    pthread_exit(0);
}

// MAIN FUNCTION
int main(int argc, char const *argv[])
{
    int sock = initServer();
    int status;
    pid_t pid;
    int len = sizeof(struct sockaddr_in);
    char clientFD[50];
    char inputFromClient[50];
    struct sockaddr_in client;    
    pthread_t serverThread; // thread ID

    signal(SIGUSR2, sigusr2_handler);
    signal(SIGQUIT, sigquit_handler);
    

    int threadReturnValue = pthread_create(&serverThread, NULL, mainServerTerminalReader, NULL); // create a thread task
    if(threadReturnValue < 0)
    {
        perror("Main Server Thread Error");
    }
    while(1)
    {
        if((cli = accept(sock, (struct sockaddr *)&client, &len)) < 0)
        {
            perror("Accept Error");
            exit(1);
        }

        appendClient(cli, client);

        // while(1)
        // {
        //     parser(cli);          
        // }

        switch (pid = fork())
        {
            case -1:
            perror("The client fork failed!");
            write(STDOUT_FILENO, "Your mom\n", sizeof("Your mom\n"));
            break;
        
            case 0:
            while(1)
            {
                char line[LINE_READ_SIZE]; // reading from terminal
                int lineCount = 0; // line count 
                lineCount = read(cli, line, LINE_READ_SIZE); // read from client fd
                int clientWantsToDisconnecFD = parser(cli, line, lineCount);
            }
            exit(cli);
            
            // default:
            // waitpid(pid, &status, WNOHANG);
            // if(WIFEXITED(status))
            // {
            //     int result = WEXITSTATUS(status);
            //     char stringStatus[20];
            //     int stringStatusCount = sprintf(stringStatus, "%d", result);
            //     write(1, stringStatus, stringStatusCount);
            // }
        }
    }

    pthread_join(serverThread, NULL); // waits until a thread is complete so that the program doesn't exit before thread execution
        
    return 0;
}