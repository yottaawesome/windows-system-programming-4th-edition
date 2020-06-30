/* Chapter 9. statsMX_ST.c                                        */
/* Simple boss/worker system, where each worker reports        */
/* its work output back to the boss for display.            */
/* MUTEX VERSION                                            */

#include "Everything.h"
#define DELAY_COUNT 20
#define CACHE_LINE_SIZE 64

/* Usage: statsMX_ST nThread ntasks [delay [trace]]                */
/* start up nThread worker threads, each assigned to perform    */
/* "ntasks" work units. Each thread reports its progress        */
/* in its own unshard slot in a work performed array            */

DWORD WINAPI Worker (void *);
int workerDelay = DELAY_COUNT;

__declspec(align(CACHE_LINE_SIZE))
typedef struct _THARG {
	HANDLE hMutex;
	int threadNumber;
    unsigned int tasksToComplete;
    unsigned int tasksComplete;
} THARG;
    
HANDLE hSem;			// Throttling semaphore

int _tmain (int argc, LPTSTR argv[])
{
    INT nThread, iThread, maxthread;
    HANDLE *hWorkers, hMutex;
    unsigned int tasksPerThread, totalTasksComplete;
    THARG ** pThreadArgsArray, *pThreadArg;
    BOOL traceFlag = FALSE;
	SYSTEM_INFO SysInfo;
    
    if (argc < 3) {
        _tprintf (_T("Usage: statsMX_ST nThread ntasks [trace]\n"));
        return 1;
    }
	_tprintf (_T("statsMX_ST %s %s %s\n"), argv[1], argv[2], argc>=4 ? argv[3] : "");
       
    nThread = _ttoi(argv[1]);
    tasksPerThread = _ttoi(argv[2]);
	if (argc >= 4) workerDelay = _ttoi(argv[3]);
    traceFlag = (argc >= 5);

	/* Create the throttling semaphore. Initial and max counts are set to the
	   number of processors, except on a single-processor system, it's set to
	   the number threads, so there is no throttling
     */
	GetSystemInfo (&SysInfo);
	maxthread = nThread;
	if (SysInfo.dwNumberOfProcessors > 1) maxthread = SysInfo.dwNumberOfProcessors;
	hSem = CreateSemaphore (NULL, maxthread, maxthread, NULL);
	if (NULL == hSem)
		ReportError (_T("Failed creating semaphore."), 1, TRUE);
	/* Create the mutex */
    if ((hMutex = CreateMutex (NULL, FALSE, NULL)) == NULL)
		ReportError (_T("Failed creating mutex."), 1, TRUE);

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
        pThreadArg->hMutex = hMutex;
		hWorkers[iThread] = (HANDLE)_beginthreadex (NULL, 0, Worker, pThreadArg, 0, NULL);
        if (hWorkers[iThread] == NULL) 
            ReportError (_T("Cannot create consumer thread"), 4, TRUE);
    }

	/* Wait for the threads to complete */
	for (iThread = 0; iThread < nThread; iThread++)
		WaitForSingleObject (hWorkers[iThread], INFINITE);
	CloseHandle (hMutex);
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

	WaitForSingleObject (hSem, INFINITE);
    while (threadArgs->tasksComplete < threadArgs->tasksToComplete) {
        delay_cpu (workerDelay);
        WaitForSingleObject (threadArgs->hMutex, INFINITE);
        (threadArgs->tasksComplete)++;
        ReleaseMutex (threadArgs->hMutex); 
    }
	ReleaseSemaphore (hSem, 1, NULL);
        
    return 0;        
}

