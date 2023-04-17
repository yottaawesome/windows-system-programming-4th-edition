/* Chapter 5, sortBT command.   Binary Tree version. */

/* sort files
	Sort one or more files.
	This limited implementation sorts on the first field only.
	The key field length is assumed to be fixed length (8-characters).
	The data fields are varying length character strings. */

/* This program illustrates:
	1.	Multiple independent heaps; one for the sort tree nodes,
		the other for the records.
	2.	Using HeapDestroy to free an entire data structure in a single operation.
	3.	Structured exception handling to catch memory allocation errors. */

/* Technique:
	1.	Scan the input file, placing each key (which is fixed size)
		into the binary search tree and each record onto the data heap.
	2. 	Traverse the search tree and output the records in order.
	3. 	Destroy the heap and repeat for the next file. */

#define _CRT_SECURE_NO_WARNINGS

#include "Everything.h"
#pragma comment(lib, "Utility_4_0_64.lib")

#define KEY_SIZE 8

	/* Structure definition for a tree node. */

typedef struct _TREENODE {
	struct _TREENODE *Left, *Right;
	TCHAR key[KEY_SIZE];
	LPTSTR pData;
} TREENODE, *LPTNODE, **LPPTNODE;

#define NODE_SIZE sizeof (TREENODE)
#define NODE_HEAP_ISIZE 0x8000
#define DATA_HEAP_ISIZE 0x8000
#define MAX_DATA_LEN 0x1000
#define TKEY_SIZE KEY_SIZE * sizeof (TCHAR)
#define STATUS_FILE_ERROR 0xE0000001    // Customer exception

LPTNODE FillTree (HANDLE, HANDLE, HANDLE);
BOOL Scan (LPTNODE);
int KeyCompare (LPCTSTR, LPCTSTR), iFile; /* for access in exception handler */
BOOL InsertTree (LPPTNODE, LPTNODE);

int _tmain (int argc, LPTSTR argv[])
{
	HANDLE hIn = INVALID_HANDLE_VALUE, hNode = NULL, hData = NULL;
	LPTNODE pRoot;
	BOOL noPrint;
	CHAR errorMessage[256];
	int iFirstFile = Options (argc, argv, _T ("n"), &noPrint, NULL);

	if (argc <= iFirstFile)
		ReportError (_T ("Usage: sortBT [options] files"), 1, FALSE);
					/* Process all files on the command line. */
	for (iFile = iFirstFile; iFile < argc; iFile++) __try {
					/* Open the input file. */
		hIn = CreateFile (argv[iFile], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hIn == INVALID_HANDLE_VALUE)
			RaiseException (STATUS_FILE_ERROR, 0, 0, NULL);
		
		__try {
			/* Allocate the two growable heaps. */
			hNode = HeapCreate ( 
				HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE, NODE_HEAP_ISIZE, 0);
			hData = HeapCreate (
				HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE, DATA_HEAP_ISIZE, 0);
		
			/* Process the input file, creating the tree. */
			pRoot = FillTree (hIn, hNode, hData);

			/* Display the tree in key order. */
			if (!noPrint) {
				_tprintf (_T ("Sorted file: %s\n"), argv[iFile]);
				Scan (pRoot);
			} 

		} __finally { /* Heaps and file handle are always closed */
			/* Destroy the two heaps and data structures. */
			if (hNode != NULL) HeapDestroy (hNode);
			if (hData != NULL) HeapDestroy (hData);
			hNode = NULL; hData = NULL;
			if (hIn != INVALID_HANDLE_VALUE) CloseHandle (hIn);
			hIn = INVALID_HANDLE_VALUE;
		}

	} /* End of main file processing loop and try block. */
	/* Handle the exceptions that we can expect - Namely, file open error or out of memory. */
	__except ( (GetExceptionCode() == STATUS_FILE_ERROR || GetExceptionCode() == STATUS_NO_MEMORY)
				? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		_stprintf (errorMessage, _T("\n%s %s"), _T("sortBT error on file:"), argv[iFile]);
		ReportError (errorMessage, 0, TRUE);
	}
	return 0;
}

LPTNODE FillTree (HANDLE hIn, HANDLE hNode, HANDLE hData)

/* Scan the input file, creating a binary search tree in the
	hNode heap with data pointers to the hData heap. */
/* Use the calling program's exception handler. */
{
	LPTNODE pRoot = NULL, pNode;
	DWORD nRead, i;
	BOOL atCR;
	TCHAR dataHold[MAX_DATA_LEN];
	LPTSTR pString;
					/* Open the input file. */
	while (TRUE) {
		pNode = HeapAlloc (hNode, HEAP_ZERO_MEMORY, NODE_SIZE);
		pNode->pData = NULL;
		(pNode->Left) = pNode->Right = NULL;
					/* Read the key. Return if done. */
		if (!ReadFile (hIn, pNode->key, TKEY_SIZE,
				&nRead, NULL) || nRead != TKEY_SIZE)
					/* Assume end of file on error. All records must be just the right size */
			return pRoot;	/* Read the data until the end of line. */
		atCR = FALSE; 		/* Last character was not a CR. */

		for (i = 0; i < MAX_DATA_LEN; i++) {
			ReadFile (hIn, &dataHold[i], TSIZE, &nRead, NULL);
			if (atCR && dataHold[i] == LF) break;
			atCR = (dataHold[i] == CR);
		}
		dataHold[i - 1] = _T('\0');

		/* dataHold contains the data without the key.
			Combine the key and the Data. */

		pString = HeapAlloc (hData, HEAP_ZERO_MEMORY,
				(SIZE_T)(KEY_SIZE + _tcslen (dataHold) + 1) * TSIZE);
		memcpy (pString, pNode->key, TKEY_SIZE);
		pString[KEY_SIZE] = _T('\0');
		_tcscat (pString, dataHold);
		pNode->pData = pString;
				/* Insert the new node into the search tree. */
		InsertTree (&pRoot, pNode);

	} /* End of while (TRUE) loop */
	return NULL; /* Failure */
}

BOOL InsertTree (LPPTNODE ppRoot, LPTNODE pNode)

/* Insert the new node, pNode, into the binary search tree, pRoot. */
{
	if (*ppRoot == NULL) {
		*ppRoot = pNode;
		return TRUE;
	}
	if (KeyCompare (pNode->key, (*ppRoot)->key) < 0)
		InsertTree (&((*ppRoot)->Left), pNode);
	else
		InsertTree (&((*ppRoot)->Right), pNode);
	return TRUE;
}

int KeyCompare (LPCTSTR pKey1, LPCTSTR pKey2)

/* Compare two records of generic characters.
	The key position and length are global variables. */
{
	return _tcsncmp (pKey1, pKey2, KEY_SIZE);
}

static BOOL Scan (LPTNODE pNode)

/* Scan and print the contents of a binary tree. */
{
	if (pNode == NULL)
		return TRUE;
	Scan (pNode->Left);
	_tprintf (_T ("%s\n"), pNode->pData);
	Scan (pNode->Right);
	return TRUE;
}
