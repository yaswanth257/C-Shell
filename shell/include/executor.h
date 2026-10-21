#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <sys/types.h>

pid_t execute_external_command(char **argv, const char *input_file, const char *output_file, int append);
void execute_two_command_pipeline(char **argv1, char **argv2);
void execute_multi_command_pipeline(
    char ***commands,
    int count,
    const char *input_file,
    const char *output_file,
    int append
);

void execute_reveal_in_pipeline(int argc, char **argv, const char *home_dir,
                                const char *input_file, const char *output_file, int append);


#endif
