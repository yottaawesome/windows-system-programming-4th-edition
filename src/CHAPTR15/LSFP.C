/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Code in text starts at ProcessItem. */
/* BEGIN BOILERPLATE CODE. */

/* Chapter 15. lsFP file list command with File Permissions interpreted as
	UNIX file permissions (previously set with chmod).
	This is extension of the ls command in Chapter 4. 
	CAUTION: Permissions and owner information will be probably be nonsense
 *	for files and directories whose permissions were not set with "chmodW" */
/*	lsFP[options][files] */

/* Chapter 15. lsFP file list command. */
/*	lsFP[options][files] */

/* List the attributes of one or more files.
	Options:
		-R	recursive
		-l	dashL listing (permissions, owner, size and time).
			Depending on the ProcessItem function, this will
			also list the owner and permissions. */

/* Thanks to Stewart N. Weiss for making some significant corrections
 * in processing the path name (there were problems processing C:, C:\,
 * and C:\dirname
 */

#include "Everything.h"

BOOL TraverseDirectory (LPTSTR, DWORD, LPBOOL);
DWORD FileType (LPWIN32_FIND_DATA);
BOOL ProcessItem (LPWIN32_FIND_DATA, DWORD, LPBOOL);

int _tmain (int argc, LPTSTR argv[])
{
	BOOL flags[MAX_OPTIONS], ok = TRUE;
	TCHAR pathName[MAX_PATH + 1], currPath[MAX_PATH + 1], tempPath[MAX_PATH+1];
	LPTSTR pSlash, pFileName;
	int i, fileIndex;

	fileIndex = Options (argc, argv, _T ("Rl"), &flags[0], &flags[1], NULL);

	/* "Parse" the search pattern into two parts:
		the "parent" and the file name or wild card expression.
		The file name is the dashLest suffix not containing a slash.
		The parent is the remaining prefix with the slash.
		This is performed for all command line search patterns.
		If no file is specified, use * as the search pattern. */

	GetCurrentDirectory (MAX_PATH, currPath);
	if (argc < fileIndex + 1) 
		ok = TraverseDirectory (_T("*"), MAX_OPTIONS, flags);
	else for (i = fileIndex; i < argc; i++) {
		_tcscpy (pathName, argv[i]);
		_tcscpy(tempPath, argv[i]);                

		/* Find the rightmost slash, if any.
			Set the path and use the rest as the file name. */

		pSlash = _tstrrchr (tempPath, _T('\\'));       

		if (pSlash != NULL) {
			*pSlash = _T('\0');
			_tcscat(tempPath, _T("\\"));           
			SetCurrentDirectory (tempPath); /* Now restore pathName. */
			pSlash = _tstrrchr (pathName, _T('\\'));   
			pFileName = pSlash + 1;
		} else pFileName = pathName;
		ok = TraverseDirectory (pFileName, MAX_OPTIONS, flags) && ok;
		SetCurrentDirectory (currPath);
	}
	return ok ? 0 : 1;
}

static BOOL TraverseDirectory (LPTSTR pathName, DWORD numberFlags, LPBOOL flags) 

/* Traverse a directory - Carrying out an implementation specific "action"
	for every name encountered.
	The action in this version is "list, with optional attributes". */

/* pathName: Relative or absolute pathName to traverse. */
{
	HANDLE searchHandle;
	WIN32_FIND_DATA findData;
	BOOL recursive = flags[0];
	DWORD fileType, iPass;
	/****  Having a large array such as currPath on the stack is a bad idea 
	 *  or, if you prefer, an extemely poor programming practice (I plead guilty) ****/
	/* 1) It can consume lots of space as you traverse the directory
	 * 2) You risk a stack overflow, which is a security risk
	 * 3) You cannot deal with long paths (> MAX_PATH), using the \\?\ prefix
	 *    SUGGESTION: See lsW (Chapter 3) for a better implementation and fix this program.
	 */

	TCHAR currPath[MAX_PATH + 2];

	// added by SNW:
	if ( _tcslen(pathName) == 0 ) 
	{
		_tprintf(_T("Usage: lsFP[options][pathName]\n"));
		_tprintf(_T("       where[pathName] must not end in '\\'\n"));
		return FALSE;
	}
    // end of new stuff added by SNW

	GetCurrentDirectory (MAX_PATH, currPath);

	/* Open up the directory search handle and get the
		first file name to satisfy the path name. Make two passes.
		The first processes the files and the second processes the directories. */

	for (iPass = 1; iPass <= 2; iPass++) {
		// if path is of the form "X:" or "X:\" change to "X:\*"
		if (_istalpha(pathName[0]) &&  (pathName[1] == _T(':')) && (_tcslen(pathName) <= 3))
		{
			TCHAR   tempPath[5] = {_T('\0'), _T('\0') };;
			tempPath[0] = pathName[0];
			_tcscat (tempPath, _T(":\\*"));
			searchHandle = FindFirstFile (tempPath, &findData);
		}
		else
			searchHandle = FindFirstFile (pathName, &findData);

		if (searchHandle == INVALID_HANDLE_VALUE) {
			ReportError (_T ("Error opening Search Handle."), 0, TRUE);
			return FALSE;
		}

		/* Scan the directory and its subdirectories
			for files satisfying the pattern. */

		do {

		/* For each file located, get the type.
			List everything on pass 1.
			On pass 2, display the directory name
			and recursively process the subdirectory contents,
			if the recursive option is set. */

			fileType = FileType (&findData);
			if (iPass == 1) /* ProcessItem is "print attributes". */
				ProcessItem (&findData, MAX_OPTIONS, flags);

			/* Traverse the subdirectory on the second pass. */

			if (fileType == TYPE_DIR && iPass == 2 && recursive) {
				_tprintf (_T ("%s\\%s:\n"), currPath, findData.cFileName);
				SetCurrentDirectory (findData.cFileName);
				TraverseDirectory (_T("*"), numberFlags, flags);
				SetCurrentDirectory (_T (".."));
			}

			/* Get the next file or directory name. */

		} while (FindNextFile (searchHandle, &findData));
		
		FindClose (searchHandle);
	}
	return TRUE;
}

static DWORD FileType (LPWIN32_FIND_DATA pFileData)

/* Return file type from the find data structure.
	Types supported:
		TYPE_FILE:	If this is a file
		TYPE_DIR:	If this is a directory other than . or ..
		TYPE_DOT:	If this is . or .. directory */
{
	BOOL isDir;
	DWORD fileType;
	fileType = TYPE_FILE;
	isDir = (pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (isDir)
		if (lstrcmp (pFileData->cFileName, _T(".")) == 0
				|| lstrcmp (pFileData->cFileName, _T("..")) == 0)
			fileType = TYPE_DOT;
		else fileType = TYPE_DIR;
	return fileType;
}


/*	Item processing function called every time a file or directory item is located. */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* END OF BOILERPLATE CODE. */
/* Code in text starts here. */

static BOOL ProcessItem (LPWIN32_FIND_DATA pFileData, DWORD numberFlags, LPBOOL flags)

/* List attributes, with file permissions and owner. */
{
	DWORD fileType = FileType(pFileData), Mode, i;
	BOOL dashL = flags[1];
	TCHAR groupName[ACCT_NAME_SIZE], userName[ACCT_NAME_SIZE];
	SYSTEMTIME timeLastWrite;
	TCHAR  permissionString[] = _T("---------");
	const TCHAR RWX[] = {_T('r'),_T('w'),_T('x')}, FileTypeChar[] = {_T(' '),_T('d')};

	if (fileType != TYPE_FILE && fileType != TYPE_DIR)
		return FALSE;
	_tprintf (_T ("\n"));

	if (dashL) {
		Mode = ReadFilePermissions (pFileData->cFileName, userName, groupName);
		if (Mode == 0xFFFFFFFF)
			Mode = 0;
		for (i = 0; i < 9; i++) {
			if ( (Mode / (1 << (8 - i)) & 0x1) )
				 permissionString[i] = RWX[i % 3];
		}
		_tprintf (_T ("%c%s %8.7s %8.7s%10d"),
			FileTypeChar[fileType-1], permissionString, userName, groupName,
			pFileData->nFileSizeLow);
		FileTimeToSystemTime (&(pFileData->ftLastWriteTime), &timeLastWrite);
		_tprintf (_T (" %02d/%02d/%04d %02d:%02d:%02d"),
				timeLastWrite.wMonth, timeLastWrite.wDay,
				timeLastWrite.wYear, timeLastWrite.wHour,
				timeLastWrite.wMinute, timeLastWrite.wSecond);
	}
	_tprintf (_T (" %s"), pFileData->cFileName);
	return TRUE;
}
