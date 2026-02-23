#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>


typedef struct {
    int to;
    char text[128];
} msg;


volatile sig_atomic_t quitting = 0;

void on_sigint(int sig)
{
    (void)sig;
    quitting = 1;
}


void error()
{
    printf("Error");
    exit(1);
}


int main()
{
    // for exit on ctrl+c
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);

    // IMPORTANT: do not restart scanf/fgets on SIGINT
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    printf("Enter number of processes\n");
    int k = 0;

    pid_t pid = getpid();

    int self_pid = 0; // used to create a name that both the parent and child know.

    if (scanf("%d", &k) != 1) error();

    int p_list[k];

    // creating pipes
    int pipes[k][2];
    for (int i = 0; i < k; i++) pipe(pipes[i]);

    // creating processes
    for (int i = 1; i < k; i++)
    {
        pid = fork();


        if (pid == 0) {
            printf("PROCESS %d SPAWNED. PID = %d\n", i, getpid());
            self_pid = i;
            break;
        }
    }

    // sets up the pipes
    // each process keeps only:
    // in  = read end of previous pipe
    // out = write end of own pipe
    int in = pipes[(self_pid - 1 + k) % k][0];
    int out = pipes[self_pid][1];

    for (int i = 0; i < k; i++) {
        if (pipes[i][0] != in)  close(pipes[i][0]);
        if (pipes[i][1] != out) close(pipes[i][1]);
    }

    int shutdown_to = -2;
    int ready_to = -3;

    // if child process, check for apple and recieve messages. If it isnt for it, is passes it along.
    if (pid == 0) {
        printf("\tPROCESS %d WAITING\n", self_pid);

        int all_processes_inited = 0;

        // process k (last process) tells process 1 (parent) it is ready
        if (self_pid == k - 1 && all_processes_inited == 0) {
            msg r;
            r.to = ready_to;
            strcpy(r.text, "ready");
            write(out, &r, sizeof(r));
            all_processes_inited = 1;
        }

        while (true) {

            msg m;

            int n = read(in, &m, sizeof(m));
            if (n <= 0) {
                // parent died or pipe closed
                break;
            }

            if (m.to == shutdown_to) {
                // forward shutdown so everyone sees it, then exit
                write(out, &m, sizeof(m));
                break;
            }

            if (m.to == self_pid) {
                m.to = -1; // sets the header to recieved.
                printf("PROCESS %d RECIEVED MESSAGE\n", self_pid);
                printf("\tContents: %s\n", m.text);

                write(out, &m, sizeof(m)); // send the ack back around the ring
                continue;
            }

            write(out, &m, sizeof(m));

        }

        close(in);
        close(out);
        return 0;
    }


    // if parent process, send messages.
    if (pid != 0) {

        // wait for process k (last process) to send "ready"
        while (true) {
            msg start;
            int n = read(in, &start, sizeof(start));
            if (n <= 0) break;

            if (start.to == ready_to) {
                // consume it (do not forward) so it doesnt loop forever
                break;
            }

            // keep ring moving if something else appears
            write(out, &start, sizeof(start));
        }

        printf("Master process ready!\n");


        while (true) {

            if (quitting) break;

            msg m;
            printf("Processes: 1");
            for (int i = 2; i <= k; i++) printf("->%d", i); // shows available recipients


            printf("\nWho is the message for: ");
            if (scanf("%d", &m.to) != 1) {
                if (quitting) break;     // ctrl+c interrupted scanf
                clearerr(stdin);          // clear EINTR/error state
                break;
            }
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);

            if (quitting) break;

            printf("\nWhat is the message? (128 characters max): ");
            if (fgets(m.text, sizeof(m.text), stdin) == NULL) {
                if (quitting) break;     // ctrl+c interrupted fgets
                clearerr(stdin);
                break;
            }
            m.text[strcspn(m.text, "\n")] = '\0';

            write(out, &m, sizeof(m));

            msg back;
            int n = read(in, &back, sizeof(back));
            if (n <= 0) break;

            if (back.to == -1)
            {
                printf("Message was delivered\n");
            }
            else {
                printf("No one recieved the message.\n");
            }
        }

        // send shutdown poison-pill once
        msg s;
        s.to = shutdown_to;
        strcpy(s.text, "shutdown");
        write(out, &s, sizeof(s));

        // wait for children to exit
        for (int i = 1; i < k; i++) wait(NULL);

        close(in);
        close(out);
    }


    return 0;
}