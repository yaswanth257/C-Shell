#define _POSIX_C_SOURCE 200809L

#include "executor.h"
#include "intrinsics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
// ############## LLM Generated Code Begins ##############
pid_t execute_external_command(char **argv, const char *input_file, const char *output_file, int append)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        if (input_file)
        {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0)
            {
                printf("No such file or directory\n");
                _exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (output_file)
        {
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            int fd = open(output_file, flags, 0666);
            if (fd < 0)
            {
                printf("Unable to create file for writing\n");
                _exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        setpgid(0, 0);

        execvp(argv[0], argv);
        printf("Command not found!\n");
        fflush(stdout);
        _exit(127);
    }
    else if (pid > 0)
    {
        setpgid(pid, pid);
        return pid;
    }
    else
    {
        perror("fork failed");
        return -1;
    }
}
// ############## LLM Generated Code Ends ################

/* Pipeline functions will be added in the next commit. */
void execute_two_command_pipeline(char **argv1, char **argv2)
{
    (void)argv1;
    (void)argv2;
}

void execute_reveal_in_pipeline(int argc, char **argv, const char *home_dir,
                                const char *input_file, const char *output_file, int append)
{
    (void)argc; (void)argv; (void)home_dir;
    (void)input_file; (void)output_file; (void)append;
}

void execute_multi_command_pipeline(
    char ***commands,
    int count,
    const char *input_file,
    const char *output_file,
    int append)
{
    (void)commands; (void)count;
    (void)input_file; (void)output_file; (void)append;
}
