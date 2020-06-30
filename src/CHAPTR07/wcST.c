/* Chapter 7. wcST.c										*/
/*															*/
/* wcST file1 file2 ... fileN								*/
/* WARNING: This code is NOT UNICODE enabled				*/
/*															*/
/* Parallel word count - Single thread version				*/
/*	Compare performance to wcMT.c							*/
/* Also. calling the thread function synchronously,			*/
/* rather than starting a thread, can be useful early in	*/
/* debugging so that you can test the program logic in a	*/
/* single-threaded environment before testing the thread	*/
/* management logic.										*/
/* Indpependent procesing of multiple files using			*/
/* the boss/worker model									*/
/* count the total number of characters, words, and lines	*/
/* in the files specified on the command line, just			*/
/* as is done by the UNIX wc utility						*/
/* Each file is processed by a separate function call.		*/
/* The main function (the boss function) accumulates		*/
/* the separate results to produce the final results		*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct { /* Structure for thread argument			*/
	char * filename;
	int kchar;
	int kword;
	int kline;
	int wcerror;
} THREAD_ARG;

int wcfunc (void *);

int main (int argc, char * argv[])

{
	int ithrd, tstatus;
	int nchar = 0, nword = 0, nline = 0;
	THREAD_ARG * targ;
	
	if (argc < 2) {
		printf ("Usage: wcST filename ... filename\n");
		return 1;
	}
	
	/* Allcoate a thread reference and a thread arg for 	*/
	/* each worker function call				*/
	targ = (THREAD_ARG *) calloc (argc-1, sizeof (THREAD_ARG));
	if (targ == NULL) {
		printf ("Cannot allocate wokring memory\n");
		return 2;
	}
	
	/* Count each file on the command line */
	for (ithrd = 0; ithrd < argc - 1; ithrd++) {
		targ[ithrd].filename = argv[ithrd+1];
		/* Create a worker thread to process the file */
		tstatus = wcfunc ((void *)&targ[ithrd]);
		if (tstatus != 0)
			printf ("Word count failed, file: %s\n",
				targ[ithrd].filename);
		nchar += targ[ithrd].kchar;
		nword += targ[ithrd].kword;
		nline += targ[ithrd].kline;
		printf ("%10d %9d %9d %s\n", targ[ithrd].kline,
			targ[ithrd].kword, targ[ithrd].kchar,
			targ[ithrd].filename);
	}
	
	
	free (targ); /* Threads have been detached by join 	*/
	printf ("%10d %9d %9d \n", nline, nword, nchar);
	return 0;
}

int wcfunc (void * arg)
/* Count the number of characters, works, and lines in		*/
/* the file targ->filename					*/
/* NOTE: Simple version; results may differ from wc utility	*/
{
	FILE * fin;
	THREAD_ARG * targ;
	int ch, c, nl, nw, nc;
	
	targ = (THREAD_ARG *)arg;
	targ->wcerror = 1; /* Error for now */
	fin = fopen (targ->filename, "r");
	if (fin == NULL) return 1;
	
	ch = nw = nc = nl = 0;
	while (!feof (fin)) {
		c = getc (fin);
		if (c == '\0') break;
		if (isspace(c) && isalpha(ch))
			nw++;
		ch = c;
		nc++;
		if (c == '\n')
			nl++;
	}
	fclose (fin);
	targ->kchar = nc; targ->kword = nw;
	targ->kline = nl;
	targ->wcerror = 0;
	return 0;
}
