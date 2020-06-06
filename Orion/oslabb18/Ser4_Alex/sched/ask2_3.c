#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proc-common.h"
#include "tree.h"
void fork_procs(struct tree_node *root)
{
	/*
	 * Start
	 */
	int i, status, children;
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), root->name);
	change_pname(root->name);
	if (root->nr_children == 0) { 
		raise(SIGSTOP);
		printf("PID = %ld, name = %s is awake\n", (long)getpid(), root->name);
		exit(0);
	} 
	/* create proccess tree*/
	else {
		children = root->nr_children;
		pid_t pid[children]; 
		for (i = 0; i < children; i++) {
			pid[i] = fork();
			if (pid[i] == 0) {
				fork_procs(root->children + i);
				exit(0);
			} 
			if (pid[i] < 0) {
				perror("fork in fork_procs");
				exit(1);
			}
		}
		/*wait for all children to stop*/
		wait_for_ready_children(children);
		/*stop*/
		raise(SIGSTOP);
		printf("*PID = %ld, name = %s is awake\n", (long)getpid(), root->name);
		/* wake up each child and wait for it to exit leading to DFS messages */
		for (i = 0; i < children; i++) {
//			printf("children of proccess with id %ld : %d\n", (long)getpid(), children);
			/* wake up first child */ 
			kill(pid[i], SIGCONT);
//			printf("proccess with id %ld sent SIGCONT to child with id %ld\n", (long)getpid(), pid[i]);
			/* wait for it to exit */
			pid[i] = waitpid(pid[i], &status, 0);
			explain_wait_status(pid[i], status);
		} 
	}		
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

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;
	struct sigaction sa;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}

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

	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	kill(pid, SIGCONT);
	pid = waitpid(pid, &status, 0);
	explain_wait_status(pid, status);
	/* Wait for the root of the process tree to terminate */
	

	return 0;
}
