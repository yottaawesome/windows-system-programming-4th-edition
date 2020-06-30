/* ThbObjectsX.c. Program 10-2		DEFECTIVE							*/
/* threshold barrier compound synch objects library		*/

#include "Everything.h"
#include "synchobj.h"

/**********************/
/*  THRESHOLD BARRIER OBJECTS */
/**********************/

DWORD CreateThresholdBarrier (THB_HANDLE *pthb, DWORD b_value)
{
	THB_HANDLE hthb;
	/* Initialize a barrier object */
	hthb = malloc (sizeof(THRESHOLD_BARRIER));
	*pthb = hthb;
	if (hthb == NULL) return 1;

	hthb->b_guard = CreateMutex (NULL, TRUE, NULL);
	if (hthb->b_guard == NULL) return 2;
	
	hthb->b_broadcast = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (hthb->b_broadcast == NULL) return 3;
	
	hthb->b_threshold = b_value;
	hthb->b_count = 0;
	return 0;
}


DWORD WaitThresholdBarrier (THB_HANDLE thb)
{
	/* Wait for the specified number of thread to reach */
	/* the barrier, then broadcast on the CV */
	
	WaitForSingleObject (thb->b_guard, INFINITE);
	thb->b_count++;  /* A new thread has arrived */
		
	while (thb->b_count < thb->b_threshold) {
		ReleaseMutex (thb->b_guard);
		WaitForSingleObject (thb->b_broadcast, CV_TIMEOUT);
		WaitForSingleObject (thb->b_guard, INFINITE);
	}
	PulseEvent (thb->b_broadcast); /* Broadcast to all waiting threads */
	ReleaseMutex (thb->b_guard);	
	return 0;
}


DWORD CloseThresholdBarrier (THB_HANDLE thb)
{
	/* Destroy the component mutexes and event */
	CloseHandle (thb->b_guard);
	CloseHandle (thb->b_broadcast);
	free (thb);
	return 0;

}
