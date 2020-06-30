/*	SendReceiveSKST.c - Chapter 12. Multithreaded Streaming socket	*/
/*	DLL. Messages are delimited by NULL characters ('\0')		*/
/*	so the message length is not know ahead of time. Therefore, incoming	*/
/*	data is buffered and must be preserved from one function call to	*/
/*	the next. Therefore, we need to use Thread Local Storage (TLS)		*/
/*	so that each thread has its own private "static storage"			*/
/*  WARNING: 8-bit characters only in messages. No 16-bit characters	*/
/*	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "Everything.h"
#include "ClientServer.h"	/* Defines MESSAGE records. */

typedef struct STATIC_BUF_T {
/* "staticBuf" contains "staticBufLen" characters of residual data */
/* There may or may not be end-of-string (null) characters */
	char staticBuf[MAX_RQRS_LEN];
	LONG32 staticBufLen;
} STATIC_BUF;

static DWORD tlsIndex = TLS_OUT_OF_INDEXES; /* TLS index - Initialize to illegal value */
/*	A single threaded library would use the following:
static char staticBuf[MAX_RQRS_LEN];
static LONG32 staticBufLen;
*/

/* number of attached, detached threads and processes */
static volatile long nPA = 0, nTA = 0, nPD = 0, nTD = 0;

/* DLL main function. */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	STATIC_BUF * pBuf; 

	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			tlsIndex = TlsAlloc();
			InterlockedIncrement (&nPA); /* Overkill, as DllMain calls are serialized */
			/*	There is no thread attach call for the primary thread or other threads created
				BEFORE this DLL was loaded.
				Carry out the thread attach operations during process attach 
				You must load this DLL before creating any threads that will use the DLL. */

		case DLL_THREAD_ATTACH:
			/* Indicate that memory has not been allocated  */
			InterlockedIncrement (&nTA); /* Preexisting threads are not counted */
			/* TlsSetValue (tlsIndex, NULL); -- not necessary; slots are initialized to 0 */
			return TRUE; /* This value is ignored */

		case DLL_PROCESS_DETACH:
		/* Free all remaining resources that this DLL uses. Some thread
			DLLs may not have been called. */
			InterlockedIncrement (&nPD);
			/* Count this as detaching the primary thread as well */

			InterlockedIncrement (&nTD);
			pBuf = TlsGetValue (tlsIndex);
			if (pBuf != NULL) {
				free (pBuf);
				pBuf = NULL;
			}

			_tprintf (_T("Process detach. Counts: %d %d %d %d\n"), nPA, nPD, nTA, nTD);
			TlsFree(tlsIndex);
			return TRUE;

		case DLL_THREAD_DETACH:
			/* May not be called for every thread using the DLL */
			InterlockedIncrement (&nTD);
			pBuf = TlsGetValue (tlsIndex);
			if (pBuf != NULL) {
				free (pBuf);
				pBuf = NULL;
			}
			return TRUE;

		default: return TRUE;
	}
}


__declspec(dllexport)
BOOL ReceiveCSMessage (MESSAGE *pMsg, SOCKET sd)
{
	/* FALSE return indicates an error or disconnect */
	BOOL disconnect = FALSE;
	LONG32 nRemainRecv, nXfer, k; /* Must be signed integers */
	LPBYTE pBuffer, message;
	CHAR tempBuff[MAX_MESSAGE_LEN+1];
	STATIC_BUF *pBuff;

	if (pMsg == NULL) return FALSE;
	pBuff = (STATIC_BUF *) TlsGetValue (tlsIndex);
	if (pBuff == NULL) { /* First time initialization. */
		/* Only threads that need this storage will allocate it */
		pBuff = malloc (sizeof (STATIC_BUF));
		if (pBuff == NULL) return FALSE; /* Error */
		TlsSetValue (tlsIndex, pBuff); 
		pBuff->staticBufLen = 0; /* Intialize state */
	}

	message = pMsg->record; /* Requests and responses have the same structure */
	/*	Read up to the null character, leaving residual data
	 *	in the static buffer */

	for (k = 0; k < pBuff->staticBufLen && pBuff->staticBuf[k] != '\0'; k++) {
		message[k] = pBuff->staticBuf[k];
	}  /* k is the number of characters transferred */
	if (k < pBuff->staticBufLen) { /* a null was found in static buf */
		message[k] = '\0'; 
		pBuff->staticBufLen -= (k+1); /* Adjust the static buffer state */
		memcpy (pBuff->staticBuf, &(pBuff->staticBuf[k+1]), pBuff->staticBufLen);
		return TRUE; /* No socket input required */
	} 
	
	/* the entire static buffer was transferred. No null found */
	nRemainRecv = sizeof(tempBuff) - sizeof(CHAR) - pBuff->staticBufLen; 
	pBuffer = message + pBuff->staticBufLen;
	pBuff->staticBufLen = 0;

	while (nRemainRecv > 0 && !disconnect) {
		nXfer = recv (sd, tempBuff, nRemainRecv, 0);
		if (nXfer <= 0) {
			disconnect = TRUE;
			continue;
		}

		/* Transfer to target message up to null, if any */
		for (k = 0; k < nXfer && tempBuff[k] != '\0'; k++) {
			*pBuffer = tempBuff[k];
			nRemainRecv -= nXfer; pBuffer++;
		}
		if (k < nXfer) { /* null has been found */
			*pBuffer = '\0';
			nRemainRecv = 0;
			/* Adjust the static buffer state for the next 
			 * ReceiveCSMessage call */
			memcpy (pBuff->staticBuf, &tempBuff[k+1], nXfer - k - 1);
			pBuff->staticBufLen = nXfer -k - 1;
		}
	}
	return !disconnect;	
}


__declspec(dllexport)
BOOL SendCSMessage (MESSAGE *pMsg, SOCKET sd)
{
	/* Send the the request to the server on socket sd */
	BOOL disconnect = FALSE;
	LONG32 nRemainSend, nXfer;
	LPBYTE pBuffer;

	if (pMsg == NULL) return FALSE;
	pBuffer = pMsg->record;
	if (pBuffer == NULL) return FALSE;
	nRemainSend = min(strlen (pBuffer) + 1, MAX_MESSAGE_LEN);

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
