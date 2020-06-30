/* Chapter 5. cci Explicit Link version.    Caesar cipher. */
/* cciEL shift file1 file2 DllName
	The DLL is loaded to provide the cci_f function. */

/* This program illustrates:
	1. Explicit DLL linking. */

#include "Everything.h"

int _tmain (int argc, LPTSTR argv[])
{
	BOOL (__cdecl *cci_f) (LPCTSTR, LPCTSTR, DWORD);
	HMODULE hDLL;
	FARPROC pcci;
	TCHAR YNResp[5] = YES;

	if (argc < 5)
		ReportError (_T("Usage: cciEL shift file1 file2 DllName"), 1, FALSE);

	/* Load the cipher function. */
	hDLL = LoadLibrary (argv[4]);
	if (hDLL == NULL)
		ReportError (_T("Failed loading DLL."), 4, TRUE);

	/*  Get the entry point address. */
	pcci = GetProcAddress (hDLL, _T("cci_f"));
	if (pcci == NULL)
		ReportError (_T ("Failed of find entry point."), 5, TRUE);
	cci_f = (BOOL (__cdecl *)(LPCTSTR, LPCTSTR, DWORD)) pcci;

	/*  Call the function. */
 
	if (!cci_f (argv[2], argv[3], _ttoi(argv[1]) ) ) {
		FreeLibrary (hDLL);
		ReportError (_T ("cci failed."), 6, TRUE);
	}
	FreeLibrary (hDLL);
	return 0;
}
