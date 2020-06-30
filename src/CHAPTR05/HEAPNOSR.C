/* Clear.c  Test Program. */
/* Clear memory for use in performance tests. */
/* Allocate a block from the process heap.
	Double the block size on success; halve it on failure. */

#include "Everything.h"

#define IBLOCK_SIZE 0x10000	/* Initial Block */

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hHeap;
	LPVOID p;
	DWORD BlockSize = IBLOCK_SIZE, Total = 0, Number = 0;
	BOOL exit = FALSE;
	
	/* Get a Growable Heap.  */

	hHeap = HeapCreate (HEAP_GENERATE_EXCEPTIONS, 0X1000000, 0);

	while (BlockSize > 0) {
		__try {
			p = HeapAlloc (hHeap, 
			HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
			BlockSize);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			p = NULL;
		}
		if (p == NULL) {
//			_tprintf (_T ("\nFailed.  Block Size = %x"), BlockSize);
			BlockSize /= 2; /* Failed. Halve block size. */
		}
		else { /* Success. Double Block Size. */
//			_tprintf (_T ("\nSuccess. Block Size = %x"), BlockSize);
			Number++;
			Total += BlockSize;
			BlockSize *= 2;
		}
	}

//	_tprintf (_T ("\nTotal # allocations = %x. Total # bytes = %x"),
//			Number, Total);
	HeapDestroy (hHeap);
	return 0;
}
