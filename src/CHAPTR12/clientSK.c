/* Chapter 12. clientSK.c										*/
/* Single threaded command line client	WINDOWS SOCKETS VERSION	*/
/* Reads a sequence of commands to sent to a server process 	*/
/* over a socket connection. Wait for the response and display	*/
/* it.								*/

/* This program illustrates:
	1. Windows sockets from the client side.
	2. Short-lived connections with a single request but multiple responses.
	3. Reading responses in server messages until the server disconnects. */

#include "Everything.h"
#include "ClientServer.h"	/* Defines the request and response records. */

static BOOL SendRequestMessage (REQUEST *, SOCKET);
static BOOL ReceiveResponseMessage (RESPONSE *, SOCKET);

struct sockaddr_in clientSAddr;		/* Client Socket address structure */

int _tmain (int argc, LPSTR argv[])
{
	SOCKET clientSock = INVALID_SOCKET;
	REQUEST request;	/* See clntcrvr.h */
	RESPONSE response;	/* See clntcrvr.h */
#ifdef WIN32					/* Exercise: Can you port this code to UNIX?
								 * Are there other sysetm dependencies? */
	WSADATA WSStartData;				/* Socket library data structure   */
#endif
	BOOL quit = FALSE;
	DWORD conVal;
	/*	Initialize the WS library. Ver 2.0 */
#ifdef WIN32
	if (WSAStartup (MAKEWORD (2, 0), &WSStartData) != 0)
		ReportError (_T("Cannot support sockets"), 1, TRUE);
#endif		
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
		clientSAddr.sin_addr.s_addr = inet_addr ("127.0.0.1");

	clientSAddr.sin_port = htons(SERVER_PORT);

	conVal = connect (clientSock, (struct sockaddr *)&clientSAddr, sizeof(clientSAddr));
	if (conVal == SOCKET_ERROR) ReportError (_T("Failed client connect() call)"), 3, TRUE);

	/*  Main loop to prompt user, send request, receive response */
	while (!quit) {
		_tprintf (_T("%s"), _T("\nEnter Command: ")); 
		_fgetts (request.record, MAX_RQRS_LEN-1, stdin); 
		/* Get rid of the new line at the end */
		/* Messages use 8-bit characters */
		request.record[strlen(request.record)-1] = '\0';
		if (strcmp (request.record, "$Quit") == 0) quit = TRUE;
		SendRequestMessage (&request, clientSock);
		if (!quit) ReceiveResponseMessage (&response, clientSock);
	}

	shutdown (clientSock, SD_BOTH); /* Disallow sends and receives */
#ifdef WIN32
	closesocket (clientSock);
	WSACleanup();
#else
	close (clientSock);
#endif
	_tprintf (_T("\n****Leaving client\n"));
	return 0;
}


BOOL SendRequestMessage (REQUEST *pRequest, SOCKET sd)
{
	/* Send the the request to the server on socket sd */
	BOOL disconnect = FALSE;
	LONG32 nRemainSend, nXfer;
	LPBYTE pBuffer;

	/* The request is sent in two parts. First the header, then
	   the command string proper. */
	
	nRemainSend = RQ_HEADER_LEN; 
	pRequest->rqLen = (DWORD)(strlen (pRequest->record) + 1);
	pBuffer = (LPBYTE)pRequest;
	while (nRemainSend > 0 && !disconnect)  {
		/* send does not guarantee that the entire message is sent */
		nXfer = send (sd, pBuffer, nRemainSend, 0);
		if (nXfer == SOCKET_ERROR) ReportError (_T("client send() failed"), 4, TRUE);
		disconnect = (nXfer == 0);
		nRemainSend -=nXfer; pBuffer += nXfer;
	}

	nRemainSend = pRequest->rqLen; 
	pBuffer = (LPBYTE)pRequest->record;
	while (nRemainSend > 0 && !disconnect)  {
		nXfer = send (sd, pBuffer, nRemainSend, 0);
		if (nXfer == SOCKET_ERROR) ReportError (_T("client send() failed"), 5, TRUE);
		disconnect = (nXfer == 0);
		nRemainSend -=nXfer; pBuffer += nXfer;
	}
	return disconnect;
}


BOOL ReceiveResponseMessage (RESPONSE *pResponse, SOCKET sd)
{
	BOOL disconnect = FALSE, LastRecord = FALSE;
	LONG32 nRemainRecv, nXfer;
	LPBYTE pBuffer;

	/*  Read the response records - there may be more than one.
		As each is received, write it to std out. */

	/*	Read each response and send it to std out. 
		First, read the record header, and then
		read the rest of the record.  */
		
	while (!LastRecord) {
		/*  Read the header */
		nRemainRecv = RS_HEADER_LEN; pBuffer = (LPBYTE)pResponse;
		while (nRemainRecv > 0 && !disconnect) {
			nXfer = recv (sd, pBuffer, nRemainRecv, 0);
			if (nXfer == SOCKET_ERROR) ReportError (_T("client header recv() failed"), 6, TRUE);
			disconnect = (nXfer == 0);
			nRemainRecv -=nXfer; pBuffer += nXfer;
		}
		/*	Read the response record */
		nRemainRecv = pResponse->rsLen;
		/* Exclude buffer overflow */
		nRemainRecv = min(nRemainRecv, MAX_RQRS_LEN);
		LastRecord = (nRemainRecv <= 1);  /* The terminating null is counted */
		pBuffer = (LPBYTE)pResponse->record;
		while (nRemainRecv > 0 && !disconnect) {
			nXfer = recv (sd, pBuffer, nRemainRecv, 0);
			if (nXfer == SOCKET_ERROR) ReportError (_T("client response recv() failed"), 7, TRUE);
			disconnect = (nXfer == 0);
			nRemainRecv -=nXfer; pBuffer += nXfer;
		}
	
		if (!disconnect) 
			_tprintf (_T("%s"), pResponse->record);
	}
	return disconnect;	
}
