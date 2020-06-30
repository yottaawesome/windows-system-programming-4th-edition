/* Chapter 10. QueueObjSig.c
/* Aug 14, 2005. There are several changes to the Wait logic		*/
/*   and queue insertion and removal logic in the QueueGet             */
/*   and QueuePut functions                                            */
/* Queue functions													*/
/* Use the "Signal model" (SetEvet/Auto-reset combination)			*/
/*	Exactly one waiting thread is released every time the queue		*/
/*	becomes non-empty or non-full. This works as long as the Q 		*/
/*	functions (QueueGet and QueuePut) only change the queue by one message*/
/*	at a time. If we released and/or obtained multiple messages		*/
/*	per get or put, then the broadcast model						*/
/*	(PulseEvent/Manual-reset) is necessary so that all waiting		*/
/*	threads will be released and can test the Queue state			*/

#include "Everything.h"
#include "SynchObj.h"

/* Finite bounded queue management functions */

DWORD QueueGet (QUEUE_OBJECT *q, PVOID msg, DWORD msize, DWORD MaxWait)
{
	DWORD TotalWaitTime = 0;
	BOOL TimedOut = FALSE;

	WaitForSingleObject (q->qGuard, INFINITE);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */

	while (QueueEmpty (q) && !TimedOut) {
		ReleaseMutex (q->qGuard);
		TimedOut = (WaitForSingleObject (q->qNe, CV_TIMEOUT) == WAIT_TIMEOUT);
		WaitForSingleObject (q->qGuard, INFINITE);
	}
	/* remove the message from the queue */
	if (!TimedOut  || !QueueEmpty (q)) QueueRemove (q, msg, msize);
	/* Signal that the queue is not full as we've removed a message */
	SetEvent (q->qNf);
	ReleaseMutex (q->qGuard);

	return TimedOut ? WAIT_TIMEOUT : 0;
}

DWORD QueuePut (QUEUE_OBJECT *q, PVOID msg, DWORD msize, DWORD MaxWait)
{
	DWORD TotalWaitTime = 0;
	BOOL TimedOut = FALSE;

	WaitForSingleObject (q->qGuard, INFINITE);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */

	while (QueueFull (q) && !TimedOut) {
		ReleaseMutex (q->qGuard);
		TimedOut = (WaitForSingleObject (q->qNf, CV_TIMEOUT) == WAIT_TIMEOUT);
		WaitForSingleObject (q->qGuard, INFINITE);
	}
	/* Put the message in the queue */
	if (!TimedOut || !QueueFull (q)) QueueInsert (q, msg, msize);	
	/* Signal that the queue is not empty as we've inserted a message */
	SetEvent (q->qNe);
	ReleaseMutex (q->qGuard);
 
	return TimedOut ? WAIT_TIMEOUT : 0;
}

DWORD QueueInitialize (QUEUE_OBJECT *q, DWORD msize, DWORD nmsgs)
{
	/* Initialize queue, including its mutex and events */
	/* Allocate storage for all messages. */
	
	if ((q->msgArray = calloc (nmsgs, msize)) == NULL) return 1;
	q->qFirst = q->qLast = 0;
	q->qSize = nmsgs;

	q->qGuard = CreateMutex (NULL, FALSE, NULL);
	q->qNe = CreateEvent (NULL, FALSE, FALSE, NULL); /* Auto-reset events */
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


DWORD QueueRemove (QUEUE_OBJECT *q, PVOID msg, DWORD msize)
{
	char *pm;

	if (QueueEmpty(q)) return 1; /* Error - Q is empty */
	pm = q->msgArray;
	/* Remove oldest ("first") message */
	memcpy (msg, pm + (q->qFirst * msize), msize);
	q->qFirst = ((q->qFirst + 1) % q->qSize);
	return 0; /* no error */
}

DWORD QueueInsert (QUEUE_OBJECT *q, PVOID msg, DWORD msize)
{
	char *pm;

	if (QueueFull(q)) return 1; /* Error - Q is full */
	pm = q->msgArray;
	/* Add a new youngest ("last") message */
	memcpy (pm + (q->qLast * msize), msg, msize);
	q->qLast = ((q->qLast + 1) % q->qSize);
	return 0;
}
