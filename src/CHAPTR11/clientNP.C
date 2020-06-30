/* Chapter 11 - Client/Server system. CLIENT VERSION.
	ClientNP - Connection-oriented client. Named Pipe version */
/* Execute a command line (on the server) and display the response. */

/*  The client creates a long-lived
	connection with the server (consuming a pipe instance) and prompts
	the user for the command to execute. */

/* This program illustrates:
	1. Named pipes from the client side. 
	2. Long-lived connections with a single request but multiple responses.
	3. Reading responses in server messages until and end of response is received.
*/

/* Special commands recognized by the server are:
	1. $Statistics: Give performance statistics.
	2. $ShutDownThread: Terminate a server thread.
	3. $Quit. Exit from the client. */

#include "Everything.h"
#include "ClientServer.h" /* Defines the resquest and response records */

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hNamedPipe = INVALID_HANDLE_VALUE;
	TCHAR quitMsg [] = _T ("$Quit");
	TCHAR serverPipeName [MAX_PATH+1];
	REQUEST request;		/* See ClientServer.h */
	RESPONSE response;		/* See ClientServer.h */
	DWORD nRead, nWrite, npMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

	LocateServer (serverPipeName, MAX_PATH);

	while (INVALID_HANDLE_VALUE == hNamedPipe) { /* Obtain a handle to a NP instance */
		if (!WaitNamedPipe (serverPipeName, NMPWAIT_WAIT_FOREVER))
			ReportError (_T("WaitNamedPipe error."), 2, TRUE);
		/* An instance has become available. Attempt to open it 
		 * Another thread could open it first, however or the server could close the instance */
		hNamedPipe = CreateFile (serverPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	/*  Read the NP handle in waiting, message mode. Note that the 2nd argument
	 *  is an address, not a value. Client and server may be on the same system, so
	 *  it is not appropriate to set the collection mode and timeout (last 2 args)
	 */
	if (!SetNamedPipeHandleState (hNamedPipe, &npMode, NULL, NULL))
		ReportError (_T("SetNamedPipeHandleState error."), 2, TRUE);
	/* Prompt the user for commands. Terminate on "$quit". */
	request.rqLen = RQ_SIZE;
	while (ConsolePrompt (_T ("\nEnter Command: "), request.record, MAX_RQRS_LEN-1, TRUE)
			&& (_tcscmp (request.record, quitMsg) != 0)) {

		if (!WriteFile (hNamedPipe, &request, RQ_SIZE, &nWrite, NULL))
			ReportError (_T ("Write NP failed"), 0, TRUE);

		/* Read each response and send it to std out */
		while (ReadFile (hNamedPipe, &response, RS_SIZE, &nRead, NULL))
		{
			if (response.rsLen <= 1) { /* 0 length indicates response end */ break; }
			_tprintf (_T("%s"), response.record);
		}
	}

	_tprintf (_T("Quit command received. Disconnect.\n"));

	CloseHandle (hNamedPipe);
	return 0;
}
