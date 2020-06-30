/* PrintMsg.c:
	ConsolePrompt
	PrintStrings
	PrintMsg */

#include "Everything.h"
#include <stdarg.h>	 									  

BOOL PrintStrings (HANDLE hOut, ...)

/* Write the messages to the output handle. Frequently hOut
	will be standard out or error, but this is not required.
	Use WriteConsole (to handle Unicode) first, as the
	output will normally be the console. If that fails, use WriteFile.

	hOut:	Handle for output file. 
	... :	Variable argument list containing TCHAR strings.
		The list must be terminated with NULL. */
{
	DWORD msgLen, count;
	LPCTSTR pMsg;
	va_list pMsgList;	/* Current message string. */
	va_start (pMsgList, hOut);	/* Start processing msgs. */
	while ((pMsg = va_arg (pMsgList, LPCTSTR)) != NULL) {
		msgLen = lstrlen (pMsg);
		if (!WriteConsole (hOut, pMsg, msgLen, &count, NULL)
				&& !WriteFile (hOut, pMsg, msgLen * sizeof (TCHAR),	&count, NULL)) {
			va_end (pMsgList);
			return FALSE;
		}
	}
	va_end (pMsgList);
	return TRUE;
}


BOOL PrintMsg (HANDLE hOut, LPCTSTR pMsg)

/* For convenience only - Single message version of PrintStrings so that
	you do not have to remember the NULL arg list terminator.

	hOut:	Handle of output file
	pMsg:	Single message to output. */
{
	return PrintStrings (hOut, pMsg, NULL);
}


BOOL ConsolePrompt (LPCTSTR pPromptMsg, LPTSTR pResponse, DWORD maxChar, BOOL echo)

/* Prompt the user at the console and get a response
	which can be up to maxChar generic characters.

	pPromptMessage:	Message displayed to user.
	pResponse:	Programmer-supplied buffer that receives the response.
	maxChar:	Maximum size of the user buffer, measured as generic characters.
	echo:		Do not display the user's response if this flag is FALSE. */
{
	HANDLE hIn, hOut;
	DWORD charIn, echoFlag;
	BOOL success;
	hIn = CreateFile (_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, 0,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	hOut = CreateFile (_T("CONOUT$"), GENERIC_WRITE, 0,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	/* Should the input be echoed? */
	echoFlag = echo ? ENABLE_ECHO_INPUT : 0;

	/* API "and" chain. If any test or system call fails, the
		rest of the expression is not evaluated, and the
		subsequent functions are not called. GetStdError ()
		will return the result of the failed call. */

	success =  SetConsoleMode (hIn, ENABLE_LINE_INPUT | echoFlag | ENABLE_PROCESSED_INPUT)
			&& SetConsoleMode (hOut, ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_PROCESSED_OUTPUT)
			&& PrintStrings (hOut, pPromptMsg, NULL)
			&& ReadConsole (hIn, pResponse, maxChar - 2, &charIn, NULL);
	
	/* Replace the CR-LF by the null character. */
	if (success)
		pResponse [charIn - 2] = _T('\0');
	else
		ReportError (_T("ConsolePrompt failure."), 0, TRUE);

	CloseHandle (hIn);
	CloseHandle (hOut);
	return success;
}

