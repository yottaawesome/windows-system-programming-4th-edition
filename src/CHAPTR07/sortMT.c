/* Chapter 7 SortMT. Work crew model
	File sorting with multiple threads and merge sort.
	sortMT [options] nt file. Work crew model.  */

/*  This program is based on sortHP.
	It allocates a block for the sort file, reads the file,
	then creates a "nt" threads (if 0, use the number of
	processors) so sort a piece of the file. It then
	merges the results in pairs. */

/* LIMITATIONS:
	1.	The number of threads must be a power of 2
	2.	The number of 64-byte records must be a multiple
		of the number of threads.
	An exercise asks you to remove these limitations. */

#include "Everything.h"

/* Definitions of the record structure in the sort file. */

#define DATALEN 56  /* Correct length for presdnts.txt and monarchs.txt. */
#define KEYLEN 8
typedef struct _RECORD {
	TCHAR key[KEYLEN];
	TCHAR data[DATALEN];
} RECORD;
#define RECSIZE sizeof (RECORD)
typedef RECORD * LPRECORD;

typedef struct _THREADARG {	/* Thread argument */
	DWORD iTh;		/* Thread number: 0, 1, 3, ... */
	LPRECORD lowRecord;	/* Low Record */
	LPRECORD highRecord;	/* High record */
} THREADARG, *PTHREADARG;

static DWORD WINAPI SortThread (PTHREADARG pThArg);
static int KeyCompare (LPCTSTR, LPCTSTR);

static DWORD nRec;	/* Total number of records to be sorted. */
static HANDLE * pThreadHandle;

int _tmain (int argc, LPTSTR argv[])
{
	/* The file is the first argument. Sorting is done in place. */
	/* Sorting is done in memory heaps. */

	HANDLE hFile, mHandle;
	LPRECORD pRecords = NULL;
	DWORD lowRecordNum, nRecTh,numFiles, iTh;
	LARGE_INTEGER fileSize;
	BOOL noPrint;
	int iFF, iNP;
	PTHREADARG threadArg;
	LPTSTR stringEnd;

	iNP = Options (argc, argv, _T("n"), &noPrint, NULL);
	iFF = iNP + 1;
	numFiles = _ttoi(argv[iNP]);

	if (argc <= iFF)
		ReportError (_T ("Usage: sortMT [options] nTh files."), 1, FALSE);

	/* Open the file and map it */
	hFile = CreateFile (argv[iFF], GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		ReportError (_T ("Failure to open input file."), 2, TRUE);
	// For technical reasons, we need to add bytes to the end.
	/* SetFilePointer is convenient as it's a short addition from the file end */
	if (!SetFilePointer(hFile, 2, 0, FILE_END) || !SetEndOfFile(hFile))
		ReportError (_T ("Failure position extend input file."), 3, TRUE);
	
	mHandle = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (NULL == mHandle)
		ReportError (_T ("Failure to create mapping handle on input file."), 4, TRUE);
	
	/* Get the file size. */
	if (!GetFileSizeEx (hFile, &fileSize))
		ReportError (_T ("Error getting file size."), 5, TRUE);

	nRec = (DWORD)fileSize.QuadPart / RECSIZE;	/* Total number of records. Note assumed limit */
	nRecTh = nRec / numFiles;	/* Records per thread. */
	threadArg = malloc (numFiles * sizeof (THREADARG));	/* Array of thread args. */ 
	pThreadHandle = malloc (numFiles * sizeof (HANDLE));
	
	/* Map the entire file */
	pRecords = MapViewOfFile(mHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0); 
	if (NULL == pRecords)
		ReportError (_T ("Failure to map input file."), 6, TRUE);
	CloseHandle (mHandle);

	/* Create the sorting threads. */
	lowRecordNum = 0;
	for (iTh = 0; iTh < numFiles; iTh++) {
		threadArg[iTh].iTh = iTh;
		threadArg[iTh].lowRecord = pRecords + lowRecordNum;
		threadArg[iTh].highRecord = pRecords + (lowRecordNum + nRecTh);
		lowRecordNum += nRecTh;
		pThreadHandle[iTh] = (HANDLE)_beginthreadex (
			NULL, 0, SortThread, &threadArg[iTh], CREATE_SUSPENDED, NULL);
	}

	/* Resume all the initially suspened threads. */

	for (iTh = 0; iTh < numFiles; iTh++)
		ResumeThread (pThreadHandle[iTh]);

	/* Wait for the sort-merge threads to complete. */

	WaitForSingleObject (pThreadHandle[0], INFINITE);
	for (iTh = 0; iTh < numFiles; iTh++)
		CloseHandle (pThreadHandle[iTh]);

	/*  Print out the entire sorted file. Treat it as one single string. */

	stringEnd = (LPTSTR) pRecords + nRec*RECSIZE;
	*stringEnd =_T('\0');
	if (!noPrint) {
	    _tprintf (_T("%s"), (LPCTSTR) pRecords); 
	}
	UnmapViewOfFile(pRecords);
	// Restore the file length
	/* SetFilePointer is convenient as it's a short addition from the file end */
	if (!SetFilePointer(hFile, -2, 0, FILE_END) || !SetEndOfFile(hFile))
	ReportError (_T("Failure restore input file lenght."), 7, TRUE);

	CloseHandle(hFile);
	free (threadArg); free (pThreadHandle);
	return 0;

} /* End of _tmain. */

static VOID MergeArrays (LPRECORD, DWORD);

DWORD WINAPI SortThread (PTHREADARG pThArg)
{
	DWORD groupSize = 2, myNumber, twoToI = 1;
			/* twoToI = 2^i, where i is the merge step number. */
	DWORD_PTR numbersInGroup;
	LPRECORD first;

	myNumber = pThArg->iTh;
	first = pThArg->lowRecord; 
	numbersInGroup = (DWORD)(pThArg->highRecord - first); 

	/* Sort this portion of the array. */
	qsort (first, numbersInGroup, RECSIZE, KeyCompare);

	/* Either exit the thread or wait for the adjoining thread. */
	while ((myNumber % groupSize) == 0 && numbersInGroup < nRec) {
				/* Merge with the adjacent sorted array. */
		WaitForSingleObject (pThreadHandle[myNumber + twoToI], INFINITE);
		MergeArrays (first, numbersInGroup);
		numbersInGroup *= 2;
		groupSize *= 2;
		twoToI *=2;
	}
	return 0;
}

static VOID MergeArrays (LPRECORD p1, DWORD nRecs)
{
	/* Merge two adjacent arrays, each with nRecs records. p1 identifies the first */
	DWORD iRec = 0, i1 = 0, i2 = 0;
	LPRECORD pDest, p1Hold, pDestHold, p2 = p1 + nRecs;

	pDest = pDestHold = malloc (2 * nRecs * RECSIZE);
	p1Hold = p1;

	while (i1 < nRecs && i2 < nRecs) {
		if (KeyCompare ((LPCTSTR)p1, (LPCTSTR)p2) <= 0) {
			memcpy (pDest, p1, RECSIZE);
			i1++; p1++; pDest++;
		}
		else {
			memcpy (pDest, p2, RECSIZE);
			i2++; p2++; pDest++;
		}
	}
	if (i1 >= nRecs)
		memcpy (pDest, p2, RECSIZE * (nRecs - i2));
	else	memcpy (pDest, p1, RECSIZE * (nRecs - i1));

	memcpy (p1Hold, pDestHold, 2 * nRecs * RECSIZE);
	free (pDestHold);
	return;
}

int KeyCompare (LPCTSTR pRec1, LPCTSTR pRec2)
{
	DWORD i;
	TCHAR b1, b2;
	LPRECORD p1, p2;
	int Result = 0;

	p1 = (LPRECORD)pRec1;
	p2 = (LPRECORD)pRec2;
	for (i = 0; i < KEYLEN && Result == 0; i++) {
		b1 = p1->key[i];
		b2 = p2->key[i];
		if (b1 < b2) Result = -1;
		if (b1 > b2) Result = +1;
	}
	return  Result;
}
