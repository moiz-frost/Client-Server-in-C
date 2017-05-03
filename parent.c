#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char const *argv[])
{
    char line[500]; // terminal input input
    int pipefd[2]; // pipe
    pid_t pid; // pid
    int status; // child status    
    int lineCount; // terminal lineCount
    char childArgument[1]; // argument to pass in child
    


    pipe(pipefd);

       
    switch (pid = fork())
    {

        case -1:
        perror("The fork failed!");
        break;
        
        case 0:
        close(pipefd[1]);
        sprintf(childArgument, "%d", pipefd[0]);
        execlp("xterm", "xterm", "-e", "./child", childArgument, (char*)0);
        
        
        default:
        close(pipefd[0]);
        while (write(STDOUT_FILENO, ">> ", sizeof(">> ")))
        {
            lineCount = read(STDIN_FILENO, line, sizeof(line));
            line[lineCount] = '\0'; // add a null character at the end
            write(pipefd[1], line, lineCount);
            if(strcasecmp(line, "-100\n") == 0)
            {
                break;
            }
        }

        waitpid(pid, &status, 0);

        if(WIFEXITED(status))
        {
            exit(0);
        }
    }



















}