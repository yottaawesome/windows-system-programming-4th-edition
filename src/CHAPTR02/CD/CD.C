/*  Chapter 3. cd. */
/*	cd [directory]: Similar to the UNIX command. */
/*  This program illustrates:
	1.  Win32 SetCurrentDirectory
	2.  How each drive has its own working directory */

#include "Everything.h"
#pragma comment(lib, "Utility_4_0_64.lib")

#define DIRNAME_LEN MAX_PATH + 2

int main (int argc, LPTSTR argv [])
{
	if (!SetCurrentDirectory (argv [1]))
		ReportError (_T ("SetCurrentDirectory error."), 1, TRUE);
	return 0;
}
