/* Chapter 12. clientSKST.c										*/
/* Single threaded command line client	WINDOWS SOCKETS VERSION	*/
/* THIS DIFFERS FROM clientSK.c ONLY IN THAT THE MESSAGE SEND	*/
/* and receive functions are now in a DLL, and the functions	*/
/* are renamed.													*/
/* Reads a sequence of commands to sent to a server process 	*/
/* over a socket connection. Wait for the response and display	*/
/* it.								*/

/* This program illustrates:
	1. Windows sockets from the client side.
	2. Short-lived connections with a single request but multiple responses.
	3. Reading responses in server messages until the server disconnects. */

#include "Everything.h"
#include "ClientServer.h"	/* Defines the request and response records. */

static BOOL ReceiveResponseMessage (RESPONSE *, SOCKET);

__declspec (dllimport) BOOL SendCSMessage (REQUEST *, SOCKET);
__declspec (dllimport) BOOL ReceiveCSMessage (RESPONSE *, SOCKET);

struct sockaddr_in clientSAddr;		/* Clients's Socket address structure */

int _tmain (int argc, LPTSTR argv [])
{
	SOCKET clientSock = INVALID_SOCKET;
	REQUEST request;	/* See clntcrvr.h */
	RESPONSE response;	/* See clntcrvr.h */
	WSADATA WSStartData;				/* Socket library data structure   */

	BOOL quit = FALSE;
	DWORD conVal, j;
	TCHAR promptMsg [] = _T("\nEnter Command> "), Req[MAX_RQRS_LEN];
	TCHAR quitMsg [] = _T("$quit"); /* request to shut down client */
	TCHAR shutMsg [] = _T("$ShutDownServer"); /* Stop all threads */
	CHAR defaultIPAddr[] = "127.0.0.1";

	/*	Initialize the WS library. Ver 2.0 */
	if (WSAStartup (MAKEWORD (2, 0), &WSStartData) != 0)
		ReportError (_T("Cannot support sockets"), 1, TRUE);
		
	/* Connect to the server */
	/* Follow the standard client socket/connect sequence */
	clientSock = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSock == INVALID_SOCKET)
		ReportError (_T("Failed client socket() call"), 2, TRUE);

	memset (&clientSAddr, 0, sizeof(clientSAddr));    
	clientSAddr.sin_family = AF_INET;	
	if (argc >= 2) 
		clientSAddr.sin_addr.s_addr = inet_addr (argv[1]); 
	else
		clientSAddr.sin_addr.s_addr = inet_addr (defaultIPAddr);

	clientSAddr.sin_port = htons(SERVER_PORT);

	conVal = connect (clientSock, (struct sockaddr *)&clientSAddr, sizeof(clientSAddr));
	if (conVal == SOCKET_ERROR) ReportError (_T("Failed client connect() call"), 3, TRUE);

	/*  Main loop to prompt user, send request, receive response */
	while (!quit) {
		_tprintf (_T("%s"), promptMsg); 
		/* Generic input, but command to server must be ASCII */
		_fgetts (Req, MAX_RQRS_LEN-1, stdin);
		for (j = 0; j <= _tcslen(Req); j++) request.record[j] = Req[j];
		/* Get rid of the new line at the end */
		request.record[strlen(request.record)-1] = '\0';
		if (strcmp (request.record, quitMsg) == 0 ||
			strcmp (request.record, shutMsg) == 0) quit = TRUE;
		SendCSMessage (&request, clientSock);
		ReceiveResponseMessage (&response, clientSock);
	}

	shutdown (clientSock, 2); /* Disallow sends and receives */
	closesocket (clientSock);
	WSACleanup();
	_tprintf (_T("\n****Leaving client\n"));
	return 0;
}

BOOL ReceiveResponseMessage (RESPONSE *pResponse, SOCKET sd)
{	
	BOOL disconnect = FALSE, lastRecord = FALSE;

	/*  Read the response records - there may be more than one.
		As each is received, write it to std out. */
	/*  Messages use 8-bit characters */
		
	while (!lastRecord) {
		disconnect = ReceiveCSMessage (pResponse, sd);
		/* Detect an end message message - 8-bit characters */
		/* Arbitrarily defined as "$$$$$$$" */
		lastRecord = (strcmp (pResponse->record, "$$$$$$$") == 0);
		if (!disconnect && !lastRecord) 
			printf ("%s", pResponse->record);
	}
	return disconnect;	
}
