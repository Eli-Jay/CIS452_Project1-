


#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <time.h>

/********Function Prototypes********/
void sigHandlerINT(int sigNum);

/********Global variables********/
volatile sig_atomic_t shutdown = 0;
pid_t *children = NULL;
int num_children = 0;

struct Message {
    int recipient;
    char message[256];
};

void error() {
    printf("Error\n");
    exit(1);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigHandlerINT;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    printf("Welcome to *One Bad Apple*! Press CTRL+C at any time to stop.\n");

    while (!shutdown) {
        printf("\nEnter number of recipients: ");
        fflush(stdout);  // Ensure prompt displays immediately
        
        int k;
        scanf("%d", &k);
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
        
        if (k <= 0 || k > 20) {
            printf("Invalid number: %d\n", k);
            continue;
        }
        num_children = k;
        
        children = realloc(children, sizeof(pid_t) * k);
        if (!children) {
            perror("malloc");
            return 1;
        }

        int pipes[20][2];
        bool is_child = false;
        pid_t self_pid = 0;

        for (int i = 0; i < k; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                shutdown = 1;
                goto cleanup;
            }
        }


        int ready_pipes[20][2];
        for (int i = 0; i < k; i++) {
            if (pipe(ready_pipes[i]) == -1) {
                perror("ready_pipe");
                shutdown = 1;
                goto cleanup;
            }
        }

        for (int i = 0; i < k; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                shutdown = 1;
                goto cleanup;
            } else if (pid == 0) {
                is_child = true;
                self_pid = i + 1;
                break;
            } else {
                children[i] = pid;
            }
        }
        
        

        if (shutdown) goto cleanup;

        if (is_child) {
            int in_fd = pipes[(self_pid - 2 + k) % k][0];
            int out_fd = pipes[self_pid - 1][1];

            // Close unused pipe ends
            for (int j = 0; j < k; j++) {
                if (j != (self_pid - 2 + k) % k) close(pipes[j][0]);
                if (j != self_pid - 1) close(pipes[j][1]);
            }

            printf("Process %d ready (PID: %d)\n", self_pid, getpid());

            write(ready_pipes[self_pid-1][1], "R", 1);  // Signal ready
            close(ready_pipes[self_pid-1][1]);

            struct Message msg;
            while (!shutdown) {
                ssize_t bytes = read(in_fd, &msg, sizeof(msg));
                if (bytes <= 0) break;

                printf("Apple at Process %d\n", self_pid);
                
                if (msg.recipient == self_pid) {
                    printf("Process %d KEEPING the apple: %s\n", self_pid, msg.message);
                    break;
                } else {
                    printf("Process %d passing apple to next...\n", self_pid);
                    write(out_fd, &msg, sizeof(msg));
                }
            }
            close(in_fd);
            close(out_fd);
            exit(0);
        } else {

            printf("Waiting for all %d processes to be ready...\n", k);
        for (int i = 0; i < k; i++) {
            char ready_sig;
            read(ready_pipes[i][0], &ready_sig, 1);  // BLOCK until child signals
            close(ready_pipes[i][0]);
            close(ready_pipes[i][1]);
            }
            printf("All processes ready! Who should KEEP the apple (1-%d): ", k);
            // Parent
            int recipient;
            //printf("Who should KEEP the apple (1-%d): ", k);
            fflush(stdout);
            scanf("%d", &recipient);
            while ((ch = getchar()) != '\n' && ch != EOF);
            
            if (recipient < 1 || recipient > k) {
                printf("Invalid keeper\n");
                shutdown = 1;
                goto cleanup;
            }
            
            printf("Enter apple message: ");
            fflush(stdout);
            char message[256];
            fgets(message, sizeof(message), stdin);
            message[strcspn(message, "\n")] = '\0';

            struct Message msg = {recipient, ""};
            strncpy(msg.message, message, sizeof(msg.message) - 1);
            msg.message[sizeof(msg.message) - 1] = '\0';

            for (int i = 0; i < k; i++) {
                close(pipes[i][0]);
            }

            printf("Starting apple at Process 1...\n");
            write(pipes[0][1], &msg, sizeof(msg));
            close(pipes[0][1]);

            // Wait for ALL children to exit before continuing
            int remaining = k;
            while (remaining > 0 && !shutdown) {
                int status;
                pid_t pid = waitpid(-1, &status, WNOHANG);
                if (pid > 0) {
                    remaining--;
                } else {
                    usleep(100000);  // 0.1 second poll
                }
            }

            printf("Round complete! All children closed.\n");
        }
        
    cleanup:
        // Force cleanup of any remaining children
        if (children) {
            for (int i = 0; i < num_children; i++) {
                if (children[i] > 0) {
                    kill(children[i], SIGTERM);
                    waitpid(children[i], NULL, 0);
                    children[i] = 0;
                }
            }
        }
        // Loop continues to next iteration
    }

    if (children) free(children);
    printf("Program fully terminated.\n");
    return 0;
}

void sigHandlerINT(int sigNum) {
    shutdown = 1;
    printf("\nStop entered. Shutting down...\n");
    fflush(stdout);
}
