/*	Chapter 3. FreeSpace.c
 *	Determine if files are stored sparsely or if all space is consumed */


#include "Everything.h"

void ReportSpace (LPCTSTR);

int _tmain (int argc, LPTSTR argv [])
{
	HANDLE hFile;
	LARGE_INTEGER FileLen, FileLenH;
	BYTE Buffer [256];
	OVERLAPPED ov;
	DWORD nWrite;

	while (TRUE) {
		FileLen.QuadPart = 0;
		_tprintf (_T("Enter file length (Zero to quit): "));
		_tscanf_s (_T("%d"), &FileLen.QuadPart);
		_tprintf (_T("Length: %d\n"), FileLen.QuadPart);
		/* Note: this does not really handle numbers bigger than 4G.
			Can you fix it? */
		if (FileLen.QuadPart== 0) break;
		FileLenH.QuadPart = FileLen.QuadPart/2;

		ReportSpace (_T("Before file creation"));

		hFile = CreateFile (_T("TempTestFile"), GENERIC_READ|GENERIC_WRITE, 0, NULL,
			CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			ReportError (_T("Cannot create TempTestFile"), 2, TRUE);
		ReportSpace (_T("After file creation"));

		if (!SetFilePointerEx (hFile, FileLen, NULL, FILE_BEGIN) == 0xffffffff)
			ReportError (_T("Cannnot set file pointer"), 3, TRUE);
		
		if (!SetEndOfFile (hFile))
			ReportError (_T("Cannot set end of file"), 4, TRUE);
		ReportSpace (_T("After setting file length"));

		ov.Offset = FileLenH.LowPart;
		ov.OffsetHigh = FileLen.HighPart;
		ov.hEvent = NULL; 

		if (!WriteFile (hFile, Buffer, sizeof(Buffer), &nWrite, &ov))
			ReportError (_T("Cannot write file in middle"), 5, TRUE);
		
		ReportSpace (_T("After writing to middle of file"));

		CloseHandle (hFile);
		DeleteFile (_T("TempTestFile"));
	}
	_tprintf (_T("End of FreeSpace"));
	return 0;
}


void ReportSpace (LPCTSTR Message)
{
	ULARGE_INTEGER FreeBytes, TotalBytes, NumFreeBytes;
	
	if (!GetDiskFreeSpaceEx (NULL, &FreeBytes, &TotalBytes, &NumFreeBytes))
		ReportError (_T("Cannot get free space"), 1, TRUE);
	/* Note: Windows NT 5.0 and greater (including 2000) - This measures
	space available to the user, accounting for disc quotas */
	_tprintf (_T("%35s.\nTotal: %16x; Free on disk: %16x; Avail to user: %16x\n"), Message,
		TotalBytes.QuadPart, NumFreeBytes.QuadPart, FreeBytes.QuadPart);

}


 