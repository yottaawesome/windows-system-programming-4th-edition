/* Chapter 3. lsREG - Registry list command. */
/* lsREG[options] SubKey 
	List the key-value pairs.
	Options:
		-R	recursive
		-l  List extended information; namely, the last write time
			and the value type					*/

/* This program illustrates:
		1.	Registry handles and traversal
		2.	Registry values
		3.	The similarity and differences between directory
			and registry traversal 

	Note that there are no wild card file names and you specify the
	subkey, with all key-value pairs being listed. This is similar to 
	ls with "SubKey\*" as the file specifier             */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "Everything.h"

BOOL TraverseRegistry(HKEY, LPTSTR, LPTSTR, LPBOOL);
BOOL DisplayPair(LPTSTR, DWORD, LPBYTE, DWORD, LPBOOL);
BOOL DisplaySubKey(LPTSTR, LPTSTR, PFILETIME, LPBOOL);

int _tmain(int argc, LPTSTR argv[])
{
	BOOL flags[2], ok = TRUE;
	TCHAR keyName[MAX_PATH+1];
	LPTSTR pScan;
	DWORD i, keyIndex;
	HKEY hKey, hNextKey;

	/* Tables of predefined key names and keys */
	LPTSTR PreDefKeyNames[] = {
		_T("HKEY_LOCAL_MACHINE"),
		_T("HKEY_CLASSES_ROOT"),
		_T("HKEY_CURRENT_USER"),
		_T("HKEY_CURRENT_CONFIG"),
		NULL };
	HKEY PreDefKeys[] = {
		HKEY_LOCAL_MACHINE,
		HKEY_CLASSES_ROOT,
		HKEY_CURRENT_USER,
		HKEY_CURRENT_CONFIG };

		if (argc < 2) {
			_tprintf(_T("Usage: lsREG[options] SubKey\n"));
			return 1;
		}

	keyIndex = Options(argc, argv, _T("Rl"), &flags[0], &flags[1], NULL);

	/* "Parse" the search pattern into two parts: the "key"
		and the "subkey". The key is the first back-slash terminated
		string and must be one of HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT,
		or HKEY_CURRENT_USER. The subkey is everything else.
		The key and subkey names will be copied from argv[keyIndex]. */

	/*  Build the Key */
	pScan = argv[keyIndex];
	for (i = 0; *pScan != _T('\\') && *pScan != _T('\0') && i < MAX_PATH; pScan++, i++)
		keyName[i] = *pScan;
	keyName[i] = _T('\0'); if (*pScan == _T('\\')) pScan++;

	/* Translate predefined key name to an HKEY */
	for (	i = 0;
			PreDefKeyNames[i] != NULL && _tcscmp(PreDefKeyNames[i], keyName) != 0;
			i++) ;
	if (PreDefKeyNames[i] == NULL) ReportError(_T("Use a Predefined Key"), 1, FALSE);
	hKey = PreDefKeys[i];

	/*  pScan points to the start of the subkey string. It is not directly
		documented that \ is the separator, but it works fine */
	if (RegOpenKeyEx(hKey, pScan, 0, KEY_READ, &hNextKey) != ERROR_SUCCESS)
		ReportError(_T("Cannot open subkey properly"), 2, TRUE);
	hKey = hNextKey;

	ok = TraverseRegistry(hKey, argv[keyIndex], NULL, flags);
	RegCloseKey(hKey);

	return ok ? 0 : 1;
}

BOOL TraverseRegistry(HKEY hKey, LPTSTR fullKeyName, LPTSTR subKey, LPBOOL flags)

/*	Traverse a registry key, listing the key-value pairs and
	traversing subkeys if the -R option is set. 
	fullKeyName is a "full key name" starting with one of the open key
	names, such as "HKEY_LOCAL_MACHINE".
	SubKey, which can be null, is the rest of the path to be traversed. */

{
	HKEY hSubKey;
	BOOL recursive = flags[0];
	LONG result;
	DWORD valueType, index;
	DWORD numSubKeys, maxSubKeyLen, numValues, maxValueNameLen, maxValueLen;
	DWORD subKeyNameLen, valueNameLen, valueLen;
	FILETIME lastWriteTime;
	LPTSTR subKeyName, valueName;
	LPBYTE value;
	/****  Having a large array such as fullSubKeyName on the stack is a bad idea 
	 *  or, if you prefer, an extemely poor programming practice, even a crime (I plead no contest) ****/
	/* 1) It can consume lots of space as you traverse the directory
	 * 2) You risk a stack overflow, which is a security risk
	 * 3) You cannot deal with long paths (> MAX_PATH), using the \\?\ prefix
	 *    SUGGESTION: See lsW (Chapter 3) for a better implementation and fix this program accordingly.
	 */

	TCHAR fullSubKeyName[MAX_PATH+1];

	/* Open up the key handle. */

	if (RegOpenKeyEx(hKey, subKey, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
		ReportError(_T("\nCannot open subkey"), 2, TRUE);

	/*  Find max size info regarding the key and allocate storage */
	if (RegQueryInfoKey(hSubKey, NULL, NULL, NULL, 
		&numSubKeys, &maxSubKeyLen, NULL, 
		&numValues, &maxValueNameLen, &maxValueLen, 
		NULL, &lastWriteTime) != ERROR_SUCCESS)
			ReportError(_T("Cannot query subkey information"), 3, TRUE);
	subKeyName = malloc(TSIZE * (maxSubKeyLen+1));   /* size in bytes */
	valueName  = malloc(TSIZE * (maxValueNameLen+1));
	value      = malloc(maxValueLen);      /* size in bytes */

	/*  First pass for key-value pairs. */
	/*  Important assumption: No one edits the registry under this subkey */
	/*  during this loop. Doing so could change add new values */
	for (	index = 0; index < numValues; index++) {
		valueNameLen = maxValueNameLen + 1; /* A very common bug is to forget to set */
		valueLen     = maxValueLen + 1;     /* these values; both are in/out params  */
		result = RegEnumValue(hSubKey, index, valueName, &valueNameLen, NULL,
				&valueType, value, &valueLen);
		if (result == ERROR_SUCCESS && GetLastError() == 0)
			DisplayPair(valueName, valueType, value, valueLen, flags);
		/*  If you wanted to change a value, this would be the place to do it.
			RegSetValueEx(hSubKey, valueName, 0, valueType, pNewValue, NewValueSize); */
	}

	/*  Second pass for subkeys */
	for (index = 0; index < numSubKeys; index++) {
		subKeyNameLen = maxSubKeyLen + 1;
		result = RegEnumKeyEx(hSubKey, index, subKeyName, &subKeyNameLen, NULL,
				NULL, NULL, &lastWriteTime);
		if (GetLastError() == 0) {
			DisplaySubKey(fullKeyName, subKeyName, &lastWriteTime, flags);

			/*  Display subkey components if -R is specified */
			if (recursive) {
				_stprintf(fullSubKeyName, _T("%s\\%s"), fullKeyName, subKeyName);
				TraverseRegistry(hSubKey, fullSubKeyName, subKeyName, flags);
			}
		}
	}

	_tprintf(_T("\n"));
	free(subKeyName); 
	free(valueName);
	free(value);
	RegCloseKey(hSubKey);
	return TRUE;
}


BOOL DisplayPair(LPTSTR valueName, DWORD valueType,
						 LPBYTE value, DWORD valueLen,
						 LPBOOL flags)

/* Function to display key-value pairs. */

{

	LPBYTE pV = value;
	DWORD i;

	_tprintf(_T("\n%s = "), valueName);
	switch (valueType) {
	case REG_FULL_RESOURCE_DESCRIPTOR: /* 9: Resource list in the hardware description */
	case REG_BINARY: /*  3: Binary data in any form. */ 
		for (i = 0; i < valueLen; i++, pV++)
			_tprintf(_T(" %x"), *pV);
		break;

	case REG_DWORD: /* 4: A 32-bit number. */
		_tprintf(_T("%x"), (DWORD)*value);
		break;

	case REG_EXPAND_SZ: /* 2: null-terminated string with unexpanded references to environment variables (for example, “%PATH%”). */ 
	case REG_MULTI_SZ: /* 7: An array of null-terminated strings, terminated by two null characters. */
	case REG_SZ: /* 1: A null-terminated string. */ 
		_tprintf(_T("%s"), (LPTSTR)value);
		break;
	
	case REG_DWORD_BIG_ENDIAN: /* 5:  A 32-bit number in big-endian format. */ 
	case REG_LINK: /* 6: A Unicode symbolic link. */
	case REG_NONE: /* 0: No defined value type. */
	case REG_RESOURCE_LIST: /* 8: A device-driver resource list. */
 	default: _tprintf(_T(" ** Cannot display value of type: %d. Exercise for reader\n"), valueType);
		break;
	}

	return TRUE;
}

BOOL DisplaySubKey(LPTSTR keyName, LPTSTR subKeyName, PFILETIME pLastWrite, LPBOOL flags)
{
	BOOL longList = flags[1];
	SYSTEMTIME sysLastWrite;

	_tprintf(_T("\n%s"), keyName);
	if (_tcslen(subKeyName) > 0) _tprintf(_T("\\%s "), subKeyName);
	if (longList) {
		FileTimeToSystemTime(pLastWrite, &sysLastWrite);
		_tprintf(_T("	%02d/%02d/%04d %02d:%02d:%02d"),
				sysLastWrite.wMonth, sysLastWrite.wDay,
				sysLastWrite.wYear, sysLastWrite.wHour,
				sysLastWrite.wMinute, sysLastWrite.wSecond);
	}
	return TRUE;
}
