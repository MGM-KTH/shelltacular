
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

#define BUFSIZE 70
#define ARGSIZE 6 /* command included in ARGSIZE */
#define PROMPT "$ "

void loop();
void newline();
void prompt();
void clearargs(char **args);
void printargs(char **args);
char *getargs(char *buffer, char **args);
void spawn_command(char *cmd, char **args);
void change_dir(char *directory);
void timeval_diff(struct timeval *diff, struct timeval *tv1, struct timeval *tv2);

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
	char *cmd;
	char *args[ARGSIZE];

	/* Start by outputing prompt */
	prompt();

	for(;fgets(line_buffer, BUFSIZE, stdin); cmd=NULL, clearargs(args)) {

		cmd = getargs(line_buffer, args);

		if(NULL != cmd) {
			spawn_command(cmd, args);

		}else{newline();}

		prompt();
	}
	newline(); /* Prettier exit with newline */
}

void spawn_command(char *cmd, char **args)
{
	pid_t child_pid;
	int status, retval;
	struct timeval tv1, tv2, diff;

	if(strcmp("exit",cmd) == 0) {
            printf(": I'm afraid. I'm afraid, Dave. Dave, my mind is going.\n");
            printf("I can feel it. I can feel it. My mind is going. There is no question about it.\n");
            printf("I can feel it. I can feel it. I can feel it. I'm a... fraid.\n");
            exit(EXIT_SUCCESS);
    }

	if(strcmp("cd",cmd) == 0) {
		change_dir(args[1]);
		return;
	}

	child_pid = fork();
	if (0 == child_pid) { /* Run command in child */
		printf("%s %d\n", "Spawned foreground process with pid:", getpid());
		retval = execvp(cmd, args);
		if(-1 == retval) {
			perror("Unknown command");
		}
	}
	else { /* Wait for child in parent */
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
		printf("Foreground process %d running '%s' ran for %ld msec\n", child_pid, cmd, msec);

	}
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
 * returns the first token or NULL if no tokens were parsed
 */
char *getargs(char *buffer, char **args)
{
	int i;
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
			break;
		}

		*ptr = token;
	}

	return args[0];
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
