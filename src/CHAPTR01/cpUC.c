/* Chapter 1. Basic cp file copy program - cpUC.c
	Win32 UNIX Compatibility Implementation. */
/* cp file1 file2: Copy file1 to file2. */

#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#define BUF_SIZE 8192

int main (int argc, char *argv [])
{
	int fdIn, fdOut;
	int bytesIn, bytesOut;
	char rec[BUF_SIZE];
	if (argc != 3) {
		fprintf (stderr, "Usage: cp file1 file2\n");
		return 1;
	}
	
	/* See http://msdn.microsoft.com/en-us/library/8ef0s5kh%28VS.80%29.aspx 
	 * regarding "secure" replacements for _open 
	 * Note that this project defines the macro _CRT_SECURE_NO_WARNINGS to avoid a warning */
	fdIn = _open (argv[1], O_RDONLY);
	if (fdIn == -1) {
		perror (argv [1]);
		return 2;
	}
	fdOut = _open (argv[2], O_WRONLY | O_CREAT, 0666);
	if (fdOut == -1) {
		perror (argv[2]);
		_close(fdIn);
		return 3;
	}

/* Process the input file a record at a time. */

	while ((bytesIn = _read (fdIn, &rec, BUF_SIZE)) > 0) {
		bytesOut = _write (fdOut, &rec, (unsigned int)bytesIn);
		if (bytesOut != bytesIn) {
			perror ("Fatal write error.");
			_close(fdIn); _close(fdOut);
			return 4;
		}
	}
	_close (fdIn);
	_close (fdOut);
	return 0;
}
