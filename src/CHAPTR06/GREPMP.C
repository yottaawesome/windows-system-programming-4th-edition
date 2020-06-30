/* Chapter 6. grepMP. */
/* Multiple process version of grep command. */
/* grep pattern files.
	Search one or more files for the pattern.
	List the complete line on which the pattern occurs.
	Include the file name if there is more than one
	file on the command line. No Options. */

/* This program illustrates:
	1. 	Creating processes.
	2. 	Setting a child process standard I/O using the process start-up structure.
	3. 	Specifying to the child process that the parent's file handles are inheritable.
	4. 	Synchronizing with process termination using WaitForMultipleObjects
		and WaitForSingleObject.
	5.	Generating and using temporary files to hold the output of each process. */

#include "Everything.h"

int _tmain (int argc, LPTSTR argv[])

/*	Create a separate process to search each file on the
	command line. Each process is given a temporary file,
	in the current directory, to receive the results. */
{
	HANDLE hTempFile;
	SECURITY_ATTRIBUTES stdOutSA = /* SA for inheritable handle. */
			{sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
	TCHAR commandLine[MAX_PATH + 100];
	STARTUPINFO startUpSearch, startUp;
	PROCESS_INFORMATION processInfo;
	DWORD exitCode, dwCreationFlags = 0;
	int iProc;
	HANDLE *hProc;  /* Pointer to an array of proc handles. */
	typedef struct {TCHAR tempFile[MAX_PATH];} PROCFILE;
	PROCFILE *procFile; /* Pointer to array of temp file names. */
#ifdef UNICODE
    dwCreationFlags = CREATE_UNICODE_ENVIRONMENT;
#endif


	if (argc < 3)
		ReportError (_T ("Usage: grepMP pattern files."), 1, FALSE);

	/* Startup info for each child search process as well as
		the child process that will display the results. */

	GetStartupInfo (&startUpSearch);
	GetStartupInfo (&startUp);

	/* Allocate storage for an array of process data structures,
		each containing a process handle and a temporary file name. */

	procFile = malloc ((argc - 2) * sizeof (PROCFILE));
	hProc = malloc ((argc - 2) * sizeof (HANDLE));

	/* Create a separate "grep" process for each file on the
		command line. Each process also gets a temporary file
		name for the results; the handle is communicated through
		the STARTUPINFO structure. argv[1] is the search pattern. */

	for (iProc = 0; iProc < argc - 2; iProc++) {

		/* Create a command line of the form: grep argv[1] argv[iProc + 2] */
		/* Allow spaces in the file names. */
		_stprintf (commandLine, _T ("grep \"%s\" \"%s\""), argv[1], argv[iProc + 2]);

		/* Create the temp file name for std output. */

		if (GetTempFileName (_T ("."), _T ("gtm"), 0, procFile[iProc].tempFile) == 0)
			ReportError (_T ("Temp file failure."), 2, TRUE);

		/* Set the std output for the search process. */

		hTempFile = /* This handle is inheritable */
			CreateFile (procFile[iProc].tempFile,
				/** GENERIC_READ | Read access not required **/ GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, &stdOutSA,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hTempFile == INVALID_HANDLE_VALUE)
			ReportError (_T ("Failure opening temp file."), 3, TRUE);

		/* Specify that the new process takes its std output
			from the temporary file's handles. */

		startUpSearch.dwFlags = STARTF_USESTDHANDLES;
		startUpSearch.hStdOutput = hTempFile;
		startUpSearch.hStdError = hTempFile;
		startUpSearch.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
		/* Create a process to execute the command line. */

		if (!CreateProcess (NULL, commandLine, NULL, NULL,
				TRUE, dwCreationFlags, NULL, NULL, &startUpSearch, &processInfo))
			ReportError (_T ("ProcCreate failed."), 4, TRUE);
		/* Close unwanted handles */
		CloseHandle (hTempFile); CloseHandle (processInfo.hThread);

		/* Save the process handle. */

		hProc[iProc] = processInfo.hProcess;
	}

	/* Processes are all running. Wait for them to complete, then output
		the results - in the order of the command line file names. */
	for (iProc = 0; iProc < argc-2; iProc += MAXIMUM_WAIT_OBJECTS)
		WaitForMultipleObjects (min(MAXIMUM_WAIT_OBJECTS, argc - 2 - iProc), 
				&hProc[iProc], TRUE, INFINITE);

	/* Result files sent to std output using "cat".
		Delete each temporary file upon completion. */

	for (iProc = 0; iProc < argc - 2; iProc++) { 
		if (GetExitCodeProcess (hProc[iProc], &exitCode) && exitCode == 0) {
			/* Pattern was detected - List results. */
			/* List the file name if there is more than one file to search */
			if (argc > 3) _tprintf (_T("%s:\n"), argv[iProc+2]);
			_stprintf (commandLine, _T ("cat \"%s\""), procFile[iProc].tempFile);
			if (!CreateProcess (NULL, commandLine, NULL, NULL,
					TRUE, dwCreationFlags, NULL, NULL, &startUp, &processInfo))
				ReportError (_T ("Failure executing cat."), 0, TRUE);
			else { 
				WaitForSingleObject (processInfo.hProcess, INFINITE);
				CloseHandle (processInfo.hProcess);
				CloseHandle (processInfo.hThread);
			}
		}

		CloseHandle (hProc[iProc]);
		if (!DeleteFile (procFile[iProc].tempFile))
			ReportError (_T ("Cannot delete temp file."), 6, TRUE);
	}
	free (procFile); free (hProc);
	return 0;
}

