#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tree.h"
#include "proc-common.h"
#include <signal.h>

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

char mycompute (char kind, char a, char b){
	int a_int = a - '0', b_int = b - '0';
	switch (kind){
	case '+': {printf("I will execute %i + %i with result %i !\n",a_int,b_int,a_int+b_int); return a_int + b_int + '0';}
	case '*': {printf("I will execute %i * %i with result %i !\n",a_int,b_int,a_int*b_int); return a_int * b_int + '0';}
	}
	return -1;
}

void fork_procs (struct tree_node *root, int fd){
	int i;
	int status;
	// If NULL return ERROR
	if (root == NULL) {
		printf("empty node");
		return;
	}	
	
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), root->name);
	
	change_pname(root->name);
	if (root->nr_children == 0) {
		printf("Leaf %s created\n",root->name);
		// We stop here and wait for parent to give SIGCONT after all leafs created
		raise(SIGSTOP);
		// root->name is something like string so we cast it to int (char) 
		char cur = atoi(root->name) + '0';
		// then we write it to the pipe
		if (write(fd, &cur, 
			sizeof(cur)) != sizeof(cur)) {
			perror("Child: write to pipe");
			exit(1);
		}
		printf("PID = %ld, name = %s is awake and wrote to FD = %d \n",(long)getpid(), root->name, fd);
		exit(5);
	}
	
	printf("%s created\n", root->name);	
	// Save the children PID's
	pid_t pid_child[root->nr_children];
	// and we create the same ammount of file discriptors
	int pfd_child[2];
	printf("Parent: Creating pipe...\n");
	if (pipe(pfd_child) < 0) {
		perror("pipe");
		exit(1);
	}
	
	// for every child we create a pipe and give the write-end to child
	for (i = 0; i < root->nr_children; i++) {	
		pid_child[i] = fork();
		if (pid_child[i] == 0){
			close(pfd_child[0]);
			fork_procs (root->children + i, pfd_child[1]);
			exit(1);
		}
	}
	close(pfd_child[1]);
	wait_for_ready_children(root->nr_children);
	// we wait for leafs to send back their "names"
	raise(SIGSTOP);
	printf("PID = %ld, name = %s is awake\n",
			(long)getpid(), root->name);
	// Char type is a hack for saving ints bigger than 0-9
	char val[2];
	for (i = 0; i < root->nr_children; i++) {
		// Its necesary to re-start children before reading
		kill(pid_child[i], SIGCONT);
		// we read from the pipe
		if (read(pfd_child[0], &val[i], sizeof(val[i])) != sizeof(val[i])) {	
			perror("child: read from pipe");
			exit(1);
		}
		printf("Parent: received value %i from the pipe. Will now compute.\n", val[i]);		
		
		pid_child[i] = waitpid(pid_child[i], &status, 0);
		explain_wait_status(pid_child[i], status);
	}	
	// Now root->name is a "special char" (int + '0') in order to transfer chars but with all the info
	char computed = mycompute(*root->name, val[0], val[1]);
	// then we write to parent using pipe
	if (write(fd, &computed, 
		sizeof(computed)) != sizeof(computed)) {
		perror("Child: write to pipe");
		exit(1);
	}
	exit(0);	

}


int main(int argc, char **argv) {
	
	int pfd[2];
	pid_t pid;
	int status;
	struct tree_node *root;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n",argv[0]);
		exit(1);
	}
	root = get_tree_from_file(argv[1]);
	printf("Parent: Creating pipe...\n");
	if (pipe(pfd) < 0) {
		perror("pipe");
		exit(1);
	}

	printf("Parent: Creating child...\n");
	pid = fork();
	if (pid < 0) { 
		perror("main:fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs(root,pfd[1]);
		exit(1);
	}
	wait_for_ready_children(1);
	show_pstree(pid);
	kill(pid, SIGCONT);
	pid = wait(&status);
	explain_wait_status(pid, status);
	printf("Parent: All done, exiting...\n");
	return 0;
}

