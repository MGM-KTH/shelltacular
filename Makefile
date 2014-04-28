all:
	gcc -Wall -ansi -o shell shell.c -D_POSIX_C_SOURCE=199506L