#define _POSIX_C_SOURCE 200809L

#include "intrinsics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LOG_CMDS 15
#define MAX_CMD_LEN 1024

int tokenize_input_to_argv(char *input, char *argv[], int max_args);
void run_shell_command(char *cmd, const char *home_dir);

static char previous_dir[PATH_MAX] = "";
static int has_previous = 0;

static char command_log[MAX_LOG_CMDS][MAX_CMD_LEN];
static int log_count = 0;
static int log_start = 0;
static char last_command[MAX_CMD_LEN] = "";
static char log_file_path[PATH_MAX] = "";
static int log_initialized = 0;
static char shell_home_dir[PATH_MAX] = "";

static void init_log_file(void)
{
    if (log_initialized) return;

    const char *home = getenv("HOME");
    if (home) {
        snprintf(log_file_path, sizeof(log_file_path), "%s/.shell_history", home);
    } else {
        strcpy(log_file_path, ".shell_history");
    }

    // Load existing history
    FILE *file = fopen(log_file_path, "r");
    if (file) {
        char line[MAX_CMD_LEN];
        while (fgets(line, sizeof(line), file) && log_count < MAX_LOG_CMDS) {
            // Remove newline
            line[strcspn(line, "\n")] = '\0';
            if (strlen(line) > 0) {
                strncpy(command_log[(log_start + log_count) % MAX_LOG_CMDS], line, MAX_CMD_LEN);
                command_log[(log_start + log_count) % MAX_LOG_CMDS][MAX_CMD_LEN - 1] = '\0';
                if (log_count < MAX_LOG_CMDS)
                    log_count++;
                else
                    log_start = (log_start + 1) % MAX_LOG_CMDS;
            }
        }
        fclose(file);
    }
    log_initialized = 1;
}

static void save_log_to_file(void)
{
    if (!log_initialized) return;

    FILE *file = fopen(log_file_path, "w");
    if (file) {
        for (int i = 0; i < log_count; i++) {
            int real_index = (log_start + i) % MAX_LOG_CMDS;
            fprintf(file, "%s\n", command_log[real_index]);
        }
        fclose(file);
    }
}

static void update_previous_dir(const char *dir)
{
    if (dir)
    {
        strncpy(previous_dir, dir, PATH_MAX);
        previous_dir[PATH_MAX - 1] = '\0';
        has_previous = 1;
    }
    else
    {
        previous_dir[0] = '\0';
        has_previous = 0;
    }
}

const char *get_previous_dir(void)
{
    return previous_dir;
}

int has_previous_dir(void)
{
    return has_previous;
}

static void log_command_store(const char *cmd)
{
    init_log_file();

    if (cmd == NULL || strlen(cmd) == 0)
        return;
    if (strncmp(cmd, "log", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' '))
        return;
    if (strcmp(cmd, last_command) == 0)
        return;

    strncpy(command_log[(log_start + log_count) % MAX_LOG_CMDS], cmd, MAX_CMD_LEN);
    command_log[(log_start + log_count) % MAX_LOG_CMDS][MAX_CMD_LEN - 1] = '\0';

    if (log_count < MAX_LOG_CMDS)
        log_count++;
    else
        log_start = (log_start + 1) % MAX_LOG_CMDS;

    strncpy(last_command, cmd, MAX_CMD_LEN);
    last_command[MAX_CMD_LEN - 1] = '\0';

    save_log_to_file();
}

static int safe_chdir(const char *new_dir)
{
    char current_dir[PATH_MAX];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        perror("getcwd failed");
        return -1;
    }
    if (chdir(new_dir) != 0)
    {
        printf("No such directory!\n");
        return -1;
    }
    update_previous_dir(current_dir);
    return 0;
}

void hop_command(int argc, char *argv[], const char *home_dir)
{
    if (argc == 1)
    {
        if (chdir(home_dir) != 0)
        {
            printf("No such directory!\n");
        }
        else
        {
            update_previous_dir(NULL);
        }
        return;
    }

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "~") == 0)
        {
            if (chdir(home_dir) != 0)
            {
                printf("No such directory!\n");
            }
            else
            {
                update_previous_dir(NULL);
            }
        }
        else if (strcmp(argv[i], ".") == 0)
        {
            continue;
        }
        else if (strcmp(argv[i], "..") == 0)
        {
            safe_chdir("..");
        }
        else if (strcmp(argv[i], "-") == 0)
        {
            if (has_previous)
            {
                char current_dir[PATH_MAX];
                if (getcwd(current_dir, sizeof(current_dir)) == NULL)
                {
                    perror("getcwd");
                    continue;
                }
                if (chdir(previous_dir) == 0)
                {
                    update_previous_dir(current_dir);
                }
                else
                {
                    printf("No such directory!\n");
                }
            }
        }
        else
        {
            safe_chdir(argv[i]);
        }
    }
}

static int compare_strings(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

void reveal_command(int argc, char *argv[], const char *home_dir)
{
    int flag_a = 0, flag_l = 0;
    char *dir_arg = NULL;
    int arg_count = 0;

    if (argc == 1)
    {
        dir_arg = NULL;
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            char *arg = argv[i];
            if (arg == NULL) continue;
            if (strcmp(arg, "-") == 0)
            {
                if (arg_count > 0)
                {
                    printf("reveal: Invalid Syntax!\n");
                    return;
                }
                dir_arg = arg;
                arg_count++;
            }
            else if (arg[0] == '-')
            {
                for (size_t j = 1; arg[j] != '\0'; j++)
                {
                    if (arg[j] == 'a')
                        flag_a = 1;
                    else if (arg[j] == 'l')
                        flag_l = 1;
                }
            }
            else
            {
                if (arg_count > 0)
                {
                    printf("reveal: Invalid Syntax!\n");
                    return;
                }
                dir_arg = arg;
                arg_count++;
            }
        }
    }

    char resolved_path[PATH_MAX];

    if (dir_arg == NULL)
    {
        if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
        {
            printf("No such directory!\n");
            return;
        }
    }
    else if (strcmp(dir_arg, "~") == 0)
    {
        strncpy(resolved_path, home_dir, PATH_MAX);
        resolved_path[PATH_MAX - 1] = '\0';
    }
    else if (strcmp(dir_arg, ".") == 0)
    {
        if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
        {
            printf("No such directory!\n");
            return;
        }
    }
    else if (strcmp(dir_arg, "..") == 0)
    {
        if (getcwd(resolved_path, sizeof(resolved_path)) == NULL)
        {
            printf("No such directory!\n");
            return;
        }
        char *last_slash = strrchr(resolved_path, '/');
        if (last_slash != resolved_path && last_slash != NULL)
        {
            *last_slash = '\0';
        }
        else
        {
            resolved_path[0] = '/';
            resolved_path[1] = '\0';
        }
    }
    else if (strcmp(dir_arg, "-") == 0)
    {
        if (!has_previous)
        {
            printf("No such directory!\n");
            return;
        }
        strncpy(resolved_path, previous_dir, PATH_MAX);
        resolved_path[PATH_MAX - 1] = '\0';
    }
    else
    {
        strncpy(resolved_path, dir_arg, PATH_MAX);
        resolved_path[PATH_MAX - 1] = '\0';
    }

    DIR *d = opendir(resolved_path);
    if (!d)
    {
        printf("No such directory!\n");
        return;
    }

    char *entries[1024];
    size_t count = 0;
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL && count < 1024)
    {
        if (!flag_a && entry->d_name[0] == '.')
            continue;

        entries[count] = malloc(strlen(entry->d_name) + 1);
        if (entries[count] == NULL)
        {
            perror("malloc");
            closedir(d);
            for (size_t j = 0; j < count; j++)
                free(entries[j]);
            return;
        }
        strcpy(entries[count], entry->d_name);
        count++;
    }
    closedir(d);

    qsort(entries, count, sizeof(char *), compare_strings);

    if (flag_l)
    {
        for (size_t i = 0; i < count; i++)
        {
            printf("%s\n", entries[i]);
            free(entries[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < count; i++)
        {
            printf("%s", entries[i]);
            if (i + 1 < count)
                printf(" ");
            free(entries[i]);
        }
        if (count > 0)
            printf("\n");
    }
    fflush(stdout);
}

void log_command(int argc, char *argv[])
{
    init_log_file();

    if (argc == 1)
    {
        for (int i = 0; i < log_count; i++)
        {
            int real_index = (log_start + i) % MAX_LOG_CMDS;
            printf("%s\n", command_log[real_index]);
        }
        return;
    }

    if (argc == 2)
    {
        if (strcmp(argv[1], "purge") == 0)
        {
            log_start = 0;
            log_count = 0;
            last_command[0] = '\0';
            save_log_to_file();
            return;
        }
        printf("log: Invalid Syntax!\n");
        return;
    }

    if (argc == 3 && strcmp(argv[1], "execute") == 0)
    {
        int idx = atoi(argv[2]);
        if (idx < 1 || idx > log_count)
        {
            printf("Invalid Syntax!\n");
            return;
        }
        int real_idx = (log_count - idx + log_start) % MAX_LOG_CMDS;
        char cmd_to_execute[MAX_CMD_LEN];
        strncpy(cmd_to_execute, command_log[real_idx], MAX_CMD_LEN - 1);
        cmd_to_execute[MAX_CMD_LEN - 1] = '\0';

        const char *home_to_use = (strlen(shell_home_dir) > 0) ? shell_home_dir : getenv("HOME");
        run_shell_command(cmd_to_execute, home_to_use);
        return;
    }

    printf("log: Invalid Syntax!\n");
}

void set_shell_home_dir(const char *home_dir)
{
    if (home_dir) {
        strncpy(shell_home_dir, home_dir, PATH_MAX - 1);
        shell_home_dir[PATH_MAX - 1] = '\0';
    }
}

void intrinsics_log_store_command(const char *cmd)
{
    log_command_store(cmd);
}
