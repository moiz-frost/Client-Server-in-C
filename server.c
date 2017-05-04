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


#define LINE_READ_SIZE 100
#define MAX_PROCESS 50
#define DATE_TIME_LENGTH 150


// ALL GLOBAL VARIABLES
char currentSystemDateTime[DATE_TIME_LENGTH]; // current system dateTime
char line[LINE_READ_SIZE]; // reading from terminal
int lineCount = 0; // line count   
char lineConcat[4]; // length = 4
char* token; // strtoken
const char s[2] = " "; // strdelimiter
int pid = 0; // kill PID from user input
char name[MAX_PROCESS]; // kill name from user input

// STRUCT FOR PROCESS LIST
typedef struct
{
    char name[50];
    pid_t pid;
    char startTime[DATE_TIME_LENGTH];
    char stopTime[DATE_TIME_LENGTH];
    char status[10];
} process;

// PROCESS COUNTER
int processCounter = 0;

// MAX PROCESS STORED CAN BE 50
process p[MAX_PROCESS];

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

static void sigchld_handler (int signo)
{
    if(signo == SIGCHLD)
    {
        write(STDOUT_FILENO, "SIGCHLD Caught\n", sizeof("SIGCHLD Caught\n"));
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

// RETURNS THE LIST OF ALL ACTIVE AND ALL KILLED PROCESS
void listAll(int fd)
{
    if(processCounter == 0)
    {
        write(fd, "Process List is Empty\n", sizeof("Process List is Empty\n"));
        return;
    }

    char string[50]; // storage for sprintf
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

void add(int fd, char* token)
{
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
        write(fd, res, buff_size);
    }
    return;
}

void subtract(int fd, char* token)
{
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
        write(fd, res, buff_size);
    }
    return;
}

void multiply(int fd, char* token)
{
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
        write(fd, res, buff_size);
    }
    return;
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

void quit(int fd)
{
    write(fd, "Not allowed!\n", sizeof("Not allowed!\n"));
    sleep(1);
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

void parser(int fd)
{
    lineCount = read(fd, line, LINE_READ_SIZE); // read from client fd
    if(lineCount == 1) // a workaround to prevent segmentation fault
    {
        write(fd, "No command entered\n", sizeof("No command entered\n"));
        return;
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
        return;
    }

    // If exit
    if(strcmp(token, "exit") == 0)
    {
        quit(fd);
        return;
    }

    // If add
    if(strcmp(token, "add") == 0)
    {
        add(fd, token);
        return;
    }

    // If sub
    if(strcmp(token, "sub") == 0)
    {
        subtract(fd, token);
        return;
    }

    // If mult
    if(strcmp(token, "mult") == 0)
    {
        multiply(fd, token);
        return;
    }

    // If list all/list
    if(strcmp(token, "list") == 0)
    {
        parseList(fd, token);
        return;
    }

    // If kill
    if(strcmp(token, "kill") == 0)
    {
        parseKill(fd, token);
        return;
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
                            "----------------------------------------------------------------------------------------\n";

        write(fd, helpOutput, sizeof(helpOutput));
        return;
    }

    write(fd, "Invalid Command\n", sizeof("Invalid Command\n")); // if all else fails

}

// THREAD
void *mainServerTerminalReader()
{
    write(STDOUT_FILENO, "``Server Started``\n", sizeof("``Server Started``\n"));
    while(write(STDOUT_FILENO, ">>", sizeof(">>")))
    {
        char *terminalLineBuffer[LINE_READ_SIZE]; // for reading from terminal
        int terminalLineCount = read(STDIN_FILENO, &terminalLineBuffer, LINE_READ_SIZE);  
        if(terminalLineCount < 0)
        {
            perror("Server Terminal Read Error");
            exit(0);
        }
        write(STDOUT_FILENO, terminalLineBuffer, terminalLineCount);
    }
    pthread_exit(0);
}

// MAIN FUNCTION
int main(int argc, char const *argv[])
{
    int sock = initServer();
    int cli;
    int status;
    pid_t pid;
    int len = sizeof(struct sockaddr_in);
    char clientFD[50];
    char inputFromClient[50];
    struct sockaddr_in client;    
    pthread_t serverThread; // thread ID

    int threadReturnValue = pthread_create(&serverThread, NULL, mainServerTerminalReader, NULL); // create a thread task

    while(1)
    {
        if((cli = accept(sock, (struct sockaddr *)&client, &len)) < 0)
        {
            perror("Accept Error");
            exit(1);
        }

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
                parser(cli);          
            }
            
            // default:
            // waitpid(pid, &status, WNOHANG);
            // if(WIFEXITED(status))
            // {
            //     exit(0);
            // }
        }
    }

    if(threadReturnValue < 0)
    {
        perror("Thread Error");
    }

    pthread_join(serverThread, NULL); // waits until a thread is complete so that the program doesn't exit before thread execution
        
    return 0;
}