/* Chapter 9. wcMT_VTP.c									*/
/*															*/
/* wcMT_VTP file1 file2 ... fileN							*/
/* WARNING: This code is NOT UNICODE enabled				*/
/*															*/
/* Parallel word count - NT6 (Vista) thread pool version	*/
/*															*/
/* Indpependent procesing of multiple files using			*/
/* the boss/worker model									*/
/* count the total number of characters, words, and lines	*/
/* in the files specified on the command line, just			*/
/* as is done by the UNIX wc utility						*/
/* Each file is processed by a separate worker in the TP.	*/
/* The main thread (the boss thread) accumulates			*/
/* the separate results to produce the final results		*/
/* As with all the other "wc" implementations, this uses 8-bit characters */
#include "Everything.h"
#define CACHE_LINE_SIZE 64

__declspec(align(CACHE_LINE_SIZE))
typedef struct { /* Structure for thread argument		*/
	char * filename;
	volatile unsigned int kchar;
	volatile unsigned int kword;
	volatile unsigned int kline;
	volatile unsigned int wcerror;
} WORK_OBJECT_ARG;

VOID CALLBACK wcfunc (PTP_CALLBACK_INSTANCE, PVOID, PTP_WORK);
int WorkerId = 0;	// Unique ID computed by each callback instance

int main (int argc, char * argv[])
{
	DWORD nchar = 0, nword = 0, nline = 0;
    PTP_WORK *pWorkObjects;
    WORK_OBJECT_ARG ** pWorkObjArgsArray, *pObjectArg;
	TP_CALLBACK_ENVIRON cbe;  // Callback environment
	int nThread, iThrd;
	
    if (!WindowsVersionOK (6, 0)) 
        ReportError ("This program requires Windows NT 6.0 or greater", 1, TRUE);

	if (argc < 2) {
		printf ("Usage: wcMT_vtp filename ... filename\n");
		return 1;
	}

	/* Create a worker thread for each file on the command line */
	nThread = (DWORD)argc - 1;
    pWorkObjects = malloc (nThread * sizeof(PTP_WORK));
	if (pWorkObjects != NULL)
		pWorkObjArgsArray = malloc (nThread * sizeof(WORK_OBJECT_ARG *));
	if (pWorkObjects == NULL || pWorkObjArgsArray == NULL)
        ReportError ("Cannot allocate working memory for worke item or argument array.", 2, TRUE);

	InitializeThreadpoolEnvironment (&cbe);

	/* Create a work object argument for each file on the command line.
	   First put the file names in the thread arguments.	*/
	for (iThrd = 0; iThrd < nThread; iThrd++) {
		pObjectArg = (pWorkObjArgsArray[iThrd] = _aligned_malloc (sizeof(WORK_OBJECT_ARG), CACHE_LINE_SIZE));
		if (NULL == pObjectArg)
			ReportError ("Cannot allocate memory for a thread argument structure.", 3, TRUE);
		pObjectArg->filename = argv[iThrd+1];
		pObjectArg->kword = pObjectArg->kchar = pObjectArg->kline = 0;
		pWorkObjects[iThrd] = CreateThreadpoolWork (wcfunc, pObjectArg, &cbe);
        if (pWorkObjects[iThrd] == NULL) 
            ReportError ("Cannot create consumer thread", 4, TRUE);
		SubmitThreadpoolWork (pWorkObjects[iThrd]);
	}
	
	/* Worker objects are all submitted. Wait for them 	*/
	/* to complete and accumulate the results			*/
	for (iThrd = 0; iThrd < nThread; iThrd++) {
		/* Wait for the thread pool work item to complete */
		WaitForThreadpoolWorkCallbacks (pWorkObjects[iThrd], FALSE);
		CloseThreadpoolWork(pWorkObjects[iThrd]);
	}
    free (pWorkObjects);
	
	/* Accumulate the results							*/
	for (iThrd = 0; iThrd < argc - 1; iThrd++) {
		pObjectArg = pWorkObjArgsArray[iThrd]; 
		nchar += pObjectArg->kchar;
		nword += pObjectArg->kword;
		nline += pObjectArg->kline;
		printf ("%10d %9d %9d %s\n", pObjectArg->kline,
			pObjectArg->kword, pObjectArg->kchar,
			pObjectArg->filename);
	}
	free (pWorkObjArgsArray);
	printf ("%10d %9d %9d \n", nline, nword, nchar);
	return 0;
}

/* Utility function. This is faster than the CLib version */
int is_a_space(int ch) {
    if(ch == ' ' || ch == '\t' || ch == '\n' ||
        ch == '\f' || ch == '\r') {
        return 1;
    } else {
        return 0;
    }
}

/* Mapping files is often faster than conventional I/O.
   This structure supports a "handle" for the upcoming 
   mapping utility functions 
 */
typedef struct {
    // Structure for mapped file
    void* pInFile;
    HANDLE hInMap;
    HANDLE hIn;
} MAPPED_FILE_HANDLE;

/* Mapping utility functions */
/* Challenge: Upgrade this to exploit 64-bit systems. This version is limited
   to small files
 */

void* map_file(LPCSTR filename, unsigned int* pFsLow, int* error, MAPPED_FILE_HANDLE* pmFH)
{
	HANDLE hIn, hInMap;
	LARGE_INTEGER fileSize;
	char* pInFile;

    *error = 0;
    hIn = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) {
        *error = 2;
        return NULL;
    }

    // Create a file mapping object on the input file. Use the file size. 
    hInMap = CreateFileMapping(hIn, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hInMap == NULL) {
        CloseHandle(hIn);
        *error = 3;
        return NULL;
    }

    // Map the input file 
    pInFile = (char*) MapViewOfFile(hInMap, FILE_MAP_READ, 0, 0, 0);
    if (pInFile == NULL) {
        CloseHandle(hInMap);
        CloseHandle(hIn);
        *error = 4;
        return NULL;
    }

    // Get the input file size. As the mapping succeeded, the file size is < 4 GB. Exercise for Reader: Remove the limitation.
    if (!GetFileSizeEx(hIn, &fileSize) || fileSize.HighPart != 0) {
        UnmapViewOfFile(pInFile);
        CloseHandle(hInMap);
        CloseHandle(hIn);
        *error = 5;
        return NULL;
    }
	*pFsLow = fileSize.LowPart;

    pmFH->pInFile = pInFile;
    pmFH->hInMap = hInMap;
    pmFH->hIn = hIn;

    return pInFile;
}

void UnMapFile(MAPPED_FILE_HANDLE* pmFH)
{
    UnmapViewOfFile(pmFH->pInFile);
    CloseHandle(pmFH->hInMap);
    CloseHandle(pmFH->hIn);
}

/* Thread worker callback function to process a single file		*/
VOID CALLBACK wcfunc (PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WORK Work)
/* Count the number of characters, works, and lines in			*/
/* the file targ->filename					*/
/* NOTE: Simple version; results may differ from wc utility		*/
{
	WORK_OBJECT_ARG * threadArgs;
	MAPPED_FILE_HANDLE fhandle;
	int iThrd;

	unsigned int ch, c, nl, nw, nc;
    int isspace_c;       // Current character
    int isspace_ch = 1;  // Previous character. Assume space to count first word

    int error;
    unsigned int fsize;
    char *fin, *fend;
	
    iThrd = InterlockedIncrement (&WorkerId) - 1;
	threadArgs = (WORK_OBJECT_ARG *)Context;
	threadArgs->wcerror = 1; /* Error for now */
	
	fin = (char*) map_file(threadArgs->filename, &fsize, &error, &fhandle);
    if (NULL == fin) {
        return;
    }

    fend = fin + fsize;

	ch = nw = nc = nl = 0;
    while (fin < fend) {
        c = *fin++;
        isspace_c = is_a_space(c);
        if (!isspace_c && isspace_ch) {
            nw++;
        }
        isspace_ch = isspace_c;
        if (c == '\n') {
            nl++;
            isspace_ch = 1; // Start looking for a new word
        }
    }

    UnMapFile(&fhandle);
	threadArgs->kchar = fsize;
	threadArgs->kword = nw;
	threadArgs->kline = nl;
	threadArgs->wcerror = 0;
	return;
}
