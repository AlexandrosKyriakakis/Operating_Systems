#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

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
/* Leaves exit with 5, Non-Leaves exit with 2 */
void fork_procs (struct tree_node *root)
{
	int i;
	int status;	
	change_pname(root->name);
	/* if current node is a leaf */
	if (root->nr_children == 0) {
		printf("Leaf %s created and sleeping!\n",root->name);
		sleep(SLEEP_PROC_SEC);
		printf("Leaf %s exiting!\n", root->name);
		exit(5);
	}
	/* if current node is not a leaf */
	printf("Node %s created\n", root->name);	
	pid_t pid_child;
	for (i = 0; i < root->nr_children; i++) {
		pid_child = fork();
		if (pid_child == 0) {
			/* call recursive function for child */
			fork_procs (root->children + i);
		}
	}
	/* wait for all children to exit */
	printf("Node %s waiting for all children to exit!\n",root->name);
	for (i = 0; i < root->nr_children; i++) {
		pid_child = waitpid(-1, &status, 0);
		explain_wait_status(pid_child, status);
	}
	printf("Node %s exiting\n", root->name);	
	exit(2);	

}


int main(int argc, char **argv) {

	pid_t pid;
	int status;
	struct tree_node *root;
	if (argc !=2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n",argv[0]);
		exit(1);
	}
	root = get_tree_from_file(argv[1]);
	print_tree(root);
	pid = fork();
	if (pid < 0) { 
		perror("main:fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs(root);
	}
	sleep(SLEEP_TREE_SEC);
	show_pstree(pid);
	pid  = wait(&status);
	explain_wait_status(pid, status);
	printf("Root: All done, exiting...\n");

	return 0;
}
