
/*
 *
 * 1) Läsa in ett kommando från kommandoraden
 * 2) Parsa (tolka) kommandot för att se om det är ett inbyggt kommando eller ej
 * 3a) Om det var ett inbyggt kommando utför det t.ex genom en sekvens av systemanrop - ta hand om eventuella fel.
 * 3b) Det var inte ett inbyggt kommando. Skapa en ny process att exekvera kommandot i. I den nya processen byter man exekverande program till det program kommandot avsåg (första argumentet på kommandoraden). Se till att alla parametrar till kommandot kommer med! Själva kommandotolken skall sedan antingen vänta på att den nya processen avslutas om kom- mandot exekveras i förgrunden eller direkt ge en ny prompt om det exekveras i bakgrunden.
 * 4) Innan den nya prompten ges skall man dock göra följande:
 *     i) Om det var ett förgrundskommando så skriv ut statistik om hur lång tid processen var igång
 *     ii) kontrollera om några bakgrundsprocesser avslutast och skriv ut information om dessa.
 * 5) om kommandot exit ges avslutas kommandotolken.
 */

#define _POSIX_C_SOURCE 199506L

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

#define PROCESS_KIND_FOREGROUND 1
#define PROCESS_KIND_BACKGROUND 2
#define BUFSIZE 70
#define ARGSIZE 6 /* command included in ARGSIZE */
#define PROMPT "$ "

void loop();
void newline();
void prompt();
void clearargs(char **args);
void printargs(char **args);
int getargs(char *buffer, char **args);
void spawn_command(char **args, int process_kind);
void change_dir(char *directory);
void spawn_foreground_process(char **args);
void spawn_background_process(char **args);
void poll_background_processes();
void timeval_diff(struct timeval *diff, struct timeval *tv1, struct timeval *tv2);
void print_color(const char *string, int fgc, int bgc, int attr);

int NUM_BACKGROUND_PROCESSES = 0;

/* 
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
     
	/* Compute the time remaining to wait.
		tv_usec is certainly positive. */
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

	retval = sigaction(signal_code, &signal_parameters, (void *) NULL);

	if(-1 == retval) {
		perror("sigaction() failed");
		exit(1);
	}
}

void sigint_handler(int sig)
{
	/* fputs("I'm sorry, Dave. I'm afraid I can't do that.\n", stdout); */
	loop();
}

int main(int argc, char **argv)
{
	/* register_sighandler(SIGINT, sigint_handler); */
	loop();
	return 0;
}

void loop()
{
	char line_buffer[BUFSIZE];
	int process_kind;
	char *args[ARGSIZE];

	/* Start by outputing prompt */
	prompt();

	for(;fgets(line_buffer, BUFSIZE, stdin); process_kind=0, clearargs(args)) {

		process_kind = getargs(line_buffer, args);

		if(0 < process_kind) {

			spawn_command(args, process_kind);

		}

		if (NUM_BACKGROUND_PROCESSES > 0) {
			poll_background_processes();
		}
		prompt();
	}

	newline(); /* Prettier exit with newline */
}

void spawn_command(char **args, int process_kind) 
{
	/* check if the command is the built-in command 'exit' */
	if(strcmp("exit",args[0]) == 0) {
            printf(": I'm afraid. I'm afraid, Dave. Dave, my mind is going.\n");
            printf("I can feel it. I can feel it. My mind is going. There is no question about it.\n");
            printf("I can feel it. I can feel it. I can feel it. I'm a... fraid.\n");
            exit(EXIT_SUCCESS);
    }

	/* check if the command is the built-in command 'cd' */
	if(strcmp("cd",args[0]) == 0) {
		change_dir(args[1]);
		return;
	}



	/* else, create a child process to run the system command */
	printf("process kind: %d\n", process_kind);
	if (process_kind == PROCESS_KIND_FOREGROUND) {
		spawn_foreground_process(args);
	}
	else if (process_kind == PROCESS_KIND_BACKGROUND) {
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
		printf("%s %d\n", "Spawned foreground process with pid:", child_pid);
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
		printf("Foreground process %d running '%s' ran for %ld msec\n", child_pid, args[0], msec);
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
		printf("%s %d\n", "Spawned background process with pid:", child_pid);
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
			if WIFEXITED(status) {
				--NUM_BACKGROUND_PROCESSES;
				printf("Background process %d terminated\n", child_pid);
			}
		}
	}
}

void print_color(const char *string, int fgc, int bgc, int attr)
{
	printf("\x1b[%dm\x1b[%dm%s\x1b[%dm\x1b[%dm",
			fgc, bgc, string, 37, 40);
}

void change_dir(char *directory)
{
	int retval;

	/*
 	 * If no directory is specified, go home.
 	 * Needs to be the first check, since strcmp fails with NULL as argument.
 	 */
	if(NULL == directory || !strcmp("~",directory)) {
		retval = chdir(getenv("HOME"));
		if(-1 == retval) {
			perror("error changing directory");
		}
		return;
	}

	/*
 	 * Fancy '..' macro
 	 */
	if(strcmp("..",directory) == 0) {
		directory = getenv("PWD");
		size_t i = strlen(directory);

		for(; directory[i] != '/'; i--) {
			/* Find the last slash character */
		}
		/* Set it to terminating null byte */
		directory[i] = '\0';
	}

	retval = chdir(directory);
	if(-1 == retval) {
		perror("error changing directory");
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
	int process_type = PROCESS_KIND_FOREGROUND;
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
				process_type = PROCESS_KIND_BACKGROUND;
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

	retval = fputs(PROMPT, stdout);
	if(EOF == retval) {
		exit(EXIT_SUCCESS);
	}
}
