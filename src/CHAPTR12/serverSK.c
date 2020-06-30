/* Chapter 12. Client/Server. SERVER PROGRAM.  SOCKET VERSION	*/
/* Execute the command in the request and return a response.	*/
/* Commands will be exeuted in process if a shared library 	*/
/* entry point can be located, and out of process otherwise	*/
/* ADDITIONAL FEATURE: argv[1] can be the name of a DLL supporting */
/* in process services */

#include "Everything.h"
#include "ClientServer.h"	/* Defines the request and response records. */

struct sockaddr_in srvSAddr;		/* Server's Socket address structure */
struct sockaddr_in connectSAddr;	/* Connected socket with client details   */
WSADATA WSStartData;				/* Socket library data structure   */

enum SERVER_THREAD_STATE {	SERVER_SLOT_FREE, SERVER_THREAD_STOPPED, 
							SERVER_THREAD_RUNNING, SERVER_SLOT_INVALID};
typedef struct SERVER_ARG_TAG { /* Server thread arguments */
	CRITICAL_SECTION threadCs;
	DWORD	number;
	SOCKET	sock;
	enum SERVER_THREAD_STATE thState; 	
	HANDLE	hSrvThread;
	HINSTANCE	 hDll; /* Shared libary handle */
} SERVER_ARG;

static BOOL ReceiveRequestMessage (REQUEST *pRequest, SOCKET);
static BOOL SendResponseMessage (RESPONSE *pResponse, SOCKET);
static DWORD WINAPI Server (PVOID);
static DWORD WINAPI AcceptThread (PVOID);
static BOOL  WINAPI Handler (DWORD);

volatile static int shutFlag = 0;
static SOCKET SrvSock = INVALID_SOCKET, connectSock = INVALID_SOCKET;

int _tmain (int argc, LPCTSTR argv [])
{
	/* Server listening and connected sockets. */
	DWORD iThread, tStatus;
	SERVER_ARG sArgs[MAX_CLIENTS];
	HANDLE hAcceptThread = NULL;
	HINSTANCE hDll = NULL;

	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater"), 1, FALSE);

	/* Console control handler to permit server shutdown */
	if (!SetConsoleCtrlHandler (Handler, TRUE))
		ReportError (_T("Cannot create Ctrl handler"), 1, TRUE);

	/*	Initialize the WS library. Ver 2.0 */
	if (WSAStartup (MAKEWORD (2, 0), &WSStartData) != 0)
		ReportError (_T("Cannot support sockets"), 1, TRUE);
	
	/* Open the shared command library DLL if it is specified on command line */
	if (argc > 1) {
		hDll = LoadLibrary (argv[1]);
		if (hDll == NULL) ReportError (argv[1], 0, TRUE);
	}

	/* Intialize thread arg array */
	for (iThread = 0; iThread < MAX_CLIENTS; iThread++) {
		InitializeCriticalSection (&sArgs[iThread].threadCs);
		sArgs[iThread].number = iThread;
		sArgs[iThread].thState = SERVER_SLOT_FREE;
		sArgs[iThread].sock = 0;
		sArgs[iThread].hDll = hDll;
		sArgs[iThread].hSrvThread = NULL;
	}
	/*	Follow the standard server socket/bind/listen/accept sequence */
	SrvSock = socket(PF_INET, SOCK_STREAM, 0);
	if (SrvSock == INVALID_SOCKET) 
		ReportError (_T("Failed server socket() call"), 1, TRUE);
    
	/*	Prepare the socket address structure for binding the
	    	server socket to port number "reserved" for this service.
	    	Accept requests from any client machine.  */

	srvSAddr.sin_family = AF_INET;	
	srvSAddr.sin_addr.s_addr = htonl( INADDR_ANY );    
	srvSAddr.sin_port = htons( SERVER_PORT );	
	if (bind (SrvSock, (struct sockaddr *)&srvSAddr, sizeof(srvSAddr)) == SOCKET_ERROR)
		ReportError (_T("Failed server bind() call"), 2, TRUE);
	if (listen (SrvSock, MAX_CLIENTS) != 0) 
		ReportError (_T("Server listen() error"), 3, TRUE);

	/* Main thread becomes listening/connecting/monitoring thread */
	/* Find an empty slot in the server thread arg array */
	while (!shutFlag) {
		iThread = 0;
		while (!shutFlag) {
			/* Continously poll the thread thState of all server slots in the sArgs table */
			EnterCriticalSection(&sArgs[iThread].threadCs);
			__try {
				if (sArgs[iThread].thState == SERVER_THREAD_STOPPED) {
					/* This thread stopped, either normally or there's a shutdown request */
					/* Wait for it to stop, and make the slot free for another thread */
					tStatus = WaitForSingleObject (sArgs[iThread].hSrvThread, INFINITE);
					if (tStatus != WAIT_OBJECT_0)
						ReportError (_T("Server thread wait error"), 4, TRUE);
					CloseHandle (sArgs[iThread].hSrvThread);
					sArgs[iThread].hSrvThread = NULL;
					sArgs[iThread].thState = SERVER_SLOT_FREE;
				}				
				/* Free slot identified or shut down. Use a free slot for the next connection */
				if (sArgs[iThread].thState == SERVER_SLOT_FREE || shutFlag) break;
			}
			__finally { LeaveCriticalSection (&sArgs[iThread].threadCs); }

			/* Fixed July 25, 2014: iThread = (iThread++) % MAX_CLIENTS; */
			iThread = (iThread+1) % MAX_CLIENTS;
			if (iThread == 0) Sleep(50); /* Break the polling loop */
			/* An alternative would be to use an event to signal a free slot */
		}
		if (shutFlag) break;
		/* sArgs[iThread] == SERVER_SLOT_FREE */
		/* Wait for a connection on this socket */
		/* Use a separate thread so that we can poll the shutFlag flag */
		hAcceptThread = (HANDLE)_beginthreadex (NULL, 0, AcceptThread, &sArgs[iThread], 0, NULL);
		if (hAcceptThread == NULL)
			ReportError (_T("Error creating AcceptThreadread."), 1, TRUE);
		while (!shutFlag) {
			tStatus = WaitForSingleObject (hAcceptThread, CS_TIMEOUT);
			if (tStatus == WAIT_OBJECT_0) {
				/* Connection is complete. sArgs[iThread] == SERVER_THREAD_RUNNING */
				if (!shutFlag) {
					CloseHandle (hAcceptThread);
					hAcceptThread = NULL;
				}
				break;
			}
		}
	}  /* OUTER while (!shutFlag) */
	
	/* shutFlag == TRUE */
	_tprintf (_T("Server shutdown in process. Wait for all server threads\n"));
	/* Wait for any active server threads to terminate */
	/* Try continuously as some threads may be long running. */

	while (TRUE) {
		int nRunningThreads = 0;
		for (iThread = 0; iThread < MAX_CLIENTS; iThread++) {
			EnterCriticalSection(&sArgs[iThread].threadCs);
			__try {
				if (sArgs[iThread].thState == SERVER_THREAD_RUNNING || sArgs[iThread].thState == SERVER_THREAD_STOPPED) {
					if (WaitForSingleObject (sArgs[iThread].hSrvThread, 10000) == WAIT_OBJECT_0) {
						_tprintf (_T("Server thread on slot %d stopped.\n"), iThread);
						CloseHandle (sArgs[iThread].hSrvThread);
						sArgs[iThread].hSrvThread = NULL;
						sArgs[iThread].thState = SERVER_SLOT_INVALID;
					} else
						if (WaitForSingleObject (sArgs[iThread].hSrvThread, 10000) == WAIT_TIMEOUT) {
						_tprintf (_T("Server thread on slot %d still running.\n"), iThread);
						nRunningThreads++;
					} else {
						_tprintf (_T("Error waiting on server thread in slot %d.\n"), iThread);
						ReportError (_T("Thread wait failure"), 0, TRUE);
					}

				}
			}
			__finally { LeaveCriticalSection(&sArgs[iThread].threadCs); }
		}
		if (nRunningThreads == 0) break;
	}

	if (hDll != NULL) FreeLibrary (hDll);	

	/* Redundant shutdown */
	shutdown (SrvSock, SD_BOTH);
	closesocket (SrvSock);
	WSACleanup();
	if (hAcceptThread != NULL && WaitForSingleObject(hAcceptThread, INFINITE) != WAIT_OBJECT_0)
		ReportError (_T("Failed waiting for accept thread to terminate."), 7, FALSE);
	return 0;
}

static DWORD WINAPI AcceptThread (PVOID pArg)
{
	LONG addrLen;
	SERVER_ARG * pThArg = (SERVER_ARG *)pArg;
	
	addrLen = sizeof(connectSAddr);
	pThArg->sock = 
		 accept (SrvSock, (struct sockaddr *)&connectSAddr, &addrLen);
	if (pThArg->sock == INVALID_SOCKET) {
		ReportError (_T("accept: invalid socket error"), 1, TRUE);
		return 1;
	}
	/* A new connection. Create a server thread */
	EnterCriticalSection(&(pThArg->threadCs));
	__try {
		pThArg->hSrvThread = (HANDLE)_beginthreadex (NULL, 0, Server, pThArg, 0, NULL);
		if (pThArg->hSrvThread == NULL) 
			ReportError (_T("Failed creating server thread"), 1, TRUE);
		pThArg->thState = SERVER_THREAD_RUNNING; 
		_tprintf (_T("Client accepted on slot: %d, using server thread %d.\n"), pThArg->number, GetThreadId (pThArg->hSrvThread));
		/* Exercise: Display client machine and process information */
	}
	__finally { LeaveCriticalSection(&(pThArg->threadCs)); }
	return 0;
}

BOOL WINAPI Handler (DWORD CtrlEvent)
{
	/* Recives ^C. Shutdown the system */
	_tprintf (_T("In console control handler\n"));
	InterlockedIncrement (&shutFlag);
	return TRUE;
}

static DWORD WINAPI Server (PVOID pArg)

/* Server thread function. There is a thread for every potential client. */
{
	/* Each thread keeps its own request, response,
		and bookkeeping data structures on the stack. */
	/* NOTE: All messages are in 8-bit characters */

	BOOL done = FALSE;
	STARTUPINFO startInfoCh;
	SECURITY_ATTRIBUTES tempSA = {sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
	PROCESS_INFORMATION procInfo;
	SOCKET connectSock;
	int commandLen;
	REQUEST request;	/* Defined in ClientServer.h */
	RESPONSE response;	/* Defined in ClientServer.h.*/
	char sysCommand[MAX_RQRS_LEN];
	TCHAR tempFile[100];
	HANDLE hTmpFile;
	FILE *fp = NULL;
	int (__cdecl *dl_addr)(char *, char *);
	SERVER_ARG * pThArg = (SERVER_ARG *)pArg;
	enum SERVER_THREAD_STATE threadState;

	GetStartupInfo (&startInfoCh);
	
	connectSock = pThArg->sock;
	/* Create a temp file name */
	tempFile[sizeof(tempFile)/sizeof(TCHAR) - 1] = _T('\0');
	_stprintf_s (tempFile, sizeof(tempFile)/sizeof(TCHAR) - 1, _T("ServerTemp%d.tmp"), pThArg->number);

	while (!done && !shutFlag) { 	/* Main Server Command Loop. */
		done = ReceiveRequestMessage (&request, connectSock);

		request.record[sizeof(request.record)-1] = '\0';
		commandLen = (int)(strcspn (request.record, "\n\t"));
		memcpy (sysCommand, request.record, commandLen);
		sysCommand[commandLen] = '\0';
		_tprintf (_T("Command received on server slot %d: %s\n"), pThArg->number, sysCommand);

		/* Restest shutFlag, as it can be set from the console control handler. */
		done = done || (strcmp (request.record, "$Quit") == 0) || shutFlag;
		if (done) continue;	

		/* Open the temporary results file. */
		hTmpFile = CreateFile (tempFile, GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, &tempSA,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hTmpFile == INVALID_HANDLE_VALUE)
			ReportError (_T("Cannot create temp file"), 1, TRUE);

		/* Check for a shared library command. For simplicity, shared 	*/
		/* library commands take precedence over process commands 	*/
		/* First, extract the command name (space delimited) */
		dl_addr = NULL; /* will be set if GetProcAddress succeeds */
		if (pThArg->hDll != NULL) { /* Try Server "In process" */
			char commandName[256] = "";
			int commandNameLength = (int)(strcspn (sysCommand, " "));
			strncpy_s (commandName, sizeof(commandName), sysCommand, min(commandNameLength, sizeof(commandName)-1));
			dl_addr = (int (*)(char *, char *))GetProcAddress (pThArg->hDll, commandName);
			/* You really need to trust this DLL not to corrupt the server */
			/* This assumes that we don't allow the DLL to generate known exceptions */
			if (dl_addr != NULL) __try { 
				(*dl_addr)(request.record, tempFile);
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { /* Exception in the DLL */
				_tprintf (_T("Unhandled Exception in DLL. Terminate server. There may be orphaned processes."));
				return 1;
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
		if (_tfopen_s (&fp, tempFile, _T("r")) == 0) {
			{	
				response.rsLen = MAX_RQRS_LEN;
				while ((fgets (response.record, MAX_RQRS_LEN, fp) != NULL)) 
					SendResponseMessage (&response, connectSock);
			}
			/* Send a zero length message. Messages are 8-bit characters, not UNICODE. */
			response.record[0] = '\0';
			SendResponseMessage (&response, connectSock);
			fclose (fp); fp = NULL;
			DeleteFile (tempFile); 
		}
		else {
			ReportError(_T("Failed to open temp file with command results"), 0, TRUE);
		}

	}   /* End of main command loop. Get next command */
	
	/* done || shutFlag */
	/* End of command processing loop. Free resources and exit from the thread. */
	_tprintf (_T("Shuting down server thread number %d\n"), pThArg->number);
	/* Redundant shutdown. There are no further attempts to send or receive */
	shutdown (connectSock, SD_BOTH); 
	closesocket (connectSock);

	EnterCriticalSection(&(pThArg->threadCs));
	__try {
		threadState = pThArg->thState = SERVER_THREAD_STOPPED;
	}
	__finally { LeaveCriticalSection(&(pThArg->threadCs)); }

	return threadState;	
}

BOOL ReceiveRequestMessage (REQUEST *pRequest, SOCKET sd)
{
	BOOL disconnect = FALSE;
	LONG32 nRemainRecv = 0, nXfer;
	LPBYTE pBuffer;

	/*	Read the request. First the header, then the request text. */
	nRemainRecv = RQ_HEADER_LEN; 
	pBuffer = (LPBYTE)pRequest;

	while (nRemainRecv > 0 && !disconnect) {
		nXfer = recv (sd, pBuffer, nRemainRecv, 0);
		if (nXfer == SOCKET_ERROR) 
			ReportError (_T("server request recv() failed"), 11, TRUE);
		disconnect = (nXfer == 0);
		nRemainRecv -=nXfer; pBuffer += nXfer;
	}
	
	/*	Read the request record */
	nRemainRecv = pRequest->rqLen;
	/* Exclude buffer overflow */
	nRemainRecv = min(nRemainRecv, MAX_RQRS_LEN);

	pBuffer = (LPSTR)pRequest->record;
	while (nRemainRecv > 0 && !disconnect) {
		nXfer = recv (sd, pBuffer, nRemainRecv, 0);
		if (nXfer == SOCKET_ERROR) 
			ReportError (_T("server request recv() failed"), 12, TRUE);
		disconnect = (nXfer == 0);
		nRemainRecv -=nXfer; pBuffer += nXfer;
	}

	return disconnect;
}

BOOL SendResponseMessage (RESPONSE *pResponse, SOCKET sd)
{
	BOOL disconnect = FALSE;
	LONG32 nRemainRecv = 0, nXfer, nRemainSend;
	LPBYTE pBuffer;

	/*	Send the response up to the string end. Send in 
		two parts - header, then the response string. */
	nRemainSend = RS_HEADER_LEN; 
	pResponse->rsLen = (long)(strlen (pResponse->record)+1);
	pBuffer = (LPBYTE)pResponse;
	while (nRemainSend > 0 && !disconnect) {
		nXfer = send (sd, pBuffer, nRemainSend, 0);
		if (nXfer == SOCKET_ERROR) ReportError (_T("server send() failed"), 13, TRUE);
		disconnect = (nXfer == 0);
		nRemainSend -=nXfer; pBuffer += nXfer;
	}

	nRemainSend = pResponse->rsLen;
	pBuffer = (LPSTR)pResponse->record;
	while (nRemainSend > 0 && !disconnect) {
		nXfer = send (sd, pBuffer, nRemainSend, 0);
		if (nXfer == SOCKET_ERROR) ReportError (_T("server send() failed"), 14, TRUE);
		disconnect = (nXfer == 0);
		nRemainSend -=nXfer; pBuffer += nXfer;
	}
	return disconnect;
}
