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
#define SCHED_TQ_SEC 2                	/* time quantum */
#define TASK_NAME_SZ 60               	/* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell"	/* executable for shell */
#define MAX_NUMBER_OF_PROCESSES 1000 	/* maximum number of processes that will be scheduled */


/* List of pids.. */
typedef struct node {
        pid_t p;
	int id;
	char *name;		
        struct node * next;
        struct node * previous;
	int isHigh;
} node_t;


/* define globally so that we can use it in sigalrm_handler */
int nproc = MAX_NUMBER_OF_PROCESSES;
node_t * head = NULL;
node_t * last = NULL;
node_t * current_process_running = NULL;
int id;
char *argv0;
int high_processes_count;

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

void fork_child(char* executable, char* argv)
{

	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };
	printf("I am PID = %ld\n",
			(long)getpid());
	printf("About to replace myself with the executable %s...\n",
			executable);
	sleep(1);

	raise(SIGSTOP);

	execve(executable, newargv, newenviron);
}


/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	node_t * current;
	current = head;

	printf("Id: %d, PID: %d, Name: %s",
			current->id, (int)current->p, current->name);
	if(current->isHigh == 1) {
		printf(", Priority: High\n");
	}
	else {
		printf(", Priority: Low\n");
	}
	current = current->next;
	while(current->p != head->p) {
		printf("Id: %d, PID: %d, Name: %s",
				current->id, (int)current->p, current->name);
		if(current->isHigh == 1) {
			printf(", Priority: High\n");
		}
		else {
			printf(", Priority: Low\n");
		}
		current = current->next;
	}
}

/* Function that helps us find pid using id, via searching in the list .
 * Returns node_t * of the process that is going to be killed */
node_t * find_next_high_node(node_t * this_node) {

	node_t * current = this_node->next;

	while(current->p != this_node->p) {
		if (current->isHigh == 1) return current;
		current = current->next;
	}
	return this_node;
}

/* Function that helps us find pid using id, via searching in the list .
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

/* Function that helps us find pid using id, via searching in the list .
 * Returns node_t * of the process that is going to be killed */
node_t * find_node_of_id(int id) {

	node_t * current;
	current = head;

	if (current->id == id) return current;
	current = current->next;
	while(current->p != head->p) {
		if (current->id == id) return current;
		current = current->next;
	}
	return NULL;
}
/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
	static int
sched_kill_task_by_id(int id)
{
	//printf("    Stopping: %d \n", (int)(current_process_running->p));

	node_t * process_killed = find_node_of_id(id);
	
	if (process_killed == NULL) {
		printf("No process with id = %d... Give some process's id!\n", id);
		return 0;
	}

	printf("We want to kill p: %d\n", (int)process_killed->p);
	pid_t p = process_killed->p;

	printf("Killing process with id = %d and PID = %d\n", id, process_killed->p);
	if (kill(process_killed->p, SIGKILL) < 0) {
		perror("kill1");
		exit(1);
	}

	/* Handling of process kill will be done in sigchld_handler() */
	return p;
}


/* Create a new task.  */
	static void
sched_create_task(char *executable)
{
	node_t * current = NULL;
	current = (node_t *) malloc(sizeof(node_t));
	if (current == NULL) {
		perror("malloc");
		exit(1);
	}

	last->next = current;
	current->previous = last;
	current->next = head;
	current->id = id;
	current->isHigh = 0;
	++id;

	/* Passing the name from *executable to nam* */
	char * nam = (char *) malloc(60*sizeof(char));
	int i;
	for(i = 0; i < 60; i++) {
		nam[i] = executable[i];
	}
	current->name = nam;

	current->p = fork();

	if (current->p < 0) {
		perror("fork");
		exit(1);
	}
	else if (current->p == 0) {
		fork_child(executable, argv0);

		/* It should never reach here...  */
		exit(1);
	}

	last = current;

	wait_for_ready_children(1);


	printf("New process created with: \n");
	printf("    Id: %d, PID: %d, Name: %s\n", 
			current->id, (int)current->p, current->name);
}

void sched_low_task(int id) 
{
	node_t *this_node = find_node_of_id(id);
	if(this_node->isHigh == 0) {
		printf("\n     Process %d is already on low priority\n", this_node->p);
	}
	else {
		printf("\n     Setting process %d on low priority\n", this_node->p);
		this_node->isHigh = 0;
		--high_processes_count;	
	}
	return;
}

void sched_high_task(int id) 
{
	node_t *this_node = find_node_of_id(id);
	if(this_node->isHigh == 1) {
		printf("\n     Process %d is already on high priority\n", this_node->p);
	}
	else {
		printf("\n     Setting process %d on high priority\n", this_node->p);
		this_node->isHigh = 1;
		++high_processes_count;	
	}
	return;
}

/* Process requests by the shell.  */
	static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

		case REQ_LOW_TASK:
			sched_low_task(rq->task_arg);
			return 0;

		case REQ_HIGH_TASK:
			sched_high_task(rq->task_arg);
			return 0;

		default:
			return -ENOSYS;
	}
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
			/* A child has died */
			current_process_running = find_node_of_pid(p);
			/* Checking if its the last process.
			 * If not, the remove this node from the list. */
			/* Here we handle the list, so that it's coherent, 
			 *      not showing in processes that have exited... */
			if (current_process_running->p != (current_process_running->next)->p) {
				node_t * ended_process;
				ended_process = current_process_running;

				current_process_running = ended_process->next;

				printf("    Process: %d has exited.\n", (int)ended_process->p);

				if ((ended_process->previous) != NULL)
					(ended_process->previous)->next = current_process_running;
				else {
					last->next = current_process_running;
					head = current_process_running;
				}
				if(last->p == ended_process->p) {
					last = ended_process->previous;
				}
				current_process_running->previous = ended_process->previous;

				free(ended_process);

				/* For handling of high - low priority processes */

				if(ended_process->isHigh == 1) {	
					--high_processes_count;
				}

				if(high_processes_count > 0) {
					current_process_running = find_next_high_node(current_process_running);
					printf("    Starting: %d\n", (int)(current_process_running->p));
					if (kill((current_process_running->p), SIGCONT) < 0) {
						perror("kill");
						exit(1);
					}
				}
				else {
					current_process_running = current_process_running->next;
					printf("    Starting: %d\n", (int)(current_process_running->p));
					if (kill((current_process_running->p), SIGCONT) < 0) {
						perror("1kill");
						exit(1);
					}
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
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc */
			if(high_processes_count > 0) {
				current_process_running = find_next_high_node(current_process_running);
				printf("    Starting: %d\n", (int)(current_process_running->p));
				if (kill((current_process_running->p), SIGCONT) < 0) {
					perror("kill");
					exit(1);
				}
			}
			else {
				current_process_running = current_process_running->next;
				printf("    Starting: %d\n", (int)(current_process_running->p));
				if (kill((current_process_running->p), SIGCONT) < 0) {
					perror("2kill");
					exit(1);
				}
			}

			/* alarm after SCHED_TQ_SEC */
			alarm(SCHED_TQ_SEC);
			return;
		}
	}
}

/* Disable delivery of SIGALRM and SIGCHLD. */
	static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
	static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
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

	static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
	pid_t
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];

	return p;
}

	static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();
		alarm(SCHED_TQ_SEC);

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}




int main(int argc, char *argv[])
{
	int i;
	int nproc = argc - 1;

	/* Initialization of global variables */
	id = 1;
	high_processes_count = 0;
	argv0 = argv[0];
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;

	/* Create the shell. */
	pid_t p = sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);

/*	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
*/

	/* Code for Shell..... */
	node_t * current = NULL;
	current = (node_t *) malloc(sizeof(node_t));
	if (current == NULL) {
		perror("malloc");
		exit(1);
	}
	/* Head of list */
	head = current;
	last = current;
	current->previous = NULL;
	current->isHigh = 0;
	current->next = head;
	current->id = id;
	++id;
	current->name = "Shell";
	current->p = p;


	/* Code for the rest of the processes */
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
			head->next = current;
			current->previous = head;
		}
		else {
			previous->next = current;
			current->previous = previous;
		}

		current->id = id;
		++id;
		current->name = argv[i+1];
		current->isHigh = 0;
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


	/* Wait for all children AND Shell to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc + 1);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	/* Start the first process(start Shell) */
	current_process_running = head;
	if (kill((current_process_running->p), SIGCONT) < 0) {
		perror("kill");
		exit(1);
	}
	/* The rest is handled in sigalrm_handler() */
	alarm(SCHED_TQ_SEC);

	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
