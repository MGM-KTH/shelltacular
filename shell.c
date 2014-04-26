
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

#define BUFSIZE 70
#define ARGSIZE 6
#define PROMPT "$"

void loop();
void newline();
void prompt();
void printargs(char **args);
char *getargs(char *buffer, char **args);

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
	fputs("\n", stdout);
	loop();
}

int main(int argc, char **argv)
{
	register_sighandler(SIGINT, sigint_handler);
	loop();
	return 0;
}

void loop()
{
	char line_buffer[BUFSIZE];
	char *args[ARGSIZE];
	char *cmd;

	/* Start by outputing prompt */
	prompt();

	for(;fgets(line_buffer, BUFSIZE, stdin); cmd=NULL) { 

		cmd = getargs(line_buffer, args);

		if(NULL != cmd) {

			printargs(args);

		}
		newline();
		prompt();
	}
	newline(); /* Prettier exit with newline */
}

/*
 * Debug method
 */
void printargs(char **args)
{
	int i;
	for(i = 0; i < ARGSIZE && *args!=NULL; ++args, ++i) {
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
		token = strtok_r(string, " ", &saveptr);

		/*
		 * 0x0A is the LF character, which apparently is the first
		 * token in an empty line, and the token returned if line
		 * ends with a space character.
		 */
		if(NULL == token || !strcmp("\x0A",token)) {
			/* No more tokens*/
			break;
		}else if(!strcmp("\x0A",token) && i == 0) {
			/* Empty command */
			return NULL;
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
		exit(0);
	}
}

void prompt()
{
	int retval;
	retval = fputs(PROMPT, stdout);
	if(EOF == retval) {
		exit(0);
	}
}
