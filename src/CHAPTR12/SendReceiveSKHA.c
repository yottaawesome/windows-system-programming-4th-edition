/*	SendReceiveSKHA.c - Chapter 12. Multithreaded Streaming socket		*/
/*	This is a modification of SendReceiveSKST.c to illustrate a			*/
/*	different thread-safe library technique								*/
/*	State is preverved in a handle-like state structure rather than		*/
/*	using TLS. This allows a thread to use several sockets on once		*/
/*	Messages are delimited by null characters ('\0')					*/
/*	so message length is not known ahead of time. Therefore, incoming	*/
/*	data is buffered and must be preserved from one function call to	*/
/*	the next. Therefore, we need to use a structure associated with 	*/
/*	each socket, and the socket has its own privage "static storage"	*/
/*  WARNING: 8-bit characters only in messages. No 16-bit characters	*/
/*	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "Everything.h"
#include "ClientServer.h"	/* Defines the MESSAGESs. */

typedef struct SOCKET_HANDLE_T {
/* Current socket state */
/* This struture contains "staticBuffLen" characters of residual data */
/* There may or may not be end-of-string (null) characters */
	SOCKET sk;
	char staticBuff[MAX_RQRS_LEN];
	LONG32 staticBuffLen;
} SOCKET_HANDLE, * PSOCKET_HANDLE;

/*	Functions to create and close "streaming socket handles" */
__declspec (dllexport)
PVOID CreateCSSocketHandle (SOCKET s)
{
	PVOID p;
	PSOCKET_HANDLE ps;

	p = malloc (sizeof(SOCKET_HANDLE));
	if (p == NULL) return NULL;
	ps = (PSOCKET_HANDLE)p;
	ps->sk = s;
	ps->staticBuffLen = 0; /* Initialize buffer state */
	return p;
}

__declspec (dllexport)
BOOL CloseCSSocketHandle (PSOCKET_HANDLE psh)
{
	if (psh == NULL) return FALSE;
	free (psh);
	return TRUE;
}


__declspec(dllexport)
BOOL ReceiveCSMessage (MESSAGE *pMsg, PSOCKET_HANDLE psh)
/*  Use a PVOID so that the calling program does not need to include the */
/*	SOCKET_HANDLE definition. */
{
	/* TRUE return indicates an error or disconnect */
	BOOL disconnect = FALSE;
	LONG32 nRemainRecv = 0, nXfer, k; /* Must be signed integers */
	LPSTR pBuffer, message;
	CHAR tempBuff[MAX_RQRS_LEN+1];
	SOCKET sd;

	if (psh == NULL) return FALSE; 
	sd = psh->sk;

	/* This is all that's changed from SendReceiveSKST! */

	message = pMsg->record; /* Requests and responses have the same structure */
	/*	Read up to the null character, leaving residual data
	 *	in the static buffer */

	for (k = 0; k < psh->staticBuffLen && psh->staticBuff[k] != '\0'; k++) {
		message[k] = psh->staticBuff[k];
	}  /* k is the number of characters transferred */
	if (k < psh->staticBuffLen) { /* a null was found in static buf */
		message[k] = '\0'; 
		psh->staticBuffLen -= (k+1); /* Adjust the static buffer state */
		memcpy (psh->staticBuff, &(psh->staticBuff[k+1]), psh->staticBuffLen);
		return TRUE; /* No socket input required */
	} 
	
	/* the entire static buffer was transferred. No eol found */
	nRemainRecv = sizeof(tempBuff) - 1 - psh->staticBuffLen; 
	pBuffer = message + psh->staticBuffLen;
	psh->staticBuffLen = 0;

	while (nRemainRecv > 0 && !disconnect) {
		nXfer = recv (sd, tempBuff, nRemainRecv, 0);
		if (nXfer <= 0) {
			disconnect = TRUE;
			continue;
		}

		nRemainRecv -=nXfer;
		/* Transfer to target message up to null, if any */
		for (k = 0; k < nXfer && tempBuff[k] != '\0'; k++) {
			*pBuffer = tempBuff[k];
			pBuffer++;
		}
		if (k >= nXfer) { /* null not found, read more */
			nRemainRecv -= nXfer;
		} else { /* null has been found */
			*pBuffer = '\0';
			nRemainRecv = 0;
			memcpy (psh->staticBuff, &tempBuff[k+1], nXfer - k - 1);
			psh->staticBuffLen = nXfer -k - 1;
		}
	}
	return !disconnect;	
}


__declspec(dllexport)
BOOL SendCSMessage (MESSAGE *pMsg, PSOCKET_HANDLE psh)
{
	/* Send the the request to the server on socket sd */
	BOOL disconnect = FALSE;
	LONG32 nRemainSend, nXfer;
	LPSTR pBuffer;
	SOCKET sd;

	if (psh == NULL || pMsg == NULL) return FALSE;
	sd = psh->sk;

	pBuffer = pMsg->record;
	nRemainSend = (LONG32)(min(strlen (pBuffer) + 1, MAX_MESSAGE_LEN));

	while (nRemainSend > 0 && !disconnect)  {
		/* send does not guarantee that the entire message is sent */
		nXfer = send (sd, pBuffer, nRemainSend, 0);
		if (nXfer <= 0) {
			disconnect = TRUE;
		}
		nRemainSend -=nXfer; pBuffer += nXfer;
	}

	return !disconnect;
}
