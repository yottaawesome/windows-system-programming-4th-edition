/* Clear.c  Test Program. */
/* Clear memory for use in performance tests. */
/* Allocate a block from the process heap.
	Double the block size on success; halve it on failure. */
/* Obsolete: This program contains additional code for operation
   on Windows 9x (95, 98, Me) systems */
/* May 17, 2010. Behvior on a 64-bit system is not dependable */

#include "Everything.h"

#define IBLOCK_SIZE 0x10000000	/* Initial Block */

static void clear95(void);

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hHeap;
	LPVOID p;
	DWORD blockSize = IBLOCK_SIZE;
	BOOL exit = FALSE;
	OSVERSIONINFO OSVer;
	BOOL Is2000NT;
	LONG64 totalAllocated = 0, numberAllocated = 0;

	/* Determine is this is Windows 2000/NT.  */
	OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx (&OSVer)) 
		ReportError (_T("Cannot get OS Version info. %d"), 2, TRUE);

	Is2000NT = (OSVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

	if (!Is2000NT) { /* Windows 95/98 must be treated differently. OBSOLETE - See comments */
		clear95();
		return 0;
	}
	/* Get a Growable Heap.  */
	hHeap = HeapCreate (0, 0, 0);

	while (blockSize > 0) {
		{
			p = HeapAlloc (hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, blockSize);
		}
		if (p == NULL) {
//			 _tprintf (_T ("\nFailed.   Block Size = %u, Number Blocks Allocated = %u, Total Bytes Allocated = %u."), blockSize, numberAllocated, totalAllocated);
			blockSize = blockSize / 2; /* Failed. Halve block size. */
		}
		else { /* Success. Double Block Size. */
//			 _tprintf (_T ("\nSuccess.  Block Size = %u, Number Blocks Allocated = %u, Total Bytes Allocated = %u."), blockSize, numberAllocated, totalAllocated);
			numberAllocated++;
			totalAllocated += blockSize;
			if (blockSize < 0x80000000) blockSize *= 2;
		}
	}

	_tprintf (_T ("\nTotal # allocations = %u. Total # bytes allocated = %u"),
			numberAllocated, totalAllocated);
	HeapDestroy (hHeap);
	return 0;
}

static void clear95(void)
/* Clear95. Test Program. OBSOLETE. */
/*	Reconfigured to clear memory for use in performance tests.
	This version is used under Windows 95/98 as the above code consumes disk
	space which is not freed until several seconds after termination.
	In the meantime, other programs fail due to lack of disk space.
	The Windows 9x version is not as agressive; it merely allocates
	as many fixed size blocks as it can, whereas the W2000/NT version
	tries to allocate every single byte it can get. */
{

	HANDLE Heap;
	LPVOID p;
	DWORD AllocCount, HeapSize;
	BOOL exit = FALSE;
	MEMORYSTATUS MemStat;

	GlobalMemoryStatus(&MemStat);
	HeapSize = (DWORD)(MemStat.dwTotalPhys * .75);
	// _tprintf (_T("Clear95. Obsolete. Mem size. %8x\n"), HeapSize);
	
		/* Create a nongrowable heap. */
	Heap = HeapCreate (HEAP_NO_SERIALIZE, HeapSize, HeapSize);
	if (Heap == NULL)
		ReportError (_T("Clear95. Obsolete. Heap creation failure"), 1, TRUE);
		/* Allocate elements and then fail when out of memory. */


	AllocCount = 0;
	
	__try {	/* try-finally */
	__try {	/* try-except */

	while (!exit) {
		p = HeapAlloc (Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
				IBLOCK_SIZE);
		if (p == NULL) { /* Should never get here as exceptions are generated */
			exit = TRUE;
			ReportError (_T ("Clear95. Obsolete. HeapAlloc Failed."), 0, TRUE);
		}
		AllocCount++;
	}
	} /* End of try-except */

	__except (EXCEPTION_EXECUTE_HANDLER) {
		// _tprintf (_T ("Clear95. Obsolete. Memory allocation exception.\n"));
		// _tprintf (_T ("Clear95. Obsolete. End Use of Non-Serialized Heap.\n"));
		if (!HeapDestroy (Heap))
			ReportError (_T ("Clear95. Obsolete. Heap Destroy Error."), 0, TRUE);
	}
	} /* end of try-finally */

	__finally {
		// _tprintf (_T ("Clear95. Obsolete. Finally block. AllocCount = %x\n"), AllocCount);
	}


	return;
}

