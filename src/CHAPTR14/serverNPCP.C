/*  Chapter 14. ServerCP.
 *	Multi-threaded command line server. Named pipe version, COMPLETION PORT example
 *	Usage:	Server[UserName GroupName]
 *	ONE PIPE INSTANCE FOR EVERY CLIENT. 
 *  Assume MAX_SERVER_TH <= 64 (because of WaitForMultipleObject) */

#include "Everything.h"
#include "ClientServer.h" /* Request and response messages defined here */

typedef struct {				/* Argument to a server thread. */
	HANDLE hCompPort;			/* Completion port handle. */
	DWORD threadNum;
} SERVER_THREAD_ARG;
typedef SERVER_THREAD_ARG *LPSERVER_THREAD_ARG;

enum CP_CLIENT_PIPE_STATE { connected, requestRead, computed, responding, respondLast };
/*	Argument structure for each named pipe instance */
typedef struct {	/* Completion port keys refer to these structures */
		HANDLE hCompPort;
		HANDLE	hNp;	/* which represent outstanding ReadFile  */
		HANDLE hTempFile;	
		FILE *tFp;		/* Used by server process to hold result */
		TCHAR tmpFileName[MAX_PATH]; /* Temporary file name for respnse. */
		REQUEST	request;	/* and ConnectNamedPipe operations */
		DWORD nBytes;
		enum CP_CLIENT_PIPE_STATE npState;
		LPOVERLAPPED pOverLap;
} CP_KEY;

OVERLAPPED overLap;
volatile static int shutDown = 0;
static DWORD WINAPI Server (LPSERVER_THREAD_ARG);
static DWORD WINAPI ServerBroadcast (LPLONG);
static BOOL  WINAPI Handler (DWORD);
static DWORD WINAPI ComputeThread (PVOID);

static CP_KEY Key[MAX_CLIENTS_CP];

_tmain (int argc, LPTSTR argv[])
{
	HANDLE hCompPort, hMonitor, hSrvrThread[MAX_CLIENTS];
	DWORD iNp, iTh;
	DWORD AceMasks[] =	/* Named pipe access rights */
		{STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0X1FF, 0, 0 };
	SECURITY_ATTRIBUTES tempFileSA = {sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
	SERVER_THREAD_ARG ThArgs[MAX_SERVER_TH]; /* MAX_SERVER_TH <= 64 */
	OVERLAPPED ov = {0};

	if (!WindowsVersionOK (6, 0)) 
		ReportError (_T("This program requires Windows NT 6.0 or greater"), 1, FALSE);

	/* Console control handler to permit server shutDown */
	if (!SetConsoleCtrlHandler (Handler, TRUE))
		ReportError (_T("Cannot create Ctrl handler"), 1, TRUE);

	/* Pipe security is commented out for simplicity */
//	if (argc == 4)		/* Optional pipe security. */
//		pNPSA = InitializeUnixSA (0440, argv[1], argv[2], AceMasks, &hSecHeap);
			
	/* Create a thread broadcast pipe name periodically. */
	hMonitor = (HANDLE) _beginthreadex (NULL, 0, ServerBroadcast, NULL, 0, NULL);

	hCompPort = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, MAX_SERVER_TH); 
	if (hCompPort == NULL) ReportError (_T("Failure to create completion port"), 2, TRUE);

	/*	Create an overlapped named pipe for every potential client, */
	/*	add to the completion port */
	/*	The assumption is that the maximum number of clients far exceeds */
	/*	the number of server threads	*/
	for (iNp = 0; iNp < MAX_CLIENTS_CP; iNp++) {
		memset (&Key[iNp], 0, sizeof(CP_KEY));
		Key[iNp].hCompPort = hCompPort;
		Key[iNp].hNp =  CreateNamedPipe ( SERVER_PIPE, 
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE,
			MAX_CLIENTS_CP, 0, 0, INFINITE, &tempFileSA);
		if (Key[iNp].hNp == INVALID_HANDLE_VALUE) 
			ReportError (_T("Error creating named pipe"), 3, TRUE);
		GetTempFileName (_T ("."), _T ("CLP"), 0, Key[iNp].tmpFileName);
		Key[iNp].pOverLap = &overLap;
		/*	Add the named pipe instance to the completion port */
		if (CreateIoCompletionPort (Key[iNp].hNp, hCompPort, (ULONG_PTR)&Key[iNp], 0) == NULL)
			ReportError (_T("Error adding NP handle to CP"), 4, TRUE);
		if (!ConnectNamedPipe (Key[iNp].hNp, &ov)
			&& GetLastError() != ERROR_IO_PENDING) 
			ReportError (_T("ConnectNamedPipe error in main thread"), 6, TRUE);
		Key[iNp].npState = connected;
	}

	/*	Create server worker threads and a temp file name for each.*/
	for (iTh = 0; iTh < MAX_SERVER_TH; iTh++) {
		ThArgs[iTh].hCompPort = hCompPort;
		ThArgs[iTh].threadNum = iTh;
		hSrvrThread[iTh] = (HANDLE)_beginthreadex (NULL, 0, Server, &ThArgs[iTh], 0, NULL);
		if (hSrvrThread[iTh] == NULL)
			ReportError (_T ("Failure to create server thread."), 2, TRUE);
	}
	_tprintf (_T("All server threads running.\n"));

	/* Wait for all server threads (<= 64) to terminate. */
	WaitForMultipleObjects (MAX_SERVER_TH, hSrvrThread, TRUE, INFINITE);
	_tprintf (_T ("All Server worker threads have shut down.\n"));
	WaitForSingleObject (hMonitor, INFINITE);
	_tprintf (_T ("Monitor thread has shut down.\n"));

	CloseHandle (hMonitor);
	for (iTh = 0; iTh < MAX_SERVER_TH; iTh++) { 
		/* Close pipe handles */
		CloseHandle (hSrvrThread[iTh]);
	}
	CloseHandle (hCompPort);

	_tprintf (_T ("Server process will exit.\n"));
	return 0;
}

static DWORD WINAPI Server (LPSERVER_THREAD_ARG pThArg)
/* Server thread function. . */
{
	/* Each thread keeps its own request, response,
	and bookkeeping data structures on the stack. */

	DWORD nXfer;
	BOOL disconnect = FALSE;
	CP_KEY *pKey;
	RESPONSE response;
	OVERLAPPED serverOv = {0}, * pOv = NULL;

	/*	Main server thread loop to process completions */
	while (!disconnect && !shutDown) {	/* Wait for an outstanding operation to complete */
		
		if (!GetQueuedCompletionStatus (pThArg->hCompPort, &nXfer, (PULONG_PTR)&pKey, &pOv, INFINITE))
		{
			DWORD errCode = GetLastError();
			if (errCode == ERROR_OPERATION_ABORTED) continue;
			if (errCode != ERROR_MORE_DATA)
				ReportError (_T("GetQueuedCompletionStatus error."), 0, TRUE);
		}
		/* Contrary to the documentation, this can return a false, indicating more data */
		if (shutDown) continue;

		/* The npState indicates which NP operation completed: connected, requestRead, ... */
		switch (pKey->npState) {
			case connected:
				{ /* A connection has completed, read a request */
					/* Open the temporary results file for this connection. */
					// _tprintf (_T("case connected.\n")); /* You may want to enable these trace statemetns */
					
					_tcscpy (pKey->request.record, _T(""));
					pKey->request.rqLen = 0;
					pKey->npState = requestRead;
					disconnect = !ReadFile (pKey->hNp, &(pKey->request), RQ_SIZE, &(pKey->nBytes), &serverOv)
						&& GetLastError() != ERROR_IO_PENDING; 
					continue;
				} 
			case requestRead:
				{ /* A read has completed. process the request */
					/* Create a separate thread to carry out the request asynchronously. */
					/* This server is then free to process other requests. */
					HANDLE hComputeThread;
					DWORD computeExitCode;
					// _tprintf (_T("case requestRead.\n"));
					hComputeThread = (HANDLE)_beginthreadex (NULL, 0, ComputeThread, pKey, 0, NULL);
					if (NULL == hComputeThread) continue;
					WaitForSingleObject(hComputeThread, INFINITE);
					GetExitCodeThread (hComputeThread, &computeExitCode);
					CloseHandle (hComputeThread);

					pKey->npState = computed;
					if (computeExitCode != 0)
					{
						pKey->npState = respondLast;
					}

					if (!PostQueuedCompletionStatus (pKey->hCompPort, 0, (ULONG_PTR)pKey, pKey->pOverLap))
						ReportError (_T("Failed to post completion status in compute thread.\n"), 0, TRUE);

					continue;
				}
			case computed:
				{
					/* Results are in the temp file */
					/* Respond a line at a time. It is convenient to use
					C library line-oriented routines at this point. */
					// _tprintf (_T("case computed.\n"));
					pKey->tFp = _tfopen (pKey->tmpFileName, _T ("r"));
					if (pKey->tFp == NULL) {
						_tperror (_T("Failure to open command output file."));
						disconnect = 1;
						continue;
					}
					pKey->npState = responding;
					if (_fgetts (response.record, MAX_RQRS_LEN, pKey->tFp) != NULL) {
						response.rsLen = strlen(response.record) + 1;
						disconnect = !WriteFile (pKey->hNp, &response, response.rsLen + sizeof(response.rsLen), &nXfer, &serverOv)
							&& GetLastError() != ERROR_IO_PENDING;
					} else {
						/* Did not read a record; post completion to get to next state */
						pKey->npState = respondLast;
						if (!PostQueuedCompletionStatus (pKey->hCompPort, 0, (ULONG_PTR)pKey, pKey->pOverLap))
							ReportError (_T("Failed to post completion status to transition to respondLast state.\n"), 20, TRUE);
					}
					continue;
				}
			case responding:
				{
					// _tprintf (_T("case responding.\n"));
					/* In the process of responding a record at a time */
					/* Continue in this state until there are no more records */
					if (_fgetts (response.record, MAX_RQRS_LEN, pKey->tFp) != NULL) {
						response.rsLen = strlen(response.record) + 1;
						disconnect = !WriteFile (pKey->hNp, &response, response.rsLen + sizeof(response.rsLen), &nXfer, &serverOv)
									&& GetLastError() != ERROR_IO_PENDING;
					} else {
						pKey->npState = respondLast;
						if (!PostQueuedCompletionStatus (pKey->hCompPort, 0, (ULONG_PTR)pKey, pKey->pOverLap))
							ReportError (_T("Failed to post completion status to transition to respondLast state\n"), 21, TRUE);
					}
					continue;
				} 
			case respondLast:
				{
					// _tprintf (_T("case respondLast.\n"));
					/* Stay connected. */
					pKey->npState = connected;
					/* Send end of response indicator. */
					_tcscpy (response.record, _T(""));
					response.rsLen = 0;
					disconnect = !WriteFile (pKey->hNp, &response, sizeof(response.rsLen), &nXfer, &serverOv) 
								&& GetLastError() != ERROR_IO_PENDING;
					continue;
				} 
			default:
				{
					// _tprintf (_T("case default.\n"));
					/* Fatal for this thread. No recovery attempted (a good exercise!) */
					ReportError (_T("Server thread is in unknown state. Terminate this thread."), 0, FALSE);
					return 1;
				}
		} 

		FlushFileBuffers (pKey->hNp);
		DisconnectNamedPipe (pKey->hNp);
		if (disconnect) {
			if (!ConnectNamedPipe (pKey->hNp, &serverOv)
					&& GetLastError() != ERROR_IO_PENDING) 
				ReportError (_T("ConnectNamedPipe error in responseComplete"), 6, TRUE);
			pKey->npState = connected;
		} else {
			_tprintf (_T("Thread %d shutting down\n"), pThArg->threadNum);
			/* End of command processing loop. Free resources and exit from the thread. */
			DeleteFile (pKey->tmpFileName);
			_tprintf (_T ("Exiting server thread number %d\n"), pThArg->threadNum);
			return 0;
		}
	}
	return 0;
}

static DWORD WINAPI ComputeThread (PVOID pArg)
{
	PROCESS_INFORMATION procInfo;
	STARTUPINFO startInfo;
	CP_KEY *pKey = (CP_KEY *)pArg;
	SECURITY_ATTRIBUTES tempSA = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	GetStartupInfo (&startInfo);

	/* Open temp file, erasing previous contents */
	pKey->hTempFile = 
		CreateFile (pKey->tmpFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, &tempSA,
		CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);

	if (pKey->hTempFile != INVALID_HANDLE_VALUE) {
		startInfo.hStdOutput = pKey->hTempFile;
		startInfo.hStdError = pKey->hTempFile;
		startInfo.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
		startInfo.dwFlags = STARTF_USESTDHANDLES;
		if (!CreateProcess (NULL, pKey->request.record, NULL, NULL, TRUE, /* Inherit handles. */
				0, NULL, NULL, &startInfo, &procInfo))
			ReportError (_T("Cannot create process for server."), 0, TRUE);

		/* Server process is running */
		CloseHandle (procInfo.hThread);
		WaitForSingleObject (procInfo.hProcess, INFINITE);
		CloseHandle (procInfo.hProcess);
		CloseHandle(pKey->hTempFile);
	} else {
		ReportError (_T("Failed to open temp file for server processing."), 0, TRUE);
		return 1;
	}

	return 0;
}

static DWORD WINAPI ServerBroadcast (LPLONG pNull)
{
	MS_MESSAGE MsNotify;
	DWORD nXfer, iNp;
	HANDLE hMsFile;

	/* Open the mailslot for the MS "client" writer. */
	while (!shutDown) { /* Run as long as there are server threads */
		/* Wait for another client to open a mailslot. */
		Sleep (CS_TIMEOUT);
		hMsFile = CreateFile (MS_CLTNAME, GENERIC_WRITE | GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hMsFile == INVALID_HANDLE_VALUE) continue;

		/* Send out the message to the mailslot. */

		MsNotify.msStatus = 0;
		MsNotify.msUtilization = 0;
		_tcscpy (MsNotify.msName, SERVER_PIPE);
		if (!WriteFile (hMsFile, &MsNotify, MSM_SIZE, &nXfer, NULL))
			ReportError (_T ("Server MS Write error."), 13, TRUE);
		CloseHandle (hMsFile);
	}

	_tprintf (_T("Shut down flag set. Cancel all outstanding I/O operations. NT6 dependency!\n"));
    /* Cancel all outstanding I/O. This is an NT6 dependency. What happens without this */
	for (iNp = 0; iNp < MAX_CLIENTS_CP; iNp++) {
		CancelIoEx (Key[iNp].hNp, NULL);
	}
	_tprintf (_T ("Shuting down monitor thread.\n"));

	_endthreadex (0);
	return 0;
}


BOOL WINAPI Handler (DWORD CtrlEvent)
{
	/* shutDown the system */
	_tprintf (_T("In console control handler\n"));
	InterlockedIncrement(&shutDown);
	return TRUE;
}
