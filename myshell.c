#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "LineParser.h"

#define MAX_LINE_LENGTH 2048
#define PATH_MAX 4096

int debug_mode = 0;

// Function to display the current working directory as a prompt
void displayPrompt() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s$ ", cwd);
    } else {
        perror("getcwd error");
        exit(EXIT_FAILURE);
    }
}

// Function to read a line from stdin
char* readLine() {
    char* buffer = malloc(MAX_LINE_LENGTH);
    if (!buffer) {
        perror("malloc error");
        exit(EXIT_FAILURE);
    }
    if (fgets(buffer, MAX_LINE_LENGTH, stdin) == NULL) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

// Function to handle built-in commands like "cd", "alarm", and "blast"
int handleBuiltInCommands(cmdLine *pCmdLine) {
    if (strcmp(pCmdLine->arguments[0], "cd") == 0) {
        char* dir = pCmdLine->arguments[1];
        if (!dir) {
            // If no directory specified, change to home directory
            dir = getenv("HOME");
            if (!dir) {
                fprintf(stderr, "cd: HOME environment variable not set\n");
                return 1; // Indicate that this was a built-in command
            }
        } else if (strcmp(dir, "~") == 0) {
            // Expand ~ to home directory
            dir = getenv("HOME");
            if (!dir) {
                fprintf(stderr, "cd: HOME environment variable not set\n");
                return 1; // Indicate that this was a built-in command
            }
        }
        if (chdir(dir) != 0) {
            perror("cd error");
        }
        return 1; // Indicate that this was a built-in command
    } else if (strcmp(pCmdLine->arguments[0], "alarm") == 0) {
        // Send SIGCONT signal to wake up a sleeping process
        pid_t pid = atoi(pCmdLine->arguments[1]);
        if (kill(pid, SIGCONT) == -1) {
            perror("alarm error");
        } else {
            printf("Alarm sent to process %d\n", pid);
        }
        return 1; // Indicate that this was a built-in command
    } else if (strcmp(pCmdLine->arguments[0], "blast") == 0) {
        // Send SIGKILL signal to terminate a process
        pid_t pid = atoi(pCmdLine->arguments[1]);
        if (kill(pid, SIGKILL) == -1) {
            perror("blast error");
        } else {
            printf("Process %d terminated\n", pid);
        }
        return 1; // Indicate that this was a built-in command
    }
    return 0; // Not a built-in command
}

// Function to execute a parsed command
void execute(cmdLine *pCmdLine) {
    pid_t pid;
    int status;

    if (debug_mode) {
        fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
    }

    if ((pid = fork()) == -1) {
        perror("fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        if (debug_mode) {
            fprintf(stderr, "Child process PID: %d\n", getpid());
        }

        // Redirect input/output if necessary
        if (pCmdLine->inputRedirect != NULL) {
            int input_fd = open(pCmdLine->inputRedirect, O_RDONLY);
            if (input_fd == -1) {
                perror("input redirection error");
                exit(EXIT_FAILURE);
            }
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (pCmdLine->outputRedirect != NULL) {
            int output_fd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (output_fd == -1) {
                perror("output redirection error");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Execute the command
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
            perror("execvp error");
            _exit(EXIT_FAILURE);
        }
    } else { // Parent process
        if (debug_mode) {
            fprintf(stderr, "Parent process PID: %d\n", getpid());
            fprintf(stderr, "Waiting for child process PID: %d\n", pid);
        }

        if (!pCmdLine->blocking) {
            // Non-blocking execution (background process)
            printf("[%d] %s\n", pid, pCmdLine->arguments[0]);
            return;
        }

        // Wait for the child process to finish
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid error");
            exit(EXIT_FAILURE);
        }

        if (debug_mode) {
            fprintf(stderr, "Child process PID: %d finished\n", pid);
        }
    }
}

int main(int argc, char *argv[]) {
    // Check for debug flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debug_mode = 1;
        fprintf(stderr, "Debug mode enabled\n");
    }

    char* input;
    cmdLine* parsedCmdLine;

    while (1) {
        // Display prompt
        displayPrompt();

        // Read a line from stdin
        input = readLine();
        if (input == NULL) {
            printf("\n");
            break;
        }

        // Parse the input
        parsedCmdLine = parseCmdLines(input);
        free(input); // Free the input buffer

        if (parsedCmdLine == NULL) {
            printf("Error parsing command line\n");
            continue;
        }

        // Check if the command is "quit"
        if (strcmp(parsedCmdLine->arguments[0], "quit") == 0) {
            freeCmdLines(parsedCmdLine); // Free the parsed command line structure
            break; // End the infinite loop
        }

        // Handle built-in commands
        if (!handleBuiltInCommands(parsedCmdLine)) {
            // Execute the parsed command
            execute(parsedCmdLine);
        }

        // Free the parsed command line structure
        freeCmdLines(parsedCmdLine);
    }

    printf("Exiting shell.\n");
    return 0;
}
