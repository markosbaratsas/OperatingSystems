#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2               	/* time quantum */
#define TASK_NAME_SZ 60               	/* maximum size for a task's name */
#define MAX_NUMBER_OF_PROCESSES 1000  	/* maximum number of processes that will be scheduled */



/* List of pids.. */
typedef struct node {
	pid_t p;
	struct node * next;
	struct node * previous;
} node_t;


/* define globally so that we can use it in sigalrm_handler */
int nproc = MAX_NUMBER_OF_PROCESSES; 
node_t * head = NULL;
node_t * last = NULL;
node_t * current_process_running = NULL;



/* Function that helps us find pid using pid, via searching in the list .
 * Returns node_t * of the process that is going to be killed */
node_t * find_node_of_pid(pid_t p) {
	node_t * current;
	current = head;

	if (current->p == p) return current;
	current = current->next;
	while(current->p != head->p) {
		if (current->p == p) return current;
		current = current->next;
	}
	return NULL;
}


/*
 * SIGALRM handler
 */
	static void
sigalrm_handler(int signum)
{
	printf("    Stopping: %d \n", (int)(current_process_running->p));
	if (kill(current_process_running->p, SIGSTOP) < 0) {
		perror("kill");
		exit(1);
	}
}

/* 
 * SIGCHLD handler
 */
	static void
sigchld_handler(int signum)
{
	pid_t p;
	int status;
	for(;;) {
		p = waitpid(-1, &status, WUNTRACED | WNOHANG);
		if (p < 0) {
			perror("waitpid");
			exit(1);
		}
		if ((int)p == 0) {
			return;
		}
		//explain_wait_status(p, status);
		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			/* A child has died or got killed */
			current_process_running = find_node_of_pid(p);

			/* Checking if its the last process.
			 * If not, the remove this node from the list. */
			/* Here we handle the list, so that it's coherent, 
			 * 	not showing in processes that have exited... */
			if (current_process_running->p != (current_process_running->next)->p) {
				node_t * ended_process;
				ended_process = current_process_running;

				current_process_running = ended_process->next; 

				printf("    Process: %d has exited.\n", (int)ended_process->p);

				if(ended_process->p == last->p) 
					last = ended_process->previous;

				if ((ended_process->previous) != NULL)
					(ended_process->previous)->next = current_process_running;
				else {
					last->next = current_process_running;
				}

				current_process_running->previous = ended_process->previous;

				free(ended_process);

				printf("    Starting: %d\n", (int)(current_process_running->p));
				if (kill((current_process_running->p), SIGCONT) < 0) {
					perror("kill");
					exit(1);
				}

				/* alarm after SCHED_TQ_SEC */
				alarm(SCHED_TQ_SEC);
			}
			else {
				printf("    Process: %d has exited.\n", (int)current_process_running->p);

				free(current_process_running);

				printf("    All the processes ended successfully!\n");	
				exit(0);	
			}
			return;
		}


		if (WIFSTOPPED(status)) {
			/* A child has stopped due to SIGSTOP */
			current_process_running = current_process_running->next; 
			printf("    Starting: %d\n", (int)(current_process_running->p));
			if (kill((current_process_running->p), SIGCONT) < 0) {
				perror("kill");
				exit(1);
			}

			/* alarm after SCHED_TQ_SEC */
			alarm(SCHED_TQ_SEC);
			return;
		}
	}
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
	static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

/* Replacing a process with some executable file given by argv[] */
void fork_child(char* executable, char* argv) 
{

	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };
	printf("I am %s, PID = %ld\n",
			argv, (long)getpid());
	printf("About to replace myself with the executable %s...\n",
			executable);
	sleep(1);

	raise(SIGSTOP);

	execve(executable, newargv, newenviron);
}

/* This function was used in debugging*/
void print_list() {
	int i =0;
	node_t * current;
	current = head;

	printf("%d: Next: %d\n", 
			(int)current->p, (int)current->next->p);
	current = current->next;
	i++;
	while(current->p != head->p) {
		printf("%d: Previous: %d, Next: %d\n", 
				(int)current->p, (int)current->previous->p, (int)current->next->p);
		current = current->next;
		i++;
	}
}

int main(int argc, char *argv[])
{
	int i;
	nproc = argc - 1; /* number of proccesses goes here */

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	/* Cycle list */	

	node_t * previous = NULL;
	for(i = 0; i < nproc; i++) {
		node_t * current = NULL;
		current = (node_t *) malloc(sizeof(node_t));
		if (current == NULL) {
			perror("malloc");
			exit(1);
		}

		/* Head of list */
		if(i ==0)  {
			head = current;
		}
		else {
			previous->next = current;
		}

		current->previous = previous;
		current->next = head;
		current->p = fork();

		if (current->p < 0) {
			perror("fork");
			exit(1);
		}
		else if (current->p == 0) {
			fork_child(argv[i+1], argv[0]);

			/* It should never reach here...  */
			exit(1);		
		}


		previous = current;
		if (i == nproc-1)
			last = current;
	}


	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}

	/* start the first process */
	current_process_running = head;
	if (kill((current_process_running->p), SIGCONT) < 0) {
		perror("kill");
		exit(1);
	}
	/* The rest is handled in sigalrm_handler() */
	alarm(SCHED_TQ_SEC);

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
