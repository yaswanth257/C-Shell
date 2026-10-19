#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "prompt.h"
#include "parser.h"

int main(void)
{
    char home_dir[PATH_MAX + 1];

    if (getcwd(home_dir, sizeof(home_dir)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

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
            printf("Invalid Syntax!\n");
            continue;
        }
    }

    return 0;
}
