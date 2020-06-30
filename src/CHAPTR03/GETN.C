/* Chapter 3. getn command. */
/* get file      
	Get a specified fixed size record from the file.
	The user is prompted for record numbers. Each
	requested record is retrieved and displayed
	until a negative record number is requested.
	Fixed size, text records are assumed. */
/*	There is a maximum line size (MAX_LINE_SIZE). */

/* This program illustrates:
	1. Setting the file pointer.
	2. LARGE_INTEGER arithmetic and using the 64-bit file positions. */

#include "Everything.h"

	/* One more than number of lines in the tail. */

#define MAX_LINE_SIZE 256

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hInFile;
	LARGE_INTEGER CurPtr;
	DWORD nRead, RecSize;
	TCHAR buffer[MAX_LINE_SIZE + 1];
	BOOL Exit = FALSE;
	LPTSTR p;
	int RecNo;

 	if (argc != 2)
		ReportError (_T ("Usage: getn file"), 1, FALSE);
	hInFile = CreateFile (argv [1], GENERIC_READ,
			0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hInFile == INVALID_HANDLE_VALUE)
		ReportError (_T ("getn error: Cannot open file."), 2, TRUE);
	
	RecSize = 0;
		/* Get the record size from the first record.
			Read the max line size and look for a CR/LF. */

	if (!ReadFile (hInFile, buffer, sizeof (buffer), &nRead, NULL) || nRead == 0)
		ReportError (_T ("Nothing to read."), 1, FALSE);
	if ((p = _tstrstr (buffer, _T ("\r\n"))) == NULL)
		ReportError (_T ("No end of line found."), 2, FALSE);
	/* Ignore Win64 warning about possible loss of data */
	RecSize = (DWORD)(p - buffer + 2);	/* 2 for the CRLF. */

	_tprintf (_T ("Record size is: %d\n"), RecSize);
	
	while (TRUE) {
		_tprintf (_T ("Enter record number. Negative to exit: "));
		_tscanf_s (_T ("%d"), &RecNo);
		if (RecNo < 0) break;

		CurPtr.QuadPart = (LONGLONG) RecNo * RecSize;
		if (!SetFilePointerEx (hInFile, CurPtr, NULL, FILE_BEGIN))
		/* Alternative: Use an overlapped structure */
			ReportError (_T ("getn Error: Set Pointer."), 3, TRUE);
		if (!ReadFile (hInFile, buffer, RecSize,  &nRead, NULL) || (nRead != RecSize))
			ReportError (_T ("Error reading n-th record."), 0, TRUE);
		buffer[RecSize] = _T('\0');
		_tprintf (_T ("%s\n"), buffer);
	}
	CloseHandle (hInFile);
	return 0;
}
