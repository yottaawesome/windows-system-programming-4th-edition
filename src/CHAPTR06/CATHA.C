/* Chapter 6. catHA. */
/* cat files DecimalHandleValue */

/* This program illustrates:
	1.	ReportError
	2.	Win32 CreateFile, ReadFile, WriteFile, CloseHandle
	3.	Loop logic to process a sequential file
	4.	Use of handle passed as a command line argument */

#include "Environment.h"
#include <windows.h>
#include <tchar.h>
#include "support.h"
#include <stdio.h>

#define BUF_SIZE 0x200

static VOID CatFile (HANDLE, HANDLE);
int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hInFile, hOut;

	_tprintf (_T("In catHA. %s %s\n"), argv [1], argv [2]);

	hInFile = CreateFile (argv [1], GENERIC_READ, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if (hInFile == INVALID_HANDLE_VALUE)
			/* Simplifying assumption: Open fails only
				if the file already exists. */
		ReportError (_T("\nCat Error: File does not exist"), 0, TRUE);

	/* The output handle is obtained from the command line. */

	hOut = (HANDLE) (_ttoi (argv [2]));

	CatFile (hInFile, hOut);
	
	CloseHandle (hOut);
	return 0;
}

static VOID CatFile (HANDLE hInFile, HANDLE hOutFile)
{
	DWORD nIn, nOut;
	BYTE Buffer [BUF_SIZE];

	while (ReadFile (hInFile, Buffer, BUF_SIZE, &nIn, NULL)
			&& (nIn == BUF_SIZE)
			&& WriteFile (hOutFile, Buffer, nIn, &nOut, NULL)) ;
					/* Write last buffer. */ ;
		WriteFile (hOutFile, Buffer, nIn, &nOut, NULL);

	WriteFile (hOutFile, CRLF, lstrlen (CRLF) * TSIZE, &nOut, NULL);

	return;
}
