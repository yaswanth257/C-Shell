#define _POSIX_C_SOURCE 200809L

#include "bg_jobs.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

extern volatile sig_atomic_t sigchld_received;

bg_job bg_jobs[MAX_BG_JOBS];
int bg_job_count = 0;
int next_job_number = 1;
pid_t fg_pid = -1;

void bg_job_add(pid_t pid, const char *cmdline, const char *state)
{
    if (bg_job_count >= MAX_BG_JOBS)
        return;

    bg_jobs[bg_job_count].pid = pid;
    bg_jobs[bg_job_count].job_number = next_job_number++;
    strncpy(bg_jobs[bg_job_count].command_line, cmdline, MAX_CMD_LEN);
    bg_jobs[bg_job_count].command_line[MAX_CMD_LEN - 1] = '\0';
    strncpy(bg_jobs[bg_job_count].state, state, sizeof(bg_jobs[bg_job_count].state));
    bg_jobs[bg_job_count].state[sizeof(bg_jobs[bg_job_count].state) - 1] = '\0';
    bg_job_count++;
}

void bg_job_remove(int idx)
{
    if (idx < 0 || idx >= bg_job_count)
        return;
    for (int i = idx; i < bg_job_count - 1; i++)
    {
        bg_jobs[i] = bg_jobs[i + 1];
    }
    bg_job_count--;
}

int find_job_by_number(int job_number)
{
    for (int i = 0; i < bg_job_count; i++)
    {
        if (bg_jobs[i].job_number == job_number)
            return i;
    }
    return -1;
}

int find_job_by_pid(pid_t pid)
{
    for (int i = 0; i < bg_job_count; i++)
    {
        if (bg_jobs[i].pid == pid)
            return i;
    }
    return -1;
}
// ############## LLM Generated Code Begins ##############
void check_bg_jobs(void)
{
    sigchld_received = 0;

    int status;
    for (int i = 0; i < bg_job_count;)
    {
        pid_t r = waitpid(bg_jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (r == 0)
        {
            ++i;
            continue;
        }
        if (r == -1)
        {
            bg_job_remove(i);
            continue;
        }
        if (WIFEXITED(status))
        {
            char cmd_name[256];
            sscanf(bg_jobs[i].command_line, "%255s", cmd_name);

            if (WEXITSTATUS(status) == 0)
                printf("%s with pid %d exited normally\n", cmd_name, bg_jobs[i].pid);
            else
                printf("%s with pid %d exited abnormally\n", cmd_name, bg_jobs[i].pid);
            fflush(stdout);
            bg_job_remove(i);
            continue;
        }
        else if (WIFSIGNALED(status))
        {
            char cmd_name[256];
            sscanf(bg_jobs[i].command_line, "%255s", cmd_name);
            printf("%s with pid %d exited abnormally\n", cmd_name, bg_jobs[i].pid);
            fflush(stdout);
            bg_job_remove(i);
            continue;
        }
        else if (WIFSTOPPED(status))
        {
            strncpy(bg_jobs[i].state, "Stopped", sizeof(bg_jobs[i].state));
            ++i;
        }
        else if (WIFCONTINUED(status))
        {
            strncpy(bg_jobs[i].state, "Running", sizeof(bg_jobs[i].state));
            ++i;
        }
        else
        {
            ++i;
        }
    }
}
// ############## LLM Generated Code Ends ################
