/* Chapter 9. statsSRW.c                                        */
/* Simple boss/worker system, where each worker reports        */
/* its work output back to the boss for display.            */
/* MUTEX VERSION                                            */

#include "Everything.h"
#define DELAY_COUNT 20
#define CACHE_LINE_SIZE 64

/* Usage: statsSRW nThread ntasks [delay [trace]]                */
/* start up nThread worker threads, each assigned to perform    */
/* "ntasks" work units. Each thread reports its progress        */
/* in its own unshard slot in a work performed array            */

DWORD WINAPI Worker (void *);
int workerDelay = DELAY_COUNT;

__declspec(align(CACHE_LINE_SIZE))
typedef struct _THARG {
	SRWLOCK SRWL;
	int threadNumber;
    unsigned int tasksToComplete;
    unsigned int tasksComplete;
} THARG;
    
int _tmain (int argc, LPTSTR argv[])
{
    INT nThread, iThread;
	HANDLE *hWorkers;
	SRWLOCK srwl;
    unsigned int tasksPerThread, totalTasksComplete;
    THARG ** pThreadArgsArray, *pThreadArg;
    BOOL traceFlag = FALSE;
    
    if (argc < 3) {
        _tprintf (_T("Usage: statsSRW nThread ntasks [trace]\n"));
        return 1;
    }
	_tprintf (_T("statsSRW %s %s %s\n"), argv[1], argv[2], argc>=4 ? argv[3] : "");
       
    nThread = _ttoi(argv[1]);
    tasksPerThread = _ttoi(argv[2]);
	if (argc >= 4) workerDelay = _ttoi(argv[3]);
    traceFlag = (argc >= 5);

	/* Initialize the SRW lock */
	InitializeSRWLock (&srwl);

    hWorkers = malloc (nThread * sizeof(HANDLE));
	pThreadArgsArray = malloc (nThread * sizeof(THARG *));

	if (hWorkers == NULL || pThreadArgsArray == NULL)
        ReportError (_T("Cannot allocate working memory for worker handles or argument array."), 2, TRUE);
    
    for (iThread = 0; iThread < nThread; iThread++) {
        /* Fill in the thread arg */
		pThreadArg = (pThreadArgsArray[iThread] = _aligned_malloc (sizeof(THARG), CACHE_LINE_SIZE));
		if (NULL == pThreadArg)
			ReportError (_T("Cannot allocate memory for a thread argument structure."), 3, TRUE);
        pThreadArg->threadNumber = iThread;
        pThreadArg->tasksToComplete = tasksPerThread;
        pThreadArg->tasksComplete = 0;
        pThreadArg->SRWL = srwl;
		hWorkers[iThread] = (HANDLE)_beginthreadex (NULL, 0, Worker, pThreadArg, 0, NULL);
        if (hWorkers[iThread] == NULL) 
            ReportError (_T("Cannot create consumer thread"), 4, TRUE);
    }

	/* Wait for the threads to complete */
	for (iThread = 0; iThread < nThread; iThread++)
		WaitForSingleObject (hWorkers[iThread], INFINITE);

    free (hWorkers);
    _tprintf (_T("Worker threads have terminated\n"));
    totalTasksComplete = 0;
    for (iThread = 0; iThread < nThread; iThread++) {
		pThreadArg = pThreadArgsArray[iThread];
        if (traceFlag) _tprintf (_T("Tasks completed by thread %5d: %6d\n"), iThread, pThreadArg->tasksComplete);
        totalTasksComplete += pThreadArg->tasksComplete;
		_aligned_free (pThreadArg);
    }
	free (pThreadArgsArray);

    if (traceFlag) _tprintf (_T("Total work performed: %d.\n"), totalTasksComplete);
    
	return 0;
}

DWORD WINAPI Worker (void *arg)
{
    THARG * threadArgs;
    int iThread;

    threadArgs = (THARG *)arg;    
    iThread = threadArgs->threadNumber;

    while (threadArgs->tasksComplete < threadArgs->tasksToComplete) {
        delay_cpu (workerDelay);
		AcquireSRWLockExclusive (&(threadArgs->SRWL));
		(threadArgs->tasksComplete)++;
		ReleaseSRWLockExclusive (&(threadArgs->SRWL));
    }
        
    return 0;        
}

