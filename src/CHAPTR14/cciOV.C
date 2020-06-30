/* Chapter 14. cciOV.
	OVERLAPPED simplified Caeser cipher file conversion. */
/* cciOV shift file1 file2 */
/* This program illustrates overlapped asynch I/O. */

/* See http://support.microsoft.com/kb/156932 for important information about asynch I/O and its behavior. */
/* This may be helpful if you wish to explore cciOV's performance, which is not as good as other cci implementations */

#include "Everything.h"

#define MAX_OVRLP 4		/* Must be <= 32 due to WaitForMultipleObjects. CPU count is best in several experiments */
#define REC_SIZE 8192 /* 16K is the minimum size to get decent performance - it was also optimal in several experiments on a 4 CPU system.
							32K was very slow, and 65K was a little better. 16K was only marginally slower than 32K.
							BUT, you may wish to tune on your target machine.
							NOTE: the loop counter is a dword, limiting the record size (an easy fix if you want to experiemnt) */

int _tmain (int argc, LPTSTR argv[])
{
	HANDLE hInputFile, hOutputFile;
	DWORD shift, nIn[MAX_OVRLP], nOut[MAX_OVRLP], ic, i;
	/* There is a copy of each of the followinng variables and */
	/* structures for each outstanding overlapped I/O operation */
	OVERLAPPED overLapIn[MAX_OVRLP], overLapOut[MAX_OVRLP];
	/* The first event index is 0 for read, 1 for write */
	/* WaitForMultipleObjects requires a contiguous array */
	HANDLE hEvents[2][MAX_OVRLP];
	/* The first index on the buffers is the I/O operation */
	CHAR buffer[MAX_OVRLP][REC_SIZE], cShift;
	LARGE_INTEGER curPosIn, curPosOut, fileSize;
	LONGLONG nRecords, iWaits;

	if (argc != 4)
		ReportError  (_T ("Usage: cciOV shift file1 file2"), 1, FALSE);
	
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

	/*  Create the manual-reset, unsignaled events for 
		the overlapped structures. Initiate a read on
		each buffer, corresponding to the overlapped structure. */
	curPosIn.QuadPart = 0;
	for (ic = 0; ic < MAX_OVRLP; ic++) {
		/* Input complete event. */
		hEvents[0][ic] = overLapIn[ic].hEvent = CreateEvent (NULL, TRUE, FALSE, NULL); 
		/* Output complete event. */
		hEvents[1][ic] = overLapOut[ic].hEvent = CreateEvent (NULL, TRUE, FALSE, NULL); 
		if (NULL == hEvents[0][ic] || NULL == hEvents[1][ic]) {
			CloseHandle(hInputFile); CloseHandle(hOutputFile);
			ReportError (_T("Fatal error creating events."), 5, TRUE);
		}
		/* Set file position; initiate first read. */
		overLapIn[ic].Offset = curPosIn.LowPart;
		overLapIn[ic].OffsetHigh = curPosIn.HighPart;
		if (curPosIn.QuadPart < fileSize.QuadPart) {
			if (!ReadFile (hInputFile, buffer[ic], REC_SIZE, &nIn[ic], &overLapIn[ic])
				&& GetLastError() != ERROR_IO_PENDING) {
				CloseHandle(hInputFile); CloseHandle(hOutputFile);
				ReportError (_T("Fatal error starting overlapped read."), 6, TRUE);
			}
		}
		/* The read is queued and will complete in the future if it has not already have completed. */
		curPosIn.QuadPart += (LONGLONG)REC_SIZE;
	}

	/*  All read operations are running. Wait for an
		event to complete.
		Wait for both read and write events.
		Continue until all records have been processed. */

	iWaits = 0;
	cShift = (CHAR)shift;
	while (iWaits < 2 * nRecords) {
		ic = WaitForMultipleObjects (2 * MAX_OVRLP,	hEvents[0], FALSE, INFINITE) - WAIT_OBJECT_0;
		iWaits++;

		if (ic < MAX_OVRLP) { /* A read completed. */
			if (!GetOverlappedResult (hInputFile, &overLapIn[ic], &nIn[ic], FALSE)) {
				CloseHandle(hInputFile); CloseHandle(hOutputFile);
				ReportError (_T ("Read failed."), 0, TRUE);
			}
			/* Reset the event so that it will not be signaled on the next WFMO call
			 * Otherwise, this event will not be reset until the nexr ReadFile for this ic value */
			ResetEvent (hEvents[0][ic]);
			/* Process the record and set the write at the same position as the read. */
			curPosIn.LowPart = overLapIn[ic].Offset;
			curPosIn.HighPart = overLapIn[ic].OffsetHigh;
			curPosOut.QuadPart = curPosIn.QuadPart;
			overLapOut[ic].Offset = curPosOut.LowPart;
			overLapOut[ic].OffsetHigh = curPosOut.HighPart;
			
			/* Encrypt the record. */
			for (i = 0; i < nIn[ic]; i++)
				buffer[ic][i] = (buffer[ic][i] + cShift);
			if (!WriteFile (hOutputFile, buffer[ic], nIn[ic], &nOut[ic], &overLapOut[ic])
				&& GetLastError() != ERROR_IO_PENDING) {
					CloseHandle(hInputFile); CloseHandle(hOutputFile);
					ReportError (_T("Fatal error starting overlapped write."), 7, TRUE);
			}

			/* Prepare the input overlapped structure
				for the next read, which will be initiated
				after the write, issued above, completes. */
			curPosIn.QuadPart += REC_SIZE * (LONGLONG) (MAX_OVRLP);
			overLapIn[ic].Offset = curPosIn.LowPart;
			overLapIn[ic].OffsetHigh = curPosIn.HighPart;

		} else if (ic < 2 * MAX_OVRLP) {	/* A write completed. */

			/* Start the read. The file position has already
				been set in the input overlapped structure.
				Check first, however, to assure that we
				do not read past the end of file. */
			ic -= MAX_OVRLP;	/* Set the output buffer index. */
			if (!GetOverlappedResult (hOutputFile, &overLapOut[ic], &nOut[ic], FALSE)) {
				CloseHandle(hInputFile); CloseHandle(hOutputFile);
				ReportError (_T ("Read failed."), 0, TRUE);
			}

			/* Reset the event so that it will not be signaled on the next WFMO call
			 * Otherwise, this event will not be reset until the nexr ReadFile for this ic value */
			ResetEvent (hEvents[1][ic]);
			curPosIn.LowPart = overLapIn[ic].Offset;
			curPosIn.HighPart = overLapIn[ic].OffsetHigh;
			/* Start a new read. */
			if (curPosIn.QuadPart < fileSize.QuadPart) {
				if (!ReadFile (hInputFile, buffer[ic], REC_SIZE, &nIn[ic], &overLapIn[ic])
					&& GetLastError() != ERROR_IO_PENDING) {
						CloseHandle(hInputFile); CloseHandle(hOutputFile);
						ReportError (_T("Fatal error starting overlapped read."), 8, TRUE);
				}
			}
		}
		else {	/* Impossible unless wait failed error. */
			CloseHandle(hInputFile); CloseHandle(hOutputFile);
			ReportError (_T ("Multiple wait error."), 0, TRUE);
		}
	}

	/*  Close all events. */
	for (ic = 0; ic < MAX_OVRLP; ic++) {
		CloseHandle (hEvents[0][ic]);
		CloseHandle (hEvents[1][ic]);
	}
	CloseHandle (hInputFile);
	CloseHandle (hOutputFile);
	return 0;
}
