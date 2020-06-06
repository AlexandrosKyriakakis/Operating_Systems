#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
	/*
	 * initial process is A.
	 */
	pid_t B_pid, C_pid, D_pid;
	int status;
	change_pname("A");
	printf("A created\n");
	B_pid = fork(); 
	if (B_pid < 0) {
		perror("fork creating tree");
		exit(1);
	}
	if (B_pid == 0) {
		change_pname("B");
		printf("B created\n");
		D_pid = fork(); 
		if (D_pid < 0) {
			perror("fork creating tree");
			exit(1);
		}
		if (D_pid == 0) {
			change_pname("D");
			printf("D created\n");
			printf("D sleeping\n");
			sleep(SLEEP_PROC_SEC);
			printf("D exiting\n");
			exit(13);
		}
		if (D_pid > 0) {
			printf("B waiting\n");
			D_pid = wait(&status);
			explain_wait_status(D_pid, status);
			printf("B exiting\n");
			exit(19);			
		}
	}
	if (B_pid > 0) {
		C_pid = fork();
		if (C_pid < 0) {
			perror("fork creating tree");
			exit(1);
		}
		if (C_pid == 0) {
			change_pname("C");
			printf("C created\n");
			printf("C sleeping\n");
			sleep(SLEEP_PROC_SEC);
			printf("C exiting\n");
			exit(17);
		}
		if (C_pid > 0) {
			printf("A waiting\n");
			C_pid = waitpid(-1, &status, 0);
			explain_wait_status(C_pid, status);
			C_pid = waitpid(-1, &status, 0);			
			explain_wait_status(C_pid, status);
			printf("A exiting\n");
			exit(16);
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
int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs();	
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	/* wait_for_ready_children(1); */

	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	/* kill(pid, SIGCONT); */

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
