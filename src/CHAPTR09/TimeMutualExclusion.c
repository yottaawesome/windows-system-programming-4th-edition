/********************************************************************/
/*   Chapter 9
     Program to test the various Windows synchronization techniques for
	 mutual exclusion performance impact. Critical Sections, mutexes, 
	 Slim Read/Write Locks are compared, in terms of performance, in a very simple fashion.
	 Nearly all the factors that can impact threaded performance
	 are represented here in a simplified way so that you can
	 study the plausibility of various designs.
		Number of threads
		Use of mutexes or critical sections
		Nested resource acquisition
		CPU and I/O intensity
		Control of the number of active threads
	 
	 Usage:
	 TimeMutualExclusion Which [Depth [Delay [NumThreads [SleepPoints [NumThActive [SharedPercent]]]]]]

		Which:	1 - Test Critical Sections
				2 - Test nested Critical Sections
				3 - Test Mutexes
				4 - Test Slim Reader/Writer locks
				CONCEPT: We will determine the relative peformance
						charateristics of CSs, mutexes, and SRW locks, especially the
						amount of time they spend in the kernel. Testing
						two distinct nested CSs will determine if there is
						a linear (doubling) or nonlinear performance impact.

		Depth:	1 (default) or more for the number of recursive enter calls followed
						by the same number of leave calls
						Can be 0 to show effect of no synchronization calls
						Assumed 1 when Which == 4
				CONCEPT: is the performance impact linear with depth?

		Delay:	0 (default) or more for a delay factor to waste CPU time
					after all nested enters and before the nested leaves.
				CONCEPT: A large delay is indicative of CPU-intensive threads.
					A CPU-Intensive thread may have its priority increased but
					it might also be prempted for another thread.

		NumThreads:	Number of concurrent threads contending for the 
					lock. Must be between 1 and
					64 (the max for WaitForMultipleObjects). 
					4 is the default value.
				CONCEPT: Determine if total elapsed time is linear with
					the number of threads. A large number of threads
					contending for the same resources might degrade
					performance non-linearly.

		SleepPoints:	0	[Default] Never sleep, hence do not yield CPU
						1	Sleep(0) after obtaining all resources, yielding
							the CPU while holding the resources
						2	Sleep(0) after releasing all resources, yielding
							the CPU without holding any resources
						3	Sleep(0) at both points
				CONCEPT: Yielding the CPU shows the best-case effect on a
					thread that performs I/O or blocks for other reasons.

		NumThActive: Number of activated threads. NumThreads is the default.
					All other threads are waiting on a semaphore. 0 means
					no limit.
				CONCEPT: Performance may improve if we limit the number of
					concurrently active threads contending for the same
					resources. This also has the effect of serializing
					thread execution, esp with low values.

	    SharedPercent: Ignored if Which != 4 (Slim Reader/Writer locks)
		            Default 0 (all locks are exclusive). Lock type is
					determined randomly.
	
	  ADDITIONAL TESTING CONCEPT: Determine if performance varies depending
	  upon operation in the foreground or background.

  Author: Johnson (John) M. Hart. April 23, 1998. Extended May 10, 1998,
        Updated May 31, 2009.
		Based on an idea from David Poulton, with reuse of a small
		amount of code
	Build as a MULTITHREADED CONSOLE APPLICATION.
*/
/********************************************************************/

#include "Everything.h"

#define ITERATIONS 10000000	/*  Number of times to iterate through the mutual exclusion test */

#define THREADS 4			/*  Default Number of threads to test mutual exclusion with.
						cannot be more than 64 (limitation of WaitForMultipleObjects) */

DWORD WINAPI CrSecProc(LPVOID);
DWORD WINAPI CrSecProcA(LPVOID);
DWORD WINAPI MutexProc(LPVOID);
DWORD WINAPI SRWLockProc(LPVOID);

int DelayFactor = 0, Which = 1, Depth = 1, NumThreads = THREADS, SleepPoints = 0, SharedPercent = 0;
int NumThActive;

HANDLE hMutex;					/* Mutex handle */
HANDLE hSem;					/* Semaphore limiting active threads */
CRITICAL_SECTION hCritical, hCriticalA;			/*  Critical Section object */
SRWLOCK hSRWL;

/********************************************************************/
/*  Time wasting function. Waste a random amount of CPU time.       */
/********************************************************************/

void WasteTime (DWORD DelayFactor)
{
	int Delay, k, i;
	if (DelayFactor == 0) return;
	Delay = (DWORD)(DelayFactor * (float)rand()/RAND_MAX); 
	for (i = 0; i < Delay; i++) k = rand()*rand(); /* Waste time */

	return;
}

/************************************************************************/
/*  For each mutual exclusion algorithm create the threads and wait for */
/*	them to finish														*/
/************************************************************************/
int _tmain(int argc, LPTSTR argv[])
{	
	HANDLE *hThreadHandles;			/* Array to hold the thread handles */
	DWORD ThreadID;							/* Dummy variable to hold the thread identifier. */
	int iTh;								/* Loop Control Variables */

	FILETIME FileTime;	/* Times to seed random #s. */
	SYSTEMTIME SysTi;

/*	TimeMutex Which [Depth [Delay [NumThreads [SleepPoints]]]] */

	/*  Randomly initialize the random number seed */
	GetSystemTime (&SysTi);
	SystemTimeToFileTime (&SysTi, &FileTime);
	srand (FileTime.dwLowDateTime);
		
	if (argc >= 4) DelayFactor = _ttoi (argv[3]);		
	if (argc >= 3) Depth = _ttoi (argv[2]);
	if (argc >= 2) Which = _ttoi (argv[1]);
	if (argc >= 5) NumThreads = _ttoi (argv[4]);
	if (argc >= 6) SleepPoints = _ttoi (argv[5]);
	if (argc >= 7) NumThActive = max (_ttoi (argv[6]), 0);
	else NumThActive = NumThreads;

	if (4 == Which && argc >= 8) {
		SharedPercent = min (_ttoi (argv[7]), 100);
		if (!WindowsVersionOK (6, 0)) 
		    ReportError (_T("This program requires Windows NT 6.0 or greater"), 1, FALSE);
	}

	if (Which != 1 && Which != 2 && Which != 3 && Which != 4) {
		_tprintf (_T("The first command line argument must be 1, 2, 3, 4.\n"));
		return 1;
	}
	if (!(NumThreads >= 1 && NumThreads <= MAXIMUM_WAIT_OBJECTS)) {
		_tprintf (_T("Number of threads must be between 1 and %d\n"), MAXIMUM_WAIT_OBJECTS);
		return 2;
	}
	NumThActive = min (NumThActive, NumThreads);
	if (!(SleepPoints >= 0 && SleepPoints <= 3)) SleepPoints = 0;

	_tprintf (_T("Which = %d, Depth = %d, Delay = %d, NumThreads = %d, SleepPoints = %d, NumThActive = %d\n"),
		Which, Depth, DelayFactor, NumThreads, SleepPoints, NumThActive);
	if (4 == Which)
		_tprintf (_T("Percent of acquistions which are shared = %d.\n"), SharedPercent);

	if (3 == Which) hMutex=CreateMutex (NULL, FALSE, NULL);		
	if (1 == Which || 2 == Which) InitializeCriticalSection (&hCritical);
	if (2 == Which) InitializeCriticalSection (&hCriticalA);
	if (4 == Which) InitializeSRWLock (&hSRWL);

	hSem = CreateSemaphore (NULL, NumThActive, max (NumThActive, 1), NULL);
	if (hSem == NULL) {
		_tprintf (_T("Cannot create throttle semaphore. %d\n"), GetLastError());
		return 3;
	}

	/*	Create all the threads, then wait for them to complete */

	hThreadHandles = malloc (NumThreads * sizeof(HANDLE));
	if (hThreadHandles == NULL) {
		_tprintf (_T("Can not allocate memory for thread handles.\n"));
		return 4;
	}

	for(iTh=0; iTh<NumThreads; iTh++)				
	{
		if((hThreadHandles[iTh] = 
			(HANDLE)_beginthreadex 
			(NULL, 0, Which == 1 ? CrSecProc : Which == 2 ? CrSecProcA : 
			Which == 3 ? MutexProc : Which == 4 ? SRWLockProc : NULL,
				NULL, 0, &ThreadID)) == NULL)
		{
			_tprintf(_T("Error Creating Thread %d\n"), iTh);
			return 1;
		}	
	}		

	WaitForMultipleObjects(NumThreads,hThreadHandles,TRUE,INFINITE);  
	
	for(iTh=0; iTh<NumThreads; iTh++)	CloseHandle(hThreadHandles[iTh]);	
	free (hThreadHandles);

	if (Which == 1 || Which == 2) DeleteCriticalSection(&hCritical);
	if (Which == 2) DeleteCriticalSection(&hCriticalA);
	if (Which == 3) CloseHandle(hMutex);
	return 0;
}

/********************************************************************/
/*	Simple thread functions to enter and leave lock (CS or mutex or SRW)
	Each thread must wait on the gating semaphore before starting
	and releases the semaphore upon completion				*/
/********************************************************************/

DWORD WINAPI MutexProc(LPVOID Nothing)
{
	int i, k;

	if (NumThActive > 0) WaitForSingleObject (hSem, INFINITE);
	for (i = 0; i < ITERATIONS; i++)
	{
		for (k = 0; k < Depth; k++) /* Depth may be 0 */
			if(WaitForSingleObject(hMutex,INFINITE)==WAIT_FAILED) {
				_tprintf(_T("Wait for Mutex failed: %d\n"), GetLastError());
				break;    
			}
		if (SleepPoints % 2 == 1) Sleep (0); /* Yield the CPU, holding the resources */
		WasteTime (DelayFactor);/*  Critical Section code to go here */
		for (k = 0; k < Depth; k++) ReleaseMutex(hMutex);
		if (SleepPoints >= 2) Sleep (0); /* Yield the CPU, holding the resources */
	}
	if (NumThActive > 0) ReleaseSemaphore (hSem, 1, &i);
	return 0;
}

DWORD WINAPI CrSecProc(LPVOID Nothing)
{
	int i, k;

	if (NumThActive > 0) WaitForSingleObject (hSem, INFINITE);
	for (i = 0; i < ITERATIONS; i++)
	{
		for (k = 0; k < Depth; k++) 
			EnterCriticalSection (&hCritical);
		if (SleepPoints % 2 == 1) Sleep (0); /* Yield the CPU, holding the resources */
		WasteTime (DelayFactor);/*  Critical Section code to go here */
		for (k = 0; k < Depth; k++) 
			LeaveCriticalSection(&hCritical);
		if (SleepPoints >= 2) Sleep (0); /* Yield the CPU, holding the resources */
	}
	if (NumThActive > 0) ReleaseSemaphore (hSem, 1, &i);
	return 0;
}

DWORD WINAPI CrSecProcA(LPVOID Nothing)
{
	int i, k;

	if (NumThActive > 0) WaitForSingleObject (hSem, INFINITE);
	for (i = 0; i < ITERATIONS; i++)
	{
		for (k = 0; k < Depth; k++) {
			EnterCriticalSection (&hCritical);
			EnterCriticalSection (&hCriticalA);
		}
		if (SleepPoints % 2 == 1) Sleep (0); /* Yield the CPU, holding the resources */
		WasteTime (DelayFactor);/*  Critical Section code to go here */
		for (k = 0; k < Depth; k++) {
			LeaveCriticalSection(&hCriticalA);
			LeaveCriticalSection(&hCritical);
		}
		if (SleepPoints >= 2) Sleep (0); /* Yield the CPU, holding the resources */
	}
	if (NumThActive > 0) ReleaseSemaphore (hSem, 1, &i);
	return 0;
}

DWORD WINAPI SRWLockProc(LPVOID Nothing)
{
	int i;

	if (NumThActive > 0) WaitForSingleObject (hSem, INFINITE);
	for (i = 0; i < ITERATIONS; i++)
	{
		if (1 || SharedPercent > 0 && (100.0 * (float)rand())/RAND_MAX > SharedPercent)
		{
			// Exclusive lock
		    AcquireSRWLockExclusive (&hSRWL);
		    if (SleepPoints % 2 == 1) Sleep (0); /* Yield the CPU, holding the resources */
		    WasteTime (DelayFactor);/*  Critical Section code to go here */
	        ReleaseSRWLockExclusive(&hSRWL);
		    if (SleepPoints >= 2) Sleep (0); /* Yield the CPU, holding the resources */
		} else {
			// Shared Lock
		    AcquireSRWLockShared (&hSRWL);
		    if (SleepPoints % 2 == 1) Sleep (0); /* Yield the CPU, holding the resources */
		    WasteTime (DelayFactor);/*  Critical Section code to go here */
	        ReleaseSRWLockShared(&hSRWL);
		    if (SleepPoints >= 2) Sleep (0); /* Yield the CPU, holding the resources */
		}
	}
	if (NumThActive > 0) ReleaseSemaphore (hSem, 1, &i);
	return 0;
}
