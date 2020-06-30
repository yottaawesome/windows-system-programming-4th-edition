/* Chapter 14. compMP.
	Background: "Windows Parallelism, Fast File Searching, and Speculative Processing"
	Written June 13, 2010, Johnson M. Hart. Copyright JMH Associates, Inc. 2010.
	Laat modified June 23, 2010, primarily to simplify code based on Michael Bruestle's comments.
	Multithreaded file compare, similar to CMD's COMP command, but no options are supported.
	    Also, there's no prompt for a second pair of files after comparing the first pair.
		Finally, the search is terminated after 8 mismatches are detected (8 is an arbitrary number
		used to illustrate speculative processing.)
	Use threads as an alternative to overlapped I/O. 
	Also, memory map the files rather than use normal I/O.
	This is expected to improve performance.
	There are extra options to allow experimentation with the block size and the number of work items.
	Reference: Windows System Programming Edition 4, Chapters 5, 7-10, and 14.
	Final note: This program requires NT kernel 6.x (Vista, Windows 7, Server 2008, ...
	It will NOT run under Windows XP. This restriction allows us to use SRW locks and other features. */
/* compMT file1 file2 [ blockSize [, nStripes ] ] 
           file1, file2: pathnames for the two files to compare
           blockSize: Size of each block (buffer). Default: 4,096. Always use a power of two. Must be >= 8
           nStripes: Number of independent stripes (parallel work items to compare the files)
                     Default: Number of processors on this system.
					 if nStripes < 0, then use conventional file processing (ReadFile), with the block size.
						This feature is included to allow baseline comparison.
						The implementation is single threaded.
					 ENHANCEMENT: If the file size is too large compared to physical memory, use the conventional]
						file processing solution. See "Windows Parallelism, Fast File Searching, and Speculative Processing"
						for more information.
					 ADDITIONAL ENHANCEMENT. Try thread pooling

    The two files are processed with work item 0 processing blocks 0, nStripes, 2*nStripes, ...
                                     work item 1 processing blocks 1, nStripes+1, 2*nStripes+1, ...
                                and so on.
 */
 /* Comment: This program may fail for large files, especially on 32-bit systems
  * where you have, at most, 3 GB to work with (of course, you have much less
  * in reality, and you need to map two files). NOTE: At this time, nearly all
  * new systems on the market are 64-bit, although legacy systems will be around for some time.
  *
  * This program works by mapping the input and output files in their entirity.
  * You could enhance this program by mapping one block at a time for each file,
  * much as blocks are used in the ReadFile/WriteFile implementations. This would
  * allow you to deal with very large files on 32-bit systems. I have not taken
  * this step and leave it as an exercise. 
  */

#include "Everything.h"

#define BLOCK_SIZE 4096  // 4K block size - Default value.
#define CACHE_LINE_SIZE 64  // used to specify alignment 
#define MAX_MISMATCH 8 // Find this number of file mismatches before terminating.
					   // BUT, the requirement is to report the FIRST mismatches in the files.

DWORD WINAPI ReadCompare (LPVOID); // Thread (work item) function
void LogMismatchQuad(unsigned __int64, PBYTE, PBYTE, PBYTE);
void LogMismatchByte(unsigned __int64, PBYTE, PBYTE, PBYTE);
int ConventionalFileAccess (HANDLE, HANDLE, unsigned int, unsigned __int64);

__declspec(align(CACHE_LINE_SIZE))
typedef struct TH_ARG_T {
	unsigned __int64 fileSize;
	PBYTE pFile1;
	PBYTE pFile2;
	HANDLE pThread;
	unsigned int blockSize;
	unsigned int threadNum;
	unsigned int nThreads;
} TH_ARG;

/* JUNE 22, 2010 -- I've included a significant simplification of the logic based on an idea from Michael Bruestle. */
/* Global structure to accumulate file mismatches */
static struct MISMATCH_T {
	SRWLOCK mmLock;
	unsigned __int64 mmLocation[MAX_MISMATCH];
	BYTE byte1[MAX_MISMATCH];
	BYTE byte2[MAX_MISMATCH];
} misMatch;

int _tmain (int argc, LPTSTR argv[])
{
	OSVERSIONINFO OSVer;
	SYSTEM_INFO SysInfo;

	HANDLE hFile1 = NULL, hFile2 = NULL, hFile1Map = NULL , hFile2Map = NULL;
	unsigned int blockSize = BLOCK_SIZE;
	int nThreads, i;
	TH_ARG * pThArg, ** threadArgs;
    LARGE_INTEGER file1Size, file2Size;
	PBYTE pFile1 = NULL, pFile2 = NULL;
	
	/* Initialized the mismatch structure, setting the MM locations high and initializing the structure lock. */
	for (i = 0; i < MAX_MISMATCH; i++) misMatch.mmLocation[i] = _UI64_MAX;
	InitializeSRWLock(&misMatch.mmLock);

	OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx (&OSVer) || OSVer.dwMajorVersion < 6) 
		ReportError (_T("This program requires Windows Kernel 6.x (Vista, ...)"), 1, FALSE);
	if (argc < 3)
		ReportError (_T("Usage: compMT file1 file2 [ blockSize [, nStripes ] ]"), 2, FALSE);
	GetSystemInfo (&SysInfo); // Gets the number of processors
	if (argc >= 4) blockSize = _ttoi(argv[3]); 
	if (blockSize < 8) ReportError (_T("Usage Error. Block size must be >= 8"), 2, FALSE);
	if (argc >= 5) { nThreads = _ttoi(argv[4]); } else { nThreads = SysInfo.dwNumberOfProcessors; }

	_tprintf (_T("Comparing %s and %s...\n"), argv[1], argv[2]);
	_tprintf (_T("Block size is %d. Number of work items is %d.\n"), blockSize, nThreads);

	/* The file mappingg set up code is long and tedious, but worth the effort.
	   You might want to put this all in a function as both files are treaed the same way 
	   (Open, CreateFileMap, MapViewOfFile) with lots of error checking 
	   NOTE: The pointer returned from MapViewOfFile is page aligned, which is essential for 
	   correct and performant operation as we compare 8-byte units. */
    // **** Start of tedious file mapping code ****
	/* Open and get file1 size */
    hFile1 = CreateFile (argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
 	if (hFile1 == INVALID_HANDLE_VALUE) 
		ReportError (_T("Fatal error opening file1 to get its size."), 3, TRUE);
    if (!GetFileSizeEx (hFile1, &file1Size))
        ReportError (_T("Fatal error sizing file1."), 3, TRUE);
	/* Open and get file2 size */
    hFile2 = CreateFile (argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
 	if (hFile2 == INVALID_HANDLE_VALUE) 
		ReportError (_T("Fatal error opening file2 to get its size."), 4, TRUE);
    if (!GetFileSizeEx (hFile2, &file2Size))
        ReportError (_T("Fatal error sizing file2."), 5, TRUE);
	if (file1Size.QuadPart != file2Size.QuadPart)
		ReportError (_T("Files are different sizes."), 6, FALSE);

	if (nThreads <= 0) {
		/* Use conventional file access */
		return ConventionalFileAccess (hFile1, hFile2, blockSize, file1Size.QuadPart);
	}
	/* Map the two files for reading */
	hFile1Map = CreateFileMapping (hFile1, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFile1Map == NULL)
		ReportError (_T("Failure Creating file1 map."), 7, TRUE);
	/* Map the input file */
	pFile1 = MapViewOfFile (hFile1Map, FILE_MAP_READ, 0, 0, 0);
	CloseHandle (hFile1Map); // No longer needed
	if (pFile1 == NULL)
		ReportError(_T("Failure Mapping file1."), 8, TRUE);

	hFile2Map = CreateFileMapping (hFile2, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFile2Map == NULL)
		ReportError (_T("Failure Creating file2 map."), 9, TRUE);
	pFile2 = MapViewOfFile (hFile2Map, FILE_MAP_READ, 0, 0, 0);
	CloseHandle (hFile2Map); // No longer needed. 
	if (pFile2 == NULL)
		ReportError (_T("Failure Mapping file2."), 10, TRUE);
    // **** End of tedious file mapping code ****

    /* Create all the Read and Compare threads suspended (may not be necessary, but be cautious)
	   Also, it's comparable to the C#/.NET pattern where you create and then start a thread. */
	/* First, allocate storage for the thread arguments */
	threadArgs = (TH_ARG **)malloc(nThreads * sizeof(PVOID));
	if (NULL == threadArgs)
		ReportError (_T("Failed allocating storage for pointers to the thread arguments."), 11, TRUE);
	for (i = 0; i < nThreads; i++) {
		threadArgs[i] = pThArg = (TH_ARG *)_aligned_malloc(sizeof(TH_ARG), CACHE_LINE_SIZE);
		if (NULL == threadArgs[i])
			ReportError (_T("Failed allocating storage for thread arguments."), 12, TRUE);
		pThArg->threadNum = i;
		pThArg->blockSize = blockSize;
		pThArg->nThreads = nThreads;
		pThArg->pFile1 = pFile1; pThArg->pFile2 = pFile2;
		pThArg->fileSize = file1Size.QuadPart;
		pThArg->pThread = (HANDLE)_beginthreadex (NULL, 0, ReadCompare, (LPVOID)pThArg, CREATE_SUSPENDED, NULL);
		if (pThArg->pThread == NULL)
			ReportError (_T ("Error creating thread."), 11, TRUE);
	}
	/* Start all threads */
	for (i = 0; i < nThreads; i++) {
		ResumeThread(threadArgs[i]->pThread);
	}
	for (i = 0; i < nThreads; i++) {
        WaitForSingleObject (threadArgs[i]->pThread, INFINITE);
        CloseHandle (threadArgs[i]->pThread);
		_aligned_free(threadArgs[i]);
	}

	free(threadArgs);
	UnmapViewOfFile (pFile1); UnmapViewOfFile (pFile2);
	CloseHandle (hFile1); CloseHandle (hFile2);
	if (misMatch.mmLocation[0] == _UI64_MAX) {
		_tprintf(_T("Files compare OK\n"));
	}
	else {
		unsigned int i;
		_tprintf(_T("Mismatches were located starting at location %I64x.\n"), misMatch.mmLocation[0]);
		for (i = 0; i < MAX_MISMATCH && misMatch.mmLocation[i] != _UI64_MAX; i++) {
			_tprintf(_T("Compare error at OFFSET %I64X\n"), misMatch.mmLocation[i]);
			_tprintf(_T("file1 = %X\n"), misMatch.byte1[i]);
			_tprintf(_T("file2 = %X\n"), misMatch.byte2[i]);
		}
		if (i > 0) _tprintf(_T("%d mismatches were logged.\n"), i);
		if (i >= MAX_MISMATCH)
			_tprintf(_T("Search was ternminated when the number of mismatches reached the maximum of %d.\n"), MAX_MISMATCH);
	}

	return i;
}

DWORD WINAPI ReadCompare (LPVOID pArg)
{
    /* Thread function to perform file comparison on one "stripe" within the files
     * ppThArg->threadNum is the "stripe number" (0, 1, ..., nThreads)
     * Stripe i processes logical records i, i+nThreads, i+2*nThreads, ... 
     * blockSize defines the logical record size */

	PBYTE pFile1, pFile2, pFile1Start;
	unsigned __int64 curPos, fileSize;
	unsigned int iTh, blockSize, nThreads, unitSize = sizeof(unsigned __int64);
	size_t nBytes, nQuads, nResidual;
	TH_ARG * ppThArg;

	ppThArg = (TH_ARG *)pArg;
	fileSize = ppThArg->fileSize;
	iTh = ppThArg->threadNum;
	nThreads = ppThArg->nThreads;
	blockSize = ppThArg->blockSize;

	curPos = (LONGLONG)blockSize * iTh;

	/* Search for mismatches; terminate if the maximum number has been reached.
	   This is a "lazy termination"; you could also terminate in the inner loop
	   but that would require a lock in the inner loop - at least, a lock would be best so as to assure
	   a barrier and that you are seeing the latest value, which could change in another thread.
	   You could also use InterlockedCompareExchange, */
	/* Notice that the files are compared in 8-byte units ("Quads" for 4 16-bit items).
	   While this leads to some code complexity to identify the individual bytes, it is MUCH
	   faster than byte-by-byte comparison. I've seen factors of 3 improvement, or better, in
	   some case. Thanks to Michael Bruestle for suggesting this. */

	while (curPos < fileSize) {
		AcquireSRWLockShared(&misMatch.mmLock);
		__try
		{
			if ( curPos > misMatch.mmLocation[MAX_MISMATCH-1] ) return 0; // This thread can terminate
		}
		__finally
		{
			ReleaseSRWLockShared(&misMatch.mmLock);
		}
		pFile1Start = pFile1 = ppThArg->pFile1 + curPos;
		pFile2 = ppThArg->pFile2 + curPos;
		nBytes = (size_t)min(blockSize, (fileSize - curPos)); nQuads = nBytes / unitSize; nResidual = nBytes % unitSize;

		__try {
			while (nQuads > 0) { // Compare 8 byte units and log mismatches.
				if (*(unsigned __int64 *)pFile1 != *(unsigned __int64 *)pFile2) LogMismatchQuad(curPos, pFile1Start, pFile1, pFile2);
				pFile1 += unitSize; pFile2 += unitSize; nQuads--;
			}
			// Compare any residual bytes and log mismatches.
			while (nResidual > 0) {
				if (*pFile1 != *pFile2) LogMismatchByte(curPos, pFile1Start, pFile1, pFile2);
				pFile1++; pFile2++; nResidual--;
			}
		}
		__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			ReportError(_T("Fatal Error accessing mapped file."), 12, TRUE);
		}

		curPos += (LONGLONG)blockSize * nThreads;
	}
	return 0;
}

void LogMismatchQuad(unsigned __int64 curPos, PBYTE pF1Start, PBYTE pq1, PBYTE pq2)
{
	// An 8-byte mismatch occurred at curPos + (pq1 - pF1Start); The two 8-byte items are *pq1 and *pq2
	// Identify every byte mismatch and accumulate state information for every byte mismatch
	// This requires an exclusive lock on the shared state, but mismatches are rare and are
	// bounded by a small number (MAX_MISMATCH, set to 8). Comparing identical or nearly identical files
	// will not be slowed down significantly.
	unsigned __int64 mmLoc = curPos + (pq1 - pF1Start);
	int iByte;

	// Perform a byte-by-byte comparison of the 8-byte items, logging each mismatch. There will be at least one.
	for (iByte = 0; iByte < 8; iByte++)
	{
		if (*pq1 != *pq2) LogMismatchByte(curPos, pF1Start, (PBYTE)pq1, (PBYTE)pq2);
		pq1++; pq2++;
	}
	return;
}

void LogMismatchByte(unsigned __int64 curPos, PBYTE pF1Start, PBYTE pb1, PBYTE pb2)
{
	// An 8-byte mismatch occurred at curPos + (pB1 - pF1Start); the two bytes are *pb1 and *pb2
	// Accumulate state information for this byte mismatch in the mismatch table.
	// This requires an exclusive lock on the shared state, but mismatches are rare and are
	// bounded by a small number (MAX_MISMATCH, set to 8). Comparing identical or nearly identical files
	// will not be slowed down significantly.
	unsigned __int64 mmLoc = curPos + (pb1 - pF1Start);

	AcquireSRWLockExclusive(&misMatch.mmLock);
	__try {
		int insertHere = 0, iTop = MAX_MISMATCH - 1;
		while (insertHere < MAX_MISMATCH && mmLoc > misMatch.mmLocation[insertHere]) { insertHere++; }
		if (insertHere >= MAX_MISMATCH) return; // The mismatch table is full and this location is larger than all ldentified mm locations
		/* Shift remaining entries up and place the new mismatch values (location and byte values) at insertHere.
		/* The table is assured of being previously sorted, which is essential for correctness */
		while (iTop > insertHere) {
			misMatch.byte1[iTop] = misMatch.byte1[iTop-1];
			misMatch.byte2[iTop] = misMatch.byte2[iTop-1];
			misMatch.mmLocation[iTop] = misMatch.mmLocation[iTop-1];
			iTop--;
		}
		misMatch.byte1[insertHere] = *pb1; misMatch.byte2[insertHere] = *pb2;
		misMatch.mmLocation[insertHere] = mmLoc;
	}
	__finally {
		ReleaseSRWLockExclusive(&misMatch.mmLock);
	}
}


int ConventionalFileAccess (HANDLE hFile1, HANDLE hFile2, unsigned int blockSize, unsigned __int64 fileSize)
{
	/* Compare two open files (hFile1, hFile2), reading units of blockSize
	   Errors are reported as they occur, up to the maximum, MAX_MISMATCH */
	PBYTE bBuffer1, bBuffer2;
	unsigned __int64 filePosition = 0, *llBuffer1, *llBuffer2; // long long (__int64) buffers
	unsigned int iBuff, mismatchCount = 0;
	DWORD nRead1, nRead2, nExpected = blockSize;
	BOOL exit = FALSE;

	bBuffer1 = (PBYTE)malloc(blockSize);
	if (NULL != bBuffer1) bBuffer2 = (PBYTE)malloc(blockSize);
	if (NULL == bBuffer1 || NULL == bBuffer2)
		ReportError (_T("Failed to allocate buffers in ConventionalFileAccess."), 21, TRUE);
	llBuffer1 = (__int64 *)bBuffer1; llBuffer2 = (__int64 *)bBuffer2;

	for (filePosition = 0; filePosition < fileSize && !exit; filePosition += blockSize) {
		if (fileSize - filePosition < blockSize) nExpected = (DWORD)(fileSize - filePosition);
		if (!ReadFile(hFile1, bBuffer1, nExpected, &nRead1, NULL) || !ReadFile(hFile2, bBuffer2, nExpected, &nRead2, NULL) 
			|| nRead1 != nRead2 || nRead1 != nExpected )
			ReportError (_T("Failure reading one of the two files."), 22, TRUE);

		for (iBuff = 0; iBuff < nExpected/8 && !exit; iBuff++) {
			if (llBuffer1[iBuff] != llBuffer2[iBuff]) {
				// Compare byte by byte. There may be more than one mismatch.
				PBYTE p1 = (PBYTE)&llBuffer1[iBuff], p2 = (PBYTE)&llBuffer2[iBuff];
				int iShift;
				for (iShift = 0; iShift < 8 && !exit; iShift++) {
					if (*p1 != *p2) {
						mismatchCount++;
						if (mismatchCount == 1) _tprintf(_T("Mismatches were located starting at location %I64x.\n"), filePosition + iBuff*8 + iShift);
						_tprintf(_T("Compare error at OFFSET %I64X\n"), filePosition + iBuff*8 + iShift);
						_tprintf(_T("file1 = %X\n"), *p1);
						_tprintf(_T("file2 = %X\n"), *p2);
						if (mismatchCount >= MAX_MISMATCH) { 
							exit = TRUE; 
							_tprintf(_T("Search was ternminated when the number of mismatches reached the maximum of %d.\n"), MAX_MISMATCH);
							continue; 
						}
					}
					p1++; p2++;
				}
			}
		}
	}
	free (bBuffer1); free(bBuffer2);
	
	return 0;
}