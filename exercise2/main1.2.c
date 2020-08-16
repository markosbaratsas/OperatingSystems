#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"


#define SLEEP_PROC_SEC  15
#define SLEEP_TREE_SEC  5


void fork_procs(struct tree_node *root)
{
        printf("PID = %ld, name %s, starting...\n",(long)getpid(), root->name);
        change_pname(root->name);

        if (root->nr_children == 0)
        {
                printf("PID = %ld, name %s, sleeping...\n",(long)getpid(), root->name);
                sleep(SLEEP_PROC_SEC);
        }

        else
        {
                printf("PID = %ld, name %s, waiting...\n",(long)getpid(), root->name);
                int i;
                for(i = 0; i < root->nr_children; i++)
                {
                        pid_t pidCHILD = fork();
                        if (pidCHILD == 0)
                        {
                                fork_procs(root->children+i);
                                break;
                        }
                }
                pid_t wpid;
                int status = 0;
                while( (wpid = wait(&status)) > 0);
        }

        printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);
        printf("PID = %ld, name %s, exiting...\n",(long)getpid(), root->name);
        exit(0);
}


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
                exit(0);
        }

        /* Print tree to see its form */
        print_tree(root);

        /* Print the process tree root at pid */
        //show_pstree(pid);
        show_pstree(getpid());

        /* Wait for the root of the process tree to terminate */
        wait(&status);
        explain_wait_status(pid, status);

        return 0;
}

