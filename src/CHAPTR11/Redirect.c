#include "Everything.h"

int _tmain (int argc, LPTSTR argv [])

/* Pipe together two programs whose names are on the command line:
		Redirect command1 = command2
	where the two commands are arbitrary strings.
	command1 uses standard input, and command2 uses standard output.
	Use = so as not to conflict with the DOS pipe. */
{
	DWORD i;
	HANDLE hReadPipe, hWritePipe;
	TCHAR command1 [MAX_PATH];
	SECURITY_ATTRIBUTES pipeSA = {sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
			/* Initialize for inheritable handles. */
		
	PROCESS_INFORMATION procInfo1, procInfo2;
	STARTUPINFO startInfoCh1, startInfoCh2;
	LPTSTR targv, cLine = GetCommandLine ();

	/* Startup info for the two child processes. */

	GetStartupInfo (&startInfoCh1);
	GetStartupInfo (&startInfoCh2);

	if (cLine == NULL)
		ReportError (_T ("\nCannot read command line."), 1, TRUE);
	targv = SkipArg(cLine, 1, argc, argv);
	i = 0;		/* Get the two commands. */
	while (*targv != _T('=') && *targv != _T('\0')) {
		command1 [i] = *targv;
		targv++; i++;
	}
	command1 [i] = _T('\0');
	if (*targv == _T('\0'))
		ReportError (_T("No command separator found."), 2, FALSE);

	/* Skip past the = and white space to the start of the second command */
	targv++;
	while ( *targv != '\0' && (*targv == ' ' || *targv == '\t') ) targv++;
	if (*targv == _T('\0'))
		ReportError (_T("Second command not found."), 2, FALSE);

	/* Create an anonymous pipe with default size.
		The handles are inheritable. */

	if (!CreatePipe (&hReadPipe, &hWritePipe, &pipeSA, 0))
		ReportError (_T ("Anon pipe create failed."), 3, TRUE);

	/* Set the output handle to the inheritable pipe handle,
		and create the first processes. */

	startInfoCh1.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
	startInfoCh1.hStdError  = GetStdHandle (STD_ERROR_HANDLE);
	startInfoCh1.hStdOutput = hWritePipe;
	startInfoCh1.dwFlags = STARTF_USESTDHANDLES;

	if (!CreateProcess (NULL, command1, NULL, NULL,
			TRUE,			/* Inherit handles. */
			0, NULL, NULL, &startInfoCh1, &procInfo1)) {
		ReportError (_T ("CreateProc1 failed."), 4, TRUE);
	}
	CloseHandle (procInfo1.hThread);
	CloseHandle (hWritePipe);

	/* Repeat (symmetrically) for the second process. */

	startInfoCh2.hStdInput  = hReadPipe;
	startInfoCh2.hStdError  = GetStdHandle (STD_ERROR_HANDLE);
	startInfoCh2.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	startInfoCh2.dwFlags = STARTF_USESTDHANDLES;

	if (!CreateProcess (NULL, targv, NULL, NULL,
			TRUE,			/* Inherit handles. */
			0, NULL, NULL, &startInfoCh2, &procInfo2))
		ReportError (_T ("CreateProc2 failed."), 5, TRUE);
	CloseHandle (procInfo2.hThread); 
	CloseHandle (hReadPipe);

	/* Wait for both processes to complete.
		The first one should finish first, although it really does not matter. */

	WaitForSingleObject (procInfo1.hProcess, INFINITE);
	WaitForSingleObject (procInfo2.hProcess, INFINITE);
	CloseHandle (procInfo1.hProcess); 
	CloseHandle (procInfo2.hProcess);
	return 0;
}
