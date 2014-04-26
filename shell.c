
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

#define BUFSIZE 256
#define PROMPT "$"

void command_loop();
void newline();
void prompt();

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
    command_loop();
}

int main(int argc, char **argv)
{
    register_sighandler(SIGINT, sigint_handler);
    command_loop();
    return 0;
}

void command_loop()
{
    char command_buffer[BUFSIZE];

    /* Start by outputing prompt */
    prompt();

    while(fgets(command_buffer, BUFSIZE, stdin)) { 
        newline();
        prompt();
    }
    newline(); /* Prettier exit with newline */
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
