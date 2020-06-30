/* Chapter 1. Basic cp file copy program. Win32 Implementation.
	FAST VERSION USING
		1.  LARGE buffer
		2.  PRE-SET OUTPUT FILE SIZE
		3.  SEQUENTIAL SCAN */
/* cp file1 file2: Copy file1 to file2. */

#include <windows.h>
#include <stdio.h>
#define BUF_SIZE 8192

int main (int argc, LPTSTR argv [])
{
	HANDLE hIn, hOut;
	DWORD nIn, nOut;
	CHAR buffer [BUF_SIZE];
	if (argc != 3) {
		fprintf (stderr, "Usage: cp file1 file2\n");
		return 1;
	}
	hIn = CreateFile (argv [1], GENERIC_READ, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		fprintf (stderr, "Cannot open input file. Error: %x\n", GetLastError ());
		return 2;
	}
	hOut = CreateFile (argv [2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		fprintf (stderr, "Cannot open output file. Error: %x\n", GetLastError ());
		CloseHandle(hIn);
		return 3;
	}

	/*  Set the output file size. */

	while (ReadFile (hIn, buffer, BUF_SIZE, &nIn, NULL) && nIn > 0) {
		WriteFile (hOut, buffer, nIn, &nOut, NULL);
		if (nIn != nOut) {
			fprintf (stderr, "Fatal write error: %x\n", GetLastError ());
			CloseHandle(hIn); CloseHandle(hOut);
			return 4;
		}
	}
	CloseHandle (hIn);
	CloseHandle (hOut);
	return 0;
}
