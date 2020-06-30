/* Chapter 7 SortMTx. Work crew model. INTENTIONALLY DEFECTIVE
/*	File sorting with multiple threads and merge sort.
	sortMTx [options] nt file. Work crew model.  */

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
	TCHAR Key [KEYLEN];
	TCHAR Data [DATALEN];
} RECORD;
#define RECSIZE sizeof (RECORD)
typedef RECORD * LPRECORD;

typedef struct _THREADARG {	/* Thread argument */
	DWORD iTh;		/* Thread number: 0, 1, 3, ... */
	LPRECORD LowRec;	/* Low Record */
	LPRECORD HighRec;	/* High record */
} THREADARG, *PTHREADARG;

static DWORD WINAPI ThSort (PTHREADARG pThArg);
static int KeyCompare (LPCTSTR, LPCTSTR);

static DWORD nRec;	/* Total number of records to be sorted. */
static HANDLE * ThreadHandle;

int _tmain (int argc, LPTSTR argv[])
{
	/* The file is the first argument. Sorting is done in place. */
	/* Sorting is done in memory heaps. */

	HANDLE hFile;
	LPRECORD pRecords = NULL;
	DWORD FsLow, nRead, LowRecNo, nRecTh, NPr, ThId, iTh;
	BOOL NoPrint;
	int iFF, iNP;
	PTHREADARG ThArg;
	LPTSTR StringEnd;

	if (argc <= 3) 
		ReportError (_T ("Usage: sortMTx [options] nTh files."), 1, FALSE);

	iNP = Options (argc, argv, _T ("n"), &NoPrint, NULL);
	iFF = iNP + 1;
	NPr = _ttoi (argv [iNP]);

	if (argc <= iFF)
		ReportError (_T ("Usage: sortMTx [options] nTh files."), 1, FALSE);

	/* Open the file (use the temporary copy). */

	hFile = CreateFile (argv[iFF], GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		ReportException (_T ("Failure to open input file."), 2);
	
	/* Get the file size. */

	FsLow = GetFileSize (hFile, NULL);
	if (FsLow == 0xFFFFFFFF)
		ReportException (_T ("Error getting file size."), 3);

	nRec = FsLow / RECSIZE;	/* Total number of records. */
	nRecTh = nRec / NPr;	/* Records per thread. */
	ThArg = malloc (NPr * sizeof (THREADARG));	/* Array of thread args. */ 
	ThreadHandle = malloc (NPr * sizeof (HANDLE));
	
	/* Allocate a buffer for the complete file, null terminated with some room to spare. */

	pRecords = malloc (FsLow + sizeof (TCHAR));	/* Read the entire file. */
	
	if (!ReadFile (hFile, pRecords, FsLow, &nRead, NULL))
		ReportException (_T ("Error reading sort file."), 4);
	CloseHandle (hFile);

	/* Create the sorting threads. */

	LowRecNo = 0;
	for (iTh = 0; iTh < NPr; iTh++) {
		ThArg [iTh].iTh = iTh;
		ThArg [iTh].LowRec = pRecords + LowRecNo;
		LowRecNo += nRecTh;
		ThreadHandle [iTh] = (HANDLE)_beginthreadex (
				NULL, 0, ThSort, &ThArg [iTh], 0, &ThId);
		ThArg [iTh].HighRec = pRecords + (LowRecNo + nRecTh);
	}

	/* Resume all the initially suspened threads. */

	for (iTh = 0; iTh < NPr; iTh++)
		ResumeThread (ThreadHandle [iTh]);

	for (iTh = 0; iTh < NPr; iTh++)
		CloseHandle (ThreadHandle [iTh]);

	/*  Print out the entire sorted file. Treat it as one single string. */

	StringEnd = (LPTSTR) pRecords + FsLow;
	*StringEnd ='\0';
	if (!NoPrint) _tprintf (_T("%s"), (LPCTSTR) pRecords); 
	free (pRecords); free (ThArg); free (ThreadHandle);
	return 0;

} /* End of _tmain. */

static VOID MergeArrays (LPRECORD, LPRECORD);

DWORD WINAPI ThSort (PTHREADARG pThArg)
{
	DWORD GrpSize = 2, RecsInGrp, MyNumber, TwoToI = 1;
			/* TwoToI = 2**i, where i is the merge step number. */
	LPRECORD First;

	MyNumber = pThArg->iTh;
	First = pThArg->LowRec; 
	RecsInGrp = pThArg->HighRec - First; 
		/* Number of records in this group. */
		/* GrpSize is the number of original groups now
			being merged at the merge step. */
		/* AdjOffset is the offset from this group number
			to the number whose thread we wait for. */

	/* Sort this portion of the array. */

	qsort (First, RecsInGrp, RECSIZE, KeyCompare);

	/* Either exit the thread or wait for the adjoining thread. */

	while ((MyNumber % GrpSize) == 0 && RecsInGrp < nRec) {
		/* Merge with the adjacent sorted array.  */
		WaitForSingleObject (ThreadHandle [MyNumber + TwoToI], 100);
		MergeArrays (First, First + RecsInGrp);
		RecsInGrp *= 2;
		GrpSize *= 2;
		TwoToI *=2;
	}
	_endthreadex (0);
	return 0;	/* Suppress a warning message. */
}

static VOID MergeArrays (LPRECORD p1, LPRECORD p2)
{
	DWORD iRec = 0, nRecs, i1 = 0, i2 = 0;
	LPRECORD pDest, p1Hold, pDestHold;

	nRecs = p2 - p1;
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
		b1 = p1->Key [i];
		b2 = p2->Key [i];
		if (b1 < b2) Result = -1;
		if (b1 > b2) Result = +1;
	}
	return  Result;
}
