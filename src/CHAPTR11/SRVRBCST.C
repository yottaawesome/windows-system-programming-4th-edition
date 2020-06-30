/* Program 11-4. Mailslot client.
	Broadcast the pipe name to waiting command line clients
	using a mailslot. The mailslot is multiple readers
	(every client) and single (or multiple) writers (every server). */

#include "Everything.h"
#include "ClientServer.h"

int _tmain (int argc, LPTSTR argv [])
{
	BOOL exit;
	MS_MESSAGE mailSlotNotify;
	DWORD nXfer;
	HANDLE hMailSlot;

	if (!WindowsVersionOK (3, 1)) 
		ReportError (_T("This program requires Windows NT 3.1 or greater"), 1, FALSE);

	/* Open the mailslot for the MS "client" writer. */

	while (TRUE) {
		exit = FALSE;
		while (!exit) { /* Wait for a client to create a MS. */
			hMailSlot = CreateFile (MS_CLTNAME, GENERIC_WRITE | GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hMailSlot == INVALID_HANDLE_VALUE) {
				Sleep (5000);
			}
			else exit = TRUE;
	  }
		/* Send out the message to the mailslot. */
		/* Avoid an "information discovery" security exposure. */
		ZeroMemory(&mailSlotNotify, sizeof(mailSlotNotify));
		mailSlotNotify.msStatus = 0;
		mailSlotNotify.msUtilization = 0;

		_tcscpy (mailSlotNotify.msName, SERVER_PIPE);
		if (!WriteFile (hMailSlot, &mailSlotNotify, MSM_SIZE, &nXfer, NULL))
			ReportError (_T ("Server MS Write error."), 13, TRUE);
		CloseHandle (hMailSlot);

		/* Wait for another client to open a mailslot. */
	}

	/* Not reachable. */
	return(0);
}
