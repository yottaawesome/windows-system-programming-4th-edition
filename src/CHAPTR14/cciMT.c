/* Chapter 14. cciMT.
	Multithreaded simplified Caesar cipher file conversion.
	Use threads as an alternative to overlapped I/O. 
	The resulting code is simpler IMHO. */

/* cciMT shift file1 file2 */
#include "Everything.h"

#define MAX_OVRLP 4  /* Number of worker threads - Exercise: Add a command line option. CPU count is generally best */
#define RECORD_SIZE 16384 /* 16K block size - Input file size should be
                            * a multiple of MAX_OVRLP * RECORD_SIZE 
                            * 16K was optimal in some simple tests - performance was fairly stable for {1024, 2048, ..., 65536}
                            * You may want to add a command line option. */
DWORD WINAPI ReadWrite (LPVOID);
typedef struct TH_ARG_T {
	volatile HANDLE hIn;
	volatile HANDLE hOut;
	volatile DWORD threadNum;
	volatile LARGE_INTEGER fileSize;
} TH_ARG;
DWORD shift;

/**** CHALLENGE: Improve the file handle closing when an operation fails in a thread.
      Using ReportException rather than ReportError may help ****/

int _tmain (int argc, LPTSTR argv[])
{
	HANDLE hThr[MAX_OVRLP], hSize;
	DWORD i, thId;
	TH_ARG thArg[MAX_OVRLP];
    LARGE_INTEGER inputFileSize, outputFileSize;

	if (argc != 4)
		ReportError (_T ("Usage: cciMT shift file1 file2"), 1, FALSE);

	shift = _ttoi(argv[1]);
    /* Find the input file size */
    hSize = CreateFile (argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			        FILE_ATTRIBUTE_NORMAL, NULL);
 	if (hSize == INVALID_HANDLE_VALUE) 
		ReportError (_T("Fatal error opening input file to get its size."), 2, TRUE);
	if (!GetFileSizeEx (hSize, &inputFileSize)) {
		CloseHandle(hSize);
        ReportError (_T("Fatal error sizing input file."), 2, TRUE);
	}
    CloseHandle (hSize);

    /* Create the output file with the correct size 
     * NOTE: It is easiest to create the output file before the thread functions execute
     * If you do not create the file here, be sure to use OPEN_ALWAYS in the thread functions */
    outputFileSize.QuadPart = inputFileSize.QuadPart;
    hSize = CreateFile (argv[3], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hSize == INVALID_HANDLE_VALUE)
		ReportError (_T ("Fatal error opening output file to set its size."), 2, TRUE);
    /* Setting the output file size is not absolutely necesary but is convenient in case, for
     * example, there is insufficient disk space. Otherwise, one of the thread functions would
     * fail. */
    if (!SetFilePointerEx (hSize, outputFileSize, NULL, FILE_BEGIN) ||
		!SetEndOfFile (hSize)) {
			CloseHandle(hSize);
			ReportError (_T ("Fatal error sizing output file."), 2, TRUE);
	}
    CloseHandle (hSize);

    /* Create all the conversion read-write threads */
	for (i = 0; i < MAX_OVRLP; i++) {
		thArg[i].threadNum = i;
		thArg[i].hIn = CreateFile (argv[2], GENERIC_READ,
			FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
 		if (thArg[i].hIn == INVALID_HANDLE_VALUE) 
			ReportError (_T ("Fatal error opening input file."), 2, TRUE);
    	thArg[i].fileSize.LowPart = inputFileSize.LowPart;
		thArg[i].fileSize.HighPart = inputFileSize.HighPart;
		thArg[i].hOut = CreateFile (argv[3], GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH*/, NULL);
		/* Note: DO NOT USE THE TWO COMMENTED OUT FLAGS AS THEY REQUIRE THAT THE OUTPUT
			BUFFER LENGTHS BE SECTOR SIZE MULTIPLES (THE WRITE WILL FAIL ) */
		if (thArg[i].hOut == INVALID_HANDLE_VALUE) {
			CloseHandle(thArg[i].hIn); /**** CHALLENGE (As noted earlier): Improve handle management to avoid resource leaks ****/
			ReportError (_T ("Fatal error opening output file."), 3, TRUE);
		}

		hThr[i] = (HANDLE) _beginthreadex (NULL, 0, ReadWrite, (LPVOID)&thArg[i], 0, &thId);
		if (hThr[i] == NULL) {
			CloseHandle(thArg[i].hIn); CloseHandle(thArg[i].hOut);
			ReportError (_T ("Error creating thread."), 6, TRUE);
		}
	}

	for (i = 0; i < MAX_OVRLP; i++) {
        WaitForSingleObject (hThr[i], INFINITE);
        CloseHandle (hThr[i]);
        FlushFileBuffers (thArg[i].hOut); 
		CloseHandle (thArg[i].hOut); 
		CloseHandle (thArg[i].hIn);
	}
	return 0;
}

DWORD WINAPI ReadWrite (LPVOID pArg)
{
    /* Thread function to perform cci conversion on one "stripe" within a file
     * pThArg->threadNum is the "stripe number" (0, 1, ..., MAX_OVRLP)
     * Stripe i processes logical records i, i+MAX_OVRLP, i+2*MAX_OVRLP, ... 
     * RECORD_SIZE defines the logical record size */

  	CHAR buffer[RECORD_SIZE], cShift = (CHAR)shift;
	LARGE_INTEGER curPosIn, curPosOut;
	DWORD i, nRead = 1, nWrite, iTh;
	HANDLE hIn, hOut;
	TH_ARG * pThArg;
	OVERLAPPED ovRead = {0, 0, 0, 0, NULL}, ovWrite = {0, 0, 0, 0, NULL};

	pThArg = (TH_ARG *)pArg;

	iTh = pThArg->threadNum;
	hIn = pThArg->hIn; hOut = pThArg->hOut;

	curPosIn.QuadPart = curPosOut.QuadPart = (LONGLONG) RECORD_SIZE * iTh;
	while (curPosIn.QuadPart < pThArg->fileSize.QuadPart) {
		ovRead.Offset      = curPosIn.LowPart;
		ovRead.OffsetHigh  = curPosIn.HighPart;
		ovWrite.Offset     = curPosOut.LowPart;
		ovWrite.OffsetHigh = curPosOut.HighPart;
		
		if (!ReadFile (hIn, buffer, RECORD_SIZE, &nRead, &ovRead)) 
            ReportError (_T("Error reading input file"), 8, TRUE);
		for (i = 0; i < nRead; i++)
			buffer[i] = (buffer[i] + cShift);
		
		if (!WriteFile (hOut, buffer, nRead, &nWrite, &ovWrite) || nWrite != nRead)
			ReportError (_T("Error writing output file"), 9, TRUE);
        
        curPosIn.QuadPart += (LONGLONG) RECORD_SIZE * MAX_OVRLP;
		curPosOut.QuadPart += (LONGLONG) RECORD_SIZE * MAX_OVRLP;
	}

	return 0;
}
