/* Chapter 14 -  TimeBeep.c. Periodic alarm. */
/* Usage: TimeBeep period (in ms). */
/* This implementation uses the "waitable timers" which are kernel objects.
   This program uses a console control handler to catch
   control C signals; see the end of Chapter 7.  */

#include "Everything.h"

static BOOL WINAPI Handler (DWORD CntrlEvent);
static VOID APIENTRY Beeper (LPVOID, DWORD, DWORD);
volatile static int exitFlag = 0;

HANDLE hTimer;

int _tmain (int argc, LPTSTR argv [])
{
	DWORD count = 0, period;
	LARGE_INTEGER dueTime;

	if (!WindowsVersionOK (5, 0)) 
		ReportError (_T("This program requires Windows NT 5.0 or greater"), 1, FALSE);

	if (argc >= 2) period = _ttoi (argv [1]) * 1000;
	else ReportError (_T ("Usage: TimeBeep period(in seconds)"), 1, FALSE);

	if (!SetConsoleCtrlHandler (Handler, TRUE))
		ReportError (_T ("Error setting event handler"), 2, TRUE);

	dueTime.QuadPart = -(LONGLONG)period * 10000;
			/*  Due time is negative for first time out relative to
				current time. period is in ms (10**-3 sec) whereas
				the due time is in 100 ns (10**-7 sec) units to be
				consistent with a FILETIME. */

	hTimer = CreateWaitableTimer (NULL /* Security attributes */,
		FALSE /*TRUE*/,	/*  Not manual reset (or "notification") timer, but
						a "synchronization timer." */
		NULL		/*  Do not name this timer - name space is shared with
						events, mutexes, semaphores, and mem map objects */);
	
	if (hTimer == NULL) 
		ReportError (_T ("Failure creating waitable timer"), 3, TRUE);
	if (!SetWaitableTimer (hTimer, 
			&dueTime /* relative time of first signal. Positive value would
						indicate an absolute time. */,
			period  /* Time period in ms */,
			Beeper  /* Timer function */,
			&count  /* Parameter passed to timer function */,
			FALSE    /* Does not apply - do not use it. */))
		ReportError (_T ("Failure setting waitable timer"), 4, TRUE);

	/*	Enter the main loop */
	while (!exitFlag) {
		_tprintf (_T("count = %d\n"), count); 
		/* count is increased in the timer routine */
		/*  Enter an alertable wait state, enabling the timer routine.
			The timer handle is a synchronization object, so you can 
			also wait on it. */
		SleepEx (INFINITE, TRUE);
		/* or WaitForSingleObjectEx (hTimer, INFINITE); Beeper(...); right? */
	}

	_tprintf (_T("Shut down. count = %d"), count);
	CancelWaitableTimer (hTimer);
	CloseHandle (hTimer);
	return 0;
}	

/*	Waitable timer callback function */
static VOID APIENTRY Beeper (LPVOID lpCount,
   DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{

	*(LPDWORD)lpCount = *(LPDWORD)lpCount + 1;

	_tprintf (_T("About to perform beep number: %d\n"), *(LPDWORD)lpCount);
	Beep (1000 /* Frequency */, 250 /* Duration (ms) */);
	return;
}

BOOL WINAPI Handler (DWORD CntrlEvent)
{
	InterlockedIncrement(&exitFlag);
	_tprintf (_T("Shutting Down\n"));

	return TRUE;
}
