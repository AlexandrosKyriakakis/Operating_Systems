#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

void fork_procs (struct tree_node *root)
{
	int i;
	int status;
	if (root == NULL) {
		printf("empty node");
		return;	}	
	change_pname(root->name);
	if (root->nr_children == 0) {
		printf("Leaf %s created\n",root->name);
		sleep(SLEEP_PROC_SEC);
		exit(5);
	}
	printf("%s created\n", root->name);	
	pid_t pid_child;
	for (i = 0; i < root->nr_children; i++) {
		pid_child = fork();
		if (pid_child == 0) {
			fork_procs (root->children + i);
			raise(SIGCONT);
			printf("Proc with name = %s started",root->name);
			exit(1);
		}
	}
	printf("Proc with name = %s stoped",root->name);
	raise(SIGSTOP);

	for (i = 0; i < root->nr_children; i++) {
		//pid_child = waitpid(-1, &status, 0);
		wait_for_ready_children(1);
		explain_wait_status(pid_child, status);
	}	
	exit(2);	

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
	//wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */

	/* Print the process tree root at pid */
	//show_pstree(pid);

	/* for ask2-signals */
	//kill(pid, SIGCONT);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);

	return 0;
}

