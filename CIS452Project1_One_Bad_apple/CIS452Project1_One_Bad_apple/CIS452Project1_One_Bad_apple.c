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


void error()
{
    printf("Error");
    exit(1);
}


int main()
{
    printf("Enter number of processes\n");
    int k = 0;

    pid_t pid = getpid();

    // DOES NOT WORK v. pipes are required. pointers get reassigned when a new process is created
    //int who_has_apple = -1; // used for the parent to say who has the apple. '-1' means the message isnt available right now.
    //int* who_has_apple_ptr = &who_has_apple;
    //char message[128];
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


    // if child process, check for apple and recieve messages. If it isnt for it, is passes it along.
    if (pid == 0) {
        printf("\tPROCESS %d WAITING\n", self_pid);

        while (true) {

            msg m;

            read(in, &m, sizeof(m));
        
            if (m.to == self_pid) {
                m.to = -1; // sets the header to recieved.
                printf("PROCESS %d RECIEVED MESSAGE\n", self_pid);
                printf("\tContents: %s\n", m.text);
                break;
            }
            
            write(out, &m, sizeof(m));
        
        }
    }


    // if parent process, send messages.
    if (pid != 0) {

        for (int i = 0; i < k * 20000; i++); // simple delay to allow all processes to spawn
        printf("Master process here %d\nProcesses: 1", self_pid);


        while (true) {

            for (int i = 0; i < k * 20000; i++); // simple delay to allow all processes to spawn

            msg m;

            

            for (int i = 2; i <= k; i++) printf("->%d", i); // shows available recipients

            int recipient = 0;


            printf("\nWho is the message for: ");
            scanf("%d", &m.to);
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}

            //m.text = "Howdy!";

            printf("\nWhat is the message? (128 characters max): ");
            fgets(m.text, sizeof(m.text), stdin);
            m.text[strcspn(m.text, "\n")] = '\0';

            write(out, &m, sizeof(m));

            msg back;
            read(in, &back, sizeof(back));

            if (back.to != -1) 
            {
                printf("Message was delivered\n");
            }
            else {
                printf("No one recieved the message.\n");
            }
        }
        
    }


    return 0;
}

