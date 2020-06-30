/* Chapter 5, sortMM command. Memory Mapped version. */

/*  sortMM [options] [file]
Sort one file. The extension to multiple files is straightforward but is omitted here.
This creates an index file and does not sort the file itself

Options:
-r	Sort in reverse order.
-I	Use an existing index file to produce the sorted file.

This limited implementation sorts on the first field only.
The key field is 8 characters, starting at record position 0. */

/* This program illustrates:
1.	Mapping files to memory.
2.	Use of memory-based string processing functions in a mapped file.
3.	Use of a memory based function (qsort) with a memory-mapped file.
4.	__based pointers. */

/* Technique:
1.	Map the input file (the file to be sorted).
2.	Using standard character processing, scan the input file,
    placing each key (which is fixed size) into the "index" file record
    (the input file with a .idx suffix).
    The index file is created using file I/O; later it will be mapped.
3.	Also place the start position of the record in the index file record.
    This is a __based pointer to the mapped input file.
4.	Map the index file.
5.	Use the C library qsort to sort the index file
    (which has fixed length records consisting of key and __based pointer).
6.	Scan the sorted index file, putting input file records in sorted order.
7.	Skip steps 2, 3, and 5 if the -I option is specified.
    Notice that this requires __based pointers. */

#include "Everything.h"

int KeyCompare (LPCTSTR, LPCTSTR);
VOID CreateIndexFile (LARGE_INSTEGER, LPCTSTR, LPTSTR);
DWORD_PTR kStart = 0, kSize = 8; 	/* Key start position & size (TCHAR). */
BOOL reverse;

int _tmain (int argc, LPTSTR argv [])
{
	/* The file is the first argument. Sorting is done in place. */
	/* Sorting is done by file memory mapping. */

	HANDLE hInFile, hInMap;	/* Input file handles. */
	HANDLE hXFile, hXMap;	/* Index file handles. */
	HANDLE hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
	BOOL idxExists, noPrint;
	DWORD indexSize, rXSize, iKey, nWrite;
	LARGE_INTEGER inputSize;
	LPTSTR pInFile = NULL;
	LPBYTE pXFile = NULL, pX; 
	TCHAR __based (pInFile) *pIn, indexFileName[MAX_PATH+1], ChNewLine = _T('\n');
	int FlIdx;

	/* Determine the options. */
	FlIdx = Options (argc, argv, _T ("rIn"), &reverse, &idxExists, &noPrint, NULL);
	if (FlIdx >= argc)
		ReportError (_T ("No file name on command line."), 1, FALSE);

	/* Step 1: Open and Map the Input File. */
	hInFile = CreateFile (argv[FlIdx], GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL);
	if (hInFile == INVALID_HANDLE_VALUE)
		ReportError (_T ("Failed to open input file."), 2, TRUE);

	/* Create a file mapping object. Use the file size. */
	hInMap = CreateFileMapping (hInFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hInMap == NULL)
		ReportError (_T ("Failed to create input file mapping."), 3, TRUE);
	pInFile = MapViewOfFile (hInMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pInFile == NULL)
		ReportError (_T ("Failed to map input file."), 4, TRUE);

	if (!GetFileSizeEx (hInFile, &inputSize))
		ReportError (_T("Failed to get input file size."), 5, TRUE);

	/* Create the index file name. */
	_stprintf_s (indexFileName, MAX_PATH, _T("%s.idx"), argv [FlIdx]);

	/* Steps 2 and 3, if necessary. */
	if (!idxExists)
		CreateIndexFile (inputSize, indexFileName, pInFile);

	/* Step 4. Map the index file. */
	hXFile = CreateFile (indexFileName, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL);
	if (hXFile == INVALID_HANDLE_VALUE)
		ReportError (_T ("Failed to open existing index file."), 6, TRUE);

	/* Create a file mapping object. Use the index file size. */
	hXMap = CreateFileMapping (hXFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hXMap == NULL)
		ReportError (_T ("Failed to create index file mapping."), 7, TRUE);
	pXFile = MapViewOfFile (hXMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pXFile == NULL)
		ReportError (_T ("Failed to map index file."), 8, TRUE);
	indexSize = GetFileSize (hXFile, NULL); /* Assume the index file is not huge */
	/* Individual index record size - Key plus a pointer. */
	rXSize = kSize * TSIZE + sizeof (LPTSTR);

	__try
	{
		/* Step 5. Sort the index file using qsort. */
		if (!idxExists)
			qsort (pXFile, indexSize / rXSize, rXSize, KeyCompare);

		/* Step 6. Output the sorted input file. */
		/* Point to the first pointer in the index file. */
		pX = pXFile + rXSize - sizeof (LPTSTR);
		if (!noPrint) {
			for (iKey = 0; iKey < indexSize / rXSize; iKey++) {		
				WriteFile (hStdOut, &ChNewLine, TSIZE, &nWrite, NULL);

				/*	The cast on pX is important, as it is a pointer to a
				character and we need the four or eight bytes of a based pointer. */
				pIn = (TCHAR __based (pInFile)*) *(DWORD *)pX;
				while ((SIZE_T)pIn < (SIZE_T)inputSize.QuadPart && (*pIn != CR || *(pIn + 1) != LF)) {
					WriteFile (hStdOut, pIn, TSIZE, &nWrite, NULL);
					pIn++;
				}
				pX += rXSize; /* Advance to the next index file pointer */
			}
			WriteFile (hStdOut, &ChNewLine, TSIZE, &nWrite, NULL);
		}
	}
	__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		ReportError(_T("Fatal Error accessing mapped file."), 9, TRUE);
	}

	/* Done. Free all the handles and maps. */
	UnmapViewOfFile (pInFile);
	CloseHandle (hInMap); CloseHandle (hInFile);
	UnmapViewOfFile (pXFile);
	CloseHandle (hXMap); CloseHandle (hXFile);
	return 0;
}

VOID CreateIndexFile (LARGE_INTEGER inputSize, LPCTSTR indexFileName, LPTSTR pInFile)

/* Perform Steps 2-3 as defined in program description. */
/* This step will be skipped if the options specify use of an existing index file. */
{
	HANDLE hXFile;
	TCHAR __based (pInFile) *pInScan = 0;
	DWORD nWrite;

	/* Step 2: Create an index file.
	Do not map it yet as its length is unknown. */
	hXFile = CreateFile (indexFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (hXFile == INVALID_HANDLE_VALUE)
		ReportError (_T ("Failure to create index file."), 10, TRUE);

	/* Step 3. Scan the complete file, writing keys
	   and record pointers to the key file. */
	while ((DWORD_PTR) pInScan < inputSize.QuadPart) {
		WriteFile (hXFile, pInScan + kStart, kSize * TSIZE, &nWrite, NULL);
		WriteFile (hXFile, &pInScan, sizeof (LPTSTR), &nWrite, NULL);
		while ((DWORD) (DWORD_PTR)pInScan < inputSize.QuadPart - sizeof(TCHAR) && 
				((*pInScan != CR) || (*(pInScan + 1) != LF))) {
			pInScan++; /* Skip to end of line. */
		}
		pInScan += 2; /* Skip past CR, LF. */
	}
	CloseHandle (hXFile);
	return;
}

int KeyCompare (LPCTSTR pKey1, LPCTSTR pKey2)

/* Compare two records of generic characters.
The key position and length are global variables. */
{
	DWORD i;
	TCHAR t1, t2;
	int Result = 0;
	for (i = kStart; i < kSize + kStart && Result == 0; i++) {
		t1 = *pKey1;
		t2 = *pKey2;
		if (t1 < t2)
			Result = -1;
		if (t1 > t2) 
			Result = +1;
		pKey1++;
		pKey2++;
	}
	return reverse ? -Result : Result;
}
