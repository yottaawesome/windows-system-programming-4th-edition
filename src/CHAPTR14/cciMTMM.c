/* Chapter 14. cciMTMM.
	Multithreaded simplified Caesar cipher file conversion.
	Use threads as an alternative to overlapped I/O. 
	Also, memory map the files rather than use normal I/O.
	This combines features of cciMM (Chapter 5) and cciMT (Chapter 14)*/
/* FEB 28, 2010 - THERE IS A BUG IN THIS - I'M WORKING ON IT AS TIME IS AVAILABLE - JH 
    *** Sept 7, 2010 -- I fixed this bug in May (?) and apologize for not updating the comment sooner */
/* cciMT shift file1 file2 */
#include "Everything.h"

#define MAX_OVRLP 4  /* Number of worker threads - Exercise: Add a command line option */
#define RECORD_SIZE 65536  /* 16K block size - Input file size should be
                            * a multiple of MAX_OVRLP * RECORD_SIZE 
                            * Increase this size for better performance - 65K is good 
                            * You may want to add a command line option. */
DWORD WINAPI ReadWrite (LPVOID);

#define CACHE_LINE_SIZE 64

__declspec(align(CACHE_LINE_SIZE))
typedef struct TH_ARG_T {
	volatile PBYTE pInFile;
	volatile PBYTE pOutFile;
	volatile LARGE_INTEGER fileSize;
	volatile DWORD threadNum;
} TH_ARG;
DWORD shift;

int _tmain (int argc, LPTSTR argv[])
{
	HANDLE hThr[MAX_OVRLP], hInput, hOutput, hInMap, hOutMap;
	DWORD i;
	TH_ARG thArg[MAX_OVRLP];
    LARGE_INTEGER inputFileSize, outputFileSize;
	LPTSTR pInFile, pOutFile;
	
	if (argc != 4)
		ReportError (_T ("Usage: cciMT shift file1 file2"), 1, FALSE);

	shift = _ttoi(argv[1]);
    /* Find the input file size */
    hInput = CreateFile (argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			        FILE_ATTRIBUTE_NORMAL, NULL);
 	if (hInput == INVALID_HANDLE_VALUE) 
		ReportError (_T("Fatal error opening input file to get its size."), 2, TRUE);
    if (!GetFileSizeEx (hInput, &inputFileSize))
        ReportError (_T("Fatal error sizing input file."), 3, TRUE);
	hInMap = CreateFileMapping (hInput, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hInMap == NULL)
		ReportError (_T ("Failure Creating input map."), 4, TRUE);
	/* Map the input file */
		/* Comment: This may fail for large files, especially on 32-bit systems
		 * where you have, at most, 3 GB to work with (of course, you have much less
		 * in reality, and you need to map two files. 
		 * This program works by mapping the input and output files in their entirity.
		 * You could enhance this program by mapping one block at a time for each file,
		 * much as blocks are used in the ReadFile/WriteFile implementations. This would
		 * allow you to deal with very large files on 32-bit systems. I have not taken
		 * this step and leave it as an exercise. 
		 */
	pInFile = MapViewOfFile (hInMap, FILE_MAP_READ, 0, 0, 0);
	if (pInFile == NULL)
		ReportError(_T ("Failure Mapping input file."), 5, TRUE);

    /* Create the output file with the correct size  */
    outputFileSize.QuadPart = inputFileSize.QuadPart;
    hOutput = CreateFile (argv[3], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
 	if (hOutput == INVALID_HANDLE_VALUE) 
		ReportError (_T ("Fatal error opening output file to set its size."), 6, TRUE);

	/* Setting the output file size is not absolutely necesary but is convenient in case, for
     * example, there is insufficient disk space. Otherwise, one of the thread functions would
     * fail. */
    if (!SetFilePointerEx (hOutput, outputFileSize, NULL, FILE_BEGIN))
		ReportError (_T ("Fatal error setting output file pointer."), 9, TRUE);  
    if (!SetEndOfFile (hOutput)) ReportError (_T ("Fatal error sizing output file."), 10, TRUE);

	/* Create a file mapping object on the input file. Use the file size. */
	hOutMap = CreateFileMapping (hOutput, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hOutMap == NULL)
		ReportError (_T ("Failure Creating output map."), 7, TRUE);
	/* Map the input file */
	pOutFile = MapViewOfFile (hOutMap, FILE_MAP_WRITE, 0, 0, (SIZE_T)outputFileSize.QuadPart);
	if (pOutFile == NULL)
		ReportError (_T ("Failure Mapping output file."), 8, TRUE);
	CloseHandle (hInMap); CloseHandle (hOutMap); 

    /* Create all the conversion read-write threads suspended (may not be necessary, but be cautious) */
	for (i = 0; i < MAX_OVRLP; i++) {
		thArg[i].threadNum = i;
		thArg[i].pInFile = pInFile;
		thArg[i].pInFile = pInFile;
		thArg[i].pOutFile = pOutFile;
		thArg[i].fileSize.QuadPart = inputFileSize.QuadPart;

		hThr[i] = (HANDLE)_beginthreadex (NULL, 0, ReadWrite, (LPVOID)&thArg[i], CREATE_SUSPENDED, NULL);
		if (hThr[i] == NULL)
			ReportError (_T ("Error creating thread."), 11, TRUE);
	}
	/* Start all threads */
	for (i = 0; i < MAX_OVRLP; i++) {
		ResumeThread(hThr[i]);
	}
	for (i = 0; i < MAX_OVRLP; i++) {
        WaitForSingleObject (hThr[i], INFINITE);
        CloseHandle (hThr[i]);
	}

	UnmapViewOfFile (pInFile); UnmapViewOfFile (pOutFile);
	CloseHandle (hInput); CloseHandle (hOutput);
	return 0;
}

DWORD WINAPI ReadWrite (LPVOID pArg)
{
    /* Thread function to perform cci conversion on one "stripe" within a file
     * pThArg->threadNum is the "stripe number" (0, 1, ..., MAX_OVRLP)
     * Stripe i processes logical records i, i+MAX_OVRLP, i+2*MAX_OVRLP, ... 
     * RECORD_SIZE defines the logical record size */

	PBYTE pIn, pOut;
	LARGE_INTEGER curPos, fileSize;
	DWORD iTh;
	size_t nBytes;
	TH_ARG * pThArg;
	BYTE bShift = (BYTE)shift;

	pThArg = (TH_ARG *)pArg;

	fileSize.QuadPart = pThArg->fileSize.QuadPart;
	iTh = pThArg->threadNum;

	curPos.QuadPart = (LONGLONG)RECORD_SIZE * iTh;

	while (curPos.QuadPart < fileSize.QuadPart) {
		size_t count = 0;
		/* There was a stupid bug in an earlier verion. Cause: The next two statements were before the while statement !  */
		pIn  = pThArg->pInFile  + curPos.QuadPart;
		pOut = pThArg->pOutFile + curPos.QuadPart;
		nBytes = (size_t)min(RECORD_SIZE, (fileSize.QuadPart - curPos.QuadPart));

		__try {
			for (count = 0; count < nBytes; count++) {
				pOut[count] = pIn[count] + bShift;
			}
		}
		__except(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			ReportError(_T("Fatal Error accessing mapped file."), 12, TRUE);
		}

		curPos.QuadPart += (LONGLONG)RECORD_SIZE * MAX_OVRLP;
	}
	return 0;
}
