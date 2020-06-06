#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>
#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */


struct Node {
	pid_t *p;
	struct Node *next;
	struct Node *prev;
}*head,*tail,*cur;
/*
 * SIGALRM handler
 */
void nextScheduled(void);
static void sigalrm_handler(int signum)
{
	if (signum != SIGALRM){
		fprintf(stderr,"Internal error: Called for signum %d, not SIGALRM\n",signum);	
		exit(1);
	}
	else {
		kill(*(cur->p),SIGSTOP);
	}
}

/* 
 * SIGCHLD handler
 */
static void sigchld_handler(int signum)
{
	if (signum != SIGCHLD){
		fprintf(stderr,"Internal error: Called for signum %d, not SIGCHLD\n",signum);	
		exit(1);
	}
	else{	
		while (true){
			int status = 0;
			int p = waitpid(-1, &status, WUNTRACED | WNOHANG);
			if (p < 0) {
				perror("waitpid"); 
				exit(1);
			}
			else if (p == 0) break;
			explain_wait_status(p, status);
			if (WIFEXITED(status) || WIFSIGNALED(status)){
				deleteNode(cur);
				nextScheduled();
				kill(*(cur->p),SIGCONT);
				alarm(SCHED_TQ_SEC);
			}
			if (WIFSTOPPED(status)){
				nextScheduled();
				kill(*(cur->p),SIGCONT);
				alarm(SCHED_TQ_SEC);
			}
		}
	}
}
void *safe_malloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL) {
		fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
			size);
		exit(1);
	}

	return p;
}
/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void install_signal_handlers(void)
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
void createNode (struct Node *node);
void deleteNode (struct Node *node);
void childProc (pid_t *p, char *argv);


int main(int argc, char *argv[])
{
	int nproc = 0,i;
	printf("Args Num: %i\n",argc);

	// Initialize DLL
	head = safe_malloc(sizeof(*head));
	tail = safe_malloc(sizeof(*tail));
	head->prev = NULL;
	head->next = tail;
	tail->next = NULL;
	tail->prev = head;
	cur = head;

	for (i = 1; i < argc; i++){
		struct stat sb;
		printf("Argument %i -> \"%s\"",i,argv[i]);
		// Check if the argument is executable
		if (stat(argv[i], &sb) == 0 && sb.st_mode & S_IXUSR){
			createNode(cur->next);
			cur->p = safe_malloc(sizeof(pid_t));
			childProc(&*(cur->p),argv[i]);
			printf("Parrent:  Is Executable with node PID:  %i\n", *(cur->p));
			nproc++;
		}
		else 
			printf("Given Argument Is NOT Executable\n");
	}
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);
	cur = head->next;
	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}


	else { 
		kill(*(cur->p),SIGCONT);
		alarm(SCHED_TQ_SEC);
	}	
	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
void nextScheduled(void){
	if (cur == tail && head->next == tail) {free(head);free(tail);exit(0);}
	else if (cur == tail && head->next != tail) cur = head->next;
	else cur = (cur->next == tail) ? head->next : cur->next;
}
void createNode (struct Node *node){
	node = safe_malloc(sizeof(*node));
//	node->p = p;
	node->prev = cur;
	node->next = tail;
	tail->prev = node;
	cur->next = node;
	cur = node;
	printf("Created Node at memory place : %li\n",(long int)node);
}
void deleteNode (struct Node *node){
	cur = node->next;
	node->prev->next = cur;
	cur->prev = node->prev;
	free(node);
	printf("Forever Free!!! Mem: %li\n",(long int)node);
}
void childProc (pid_t *p, char *argv){
	*p = fork();
	if (*p < 0) {
		perror("scheduler: fork");
		exit(1);
	}
	if (*p == 0){
		char *newargv[] = { argv, NULL, NULL, NULL };
		char *newenviron[] = { NULL };
		printf("\nChild HERE PID: %li!!!\n",(long int)getpid());
		kill(getpid(),SIGSTOP);
		execve(argv,newargv,newenviron);
	}
}
