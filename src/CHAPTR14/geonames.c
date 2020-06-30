/* Chapter 14. geonames. July 13, 2010. Johnson M. Hart (Copyright 2010 JMH Associates, Inc.)
	Background: 
	1.  "LINQ to Objects Using C# 4.0" by Troy Magennis, Addison-Wesley, 2010. "LINQ Data Parallelism", pp 272ff.
	    This section describes a PLINQ solution to process an 825MB "geographical names" text file.
		This program will solve the same problem using native code and techniques from the next reference.
	2.  "Windows Parallelism, Fast File Searching, and Speculative Processing" by Johnson M. Hart, June 30, 2010.
	    http://www.informit.com/articles/article.aspx?p=1606242.
		The project Projects2008_64\compMP (source file CHAPTR14\compMP.c) implements the the solution discussed in the article.
	3.	"Sequential File Processing: A Fast Path from Native Code to .NET with Visits to Parallelism and Memory Mapped Files" by Johnson M. Hart, June 13, 2010.
		is a related C#/.NET solution (not using LINQ). The article will appear in Dr. Dobb's Journal in the future. A current copy (article, code, etc.)
		is at http://www.jmhartsoftware.com/SequentialFileProcessingSupport.html.
	4.  Windows System Programming Edition 4, Chapters 5, 7-10, and 14, describe the Windows API features that are used in this program.

	The code here uses native code to solve the same problem that Magennis (#1) solves with PLINQ so as to compare performance.
	Comment: Assume that we want to optimize performance, even if the code is less elegant and more difficult to write.

	Performance Results Overview, elapsed time (seconds) running on a 4-CPU system.

		#CPUs		Native, 64-bit code.		Native, 32-bit code.		PLINQ (#1 above), 32-bit code	C# 4.0/.NET 4.0
			1			8.613						7.678						28.569							3.142
			2			3.570						3.398						14.434							1.538
			4			1.937						1.845						10.630							0.849

	Quick conclusions:
		1.  The native code solution with memory mapped files is considrably faster than the PLINQ solution, but a factor of ~4
		2.  The PLINQ solution does not scale with the number of processors beyond 2 processors. The native code solution does scale well.
		3.  The native code, however, is far more complex and much less elegant than the PLINQ code. This complexity, in turn, can lead
			to maintenance difficulties and unpleasant bugs. The performance, however, may be well worth the trouble.
		4.  32-bit code is marginally faster than 64-bit code, but larger files than used in this example will require
			64-bit code.
		5.  A faster PLINQ solution may be possible, however, with a Memory Mapped File class that supports the IEnumberable interface.
			That will be a future project.
		6.  For the time being, it's safe to say that native code using threads and memory mapped files can provide excellent performance
			for numerous file processing problems. Furthermore, the basic coding pattern is well established with the WSP4 examples 
			and Background articles #2 and #3. Nonetheless, the code can become complex.

/*  Problem overview: Find all locations with an elevation (column 15) > 8000 meters. Retain the name (column 1), the country (column 8)
    and the elevation  Sort in descenting order. Display the results (there are 16 places above 8000 meters. Only two names were familiar to me).
*/
/* geonames file [nThreads ] 
           file: pathname for the geonames database text file.
				Download and unzip from: http://download.geonames.org/export/dump/allCountries.zip. The unzipped file, allCountries.txt, is about 825MB.
				The fields are tab-separated, with one geographical location per line.
           nThreads: Number of independent threads (parallel work items to process the file)
                     Default: Number of processors on this system.
					 POTENTIAL ENHANCEMENT: If the file size is too large compared to physical memory, use the conventional]
						file processing solution. See "Windows Parallelism, Fast File Searching, and Speculative Processing"
						for more information.
					 ADDITIONAL ENHANCEMENT. Try thread pooling
 */
 /* Comment: This program may fail for large files, especially on 32-bit systems
  * where you have, at most, 3 GB to work with (of course, you have much less
  * in reality, and you need to map the file). However, the example file is small
  * enough that it should work on any system with 2GB RAM.
  */

#include "Everything.h"

#define MAX_FILTERED_ELEVATION 64  // Maximum number of records that a single thread can filter. We assume that the number of results is low (but detect a violation)
#define BLOCK_SIZE 65536  // 64K block size - Default value.
#define CACHE_LINE_SIZE 64  // used to specify alignment 
#define MAX_NUMBER_COLUMNS 30  // Maximum number of columns (fields) allowed in a geobase line

DWORD WINAPI ReadAndFilter (LPVOID); // Thread (work item) function
int CompareElevations (const void *, const void *);

typedef struct QUALIFYING_LOCATION_T {
	unsigned __int64 dataFileIndex;
	int elevation;
} QUALIFYING_LOCATION, *PQUALIFYING_LOCATION;

/* Table of elevations that satisfy the condition ( > 8000, or targetElevation) and an index to the corresponding line in the data file */
typedef struct FILTERED_ELEVATION_T {
	QUALIFYING_LOCATION qLocation[MAX_FILTERED_ELEVATION];
	int numberFound;
	int maxSize;
} FILTERED_ELEVATION, *PFILTERED_ELEVATION;

__declspec(align(CACHE_LINE_SIZE))
typedef struct TH_ARG_T {
	unsigned __int64 fileSize;
	PBYTE pFile;
	PFILTERED_ELEVATION pResults;
	HANDLE pThread;
	unsigned __int64 partitionSize;
	unsigned int threadNum;
	unsigned int nThreads;
} TH_ARG;

/* Hard coded column numbers (0 based) for the information we will keep. The text file is tab separated */
const int nameColumn = 1;
const int countryColumn = 8;
const int elevationColumn = 15;
const int targetElevation = 8000;

int _tmain (int argc, LPTSTR argv[])
{
	OSVERSIONINFO OSVer;
	SYSTEM_INFO SysInfo;

	HANDLE hFile = NULL, hFileMap = NULL;
	unsigned __int64 partitionSize;
	int nThreads, i, j, nResults = 0, iResults = 0;
	TH_ARG * pThArg, ** threadArgs;
    LARGE_INTEGER fileSize;
	PBYTE pFile = NULL;
	PQUALIFYING_LOCATION pMergedResults;
	
	OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx (&OSVer) || OSVer.dwMajorVersion < 5) 
		ReportError ("This program requires Windows Kernel 5.x (XP, ...", 1, FALSE);
	if (argc < 2)
		ReportError ("Usage: geonames file [ nThreads ]", 2, FALSE);
	GetSystemInfo (&SysInfo); // Gets the number of processors
	if (argc >= 3) { nThreads = _ttoi(argv[2]); } else { nThreads = SysInfo.dwNumberOfProcessors; }

	printf ("Serching file %s...\n", argv[1]);

	/* The file mapping set up code is long and tedious, but worth the effort.
	   You might want to put this all in a function as the sequence is standard.
	   (Open, CreateFileMap, MapViewOfFile) with lots of error checking 
	   NOTE: The pointer returned from MapViewOfFile is page aligned, which is essential
	   in many applications, but not this one. */
    // **** Start of tedious file mapping code ****
	/* Open and get file size */
    hFile = CreateFile (argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
 	if (hFile == INVALID_HANDLE_VALUE) 
		ReportError ("Fatal error opening data file.", 3, TRUE);
    if (!GetFileSizeEx (hFile, &fileSize))
        ReportError ("Fatal error sizing data file.", 4, TRUE);
	partitionSize = (fileSize.QuadPart + nThreads -1) / nThreads;

	/* Map the file for reading */
	hFileMap = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFileMap == NULL)
		ReportError ("Failure Creating data file map.", 5, TRUE);
	/* Map the input file */
	pFile = MapViewOfFile (hFileMap, FILE_MAP_READ, 0, 0, 0);
	CloseHandle (hFileMap); // No longer needed
	if (pFile == NULL)
		ReportError("Failure Mapping data file.", 6, TRUE);

	// **** End of tedious file mapping code ****

	/* Create all the Read and Compare threads suspended (may not be necessary, but be cautious)
	Also, it's comparable to the C#/.NET pattern where you create and then start a thread. */
	/* First, allocate storage for the thread arguments */
	threadArgs = (TH_ARG **)malloc(nThreads * sizeof(PVOID));
	if (NULL == threadArgs)
		ReportError ("Failed allocating storage for pointers to the thread arguments.", 7, TRUE);
	for (i = 0; i < nThreads; i++) {
		threadArgs[i] = pThArg = (TH_ARG *)_aligned_malloc(sizeof(TH_ARG), CACHE_LINE_SIZE);
		if (NULL == threadArgs[i])
			ReportError ("Failed allocating storage for thread arguments.", 8, TRUE);
		pThArg->pResults = (PFILTERED_ELEVATION)calloc(1, sizeof(FILTERED_ELEVATION)); 
		pThArg->pResults->maxSize = MAX_FILTERED_ELEVATION;
		if (NULL == pThArg->pResults) ReportError ("Failed allocating storage for thread results.", 9, TRUE);
		pThArg->threadNum = i;
		pThArg->partitionSize = partitionSize;
		pThArg->nThreads = nThreads;
		pThArg->pFile = pFile;
		pThArg->fileSize = fileSize.QuadPart;
		pThArg->pThread = (HANDLE)_beginthreadex (NULL, 0, ReadAndFilter, (LPVOID)pThArg, CREATE_SUSPENDED, NULL);
		if (pThArg->pThread == NULL)
			ReportError ("Error creating ReadAndFilter thread.", 10, TRUE);
	}
	/* Start all threads */
	for (i = 0; i < nThreads; i++) {
		ResumeThread(threadArgs[i]->pThread);
	}
	/* Wait for thread completion, close thread handles, and count the number of identified locations */
	for (i = 0; i < nThreads; i++) {
		WaitForSingleObject (threadArgs[i]->pThread, INFINITE);
		CloseHandle (threadArgs[i]->pThread);
		nResults += threadArgs[i]->pResults->numberFound;
	}

	/* Merge, sort, and report results. Use just two fields (elevation and index) in FILTERED_ELEVATION structure. */
	pMergedResults = calloc(nResults, sizeof(QUALIFYING_LOCATION));
	if (pMergedResults == NULL) ReportError ("Error allocating memory for merged results.", 11, TRUE);
	for (i = 0; i < nThreads; i++) {
		for (j = 0; j < threadArgs[i]->pResults->numberFound && iResults < nResults; j++) {
			pMergedResults[iResults].elevation = threadArgs[i]->pResults->qLocation[j].elevation;
			pMergedResults[iResults].dataFileIndex = threadArgs[i]->pResults->qLocation[j].dataFileIndex;
			iResults++;
		}
	}
	if (iResults != nResults) ReportError ("Error - inexplicable. Did not find the expected number of merged results.", 12, FALSE);

	// Sort
	qsort((PVOID)pMergedResults, (size_t)nResults, sizeof(QUALIFYING_LOCATION), CompareElevations);

	// Display the results
	__try
	{
		for (iResults = 0; iResults < nResults; iResults++) {
			__int64 iScan;
			int iColumn;
			// Skip to the location field. Put out the location and country a character at a time for simplicity
			// This would be simpler with a "Split" function (as in .NET and Perl).
			for (iScan = pMergedResults[iResults].dataFileIndex; pFile[iScan] != '\t'; iScan++) {} 
			iScan++;
			while (pFile[iScan] != '\t') { printf ("%c", pFile[iScan++]); }
			iScan++;
			printf (" ");
			printf ("(%dm) - located in ", pMergedResults[iResults].elevation, pMergedResults[iResults].dataFileIndex);
			for (iColumn = nameColumn + 1; iColumn < countryColumn; iColumn++) {
				while (pFile[iScan++] != '\t') { }
			}
			// Print the country.
			while (pFile[iScan] != '\t') { printf ("%c", pFile[iScan++]); }
			printf ("\n");
		}
	}
	__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		ReportError("Fatal Error accessing mapped file.", 13, TRUE);
	}

	free (pMergedResults);

	/* Free per-thread memory */
	for (i = 0; i < nThreads; i++) {
		free(threadArgs[i]->pResults);
		_aligned_free(threadArgs[i]);
	}

	free(threadArgs);
	UnmapViewOfFile (pFile);
	CloseHandle (hFile);

	return i;
}

int CompareElevations (const void * pLoc1, const void * pLoc2)
/* Compare two location structures on the basis of decreasting elevation */
{
	PQUALIFYING_LOCATION p1 = (PQUALIFYING_LOCATION)pLoc1;
	PQUALIFYING_LOCATION p2 = (PQUALIFYING_LOCATION)pLoc2;
	if (p1->elevation < p2->elevation) return +1;
	if (p1->elevation > p2->elevation) return -1;
	return 0;
}

DWORD WINAPI ReadAndFilter (LPVOID pArg)
{
    /* Thread function to perform file filtering on one "partition" within the file
     * ppThArg->threadNum is the "partition number" (0, 1, ..., nThreads)
     * Partition i processes text starting at position i * partitionSize on the 
	 * file (which is mapped). It first identifies the start of the first full
	 * text line within the partition, and it will process into the next partion, if
	 * necessary, to find the end of a text line.
	 * Text line records consist of tab-delimited fields 
	 */
     
	PBYTE pFile;
	FILTERED_ELEVATION * pResults;
	unsigned __int64 lineEndPos, curPos, iPos, elevationEndPos, fileSize, partitionSize, columnStart[MAX_NUMBER_COLUMNS+1];
	int iTh, columnNumber, elevation, nThreads;  // Location of column start in currently scanned line
	TH_ARG * ppThArg;

	ppThArg = (TH_ARG *)pArg;
	fileSize = ppThArg->fileSize;
	pFile = ppThArg->pFile;
	pResults = ppThArg->pResults;

	partitionSize = ppThArg->partitionSize;
	iTh = ppThArg->threadNum;
	nThreads = ppThArg->nThreads;

	/* Scan the full lines in this file partition (first new line after pFile+curPos) until the end of the partition, scanning
	 * beyond the end of the partition for the full line.
	 * Record each line with a sufficiently high elevation, retaining the name, country, and elevation - Assume that the number of
	 * qualifying locations is small and does not exceed MAX_FILTERED_ELEVATION (additional qualifying elevations are ignored - an enhancement
	 * would be to report an error). Records are maintained in "pResults"
	 */
	
	__try {
		curPos = partitionSize * iTh;
		if (curPos > 0) {
			while (curPos < fileSize && pFile[curPos] != '\n') { curPos++; }
			curPos++;
		}
		while (curPos < fileSize && curPos < partitionSize * (iTh + 1) && pResults->numberFound < pResults->maxSize) {
			// (curPos == 0 && iTh == 0) || pFile[curPos-1] == '\n'
			// Scan each line, recording the start of each tab-separated column in columnStart[]. Test the elevation and record as appropriate
			// This code would be much simpler with a "Split" function (as in .NET and Perl). However, performance might suffer as you would
			// need to allocate memory to store all the strings; we're only interested in the elevation at this point.
			columnNumber = 0;
			while (pFile[curPos] != '\n' && curPos < fileSize) {
				columnStart[columnNumber++] = curPos;
				while (pFile[curPos] != '\t' && pFile[curPos] != '\n') { curPos++; }
				if (pFile[curPos] == '\t') { curPos++; }
			}
			lineEndPos = curPos;
			// The line scan is complete and field boundaries are marked in columnStart[]
			// Extract the elevation, if any.
			if (columnNumber < elevationColumn) continue;  // No elevation data in this line. Go to the next line.
			elevationEndPos = (columnNumber == elevationColumn) ? lineEndPos - 1 : columnStart[elevationColumn+1] - 1;
			elevation = 0;

			// Brute force ascii to decimal conversion. Using atoi() would require modifying the file temporarily. The file and map would then need to be read/write 
			// Many fields are empty.
			for (iPos = columnStart[elevationColumn]; iPos < elevationEndPos; iPos++) {
				elevation = elevation*10 + (int)pFile[iPos] - 0x30;
			}

			if (elevation >= targetElevation) {
				// Record this location
				int index = pResults->numberFound;
				pResults->qLocation[index].elevation = elevation;
				pResults->qLocation[index].dataFileIndex = columnStart[0];
				pResults->numberFound++;
			}
			curPos++;
		}

	}
	__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		ReportError("Fatal Error accessing mapped file.", 13, TRUE);
	}

	return 0;
}

