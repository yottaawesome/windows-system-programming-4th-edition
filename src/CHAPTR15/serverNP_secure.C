/*  Chapter 15. ServerNP_secure - Developed from serverNP.c in Chapter 11
 *	Multi-threaded command line server. Named pipe version
 *	Usage:	Server [UserName GroupName]
 *	ONE THREAD AND PIPE INSTANCE FOR EVERY CLIENT. */

#include "Everything.h"
#include "ClientServer.h" /* request and response messages defined here */

typedef struct {				/* Argument to a server thread. */
	HANDLE hNamedPipe;			/* Named pipe instance. */
	DWORD ThreadNo;
	TCHAR TmpFileName [MAX_PATH]; /* Temporary file name. */
} THREAD_ARG;
typedef THREAD_ARG *LPTHREAD_ARG;

volatile static BOOL ShutDown = FALSE;
static DWORD WINAPI Server (LPTHREAD_ARG);
static DWORD WINAPI Connect (LPTHREAD_ARG);
static DWORD WINAPI ServerBroadcast (LPLONG);
static BOOL  WINAPI Handler (DWORD);
static TCHAR ShutRqst [] = _T ("$ShutDownServer");

_tmain (int argc, LPTSTR argv [])
{
	/* MAX_CLIENTS is defined in ClientServer.h. */
	/* Currently limited to MAXIMUM_WAIT_OBJECTS WaitForMultipleObjects */
	/* is used by the main thread to wait for the server threads */

	HANDLE hNp, hMonitor, hSrvrThread [MAX_CLIENTS], hSecHeap = NULL;
	DWORD iNp, MonitorId, ThreadId;
	DWORD AceMasks [] =	/* Named pipe access rights - Only clients
		* run by the owner will be able to connect. */
		{FILE_GENERIC_READ | FILE_GENERIC_WRITE, 0, 0 };
	LPSECURITY_ATTRIBUTES pNPSA = NULL;
	THREAD_ARG ThArgs [MAX_CLIENTS];

	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater"), 1, FALSE);

	/* Console control handler to permit server shutdown */
	if (!SetConsoleCtrlHandler (Handler, TRUE))
		ReportError (_T("Cannot create Ctrl handler"), 1, TRUE);

	
	if (argc == 4)		/* Optional pipe security. */
		pNPSA = InitializeAccessOnlySA (0440, argv [1], argv [2], AceMasks, &hSecHeap);
			
	/* Create a thread broadcast pipe name periodically. */
	hMonitor = (HANDLE) _beginthreadex (NULL, 0, ServerBroadcast, NULL, 0, &MonitorId);

	/*	Create a pipe instance for every server thread.
	 *	Create a temp file name for each thread.
	 *	Create a thread to service that pipe. */

	for (iNp = 0; iNp < MAX_CLIENTS; iNp++) {
		hNp = CreateNamedPipe ( SERVER_PIPE, PIPE_ACCESS_DUPLEX,
				PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE | PIPE_WAIT,
				MAX_CLIENTS, 0, 0, INFINITE, pNPSA);

		if (hNp == INVALID_HANDLE_VALUE)
			ReportError (_T ("Failure to open named pipe."), 1, TRUE);
		ThArgs [iNp].hNamedPipe = hNp;
		ThArgs [iNp].ThreadNo = iNp;
		GetTempFileName (_T ("."), _T ("CLP"), 0, ThArgs [iNp].TmpFileName);
		hSrvrThread [iNp] = (HANDLE)_beginthreadex (NULL, 0, Server,
				&ThArgs [iNp], 0, &ThreadId);
		if (hSrvrThread [iNp] == NULL)
			ReportError (_T ("Failure to create server thread."), 2, TRUE);
	}
	
	/* Wait for all the threads to terminate. */

	WaitForMultipleObjects (MAX_CLIENTS, hSrvrThread, TRUE, INFINITE);
	_tprintf (_T("All Server worker threads have shut down.\n"));

	WaitForSingleObject (hMonitor, INFINITE);
	_tprintf (_T("Monitor thread has shut down.\n"));

	CloseHandle (hMonitor);
	for (iNp = 0; iNp < MAX_CLIENTS; iNp++) { 
		/* Close pipe handles and delete temp files */
		/* Closing temp files is redundant, as the worker threads do it */
		CloseHandle (hSrvrThread [iNp]);
		DeleteFile (ThArgs [iNp].TmpFileName);
	}
	if (hSecHeap != NULL) HeapDestroy (hSecHeap);
	_tprintf (_T("Server process will exit.\n"));
	/*	ExitProcess assures a clean exit. All DLL entry points will be 
		called, indicating process detach. Among other reasons that this is
		important is that connection threads may still be blocked on
		ConnectNamedPipe calls. */
	ExitProcess (0);

	return 0;
}


static DWORD WINAPI Server (LPTHREAD_ARG pThArg)

/* Server thread function. There is a thread for every potential client. */
{
	/* Each thread keeps its own request, response,
		and bookkeeping data structures on the stack. 
		Also, each thread creates an additional "connect thread"
		so that the main worker thread can test the shutdown flag
		periodically while waiting for a client connection. */

	HANDLE hNamedPipe, hTmpFile = INVALID_HANDLE_VALUE, hConTh = NULL, hClient;
	DWORD nXfer, ConThId, ConThStatus;
	STARTUPINFO StartInfoCh;
	SECURITY_ATTRIBUTES TempSA = {sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
	PROCESS_INFORMATION ProcInfo;
	FILE *fp;
	REQUEST request;
	RESPONSE response;

	GetStartupInfo (&StartInfoCh);
	hNamedPipe = pThArg->hNamedPipe;

	while (!ShutDown) { 	/* Connection loop */

		/* Create a connection thread, and wait for it to terminate */
		/* Use a timeout on the wait so that the shutdown flag can be tested */
		hConTh = (HANDLE)_beginthreadex (NULL, 0, Connect, pThArg, 0, &ConThId);
		if (hConTh == NULL) {
			ReportError (_T("Cannot create connect thread"), 0, TRUE);
			_endthreadex(2);
		}

		/* Wait for a client connection. */
		
		while (!ShutDown && WaitForSingleObject (hConTh, CS_TIMEOUT) == WAIT_TIMEOUT) 
			{ /* Empty loop body */};
		if (ShutDown) _tprintf(_T("Thread %d received shutdown\n"), pThArg->ThreadNo);
		if (ShutDown) continue;	/* Flag could also be set by a different thread */

		CloseHandle (hConTh); hConTh = NULL;
		/* A connection now exists */
		/* Open the temporary results file for this connection. */
		hTmpFile = CreateFile (pThArg->TmpFileName, GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, &TempSA,
				CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
		if (hTmpFile == INVALID_HANDLE_VALUE) { /* About all we can do is stop the thread */
			ReportError (_T("Cannot create temp file"), 0, TRUE);
			_endthreadex(1);
		}

		while (!ShutDown && ReadFile (hNamedPipe, &request, RQ_SIZE, &nXfer, NULL)) {
			/* Receive new commands until the client disconnects */

			ShutDown = ShutDown || (_tcscmp (request.record, ShutRqst) == 0);
			if (ShutDown)  continue;

			/* Main command loop */
			/* Create a process to carry out the command. */
			StartInfoCh.hStdOutput = hTmpFile;
			StartInfoCh.hStdError = hTmpFile;
			StartInfoCh.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
			StartInfoCh.dwFlags = STARTF_USESTDHANDLES;

			if (!CreateProcess (NULL, request.record, NULL,
				NULL, TRUE, /* Inherit handles. */
				0, NULL, NULL, &StartInfoCh, &ProcInfo)) {
					PrintMsg (hTmpFile, _T("ERR: Cannot create process."));
					ProcInfo.hProcess = NULL;
			}

			if (ProcInfo.hProcess != NULL ) { /* Server process is running */
				CloseHandle (ProcInfo.hThread);
				WaitForSingleObject (ProcInfo.hProcess, INFINITE);
				CloseHandle (ProcInfo.hProcess);
			}
			
			/* Respond a line at a time. It is convenient to use
				C library line-oriented routines at this point. */

			fp = _tfopen (pThArg->TmpFileName, _T ("r"));
			if (fp == NULL)
				perror ("Failure to open command output file.");
			while (_fgetts (response.record, MAX_RQRS_LEN, fp) != NULL) 
				WriteFile (hNamedPipe, &response, RS_SIZE, &nXfer, NULL); 
			fclose (fp);

			/* Erase temp file contents */
			SetFilePointer (hTmpFile, 0, NULL, FILE_BEGIN);
			SetEndOfFile (hTmpFile);

			/* Send an end of response indicator. */
			strcpy (response.record, "");
			WriteFile (hNamedPipe, &response, RS_SIZE, &nXfer, NULL);
		}   /* End of main command loop. Get next command */

		/* Client has disconnected or there has been a shutdown requrest */
		/* Terminate this client connection and then wait for another */
		FlushFileBuffers (hNamedPipe);
		DisconnectNamedPipe (hNamedPipe);
		CloseHandle (hTmpFile); hTmpFile = INVALID_HANDLE_VALUE;
		DeleteFile (pThArg->TmpFileName);
	}

	/*  Force the connection thread to shut down if it is still active 
	 *  But, see the comments below. */
	GetExitCodeThread (hConTh, &ConThStatus);
	if (ConThStatus == STILL_ACTIVE) {
		hClient = CreateFile (SERVER_PIPE, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hClient != INVALID_HANDLE_VALUE) CloseHandle (hClient);
		WaitForSingleObject (hConTh, INFINITE);
	}


/*	There is an issue, solved above, with the fact that the Connect thread
	could still be
	running. You would like to terminate it, but then there would not be
	a clean thread shutdown, and, for example, DLL entry points would not
	called, resulting in potential resource leaks. Fortunately, the 
	connection thread can be resumed by having the server perform a 
	CreateFile to get a (very) short lived connection, allowing the connection
	thread to shut down.

	Nonetheless, here are a couple of other solutions that are tempting
	to try, but they both have problems.
	
	IDEA 1. Close the named pipe handle. Perhaps, that will cause the ConnectNamedPipe
	call to fail and the Connect thread can terminate. Unfortunately, CloseHandle 
	blocks while there is an active ConnectNamedPipe call.
		CloseHandle (hNamedPipe);  // Don't do this! It could block.
	
	IDEA 2. Terminate the thread. The problem is that DLL entry points will
	not be called, resulting in potential resource leaks.

	NOTE: DLL entry points were discussed in Chapter 5 but have not been illustrated
	yet. There is an example in Chapter 12.

	IDEA 3. Use thread cancellation using asynch proc calls (chapter 10).
	Unfortunately, the client may be in WaitNamedPipe, but not in an alertable
	wait state.

	ANOTHER NOTE: Pthraeds, provided by most UNIX systems, allows one thread to cancel
	another in such a way that the cancelled thread can execute a "cancellation
	handler" before termination. Windows would benefit from a thread cancellation
	capability.

*/
	_tprintf (_T("Thread %d shutting down.\n"), pThArg->ThreadNo);
	/* End of command processing loop. Free resources and exit from the thread. */
	if (hTmpFile != INVALID_HANDLE_VALUE) CloseHandle (hTmpFile);
	DeleteFile (pThArg->TmpFileName);
	_tprintf (_T("Exiting server thread number %d\n"), pThArg->ThreadNo);
	_endthreadex (0);
	return 0;	/* Suppress a compiler warning message. */
}


static DWORD WINAPI Connect (LPTHREAD_ARG pThArg)
{
	BOOL f;
	/*	Pipe connection thread that allows the server worker thread 
		to poll the ShutDown flag. */
	f = ConnectNamedPipe (pThArg->hNamedPipe, NULL);
	_tprintf (_T("ConnNP finished: %d\n"), f);
	_endthreadex (0);
	return 0; 
}


static DWORD WINAPI ServerBroadcast (LPLONG pNull)
{
	MS_MESSAGE MsNotify;
	DWORD nXfer;
	HANDLE hMsFile;

	/* Open the mailslot for the MS "client" writer. */
	while (!ShutDown) { /* Run as long as there are server threads */
		/* Wait for another client to open a mailslot. */
		Sleep (CS_TIMEOUT);
		/* NOTE (Oct 12, 2001). The mailslot CreateFile uses GENERIC_WRITE
		   but not GENERIC_READ. GENERIC_READ will work on Windows NT/2000/XP,
		   but will fail ("BAD PARAMETER") on Windows 9x/Me. Now, this particular
		   program is not designed for 9x/Me since it is a named pipe server, but
		   there are many cases is which a mailslot client would run on 9x/Me, so
		   do not use GENERIC_READ when creating a mailslot client file */
		hMsFile = CreateFile (MS_CLTNAME, GENERIC_WRITE,
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
	_tprintf (_T ("Shuting down monitor thread.\n"));

	_endthreadex (0);
	return 0;
}


BOOL WINAPI Handler (DWORD CtrlEvent)
{
	/* Shutdown the system */
	_tprintf (_T("In console control handler\n"));
	ShutDown = TRUE;
	return TRUE;
}
