#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */

/* ##### CREATE A ##### */
void fork_procs(void)
{
	change_pname("A");
	printf("A: Initializing...\n");
	printf("A: Waiting for children...\n");
	
	/* ##### CREATE B ##### */
	pid_t pidB = fork();
	if (pidB == 0)
	{ 
		change_pname("B");
		printf("B: Initializing...\n");
	        printf("B: Waiting for children...\n");
		
		/* ##### CREATE D ##### */
		pid_t pidD = fork();
		if (pidD == 0)
		{
			change_pname("D");
                	printf("D: Initializing...\n");
			printf("D: Sleeping...\n");
			sleep(SLEEP_PROC_SEC);
			printf("D: Exiting...\n");
        		exit(13);	
		}

		sleep(1.1*SLEEP_PROC_SEC); // creating delay for message	
		printf("B: Exiting...\n");
                exit(19);
	}
	
	/* ##### CREATE C ##### */
	pid_t pidC = fork();
	if (pidC == 0)
	{
		change_pname("C");
                printf("C: Initializing...\n");
		printf("C: Sleeping...\n");             
                sleep(SLEEP_PROC_SEC);
                printf("C: Exiting...\n");
                exit(17);
	}

	sleep(1.3*SLEEP_PROC_SEC); // creating delay for message 
	printf("A: Exiting...\n");
	exit(16);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs();
		exit(1);
	}
	
	//if we want to kill the process prematurely
        kill(pid, SIGKILL);

	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	//sleep(5*SLEEP_PROC_SEC); // creating delay for fork_procs() to finish
	show_pstree(pid); // show defferent results depenting from delay
	show_pstree(getpid());

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
