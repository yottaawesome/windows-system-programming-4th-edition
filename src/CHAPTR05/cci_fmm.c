/* Chapter 5.
cci_fMM.c function: Memory Mapped implementation of the
simple Caeser cipher function. */

#include "Everything.h"

BOOL cci_f (LPCTSTR fIn, LPCTSTR fOut, DWORD shift)

/* Caesar cipher function. 
*	fIn:		Source file pathname.
*	fOut:		Destination file pathname.
*	shift:		Numeric shift value */
{
	BOOL complete = FALSE;
	HANDLE hIn = INVALID_HANDLE_VALUE, hOut = INVALID_HANDLE_VALUE;
	HANDLE hInMap = NULL, hOutMap = NULL;
	LPTSTR pIn = NULL, pInFile = NULL, pOut = NULL, pOutFile = NULL;

	__try {
		LARGE_INTEGER fileSize;

		/* Open the input file. */
		hIn = CreateFile (fIn, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hIn == INVALID_HANDLE_VALUE) 
			ReportException (_T ("Failure opening input file."), 1);

		/* Get the input file size. */
		if (!GetFileSizeEx (hIn, &fileSize))
			ReportException (_T ("Failure getting file size."), 4);
		/* This is a necessar, but NOT sufficient, test for mappability on 32-bit systems S
		 * Also see the long comment a few lines below */
		if (fileSize.HighPart > 0 && sizeof(SIZE_T) == 4)
			ReportException (_T ("This file is too large to map on a Win32 system."), 4);

		/* Create a file mapping object on the input file. Use the file size. */
		hInMap = CreateFileMapping (hIn, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hInMap == NULL)
			ReportException (_T ("Failure Creating input map."), 2);

		/* Map the input file */
		/* Comment: This may fail for large files, especially on 32-bit systems
		 * where you have, at most, 3 GB to work with (of course, you have much less
		 * in reality, and you need to map two files. 
		 * This program works by mapping the input and output files in their entirity.
		 * You could enhance this program by mapping one block at a time for each file,
		 * much as blocks are used in the ReadFile/WriteFile implementations. This would
		 * allow you to deal with very large files on 32-bit systems. I have not taken
		 * this step and leave it as an exercise. 
		 */
		pInFile = MapViewOfFile (hInMap, FILE_MAP_READ, 0, 0, 0);
		if (pInFile == NULL)
			ReportException (_T ("Failure Mapping input file."), 3);

		/*  Create/Open the output file. */
		/* The output file MUST have Read/Write access for the mapping to succeed. */
		hOut = CreateFile (fOut, GENERIC_READ | GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOut == INVALID_HANDLE_VALUE) {
			complete = TRUE; /* Do not delete an existing file. */
			ReportException (_T ("Failure Opening output file."), 5);
		}

		/* Map the output file. CreateFileMapping will expand
		the file if it is smaller than the mapping. */

		hOutMap = CreateFileMapping (hOut, NULL, PAGE_READWRITE, fileSize.HighPart, fileSize.LowPart, NULL);
		if (hOutMap == NULL)
			ReportException (_T ("Failure creating output map."), 7);
		pOutFile = MapViewOfFile (hOutMap, FILE_MAP_WRITE, 0, 0, (SIZE_T)fileSize.QuadPart);
		if (pOutFile == NULL)
			ReportException (_T ("Failure mapping output file."), 8);

		/* Now move the input file to the output file, doing all the work in memory. */
		__try
		{
			CHAR cShift = (CHAR)shift;
			pIn = pInFile;
			pOut = pOutFile;

			while (pIn < pInFile + fileSize.QuadPart) {
				*pOut = (*pIn + cShift);
				pIn++; pOut++;
			}
			complete = TRUE;
		}
		__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			complete = FALSE;
			ReportException(_T("Fatal Error accessing mapped file."), 9);
		}

		/* Close all views and handles. */
		UnmapViewOfFile (pOutFile); UnmapViewOfFile (pInFile);
		CloseHandle (hOutMap); CloseHandle (hInMap);
		CloseHandle (hIn); CloseHandle (hOut);
		return complete;
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (pOutFile != NULL) UnmapViewOfFile (pOutFile); if (pInFile != NULL) UnmapViewOfFile (pInFile);
		if (hOutMap != NULL) CloseHandle (hOutMap); if (hInMap != NULL) CloseHandle (hInMap);
		if (hIn != INVALID_HANDLE_VALUE) CloseHandle (hIn); if (hOut != INVALID_HANDLE_VALUE) CloseHandle (hOut);

		/* Delete the output file if the operation did not complete successfully. */
		if (!complete)
			DeleteFile (fOut);
		return FALSE;
	}
}
