/* Chapter 10. QueueObjCS.c	CRITICAL_SECTION version			*/
/* Queue functions												*/

#include "Everything.h"
#include "SynchObjCS.h"

/* Finite bounded queue management functions */

DWORD QueueGet (QUEUE_OBJECT *q, PVOID msg, DWORD msize, DWORD MaxWait)
{
	DWORD TotalWaitTime = 0;
	BOOL TimedOut = FALSE;

	EnterCriticalSection (&q->qGuard);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */

	while (QueueEmpty (q) && !TimedOut) {
		LeaveCriticalSection (&q->qGuard);
		WaitForSingleObject (q->qNe, CV_TIMEOUT);
		if (MaxWait != INFINITE) {
			TotalWaitTime += CV_TIMEOUT;
			TimedOut = (TotalWaitTime > MaxWait);
		}
		EnterCriticalSection (&q->qGuard);
	}
	/* remove the message from the queue */
	if (!TimedOut) QueueRemove (q, msg, msize);
	/* Signal that the queue is not full as we've removed a message */
	PulseEvent (q->qNf);
	LeaveCriticalSection (&q->qGuard);

	return TimedOut ? WAIT_TIMEOUT : 0;
}

DWORD QueuePut (QUEUE_OBJECT *q, PVOID msg, DWORD msize, DWORD MaxWait)
{
	DWORD TotalWaitTime = 0;
	BOOL TimedOut = FALSE;

//	WaitForSingleObject (q->qGuard, INFINITE);
	EnterCriticalSection (&q->qGuard);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */
	while (QueueFull (q) && !TimedOut) {
//		ReleaseMutex (q->qGuard);
		LeaveCriticalSection (&q->qGuard);
		WaitForSingleObject (q->qNf, CV_TIMEOUT);
		if (MaxWait != INFINITE) {
			TotalWaitTime += CV_TIMEOUT;
			TimedOut = (TotalWaitTime > MaxWait);
		}
//		WaitForSingleObject (q->qGuard, INFINITE);
		EnterCriticalSection (&q->qGuard);
	}
	/* Put the message in the queue */
	if (!TimedOut) QueueInsert (q, msg, msize);	
	/* Signal that the queue is not empty as we've inserted a message */
	PulseEvent (q->qNe);
//	ReleaseMutex (q->qGuard);
	LeaveCriticalSection (&q->qGuard);
 
	return TimedOut ? WAIT_TIMEOUT : 0;
}

DWORD QueueInitialize (QUEUE_OBJECT *q, DWORD msize, DWORD nmsgs)
{
	/* Initialize queue, including its mutex and events */
	/* Allocate storage for all messages. */
	
	if ((q->msgArray = calloc (nmsgs, msize)) == NULL) return 1;
	q->qFirst = q->qLast = 0;
	q->qSize = nmsgs;

	InitializeCriticalSection (&q->qGuard);
//	q->qGuard = CreateMutex (NULL, FALSE, NULL);
	q->qNe = CreateEvent (NULL, TRUE, FALSE, NULL);
	q->qNf = CreateEvent (NULL, TRUE, FALSE, NULL);
	return 0; /* No error */
}

DWORD QueueDestroy (QUEUE_OBJECT *q)
{
	/* Free all the resources created by QueueInitialize */
//	WaitForSingleObject (q->qGuard, INFINITE);
	EnterCriticalSection (&q->qGuard);
	free (q->msgArray);
	q->msgArray = NULL;
	CloseHandle (q->qNe);
	CloseHandle (q->qNf);
//	ReleaseMutex (q->qGuard);
	LeaveCriticalSection (&q->qGuard);
//	CloseHandle (q->qGuard);
	DeleteCriticalSection (&q->qGuard);

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
