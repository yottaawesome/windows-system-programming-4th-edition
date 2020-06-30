/* Chapter 5. sortFL.  File sorting with memory mapping.
sortFL file */

/* This program illustrates:
1.	Using a memory-based algorithm (qsort) with a memory mapped file.
2.	Outputting the mapped file as a single string.
3.	try-except block to clean up (delete a temp file, in this case)
either on normal completion or in case of an exception. */

#include "Everything.h"

/* Definitions of the record structure in the sort file. */

#define DATALEN 56
#define KEY_SIZE 8

typedef struct _RECORD {
	TCHAR key[KEY_SIZE];
	TCHAR data[DATALEN];
} RECORD;

#define RECSIZE sizeof (RECORD)
typedef RECORD * LPRECORD;
int KeyCompare (LPCTSTR, LPCTSTR);

int _tmain (int argc, LPTSTR argv[])
{
	/* The file is the first argument. Sorting is done in place. */
	/* Sorting is done by file memory mapping. */

	HANDLE hFile = INVALID_HANDLE_VALUE, hMap = NULL;
	HANDLE hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
	LPVOID pFile = NULL;
	DWORD result = 2;
	TCHAR tempFile[MAX_PATH];
	LPTSTR pTFile;
	LARGE_INTEGER fileSize;

	BOOL NoPrint;
	int iFirstFile;

	iFirstFile = Options (argc, argv, _T ("n"), &NoPrint, NULL);

	if (argc <= iFirstFile)
		ReportError (_T ("Usage: sortFL [options] files"), 1, FALSE);

	__try {	/* try-except */
		/* Copy the input file to a temp output file that will be sorted.
		Do not alter the input file. */
		_stprintf_s (tempFile, MAX_PATH, _T ("%s.tmp"), argv[iFirstFile]);
		CopyFile (argv[iFirstFile], tempFile, TRUE);

		result = 1; 	/* tmp file is new and should be deleted. */
		/* Open the file (use the temporary copy). */
		hFile = CreateFile (tempFile, GENERIC_READ
			| GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile ==INVALID_HANDLE_VALUE)
			ReportException (_T ("File open failed."), 2);

		/* Get file size.
		If the file is too large, catch that when it is mapped. */
		if (!GetFileSizeEx (hFile, &fileSize))
			ReportException (_T ("GetFileSizeEx failed."), 2);

		if (fileSize.QuadPart == 0) { /* There is nothing to sort */
			CloseHandle (hFile);
			return 0;	/* You might want to put out an informational message */
		}
		/* Create a file mapping object.
		Use the file size but add space for the null character. */

		fileSize.QuadPart += 2;
		if (fileSize.HighPart > 0 && sizeof(SIZE_T) == 4)
			ReportException (_T ("This file is too large to map on a Win32 system."), 4);

		hMap = CreateFileMapping (hFile, NULL, PAGE_READWRITE, fileSize.HighPart, fileSize.LowPart, NULL);
		if (hMap == NULL)
			ReportException (_T ("Create File map failed."), 3);

		pFile = MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (pFile == NULL) 
			ReportException (_T ("MapView failed"), 4);

		/* Now sort the file.
		Perform the sort with the C library - in mapped memory. */
		__try {
			qsort (pFile, (SIZE_T)fileSize.QuadPart / RECSIZE, RECSIZE, KeyCompare);

			/* Print out the entire sorted file. Treat it as one single string. */
			pTFile = (LPTSTR) pFile;
			pTFile[fileSize.QuadPart/TSIZE] = _T('\0');
			if (!NoPrint)
				PrintMsg (hStdOut, pFile);
		}
		__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			ReportError(_T("Fatal Error accessing mapped file."), 9, TRUE);
		}

		/* Indicate successful completion. */
		result = 0; 			 
		ReportException (EMPTY, 5); /* Force an exception to clean up. */
		return 0;
	} /* End of inner try-except. */

	__except (EXCEPTION_EXECUTE_HANDLER) {

		/* Catch any exception here. Indicate any error.
		This is the normal termination. Delete the temporary
		file and Free all the resources. */

		if (pFile != NULL)
			UnmapViewOfFile (pFile);
		if (hMap != NULL)
			CloseHandle (hMap);
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle (hFile);
		if (result != 2)
			DeleteFile (tempFile);
		return result;
	}
} /* End of _tmain */

/*  CODE FROM HERE TO END IS NOT INCLUDED IN TEXT. */

int KeyCompare (LPCTSTR pKey1, LPCTSTR pKey2)

/* Compare two records of generic characters.
The key position and length are global variables. */
{
	return _tcsncmp (pKey1, pKey2, KEY_SIZE);
}

