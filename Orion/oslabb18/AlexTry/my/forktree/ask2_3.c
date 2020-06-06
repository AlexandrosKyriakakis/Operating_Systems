#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tree.h"
#include "proc-common.h"
#include <signal.h>

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
 
void sigchild_handler(int signum) {
	pid_t p;
	int status;
	do {
		p = waitpid(-1, &status, WUNTRACED| WNOHANG);
		if (p < 0) {
			perror("waitpid");
			exit(1);
		}
		printf("Signal handled\n");
		explain_wait_status(p, status);
	} while (p > 0);
}
*/
void fork_procs (struct tree_node *root)
{
	int i;
	int status;
	if (root == NULL) {
		printf("empty node");
		return;
	}	
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), root->name);
	change_pname(root->name);
	if (root->nr_children == 0) {
		printf("Leaf %s created\n",root->name);
		raise(SIGSTOP);
		printf("PID = %ld, name = %s is awake\n",
			(long)getpid(), root->name);
		//wait_for_ready_children(1);
		exit(5);
	}
	printf("%s created\n", root->name);	
	pid_t pid_child[root->nr_children];
	
	for (i = 0; i < root->nr_children; i++) {
		pid_child[i] = fork();
		if (pid_child[i] == 0){
			fork_procs (root->children + i);
		//	raise(SIGCONT);
			exit(1);
		}
		/*
		if (pid_child > 0){
			//raise(SIGSTOP);
			//kill(pid_child ,SIGCONT);
			//printf("PID = %ld, name = %s is CONT\n",
			//	(long)getpid(), root->name);

			printf("PID = %ld, name = %s is awake\n",
				(long)getpid(), root->name);
		}*/
	}
	wait_for_ready_children(root->nr_children);
	raise(SIGSTOP);
	printf("PID = %ld, name = %s is awake\n",
			(long)getpid(), root->name);
	for (i = 0; i < root->nr_children; i++) {
		kill(pid_child[i], SIGCONT);
		pid_child[i] = waitpid(pid_child[i], &status, 0);
		explain_wait_status(pid_child[i], status);
	}	
	exit(0);	

}


int main(int argc, char **argv) {

	pid_t pid;
	int status;
	struct tree_node *root;
	/*
	struct sigaction sa;
	sigset_t sigset;
	sa.sa_handler = sigchild_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	*/
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n",argv[0]);
		exit(1);
	}
	root = get_tree_from_file(argv[1]);
	//print_tree(root);

	pid = fork();
	if (pid < 0) { 
		perror("main:fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs(root);
		exit(1);
	}
	wait_for_ready_children(1);
//	sleep(SLEEP_TREE_SEC);
	show_pstree(pid);
	kill(pid, SIGCONT);
	pid  = wait(&status);
	explain_wait_status(pid, status);
	printf("Parent: All done, exiting...\n");

	return 0;
}

