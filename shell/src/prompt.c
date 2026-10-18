#define _POSIX_C_SOURCE 200809L

#include "prompt.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

void print_prompt(const char *home_dir)
{
    char hostname[HOST_NAME_MAX + 1];
    char cwd[PATH_MAX + 1];
    char *user;

    user = getlogin();
    if (!user)
        user = "unknown";

    if (gethostname(hostname, sizeof(hostname)) != 0)
        strcpy(hostname, "unknown");

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strcpy(cwd, "?");

    size_t home_len = strlen(home_dir);
    char disp[PATH_MAX + 1];

    if (strncmp(cwd, home_dir, home_len) == 0)
    {
        if (cwd[home_len] == '\0')
            snprintf(disp, sizeof(disp), "~");
        else if (cwd[home_len] == '/')
            snprintf(disp, sizeof(disp), "~%s", cwd + home_len);
        else
            snprintf(disp, sizeof(disp), "%s", cwd);
    }
    else
    {
        snprintf(disp, sizeof(disp), "%s", cwd);
    }

    printf("<%s@%s:%s> ", user, hostname, disp);
    fflush(stdout);
}

