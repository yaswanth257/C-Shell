#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "intrinsics.h"
#include "prompt.h"
#include "parser.h"
#include "executor.h"
#include "signals.h"
#include "bg_jobs.h"

int tokenize_input_to_argv(char *input, char *argv[], int max_args)
{
    int argc = 0;
    char *token = strtok(input, " \t\r\n");
    while (token && argc < max_args - 1)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    argv[argc] = NULL;
    return argc;
}

void run_shell_command(char *cmd, const char *home_dir);
void run_shell_command(char *cmd, const char *home_dir)
{
    char input_copy[MAX_CMD_LEN];
    strncpy(input_copy, cmd, MAX_CMD_LEN);
    input_copy[MAX_CMD_LEN - 1] = '\0';

    char *argv[100];
    int argc = tokenize_input_to_argv(input_copy, argv, 100);
    if (argc == 0)
        return;

    int background = 0;
    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0)
    {
        background = 1;
        argv[--argc] = NULL;
    }

    for (int i = 0; argv[i]; ++i)
    {
        if (strcmp(argv[i], "<") == 0)
        {
            if (!argv[i + 1] || access(argv[i + 1], R_OK) != 0)
            {
                printf("No such file or directory\n");
                return;
            }
            i++;
        }
    }

    for (int i = 0; argv[i]; ++i)
    {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0)
        {
            if (!argv[i + 1])
            {
                printf("Invalid output redirection\n");
                return;
            }
            int flags = O_WRONLY | O_CREAT | (strcmp(argv[i], ">>") == 0 ? O_APPEND : O_TRUNC);
            int fd = open(argv[i + 1], flags, 0666);
            if (fd < 0)
            {
                printf("Unable to create file for writing\n");
                return;
            }
            close(fd);
            i++;
        }
    }

    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;

    for (int i = 0; argv[i]; ++i)
    {
        if (strcmp(argv[i], "<") == 0)
        {
            input_file = argv[i + 1];
            argv[i] = NULL;
            argv[i + 1] = NULL;
            i++;
        }
        else if (strcmp(argv[i], ">>") == 0)
        {
            output_file = argv[i + 1];
            append = 1;
            argv[i] = NULL;
            argv[i + 1] = NULL;
            i++;
        }
        else if (strcmp(argv[i], ">") == 0)
        {
            output_file = argv[i + 1];
            append = 0;
            argv[i] = NULL;
            argv[i + 1] = NULL;
            i++;
        }
    }

    int has_pipe = 0;
    for (int i = 0; argv[i]; ++i)
    {
        if (strcmp(argv[i], "|") == 0)
        {
            has_pipe = 1;
            break;
        }
    }

    if (has_pipe && strcmp(argv[0], "reveal") == 0)
    {
        execute_reveal_in_pipeline(argc, argv, home_dir, input_file, output_file, append);
        intrinsics_log_store_command(cmd);
        return;
    }

    if (strcmp(argv[0], "hop") == 0)
    {
        hop_command(argc, argv, home_dir);
        intrinsics_log_store_command(cmd);
        return;
    }

    if (strcmp(argv[0], "reveal") == 0)
    {
        int saved_stdin = -1, saved_stdout = -1;

        if (input_file)
        {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0)
            {
                printf("No such file or directory\n");
                return;
            }
            saved_stdin = dup(STDIN_FILENO);
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
                if (saved_stdin >= 0)
                {
                    dup2(saved_stdin, STDIN_FILENO);
                    close(saved_stdin);
                }
                return;
            }
            saved_stdout = dup(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        reveal_command(argc, argv, home_dir);
        fflush(stdout);

        if (saved_stdin >= 0)
        {
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdin);
        }
        if (saved_stdout >= 0)
        {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }

        intrinsics_log_store_command(cmd);
        return;
    }

    if (strcmp(argv[0], "log") == 0)
    {
        log_command(argc, argv);
        return;
    }

    if (strcmp(argv[0], "activities") == 0)
    {
        check_bg_jobs();
        for (int i = 0; i < bg_job_count; ++i)
            for (int j = i + 1; j < bg_job_count; ++j)
            {
                char cmd1[256], cmd2[256];
                sscanf(bg_jobs[i].command_line, "%255s", cmd1);
                sscanf(bg_jobs[j].command_line, "%255s", cmd2);

                if (strcmp(cmd1, cmd2) > 0)
                {
                    bg_job tmp = bg_jobs[i];
                    bg_jobs[i] = bg_jobs[j];
                    bg_jobs[j] = tmp;
                }
            }
        for (int i = 0; i < bg_job_count; ++i)
            printf("[%d] : %s - %s\n", (int)bg_jobs[i].pid, bg_jobs[i].command_line, bg_jobs[i].state);
        return;
    }

    if (strcmp(argv[0], "ping") == 0)
    {
        if (argc != 3)
        {
            printf("Invalid syntax!\n");
            return;
        }

        char *endptr;
        long pid_val = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0' || pid_val <= 0)
        {
            printf("Invalid syntax!\n");
            return;
        }

        long sig_val = strtol(argv[2], &endptr, 10);
        if (*endptr != '\0')
        {
            printf("Invalid syntax!\n");
            return;
        }

        pid_t pid = (pid_t)pid_val;
        int sig = (int)sig_val;

        if (kill(pid, 0) < 0)
        {
            printf("No such process found\n");
            return;
        }
        if (kill(pid, sig % 32) < 0)
        {
            printf("No such process found\n");
            return;
        }
        printf("Sent signal %d to process with pid %d\n", sig, (int)pid);
        return;
    }

    if (strcmp(argv[0], "fg") == 0)
    {
        int jobnum = (argc >= 2) ? atoi(argv[1]) : -1;
        int idx = (jobnum == -1) ? (bg_job_count - 1) : find_job_by_number(jobnum);
        if (idx < 0 || idx >= bg_job_count)
        {
            printf("No such job\n");
            return;
        }
        printf("%s\n", bg_jobs[idx].command_line);
        if (strcmp(bg_jobs[idx].state, "Stopped") == 0)
            kill(bg_jobs[idx].pid, SIGCONT);

        pid_t job_pid = bg_jobs[idx].pid;
        bg_job_remove(idx);

        fg_pid = job_pid;
        int status;
        waitpid(fg_pid, &status, WUNTRACED);

        if (WIFSTOPPED(status))
        {
            char cmd_name[256];
            sscanf(cmd, "%255s", cmd_name);
            printf("\n[%d] Stopped %s\n", next_job_number, cmd_name);
            fflush(stdout);
            bg_job_add(fg_pid, cmd, "Stopped");
            if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0)
            {
                perror("tcsetpgrp");
            }
        }

        fg_pid = -1;
        return;
    }

    if (strcmp(argv[0], "bg") == 0)
    {
        int jobnum = (argc >= 2) ? atoi(argv[1]) : -1;
        int idx = (jobnum == -1) ? (bg_job_count - 1) : find_job_by_number(jobnum);
        if (idx < 0 || idx >= bg_job_count)
        {
            printf("No such job\n");
            return;
        }
        if (strcmp(bg_jobs[idx].state, "Running") == 0)
        {
            printf("Job already running\n");
            return;
        }
        kill(bg_jobs[idx].pid, SIGCONT);
        strncpy(bg_jobs[idx].state, "Running", sizeof(bg_jobs[idx].state));
        printf("[%d] %s &\n", bg_jobs[idx].job_number, bg_jobs[idx].command_line);
        return;
    }

#define MAX_PIPE_COMMANDS 10
    char **cmd_ptrs[MAX_PIPE_COMMANDS];
    int pipe_positions[MAX_PIPE_COMMANDS];
    int command_count = 0;
    pipe_positions[command_count++] = 0;
    for (int i = 0; argv[i]; ++i)
    {
        if (strcmp(argv[i], "|") == 0)
        {
            argv[i] = NULL;
            pipe_positions[command_count++] = i + 1;
        }
    }

    for (int i = 0; i < command_count; ++i)
        cmd_ptrs[i] = &argv[pipe_positions[i]];

    if (command_count == 1)
    {
        pid_t child_pid = execute_external_command(argv, input_file, output_file, append);
        if (child_pid < 0)
        {
            fprintf(stderr, "Failed to execute command\n");
            return;
        }

        if (background)
        {
            bg_job_add(child_pid, cmd, "Running");
            printf("[%d] %d\n", next_job_number - 1, (int)child_pid);
            fflush(stdout);
        }
        else
        {
            fg_pid = child_pid;
            int status;
            waitpid(child_pid, &status, WUNTRACED);
            if (WIFSTOPPED(status))
            {
                char cmd_name[256];
                sscanf(cmd, "%255s", cmd_name);
                printf("\n[%d] Stopped %s\n", next_job_number, cmd_name);
                fflush(stdout);
                bg_job_add(child_pid, cmd, "Stopped");
                if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0)
                {
                    perror("tcsetpgrp");
                }
            }
            fg_pid = -1;
            intrinsics_log_store_command(cmd);
        }
    }
    else
    {
        execute_multi_command_pipeline(cmd_ptrs, command_count, input_file, output_file, append);
        intrinsics_log_store_command(cmd);
    }
}

int main(void)
{
    char home_dir[PATH_MAX + 1];
    char debugdir[PATH_MAX + 1];

    if (getcwd(debugdir, sizeof(debugdir)))
    {
        printf("[DEBUG SHELL START DIRECTORY]: %s\n", debugdir);
        fflush(stdout);
    }

    if (getcwd(home_dir, sizeof(home_dir)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    set_shell_home_dir(home_dir);
    install_signal_handlers();

    while (1)
    {

        check_bg_jobs();
        print_prompt(home_dir);

        char input[1024];
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            if (isatty(STDIN_FILENO))
            {
                for (int i = 0; i < bg_job_count; ++i)
                    if (strcmp(bg_jobs[i].state, "Running") == 0)
                        kill(bg_jobs[i].pid, SIGKILL);
                printf("logout\n");
                exit(0);
            }
            else
            {
                break;
            }
        }
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0)
            continue;

        if (parse_command(input) != 0)
        {
            char *argv[100];
            char input_copy[1024];
            strncpy(input_copy, input, sizeof(input_copy) - 1);
            input_copy[1023] = '\0';
            int argc = tokenize_input_to_argv(input_copy, argv, 100);
            if (argc > 0 && (strcmp(argv[0], "hop") == 0 || strcmp(argv[0], "reveal") == 0 || strcmp(argv[0], "log") == 0))
            {
            }
            else
            {
                printf("Invalid Syntax!\n");
                continue;
            }
        }

        char *saveptr = NULL;
        char *cmd = strtok_r(input, ";", &saveptr);
        while (cmd != NULL)
        {
            while (*cmd == ' ' || *cmd == '\t')
                ++cmd;
            char *end = cmd + strlen(cmd) - 1;
            while (end > cmd && (*end == ' ' || *end == '\t'))
                *end-- = '\0';
            if (strlen(cmd) > 0)
                run_shell_command(cmd, home_dir);
            cmd = strtok_r(NULL, ";", &saveptr);
        }
    }

    return 0;
}
