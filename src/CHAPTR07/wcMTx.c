/* Chapter 7. wcMTx.c										*/
/*		INTENTIONALLY DEFECTIVE. FIND AND FIX THE ERRORS	*/
/*      ALSO - the source is not UNICODE enabled with _T    */
/* wcMT file1 file2 ... fileN								*/
/*															*/
/* Parallel word count - mulitple thread version			*/
/*															*/
/* Indpependent procesing of multiple files using			*/
/* the boss/worker model									*/
/* count the total number of characters, words, and lines	*/
/* in the files specified on the command line, just			*/
/* as is done by the UNIX wc utility						*/
/* Each file is processed by a separate worker thread.		*/
/* The main thread (the boss thread) accumulates			*/
/* the separate results to produce the final results		*/

#include "Everything.h"

typedef struct { /* Structure for thread argument		*/
	char * filename;
	volatile int kchar;
	volatile int kword;
	volatile int kline;
	volatile int wcerror;
} THREAD_ARG;

DWORD WINAPI wcfunc (void *);

int main (int argc, char * argv[])

{
	HANDLE *tref;
	DWORD ithrd, tstatus, tid;
	DWORD nchar = 0, nword = 0, nline = 0;
	THREAD_ARG * targ;
	
	if (argc < 2) {
		printf ("Usage: wcMTx filename ... filename\n");
		return 1;
	}
	
	/* Allcoate a thread reference and a thread arg for 	*/
	/* each worker thread					*/
	tref = (HANDLE *) calloc (argc-1, sizeof(HANDLE));
	targ = (THREAD_ARG *) calloc (argc-1, sizeof (THREAD_ARG));
	if (tref == NULL || targ == NULL) {
		printf ("Cannot allocate wokring memory\n");
		return 2;
	}
	
	/* Create a worker thread for each file on the command line */
	for (ithrd = 0; ithrd < (DWORD)argc - 1; ithrd++) {
		/* Create a worker thread to process the file */
		tref[ithrd] = (HANDLE)_beginthreadex (NULL, 0, wcfunc, (PVOID)targ[ithrd],
			0, &tid);
		targ[ithrd].filename = argv[ithrd+1];
	}
	
	/* Worker threads are all running. Wait for them 	*/
	/* to complete and accumulate the results		*/
	for (ithrd = 0; ithrd < (DWORD)argc - 1; ithrd++) {
		tstatus = WaitForSingleObject (tref[ithrd], INFINITE);
		/* Report failure but continue 		*/
		if (tstatus != WAIT_OBJECT_0) 
			ReportError ("Cannot wait for thread", 0, TRUE);
		nchar += targ[ithrd].kchar;
		nword += targ[ithrd].kword;
		nline += targ[ithrd].kline;
		printf ("%10d %9d %9d %s\n", targ[ithrd].kline,
			targ[ithrd].kword, targ[ithrd].kchar,
			targ[ithrd].filename);
	}
	
	free (tref); /* Deallocate memory resources 		*/
	free (targ); /* Threads have been detached by join 	*/
	printf ("%10d %9d %9d \n", nline, nword, nchar);
	return 0;
}

static int ch, c, nl, nw, nc;

DWORD WINAPI wcfunc (void * arg)
/* Count the number of characters, works, and lines in		*/
/* the file targ->filename					*/
/* NOTE: Simple version; results may differ from wc utility	*/
{
	FILE * fin;
	THREAD_ARG * targ;
	
	targ = (THREAD_ARG *)arg;
	targ->wcerror = 1; /* Error for now */
	fin = fopen (targ->filename, "r");
	if (fin == NULL) return (targ->wcerror);
	
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
