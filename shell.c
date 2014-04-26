#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 256

void command_loop();

int main(int argc, char **argv) {
    command_loop();
    return 0;
}

void command_loop() {
    char command_buffer[BUFSIZE];
    int retval;
    while(fgets(command_buffer, BUFSIZE, stdin)) { 
        retval = fputs("\n", stdout);
        if(EOF == retval) {
            exit(0);
        }
    }
}
