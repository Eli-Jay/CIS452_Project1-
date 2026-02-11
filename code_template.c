#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <stdio.h>
//#include <unistd.h>
//#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

struct process;


// Notes: each process will have a one way write to the next process. a->b->c....k
// process k (node k) will have a one way write to process 0 (node 0) to complete the circle.
// node 0 will have a one way 

struct process {

    struct process* next_process;
	struct process* previous_process;
    struct process* first_process;

	int process_id;
    bool has_apple;

};

void create_processes(int k) {

    int x = fork();
}

void error() {
    printf("Error");
    return 1;
}


int main()
{
    printf("Enter number of processes\n");
    int k = 0;

    if (scanf("%d", &k) != 1) error();
    


    return 0;
}

