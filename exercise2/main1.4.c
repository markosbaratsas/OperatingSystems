#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root, int fd)
{
        printf("PID = %ld, name %s, starting...\n",(long)getpid(), root->name);
        change_pname(root->name);

        if (root->nr_children != 0)
        {
                printf("PID = %ld, name %s, waiting...\n",(long)getpid(), root->name);
                int i=0;
                pid_t pidCHILD[root->nr_children];
                int compute[2];
                int pfd[2][2];

                for(i = 0; i < root->nr_children; i++)
                {
                        if (pipe(pfd[i]) < 0)
                        {
                                perror("pipe");
                                exit(1);
                        }
                        pidCHILD[i] = fork();
                        if (pidCHILD[i] < 0) 
                        {
                                perror("fork_procs(): fork");
                                exit(1);
                        } else if (pidCHILD[i] == 0)
                        {
                                fork_procs(root->children+i, pfd[i][1]);
                        }
                }
                wait_for_ready_children(2);
                if (read(pfd[0][0], &(compute[0]), sizeof(compute[0])) != sizeof(compute[0])) {
                        perror("child: read from pipe");
                        exit(1);
                }
                printf("PID = %ld, name %s readed value %d from child\n",
                    (long)getpid(), root->name, compute[0]);
                if (read(pfd[1][0], &(compute[1]), sizeof(compute[1])) != sizeof(compute[1])) {
                        perror("child: read from pipe");
                        exit(1);
                }
                printf("PID = %ld, name %s readed value %d from child\n",
                    (long)getpid(), root->name, compute[1]);


                int result = 0;
                // function strcmp(str1, str2) compares str1, str2
                // and returns zero if they are identical
                if(strcmp(root->name, "*") == 0) {
                    printf("PID = %ld, computing %d %s %d\n",
                            (long)getpid(), compute[0], root->name, compute[1]);
                    result = compute[0] * compute[1];
                }
                else {
                    printf("PID = %ld, computing %d %s %d\n",
                            (long)getpid(), compute[0], root->name, compute[1]);
                    result = compute[0] + compute[1];
                }

                printf("PID = %ld, writing result = %d to parent\n",
                            (long)getpid(), result);
                //write to pipe
                if (write(fd, &result, sizeof(result)) != sizeof(result))
                {
                        perror("parent: write to pipe");
                        exit(1);
                }
                // raise a signal so that parent knows that you have computed the result
                raise(SIGSTOP);

                // code executed after SIGCONT is raised for this process
                printf("PID = %ld, name %s is awake\n",(long)getpid(), root->name);

                // wake up a child, wait for it to terminate
                // do the same for all of your children
                for(i = 0; i < root->nr_children; i++) {
                        // wake up child with pid: pidCHILD[i]
                        kill(pidCHILD[i], SIGCONT);
                }
                pid_t wpid;
                int status = 0;
                while( (wpid = wait(&status)) > 0);
       }
        else
        {
                int charToInt;
                // convert char to int using sscanf
                sscanf((root->name), "%d", &charToInt);
                printf("PID = %ld, name %s writing value %d to parent\n",
                    (long)getpid(), root->name, charToInt);
                if (write(fd, &charToInt, sizeof(charToInt)) != sizeof(charToInt))
                {
                        perror("parent: write to pipe");
                        exit(1);
                }
                // raise a signal so that parent knows that you have computed the result
                raise(SIGSTOP);

                // code executed after SIGCONT is raised for this process
                printf("PID = %ld, name %s is awake\n",(long)getpid(), root->name);
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

        /* Read tree into memory */
        root = get_tree_from_file(argv[1]);
        /* Fork root of process tree */
        int pfd[2];
        if (pipe(pfd) < 0)
        {
                perror("pipe");
                exit(1);
        }

        pid = fork();
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }
        if (pid == 0) {
                fork_procs(root, pfd[1]);

                // child should never reach this point(it should have exited already)
                exit(1);
        }
        // wait until it raises SIGSTOP (the result is computed)
        wait_for_ready_children(1);

        // read what root computed
        int result;
        if (read(pfd[0], &result, sizeof(result)) != sizeof(result)) {
                perror("child: read from pipe");
                exit(1);
        }
        printf("PID = %ld, reading result from root of tree = %d\n",
                (long)getpid(), result);

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
