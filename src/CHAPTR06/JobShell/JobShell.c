/*	Chapter 6 */
/* JobShell.c One program combining three
job management commands:
	Jobbg	- Run a job in the background
	jobs	- List all background jobs
	kill	- Terminate a specified job of job family
			  There is an option to generate a console
			  control signal.
*/
/* Contains several fixes suggested by Daniel Jiang */

#include "Everything.h"
#include "JobManagement.h"
#pragma comment(lib, "Utility_4_0_64.lib")

static int Jobbg (int, LPTSTR *, LPTSTR);
static int Jobs  (int, LPTSTR *, LPTSTR);
static int Kill  (int, LPTSTR *, LPTSTR);

int _tmain (int argc, LPTSTR argv[])
{
	BOOL exitFlag = FALSE;
	TCHAR command[MAX_COMMAND_LINE], *pc;
	DWORD i, localArgc; /* Local argc */
	TCHAR argstr[MAX_ARG][MAX_COMMAND_LINE];
	LPTSTR pArgs[MAX_ARG];

	/* NT Only - due to file locking */
	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater"), 1, FALSE);

	for (i = 0; i < MAX_ARG; i++)
		pArgs[i] = argstr[i];

	_tprintf (_T("Windows Job Mangement\n"));
	while (!exitFlag) {
		_tprintf (_T("%s"), _T("JM$"));
		_fgetts (command, MAX_COMMAND_LINE, stdin);
		pc = _tcschr (command, _T('\n'));
		*pc = _T('\0');

		GetArgs (command, &localArgc, pArgs);
		CharLower (argstr[0]);

		if (_tcscmp (argstr[0], _T("jobbg")) == 0) {
			Jobbg (localArgc, pArgs, command);
		}
		else if (_tcscmp (argstr[0], _T("jobs")) == 0) {
			Jobs (localArgc, pArgs, command);
		}
		else if (_tcscmp (argstr[0], _T("kill")) == 0) {
			Kill (localArgc, pArgs, command);
		}
		else if (_tcscmp (argstr[0], _T("quit")) == 0) {
			exitFlag = TRUE;
		}
		else _tprintf (_T("Illegal command. Try again.\n"));
	}


	return 0;
}

/* Jobbg: Execute a command line in the background, put
	the job identity in the user's job file, and exit.
	Related commands (jobs, fg, kill, and suspend) can be used to manage the jobs. */

/* jobbg [options] command-line
		-c: Give the new process a console.
		-d: The new process is detached, with no console.
	These two options are mutually exclusive.
	If neither is set, the background process shares the console with jobbg. */

/* This new features this program illustrates:
		1. Creating detached and with separate consoles.
		2. Maintaining a Jobs/Process list in a shared file.
		3. Determining a process status from a process ID.*/

/* Standard include files. */
/* - - - - - - - - - - - - - - - - - - - - - - - - - */

int Jobbg (int argc, LPTSTR argv[], LPTSTR command)
{
	/*	Similar to timep.c to process command line. */
	/*	- - - - - - - - - - - - - - - - - - - - - - */
	/*	Execute the command line (targv) and store the job id,
		the process id, and the handle in the jobs file. */

	DWORD fCreate;
	LONG jobNumber;
	BOOL flags[2];

	STARTUPINFO StartUp;
	PROCESS_INFORMATION processInfo;
	LPTSTR targv = SkipArg (command, 1, argc, argv);
	
	GetStartupInfo (&StartUp);

		/* Determine the options. */
	Options (argc, argv, _T ("cd"), &flags[0], &flags[1], NULL);

		/* Skip over the option field as well, if it exists. */
		/* Simplifying assumptions: There's only one of -d, -c (they are mutually exclusive.
		   Also, commands can't start with -. etc. You may want to fix this. */
	if (argv[1][0] == _T('-'))
		targv = SkipArg (command, 2, argc, argv);

	fCreate = flags[0] ? CREATE_NEW_CONSOLE : flags[1] ? DETACHED_PROCESS : 0;

		/* Create the job/thread suspended.
			Resume it once the job is entered properly. */
	if (!CreateProcess (NULL, targv, NULL, NULL, TRUE,
			fCreate | CREATE_SUSPENDED | CREATE_NEW_PROCESS_GROUP,
			NULL, NULL, &StartUp, &processInfo))  {
		ReportError (_T ("Error starting process."), 0, TRUE);
		return 4;
	}

		/* Create a job number and enter the process Id and handle
			into the Job "data base" maintained by the
			GetJobNumber function (part of the job management library). */
	
	jobNumber = GetJobNumber (&processInfo, targv);
	if (jobNumber >= 0)
		ResumeThread (processInfo.hThread);
	else {
		TerminateProcess (processInfo.hProcess, 3);
		CloseHandle (processInfo.hThread);
		CloseHandle (processInfo.hProcess);
		ReportError (_T ("Error: No room in job control list."), 0, FALSE);
		return 5;
	}

	CloseHandle (processInfo.hThread);
	CloseHandle (processInfo.hProcess);
	_tprintf (_T (" [%d] %d\n"), jobNumber, processInfo.dwProcessId);
	return 0;
}

/* Jobs: List all running or stopped jobs that have
	been created by this user under job management;
	that is, have been started with the jobbg command.
	Related commands (jobbg and kill) can be used to manage the jobs. */
/* This new features this program illustrates:
	1. Determining process status.
	2. Maintaining a Jobs/Process list in a shared file. */

int Jobs (int argc, LPTSTR argv[], LPTSTR command)
{
	if (!DisplayJobs ()) return 1;
	return 0;
}

/* kill [options] jobNumber
	Terminate the process associated with the specified job number. */
/* This new features this program illustrates:
	1. Using TerminateProcess 
	2. Console control events */

/* Options:
	-b  Generate a Ctrl-Break 
	-c  Generate a Ctrl-C
		Otherwise, terminate the process. */

/* The Job Management data structures, error codes,
	constants, and so on are in the following file. */

int Kill (int argc, LPTSTR argv[], LPTSTR command)
{
	DWORD processId, jobNumber, iJobNo;
	HANDLE hProcess;
	BOOL cntrlC, cntrlB, killed;

	iJobNo = Options (argc, argv, _T ("bc"), &cntrlB, &cntrlC, NULL);
	
	/* Find the process ID associated with this job. */

	jobNumber = _ttoi (argv[iJobNo]);
	processId = FindProcessId (jobNumber);
	if (processId == 0)	{
		ReportError (_T ("Job number not found.\n"), 0, FALSE);
		return 1;
	}
	hProcess = OpenProcess (PROCESS_TERMINATE, FALSE, processId);
	if (hProcess == NULL) {
		ReportError (_T ("Process already terminated.\n"), 0, FALSE);
		return 2;
	}

	if (cntrlB)
		killed = GenerateConsoleCtrlEvent (CTRL_BREAK_EVENT, 0);
	else if (cntrlC)
		killed = GenerateConsoleCtrlEvent (CTRL_C_EVENT, 0);
	else
		killed = TerminateProcess (hProcess, JM_EXIT_CODE);

	if (!killed) { 
		ReportError (_T ("Process termination failed."), 0, TRUE);
		return 3;
	}
	
	WaitForSingleObject (hProcess, 5000);
	CloseHandle (hProcess);

	_tprintf (_T ("Job [%d] terminated or timed out\n"), jobNumber);
	return 0;
}
