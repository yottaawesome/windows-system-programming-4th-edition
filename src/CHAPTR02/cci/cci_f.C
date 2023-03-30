/* Chapter 2. Simple cci_f (modified Caesar cipher) implementation */
#include "Everything.h"

#define BUF_SIZE 65536	  /* Generally, you will get better performance with larger buffers (use powers of 2). 
					      /* 65536 worked well; larger numbers did not help in some simple tests. */

BOOL cci_f (LPCTSTR fIn, LPCTSTR fOut, DWORD shift)

/* Caesar cipher function  - Simple implementation
 *		fIn:		Source file pathname
 *		fOut:		Destination file pathname
 *		shift:		Numerical shift
 *	Behavior is modeled after CopyFile */
{
	HANDLE hIn, hOut;
	DWORD nIn, nOut, iCopy;
	BYTE buffer [BUF_SIZE], bShift = (BYTE)shift;
	BOOL writeOK = TRUE;

	hIn = CreateFile (fIn, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) return FALSE;

	hOut = CreateFile (fOut, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		CloseHandle(hIn);
		return FALSE;
	}

	while (writeOK && ReadFile (hIn, buffer, BUF_SIZE, &nIn, NULL) && nIn > 0) {
		/* Mar 24, 2010. The book, and an earlier version of this program, used separate input and output buffers, which is not necessary
		   and which hurts performance slightly due to cache behavior (thanks to Melvin Smith for pointing this out. HOWEVER, some read-modify-write
		   applications would need two buffers. For example, you might be expanding the file in some way. Also, the App C results are still
		   useful as the compare different implementations and different platforms. */
		for (iCopy = 0; iCopy < nIn; iCopy++)
			buffer[iCopy] = buffer[iCopy] + bShift;
		writeOK = WriteFile (hOut, buffer, nIn, &nOut, NULL);
	}

	CloseHandle (hIn);
	CloseHandle (hOut);

	return writeOK;
}
