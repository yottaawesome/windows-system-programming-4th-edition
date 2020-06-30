/* Chapter 2. pwd. */
/* pwd: Print the current directory. */
/* This program illustrates:
	1. Windows GetCurrentDirectory
	2. Testing the length of a returned string */

#include "Everything.h"

#define DIRNAME_LEN (MAX_PATH + 2)

int _tmain (int argc, LPTSTR argv [])
{
	/* Buffer to receive current directory allows for the CR,
		LF at the end of the longest possible path. */

	TCHAR pwdBuffer [DIRNAME_LEN];
	DWORD lenCurDir;
	lenCurDir = GetCurrentDirectory (DIRNAME_LEN, pwdBuffer);
	if (lenCurDir == 0)
		ReportError (_T ("Failure getting pathname."), 1, TRUE);
	if (lenCurDir > DIRNAME_LEN)
		ReportError (_T ("Pathname is too long."), 2, FALSE);
	
	PrintMsg (GetStdHandle (STD_OUTPUT_HANDLE), pwdBuffer);
	return 0;
}
