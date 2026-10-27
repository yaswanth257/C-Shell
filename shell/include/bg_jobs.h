#ifndef BG_JOBS_H
#define BG_JOBS_H

#include <sys/types.h>

#define MAX_BG_JOBS 100
#define MAX_CMD_LEN 1024

typedef struct {
    pid_t pid;
    int job_number;
    char command_line[MAX_CMD_LEN];
    char state[16];
} bg_job;

extern bg_job bg_jobs[MAX_BG_JOBS];
extern int bg_job_count;
extern int next_job_number;
extern pid_t fg_pid;

void bg_job_add(pid_t pid, const char *cmdline, const char *state);
void bg_job_remove(int idx);
int find_job_by_number(int job_number);
int find_job_by_pid(pid_t pid);
void check_bg_jobs(void);

#endif
