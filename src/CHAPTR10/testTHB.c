/* Chapter 10. testTHB.c	  			*/
/* Test the threshold Barrier extended object type		*/

#include "Everything.h"
#include "synchobj.h"
#include <time.h>
#define DELAY_COUNT 100
#define MAX_THREADS 1024

/*  Usage: test_thb nThread threshold			*/
/* start up nThread worker threads, each of which waits at	*/
/* the Barrier for enough threads to arrive.			*/
/* The threads then all proceed. The threads log their time	*/
/* of arrival and departure from the Barrier			*/

DWORD WINAPI worker (PVOID);

typedef struct _THARG {
	int tNumber;
	THRESHOLD_BARRIER *pBarrier;
	char waste [8]; /* Assure there is no quadword overlap */
} THARG;

int _tmain (int argc, LPTSTR argv[])
{
	int tStatus, nThread, iThread, bValue;
	HANDLE *hWorker;
	THB_OBJECT oBarrier;
	THARG * tArg;
	
	if (!WindowsVersionOK (4, 0)) 
		ReportError (_T("This program requires Windows NT 4.0 or greater"), 1, FALSE);

	if (argc != 3) {
		_tprintf (_T("Usage: test_thb nThread threshold\n"));
		return 1;
	}
		
	srand ((DWORD)time(NULL)); /* Seed the random # generator 	*/

	nThread = _ttoi(argv[1]);
	if (nThread > MAX_THREADS) {
		_tprintf (_T("Maximum number of threads is %d.\n"), MAX_THREADS);
		return 2;
	}
	bValue = _ttoi(argv[2]);
	hWorker = malloc (nThread * sizeof(HANDLE));
	tArg = calloc (nThread, sizeof (THARG));
	if (hWorker == NULL || tArg == NULL)
		ReportError (_T("Cannot allocate working memory"), 1, TRUE);
	
	tStatus = CreateThresholdBarrier (&oBarrier, bValue);
	
	for (iThread = 0; iThread < nThread; iThread++) {
		/* Fillin the thread arg */
		tArg[iThread].tNumber = iThread;
		tArg[iThread].pBarrier = oBarrier;
		hWorker[iThread] = (HANDLE)_beginthreadex (NULL, 0, worker, 
			&tArg[iThread], 0, NULL);
		if (hWorker[iThread] == NULL) 
			ReportError (_T("Cannot create consumer thread"), 2, TRUE);
		Sleep(rand()/10);
	}
	
	/* Wait for the threads to complete */
	WaitForMultipleObjects (nThread, hWorker, TRUE, INFINITE);
	free (hWorker);
	_tprintf (_T("Worker threads have terminated\n"));
	free (tArg);
	_tprintf (_T("Result of closing THB. %d\n"), CloseThresholdBarrier (oBarrier));
	return 0;
}

DWORD WINAPI worker (PVOID arg)
{
	THARG * tArg;
	int iThread, tStatus;
	time_t tStart, tEnd;

	tArg = (THARG *)arg;	
	iThread = tArg->tNumber;

	tStart = time(NULL);	
	_tprintf (_T("Start: Thread number %d %s"), iThread, ctime (&tStart));

	tStatus = WaitThresholdBarrier (tArg->pBarrier);
	if (tStatus != 0) ReportError (_T("Error waiting on at threshold Barrier"), 9, TRUE);

	tEnd = time(NULL);
	_tprintf (_T("End:   Thread number %d %s"), iThread, ctime (&tEnd));
	return 0;		
}

