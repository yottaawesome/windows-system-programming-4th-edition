/*Chapter 14. serviceSK.c
	serverSK (Chapter 12) converted to an NT service */
/* Build as a mutltithreaded console application */
/*	Usage: simpleService [-c]								*/
/*			-c says to run as a console app, not a service	*/						 
/*  Main routine that starts the service control dispatcher */
/*	Everything at the beginning is a "service wrapper", 
	the same as in SimpleService.c" */
#include "Everything.h"
#include "ClientServer.h"
#define UPDATE_TIME 1000	/* One second between udpdates */

VOID FileLogEvent (LPCTSTR, DWORD, BOOL); 
void WINAPI ServiceMain (DWORD argc, LPTSTR argv[]);
VOID WINAPI ServerCtrlHandler(DWORD);
void UpdateStatus (int, int);
int  ServiceSpecific (int, LPTSTR *);

static FILE *hLogFile; /* Text Log file */
static SERVICE_STATUS hServStatus;
static SERVICE_STATUS_HANDLE hSStat; /* Service status handle for setting status */
volatile static ShutFlag = FALSE;
									 
static LPTSTR ServiceName = _T("serviceSK");
static LPTSTR LogFileName = "serviceSKLog.txt";
static BOOL consoleApp = FALSE, isService;

VOID _tmain (int argc, LPTSTR argv [])
{
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ ServiceName,				ServiceMain	},
		{ NULL,						NULL }
	};

	Options (argc, argv, _T ("c"), &consoleApp, NULL);
	isService = !consoleApp;

	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater"), 1, FALSE);

	if (isService) {
	if (!StartServiceCtrlDispatcher (DispatchTable))
        ReportError (_T("Failed calling StartServiceCtrlDispatcher"), 1, FALSE);
	} else {
		ServiceSpecific (argc, argv);
	}
	return;
}


/*	ServiceMain entry point, called when the service is created by
	the main program.  */
void WINAPI ServiceMain (DWORD argc, LPTSTR argv[])
{
	DWORD i;

	/*  Set the current directory and open a log file, appending to
		an existing file */
	if (argc > 2) SetCurrentDirectory (argv[2]);
	hLogFile = fopen (LogFileName, _T("w+"));
	if (hLogFile == NULL) return ;

	FileLogEvent (_T("Starting service. First log entry."), 0, FALSE);
	_ftprintf (hLogFile, _T("\nargc = %d"), argc);
	for (i = 0; i < argc; i++) 
		_ftprintf (hLogFile, _T("\nargv[%d] = %s"), i, argv[i]);
	FileLogEvent (_T("Entering ServiceMain."), 0, FALSE);

	hServStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	hServStatus.dwCurrentState = SERVICE_START_PENDING;
	hServStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
		SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	hServStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	hServStatus.dwServiceSpecificExitCode = 0;
	hServStatus.dwCheckPoint = 0;
	hServStatus.dwWaitHint = 2*UPDATE_TIME;

	hSStat = RegisterServiceCtrlHandler( ServiceName, ServerCtrlHandler);
	if (hSStat == 0) 
		FileLogEvent (_T("Cannot register control handler"), 100, TRUE);

	FileLogEvent (_T("Control handler registered successfully"), 0, FALSE);
	SetServiceStatus (hSStat, &hServStatus);
	FileLogEvent (_T("Service status set to SERVICE_START_PENDING"), 0, FALSE);

	/*  Start the service-specific work, now that the generic work is complete */
	if (ServiceSpecific (argc, argv) != 0) {
		hServStatus.dwCurrentState = SERVICE_STOPPED;
		hServStatus.dwServiceSpecificExitCode = 1;  /* Server initilization failed */
		SetServiceStatus (hSStat, &hServStatus);
		return;
	}
	FileLogEvent (_T("Service threads shut down. Set SERVICE_STOPPED status"), 0, FALSE);
	/*  We will only return here when the ServiceSpecific function
		completes, indicating system shutdown. */
	UpdateStatus (SERVICE_STOPPED, 0);
	FileLogEvent (_T("Service status set to SERVICE_STOPPED"), 0, FALSE);
	fclose (hLogFile);  /*  Clean up everything, in general */
	return;

}


/*	Control Handler Function */
VOID WINAPI ServerCtrlHandler( DWORD Control)
 // requested control code 
{
	switch (Control) {
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		ShutFlag = TRUE;	/* Set the global ShutFlag flag */
		UpdateStatus (SERVICE_STOP_PENDING, -1);
		break;
	case SERVICE_CONTROL_PAUSE:
		break;
	case SERVICE_CONTROL_CONTINUE:
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		if (Control > 127 && Control < 256) /* User Defined */
		break;
	}
	UpdateStatus (-1, -1);
	return;
}

void UpdateStatus (int NewStatus, int Check)
/*  Set a new service status and checkpoint (either specific value or increment) */
{
	if (Check < 0 ) hServStatus.dwCheckPoint++;
	else			hServStatus.dwCheckPoint = Check;
	if (NewStatus >= 0) hServStatus.dwCurrentState = NewStatus;
	if (!SetServiceStatus (hSStat, &hServStatus))
		FileLogEvent (_T("Cannot set service status"), 101, TRUE);
	return;
}

/*	FileLogEvent is similar to the ReportError function used elsewhere
	For a service, however, we ReportEvent rather than write to standard
	error. Eventually, this function should go into the utility
	library.  */

VOID FileLogEvent (LPCTSTR UserMessage, DWORD EventCode, BOOL PrintErrorMsg)

/*  General-purpose function for reporting system errors.
	Obtain the error number and turn it into the system error message.
	Display this information and the user-specified message to the open log FILE
	UserMessage:		Message to be displayed to standard error device.
	EventCode:			
	PrintErrorMessage:	Display the last system error message if this flag is set. */
{
	DWORD eMsgLen, ErrNum = GetLastError ();
	LPTSTR lpvSysMsg;
	TCHAR MessageBuffer[512];
	/*  Not much to do if this fails but to keep trying. */

	if (PrintErrorMsg) {
		eMsgLen = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			ErrNum, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpvSysMsg, 0, NULL);

		_stprintf (MessageBuffer, _T("\n%s %s ErrNum = %d. EventCode = %d."),
			UserMessage, lpvSysMsg, ErrNum, EventCode);
		if (lpvSysMsg != NULL) LocalFree (lpvSysMsg);
				/* Explained in Chapter 5. */
	} else {
		_stprintf (MessageBuffer, _T("\n%s EventCode = %d."),
			UserMessage, EventCode);
	}

	fputs (MessageBuffer, hLogFile);
	return;
}


/*	This is the service-specific function, or "main" and is 
	called from the more generic ServiceMain
	It calls a renamed version of serverSK from
	Chapter 12.
	In general, you could use a separate source file, or even
	put this in a DLL.  */

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Chapter 12. Client/Server. SERVER PROGRAM.  SOCKET VERSION	*/
/* MODIFIED TO BE A SERVICE										*/
/* Execute the command in the request and return a response.	*/
/* Commands will be exeuted in process if a shared library 	*/
/* entry point can be located, and out of process otherwise	*/
/* ADDITIONAL FEATURE: argv[1] can be the name of a DLL supporting */
/* in process services */

struct sockaddr_in SrvSAddr;		/* Server's Socket address structure */
struct sockaddr_in ConnectSAddr;	/* Connected socket with client details   */
WSADATA WSStartData;				/* Socket library data structure   */

typedef struct SERVER_ARG_TAG { /* Server thread arguments */
	volatile DWORD	number;
	volatile SOCKET	sock;
	volatile DWORD	status; /* 0: Does not exist, 1: Stopped, 2: Running 
				3: Stop entire system */
	volatile HANDLE srv_thd;
	HINSTANCE	 dlhandle; /* Shared libary handle */
} SERVER_ARG;

static BOOL ReceiveRequestMessage (REQUEST *pRequest, SOCKET);
static BOOL SendResponseMessage (RESPONSE *pResponse, SOCKET);
static DWORD WINAPI Server (SERVER_ARG *);
static DWORD WINAPI AcceptTh (SERVER_ARG *);
static DWORD WINAPI CheckpointTh (SERVER_ARG *);

static HANDLE srv_thd[MAX_CLIENTS];
static SOCKET SrvSock = INVALID_SOCKET, ConnectSock = INVALID_SOCKET;

int ServiceSpecific (int argc, LPTSTR argv[])
/* int _tmain (int argc, LPCTSTR argv []) */
{
	/* serverSK source goes here */
   return 0;
}