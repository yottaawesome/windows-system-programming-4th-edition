/* Chapter 7. grepMT. */
/* Parallel grep - Multiple thread version. */

/* grep pattern files.
	Search one or more files for the pattern.
	The results are listed in the order in which the
	threads complete, not in the order the files are
	on the command line. This is primarily to illustrate
	the non-determinism of thread completion. To obtain
	ordered output, use the technique in Program 8-1. */

/* Be certain to define _MT in Environment.h
	or use the build...settings...C/C++...CodeGeneration...Multithreaded library. */

#include "Everything.h"

typedef struct {	/* grep thread's data structure. */
	int argc;
	TCHAR targv[4][MAX_COMMAND_LINE];
} GREP_THREAD_ARG;

typedef GREP_THREAD_ARG *PGR_ARGS;
static DWORD WINAPI ThGrep (PGR_ARGS pArgs);

VOID _tmain (int argc, LPTSTR argv[])

/* Create a separate THREAD to search each file on the command line.
	Report the results as they come in.
	Each thread is given a temporary file, in the current
	directory, to receive the results.
	This program modifies Program 8-1, which used processes. */
{
	PGR_ARGS gArg;		/* Points to array of thread args. */ 
	HANDLE * tHandle;	/* Points to array of thread handles. */
	TCHAR commandLine[MAX_COMMAND_LINE];
	BOOL ok;
	DWORD threadIndex, exitCode;
	int iThrd, threadCount;
	STARTUPINFO startUp;
	PROCESS_INFORMATION processInfo;

	/* Start up info for each new process. */

	GetStartupInfo (&startUp);

	if (argc < 3)
		ReportError (_T ("No file names."), 1, TRUE);
	
	/* Create a separate "grep" thread for each file on the command line.
		Each thread also gets a temporary file name for the results.
		argv[1] is the search pattern. */

	tHandle = malloc ((argc - 2) * sizeof (HANDLE));
	gArg = malloc ((argc - 2) * sizeof (GREP_THREAD_ARG));

	for (iThrd = 0; iThrd < argc - 2; iThrd++) {

			/* Set:	targv[1] to the pattern
				targv[2] to the input file
				targv[3] to the output file. */

		_tcscpy (gArg[iThrd].targv[1], argv[1]); /* Pattern. */
		_tcscpy (gArg[iThrd].targv[2], argv[iThrd + 2]); /* Search file. */

		if (GetTempFileName	/* Temp file name */
				(".", "Gre", 0,	gArg[iThrd].targv[3]) == 0)
			ReportError (_T ("Temp file failure."), 3, TRUE);

		/* Output file. */

		gArg[iThrd].argc = 4;

		/* Create a thread to execute the command line. */

		tHandle[iThrd] = (HANDLE)_beginthreadex (
				NULL, 0, ThGrep, &gArg[iThrd], 0, NULL);

		if (tHandle[iThrd] == 0)
			ReportError (_T ("ThreadCreate failed."), 4, TRUE);
	}

	/* Threads are all running. Wait for them to complete
	 	one at a time and put out the results. */
	/*  Redirect output for "cat" process listing results. */

	startUp.dwFlags = STARTF_USESTDHANDLES;
	startUp.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	startUp.hStdError = GetStdHandle (STD_ERROR_HANDLE);

	threadCount = argc - 2;
	while (threadCount > 0) {
		threadIndex = WaitForMultipleObjects (threadCount, tHandle, FALSE, INFINITE);
		iThrd = (int) threadIndex - (int) WAIT_OBJECT_0;
		if (iThrd < 0 || iThrd >= threadCount)
			ReportError (_T ("Thread wait error."), 5,TRUE);
		GetExitCodeThread (tHandle[iThrd], &exitCode);
		CloseHandle (tHandle[iThrd]);
		
		/* List file contents (if a pattern match was found)
			and wait for the next thread to terminate. */

		if (exitCode == 0) {
			if (argc > 3) {		/* Print  file name if more than one. */
				_tprintf (_T ("%s\n"),
						gArg[iThrd].targv[2]);
				fflush (stdout);
			}

			_stprintf (commandLine, _T ("cat \"%s\""), gArg[iThrd].targv[3]);

			ok = CreateProcess (NULL, commandLine, NULL, NULL,
					TRUE, 0, NULL, NULL, &startUp, &processInfo);

			if (!ok) ReportError (_T ("Failure executing cat."), 6, TRUE);
			WaitForSingleObject (processInfo.hProcess, INFINITE);
			CloseHandle (processInfo.hProcess);
			CloseHandle (processInfo.hThread);
		}

		if (!DeleteFile (gArg[iThrd].targv[3]))
			ReportError (_T ("Cannot delete temp file."), 7, TRUE);

		/* Move the handle of the last thread in the list
			to the slot occupied by thread that just completed
			and decrement the thread count. Do the same for
			the temp file names. */

		tHandle[iThrd] = tHandle[threadCount - 1];
		_tcscpy (gArg[iThrd].targv[3], gArg[threadCount - 1].targv[3]);
		_tcscpy (gArg[iThrd].targv[2], gArg[threadCount - 1].targv[2]);
		threadCount--;
	}
	free (tHandle);
	free (gArg);
}

/* Source code for grep follows and is omitted from text. */
/* The form of the code is:
	static DWORD WINAPI ThGrep (GR_ARGS pArgs)
	{
	}
*/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*	grep, written as a function to be executed on a thread. */
/*	Copyright 1995, Alan R. Feuer. */
/*	Modified version: The input and output file names
	are taken from the argument data structure for the file.
	This function uses the C library and therefore must
	be invoked by _beginthreadex. */

/*	grep pattern file(s)
	Looks for pattern in files. A file is scanned line-by-line.

	These metacharacters are used:
		*	matches zero or more characters
		?	matches exactly one character
		[...]	specifies a character class
			matches any character in the class
		^	matches the beginning of the line
		$	matches the end of the line
	In addition, the standard C escape sequences are
	understood: \a, \b, \f, \t, \v */

/*	Codes for pattern metacharacters. */

#define ASTRSK		1
#define QM		2
#define BEGCLASS	3
#define ENDCLASS	4
#define ANCHOR		5

FILE * openFile (char *, char *);
void prepSearchString (char *, char *);

BOOL patternMatch (char *,char *);

/* Other codes and definitions. */

#define EOS _T('\0')

/* Options for pattern match. */

static BOOL ignoreCase = FALSE;

static DWORD WINAPI ThGrep (PGR_ARGS pArgs)
{
	/* Modified version - the first argument is the
		pattern, the second is the input file and the
		third is the output file.
		argc is not used but is assumed to be 4. */

	char *file;
	int i, patternSeen = FALSE, showName = FALSE, argc, result = 1;
	char pattern[256];
	char string[2048];
	TCHAR argv[4][MAX_COMMAND_LINE];
	FILE *fp, *fpout;

	argc = pArgs->argc;
	_tcscpy (argv[1], pArgs->targv[1]);
	_tcscpy (argv[2], pArgs->targv[2]);
	_tcscpy (argv[3], pArgs->targv[3]);
	if (argc < 3 ) {
		puts ("Usage: grep output_file pattern file(s)");
		return 1;
	}

	/* Open the output file. */

	fpout = openFile (file = argv[argc - 1],"wb");
	if (fpout == NULL) {
		printf ("Failure to open output file.");
		return 1;
	}

	for (i = 1; i < argc - 1; ++i ) {
		if (argv[i][0] == _T('-') ) {
			switch (argv[i][1] ) {
				case _T('y'):
					ignoreCase = TRUE;
					break;
				}
		} else {
			if (!patternSeen++)
				prepSearchString (argv[i], pattern);
			else if ((fp = openFile (file = argv[i], _T("rb")))
					!= NULL ) {
				if (!showName && i < argc - 2 ) ++showName;
				while (fgets (string, sizeof (string), fp)
						!= NULL && !feof (fp)) {
					if (ignoreCase) _strlwr (string);
					if (patternMatch (pattern, string)) {
						result = 0;
						if (showName) {
							fputs (file,fpout);
							fputs (string, fpout);
						} else fputs (string, fpout);
					}
				}
				fclose (fp);
				fclose (fpout);
			}
		}
	}
	return result;
}

static FILE *
openFile (char * file, char * mode)
{
	FILE *fp;

	/* printf ("Opening File: %s", file); */

	if ((fp = fopen (file, mode)) == NULL )
		perror (file);
	return (fp);
}

static void
prepSearchString (char *p, char *buf)

/* Copy prep'ed search string to buf. */
{
	register int c;
	register int i = 0;

	if (*p == _T('^') ) {
		buf[i++] = ANCHOR;
		++p;
	}

	for(;;) {
		switch (c = *p++) {
		case EOS: goto Exit;
		case _T('*'): if (i >= 0 && buf[i - 1] != ASTRSK)
				c = ASTRSK; break;
		case _T('?'): c = QM; break;
		case _T('['): c = BEGCLASS; break;
		case _T(']'): c = ENDCLASS; break;

		case _T('\\'):
			switch (c = *p++) {
			case EOS: goto Exit;
			case _T('a'): c = _T('\a'); break;
			case _T('b'): c = _T('\b'); break;
			case _T('f'): c = _T('\f'); break;
			case _T('t'): c = _T('\t'); break;
			case _T('v'): c = _T('\v'); break;
			case _T('\\'): c = _T('\\'); break;
			}
			break;
		}

		buf[i++] = (ignoreCase ? tolower (c) : c);
	}

Exit:
	buf[i] = EOS;
}

static BOOL
patternMatch (char *pattern, char *string)

	/* Return TRUE if pattern matches string. */
{
	register char pc, sc;
	char *pat;
	BOOL anchored;

	if (anchored = (*pattern == ANCHOR))
		++pattern;

Top:			/* Once per char in string. */
	pat = pattern;

Again:
	pc = *pat;
	sc = *string;

	if (sc == _T('\n') || sc == EOS ) {
				/* at end of line or end of text */
		if (pc == EOS ) goto Success;
		else if (pc == ASTRSK ) {
				/* patternMatch (pat + 1,base, index, end) */
			++pat;
			goto Again;
		} else return (FALSE);
	} else {
		if (pc == sc || pc == QM ) {
					/* patternMatch (pat + 1,string + 1) */
			++pat;
			++string;
			goto Again;
		} else if (pc == EOS) goto Success;
		else if (pc == ASTRSK ) {
			if (patternMatch (pat + 1,string)) goto Success;
			else {
					/* patternMatch (pat, string + 1) */
				++string;
				goto Again;
			}
		} else if (pc == BEGCLASS ) { /* char class */
			BOOL clmatch = FALSE;
			while (*++pat != ENDCLASS ) {
				if (!clmatch && *pat == sc ) clmatch = TRUE;
			}
			if (clmatch) {
				++pat;
				++string;
				goto Again;
			}
		}
	}

	if (anchored) return (FALSE);

	++string;
	goto Top;

Success:
	return (TRUE);
}

