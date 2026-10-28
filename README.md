# OSN C-Shell

A custom Unix-like command-line shell implemented in C as part of the Operating Systems and Networks (OSN) course.

---

## Features

### 1. Prompt & Display
- Dynamic prompt format: `<Username@SystemName:~>`
- Automatically displays current working directory relative to the shell start directory (represented as `~`).

### 2. Built-in Commands (Intrinsics)
- **`hop`**: Changes directory with support for absolute/relative paths, `~` (home), `..` (parent), `.` (current), and `-` (previous directory).
- **`reveal`**: Lists files and directories with optional flags:
  - `-a`: Show all files including hidden dotfiles.
  - `-l`: Detailed line-by-line listing.
  - Supports combinations (e.g. `reveal -la`, `reveal -al`), custom target paths, and `-` for previous directory.
- **`log`**: Persistent command history across sessions (stored in `~/.shell_history`):
  - `log`: Display up to 15 most recent commands.
  - `log purge`: Clear command history.
  - `log execute <index>`: Re-execute a command by its history index.

### 3. Command Execution & I/O Redirection
- Foreground execution of system commands via `fork()` and `execvp()`.
- Full input and output redirection support:
  - `<` : Input redirection from file.
  - `>` : Output redirection (overwrite).
  - `>>` : Output redirection (append).
  - Combined input and output redirection in a single command.

### 4. Multi-Stage Pipelines
- Support for arbitrary number of piped commands (`cmd1 | cmd2 | ... | cmdN`).
- Seamless combination of pipelines with input/output redirection.
- Piped execution support for `reveal` as well.

### 5. Sequential Execution
- Semicolon `;` separated command lists executed sequentially left-to-right.

### 6. Background Processing & Job Control
- **Background execution**: Trailing `&` spawns processes asynchronously and prints `[<job_number>] <pid>`.
- Background process termination detection with exit status messages (normal or abnormal).
- **`activities`**: Lists all active background processes sorted alphabetically by command name.
- **`ping <pid> <signal_number>`**: Sends a specified UNIX signal to a process.
- **`fg <job_number>`**: Brings a running or stopped background process to foreground.
- **`bg <job_number>`**: Resumes a stopped background process in the background.

### 7. Signal Handling
- **`Ctrl+C` (SIGINT)**: Interrupts only the active foreground process without terminating the shell.
- **`Ctrl+Z` (SIGTSTP)**: Stops the active foreground process and moves it to the background jobs list.
- **`Ctrl+D` (EOF)**: Gracefully terminates all running background processes and logs out of the shell.

---

## Project Structure

```text
├── .gitignore
├── README.md
├── shell.tcl                  # Test script using expect / tcl
└── shell/
    ├── Makefile               # Build script (flags: -Wall -Wextra -Werror -std=c99)
    ├── include/
    │   ├── bg_jobs.h          # Background job structures and state tracking
    │   ├── executor.h        # Command execution, redirection, and pipeline definitions
    │   ├── intrinsics.h      # hop, reveal, log prototypes
    │   ├── parser.h          # Grammar tokenization & syntax parsing
    │   ├── prompt.h          # Shell prompt rendering
    │   └── signals.h         # Signal handling setups
    └── src/
        ├── bg_jobs.c
        ├── executor.c
        ├── intrinsics.c
        ├── main.c
        ├── parser.c
        ├── prompt.c
        └── signals.c
```

---

## Building and Running

### Compilation
Navigate to the `shell/` directory and run:
```bash
make
```

### Running the Shell
```bash
./shell.out
```

### Cleaning Build Artifacts
```bash
make clean
```

### Running Automated Tests
```bash
expect ../shell.tcl
```
