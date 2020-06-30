#include "Everything.h"

VOID ReportError (LPCTSTR userMessage, DWORD exitCode, BOOL printErrorMessage)

/* General-purpose function for reporting system errors.
	Obtain the error number and convert it to the system error message.
	Display this information and the user-specified message to the standard error device.
	userMessage:		Message to be displayed to standard error device.
	exitCode:		0 - Return.
					> 0 - ExitProcess with this code.
	printErrorMessage:	Display the last system error message if this flag is set. */
{
	DWORD eMsgLen, errNum = GetLastError ();
	LPTSTR lpvSysMsg;
	_ftprintf (stderr, _T("%s\n"), userMessage);
	if (printErrorMessage) {
		eMsgLen = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, errNum, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpvSysMsg, 0, NULL);
		if (eMsgLen > 0)
		{
			_ftprintf (stderr, _T("%s\n"), lpvSysMsg);
		}
		else
		{
			_ftprintf (stderr, _T("Last Error Number; %d.\n"), errNum);
		}

		if (lpvSysMsg != NULL) LocalFree (lpvSysMsg); /* Explained in Chapter 5. */
	}

	if (exitCode > 0)
		ExitProcess (exitCode);

	return;
}

/* Extension of ReportError to generate a user exception
	code rather than terminating the process. */

VOID ReportException (LPCTSTR userMessage, DWORD exceptionCode)
			
/* Report as a non-fatal error.
	Print the system error message only if the message is non-null. */
{	
	if (lstrlen (userMessage) > 0)
		ReportError (userMessage, 0, TRUE);
			/* If fatal, raise an exception. */

	if (exceptionCode != 0) 
		RaiseException (
			(0x0FFFFFFF & exceptionCode) | 0xE0000000, 0, 0, NULL);
	
	return;
}