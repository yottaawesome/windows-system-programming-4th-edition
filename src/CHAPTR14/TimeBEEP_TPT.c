/* Chapter 14 -  TimeBeep_TPT.c. Periodic alarm. */
/* Usage: TimeBeep_TPT period (in ms). */
/* This implementation uses the "thread pool timers" introduced
   with NT 6.0. 
   This program uses a console control handler to catch
   control C signals; this subject is covered at the end of
   Chapter 7.  */

#include "Everything.h"

static BOOL WINAPI Handler (DWORD CntrlEvent);
static VOID CALLBACK Beeper (PTP_CALLBACK_INSTANCE, PVOID, PTP_TIMER);
volatile static BOOL exitFlag = FALSE;
volatile static int count = 0;
volatile static PTP_TIMER pTimerPool;

HANDLE hTimer;

int _tmain (int argc, LPTSTR argv [])
{
    DWORD period;
    FILETIME dueTime;
    LARGE_INTEGER lDueTime;
    TP_CALLBACK_ENVIRON cbe;  // Callback environment

    if (!WindowsVersionOK (6, 0)) 
        ReportError (_T("This program requires Windows NT 6.0 or greater"), 1, FALSE);

    if (argc >= 2)
        period = _ttoi (argv [1]) * 1000;
    else ReportError (_T ("Usage: TimeBeep_TPT period(in seconds)"), 1, FALSE);

    if (!SetConsoleCtrlHandler (Handler, TRUE))
        ReportError (_T ("Error setting event handler"), 2, TRUE);

    InitializeThreadpoolEnvironment (&cbe);
    pTimerPool = CreateThreadpoolTimer (Beeper, "Timer Thread Pool", &cbe);
    if (NULL == pTimerPool)
        ReportError (_T("CreateThreadpoolWork failed"), 2, TRUE);

    /* The due time is relative, so it is a negative time in a FILETIME structure
       Due time is negative relative to
       current time. period is in ms (10**-3 sec) whereas
       the due time is in 100 ns (10**-7 sec) units to be
       consistent with a FILETIME. */
    lDueTime.QuadPart = -(LONGLONG)period * 10000;
    dueTime.dwLowDateTime = lDueTime.LowPart;
    dueTime.dwHighDateTime = lDueTime.HighPart;
    SetThreadpoolTimer (pTimerPool, &dueTime, period, 0);

    /*    Enter the main loop - test for exitFlag every 5 sec and print the current count. */
    while (!exitFlag) {
        _tprintf (_T("count = %d\n"), count); 
        /* count is increased in the timer routine each time it's called */
        Sleep (5000);
    }

    _tprintf (_T("Shut down. count = %d"), count);
    return 0;
}    

static VOID CALLBACK Beeper (PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer)
{
    int currentCount = InterlockedIncrement(&count);

    _tprintf (_T("About to perform beep number: %d\n"), currentCount);
    Beep (1000 /* Frequency */, 250 /* Duration (ms) */);
    return;
}

BOOL WINAPI Handler (DWORD CntrlEvent)
{
    exitFlag = TRUE;
    CloseThreadpoolTimer(pTimerPool);
    _tprintf (_T("Shutting Down\n"));

    return TRUE;
}
