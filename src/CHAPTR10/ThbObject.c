/* ThbObjects.c. Program 10-2							*/
/* threshold oBarrier compound synch objects library		*/

#include "Everything.h"
#include "synchobj.h"

/**********************/
/*  THRESHOLD oBarrier OBJECTS */
/**********************/

DWORD CreateThresholdBarrier (THB_OBJECT *pThb, DWORD bValue)
{
	THB_OBJECT objThb;
	/* Initialize a oBarrier object */
	objThb = malloc (sizeof(THRESHOLD_BARRIER));
	if (objThb == NULL) return SYNCH_OBJ_NOMEM;

	objThb->bGuard = CreateMutex (NULL, FALSE, NULL);
	if (objThb->bGuard == NULL) return SYNCH_OBJ_CREATE_FAILURE;
	
	/* Manual reset event */
	objThb->bEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (objThb->bEvent == NULL) return SYNCH_OBJ_CREATE_FAILURE;
	
	objThb->bThreshold = bValue;
	objThb->bCount = 0;

	*pThb = objThb;

	return 0;
}

DWORD WaitThresholdBarrier (THB_OBJECT thb)
{
	/* Wait for the specified number of threads to reach */
	/* the oBarrier, then broadcast on the CV */

	WaitForSingleObject (thb->bGuard, INFINITE);
	thb->bCount++;  /* A new thread has arrived */
	while (thb->bCount < thb->bThreshold) {
		SignalObjectAndWait (thb->bGuard, thb->bEvent, INFINITE, FALSE);
		WaitForSingleObject (thb->bGuard, INFINITE);
	}
	SetEvent (thb->bEvent); /* Broadcast to all waiting threads */
	/* NOTE: We are using the broadcast model, which is appropriate.
	 * HOWEVER, we can set the event because the condition is persistent
	 * and there is no need to reset the event. */
	ReleaseMutex (thb->bGuard);	
	return 0;
}

DWORD CloseThresholdBarrier (THB_OBJECT thb)
{
	/* Destroy the component mutexes and event once it is safe to do so */
	/* This is not entirely satisfactory, but is consistent with what would */
	/* happen if a mutex handle were destroyed while another thread waited on it */
	WaitForSingleObject (thb->bGuard, INFINITE);
	/* Be certain that no thread is waiting on the object. */
	if (thb->bCount < thb->bThreshold) {
		ReleaseMutex (thb->bGuard);
		return SYNCH_OBJ_BUSY;
	}

	/* Now release the mutex and close the handle */
	ReleaseMutex (thb->bGuard);
	CloseHandle (thb->bEvent);
	CloseHandle (thb->bGuard);
	free (thb);
	return 0;
}
