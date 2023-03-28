/* Chapter 1. Basic cp file copy program. UNIX Implementation. */
/* cp file1 file2: Copy file1 to file2. */
#define _CRT_SECURE_NO_WARNINGS

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#define BUF_SIZE 256

int main (int argc, char *argv [])
{
	int input_fd, output_fd;
	size_t bytes_in, bytes_out;
	char rec [BUF_SIZE];
	if (argc != 3) {
		printf ("Usage: cp file1 file2\n");
		return 1;
	}
	input_fd = _open (argv [1], O_RDONLY);
	if (input_fd == -1) {
		perror (argv [1]);
		return 2;
	}
	output_fd = _open (argv [2], O_WRONLY | O_CREAT, 0666);
	if (output_fd == -1) {
		perror (argv [2]);
		return 3;
	}

/* Process the input file a record at a time. */

	while ((bytes_in = _read (input_fd, &rec, BUF_SIZE)) > 0) {
		bytes_out = _write (output_fd, &rec, (size_t)bytes_in);
		if (bytes_out != bytes_in) {
			perror ("Fatal write error.");
			return 4;
		}
	}
	_close (input_fd);
	_close (output_fd);
	return 0;
}
