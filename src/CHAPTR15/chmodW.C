/* Chapter 15-1. Windows chmodW command (Windows-specific name to
 * distinguish from any Unix emulation system you may have installed,
 * such as cygwin. */

/* chmodW [options] mode file [groupName]
	Update access rights of the named file.
	Options:
		-f  force - do not complain if unable to change.
		-c  Create the file if it does not exist.
			NOT AN OPTION FOR THE STANDARD UNIX COMMAND. 
			The group name is after the file name! 
			If the group name is not given, it is taken from the
			process's token.  */

/* This program illustrates:
	1.  Setting the file security attributes.
	2.  Changing a security descriptor. */

#include "Everything.h"

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hFile, heap = NULL;
	BOOL force, createNew, change, exists;
	DWORD mode, userCount = ACCT_NAME_SIZE;
	TCHAR userName [ACCT_NAME_SIZE];
	int fileIndex, groupIndex, modeIndex;
		/* Array of rights settings in "UNIX order". */
		/* There are distinct arrays for the allowed and denied
		 * ACE masks so that SYNCHRONIZE is never denied.
		 * This is necessary as both FILE_GENERIC_READ and FILE_GENERIC_WRITE
		 * include SYNCHRONIZE access (see WINNT.H).
		 */
	DWORD allowedAceMasks [] =
		{FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE};
	DWORD deniedAceMasks [] =
		{FILE_GENERIC_READ & ~SYNCHRONIZE, FILE_GENERIC_WRITE & ~SYNCHRONIZE, 
		 FILE_GENERIC_EXECUTE & ~SYNCHRONIZE};
	LPSECURITY_ATTRIBUTES pSa = NULL;
	LPCTSTR groupName = NULL;

	if (argc < 3) ReportError (_T("Usage: chmodW [options] mode file [groupName]"), 1, FALSE);
	modeIndex = Options (argc, argv, _T ("fc"), &force, &createNew, NULL);
	groupIndex = modeIndex + 2;
	fileIndex = modeIndex + 1;
	/*	Mode is base 8, convert to decimal */
	mode = _tcstoul(argv [modeIndex], NULL, 8);

	exists = (_taccess (argv [fileIndex], 0) == 0);

	if (!exists && createNew) {
				/* File does not exist; create a new one. */
		if (argc < groupIndex) 
			ReportError (_T ("Usage: chmod -c Perm file [groupName]."), 1, FALSE);
		if (argc == groupIndex) { /* Find the primary group name */
			groupName = NULL;
		} else
			groupName = argv[groupIndex];

		if (!GetUserName (userName, &userCount))
			ReportError (_T ("Failed to get user name."), 2, TRUE);
		/* Exercise: The trouble with GetUserName is that it will return the name
		 * of an impersonator. Instead, get the user SID from current token */

		pSa = InitializeUnixSA (mode, userName, groupName, allowedAceMasks, deniedAceMasks, &heap);

		if (pSa == NULL)
			ReportError (_T ("Failure setting security attributes."), 3, TRUE);
		hFile = CreateFile (argv [fileIndex], 0,
				0, pSa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			ReportError (_T ("Failure creating secure file."), 4, TRUE);
			DestroyUnixSA (pSa, heap);
		}
		CloseHandle (hFile);
	}
	else if (exists) {
				/* File exists; change permissions. */
		change = ChangeFilePermissions (mode, argv [fileIndex], allowedAceMasks,
			deniedAceMasks);
		if (!change && !force) {
			ReportError (_T ("change Permissions error."), 5, TRUE);
			DestroyUnixSA (pSa, heap);
		}
	} else { /* Error - File does not exist and no -c option. */
		ReportError (_T ("File to change does not exist."), 6, 0);
		DestroyUnixSA (pSa, heap);
	}
	DestroyUnixSA (pSa, heap);
	return 0;
}
