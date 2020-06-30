/* Chapter 2. cci Version 1. Modified Caesar cipher (http://en.wikipedia.org/wiki/Caesar_cipher). */
/* Main program, which can be linked to different implementations */
/* of the cci_f function. */

/* cci shift file1 file2
 *		shift is the integer added mod 256 to each byte.
 *	Otherwise, this program is like cp and cpCF but there is no direct UNIX equivalent. */

/* This program illustrates:
 *		1. File processing with converstion.
 *		2. Boilerplate code to process the command line.
 */

#include "Everything.h"
#include <io.h>

BOOL cci_f (LPCTSTR, LPCTSTR, DWORD);

int _tmain (int argc, LPTSTR argv [])
{
	if (argc != 4)
		ReportError  (_T ("Usage: cci shift file1 file2"), 1, FALSE);
	
	if (!cci_f (argv [2], argv [3], _ttoi(argv[1])))
		ReportError (_T ("Encryption failed."), 4, TRUE);

	return 0;
}
