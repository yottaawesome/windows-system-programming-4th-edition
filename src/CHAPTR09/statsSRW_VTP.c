/* Chapter 9. statsSRW_VTP.c                                        */
/* Simple boss/worker system, where each worker reports        */
/* its work output back to the boss for display.            */
/* MUTEX VERSION                                            */

#include "Everything.h"
#define DELAY_COUNT 20
#define CACHE_LINE_SIZE 64

/* Usage: statsSRW_VTP nThread ntasks [delay [trace]]                */
/* start up nThread worker threads, each assigned to perform    */
/* "ntasks" work units. Each thread reports its progress        */
/* in its own unshard slot in a work performed array            */

VOID CALLBACK Worker (PTP_CALLBACK_INSTANCE, PVOID, PTP_WORK);
int workerDelay = DELAY_COUNT;
BOOL traceFlag = FALSE;

__declspec(align(CACHE_LINE_SIZE))
typedef struct _THARG {
	SRWLOCK slimRWL;
	int objectNumber;
    unsigned int tasksToComplete;
    unsigned int tasksComplete;
} THARG;
    
int _tmain (int argc, LPTSTR argv[])
{
    INT nThread, iThread;
	HANDLE *pWorkObjects;
	SRWLOCK srwl;
    unsigned int tasksPerThread, totalTasksComplete;
    THARG ** pWorkObjArgsArray, *pThreadArg;
	TP_CALLBACK_ENVIRON cbe;  // Callback environment
    
    if (argc < 3) {
        _tprintf (_T("Usage: statsSRW_VTP nThread ntasks [trace]\n"));
        return 1;
    }
	_tprintf (_T("statsSRW_VTP %s %s %s\n"), argv[1], argv[2], argc>=4 ? argv[3] : "");
       
    nThread = _ttoi(argv[1]);
    tasksPerThread = _ttoi(argv[2]);
	if (argc >= 4) workerDelay = _ttoi(argv[3]);
    traceFlag = (argc >= 5);

	/* Initialize the SRW lock */
	InitializeSRWLock (&srwl);

    pWorkObjects = malloc (nThread * sizeof(PTP_WORK));
	if (pWorkObjects != NULL)
		pWorkObjArgsArray = malloc (nThread * sizeof(THARG *));

	if (pWorkObjects == NULL || pWorkObjArgsArray == NULL)
        ReportError (_T("Cannot allocate working memory for worke item or argument array."), 2, TRUE);
	InitializeThreadpoolEnvironment (&cbe);
    
    for (iThread = 0; iThread < nThread; iThread++) {
        /* Fill in the thread arg */
		pThreadArg = (pWorkObjArgsArray[iThread] = _aligned_malloc (sizeof(THARG), CACHE_LINE_SIZE));
		if (NULL == pThreadArg)
			ReportError (_T("Cannot allocate memory for a thread argument structure."), 3, TRUE);
        pThreadArg->objectNumber = iThread;
        pThreadArg->tasksToComplete = tasksPerThread;
        pThreadArg->tasksComplete = 0;
        pThreadArg->slimRWL = srwl;
		pWorkObjects[iThread] = CreateThreadpoolWork (Worker, pThreadArg, &cbe);
        if (pWorkObjects[iThread] == NULL) 
            ReportError (_T("Cannot create consumer thread"), 4, TRUE);
		SubmitThreadpoolWork (pWorkObjects[iThread]);
    }

	/* Wait for the threads to complete */
	for (iThread = 0; iThread < nThread; iThread++) {
		/* Wait for the thread pool work item to complete */
		WaitForThreadpoolWorkCallbacks (pWorkObjects[iThread], FALSE);
		CloseThreadpoolWork(pWorkObjects[iThread]);
	}

    free (pWorkObjects);
    _tprintf (_T("Worker threads have terminated\n"));
    totalTasksComplete = 0;
    for (iThread = 0; iThread < nThread; iThread++) {
		pThreadArg = pWorkObjArgsArray[iThread];
        if (traceFlag) _tprintf (_T("Tasks completed by thread %5d: %6d\n"), iThread, pThreadArg->tasksComplete);
        totalTasksComplete += pThreadArg->tasksComplete;
		_aligned_free (pThreadArg);
    }
	free (pWorkObjArgsArray);

    if (traceFlag) _tprintf (_T("Total work performed: %d.\n"), totalTasksComplete);
    
	return 0;
}

VOID CALLBACK Worker (PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WORK Work)
{
    THARG * threadArgs;

    threadArgs = (THARG *)Context;
	if (traceFlag)
		_tprintf (_T("Worker: %d. Thread Number: %d.\n"), threadArgs->objectNumber, GetCurrentThreadId());

    while (threadArgs->tasksComplete < threadArgs->tasksToComplete) {
        delay_cpu (workerDelay);
		AcquireSRWLockExclusive (&(threadArgs->slimRWL));
		(threadArgs->tasksComplete)++;
		ReleaseSRWLockExclusive (&(threadArgs->slimRWL));
    }
        
    return;        
}

