/* Chapter 4. CNTRLC.C */
/* Catch Cntrl-C signals. */

#include "Everything.h"

static BOOL WINAPI Handler(DWORD cntrlEvent);
static BOOL exitFlag = FALSE;

int _tmain(int argc, LPTSTR argv[])

/* Beep periodically until signaled to stop. */
{
		/* Add an event handler. */
	if (!SetConsoleCtrlHandler(Handler, TRUE))
		ReportError(_T("Error setting event handler"), 1, TRUE);
	
	while (!exitFlag) { /* This flag is detected right after a beep, before a handler exits */
		Sleep(4750); /* Beep every 5 seconds; allowing 250 ms of beep time. */
		Beep(1000 /* Frequency */, 250 /* Duration */);
	}
	_tprintf(_T("Stopping the main program as requested.\n"));
	return 0;
}	

BOOL WINAPI Handler(DWORD cntrlEvent)
{
	switch (cntrlEvent) { 
		/* The signal timing will determine if you see the second handler message */
		case CTRL_C_EVENT:
			_tprintf(_T("Ctrl-C received by handler. Leaving in 5 seconds or less.\n"));
			exitFlag = TRUE;
			Sleep(4000); /* Decrease this time to get a different effect */
			_tprintf(_T("Leaving handler in 1 second or less.\n"));
			return TRUE; /* TRUE indicates that the signal was handled. */
		case CTRL_CLOSE_EVENT:
			_tprintf(_T("Close event received by handler. Leaving the handler in 5 seconds or less.\n"));
			exitFlag = TRUE;
			Sleep(4000); /* Decrease this time to get a different effect */
			_tprintf(_T("Leaving handler in 1 second or less.\n"));
			return TRUE; /* Try returning FALSE. Any difference? */
		default:
			_tprintf(_T("Event: %d received by handler. Leaving in 5 seconds or less.\n"), cntrlEvent);
			exitFlag = TRUE;
			Sleep(4000); /* Decrease this time to get a different effect */
			_tprintf(_T("Leaving handler in 1 seconds or less.\n"));
			return TRUE; /* TRUE indicates that the signal was handled. */
	}
}
