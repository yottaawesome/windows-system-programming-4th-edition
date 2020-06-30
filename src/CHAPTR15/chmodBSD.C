/* Chapter 7. chmodBSD command. */

/* chmod [options] mode file [GroupName]
	Update access rights of the named file.
	Options:
		-f  Force - do not complain if unable to change.
		-c  Create the file if it does not exist.
			NOT AN OPTION FOR THE STANDARD UNIX COMMAND. 
			The group name is after the file name! 
			If the group name is not given, it is taken from the
			process's token.  */

/* This program illustrates:
	1.  Setting the file security attributes.
	2.  Changing a security descriptor. 
	3.	Using BuildSecurityDescriptor, available in VC++ Ver 5.0*/

#include "Everything.h"

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hFile;
	BOOL Force, CreateNew, Change, Exists;
	DWORD Mode, DecMode, UsrCnt = ACCT_NAME_SIZE;
	TCHAR UsrNam [ACCT_NAME_SIZE];
	int FileIndex, GrpIndex, ModeIndex;
		/* Array of rights settings in "UNIX order". */
	DWORD AceMasks [] =
		{GENERIC_READ, GENERIC_WRITE, GENERIC_EXECUTE};
	SECURITY_ATTRIBUTES Sa = { sizeof (SECURITY_ATTRIBUTES), NULL, FALSE };
	LPCTSTR GroupName = NULL;

	ModeIndex = Options (argc, argv, _T ("fc"), &Force, &CreateNew, NULL);
	GrpIndex = ModeIndex + 2;
	FileIndex = ModeIndex + 1;
	DecMode = _ttoi (argv [ModeIndex]);

	/* Decimal - Assume every digit is between 0 and 7 and convert. */

	Mode = ((DecMode / 100) % 10) * 64 
			 + ((DecMode / 10) % 10) * 8 + (DecMode % 10);
	Exists = (_taccess (argv [FileIndex], 0) == 0);

	if (!Exists && CreateNew) {
				/* File does not exist; create a new one. */
		if (argc < GrpIndex) 
			ReportError (_T ("Usage: Chmod -c Perm file [GroupName]."), 1, FALSE);
		if (argc == GrpIndex) { /* Find the primary group name */
			GroupName = NULL;
		} else
			GroupName = argv[GrpIndex];

		if (!GetUserName (UsrNam, &UsrCnt))
			ReportError (_T ("Failed to get user name."), 2, TRUE);
		Sa.lpSecurityDescriptor = InitializeSD (Mode, UsrNam, argv [GrpIndex], AceMasks);
		if (Sa.lpSecurityDescriptor == NULL)
			ReportError (_T ("Failure setting security attributes."), 3, TRUE);
		hFile = CreateFile (argv [FileIndex], 0,
				0, &Sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			ReportError (_T ("Failure creating secure file."), 4, TRUE);
		CloseHandle (hFile);
	}
	else if (Exists) {
				/* File exists; change permissions. */
		Change = ChangeFilePermissions (Mode, argv [FileIndex], AceMasks);
		if (!Change && !Force)
			ReportError (_T ("Change Permissions error."), 5, TRUE);
	}
	else /* Error - File does not exist and no -c option. */
		ReportError (_T ("File to change does not exist."), 6, 0);
	
	return 0;
}


