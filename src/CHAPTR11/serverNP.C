/*  Chapter 11. ServerNP.
 *	Multi-threaded command line server. Named pipe version
 *	Usage:	Server [UserName GroupName]
 *	ONE THREAD AND PIPE INSTANCE FOR EVERY CLIENT. */

#include "Everything.h"
#include "ClientServer.h" /* Request and Response messages defined here */

typedef struct {				/* Argument to a server thread. */
	HANDLE hNamedPipe;			/* Named pipe instance. */
	DWORD threadNumber;
	TCHAR tempFileName[MAX_PATH]; /* Temporary file name. */
} THREAD_ARG;
typedef THREAD_ARG *LPTHREAD_ARG;

volatile static int shutDown = 0;
static DWORD WINAPI Server (LPTHREAD_ARG);
static DWORD WINAPI Connect (LPTHREAD_ARG);
static DWORD WINAPI ServerBroadcast (LPLONG);
static BOOL  WINAPI Handler (DWORD);
static TCHAR shutRequest[] = _T("$ShutDownServer");
static THREAD_ARG threadArgs[MAX_CLIENTS];

_tmain (int argc, LPTSTR argv[])
{
	/* MAX_CLIENTS is defined in ClientServer.h. */
	/* Currently limited to MAXIMUM_WAIT_OBJECTS WaitForMultipleObjects */
	/* is used by the main thread to wait for the server threads */

	HANDLE hNp, hMonitor, hSrvrThread[MAX_CLIENTS];
	DWORD iNp, monitorId, threadId;
	DWORD AceMasks[] =	/* Named pipe access rights - described in Chapter 15 */
		{STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0X1FF, 0, 0 };
	LPSECURITY_ATTRIBUTES pNPSA = NULL;

	if (!WindowsVersionOK (6, 0)) 
		ReportError (_T("This program requires Windows NT 6.0 or greater"), 1, FALSE);

	/* Console control handler to permit server shut down */
	if (!SetConsoleCtrlHandler (Handler, TRUE))
		ReportError (_T("Cannot create Ctrl handler"), 1, TRUE);

	/* Pipe security is commented out for simplicity - See chapter 16 */
//	if (argc == 4)		/* Optional pipe security - Uses a simpler function. */
//		pNPSA = InitializeAccessOnlySA (0440, argv[1], argv[2], AceMasks, &hSecHeap);
			
	/* Create a thread broadcast pipe name periodically. */
	hMonitor = (HANDLE) _beginthreadex (NULL, 0, ServerBroadcast, NULL, 0, &monitorId);

	/*	Create a pipe instance for every server thread.
	 *	Create a temp file name for each thread.
	 *	Create a thread to service that pipe. */

	for (iNp = 0; iNp < MAX_CLIENTS; iNp++) {
		hNp = CreateNamedPipe ( SERVER_PIPE, PIPE_ACCESS_DUPLEX,
				PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE | PIPE_WAIT,
				MAX_CLIENTS, 0, 0, INFINITE, pNPSA);

		if (hNp == INVALID_HANDLE_VALUE)
			ReportError (_T ("Failure to open named pipe."), 1, TRUE);
		threadArgs[iNp].hNamedPipe = hNp;
		threadArgs[iNp].threadNumber = iNp;
		GetTempFileName (_T ("."), _T ("CLP"), 0, threadArgs[iNp].tempFileName);
		hSrvrThread[iNp] = (HANDLE)_beginthreadex (NULL, 0, Server,
				&threadArgs[iNp], 0, &threadId);
		if (hSrvrThread[iNp] == NULL)
			ReportError (_T ("Failure to create server thread."), 2, TRUE);
	}
	
	/* Wait for all the threads to terminate. */

	WaitForMultipleObjects (MAX_CLIENTS, hSrvrThread, TRUE, INFINITE);
	_tprintf (_T ("All Server worker threads have shut down.\n"));

	WaitForSingleObject (hMonitor, INFINITE);
	_tprintf (_T ("Monitor thread has shut down.\n"));

	CloseHandle (hMonitor);
	for (iNp = 0; iNp < MAX_CLIENTS; iNp++) { 
		/* Close pipe handles and delete temp files */
		/* Closing temp files is redundant, as the worker threads do it */
		CloseHandle (hSrvrThread[iNp]);
		DeleteFile (threadArgs[iNp].tempFileName);
	}

	_tprintf (_T ("Server process will exit.\n"));
	return 0;
}


static DWORD WINAPI Server (LPTHREAD_ARG pThArg)

/* Server thread function. There is a thread for every potential client. */
{
	/* Each thread keeps its own request, response,
		and bookkeeping data structures on the stack. 
		Also, each thread creates an additional "connect thread"
		so that the main worker thread can test the shut down flag
		periodically while waiting for a client connection. */

	HANDLE hNamedPipe, hTmpFile = INVALID_HANDLE_VALUE, hConTh = NULL, hClient;
	DWORD nXfer, conThStatus, clientProcessId;
	STARTUPINFO startInfoCh;
	SECURITY_ATTRIBUTES tempSA = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	PROCESS_INFORMATION procInfo;
	FILE *fp;
	REQUEST request;
	RESPONSE response;
	TCHAR clientName[256];

	GetStartupInfo (&startInfoCh);
	hNamedPipe = pThArg->hNamedPipe;

	while (!shutDown) { 	/* Connection loop */

		/* Create a connection thread, and wait for it to terminate */
		/* Use a timeout on the wait so that the shut down flag can be tested */
		hConTh = (HANDLE)_beginthreadex (NULL, 0, Connect, pThArg, 0, NULL);
		if (hConTh == NULL) {
			ReportError (_T("Cannot create connect thread"), 0, TRUE);
			_endthreadex(2);
		}

		/* Wait for a client connection. */	
		while (!shutDown && WaitForSingleObject (hConTh, CS_TIMEOUT) == WAIT_TIMEOUT) 
			{ /* Empty loop body */};
		if (shutDown) _tprintf (_T("Thread %d received shut down\n"), pThArg->threadNumber);
		if (shutDown) continue;	/* Flag could also be set by a different thread */

		CloseHandle (hConTh); hConTh = NULL;
		/* A connection now exists */
        
		if (!GetNamedPipeClientComputerName(pThArg->hNamedPipe, clientName, sizeof(clientName))) {
			_tcscpy_s(clientName, sizeof(clientName)/sizeof(TCHAR)-1, _T("localhost"));
		}
        GetNamedPipeClientProcessId(pThArg->hNamedPipe, &clientProcessId);
		_tprintf(_T("Connect to client process id: %d on computer: %s\n"), clientProcessId, clientName);
        
		while (!shutDown && ReadFile (hNamedPipe, &request, RQ_SIZE, &nXfer, NULL)) {
			/* Receive new commands until the client disconnects */
			_tprintf(_T("Command from client thread: %d. %s\n"), clientProcessId, request.record);
			shutDown = shutDown || (_tcscmp (request.record, shutRequest) == 0);
			if (shutDown)  continue;

			/* Open the temporary results file used by all connections to this instance. */
			hTmpFile = CreateFile (pThArg->tempFileName, GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, &tempSA,
				CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
			if (hTmpFile == INVALID_HANDLE_VALUE) { /* About all we can do is stop the thread */
				ReportError (_T("Cannot create temp file"), 0, TRUE);
				_endthreadex(1);
			}

			/* Main command loop */
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

			CloseHandle(hTmpFile);  /* The child process is using the temp file, but the parent handle isn't needed */
			if (procInfo.hProcess != NULL ) { /* Server process is running */
				CloseHandle (procInfo.hThread);
				WaitForSingleObject (procInfo.hProcess, INFINITE);
				CloseHandle (procInfo.hProcess);
			}
			
			/* Respond a line at a time. It is convenient to use
				C library line-oriented routines at this point. */

			if (_tfopen_s (&fp, pThArg->tempFileName, _T("r")) != 0) {	
				_tprintf (_T("Temp output file is: %s.\n"), pThArg->tempFileName);
				_tperror (_T("Failure to open command output file."));
				break;  /* Try the next command. */
			}

			/* Avoid an "information discovery" security exposure. */
			/* ZeroMemory(&response, sizeof(response));  */
			while (_fgetts (response.record, MAX_RQRS_LEN, fp) != NULL) {
				response.rsLen = (LONG32)(strlen(response.record) + 1);
				WriteFile (hNamedPipe, &response, response.rsLen + sizeof(response.rsLen), &nXfer, NULL);
			}
			/* Write a terminating record. Messages use 8-bit characters, not UNICODE */
			response.record[0] = '\0';
			response.rsLen = 0;
			WriteFile (hNamedPipe, &response, sizeof(response.rsLen), &nXfer, NULL); 

			FlushFileBuffers (hNamedPipe);
			fclose (fp);

		}   /* End of main command loop. Get next command */

		/* Client has disconnected or there has been a shut down requrest */
		/* Terminate this client connection and then wait for another */
		FlushFileBuffers (hNamedPipe);
		DisconnectNamedPipe (hNamedPipe);
	}

	/*  Force the connection thread to shut down if it is still active */
    if (hConTh != NULL) {
        GetExitCodeThread (hConTh, &conThStatus);
	    if (conThStatus == STILL_ACTIVE) {
		    hClient = CreateFile (SERVER_PIPE, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		    if (hClient != INVALID_HANDLE_VALUE) CloseHandle (hClient);
		    WaitForSingleObject (hConTh, INFINITE);
        }
	}


/*	Comments about terminating the connection thread:
	There is an issue with the fact that the Connect thread
	could still be running. You would like to terminate it, but then 
	there would not be a clean thread shut down, and, for example, 
	DLL entry points would not be called, resulting in potential resource leaks.
	Now, in this case, the connection thread is minimal, so there should be
	no harm, but, here we've used a different approach:

	The connection thread can be resumed by having the server
	act like a client and perform a CreateFile to get a (very) short lived 
	connection, allowing the connection thread to shut down.

    Unfortunately, there is a problem here; any Connect thread could respond,
	not the one on the server instance we're interested in. We've ignored that
	here as there's only on connect thread.

	Nonetheless, here are a couple of other solutions that are tempting
	to try, but they both have problems.
	
	IDEA 1. Close the named pipe handle. Perhaps, that will cause the ConnectNamedPipe
	call to fail and the Connect thread can terminate. Unfortunately, CloseHandle 
	blocks while there is an active ConnectNamedPipe call.
		CloseHandle (hNamedPipe);  // Don't do this! It could block.
	
	IDEA 2. Terminate the thread. The problem is that DLL entry points will
	not be called, resulting in potential resource leaks mentioned above.

	NOTE: DLL entry points were discussed in Chapter 5 but have not been illustrated
	yet. There is an example in Chapter 12.

	IDEA 3. Use thread cancellation using asynch proc calls (Chapter 10).
	Unfortunately, the client may not be in an alertable wait state.

*/
	_tprintf (_T("Thread %d shutting down.\n"), pThArg->threadNumber);
	/* End of command processing loop. Free resources and exit from the thread. */
	CloseHandle (hTmpFile); hTmpFile = INVALID_HANDLE_VALUE;
	if (!DeleteFile (pThArg->tempFileName)) {
		ReportError (_T("Failed deleting temp file."), 0, TRUE);
	}
	_tprintf (_T ("Exiting server thread number %d.\n"), pThArg->threadNumber);
	return 0;
}


static DWORD WINAPI Connect (LPTHREAD_ARG pThArg)
{
	BOOL fConnect;
	/*	Pipe connection thread that allows the server worker thread 
		to poll the shut down flag. */
	fConnect = ConnectNamedPipe (pThArg->hNamedPipe, NULL);
	_endthreadex (0);
	return 0;	/* Suppress a compiler warning message. */ 
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
		/* NOTE (Oct 12, 2001). The mailslot CreateFile uses GENERIC_WRITE
		   but not GENERIC_READ. GENERIC_READ will work on Windows NT/2000/XP,
		   but will fail ("BAD PARAMETER") on Windows 9x/Me. Now, this particular
		   program is not designed for 9x/Me since it is a named pipe server, but
		   there are many cases is which a mailslot client would run on 9x/Me, so
		   do not use GENERIC_READ when creating a mailslot client file */
		hMsFile = CreateFile (MS_CLTNAME, GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hMsFile == INVALID_HANDLE_VALUE) continue;

		/* Send out the message to the mailslot. */

		MsNotify.msStatus = 0;
		MsNotify.msUtilization = 0;
		_tcsncpy_s (MsNotify.msName, sizeof(MsNotify.msName)/sizeof(TCHAR), SERVER_PIPE, _TRUNCATE);
		if (!WriteFile (hMsFile, &MsNotify, MSM_SIZE, &nXfer, NULL))
			ReportError (_T ("Server MS Write error."), 13, TRUE);
		CloseHandle (hMsFile);
	}

	/* Cancel all outstanding NP I/O commands. See Chapter 14 for CancelIoEx */
	_tprintf (_T("Shut down flag set. Cancel all outstanding I/O operations.\n"));
	/* This is an NT6 dependency. On Windows XP, outstanding NP I/O operations hang. */
	for (iNp = 0; iNp < MAX_CLIENTS; iNp++) {
		CancelIoEx (threadArgs[iNp].hNamedPipe, NULL);
	}
	_tprintf (_T ("Shuting down monitor thread.\n"));

	_endthreadex (0);
	return 0;
}


BOOL WINAPI Handler (DWORD CtrlEvent)
{
	/* shut down the system */
	_tprintf (_T("In console control handler\n"));
	InterlockedIncrement(&shutDown);
	return TRUE;
}
