/* Chapter 10. QueueObjCancel.c	- Prepare to be cancelled with alertable waits */
/* Queue functions. Signal model												*/

#include "Everything.h"
#include "SynchObj.h"

/* Finite bounded queue management functions */
static BOOL shutDownGet = FALSE;
static BOOL ShutDownPut = FALSE;

DWORD QueueGet (QUEUE_OBJECT *q, PVOID msg, DWORD mSize, DWORD maxWait)
{
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */
	WaitForSingleObject (q->qGuard, INFINITE);

	while (!shutDownGet && QueueEmpty (q)) {
		if (SignalObjectAndWait (q->qGuard, q->qNe, INFINITE, TRUE) == WAIT_IO_COMPLETION
			&& shutDownGet) {
			continue;
		}
		WaitForSingleObject (q->qGuard, INFINITE);
	}
	/* remove the message from the queue */
	if (!shutDownGet) {
		QueueRemove (q, msg, mSize);
		/* Signal that the queue is not full as we've removed a message */
		SetEvent (q->qNf);
		ReleaseMutex (q->qGuard);
	}
	return shutDownGet ? WAIT_TIMEOUT : 0;
}

DWORD QueuePut (QUEUE_OBJECT *q, PVOID msg, DWORD mSize, DWORD maxWait)
{
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */
	WaitForSingleObject (q->qGuard, INFINITE);

	while (!ShutDownPut && QueueFull (q)) {
		if (SignalObjectAndWait(q->qGuard, q->qNf, INFINITE, TRUE) == WAIT_IO_COMPLETION 
			&& ShutDownPut) {
			continue;
		}
		WaitForSingleObject (q->qGuard, INFINITE);
	}
	/* Put the message in the queue */
	if (!ShutDownPut) {
		QueueInsert (q, msg, mSize);	
		/* Signal that the queue is not empty as we've inserted a message */
		SetEvent (q->qNe);
		ReleaseMutex (q->qGuard);
	}
	return ShutDownPut ? WAIT_TIMEOUT : 0;
}

DWORD QueueInitialize (QUEUE_OBJECT *q, DWORD mSize, DWORD nMsgs)
{
	/* Initialize queue, including its mutex and events */
	/* Allocate storage for all messages. */
	
	if ((q->msgArray = calloc (nMsgs, mSize)) == NULL) return 1;
	q->qFirst = q->qLast = 0;
	q->qSize = nMsgs;

	q->qGuard = CreateMutex (NULL, FALSE, NULL);
	/* Auto reset events; this is the signal model */
	q->qNe = CreateEvent (NULL, FALSE, FALSE, NULL);
	q->qNf = CreateEvent (NULL, FALSE, FALSE, NULL);
	return 0; /* No error */
}

DWORD QueueDestroy (QUEUE_OBJECT *q)
{
	/* Free all the resources created by QueueInitialize */
	WaitForSingleObject (q->qGuard, INFINITE);
	free (q->msgArray);
	q->msgArray = NULL;
	CloseHandle (q->qNe);
	CloseHandle (q->qNf);
	ReleaseMutex (q->qGuard);
	CloseHandle (q->qGuard);

	return 0;
}


DWORD QueueEmpty (QUEUE_OBJECT *q)
{
	return (q->qFirst == q->qLast);
}

DWORD QueueFull (QUEUE_OBJECT *q)
{
	return ((q->qFirst - q->qLast) == 1 ||
		    (q->qLast == q->qSize-1 && q->qFirst == 0));
}

DWORD QueueRemove (QUEUE_OBJECT *q, PVOID msg, DWORD mSize)
{
	char *pm;

	if (QueueEmpty(q)) return 1; /* Error - Q is empty */
	pm = q->msgArray;
	/* Remove oldest ("first") message */
	memcpy (msg, pm + (q->qFirst * mSize), mSize);
	q->qFirst = ((q->qFirst + 1) % q->qSize);
	return 0; /* no error */
}

DWORD QueueInsert (QUEUE_OBJECT *q, PVOID msg, DWORD mSize)
{
	char *pm;

	if (QueueFull(q)) return 1; /* Error - Q is full */
	pm = q->msgArray;
	/* Add a new youngest ("last") message */
	memcpy (pm + (q->qLast * mSize), msg, mSize);
	q->qLast = ((q->qLast + 1) % q->qSize);
	return 0;
}

void CALLBACK QueueShutDown (ULONG_PTR pn)
{
	DWORD n = (DWORD)pn;   /* This cast is safe as the value is small */
	_tprintf (_T("In ShutDownQueue callback. %d\n"), n);
	if (n%2 != 0) shutDownGet = TRUE;
	if ( (n/2) % 2 != 0) ShutDownPut = TRUE;
	/* Free any resource (none in this example). */
	return;
}
