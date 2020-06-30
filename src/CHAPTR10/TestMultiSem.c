/*	Test the Multiple Wait Semaphore sycnchronization compound object */


#include "Everything.h"
#include "Synchronize.h"

static DWORD WINAPI ProducerTh (LPVOID);
static DWORD WINAPI ConsumerTh (LPVOID);
static DWORD WINAPI MonitorTh (LPVOID);
static BOOL WINAPI CtrlcHandler (DWORD);

static SYNCHHANDLE hSemMW;
static LONG SemMax, SemInitial, NumProducers, NumConsumers, TotalExcess = 0;
static HANDLE hTestMutex;
static volatile BOOL Debug = FALSE;

static volatile BOOL Exit = FALSE;

static volatile DWORD NumSuccess = 0, NumFailures = 0, NumProduced = 0,
				FailureCount = 0, NumConsumed = 0;
static LPDWORD ProducerCount, ConsumerCount;


int _tmain (int argc, LPTSTR argv[])
	/*	Test CE semaphores; that is, semaphores created out of a mutex, a
		counter, and an autoreset event */

{

	HANDLE * Producer, * Consumer;
	LONG_PTR iThread;
	DWORD ThreadId;
	HANDLE hMonitor;
	BOOL FirstCFS;		/*  First come, first served, or first available? */
	TCHAR Name [MAX_PATH] = _T("");

	FILETIME FileTime;	/* Times to seed random #s. */
	SYSTEMTIME SysTi;

	if (!WindowsVersionOK (4, 0)) 
		ReportError (_T("This program requires Windows NT 4.0 or greater"), 1, FALSE);

	/*  Randomly initialize the random number seed */

	GetSystemTime (&SysTi);
	SystemTimeToFileTime (&SysTi, &FileTime);
	srand (FileTime.dwLowDateTime);

	if (!SetConsoleCtrlHandler (CtrlcHandler, TRUE))
		ReportError (_T("Failed to set console control handler"), 1, TRUE);

	_tprintf (_T("\nEnter SemMax, SemInitial, NumConsumers, NumProducers, FCFS?, Debug "));
	_tscanf (_T("%d %d %d %d %d %d"), &SemMax, &SemInitial, &NumConsumers, 
		&NumProducers, &FirstCFS, &Debug);
	_tprintf (_T("\nEnter Name - NULL for none: "));
	_tscanf (_T("%s"), Name);
	if (_tcscmp (Name, _T("NULL")) == 0) _tcscpy (Name, _T(""));	

	/* if (Debug) */ _tprintf (_T("You entered: %d %d %d %d %d %s\n"), 
		SemMax, SemInitial, NumConsumers, NumProducers, FirstCFS, Name);

	/*	Create a mutex to synchronize various aspects of the test, such as
		updating statistics. A CS won't work as we need a WaitForMultipleObjects
		involving this mutex in the monitor thread. */
	hTestMutex = CreateMutex (NULL, FALSE, NULL);
	if (hTestMutex == NULL) ReportError (_T("Could not create test mutex"), 2, TRUE);

	NumProduced = SemInitial; /* Initialize the usage statistics for the semaphore. */


	hSemMW = CreateSemaphoreMW (NULL, SemInitial, SemMax, FirstCFS, Name); 
	if (hSemMW == NULL) {
		_tprintf (_T("LastError: %x\n"), GetLastError());	
		ReportError (_T("Failed to create MW semaphore"), 3, TRUE);
	}
	if (Debug) _tprintf (_T("MW Semaphore created successfully\n"));

	/*  Create all the semaphore consuming and releasing threads */
	/*	Create arrays to hold the thread handles, and also
		create integer arrays to count number of iterations by each thread. By
		observing these counts, we can be sure that no deadlocks have occurred. */

	if (NumConsumers > 0) {
		Consumer = malloc (NumConsumers*sizeof(HANDLE));
		if (Consumer == NULL)
			ReportError (_T("Can not allocate Consumer handles"), 5, FALSE);
		ConsumerCount = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, NumConsumers*sizeof(DWORD));
		if (ConsumerCount == NULL)
			ReportError (_T("Can not allocate Consumer handles"), 5, FALSE);
	}
	
	if (NumProducers > 0) {
		Producer = malloc (NumProducers*sizeof(HANDLE));
		if (Producer == NULL)
			ReportError (_T("Can not allocate Producer handles"), 4, FALSE);
		ProducerCount = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, NumProducers*sizeof(DWORD));
		if (ProducerCount == NULL) 
			ReportError (_T("Can not allocate Producer handles"), 4, FALSE);
	}



	hMonitor = (HANDLE)_beginthreadex (NULL, 0, MonitorTh, (LPVOID)5000, CREATE_SUSPENDED, &ThreadId);
	if (hMonitor == NULL) ReportError (_T("Can not create monitor thread"), 6, TRUE);
	SetThreadPriority (hMonitor, THREAD_PRIORITY_HIGHEST);

	for (iThread = 0; iThread < NumConsumers; iThread++) {
		Consumer [iThread] = (HANDLE)_beginthreadex (NULL, 0, ConsumerTh, (LPVOID)iThread, CREATE_SUSPENDED, &ThreadId);
		if (Consumer[iThread] == NULL) ReportError (_T("Can not create consumer thread"), 3, TRUE);
	}
	for (iThread = 0; iThread < NumProducers; iThread++) {
		Producer [iThread] = (HANDLE)_beginthreadex (NULL, 0, ProducerTh, (LPVOID)iThread, CREATE_SUSPENDED, &ThreadId);
		if (Producer[iThread] == NULL) ReportError (_T("Can not create producder thread"), 3, TRUE);
	}

	WaitForSingleObject (hTestMutex, INFINITE);
	_tprintf (_T("All threads created successfully.\n"));
	ReleaseMutex (hTestMutex);
	for (iThread = 0; iThread < NumConsumers; iThread++) ResumeThread (Consumer [iThread]);
	for (iThread = 0; iThread < NumProducers; iThread++) ResumeThread (Producer [iThread]);

	ResumeThread (hMonitor);

	if (NumConsumers > 0) WaitForMultipleObjects (NumConsumers, Consumer, TRUE, INFINITE);
	if (NumProducers > 0) WaitForMultipleObjects (NumProducers, Producer, TRUE, INFINITE);
	WaitForSingleObject (hMonitor, INFINITE);

	for (iThread = 0; iThread < NumConsumers; iThread++) CloseHandle (Consumer [iThread]);
	for (iThread = 0; iThread < NumProducers; iThread++) CloseHandle (Producer [iThread]);


	HeapFree (GetProcessHeap(), 0, Producer); HeapFree (GetProcessHeap(), 0, Consumer);
	HeapFree (GetProcessHeap(), 0, ProducerCount); 
	HeapFree (GetProcessHeap(), 0, ConsumerCount);
	
	CloseHandle (hMonitor);
	if (!CloseSynchHandle (hSemMW))
		ReportError (_T("Failed closing synch handle"), 0, TRUE);

	_tprintf (_T("All threads terminated\n"));
	return 0;
}


static DWORD WINAPI ProducerTh (LPVOID ThNumber)
/*	Producer thread:
	1.	Compute for a random period of time.
	2.	Produce a random number of units, uniformly distributed [1, SemMax]
		The production will return with no units produced if it would cause the
		semaphore count to exceed the maximum (this is consistent with Win32 
		semaphore semantics).
	3.	Produce twice as quickly as consuming, as many productions will fail because
		the number of units is too large.
*/
{
	DWORD_PTR Id = (DWORD_PTR)ThNumber;
	DWORD Delay, i, k;
	LONG PrevCount = 0, RelCount = 0;

	WaitForSingleObject (hTestMutex, INFINITE);
	if (Debug) _tprintf (_T("Starting producer number %d.\n"), Id);
	ReleaseMutex (hTestMutex);

	while (!Exit) {
		/* Delay without necessarily giving up processor control. Notice how the production
			rate is the same as the consumption rate so as to keep things
			balanced. */
		Delay = rand() / 2; 
		for (i = 0; i < Delay; i++) k = rand()*rand() / 2; /* Waste time */
		if (rand() % 3 == 0) Sleep (rand()/(RAND_MAX/500)); /* Give up the processor 1/3 of the time
									just to make the thread interaction more interesting */

		RelCount = (long)(((float)rand()/RAND_MAX) * SemMax)+1;

		if (!ReleaseSemaphoreMW (hSemMW, RelCount, &PrevCount)) {
			WaitForSingleObject (hTestMutex, INFINITE);
			if (Debug) _tprintf (_T("Producer #: %d Failed. PrevCount = %d RelCount = %d\n"),
				Id, PrevCount, RelCount);
			NumFailures++;				/* Maintain producer statistics */
			FailureCount += RelCount;
			ReleaseMutex (hTestMutex);
		} else {
			WaitForSingleObject (hTestMutex, INFINITE);
			if (Debug) _tprintf (_T("Producer #: %d Succeeded. PrevCount = %d, RelCount = %d\n"),
				Id, PrevCount, RelCount);
			NumSuccess++;
			NumProduced += RelCount;
			ReleaseMutex (hTestMutex);
		}
		ProducerCount[Id]++;	/*	Number of producer iterations. */
	}


	_endthreadex (0);
	return 0;
}

static DWORD WINAPI ConsumerTh (LPVOID ThNumber)
{
	DWORD_PTR Id = (DWORD_PTR)ThNumber;
	DWORD Delay, i, k;
	LONG SeizeCount;

	WaitForSingleObject (hTestMutex, INFINITE);
	if (Debug) _tprintf (_T("Starting consumer number %d.\n"), Id);
	ReleaseMutex (hTestMutex);

	while (!Exit) {
		/* Delay without necessarily giving up processor control */
		Delay = rand();
		for (i = 0; i < Delay; i++) k = rand()*rand(); /* Waste time */;
		if (rand() % 3 == 0) Sleep (rand()/(RAND_MAX/1000)); 
				/*	Give up the processor 1/3 of the time
					just to make the thread interaction more interesting */

		/*	Random request size */
		SeizeCount = (long)(((float)rand()/RAND_MAX) * SemMax)+1;

		if (WaitForSemaphoreMW (hSemMW, SeizeCount, rand()) != WAIT_OBJECT_0) {
			if (Debug) {
				WaitForSingleObject (hTestMutex, INFINITE);
				if (Debug) _tprintf (_T("Consumer #: %d Timed out waiting for %d units\n"), 
					Id, SeizeCount);
				ReleaseMutex (hTestMutex);
			}
		} else { /*  The semaphore unit was obtained successfully - update statistics */
			WaitForSingleObject (hTestMutex, INFINITE);
			if (Debug) _tprintf (_T("Consumer #: %d Obtained %d units\n"), Id, SeizeCount);
			NumConsumed += SeizeCount;
			ReleaseMutex (hTestMutex);
		}
		ConsumerCount[Id]++;	/*  Number of consumer iterations. */
	}

	_endthreadex (0);
	return 0;
}


static DWORD WINAPI MonitorTh (LPVOID Delay)
/*	Monitor thread - periodically check the statistics for consistency */
{
	LONG CurCount = 0, Max = 0, ExcessCount, i;
	SYNCHHANDLE hState;

	HANDLE hBoth[2] = {hTestMutex, hSemMW->hMutex}; 
		/*	This is cheating to reach inside the opaque handle, but we need to get
			simultaneous access to both the semaphore state and to the statistics
			in order to test consistency. Some consistency failures are still possible,
			however, as the statistics are updated independently of changing the semaphore
			state, so the semaphore might be changed by another thread before the producer
			or consumer can update the global statistics, Thus, there can be no absolute
			consistency between the semaphore state and the global statistics maintained
			by this test program, and, to make them consistent would require putting a 
			critical section around the producer/consumer loop bodies, which would destroy
			the asynchronous operation required for testing.
			But, the general state of the semaphore can still be checked, and we can
			also be sure that it has not been corrupted. */

	/*	Get the handle to the semaphore state - more cheating */
	hState = (hSemMW->SharedHandle == NULL) ? hSemMW : hSemMW->SharedHandle;

	while (!Exit) {
		WaitForMultipleObjects (2, hBoth, TRUE, INFINITE);
		CurCount = hState->CurCount;
		Max = hState->MaxCount;

		ExcessCount = NumProduced - (NumConsumed + CurCount); 
		/*  Excess of production over consumption which should average out to 0
			Notice that if you have a shared semaphore, you might see discrepancies
			in each individual process as the counts used for this test are local
			to each process.  */
		_tprintf (_T("**Monitor - statistics:\nProduced: %d\nConsumed: %d\nCurrent:  %d\nMaximum:  %d\n"), 
			NumProduced,  NumConsumed, CurCount, Max);
		/*  For Correctness, we must have: Produced == Consumed + CurCount */
		if (ExcessCount != 0 || CurCount < 0 || CurCount > SemMax || SemMax != Max) {
			 _tprintf (_T("****Discrepancy: %d\n"), ExcessCount);
			 		TotalExcess += ExcessCount;
		}
		else _tprintf (_T("****Consistency test passed. Total excess count: %d\n"), TotalExcess);
		
		_tprintf (_T("Successful release calls: %d\nFailed release calls:     %d\n"), NumSuccess, NumFailures);
		_tprintf (_T("Units not released:       %d\n"), FailureCount);
		_tprintf (_T("Number of Consumer iterations, by thread\n"));
		for (i = 0; i < NumConsumers; i++) _tprintf (_T("%6d"), ConsumerCount[i]);
		_tprintf (_T("\nNumber of Producer iterations, by thread\n"));
		for (i = 0; i < NumProducers; i++) _tprintf (_T("%6d"), ProducerCount[i]);

		_tprintf (_T("\n**************\n"));

		ReleaseMutex (hTestMutex);
		ReleaseMutex (hSemMW->hMutex);

		/* Ignore Win64 warning about pointer truncation */
		Sleep ((DWORD)Delay);
	}

	_endthreadex(0);
	return 0;
}

static BOOL WINAPI CtrlcHandler (DWORD CtrlEvent)
{
	DWORD PrevCount;
	LONG i, c;

	if (CtrlEvent == CTRL_C_EVENT) {
		WaitForSingleObject (hTestMutex, INFINITE);
		_tprintf (_T("Control-c received. Shutting down.\n"));
		ReleaseMutex (hTestMutex);
	}

	else if (CtrlEvent == CTRL_CLOSE_EVENT) {
		WaitForSingleObject (hTestMutex, INFINITE);
		_tprintf (_T("Close event received. Shutting down.\n"));
		ReleaseMutex (hTestMutex);
	}

//	else if (CtrlEvent == CTRL_LOGOFF_EVENT) {
//		WaitForSingleObject (hTestMutex, INFINITE);
//		_tprintf (_T("LogOff received. OK to continue?\n"));
//		_tscanf (_T("%d"), &c);
//		_tprintf (_T("OK\n"));
//		ReleaseMutex (hTestMutex);
//	}

	else if (CtrlEvent == CTRL_SHUTDOWN_EVENT) {
		WaitForSingleObject (hTestMutex, INFINITE);
		_tprintf (_T("ShutDown received. OK to continue?\n"));
		_tscanf (_T("%d"), &c);
		_tprintf (_T("OK\n"));
		ReleaseMutex (hTestMutex);
	}

	else return FALSE;

	Exit = TRUE;

	/*	Assure that all Consumer threads are not blocked waiting on
		a semaphore so that they have a chance to shut down. */
	for (i = 0; i < NumConsumers; i++) {
		ReleaseSemaphoreMW (hSemMW, 1, &PrevCount);	
		/*  Release the CPU so that the consumer can be scheduled */
		Sleep (0);
	}

	return TRUE;

}

