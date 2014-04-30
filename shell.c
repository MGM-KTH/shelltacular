/*
 * shelltacular - command interpreter (shell)
 *
 * SYNTAX
 *	   shelltacular - start the shell
 *     BUILT-IN COMMANDS
 *         cd - change working directory
 *         exit - exit the shell
 *
 *     <cmd>     - run command <cmd> as a foreground process
 *     <cmd> &   - run command <cmd> as a background process
 *
 * DESCRIPTION
 *     shelltacular is a minimalistic shell with two built-in commands,
 * 	   cd and exit. All other commands will be executed using exec.
 *     The shell supports foreground and background processes.
 *     By default, a SIGINT signal handler is registered for all processes.
 *
 * OPTIONS
 *     N/A
 *
 * SEE ALSO
 *     bash(1), wait(2), exec(3), fork(2), signal(7), getenv(3), gettimeofday(2)
 *
 * AUTHORS
 *      Gustaf Lindstedt (glindste@kth.se)
 *      Martin Runelöv (mrunelov@kth.se)
 */

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __APPLE__
#include <sys/wait.h>
#else
#include <wait.h>
#endif

#define PROCESS_TYPE_FOREGROUND 1
#define PROCESS_TYPE_BACKGROUND 2
#define BUFSIZE 70
#define ARGSIZE 6 /* command included in ARGSIZE */
#define PROMPT "$ "

#define C_FG_BLACK   "\x1b[30m"
#define C_FG_RED     "\x1b[31m"
#define C_FG_GREEN   "\x1b[32m"
#define C_FG_YELLOW  "\x1b[33m"
#define C_FG_BLUE    "\x1b[34m"
#define C_FG_MAGENTA "\x1b[35m"
#define C_FG_CYAN    "\x1b[36m"
#define C_FG_WHITE   "\x1b[37m"
#define C_FG_RESET   "\x1b[39m"
#define BOLD_ON      "\x1b[1m"
#define BLINK_ON     "\x1b[5m"
#define ALL_OFF      "\x1b[0m"

void loop();
void newline();
void prompt();
void clearargs(char **args);
void printargs(char **args);
int getargs(char *buffer, char **args);
void spawn_command(char **args, int process_type);
void change_dir(char *directory);
void spawn_foreground_process(char **args);
void spawn_background_process(char **args);
void poll_background_processes();
void timeval_diff(struct timeval *diff, struct timeval *tv1, struct timeval *tv2);
void print_color(const char *string, int fgc, int bgc, int attr);

int NUM_BACKGROUND_PROCESSES = 0;

/* 
 * Calculates the difference (tv1-tv2) between two timevals. 
 *
 * source: http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html 
 */
void timeval_diff(struct timeval *diff, struct timeval *tv1, struct timeval *tv2) {
	/* Perform the carry for the later subtraction by updating y. */
	if (tv1->tv_usec < tv2->tv_usec) {
		long int nsec = (tv2->tv_usec - tv1->tv_usec) / 1000000 + 1;
		tv2->tv_usec -= 1000000 * nsec;
		tv2->tv_sec += nsec;
	}
	if (tv1->tv_usec - tv2->tv_usec > 1000000) {
		long int nsec = (tv1->tv_usec - tv2->tv_usec) / 1000000;
		tv2->tv_usec += 1000000 * nsec;
		tv2->tv_sec -= nsec;
	}
     
	/* Compute the difference. tv_usec is positive. */
       diff->tv_sec = tv1->tv_sec - tv2->tv_sec;
       diff->tv_usec = tv1->tv_usec - tv2->tv_usec;
       return;
}

/*
 * Register a signal handler
 */
void register_sighandler( int signal_code, void (*handler) (int sig) )
{
	int retval;
	struct sigaction signal_parameters;

	signal_parameters.sa_handler = handler;
	sigemptyset(&signal_parameters.sa_mask);
	signal_parameters.sa_flags = 0;

	retval = sigaction(signal_code, &signal_parameters, NULL);

	if(-1 == retval) {
		perror("sigaction() failed");
		exit(1);
	}
}

void sigint_handler(int sig)
{
	fputs(" I'm sorry, Dave. I'm afraid I can't do that.\n", stdout); 
}

int main(int argc, char **argv)
{
	register_sighandler(SIGINT, sigint_handler);

	/*
	printf("%sCOLOR TEST\n%sBLACK %sRED %sGREEN %sYELLOW %sBLUE %sMAGENTA %sCYAN %sWHITE\n%s",
			BLINK_ON,
			C_FG_BLACK, C_FG_RED, C_FG_GREEN, C_FG_YELLOW, C_FG_BLUE,
			C_FG_MAGENTA, C_FG_CYAN, C_FG_WHITE, ALL_OFF);
			*/
	loop();
	return 0;
}

void loop()
{
	char line_buffer[BUFSIZE];
	int process_type;
	char *read_status;
	char *args[ARGSIZE];

	/* Start by outputing prompt */
	prompt();
	while(1) {
		read_status = fgets(line_buffer, BUFSIZE, stdin);
		if (read_status == NULL) {
			/*printf("Read failed\n");*/
		}
		process_type = getargs(line_buffer, args);

		if(0 < process_type) {
			spawn_command(args, process_type);
		}
		
		if (NUM_BACKGROUND_PROCESSES > 0) {
			poll_background_processes();
		}

		clearargs(args);
		prompt();
	}

	newline(); /* Prettier exit with newline */
}

void spawn_command(char **args, int process_type) 
{
	/* check if the command is the built-in command 'exit' */
	if(strcmp("exit",args[0]) == 0) {
            fputs(": I'm afraid. I'm afraid, Dave. Dave, my mind is going.\n", stdout);
            fputs("I can feel it. I can feel it. My mind is going. There is no question about it.\n", stdout);
            fputs("I can feel it. I can feel it. I can feel it. I'm a... fraid.\n", stdout);
            exit(EXIT_SUCCESS);
    }

	/* check if the command is the built-in command 'cd' */
	if(strcmp("cd",args[0]) == 0) {
		change_dir(args[1]);
		return;
	}

	/* else, create a child process to run the system command */
	if (process_type == PROCESS_TYPE_FOREGROUND) {
		spawn_foreground_process(args);
	}
	else if (process_type == PROCESS_TYPE_BACKGROUND) {
		spawn_background_process(args);
	}
}

void spawn_foreground_process(char **args) 
{
	pid_t child_pid;
	int status, retval;
	struct timeval tv1, tv2, diff;

	child_pid = fork();
	if (0 == child_pid) { /* Run command in child */
		retval = execvp(args[0], args);
		if(-1 == retval) {
			perror("Unknown command");
			exit(EXIT_FAILURE);
		}
	}
	else { /* Wait for child in parent */
		printf("%s%s %d%s\n", C_FG_YELLOW,
				"Spawned foreground process with pid:", child_pid, C_FG_RESET);
		gettimeofday(&tv1, NULL);

		if(-1 == child_pid) { 
			perror("fork() failed!");
			exit(EXIT_FAILURE);
		}

		waitpid(child_pid, &status, 0); /* 0: No options */

		/*
 		 * Calculate running time of child process
 		 */
		gettimeofday(&tv2, NULL);
		timeval_diff(&diff, &tv2, &tv1);
		long int msec = diff.tv_sec*1000000 + diff.tv_usec; /* or, %ld.%06ld seconds */
		printf("%sForeground process %d running '%s' ran for %ld msec%s\n",
				C_FG_BLUE, child_pid, args[0], msec, C_FG_RESET);
	}
	return;
}

void spawn_background_process(char **args) 
{
	pid_t child_pid;
	int retval;

	child_pid = fork();
	if (0 == child_pid) { /* Run command in child */
		retval = execvp(args[0], args);
		if(-1 == retval) {
			perror("Unknown command");
			exit(EXIT_FAILURE);
		}
	}
	else {
		++NUM_BACKGROUND_PROCESSES;
		printf("%s%s %d%s\n", C_FG_YELLOW,
				"Spawned background process with pid:", child_pid, C_FG_RESET);
	}
	return;
}

void poll_background_processes()
{
	pid_t child_pid;
	int status;
	while(1) {
		child_pid = waitpid(-1, &status, WNOHANG); /* Non-blocking polling for any child process */
		if (0 == child_pid) { /* No child processes wishes to report status */
			return;
		}
		if (-1 == child_pid) { /* received signal? */
			return;
		}
		else {
			if (WIFEXITED(status)) {
				--NUM_BACKGROUND_PROCESSES;
				printf("%sBackground process %d terminated normally%s\n",
						C_FG_BLUE, child_pid, C_FG_RESET);
			}
			else if (WIFSIGNALED(status)) {
				printf("%sBackground process %d killed by signal %d%s\n",
						C_FG_BLUE, child_pid, WTERMSIG(status), C_FG_RESET);
			}
		}
	}
}

void change_dir(char *directory)
{
	int retval;
	char buf[256];

	/*
 	 * If no directory is specified, go home.
 	 * Needs to be the first check, since strcmp fails with NULL as argument.
 	 */
	if(NULL == directory || !strcmp("~",directory)) {
		retval = chdir(getenv("HOME"));
		if(-1 == retval) {
			perror("error changing directory");
		}else{
			if(NULL != getcwd(buf, sizeof(buf))) {
				setenv("PWD", buf, 1);
			}
		}
		return;
	}

	/*
 	 * Fancy '..' macro
 	 */
	if(strcmp("..",directory) == 0) {
		directory = getenv("PWD");
		size_t i = strlen(directory);

		for(; i > 1 && directory[i] != '/'; i--) {
			/*
 			 * Find the last slash character.
			 * Stop search at i == 2, since i will then be 1 at loop exit
			 * and the last slash will be at index 0, meaning this is a
			 * reference to the root directory.
 			 */
		}
		/* Set it to terminating null byte */
		directory[i] = '\0';
	}

	retval = chdir(directory);
	if(-1 == retval) {
		perror("error changing directory");
	}else{
		if(NULL != getcwd(buf, sizeof(buf))) {
			setenv("PWD", buf, 1);
		}
	}
}

/*
 * Clear the arguments by setting the first char of
 * every arg to \0.
 */
void clearargs(char **args)
{
	int i;
	for(i = 0; i < ARGSIZE && args[i] != NULL; i++) {
		*args[i] = '\0';
	}
}

/*
 * Debug method
 */
void printargs(char **args)
{
	int i;
	for(i = 0; i < ARGSIZE && *args!=NULL && *args[0]!='\0'; ++args, ++i) {
		fprintf(stdout, "arg %d: %s\n", i, *args);
	}
}

/*
 * Extract the arguments from the buffer into args
 *
 * returns 0 if no args were parsed
 * returns 1 if command is for a foreground process
 * returns 2 if command is for a background process
 */
int getargs(char *buffer, char **args)
{
	int i;
	int process_type = PROCESS_TYPE_FOREGROUND;
	char *token;
	char *string;
	char *saveptr;

	char **ptr = args;

	for(i = 0, string = buffer;
		i < ARGSIZE;
		string = NULL, token = NULL, i++, ptr++) {

		/*
		 * 'string' should be the string to be parsed the first time
		 * the call is made. Every subsequent call string should
		 * be NULL.
		 */
		token = strtok_r(string, " \n", &saveptr);

		if(NULL == token) {
			/* No more tokens*/
			*ptr = NULL;
			if(0 == i) {
				return 0;
			}
		}
		else {
			char last_char = token[strlen(token)-1];
			if (last_char == '&') {
				process_type = PROCESS_TYPE_BACKGROUND;
				*ptr = NULL;
				break;
			}
			else {
				*ptr = token;
			}
		}
	}
	return process_type;
}

void newline()
{
	int retval;
	retval = fputs("\n", stdout);
	if(EOF == retval) {
		exit(EXIT_SUCCESS);
	}
}

void prompt()
{
	int retval;
	char prompt[256];
	prompt[0] = '\0'; /* Make sure that prompt buffer is empty */

	strcat(prompt, C_FG_MAGENTA);
	strcat(prompt, getenv("PWD")); /* Maybe use getcwd instead?? */
	strcat(prompt, C_FG_CYAN);
	strcat(prompt, PROMPT);
	strcat(prompt, C_FG_RESET);

	retval = fputs(prompt, stdout);
	if(EOF == retval) {
		exit(EXIT_SUCCESS);
	}
}
