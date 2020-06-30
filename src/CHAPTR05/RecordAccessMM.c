// recordAccessMM.cpp : Defines the entry point for the console application.
//
//  Copyright 2004-2009, JMH Associates, Inc.
//
/*	Based on WSP3, Chapter 3. recordAccessMM - Memory mapped version. */
/*	Demonstrates simple record management in a file.
	The file is specified as a collection of an arbitrary
	number of records (The number is specified on the command
	line). Each record has a character string (limited length),
	times of creation and last update, and an update count.
	The file has an 8-byte header containing the total number
	of record slots and the total number of non-empty records.
	The user can interactively create, update, and modify
	records.
	Usage: recordAccessMM FileName [nrec [prompt]]
	If nrec is omitted, FileName must already exist. 
	If nrec is supplied and > 0, FileName is recreated (destroying any existing file)
	   and the program exits, having created an empty file.
    All user interaction is suppressed if prompt is specified - useful if input 
	   commands are redirected from a file, as in the performanc tests
	   [as in recordAccessTIME.bat]
	   
	If the number of records is large, a sparse file is recommended.
	The techniques here could also be used if a hash function
	determined the record location (this would be a simple
	prorgam enhancment). */

/* This program illustrates:
	1. Random file access.
	2. LARGE_INTEGER arithmetic and using the 64-bit file positions. 
	3. record update in place.
	4. File initialization to 0 (this will not work under Windows 9x
		or with a FAT file system. 
*/
#include "Everything.h"

#define STRING_SIZE 256
typedef struct _record { /* File record structure */
	DWORD			referenceCount;  /* 0 meands an empty record */
	SYSTEMTIME		recordCreationTime;
	SYSTEMTIME		recordLastRefernceTime;
	SYSTEMTIME		recordUpdateTime;
	TCHAR			dataString[STRING_SIZE];
} record;
typedef struct _header { /* File header descriptor */
	DWORD			numRecords;
	DWORD			numNonEmptyrecords;
} header;

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hFile;
	HANDLE hFileMap = INVALID_HANDLE_VALUE;
	char * pInFile = NULL;
	LARGE_INTEGER curPtr;
	DWORD openOption, nXfer, recNumber;
	record record;
	TCHAR String[STRING_SIZE], command, extra[2];
	OVERLAPPED ov = {0, 0, 0, 0, NULL}, ovZero = {0, 0, 0, 0, NULL};
	header header = {0, 0}; 
	SYSTEMTIME currentTime;
	BOOLEAN headerChange, recordChange;
    long recordSize = sizeof(record);
	long headerSize = sizeof(header);
	int prompt = (argc <= 3) ? 1 : 0;

	// This is useful to remind yourself if you built for 32 or 64 bits.
	// _tprintf (_T("Size of pointer: %d. Size of LARGE_INTEGER: %d.\n"), sizeof(pInFile), sizeof(curPtr));

	if (argc < 2) {
		_tprintf (_T("record size = %d.\n"), recordSize);
		ReportError (_T("Usage: recordAccessMM file [nrec [prompt]]"), 1, FALSE);
	}

	openOption = ((argc > 2 && _ttoi(argv[2]) <= 0) || argc <= 2) ? OPEN_EXISTING : CREATE_ALWAYS;
	hFile = CreateFile (argv [1], GENERIC_READ | GENERIC_WRITE,
		0, NULL, openOption, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		ReportError (_T("recordAccess error: Cannot open existing file."), 2, TRUE);

	if (argc >= 3 && _ttoi(argv[2]) > 0)  { /* Write the header (all header and pre-size the new file) */
#if 0
/* First, make the file sparse if possible - Recommended for large files */
/*      Challenge - get this to work properly - alternatively, make it sparse */
/*		adminstratively. This code, taken from several MS examples, fails on XP Home. */

		DWORD FsFlags = 0;
		if (!GetVolumeInformation (_T("C:\\")), NULL, 0, NULL, NULL, &FsFlags , NULL, 0))
			ReportError (_T("recordAccess Error: GetVolumeInformation."), 3, TRUE);
		if (FsFlags & FILE_SUPPORTS_SPARSE_FILES) {
			if ((FsFlags & FILE_SUPPORTS_SPARSE_FILES) &&
				!DeviceIoControl (hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &nXfer, NULL)) 
				ReportError (_T("recordAccess Error: Making new file sparse."), 0, TRUE);
		} 
		_tprintf (_T("Is it sparse? %x\n"), 
			GetFileAttributes(argv[1]) & FILE_ATTRIBUTE_SPARSE_FILE);
#endif
		header.numRecords = _ttoi(argv[2]);
		if (!WriteFile(hFile, &header, headerSize, &nXfer, &ovZero))
			ReportError (_T("recordAccess Error: WriteFile header."), 4, TRUE);
		
		curPtr.QuadPart = (LONGLONG)recordSize * _ttoi(argv[2]) + headerSize;
		if (!SetFilePointerEx (hFile, curPtr, NULL, FILE_BEGIN))
			ReportError (_T("recordAccess Error: Set Pointer."), 4, TRUE);
		if (!SetEndOfFile(hFile))
			ReportError (_T("recordAccess Error: Set End of File."), 5, TRUE);
		if (prompt) _tprintf (_T("Empty file with %d records created.\n"), header.numRecords);

#if 0  // We don't need this; the file is sparse, which zeros it.
        /* Map the file */
	    /* Create a file mapping object on the input file. Use the file size. */
	    hFileMap = CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	    if (hFileMap == NULL)
		    ReportException (_T("Failure Creating map."), 2);

    	pInFile = (char *)MapViewOfFile (hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	    if (pInFile == NULL)
		    ReportException (_T("Failure Mapping input file."), 3);
        memset(pInFile+headerSize, 0, header.numRecords * recordSize ); 
        
		UnmapViewOfFile(pInFile);
        CloseHandle(hFileMap);
#endif

	    CloseHandle(hFile);
	    return 0;
    }

	/* Read the file header to find the number of records and non-empty records */
	if (!ReadFile(hFile, &header, headerSize, &nXfer, &ovZero))
		ReportError (_T("recordAccess Error: ReadFile header."), 6, TRUE);
	if (prompt) _tprintf (_T("File %s contains %d non-empty records of size %d. Total capacity is: %d\n"),
		argv[1], header.numNonEmptyrecords, recordSize, header.numRecords);

    /* Map the file */
	/* Create a file mapping object on the input file. Use the file size. */
	hFileMap = CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hFileMap == NULL)
		ReportException (_T("Failure Creating map."), 2);
	pInFile = (char *)MapViewOfFile (hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pInFile == NULL)
		ReportException (_T("Failure Mapping input file."), 3);


	/* Prompt the user to read or write a numbered record */
	while (TRUE) {
		headerChange = FALSE; recordChange = FALSE;
		if (prompt) _tprintf (_T("Enter r(ead)/w(rite)/d(elete)/qu(it) record#\n"));
		_tscanf (_T("%1c%d%1c"), &command, &recNumber, &extra);
		if (command == _T('q')) break;
		if (recNumber >= header.numRecords) { 
			if (prompt) _tprintf (_T("record Number is too large. Try again.\n"));
			continue;
		}
		curPtr.QuadPart = (LONGLONG)recNumber * recordSize + headerSize;

		if (prompt) _tprintf (_T("command: %c: "), command, _T(" record Number: %d.\n"), recNumber);
        
		// Read the current record contents
		memcpy (&record, pInFile+curPtr.QuadPart, recordSize);

		GetSystemTime (&currentTime); /* Use to update record time fields */
		record.recordLastRefernceTime = currentTime;
		if (command == _T('r') || command == _T('d')) { /* Report record contents, if any */
			if (prompt) _tprintf (_T("About record number: %d.\n"), recNumber);
			if (record.referenceCount == 0) {
				if (prompt) _tprintf (_T("record Number %d is empty.\n"), recNumber);
				continue;
			} else {
				if (prompt) _tprintf (_T("record Number %d. Reference Count: %d \n"), 
					recNumber, record.referenceCount);
				if (prompt) _tprintf (_T("Data: %s\n"), record.dataString);
				/* Exercise: Display times. See ls.c for an example */
				recordChange = TRUE;
			}
			if (command == _T('d')) { /* Delete the record */
				/* Challenge: In a sparse file, implement this with the
				   FSCTL_SET_ZERO_DATA flag on DeviceIoControl() */
				if (prompt) _tprintf (_T("About to delete this record.\n"));
				record.referenceCount = 0;
				header.numNonEmptyrecords--;
				headerChange = TRUE;
				recordChange = TRUE;
			}
		} else if (command == _T('w')) { /* Write the record, even if for the first time */
			if (prompt) _tprintf (_T("Enter new data string for the record.\n"));
			_fgetts (String, sizeof(String), stdin); // Don't use _getts (potential buffer overflow)
			String[_tcslen(String)-1] = _T('\0'); // remove the newline character
			if (record.referenceCount == 0) {
				record.recordCreationTime = currentTime;
				header.numNonEmptyrecords++;
				headerChange = TRUE;
			}
			record.recordUpdateTime = currentTime;
			record.referenceCount++;
			_tcsncpy_s (record.dataString, sizeof(record.dataString), String, STRING_SIZE-1);
			recordChange = TRUE;
			if (prompt) _tprintf (_T("Wrote record number: %d.\n"), recNumber);
			if (prompt) _tprintf (_T("Data: %s.\n"), String);
		} else {
			if (prompt) _tprintf (_T("command must be r, w, or d. Try again.\n"));
		}
		/* Update the record in place if any record contents have changed. */
		if (recordChange) {
			memcpy (pInFile+curPtr.QuadPart, &record, recordSize );//    !WriteFile (hFile, &record, sizeof (record), &nXfer, &ov))
			if (prompt) _tprintf (_T("Updated record Number: %d at location %ud. Reference count: %d\n"), recNumber, curPtr.QuadPart, record.referenceCount);
		}
		/* Update the number of non-empty records if required */
		if (headerChange) {
			memcpy (pInFile, &header, headerSize);
			if (prompt) _tprintf (_T("Updated header. Numuber Nonempty records = %d.\n"), header.numNonEmptyrecords);
		}
	}

	if (prompt) _tprintf (_T("Computed number of non-empty records is: %d\n"), header.numNonEmptyrecords);
	if (!ReadFile(hFile, &header, headerSize, &nXfer, &ovZero))
		ReportError (_T("recordAccess Error: ReadFile header."), 10, TRUE);
	if (prompt) _tprintf (_T("File %s NOW contains %d non-empty records. Total capacity is: %d\n"),
		argv[1], header.numNonEmptyrecords, header.numRecords);

	if (NULL != pInFile) UnmapViewOfFile(pInFile);
	if (INVALID_HANDLE_VALUE != hFileMap) CloseHandle (hFileMap);
	CloseHandle (hFile);
	return 0;
}
