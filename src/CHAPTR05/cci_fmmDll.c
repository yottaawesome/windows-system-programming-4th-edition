/* cci_fMMDll.c. Chapter 5  */
/* cci_fMM.c, Chapter 5, modified to build as a DLL */
/* Chapter 5.
cci_fMM.c function: Memory Mapped implementation of the
Simple Caesar cipher function. */

#include "Everything.h"
/* The following line [__declspec (dllexport)] is the only change from cci_fMM.c */
/* In general, you should have a single function and use a preprocessor */
/* variable to determine if the function is exported or not */

__declspec (dllexport)
BOOL __cdecl cci_f (LPCTSTR fIn, LPCTSTR fOut, DWORD shift)

/* Caesar cipher function. 
*	fIn:		Source file pathname.
*	fOut:		Destination file pathname.
*	shift:		Numeric shift value */
{
	BOOL Complete = FALSE;
	__try {
		HANDLE hIn, hOut;
		HANDLE hInMap, hOutMap;
		LPTSTR pIn, pInFile, pOut, pOutFile;
		LARGE_INTEGER FSize;

		/* Open the input file. */
		hIn = CreateFile (fIn, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hIn == INVALID_HANDLE_VALUE) 
			ReportException (_T ("Failure opening input file."), 1);

		/* Get the input file size. */
		if (!GetFileSizeEx (hIn, &FSize))
			ReportException (_T ("Failure getting file size."), 4);
		if (FSize.HighPart > 0 && sizeof(SIZE_T) == 4)
			ReportException (_T ("This file is too large to map on a Win32 system."), 4);

		/* Create a file mapping object on the input file. Use the file size. */
		hInMap = CreateFileMapping (hIn, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hInMap == NULL)
			ReportException (_T ("Failure Creating input map."), 2);

		/* Map the input file */
		pInFile = MapViewOfFile (hInMap, FILE_MAP_READ, 0, 0, 0);
		if (pInFile == NULL)
			ReportException (_T ("Failure Mapping input file."), 3);

		/*  Create/Open the output file. */
		/* The output file MUST have Read/Write access for the mapping to succeed. */
		hOut = CreateFile (fOut, GENERIC_READ | GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOut == INVALID_HANDLE_VALUE) {
			Complete = TRUE; /* Do not delete an existing file. */
			ReportException (_T ("Failure Opening output file."), 5);
		}

		/* Map the output file. CreateFileMapping will expand
		the file if it is smaller than the mapping. */

		hOutMap = CreateFileMapping (hOut, NULL, PAGE_READWRITE, FSize.HighPart, FSize.LowPart, NULL);
		if (hOutMap == NULL)
			ReportException (_T ("Failure creating output map."), 7);
		pOutFile = MapViewOfFile (hOutMap, FILE_MAP_WRITE, 0, 0, (SIZE_T)FSize.QuadPart);
		if (pOutFile == NULL)
			ReportException (_T ("Failure mapping output file."), 8);

		/* Now move the input file to the output file, doing all the work in memory. */
		__try {
			pIn = pInFile;
			pOut = pOutFile;
			while (pIn < pInFile + FSize.QuadPart) {
				*pOut = (BYTE)((*pIn + shift) % 256);
				pIn++; pOut++;
				Complete = TRUE;
			}
		}
		__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			Complete = FALSE;
		}

		/* Close all views and handles. */

		UnmapViewOfFile (pOutFile); UnmapViewOfFile (pInFile);
		CloseHandle (hOutMap); CloseHandle (hInMap);
		CloseHandle (hIn); CloseHandle (hOut);
		return Complete;
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {
		/* Delete the output file if the operation did not complete successfully. */
		if (!Complete)
			DeleteFile (fOut);
		return FALSE;
	}
}




