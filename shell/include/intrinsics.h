#ifndef INTRINSICS_H
#define INTRINSICS_H

#define MAX_CMD_LEN 1024

void hop_command(int argc, char *argv[], const char *home_dir);
void reveal_command(int argc, char *argv[], const char *home_dir);
void log_command(int argc, char *argv[]);
void intrinsics_log_store_command(const char *cmd);
void set_shell_home_dir(const char *home_dir);

const char *get_previous_dir(void);
int has_previous_dir(void);

void run_shell_command(char *cmd, const char *home_dir);

#endif
