/* Process a string to turn it into a command line. */

#include "Everything.h"

void GetArgs (LPCTSTR Command, int *pArgc, LPTSTR argstr [])
{
	int i, icm = 0;
	DWORD ic = 0;

 	for (i = 0; ic < _tcslen (Command); i++) {
		while (ic < _tcslen (Command) && 
				Command [ic] != _T(' ') && Command [ic] != _T('\t')) {
			argstr [i] [icm] = Command [ic];
			ic++;
			icm++;
		}
		argstr [i] [icm] = _T('\0');
		while (ic < _tcslen (Command) && 
				(Command [ic] == _T(' ') || Command [ic] == _T('\t')))
			ic++;
			icm = 0;
	}

	if (pArgc != NULL) *pArgc = i;
	return;
}

