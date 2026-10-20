#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "prompt.h"
#include "parser.h"
#include "intrinsics.h"

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

int main(void)
{
    char home_dir[PATH_MAX + 1];

    if (getcwd(home_dir, sizeof(home_dir)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    set_shell_home_dir(home_dir);

    while (1)
    {
        print_prompt(home_dir);

        char input[1024];
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
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
            if (argc > 0 && (strcmp(argv[0], "hop") == 0 || strcmp(argv[0], "reveal") == 0))
            {
                /* allow these even if parser flags syntax edge-cases */
            }
            else
            {
                printf("Invalid Syntax!\n");
                continue;
            }
        }

        char input_copy2[1024];
        strncpy(input_copy2, input, sizeof(input_copy2) - 1);
        input_copy2[1023] = '\0';

        char *argv[100];
        int argc = tokenize_input_to_argv(input_copy2, argv, 100);
        if (argc == 0)
            continue;

        if (strcmp(argv[0], "hop") == 0)
        {
            hop_command(argc, argv, home_dir);
        }
        else if (strcmp(argv[0], "reveal") == 0)
        {
            reveal_command(argc, argv, home_dir);
        }
        else
        {
            printf("%s: command not found\n", argv[0]);
        }
    }

    return 0;
}
