/* Partial implementation of the UNIX wc command. */
/* Counts the number of lines, words, and characters from standard input. */
/* This program is Figure 18 in "Using Program
	Slicing in Software Maintenance" by K.B. Gallagher
	and J.R. Lyle, IEEE Transactions on Software Engineering,
	Vol 17, No 8, Aug, 1991. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

int main (int argc, char * argv[])
{
	FILE * fin;
	int ch, c, nl, nw, nc;

	if (argc == 1) fin = stdin;
	else fin = fopen (argv[1], "r");
	if (fin == NULL) 
		perror ("wc: input file failure\n");

	ch = nw = nc = nl = 0;
	while (!feof (fin) /* c != EOF */ ) {
		c = getc (fin);
		if (c == '\0') break;
		if (isspace(c) && isalpha(ch))
			nw = nw + 1;
		ch = c;
		nc = nc + 1;
		if (c == '\n')
			nl = nl + 1;
	}
	if (fin != stdin) fclose (fin);
	printf ("\n %d %d %d\n", nl, nw, nc);
	return 0;
}