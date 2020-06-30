/* Chapter 9 statsNS.c	  							*/
/* Simple boss/worker system, where each worker reports		*/
/* its work output back to the boss for display.			*/
/* NO SYNCHRONIZATION VERSION								*/

#include "Everything.h"
#define DELAY_COUNT 20

/* Usage: statsNS nThread ntasks [trace]				*/
/* start up nThread worker threads, each assigned to perform	*/
/* "ntasks" work units. Each thread reports its progress		*/
/* in its own unshard slot in a work performed array			*/

DWORD WINAPI worker (void *);

typedef struct _THARG {
	int threadNumber;
	unsigned int tasksToComplete;
    volatile unsigned int tasksComplete;
    int waste[12];      // Assure that each arg is on a distinct 64-byte cache line
} THARG;
	
int _tmain (int argc, LPTSTR argv[])
{
	INT nThread, iThread;
	HANDLE *hWorkers;
    unsigned int tasksPerThread, totalTasksComplete;
    THARG * threadArgs;
    DWORD ThId;
    BOOL traceFlag = FALSE;
	
	if (argc < 3) {
		_tprintf (_T("Usage: statsNS nThread ntasks [trace]\n"));
		return 1;
	}
	_tprintf ("statsNS %s %s\n", argv[1], argv[2]);	

	nThread = _ttoi(argv[1]);
	tasksPerThread = _ttoi(argv[2]);

	traceFlag = (argc >= 4);
	hWorkers = malloc (nThread * sizeof(HANDLE));
	threadArgs = calloc (nThread, sizeof (THARG));
    if (hWorkers == NULL || threadArgs == NULL)
		ReportError (_T("Cannot allocate working memory"), 1, TRUE);
	
	for (iThread = 0; iThread < nThread; iThread++) {
		/* Fill in the thread arg */
		threadArgs[iThread].threadNumber = iThread;
		threadArgs[iThread].tasksToComplete = tasksPerThread;
        threadArgs[iThread].tasksComplete = 0;
		hWorkers[iThread] = (HANDLE)_beginthreadex (NULL, 0, worker,
			&threadArgs[iThread], 0, &ThId);
		if (hWorkers[iThread] == NULL) 
			ReportError (_T("Cannot create consumer thread"), 2, TRUE);
	}
	
	/* Wait for the threads to complete */
	for (iThread = 0; iThread < nThread; iThread++)
		WaitForSingleObject (hWorkers[iThread], INFINITE);

	free (hWorkers);
	_tprintf (_T("Worker threads have terminated\n"));
	totalTasksComplete = 0;
	for (iThread = 0; iThread < nThread; iThread++) {
        if (traceFlag) _tprintf (_T("Tasks completed by thread %5d: %6d\n"), 
                    iThread, threadArgs[iThread].tasksComplete);
        totalTasksComplete += threadArgs[iThread].tasksComplete;
    }
    if (traceFlag) _tprintf (_T("Total work performed: %d.\n"), totalTasksComplete);
	
	free (threadArgs);
	return 0;
}


DWORD WINAPI worker (void *arg)
{
	THARG * threadArgs;
	int iThread;

	threadArgs = (THARG *)arg;	
	iThread = threadArgs->threadNumber;
		
	while (threadArgs->tasksComplete < threadArgs->tasksToComplete) {
		delay_cpu (DELAY_COUNT);
		(threadArgs->tasksComplete)++;
	}
		
	return 0;		
}

