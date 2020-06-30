/* Chapter 7. wcMTMM.c										*/
/*															*/
/* wcMTMM file1 file2 ... fileN								*/
/* WARNING: This code is NOT UNICODE enabled				*/
/*															*/
/* Parallel word count - mulitple thread version			*/
/*    With Memory Mapped file I/O							*/
/*															*/
/* Indpependent procesing of multiple files using			*/
/* the boss/worker model									*/
/* count the total number of characters, words, and lines	*/
/* in the files specified on the command line, just			*/
/* as is done by the UNIX wc utility						*/
/* Each file is processed by a separate worker thread.		*/
/* The main thread (the boss thread) accumulates			*/
/* the separate results to produce the final results		*/
/* LIMITATION: This will not handle "huge" files (> 4 GB)	*/

#include "Everything.h"

typedef struct { /* Structure for thread argument		*/
	char * filename;
	volatile unsigned int kchar;
	volatile unsigned int kword;
	volatile unsigned int kline;
	volatile unsigned int wcerror;
} THREAD_ARG;

DWORD WINAPI wcfunc (void *);

int main (int argc, char * argv[])

{
	HANDLE *tref;
	DWORD ithrd, tstatus, tid;
	DWORD nchar = 0, nword = 0, nline = 0;
	THREAD_ARG * targ;
	
	if (argc < 2) {
		printf ("Usage: wcMT filename ... filename\n");
		return 1;
	}
	
	/* Allcoate a thread reference and a thread arg for 	*/
	/* each worker thread					*/
	tref = (HANDLE *) calloc (argc-1, sizeof(HANDLE));
	targ = (THREAD_ARG *) calloc (argc-1, sizeof (THREAD_ARG));
	if (tref == NULL || targ == NULL) {
		printf ("Cannot allocate wokring memory\n");
		return 2;
	}
	
	/* Create a worker thread for each file on the command line */
	for (ithrd = 0; ithrd < (DWORD)argc - 1; ithrd++) {
		targ[ithrd].filename = argv[ithrd+1];
		/* Create a worker thread to process the file */
		tref[ithrd] = (HANDLE)_beginthreadex (NULL, 0, wcfunc, &targ[ithrd],
			0, &tid);
		if (tref[ithrd] == NULL) ReportError ("Cannot create thread.", 1, TRUE);
	}
	
	/* Worker threads are all running. Wait for them 	*/
	/* to complete and accumulate the results		*/
	for (ithrd = 0; ithrd < (DWORD)argc - 1; ithrd++) {
		tstatus = WaitForSingleObject (tref[ithrd], INFINITE);
		/* Report failure but continue 		*/
		if (tstatus != WAIT_OBJECT_0) 
			ReportError ("Cannot wait for thread", 0, TRUE);
		CloseHandle (tref[ithrd]);
		nchar += targ[ithrd].kchar;
		nword += targ[ithrd].kword;
		nline += targ[ithrd].kline;
		printf ("%10d %9d %9d %s\n", targ[ithrd].kline,
			targ[ithrd].kword, targ[ithrd].kchar,
			targ[ithrd].filename);
	}
	
	free (tref); /* Deallocate memory resources 		*/
	free (targ); /* Threads have been detached by join 	*/
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

void* map_file(LPCSTR filename, unsigned unsigned int* pFsLow, int* error, MAPPED_FILE_HANDLE* pmFH)
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

/* Thread worker function to process a single file          */
DWORD WINAPI wcfunc (void * arg)
/* Count the number of characters, works, and lines in		*/
/* the file targ->filename					*/
/* NOTE: Simple version; results may differ from wc utility	*/
{
	THREAD_ARG * targ;
	MAPPED_FILE_HANDLE fhandle;
	unsigned int ch, c, nl, nw, nc;
    int isspace_c;       // Current character
    int isspace_ch = 1;  // Previous character. Assume space to count first word

    int error;
    unsigned int fsize;
    char *fin, *fend;
	
	targ = (THREAD_ARG *)arg;
	targ->wcerror = 1; /* Error for now */
	
	fin = (char*) map_file(targ->filename, &fsize, &error, &fhandle);
    if (NULL == fin) {
        return 1;
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
	targ->kchar = fsize;
	targ->kword = nw;
	targ->kline = nl;
	targ->wcerror = 0;
	return 0;
}
