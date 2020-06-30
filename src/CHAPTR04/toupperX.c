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
	HANDLE hIn, hOut;
	DWORD FileSize, nXfer, iFile, j;
	CHAR OutFileName[256] = "", *pBuffer = NULL;
	OVERLAPPED ov = { 0, 0, 0, 0, NULL};

	if (argc <= 1)
		ReportError (_T ("Usage: toupper files"), 1, FALSE);
					/* Process all files on the command line. */
	for (iFile = 1; iFile < argc; iFile++) __try { /* Exception block */
		/*	All file handles are invalid, pBuffer == NULL, and 
			OutFileName is empty. This is assured by the handlers */
		_stprintf (OutFileName, "UC_%s", argv[iFile]);
		__try { /* Inner try-finally block */
			/* Create the output file name, and create it (fail if file exists) */
			hIn  = CreateFile (argv [iFile], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hIn == INVALID_HANDLE_VALUE) ReportException (argv[iFile], 1);
			FileSize = GetFileSize (hIn, NULL); /* Assume "small" file */
			hOut = CreateFile (OutFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
				CREATE_NEW, 0, NULL);
			if (hOut == INVALID_HANDLE_VALUE) ReportException (OutFileName, 1);;
			/* Allocate memory for the file contents */
			pBuffer = malloc (FileSize); 
			if (pBuffer == NULL) ReportException (_T("Memory allocation error"), 1);;

			/* Lock both files to assure the output accurately reflects the input */
			/* at this point. */
			/* Read the data, covert it, and write to the output file */
			/* Free all resources upon completion, and then process the next file */
			/* Modify the file locks if you are using Windows 9x */
			if (!LockFileEx (hIn,  LOCKFILE_FAIL_IMMEDIATELY, 0, 0, 0, &ov))
				ReportException (_T("Input file lock error"), 1);
			if (!LockFileEx (hOut, LOCKFILE_FAIL_IMMEDIATELY, 0, FileSize, 0, &ov))
				ReportException (_T("Output file lock error"), 1);

			if (!ReadFile (hIn, pBuffer, FileSize, &nXfer, NULL))
				ReportException (_T("ReadFile error"), 1);
			for (j = 0; j < FileSize; j++) /* Convert data */
				if (isalpha(pBuffer[j])) pBuffer[j] = toupper(pBuffer[j]);
			if (!WriteFile (hOut, pBuffer, FileSize, &nXfer, NULL))
				ReportException (_T("WriteFile error"), 1);

		} __finally { /* File handles are always closed */
			/* memory freed, and handles and pointer reinitialized. */
			if (pBuffer != NULL) free (pBuffer); pBuffer = NULL;
			if (hIn  != INVALID_HANDLE_VALUE) {
				CloseHandle (hIn);
				UnlockFileEx (hIn, 0, FileSize, 0, &ov);
			}
			if (hOut != INVALID_HANDLE_VALUE) {
				CloseHandle (hOut);
				UnlockFileEx (hOut, 0, FileSize, 0, &ov);
				hOut = INVALID_HANDLE_VALUE;
			}
			_tcscpy (OutFileName, _T(""));
		}
	} /* End of main file processing loop and try block. */
	/* This exception handler applies to the loop body */

	__except (EXCEPTION_EXECUTE_HANDLER) {
		_tprintf (_T("Error occured processing file %s\n"), argv[iFile]);
		DeleteFile (OutFileName);
	}
	_tprintf (_T("All files converted, except as noted above\n"));
	return 0;
}

