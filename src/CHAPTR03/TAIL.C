/* Chapter 3. tail command. */
/* tail file - Print the last 10 lines of the named file.
	All options are ignored. The file must be specified. */

/* This program illustrates:
	1. Getting the file size.
	2. Setting the file pointer.
	3. LARGE_INTEGER arithmetic and using the 64-bit file positions.
	4. Getting the current file position by moving
		0 bytes from the current position. */

/*  UPDATES. APRIL 12, 2010. 
 *  1. The program uses GetFileSizeEx and SetFilePointerEx, which are much easier to use than the non-Ex versions and
       are supported in all the target Windows systems.
 *  2. There are some corrections, suggested by Kwan Ting Chan (KTC) to account for the line size and UNICODE usage.
 *
 *  LIMITATIONS: Lines shouldn't be longer than 256 characters
 *  It's assumed that the text file is UNICODE or not depending on how the program is built. There is no attempt to
 *  deteect the file type (we leave that as an exercise for the reader).
 */
#include "Everything.h"

#define NUM_LINES 11
	/* One more than number of lines in the tail. */
#define MAX_LINE_SIZE 256
#define MAX_CHAR NUM_LINES*(MAX_LINE_SIZE+1)

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hInFile;
	HANDLE hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
	LARGE_INTEGER FileSize, CurPtr, FPos;
	LARGE_INTEGER LinePos[NUM_LINES];
	DWORD LastLine, FirstLine, LineCount, nRead;
	TCHAR buffer[MAX_CHAR + 1], c;

	if (argc != 2)
		ReportError (_T("Usage: tail file"), 1, FALSE);
	hInFile = CreateFile (argv [1], GENERIC_READ,
			0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hInFile == INVALID_HANDLE_VALUE)
		ReportError (_T("tail error: Cannot openfile."), 2, TRUE);

	/* Get the current file size. */
	if (!GetFileSizeEx (hInFile, &FileSize))
		ReportError (_T("tail error: file size"), 3, TRUE);

	/* Set the file pointer on the assumption that 256
		is the maximum size and look for the line beginnings.
		If 10 lines are not located, then just put out the
		ones that are found. A more general solution would look
		farther back in the file if necessary. Alternatively,
		start the scan at the beginning of the file, but that
		would be time consuming for a large file. */

	CurPtr.QuadPart = (LONGLONG)FileSize.QuadPart
			- NUM_LINES * MAX_LINE_SIZE * sizeof (TCHAR);
	if (CurPtr.QuadPart < 0) CurPtr.QuadPart = 0;
	if (!SetFilePointerEx (hInFile, CurPtr, &FPos, FILE_BEGIN))
		ReportError (_T("tail Error: Set Pointer."), 4, TRUE);

	/*  Scan the file for the start of new lines and retain their
		position. Assume that a line starts at the current position. */

	LinePos [0].QuadPart = CurPtr.QuadPart;
	LineCount = 1;
	LastLine = 1;
	while (TRUE) {
		while (ReadFile (hInFile, &c, sizeof(TCHAR), &nRead, NULL)
				&& nRead > 0 && c != CR) ; /* Empty loop body */
		if (nRead < sizeof(TCHAR)) break;
					/* Found a CR. Is LF next? */
		ReadFile (hInFile, &c, sizeof(TCHAR), &nRead, NULL);
		if (nRead < sizeof(TCHAR)) break;
		if (c != LF) continue;
		CurPtr.QuadPart = 0;
		/* Get the current file position. */
		if (!SetFilePointerEx (hInFile, CurPtr, &CurPtr, FILE_CURRENT))
			ReportError (_T("tail Error: Set Pointer to get current position."), 5, TRUE);
				/* Retain the start-of-line position */
		LinePos[LastLine].QuadPart = CurPtr.QuadPart;
		LineCount++;
		LastLine = LineCount % NUM_LINES;
	}
	
	FirstLine = LastLine % NUM_LINES;
	if (LineCount < NUM_LINES) FirstLine = 0;
	CurPtr.QuadPart = LinePos[FirstLine].QuadPart;

	/* The start of each line is now stored in LinePos []
		with the last line having index LastLine.
		Display the last strings. */

	if (!SetFilePointerEx (hInFile, CurPtr, NULL, FILE_BEGIN))
		ReportError (_T("tail Error: Set Pointer to new line position."), 6, TRUE);

	ReadFile (hInFile, buffer, MAX_CHAR * sizeof(TCHAR), &nRead, NULL);
	buffer [nRead/sizeof(TCHAR)] = _T('\0');
	PrintMsg (hStdOut, buffer);
	CloseHandle (hInFile);
	return 0;
}
