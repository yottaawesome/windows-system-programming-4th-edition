/****	Copyright 1995, Alan R. Feuer	****/
/**** THIS IS NOT UNICODE ENABLED ****/
/* grep pattern file(s)
Looks for pattern in files. A file is scanned line-by-line.

These metacharacters are used:
*	matches zero or more characters
?	matches exactly one character
[...]	specifies a character class
matches any character in the class
^	matches the beginning of the line
$	matches the end of the line

In addition, the standard C escape sequences are understood:
\a, \b, \f, \t, \v
*/

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Codes for pattern meta characters. */
#define STAR        1
#define QM          2
#define BEGCLASS    3
#define ENDCLASS    4
#define ANCHOR      5

/* Other codes and definitions. */
#define EOS '\0'

/* Options for pattern match. */
BOOL ignoreCase = FALSE;

FILE * openFile (char * file, char * mode) {
	FILE *fp;
	if ((fp = fopen (file,mode)) == NULL)
		perror (file);
	return(fp);
}

void
prepSearchString (char *p, char *buf) {
	/* Copy prep'ed search string to buf. */
	register int c;
	register int i = 0;

	if (*p=='^') {
		buf [i++] = ANCHOR;
		++p;
	}

	for (;;) {
		switch (c=*p++) {
		case EOS: goto Exit;
		case '*': if (i >= 0 && buf [i - 1] != STAR ) c = STAR; break;
		case '?': c = QM; break;
		case '[': c = BEGCLASS; break;
		case ']': c = ENDCLASS; break;

		case '\\':
			switch( c = *p++ ) {
		case EOS: goto Exit;
		case 'a': c = '\a'; break;
		case 'b': c = '\b'; break;
		case 'f': c = '\f'; break;
		case 't': c = '\t'; break;
		case 'v': c = '\v'; break;
		case '\\': c = '\\'; break;
			}
			break;
		}

		buf [i++] = (ignoreCase ? tolower (c) : c);
	}

Exit:
	buf [i] = EOS;
}

BOOL
patternMatch (char *pattern, char *string)
/* Return TRUE if pattern matches string. */
{
	register char pc, sc;
	char *pat;
	BOOL anchored;

	if (anchored = (*pattern == ANCHOR))
		++pattern;

Top:		/* Once per char in string. */
	pat = pattern;

Again:
	pc = *pat;
	sc = *string;

	if (sc == '\n' || sc == EOS ) {
		/* At end of line or end of text. */
		if (pc==EOS) goto Success;
		else if (pc==STAR) {
			/* patternMatch (pat + 1, base, index, end) */
			++pat;
			goto Again;
		} else return (FALSE);
	} else {
		if (pc == sc || pc == QM) {
			/* patternMatch (pat + 1, string + 1) */
			++pat;
			++string;
			goto Again;
		} else if (pc == EOS) goto Success;
		else if (pc == STAR) {
			if (patternMatch (pat + 1, string)) goto Success;
			else { /* patternMatch (pat, string + 1) */
				++string;
				goto Again;
			}
		} else if(pc == BEGCLASS) { /* char class */
			BOOL clmatch = FALSE;
			while (*++pat != ENDCLASS) {
				if (!clmatch && *pat == sc) clmatch = TRUE;
			}
			if(clmatch) {
				++pat;
				++string;
				goto Again;
			}
		}
	}

	if (anchored) return(FALSE);

	++string;
	goto Top;

Success:
	return (TRUE);
}

void main (int argc, char **argv)

/* This version is modified to use stdout exclusively.
Only the pattern should be on the command line. */
{
	int i, patternSeen = FALSE, showName = FALSE, result = 1, iFile;
	char pattern [256];
	char string [2048];
	FILE *fp;

	for (iFile = 2; iFile < argc; iFile++)
	{
		fp = openFile (argv [iFile], "r"); /* Input file. */
		if (fp == NULL)	{
			perror ("File open failure\n");
			fputs ("", stdout);
			fputc ('\0', stdout);
			exit (1);
		}

		if (argc < 1) {
			puts ("Usage: grep pattern file(s)");
			exit (2);
		}

		for (i = 1; i < argc + 1; ++i) {
			if (i < argc && argv [i] [0] == '-') {
				switch (argv [i] [1]) {
				case 'y':
					ignoreCase = TRUE;
					break;
				}
			} else {
				if (!patternSeen++)
					prepSearchString (argv [i], pattern);
				else if(/*(fp=openFile(file=argv[i],"rb"))!=NULL*/ TRUE) {
					if (!showName && i < argc - 1 ) ++showName;
					while (fgets (string, sizeof (string), fp) != NULL
						&& !feof (fp)) {
							if (ignoreCase) _strlwr (string);
							if (patternMatch (pattern, string)) {
								result = 0;
								if (showName) {
									fputs (string, stdout);
								}
								else fputs (string,stdout);
							}
					}
				}
			}
		}
		fputc ('\n', stdout);
		fclose (fp);
	}
	exit(result);
}

