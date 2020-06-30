/* cci_fDll.c  Chapter 5 */
/* cci_f.c, from Chapter 2, modified to build as a DLL */
#include "Everything.h"

#define BUF_SIZE 256

/* The following line [__declspec (dllexport)] is the only change from cci_f.c */
/* In general, you should have a single function and use a preprocessor */
/* variable to determine if the function is exported or not */

__declspec (dllexport)
BOOL __cdecl cci_f (LPCTSTR fIn, LPCTSTR fOut, DWORD shift)
/* Caesar cipher function  - Simple implementation
*		fIn:		Source file pathname
*		fOut:		Destination file pathname
*		shift:		Numerical shift
*	Behavior is modeled after CopyFile */
{
	HANDLE hIn, hOut;
	DWORD nIn, nOut, iCopy;
	CHAR aBuffer [BUF_SIZE], ccBuffer [BUF_SIZE];
	BOOL WriteOK = TRUE;

	hIn = CreateFile (fIn, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) return FALSE;

	hOut = CreateFile (fOut, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) return FALSE;

	__try
	{
		while (WriteOK && ReadFile (hIn, aBuffer, BUF_SIZE, &nIn, NULL) && nIn > 0) {
			for (iCopy = 0; iCopy < nIn; iCopy++)
				ccBuffer[iCopy] = (BYTE)(aBuffer[iCopy] + shift) % 256;
			WriteOK = WriteFile (hOut, ccBuffer, nIn, &nOut, NULL);
		}
	}
	__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		WriteOK = FALSE;
	}

	CloseHandle (hIn);
	CloseHandle (hOut);

	return WriteOK;
}
