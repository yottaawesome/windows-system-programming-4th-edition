/*	Chapter 13 */
/*	ServiceShell.c  Windows Service Management shell program.
	This program modifies Chapter 6's Job Management program,
	managing services rather than jobs. */
/*  Illustrates service control from a program
	In general, use the sc.exe command or the "Services" 
	Administrative tools */
/*	commandList supported are:
		create		Create a service
		delete		Delete a service
		start		Start a service
		control		Control a service */
#include "Everything.h"

static int Create   (int, LPTSTR *, LPTSTR);
static int Delete   (int, LPTSTR *, LPTSTR);
static int Start    (int, LPTSTR *, LPTSTR);
static int Control  (int, LPTSTR *, LPTSTR);

static SC_HANDLE hScm;
static BOOL debug;

int _tmain (int argc, LPTSTR argv [])
{
	BOOL exitFlag = FALSE;
	TCHAR command [MAX_COMMAND_LINE+10], *pc;
	DWORD i, locArgc; /* Local argc */
	TCHAR argstr [MAX_ARG] [MAX_COMMAND_LINE];
	LPTSTR pArgs [MAX_ARG];

	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater"), 1, FALSE);

	debug = (argc > 1); /* simple debug flag */
	/*  Prepare the local "argv" array as pointers to strings */
	for (i = 0; i < MAX_ARG; i++) pArgs[i] = argstr[i];

	/*  Open the SC Control Manager on the local machine,
		with the default database, and all access. */
	hScm = OpenSCManager (NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
	if (hScm == NULL) ReportError (_T("Cannot open SC Manager"), 1, TRUE);

	/*  Main command proceesing loop  */
	_tprintf (_T("\nWindows Service Management"));
	while (!exitFlag) {
		_tprintf (_T("\nSM$"));
		_fgetts (command, MAX_COMMAND_LINE, stdin);
		/*  Replace the new line character with a string end. */
		pc = _tcschr (command, _T('\n')); *pc = _T('\0');

		if (debug) _tprintf (_T("%s\n"), command); 
		/*  Convert the command to "argc, argv" form. */
		GetArgs (command, &locArgc, pArgs);
		CharLower (argstr [0]);  /* The command is case-insensitive */

		if (debug) _tprintf (_T("\n%s %s %s %s"), argstr[0], argstr[1],
			argstr[2], argstr[3]);

		if (_tcscmp (argstr[0], _T("create")) == 0) {
			Create (locArgc, pArgs, command);
		}
		else if (_tcscmp (argstr[0], _T("delete")) == 0) {
			Delete (locArgc, pArgs, command);
		}
		else if (_tcscmp (argstr[0], _T("start")) == 0) {
			Start (locArgc, pArgs, command);
		}
		else if (_tcscmp (argstr[0], _T("control")) == 0) {
			Control (locArgc, pArgs, command);
		}
		else if (_tcscmp (argstr[0], _T("quit")) == 0) {
			exitFlag = TRUE;
		}
		else _tprintf (_T("\nCommand not recognized"));
	}

	CloseServiceHandle (hScm);
	return 0;
}


int Create (int argc, LPTSTR argv [], LPTSTR command)
{
	/*  Create a new service as a "demand start" service:
		argv[1]: Service Name
		argv[2]: Display Name
		argv[3]: binary executable */
	SC_HANDLE hSc;
	TCHAR executable[MAX_PATH+1], quotedExecutable[MAX_PATH+3] = _T("\"");

	if (argc < 4) {
		_tprintf (_T("\nUsage: create ServiceName, DisplayName, .exe"));
		return 1;
	}

	/* You need the full path name, and it needs quotes if there are spaces */
	GetFullPathName (argv[3], MAX_PATH+1, executable, NULL);
	_tcscat(quotedExecutable, executable);
	_tcscat(quotedExecutable, _T("\""));

	if (debug) _tprintf (_T("\nService Full Path Name: %s"), executable);

	hSc = CreateService (hScm, argv[1], argv[2],
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, 
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		quotedExecutable, NULL, NULL, NULL, NULL, NULL);
	if (hSc == NULL) ReportError (_T("Cannot create service"), 0, TRUE);
	else CloseServiceHandle (hSc); /* No need to retain the handle as
								   OpenService will query the service DB */
	return 0;

}

/*  Delete a service 
		argv[1]: ServiceName to delete  */
int Delete (int argc, LPTSTR argv [], LPTSTR command)
{
	SC_HANDLE hSc;

	if (debug) _tprintf (_T("\nAbout to delete service: %s"), argv[1]);

	hSc = OpenService(hScm,  argv[1], DELETE);
	if (hSc == NULL) {
		_tprintf (_T("\nService Management Error"));
		ReportError (_T("\nCannot open named service for deletion"), 0, TRUE);
	}
	else {
		if (!DeleteService (hSc)) 
			ReportError (_T("\nCannot delete service"), 0, TRUE);
		CloseServiceHandle (hSc);
		if (debug) _tprintf (_T("\nService %s deleted if no error msg"), argv[1]);
	}

	return 0;
}

/*  Start a named service.
		argv[1]: Service name to start */
int Start (int argc, LPTSTR argv [], LPTSTR command)
{
	SC_HANDLE hSc;
	TCHAR workingDir [MAX_PATH+1];
	LPTSTR argvStart[] = {argv[1], workingDir};

	GetCurrentDirectory (MAX_PATH+1, workingDir);
	if (debug) _tprintf (_T("\nAbout to start service: %s in directory: %s"),
		argv[1], workingDir);
	
	/* Get a handle to the service named on the command line (argv[1]) */
	hSc = OpenService(hScm,  argv[1], SERVICE_ALL_ACCESS);
	if (hSc == NULL) {
		_tprintf (_T("\nService Management Error"));
		ReportError (_T("\nCannot open named service for startup"), 0, TRUE);
	}
	else {
		/*  Start the service with one arg, the working directory */
		/*  The service name is taken from the program command line (argv[1]) */
		/*	Note that the name is also specified when opening the service handle */
		/*	Suggested experiment: What happens if the names don't agree? */

		if (!StartService (hSc, 2, argvStart)) {
			ReportError (_T("\nCannot start service"), 0, TRUE);
		}
		CloseServiceHandle (hSc);
		if (debug) _tprintf (_T("\nService %s started if no error msg"), argv[1]);
	}

	return 0;
}

/*  Control a named service.
		argv[1]:  Service name to control 
		argv[2]:  Control command (case insenstive):
					stop
					pause
					resume
					interrogate
					user  user defined
		argv[3]:    a number from 128 to 255, if the control command is "user"
					*/
static LPCTSTR commandList [] = 
	{_T("stop"), _T("pause"), _T("resume"), _T("interrogate"), _T("user") };
static DWORD controlsAccepted [] = {
	SERVICE_CONTROL_STOP, SERVICE_CONTROL_PAUSE,
	SERVICE_CONTROL_CONTINUE, SERVICE_CONTROL_INTERROGATE, 128 };

int Control (int argc, LPTSTR argv [], LPTSTR command)
{

	SC_HANDLE hSc;
	SERVICE_STATUS serviceStatus;
	DWORD dwControl, i;
	BOOL found = FALSE;

	if (debug) _tprintf (_T("\nAbout to control service: %s"), argv[1]);

	for (i= 0; i < sizeof(controlsAccepted)/sizeof(DWORD) && !found; i++)
		found = (_tcscmp (commandList[i], argv[2]) == 0);
	if (!found) {
		_tprintf (_T("\nIllegal Control command %s"), argv[1]);
		return 1;
	}
	dwControl = controlsAccepted[i-1];
	if (dwControl == 128) dwControl = _ttoi (argv[3]);
	if (debug) _tprintf (_T("\ndwControl = %d"), dwControl);

	hSc = OpenService(hScm,  argv[1],
		SERVICE_INTERROGATE | SERVICE_PAUSE_CONTINUE | 
		SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL | 
		SERVICE_QUERY_STATUS );
	if (hSc == NULL) {
		_tprintf (_T("\nService Management Error"));
		ReportError (_T("\nCannot open named service for control"), 0, TRUE);
	}
	else {
		if (!ControlService (hSc, dwControl, &serviceStatus))
			ReportError (_T("\nCannot control service"), 0, TRUE);
		if (debug) _tprintf (_T("\nService %s controlled if no error msg"), argv[1]);
	}

	if (dwControl == SERVICE_CONTROL_INTERROGATE) {
		if (!QueryServiceStatus (hSc, &serviceStatus))
			ReportError (_T("\nCannot query service status"), 0, TRUE);
		_tprintf (_T("\nStatus from QueryServiceStatus"));
		_tprintf (_T("\nSerice Status"));
		_tprintf (_T("\ndwServiceType: %d"),             serviceStatus.dwServiceType);
		_tprintf (_T("\ndwCurrentState: %d"),            serviceStatus.dwCurrentState);
		_tprintf (_T("\ndwControlsAccepted: %d"),        serviceStatus.dwControlsAccepted);
		_tprintf (_T("\ndwWin32ExitCode: %d"),           serviceStatus.dwWin32ExitCode);
		_tprintf (_T("\ndwServiceSpecificExitCode: %d"), serviceStatus.dwServiceSpecificExitCode);
		_tprintf (_T("\ndwCheckPoint: %d"),              serviceStatus.dwCheckPoint);
		_tprintf (_T("\ndwWaitHint: %d"),                serviceStatus.dwWaitHint);

	}
	if (hSc != NULL) CloseServiceHandle (hSc);
	return 0;
}
 
 







