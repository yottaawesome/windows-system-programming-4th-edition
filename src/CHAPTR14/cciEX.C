/* Chapter 14. cciEX.   
		EXTENDED simplified Caesar cipher conversion. */
/*	cciEX file1 file2  */
/* This program illustrates Extended (a.k.a.
	alterable or completion routine asynch) I/O.
	It was developed by restructuring cciOV. */
/* DWORD_PTR (pointer precision unsigned integer) is used for integers 
 * that are converted to handles
 * (namely, in the overlapped structure hEvent field).
 * This eliminates Win64 warnings regarding conversion between
 * 32 and 64-bit data, as HANDLEs are 64 bits in Win64
 * This is enable only if _Wp64 is defined.
 */
#include "Everything.h"

#define MAX_OVRLP 4		/* Best results on a 4-CPU system, although values such as 2 and 8 work nearly as well - suggest using the CPU count */
#define REC_SIZE 16384 /* Best value in several tests, but, block size is not as important for performance as with cciOV (see the cciOV.c comments) */

static VOID WINAPI ReadDone (DWORD, DWORD, LPOVERLAPPED);
static VOID WINAPI WriteDone (DWORD, DWORD, LPOVERLAPPED);

/* The first overlapped structure is for reading,
	and the second is for writing. Structures and buffers are
	allocated for each oustanding operation */

OVERLAPPED overLapIn[MAX_OVRLP], overLapOut[MAX_OVRLP];
CHAR rawRec[MAX_OVRLP][REC_SIZE], cciRec[MAX_OVRLP][REC_SIZE];
HANDLE hInputFile, hOutputFile;
LONGLONG nRecords, nDone;
LARGE_INTEGER fileSize;
DWORD shift;

int _tmain (int argc, LPTSTR argv[])
{
	DWORD ic;
	LARGE_INTEGER curPosIn;

	if (argc != 4)
		ReportError (_T ("Usage: cciEX shift file1 file2"), 1, FALSE);

	shift = _ttoi(argv[1]);
	hInputFile = CreateFile (argv[2], GENERIC_READ,
			0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hInputFile == INVALID_HANDLE_VALUE) 
		ReportError (_T ("Fatal error opening input file."), 2, TRUE);
	
	hOutputFile = CreateFile (argv[3], GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (hOutputFile == INVALID_HANDLE_VALUE) {
		CloseHandle(hInputFile);
		ReportError (_T ("Fatal error opening output file."), 3, TRUE);
	}

	/* Compute the total number of records to be processed
		from the input file size - There may not be a partial record at the end. */
	if (!GetFileSizeEx (hInputFile, &fileSize)) {
		CloseHandle(hInputFile); CloseHandle(hOutputFile);
		ReportError (_T("Fatal error getting input file size."), 4, TRUE);
	}
	nRecords = (fileSize.QuadPart + REC_SIZE - 1) / REC_SIZE;

	/* Initiate a read on each buffer, corresponding
		to the overlapped structure. */
	curPosIn.QuadPart = 0;
	for (ic = 0; ic < MAX_OVRLP; ic++) {
		overLapIn[ic].hEvent = (HANDLE)ic;	/* Overload the event fields to */
		overLapOut[ic].hEvent = (HANDLE)ic;	/* hold the event number. */

						/* Set file position. */
		overLapIn[ic].Offset = curPosIn.LowPart;
		overLapIn[ic].OffsetHigh = curPosIn.HighPart;
		if (curPosIn.QuadPart < fileSize.QuadPart)
			ReadFileEx (hInputFile, rawRec[ic], REC_SIZE,
					&overLapIn[ic], ReadDone);
		curPosIn.QuadPart += (LONGLONG) REC_SIZE;
	}

	/*  All read operations are running. Enter an alterable
		wait state and continue until all records have been processed. */
	nDone = 0;
	while (nDone < 2* nRecords) {
		SleepEx (0, TRUE);
	}
	
	CloseHandle (hInputFile);
	CloseHandle (hOutputFile);
	return 0;
}

static VOID WINAPI ReadDone (DWORD Code, DWORD nBytes, LPOVERLAPPED pOv)
{
	LARGE_INTEGER curPosIn, curPosOut;
	DWORD ic, i;
	CHAR cShift = (CHAR)shift;

	nDone++;
	/* Process the record and initiate the write. */
	/* Get the overlapped structure ID from the event field. */
	ic = PtrToInt(pOv->hEvent);
	curPosIn.LowPart = overLapIn[ic].Offset;
	curPosIn.HighPart = overLapIn[ic].OffsetHigh;
	curPosOut.QuadPart = (curPosIn.QuadPart / REC_SIZE) * REC_SIZE;
	overLapOut[ic].Offset = curPosOut.LowPart;
	overLapOut[ic].OffsetHigh = curPosOut.HighPart;
	/* Encrypt the characters; write the record */
	for (i = 0; i < nBytes; i++)
		cciRec[ic][i] = (rawRec[ic][i] + cShift);
	WriteFileEx (hOutputFile, cciRec[ic], nBytes,
		&overLapOut[ic], WriteDone);

	/* Prepare the input overlapped structure
		for the next read, which will be initiated
		after the write, issued above, completes. */
	curPosIn.QuadPart += REC_SIZE * (LONGLONG) (MAX_OVRLP);
	overLapIn[ic].Offset = curPosIn.LowPart;
	overLapIn[ic].OffsetHigh = curPosIn.HighPart;

	return;
}

static VOID WINAPI WriteDone (DWORD Code, DWORD nBytes, LPOVERLAPPED pOv)
{
	LARGE_INTEGER curPosIn;
	DWORD ic;

	nDone++;
	/* Get the overlapped structure ID from the event field. */
	ic = PtrToInt(pOv->hEvent);

	/* Start the read. The file position has already
		been set in the input overlapped structure.
		Check first, however, to assure that we
		do not read past the end of file. */

	curPosIn.LowPart = overLapIn[ic].Offset;
	curPosIn.HighPart = overLapIn[ic].OffsetHigh;
	if (curPosIn.QuadPart < fileSize.QuadPart) {
		/* Start a new read. */
		ReadFileEx (hInputFile, rawRec[ic], REC_SIZE,
				&overLapIn[ic], ReadDone);
	}
	return;
}
