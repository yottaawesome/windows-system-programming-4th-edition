/* Chapter 10. QueueObj.c	SingalObjectAndWait version		*/
/* Queue functions												*/
#include "Everything.h"
#include "SynchObj.h"
#include "messages.h"

/* Finite bounded queue management functions */
/* Note the placeholders for maxWait and TimedOut
 * They are not used in this solution; alternative solutions
 * could implement a timeout feature (example, the solution using
 * a CRITICAL_SECTION.
 */

DWORD QueueGet (QUEUE_OBJECT *q, PVOID msg, DWORD mSize, DWORD maxWait)
{
	WaitForSingleObject (q->qGuard, INFINITE);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */
	while (QueueEmpty (q)) {
		SignalObjectAndWait (q->qGuard, q->qNe, INFINITE, FALSE);
		WaitForSingleObject (q->qGuard, INFINITE);
	}
	/* remove the message from the queue */
	QueueRemove (q, msg, mSize);
	/* Signal that the queue is not full as we've removed a message */
	PulseEvent (q->qNf);
	ReleaseMutex (q->qGuard);

	return 0;
}

DWORD QueuePut (QUEUE_OBJECT *q, PVOID msg, DWORD mSize, DWORD maxWait)
{
	WaitForSingleObject (q->qGuard, INFINITE);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */
	while (QueueFull (q)) {
		SignalObjectAndWait (q->qGuard, q->qNf, INFINITE, FALSE);
		WaitForSingleObject (q->qGuard, INFINITE);
	}
	/* Put the message in the queue */
	QueueInsert (q, msg, mSize);
	/* Signal that the queue is not empty as we've inserted a message */
	PulseEvent (q->qNe);
	ReleaseMutex (q->qGuard);
 
	return 0;
}

DWORD QueueInitialize (QUEUE_OBJECT *q, DWORD mSize, DWORD nMsgs)
{
	/* Initialize queue, including its mutex and events */
	/* Allocate storage for all messages. */
	
	if ((q->msgArray = calloc (nMsgs, mSize)) == NULL) return 1;
	q->qFirst = q->qLast = 0;
	q->qSize = nMsgs;

	q->qGuard = CreateMutex (NULL, FALSE, NULL);
	q->qNe = CreateEvent (NULL, TRUE, FALSE, NULL);
	q->qNf = CreateEvent (NULL, TRUE, FALSE, NULL);
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
