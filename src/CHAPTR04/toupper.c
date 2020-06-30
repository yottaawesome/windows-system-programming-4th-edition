/* Chapter 4, toupper command.   */

/* 
	Convert one or more files, changing all letters to upper case
	The output file will be the same name as the input file, except
	a "UC_" prefix will be attached to the file name.

/* This program illustrates:
	1.	Exception handling to recover from a run-time exceptions
		and errors by using RaiseException).
	2.	Completion hanlders.
	3.	Program logic simplification using handlers. */

/* Technique:
	1.	Loop to process one file at a time.
	2.	Read the entire file into memory, allocating the memory and
		output file first. DO NOT WRITE OVER AN EXISTING FILE
	3. 	Convert the data in memory.
	4. 	Write the converted data to the output file.
	5.  Free memory and close file handles on each loop iteration. */

#include "Everything.h"

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hIn = INVALID_HANDLE_VALUE, hOut = INVALID_HANDLE_VALUE;
	DWORD nXfer, iFile, j;
	TCHAR outFileName[256] = _T(""), *pBuffer = NULL;
	OVERLAPPED ov = { 0, 0, 0, 0, NULL};
	LARGE_INTEGER fSize;

	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater to support LockFileEx"),
		               1, FALSE);

	if (argc <= 1)
		ReportError (_T ("Usage: toupper files"), 1, FALSE);
					/* Process all files on the command line. */
	for (iFile = 1; iFile < (unsigned int)argc; iFile++) __try { /* Exception block */
		/*	All file handles are invalid, pBuffer == NULL, and 
			outFileName is empty. This is assured by the handlers */
		if (_tcslen(argv[iFile]) > 250)
			ReportException(_T("The file name is too long."), 1);
		_stprintf (outFileName, _T("UC_%s"), argv[iFile]);
		__try { /* Inner try-finally block */
			/* Create the output file name, and create it (fail if file exists) */
			/* The file can be shared by other processes or threads */
			hIn  = CreateFile (argv [iFile], GENERIC_READ, 
				0, NULL, OPEN_EXISTING, 0, NULL);
			if (hIn == INVALID_HANDLE_VALUE) ReportException (argv[iFile], 1);
			if (!GetFileSizeEx (hIn, &fSize) || fSize.HighPart > 0)
				ReportException(_T("This file is too large for this program."), 1);
			hOut = CreateFile (outFileName, GENERIC_READ | GENERIC_WRITE, 
				0, NULL, CREATE_NEW, 0, NULL);
			if (hOut == INVALID_HANDLE_VALUE) ReportException (outFileName, 1);;
			/* Allocate memory for the file contents */
			pBuffer = malloc (fSize.LowPart); 
			if (pBuffer == NULL) ReportException (_T("Memory allocation error"), 1);;

			/* Read the data, convert it, and write to the output file */
			/* Free all resources upon completion, and then process the next file */
			/* Files are not locked, as the files are open with 0
			 *     for the share mode. However, in a more general situation, you
			 *     might have a process (Chapter 7) inherit the file handle or a 
			 *     thread (Chapter 7) might access the handle. Or, you might
			 *     actually want to open the file in share mode                    */

			if (!ReadFile (hIn, pBuffer, fSize.LowPart, &nXfer, NULL) || (nXfer != fSize.LowPart))
				ReportException (_T("ReadFile error"), 1);
			for (j = 0; j < fSize.LowPart; j++) /* Convert data */
				if (isalpha(pBuffer[j])) pBuffer[j] = toupper(pBuffer[j]);
			if (!WriteFile (hOut, pBuffer, fSize.LowPart, &nXfer, NULL) || (nXfer != fSize.LowPart))
				ReportException (_T("WriteFile error"), 1);

		} __finally { /* File handles are always closed */
			/* memory freed, and handles and pointer reinitialized. */
			if (pBuffer != NULL) free (pBuffer); pBuffer = NULL;
			if (hIn  != INVALID_HANDLE_VALUE) {
				CloseHandle (hIn);
				hIn  = INVALID_HANDLE_VALUE;
			}
			if (hOut != INVALID_HANDLE_VALUE) {
				CloseHandle (hOut);
				hOut = INVALID_HANDLE_VALUE;
			}
			_tcscpy (outFileName, _T(""));
		}
	} /* End of main file processing loop and try block. */
	/* This exception handler applies to the loop body */

	__except (EXCEPTION_EXECUTE_HANDLER) {
		_tprintf (_T("Error occured processing file %s\n"), argv[iFile]);
		DeleteFile (outFileName);
	}
	_tprintf (_T("All files converted, except as noted above\n"));
	return 0;
}
