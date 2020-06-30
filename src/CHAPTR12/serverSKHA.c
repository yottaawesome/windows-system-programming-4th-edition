/* Chapter 12. serverSKHA.c													*/
/* Client/Server. SERVER PROGRAM.  SOCKET, STREAM VERSION					*/
/*		THIS VERSSION ALSO USES A "SOCKET HANDLE" TO PRESERVE STATE			*/
/*		IT IS A SLIGHT VARIATION OF serverSKST.c							*/
/* Execute the command in the request and return a response.				*/
/* Commands will be executed in process if a shared library					*/
/* entry point can be located, and out of process otherwise					*/
/* ADDITIONAL FEATURE: argv[1] can be the name of a DLL supporting			*/
/* in process services														*/
/* This implementation differs from serverSK in that it uses a				*/
/* DLL to support socket I/O; the DLL provided uses streams with			*/
/* end of lines to terminate messages, rather than use length headers		*/
/* The differences are all hidden in ReceiveCSMessage and					*/
/* SendCSMessage (renamings from serverSK) so that there are no differences in */
/* the main program. Just the function implementations have been removed	*/
/* and the two message functions are imported */

#include "Everything.h"
#include "ClientServer.h"	/* Defines the request and response records. */

struct sockaddr_in srvSAddr;		/* Server's Socket address structure */
struct sockaddr_in connectSAddr;	/* Connected socket with client details   */
WSADATA WSStartData;				/* Socket library data structure   */

typedef struct SERVER_ARG_TAG { /* Server thread arguments */
	volatile DWORD	number;
	volatile SOCKET sock;
	volatile DWORD	status; /* 0: Does not exist, 1: Stopped, 2: Running 
				3: Stop entire system */
	volatile HANDLE hSrvThread;
	HINSTANCE hDll; /* Shared libary handle */
} SERVER_ARG;

__declspec (dllimport) PVOID CreateCSSocketHandle (SOCKET);
__declspec (dllimport) BOOL CloseCSSocketHandle (PVOID);
__declspec (dllimport) BOOL ReceiveCSMessage (REQUEST *pRequest, PVOID);
__declspec (dllimport) BOOL SendCSMessage (RESPONSE *pResponse, PVOID);
static DWORD WINAPI Server (SERVER_ARG *);
static DWORD WINAPI AcceptThread (SERVER_ARG *);
static BOOL  WINAPI Handler (DWORD);

static HANDLE hSrvThread[MAX_CLIENTS];
volatile static shutFlag = FALSE;
static SOCKET serverSocket = INVALID_SOCKET, ConnectSock = INVALID_SOCKET;

int _tmain (int argc, LPCTSTR argv [])
{
	/* Server listening and connected sockets. */
	BOOL done = FALSE;
	DWORD iThread, tStatus;
	SERVER_ARG serverArg[MAX_CLIENTS];
	HANDLE hAcceptTh = NULL;
	HINSTANCE hDll = NULL;

	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater"), 1, FALSE);


	/* Console control handler to permit server shutdown */
	if (!SetConsoleCtrlHandler (Handler, TRUE))
		ReportError (_T("Cannot create Ctrl handler"), 1, TRUE);

	/*	Initialize the WS library. Ver 1.1 */
	if (WSAStartup (MAKEWORD (1, 1), &WSStartData) != 0)
		ReportError (_T("Cannot support sockets"), 1, TRUE);
	
	/* Open the shared command library DLL if it is specifiec on command line */
	if (argc > 1) {
		hDll = LoadLibrary (argv[1]);
		if (hDll == NULL) ReportError (argv[1], 0, TRUE);
	}

	/* Intialize thread arg array */
	for (iThread = 0; iThread < MAX_CLIENTS; iThread++) {
		serverArg[iThread].number = iThread;
		serverArg[iThread].status = 0;
		serverArg[iThread].sock = 0;
		serverArg[iThread].hDll = hDll;
		serverArg[iThread].hSrvThread = NULL;
	}
	/*	Follow the standard server socket/bind/listen/accept sequence */
	serverSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) 
		ReportError (_T("Failed server socket() call"), 1, TRUE);
    
	/*	Prepare the socket address structure for binding the
	    	server socket to port number "reserved" for this service.
	    	Accept requests from any client machine.  */

    srvSAddr.sin_family = AF_INET;	
    srvSAddr.sin_addr.s_addr = htonl( INADDR_ANY );    
    srvSAddr.sin_port = htons( SERVER_PORT );	
	if (bind (serverSocket, (struct sockaddr *)&srvSAddr, sizeof(srvSAddr)) == SOCKET_ERROR)
		ReportError (_T("Failed server bind() call"), 2, TRUE);
	if (listen (serverSocket, MAX_CLIENTS) != 0) 
		ReportError (_T("Server listen() error"), 3, TRUE);

	/* Main thread becomes listening/connecting/monitoring thread */
	/* Find an empty slot in the server thread arg array */
	/* status values:	0 - Slot is free;	1 - thread stopped; 
						2 - thread running; 3 - Stop entire system */
	while (!shutFlag) {
		for (iThread = 0; iThread < MAX_CLIENTS && !shutFlag; ) {
			if (serverArg[iThread].status == 1 || serverArg[iThread].status == 3) {
				/* This thread stopped, either normally or with a shutdown request */
				tStatus = WaitForSingleObject (serverArg[iThread].hSrvThread, INFINITE);
				if (tStatus != WAIT_OBJECT_0) 
					ReportError (_T("Server thread wait error"), 4, TRUE);
				CloseHandle (serverArg[iThread].hSrvThread);
				if (serverArg[iThread].status == 3) shutFlag = TRUE;
				else serverArg[iThread].status = 0; /* Free thread slot */
			}				
			if (serverArg[iThread].status == 0 || shutFlag) break;
			iThread = (iThread+1) % MAX_CLIENTS;
			if (iThread == 0) Sleep(1000); /* Break the polling loop */
			/* An alternative would be to use an event to signal a free slot */
		}

		/* Wait for a connection on this socket */
		/* Use a separate thread so that we can poll the shutFlag flag */
		hAcceptTh = (HANDLE)_beginthreadex (NULL, 0, AcceptThread, &serverArg[iThread], 0, NULL);
		while (!shutFlag) {
			tStatus = WaitForSingleObject (hAcceptTh, CS_TIMEOUT);
			if (tStatus == WAIT_OBJECT_0) break; /* the connection was made */
		}
		CloseHandle (hAcceptTh);
		hAcceptTh = NULL; /* prepare for next connection */
	}
	
	_tprintf (_T("Server shutdown in process. Wait for all server threads\n"));
	/* Terminate the accept thread if it is still running */
	if (hAcceptTh != NULL) TerminateThread (hAcceptTh, 0);
	/* Wait for any active server threads to terminate */
	shutdown (serverSocket, 2);
	closesocket (serverSocket);
	WSACleanup();
	for (iThread = 0; iThread < MAX_CLIENTS; iThread++) {
		if (serverArg[iThread].status != 0) WaitForSingleObject (serverArg[iThread].hSrvThread, INFINITE);
		CloseHandle (serverArg[iThread].hSrvThread);
	}
	if (hDll != NULL) FreeLibrary (hDll);	
	_tprintf (_T("All server resources deallocated. ExitProcess(0)\n"));
	ExitProcess(0);
	return 0;
}

static DWORD WINAPI AcceptThread (SERVER_ARG * pThArg)
{
	LONG AddrLen, ThId;
	
	AddrLen = sizeof(connectSAddr);
	pThArg->sock = 
		 accept (serverSocket, (struct sockaddr *)&connectSAddr, &AddrLen);
	if (pThArg->sock == INVALID_SOCKET) ReportError (_T("accept error"), 1, TRUE);
	/* A new connection. Create a server thread */
	pThArg->status = 2; 
	pThArg->hSrvThread = (HANDLE)_beginthreadex (NULL, 0, Server, pThArg, 0, &ThId);
	if (pThArg->hSrvThread == NULL) 
		ReportError (_T("Failed creating server thread"), 0, TRUE);
	return 0; /* Server thread remains running */
}


static DWORD WINAPI Server (SERVER_ARG * pThArg)

/* Server thread function. There is a thread for every potential client. */
{
	/* Each thread keeps its own request, response,
		and bookkeeping data structures on the stack. */
	/* All messages are in 8-bit characters, but the server itself is generic */
	BOOL done = FALSE;
	STARTUPINFO startInfoCh;
	SECURITY_ATTRIBUTES tempSA = {sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
	PROCESS_INFORMATION procInfo;
	SOCKET connectSocket;
	PVOID sh; /* Socket handle pointer */
	int disconnect = 0, iTokenLen;
	REQUEST request;	/* Defined in ClientServer.h */
	RESPONSE response;	/* Defined in ClientServer.h.*/
	char sysCommand[MAX_RQRS_LEN];
	TCHAR tempFile[100];
	HANDLE hTmpFile;
	FILE *fp = NULL;
	int (*dl_addr)(char *, char *);
	char *ws = " \0\t\n"; /* white space */

	GetStartupInfo (&startInfoCh);
	
	connectSocket = pThArg->sock;
	/* Create a streaming socket "handle" */
	sh = CreateCSSocketHandle (connectSocket);
	/* Create a temp file name */
	_stprintf (tempFile, _T("%s%d%s"), _T("ServerTemp"), pThArg->number, _T(".tmp"));

	while (!done && !shutFlag) { 	/* Main Command Loop. */

		disconnect = ReceiveCSMessage (&request, sh);
		done = disconnect || (strcmp (request.record, "$Quit") == 0);
		if (done) continue;	
		/* Stop this thread on "$Quit" or "$ShutDownServer" command. */

		/* Open the temporary results file. */
		hTmpFile = CreateFile (tempFile, GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, &tempSA,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hTmpFile == INVALID_HANDLE_VALUE)
			ReportError (_T("Cannot create temp file"), 1, TRUE);

		/* Check for a shared library command. For simplicity, shared 	*/
		/* library commands take precedence over process commands 	*/
		/* First, extract the command name				*/

		iTokenLen = (int)strcspn (request.record, ws); /* length of token */
		memcpy (sysCommand, request.record, iTokenLen);
		sysCommand[iTokenLen] = '\0';

		dl_addr = NULL; /* will be set if GetProcAddress succeeds */
		if (pThArg->hDll != NULL) { /* Try Server "In process" */
			dl_addr = (int (*)(char *, char *))GetProcAddress (pThArg->hDll, sysCommand);
			if (dl_addr != NULL) __try { /* Protect server process from exceptions in DLL */
				(*dl_addr)(request.record, tempFile);
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { /* Exception in the DLL */
				ReportError (_T("Exception in DLL."), 0, FALSE);
			}
		}
			
		if (dl_addr == NULL) { /* No inprocess support */
			/* Create a process to carry out the command. */
			startInfoCh.hStdOutput = hTmpFile;
			startInfoCh.hStdError = hTmpFile;
			startInfoCh.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
			startInfoCh.dwFlags = STARTF_USESTDHANDLES;

			if (!CreateProcess (NULL, request.record, NULL,
					NULL, TRUE, /* Inherit handles. */
					0, NULL, NULL, &startInfoCh, &procInfo)) {
				PrintMsg (hTmpFile, _T("ERR: Cannot create process."));
				procInfo.hProcess = NULL;
			}
			CloseHandle (hTmpFile);
			if (procInfo.hProcess != NULL ) {
				CloseHandle (procInfo.hThread);
				WaitForSingleObject (procInfo.hProcess, INFINITE);
				CloseHandle (procInfo.hProcess);
			}
		}
			
		/* Respond a line at a time. It is convenient to use
			C library line-oriented routines at this point. */

		/* Send the temp file, one line at a time, with header, to the client. */

		fp = fopen (tempFile, "r");
		
		if (fp != NULL) {
			response.rsLen = MAX_RQRS_LEN;
			while ((fgets (response.record, MAX_RQRS_LEN, fp) != NULL)) {
				SendCSMessage (&response, sh);
			}
		}
		/* Send an end message message */
		/* Arbitrarily defined as "$$$$$$$" */
		strcpy (response.record, "$$$$$$$");
		SendCSMessage (&response, sh);
		fclose (fp); fp = NULL;
		DeleteFile (tempFile); 

	}   /* End of main command loop. Get next command */

	/* End of command processing loop. Free resources and exit from the thread. */

	_tprintf (_T("Shuting down server thread number %d\n"), pThArg->number);
	CloseCSSocketHandle (sh);
	closesocket (ConnectSock);
	shutdown (ConnectSock, 2);
	pThArg->status = 1;
	if (strcmp (request.record, "$ShutDownServer") == 0)	{
		pThArg->status = 3;
		shutFlag = TRUE;
	}

	return pThArg->status;	
}


BOOL WINAPI Handler (DWORD ctrlEvent)
{
	/* Shutdown the system */
	shutFlag = TRUE;
	return TRUE;
}
