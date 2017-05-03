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
char *ptr; // int pointer for add/mult
int result; // stores result for add/mult
int err = 0; // stores error for add/mult
char res[LINE_READ_SIZE]; // stores result in string
int buff_size; // storing size from sprintf
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
void killWithPID(int pid)
{
    char message[50];
    int count;
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
                write(STDOUT_FILENO, message, count);
            }
            else
            {
                write(STDOUT_FILENO, "\nProcess already Inactive\n\n", sizeof("\nProcess already Inactive\n\n"));
            }
            break;
        }
    }
}

// KILL WITH PROCESS Name
void killWithName(char *name)
{
    char processName[50];
    char message[50]; // Message to print on screen
    int count; // Stores sprintf
    int inactiveCounter; // 

    count = sprintf(processName, "%s\n", name);

    for (int i = 0; i < processCounter; i++)
    {
        if(strcmp(p[i].name, processName) == 0)
        {
            if((strcmp(p[i].status, "Active") == 0))
            {
                pid = p[i].pid;
                kill(pid, SIGTERM);
                sprintf(p[i].stopTime, "%s", currentSystemDateTime);
                sprintf(p[i].status, "%s", "Inactive");
                count = sprintf(message, "~~~Killed Process~~~\n   Name : %s    PID : %d\n", processName, pid);
                write(STDOUT_FILENO, message, count);
                break;
            }
            if((strcmp(p[i].status, "Inactive") == 0))
            {
                inactiveCounter++;
            }
            count = sprintf(message, "\n%d Processe(s) are already inactive\n\n", inactiveCounter);
            write(STDOUT_FILENO, message, count);
            
        }
    }
}

//KILL ALL PROCESS WITH SAME NAME
void killAllWithName(char *name)
{
    char processName[50];
    char message[100];
    int count;
    count = sprintf(processName, "%s\n", name);
    for (int i = 0; i < processCounter; i++)
    {
        if(strcmp(p[i].name, processName) == 0)
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
    count = sprintf(message, "~~~Killed all instances of Process~~~\n   Name : %s\n", processName);
    write(STDOUT_FILENO, message, count);
}

// RUNS A PROGRAM
void run(char program[])
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
void listAll()
{
    if(processCounter == 0)
    {
        write(STDOUT_FILENO, "Process List is Empty\n", sizeof("Process List is Empty\n"));
        return;
    }

    char string[50]; // storage for sprintf
    int stringCount; // Count value for sprintf

    for (int i = 0; i < processCounter; i++)
    {
        write(STDOUT_FILENO, "-------------------------------------------------------\n", sizeof("-------------------------------------------------------\n"));
        stringCount = sprintf(string, "Name : %s\n", p[i].name);
        write(STDOUT_FILENO, string, stringCount);
        stringCount = sprintf(string, "PID : %d\n", p[i].pid);
        write(STDOUT_FILENO, string, stringCount);
        stringCount = sprintf(string, "Start Time : %s", p[i].startTime);
        write(STDOUT_FILENO, string, stringCount);
        stringCount = sprintf(string, "Stop Time : %s", p[i].stopTime);
        write(STDOUT_FILENO, string, stringCount);
        stringCount = sprintf(string, "Status : %s\n", p[i].status);
        write(STDOUT_FILENO, string, stringCount);
        write(STDOUT_FILENO, "-------------------------------------------------------\n", sizeof("-------------------------------------------------------\n"));
    }
}

// RETURNS THE LIST OF ALL ACTIVE PROCESS
void listActive()
{
    if(processCounter == 0)
    {
        write(STDOUT_FILENO, "Process List is Empty\n", sizeof("Process List is Empty\n"));
        return;
    }

    char string[50]; // storage for sprintf
    int stringCount; // Count value for sprintf

    for (int i = 0; i < processCounter; i++)
    {
        if(strcmp(p[i].status, "Active") == 0)
        {
            write(STDOUT_FILENO, "-------------------------------------------------------\n", sizeof("-------------------------------------------------------\n"));
            stringCount = sprintf(string, "Name : %s\n", p[i].name);
            write(STDOUT_FILENO, string, stringCount);
            stringCount = sprintf(string, "PID : %d\n", p[i].pid);
            write(STDOUT_FILENO, string, stringCount);
            stringCount = sprintf(string, "Start Time : %s", p[i].startTime);
            write(STDOUT_FILENO, string, stringCount);
            stringCount = sprintf(string, "Status : %s\n", p[i].status);
            write(STDOUT_FILENO, string, stringCount);
            write(STDOUT_FILENO, "-------------------------------------------------------\n", sizeof("-------------------------------------------------------\n"));
        }
    }
}

// MAIN FUNCTION
int main(int argc, char const *argv[])
{
    int parentfd = atoi(argv[1]);  
    while (1)
    {
        lineCount = read(parentfd, line, LINE_READ_SIZE); // read from parentfd
        if(lineCount == 1) // a workaround to prevent segmentation fault
        {
            continue;
        }
        line[lineCount - 1] = '\0'; // add a null character at the end
        token = strtok(line, s); // initial tokenization

        // If run
        if(strcmp(token, "run") == 0)
        {
            token = strtok(NULL, s);
            if(token != NULL)
            {
                run(token);
            }
            else
            {
                write(STDOUT_FILENO, "run <process name>\n", sizeof("run <process name>\n"));
                continue;
            }
        }

        // If exit
        if(strcmp(token, "exit") == 0)
        {
            write(STDOUT_FILENO, "Exiting...\n", sizeof("Exiting...\n"));
            sleep(1);
            exit(0);
        }

        // If add
        if(strcmp(token, "add") == 0)
        {
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
            }
            continue;
        }

        // If sub
        if(strcmp(token, "sub") == 0)
        {
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
            }
            continue;
        }

        // If mult
        if(strcmp(token, "mult") == 0)
        {
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
            }
            continue;
        }

        // If list all/list
        if(strcmp(token, "list") == 0)
        {
            token = strtok(NULL, s);
            if(token != NULL)
            {            
                if(strcmp(token, "all") == 0)
                {
                    listAll();
                }
            }
            else
            {
                listActive();
            }
            continue;
        }

        // If kill
        if(strcmp(token, "kill") == 0)
        {
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
                        killAllWithName(ptr);
                    }
                }
                else if(pid > 0)
                {
                    killWithPID(pid);
                }
                else if(pid == 0)
                {
                    killWithName(ptr);
                }
            }
            else
            {
                write(STDOUT_FILENO, "kill <pid>\n", sizeof("kill <pid>\n"));
                write(STDOUT_FILENO, "kill <process name>\n", sizeof("kill <process name>\n"));
                write(STDOUT_FILENO, "kill <process name> all\n", sizeof("kill <process name> all\n"));
            }
        }

        // If help
        if(strcmp(line, "help") == 0)
        {
            puts("-------------------------------------------------------------------------");
            puts("'run <process name>' to execute a program");
            puts("'kill <process PID>' to kill a process by PID");
            puts("'kill <process name>' to kill a process by name");
            puts("'kill <process name> all' to kill all process");
            puts("'add <integer1> <integer2> ... <integerN>' to add multiple numbers");
            puts("'sub <integer1> <integer2> ... <integerN>' to subtract multiple numbers");
            puts("'mult <integer1> <integer2> ... <integerN>' to multiply multiple numbers");
            puts("'exit' to quit now!");
            puts("-------------------------------------------------------------------------");
            continue;
        }

    }
        
    return 0;
}