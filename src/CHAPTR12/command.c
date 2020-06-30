/* Chapter 12. commands.c									*/
/* WARNING: THESE COMMANDS ARE NOT UNICODE ENABLED			*/
/*															*/
/* "In Process Server" commands to use with serverSK, etc.	*/
/*															*/
/* There are several commands implemented as DLLs			*/
/* Each command function must be a thread-safe function 	*/
/* and take two parameters. The first is a string:			*/
/* command arg1 arg2 ... argn (i.e.; a RESTRICTED command	*/
/* line with no spaces or quotes in the command or args)	*/
/* and the second is the file name for the output			*/
/* The code is C, without decorated names					*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void ExtractToken (int, char *, char *);

__declspec (dllexport)
int __cdecl wcip (char * command, char * output_file)
/* word count; in process (ONE FILE ONLY) - Not UNICODE		*/
/* Count the number of characters, works, and lines in		*/
/* the file specified as the second token in "command"		*/
/* NOTE: Simple version; results may differ from wc utility	*/
/* This implementation DOES NOT allow the input or output file to be shared du to the use of fopen_s */ 
{
	FILE * fIn, *fOut;
	int ch, c, nl, nw, nc;
	char inputFile[256];
	
	ExtractToken (1, command, inputFile);
			
	if (fopen_s (&fIn, inputFile, "r") != 0) return 1;
	
	ch = nw = nc = nl = 0;
	while ((c = fgetc (fIn)) != EOF) {
		if (c == '\0') break;
		if (isspace(c) && isalpha(ch))
			nw++;
		ch = c;
		nc++;
		if (c == '\n')
			nl++;
	}
	fclose (fIn);

	/* Write the results */
	if (fopen_s (&fOut, output_file, "w") != 0) return 2;
	fprintf (fOut, " %9d %9d %9d %s\n", nl, nw, nc, inputFile);	
	fclose (fOut);
	return 0;
}

__declspec (dllexport)
int __cdecl toupperip (char * command, char * output_file)
/* convert input to upper case; in process			*/
/* Input file is the second token ("toupperip" is the first)	*/
{
	FILE * fIn, *fOut;
	int c;
	char inputFile[256];
	
	ExtractToken (1, command, inputFile);
			
	if (fopen_s (&fIn, inputFile, "r") != 0) return 1;
	if (fopen_s (&fOut, output_file, "w") != 0) return 2;
	
	while ((c = fgetc (fIn)) != EOF) {
		if (c == '\0') break;
		if (isalpha(c)) c = toupper(c);
		fputc  (c, fOut);	
	}
	fclose (fIn);
	fclose (fOut);	
	return 0;
}

static void ExtractToken (int it, char * command, char * token)
{
	/* Extract token number "it" (first token is number 0)	*/
	/* from "command". Result goes in "token"		*/
	/* tokens are white space delimited			*/

	int i;
	size_t tlen;
	char *pc, *pe, *ws = " \t\n"; /* white space */
	
	/* Skip first "it" tokens	*/
	
	pc = command;
	tlen = strlen(command);
	pe = pc + tlen;
	for (i = 0; i < it && pc < pe; i++) {
		pc += strcspn (pc, ws); /* Add length of next token */
			/* pc points to start of white space field */
		pc += strspn (pc, ws); /* Add length of white space field */
	}
	/* pc now points to the start of the token */
	tlen = strcspn (pc, ws);
	memcpy (token, pc, (int)tlen);
	token[tlen] = '\0';

	return;
}