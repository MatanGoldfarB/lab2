#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>

// Signal handler function
void handler(int sig) {
    printf("\nReceived Signal: %s\n", strsignal(sig));

    // Propagate the signal to the default signal handler
    signal(sig, SIG_DFL);
    raise(sig);

    // Reinstall custom handlers as needed
    if (sig == SIGTSTP) {
        signal(SIGCONT, handler);
    } else if (sig == SIGCONT) {
        signal(SIGTSTP, handler);
    }
}

int main(int argc, char **argv) {
    printf("Starting the program\n");

    // Set up the signal handlers
    signal(SIGINT, handler);
    signal(SIGTSTP, handler);
    signal(SIGCONT, handler);

    // Infinite loop to keep the program running
    while (1) {
        sleep(1);
    }

    return 0;
}
