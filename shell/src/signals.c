#define _POSIX_C_SOURCE 200809L

#include "signals.h"
#include "bg_jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

void sigint_handler(int sig)
{
    (void)sig;
    if (fg_pid > 0)
    {
        kill(-fg_pid, SIGINT);
    }
}

void sigtstp_handler(int sig)
{
    (void)sig;
    if (fg_pid > 0)
    {
        kill(-fg_pid, SIGTSTP);
    }
}

volatile sig_atomic_t sigchld_received = 0;

void sigchld_handler(int sig)
{
    (void)sig;
    sigchld_received = 1;
}

void install_signal_handlers(void)
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    signal(SIGQUIT, SIG_IGN);
}
