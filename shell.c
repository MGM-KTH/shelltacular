#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define BUFSIZE 256

void command_loop();

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
    int retval;
    while(fgets(command_buffer, BUFSIZE, stdin)) { 
        retval = fputs("\n", stdout);
        if(EOF == retval) {
            exit(0);
        }
    }
}
