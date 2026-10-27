#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

extern pid_t fg_pid;

void install_signal_handlers(void);
void sigint_handler(int sig);
void sigtstp_handler(int sig);

#endif
