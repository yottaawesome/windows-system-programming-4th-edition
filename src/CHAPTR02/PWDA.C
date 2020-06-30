/* Chapter 2. pwda. */
/* pwd: Similar to the UNIX command. */
/* This program illustrates:
		1. Win32 GetCurrentDirectory
		2. Testing the length of a returned string
		3. A Version - allocate enough memory for the path */

#include "Everything.h"

int _tmain (int argc, LPTSTR argv [])
{
	/* Buffer to receive current directory allows
		for longest possible path. */

	DWORD LenCurDir = GetCurrentDirectory (0, NULL);
	/* Return length allows for terminating null character
		Note that the returned length accounts for the null character
		when the buffer is too small (thus giving the number of bytes needed) and
		gives the string length (without the NULL) on success. */
	LPTSTR pwdBuffer = malloc (LenCurDir * sizeof (TCHAR));

	if (pwdBuffer == NULL)
		ReportError (_T ("Failure allocating pathname buffer."), 1, TRUE);

	LenCurDir = GetCurrentDirectory (LenCurDir, pwdBuffer);
	
	PrintMsg (GetStdHandle (STD_OUTPUT_HANDLE), pwdBuffer);

	free (pwdBuffer);
	return 0;
}
