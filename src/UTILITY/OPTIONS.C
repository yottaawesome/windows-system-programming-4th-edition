/* Utility function to extract option flags from the command line.  */

#include "Everything.h"
#include <stdarg.h>

DWORD Options (int argc, LPCTSTR argv [], LPCTSTR OptStr, ...)

/* argv is the command line.
	The options, if any, start with a '-' in argv[1], argv[2], ...
	OptStr is a text string containing all possible options,
	in one-to-one correspondence with the addresses of Boolean variables
	in the variable argument list (...).
	These flags are set if and only if the corresponding option
	character occurs in argv [1], argv [2], ...
	The return value is the argv index of the first argument beyond the options. */

{
	va_list pFlagList;
	LPBOOL pFlag;
	int iFlag = 0, iArg;

	va_start (pFlagList, OptStr);

	while ((pFlag = va_arg (pFlagList, LPBOOL)) != NULL
				&& iFlag < (int)_tcslen (OptStr)) {
		*pFlag = FALSE;
		for (iArg = 1; !(*pFlag) && iArg < argc && argv [iArg] [0] == _T('-'); iArg++)
			*pFlag = _memtchr (argv [iArg], OptStr [iFlag],
					_tcslen (argv [iArg])) != NULL;
		iFlag++;
	}

	va_end (pFlagList);

	for (iArg = 1; iArg < argc && argv [iArg] [0] == _T('-'); iArg++);

	return iArg;
}
