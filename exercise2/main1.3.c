#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root)
{
        printf("PID = %ld, name %s, starting...\n",(long)getpid(), root->name);
        change_pname(root->name);

        if (root->nr_children != 0)
        {
                printf("PID = %ld, name %s, waiting...\n",(long)getpid(), root->name);
                int i=0;
                pid_t pidCHILD[root->nr_children];

                for(i = 0; i < root->nr_children; i++)
                {
			pidCHILD[i] = fork();
                        if (pidCHILD[i] == 0)
                        {
                                fork_procs(root->children+i);
                        }
                        wait_for_ready_children(1);
                }
                raise(SIGSTOP);

		// code executed after SIGCONT is raised for this process
        	printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);

		// wake up a child, wait for it to terminate
		// do the same for all of your children
		for(i = 0; i < root->nr_children; i++) {
			// wake up child with pid: pidCHILD[i]
			kill(pidCHILD[i], SIGCONT);

			// when a child dies wait(&status) will return 
			// pid of the process that exited
			// so we wait until pidCHILD[i] dies
			int status = 0;
			while((pidCHILD[i] != wait(&status))) ;
		}
       }
        else
        {
                raise(SIGSTOP);

		// code executed after SIGCONT is raised for this process
        	printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);
        }

        printf("PID = %ld, name %s, exiting...\n",(long)getpid(), root->name);
        exit(0);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * use wait_for_ready_children() to wait until
 * the first process raises SIGSTOP.
 * Then SIGCONT root, and the rest of the tree 
 * will wake up from fork_procs()
 */

int main(int argc, char *argv[])
{
        pid_t pid;
        int status;
        struct tree_node *root;

        if (argc < 2){
                fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
                exit(1);
        }

	/* Read tree into memory */
        root = get_tree_from_file(argv[1]);

        /* Fork root of process tree */
        pid = fork();
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }
        if (pid == 0) {
                /* Child */
                fork_procs(root);
                exit(1);
        }

        wait_for_ready_children(1);

        /* Print tree to see its form */
        print_tree(root);

        /* Print the process tree root at pid */
        show_pstree(getpid());

	// wake up root of tree
        printf("Waking up PID: %d\n", pid);
        kill(pid, SIGCONT);

        /* Wait for the root of the process tree to terminate */
        wait(&status);
        explain_wait_status(pid, status);

        return 0;
}


