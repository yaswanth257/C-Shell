#define _POSIX_C_SOURCE 200809L

#include "executor.h"
#include "intrinsics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
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
void execute_two_command_pipeline(char **argv1, char **argv2)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        setpgid(0, 0);
        execvp(argv1[0], argv1);
        printf("Command not found!\n");
        _exit(127);
    }
    else if (pid1 < 0)
    {
        perror("fork failed");
        return;
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        setpgid(0, 0);
        execvp(argv2[0], argv2);
        printf("Command not found!\n");
        _exit(127);
    }
    else if (pid2 < 0)
    {
        perror("fork failed");
        return;
    }

    setpgid(pid1, pid1);
    setpgid(pid2, pid2);

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void execute_reveal_in_pipeline(int argc, char **argv, const char *home_dir,
                                const char *input_file, const char *output_file, int append)
{
    char *reveal_argv[100];
    int reveal_argc = 0;
    int pipe_pos = -1;

    for (int i = 0; i < argc; i++)
    {
        if (argv[i] && strcmp(argv[i], "|") == 0)
        {
            pipe_pos = i;
            break;
        }
        if (argv[i] && argv[i][0] != '\0')
        {
            reveal_argv[reveal_argc++] = argv[i];
        }
    }
    reveal_argv[reveal_argc] = NULL;
    char **pipe_cmd = &argv[pipe_pos + 1];

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        close(pipefd[0]);

        if (input_file)
        {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0)
            {
                perror("open input_file");
                _exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        setpgid(0, 0);

        reveal_command(reveal_argc, reveal_argv, home_dir);
        _exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        if (output_file)
        {
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            int fd = open(output_file, flags, 0666);
            if (fd < 0)
            {
                perror("open output_file");
                _exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        setpgid(0, 0);
        execvp(pipe_cmd[0], pipe_cmd);
        printf("Command not found!\n");
        _exit(127);
    }

    setpgid(pid1, pid1);
    setpgid(pid2, pid2);

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void execute_multi_command_pipeline(
    char ***commands,
    int count,
    const char *input_file,
    const char *output_file,
    int append)
{
    int pipefd[2];
    int fd_in = 0;
    pid_t pids[count];

    for (int i = 0; i < count; ++i)
    {
        if (i < count - 1)
        {
            if (pipe(pipefd) == -1)
            {
                perror("pipe failed");
                return;
            }
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            if (i == 0 && input_file)
            {
                int fd = open(input_file, O_RDONLY);
                if (fd < 0)
                {
                    perror("open input_file");
                    _exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            else if (fd_in != 0)
            {
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            if (i == count - 1 && output_file)
            {
                int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
                int fd = open(output_file, flags, 0666);
                if (fd < 0)
                {
                    perror("open output_file");
                    _exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            else if (i < count - 1)
            {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }

            setpgid(0, 0);
            execvp(commands[i][0], commands[i]);
            printf("Command not found!\n");
            _exit(127);
        }
        else if (pid < 0)
        {
            perror("fork failed");
            return;
        }

        pids[i] = pid;
        setpgid(pid, pid);

        if (fd_in != 0)
            close(fd_in);

        if (i < count - 1)
        {
            close(pipefd[1]);
            fd_in = pipefd[0];
        }
    }

    for (int i = 0; i < count; ++i)
    {
        waitpid(pids[i], NULL, 0);
    }
}
