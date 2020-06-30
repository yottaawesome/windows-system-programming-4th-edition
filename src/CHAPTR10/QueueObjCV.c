/* Chapter 10. QueueObjCV */
/* Use Windows Condition Variables introduced with NT6.         */
/* Queue functions												*/

#include "Everything.h"
#include "SynchObj.h"

/* Finite bounded queue management functions */

DWORD QueueGet (QUEUE_OBJECT *q, PVOID msg, DWORD msize, DWORD MaxWait)
{
	AcquireSRWLockExclusive (&q->qGuard);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */

	while (QueueEmpty (q)) {
		if (!SleepConditionVariableSRW (&q->qNe, &q->qGuard, INFINITE, 0))
			ReportError(_T("QueueGet failed. SleepConditionVariableCS."), 1, TRUE);
	}

	/* remove the message from the queue */
	QueueRemove (q, msg, msize);
	/* Signal that the queue is not full as we've removed a message */
	WakeConditionVariable (&q->qNf);
	ReleaseSRWLockExclusive (&q->qGuard);
	return 0;
}

DWORD QueuePut (QUEUE_OBJECT *q, PVOID msg, DWORD msize, DWORD MaxWait)
{
	AcquireSRWLockExclusive (&q->qGuard);
	if (q->msgArray == NULL) return 1;  /* Queue has been destroyed */

	while (QueueFull (q)) {
		if (!SleepConditionVariableSRW(&q->qNf, &q->qGuard, INFINITE, 0))
			ReportError(_T("QueuePut failed. SleepConditionVariableCS."), 1, TRUE);
	}
	/* Put the message in the queue */
	QueueInsert (q, msg, msize);	
	/* Signal that the queue is not empty as we've inserted a message */
	WakeConditionVariable (&q->qNe);
	ReleaseSRWLockExclusive (&q->qGuard);
	return 0;
}

DWORD QueueInitialize (QUEUE_OBJECT *q, DWORD msize, DWORD nmsgs)
{
	/* Initialize queue, including its mutex and events */
	/* Allocate storage for all messages. */
	
	if ((q->msgArray = calloc (nmsgs, msize)) == NULL) return 1;
	q->qFirst = q->qLast = 0;
	q->qSize = nmsgs;

	InitializeSRWLock(&q->qGuard);
	InitializeConditionVariable(&q->qNe);
	InitializeConditionVariable(&q->qNf);
	return 0; /* No error */
}

DWORD QueueDestroy (QUEUE_OBJECT *q)
{
	/* Free all the resources created by QueueInitialize */
	AcquireSRWLockExclusive (&q->qGuard);
	free (q->msgArray);
	q->msgArray = NULL;
	ReleaseSRWLockExclusive (&(q->qGuard));

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

