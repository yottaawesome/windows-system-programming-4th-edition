/* Chapter 5.   SortHP.   File sorting with heaps.
	sortHP files   */

/* This program illustrates:
	1.	Using a memory-based algorithm (qsort).
	2.	Using heaps to allocate and destroy structures.
	2.	Outputing the sorted file as a single string.
	3.	try-except block to clean up either
		on normal completion or in case of an exception. */

/*  LIMITATION: The file is not "huge" (it is less than 4GB) */

#include "Everything.h"	/* Definitions of the record structure in the sort file. */
#define DATALEN 56	/* Correct length for presdnts.txt and monarchs.txt. */
#define KEYLEN 8

typedef struct _RECORD {
	TCHAR Key [KEYLEN];
	TCHAR Data [DATALEN];
} RECORD;

#define RECSIZE sizeof (RECORD)
typedef RECORD * LPRECORD;

int KeyCompare (LPCTSTR, LPCTSTR);

int _tmain (int argc, LPTSTR argv [])
{
	/* The file is the first argument. Sorting is done in place. */
	/* Sorting is done in memory heaps.*/

	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
	HANDLE SortHeap = NULL;
	LPBYTE pRecords = NULL;
	DWORD Result = 2, fsLow, nRead;
	LARGE_INTEGER fileSize;
	BOOL NoPrint;
	int iFirstFile, iF;

	iFirstFile = Options (argc, argv, _T ("n"), &NoPrint, NULL);

	if (argc <= iFirstFile)
		ReportError (_T ("Usage: sortHP [options] files"), 1, FALSE);
		
 for (iF = iFirstFile; iF < argc; iF++)
__try {  /* try-except */
			/* Open the file (use the temporary copy). */
	hFile = CreateFile (argv [iF], GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		ReportException (_T ("Failure to open input file."), 2);
			/* Get the file size. */
	if (!GetFileSizeEx (hFile, &fileSize))
		ReportException (_T ("Error getting file size."), 3);
	fsLow = fileSize.LowPart;
	if (fileSize.HighPart != 0)
		ReportException (_T ("File is larger than 4GB and cannot be processed."), 3);
	/* Allocate a buffer for the complete file,
				null terminated with some room to spare. */
	SortHeap = HeapCreate (HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE,
			2 * fsLow, 0);
	pRecords = HeapAlloc (SortHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE, 
			fsLow + sizeof (TCHAR));
				/* Read the entire file. */
	if (!ReadFile (hFile, pRecords, fsLow, &nRead, NULL))
		ReportException (_T ("Error reading sort file."), 4);
	CloseHandle (hFile);
		/* Now sort the file.
				Perform the sort with the C library - in memory. */
	qsort (pRecords, fsLow/RECSIZE, RECSIZE, KeyCompare);
			/* Print out the entire sorted file.
				Treat it as one single string. */
	*(pRecords + fsLow) = _T('\0');
	if (!NoPrint)
		PrintMsg (hStdOut, pRecords);
	HeapDestroy (SortHeap);
	SortHeap = NULL;

  }  /* End of inner try-except. */

  __except (EXCEPTION_EXECUTE_HANDLER) {
	/* Catch any exception here. Indicate any error.
		This is the normal termination. 
		Delete the temporary file and free all the resources. */

	if (SortHeap != NULL)
		HeapDestroy (SortHeap);
	return 0;
  }
  return 0;

} /* End of _tmain */

int KeyCompare (LPCTSTR pRec1, LPCTSTR pRec2)
{
	DWORD i;
	TCHAR b1, b2;
	LPRECORD p1, p2;
	int Result = 0;

	p1 = (LPRECORD) pRec1;
	p2 = (LPRECORD) pRec2;
	for (i = 0; i < KEYLEN && Result == 0; i++) {
		b1 = p1->Key [i];
		b2 = p2->Key [i];
		if (b1 < b2)
			Result = -1;
		if (b1 > b2)
			Result = +1;
	}
	return Result;
}
