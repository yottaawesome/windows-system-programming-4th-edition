/* Job management utility functions. */
/* These functions illustrate file locking. */
/*  As implemented, these functions use LockFileEx
	and will not work under Windows 9x.
	Use LockFile for Windows 9x operation, in which case
	all locks will be exclusive. */

#include "Everything.h"
#include "JobManagement.h"

LONG GetJobNumber (PROCESS_INFORMATION *pProcessInfo, LPCTSTR Command)

/* Create a job number for the new process, and enter
	the new process information into the job database. */

/* The job database is maintained in a shared file in the C:\temp directory.
	The name is a combination of the user name and ".JobMgt".
	The file is created at the time of the first job and deleted
	when the last job is killed.
	This file is shared by all the job management functions. 
	The file is assumed to be "small" (< 4 GB). If the file
	is large, the code will fail in several places, but there
	is protection to limit the number of jobs (arbitrarily, but
	not unreasonably) to 10000 (MAX_JOBS_ALLOWED).
	Return -1 in case of failure. */
{
	HANDLE hJobData, hProcess;
	JM_JOB jobRecord;
	DWORD jobNumber = 0, nXfer, exitCode;
	LARGE_INTEGER fileSize, fileSizePlus;
	TCHAR jobMgtFileName[MAX_PATH];
	OVERLAPPED regionStart;
	
	if (!GetJobMgtFileName (jobMgtFileName)) return -1;
	hJobData = CreateFile (jobMgtFileName, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hJobData == INVALID_HANDLE_VALUE) {
		ReportError (_T ("Failed to open Job DB."), 0, TRUE);
		return -1;
	}

	/*  Lock the entire file plus one record for exclusive
		access as we will be making a new entry. Note how it
	    is necessary (and possible!) to lock beyond the current
		end of file. Doing so prevents concurrent attempts to add
		a new record at the end of the file. */

	regionStart.Offset = 0;
	regionStart.OffsetHigh = 0;
	regionStart.hEvent = (HANDLE)0;
	GetFileSizeEx (hJobData, &fileSize);
	fileSizePlus.QuadPart = fileSize.QuadPart + SJM_JOB; 
	LockFileEx (hJobData, LOCKFILE_EXCLUSIVE_LOCK,
		0, fileSizePlus.LowPart, fileSizePlus.HighPart, &regionStart);

	__try { /* Assure that the region is unlocked. */
	/* Read records to find empty slot (corresponding to the job number).
		Extend the file to create a new job number if required. */
	/*	See text comments and Exercise 6-8 regarding a potential defect (and fix)
		caused by process ID reuse. */

	while (jobNumber < MAX_JOBS_ALLOWED &&
		ReadFile (hJobData, &jobRecord, SJM_JOB, &nXfer, NULL) && (nXfer > 0)) {
		if (jobRecord.ProcessId == 0) break;

		/* Also use this slot if corresponding job has ended. */

		hProcess = OpenProcess (PROCESS_ALL_ACCESS, FALSE, jobRecord.ProcessId);
		if (hProcess == NULL) break;
		if (GetExitCodeProcess (hProcess, &exitCode)
			&& CloseHandle (hProcess) && (exitCode != STILL_ACTIVE))
			break;

		jobNumber++;
	}

	/* Either an empty slot has been found, or we are at end
		of the file and need to create a new one, or we've
	    exceeded the max number of jobs. */
	if (jobNumber >= MAX_JOBS_ALLOWED) return -1;

	if (nXfer != 0)		/* Not at end of file. Back up a record. */
		/* SetFilePoiner is more convenient here than SetFilePointerEx since the move is a short shift from the current position */
		SetFilePointer (hJobData, -(LONG)SJM_JOB, NULL, FILE_CURRENT);
					/* Enter the new Job record. */
	jobRecord.ProcessId = pProcessInfo->dwProcessId;
	_tcsnccpy_s (jobRecord.CommandLine, sizeof(jobRecord.CommandLine), Command, MAX_PATH);
	WriteFile (hJobData, &jobRecord, SJM_JOB, &nXfer, NULL);
	}

	__finally {		/* Release the lock on the file. */
		UnlockFileEx (hJobData, 0, fileSizePlus.LowPart, fileSizePlus.HighPart, &regionStart);
		CloseHandle (hJobData);
	}
	return jobNumber + 1;
}

BOOL DisplayJobs (void)

/* Scan the job database file, reporting on the status of all jobs.
	In the process remove all jobs that no longer exist in the system. */
{
	HANDLE hJobData, hProcess;
	JM_JOB jobRecord;
	DWORD jobNumber = 0, nXfer, exitCode;
	TCHAR jobMgtFileName[MAX_PATH];
	OVERLAPPED regionStart;
	
	if ( !GetJobMgtFileName (jobMgtFileName) )
		return FALSE;
	hJobData = CreateFile (jobMgtFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hJobData == INVALID_HANDLE_VALUE)
		return FALSE;

	/* Read records and report on each job. */
	/*  First, create an exclusive lock on the entire
		file as entries will be changed. */
	regionStart.Offset = 0;
	regionStart.OffsetHigh = 0;
	regionStart.hEvent = (HANDLE)0;
	LockFileEx (hJobData, LOCKFILE_EXCLUSIVE_LOCK, 0, 0, 0, &regionStart);

	__try {
	while (ReadFile (hJobData, &jobRecord, SJM_JOB, &nXfer, NULL) && (nXfer > 0)) {
		jobNumber++; /* jobNumber won't exceed MAX_JOBS_ALLOWED as the file is
					    not allowed to grow that long */
		hProcess = NULL;
		if (jobRecord.ProcessId == 0) continue;
					/* Obtain process status. */
		hProcess = OpenProcess (PROCESS_ALL_ACCESS, FALSE, jobRecord.ProcessId);
		if (hProcess != NULL) {
			GetExitCodeProcess (hProcess, &exitCode);
			CloseHandle (hProcess);
		}
					/* Show the job status. */
		_tprintf (_T (" [%d] "), jobNumber);
		if (NULL == hProcess)
			_tprintf (_T (" Done"));
		else if (exitCode != STILL_ACTIVE)
			_tprintf (_T ("+ Done"));
		else _tprintf (_T ("      "));
					/* Show the command. */
		_tprintf (_T (" %s\n"), jobRecord.CommandLine);

		/* Remove processes that are no longer in system. */

		if (NULL == hProcess) { /* Back up one record. */
			/* SetFilePoiner is more convenient here than SetFilePointerEx since the move is a short shift from the current position */
			SetFilePointer (hJobData, -(LONG)nXfer, NULL, FILE_CURRENT);
					/* Set the job number to 0. */
			jobRecord.ProcessId = 0;
			if (!WriteFile (hJobData, &jobRecord, SJM_JOB, &nXfer, NULL))
				ReportError (_T ("Rewrite error."), 1, TRUE);
		}
	}  /* End of while. */
	}  /* End of try.   */
	
	__finally {		/* Release the lock on the file. */
		UnlockFileEx (hJobData, 0, 0, 0, &regionStart);
		if (NULL != hProcess) CloseHandle (hProcess);
	}

	CloseHandle (hJobData);
	return TRUE;
}

DWORD FindProcessId (DWORD jobNumber)

/* Obtain the process ID of the specified job number. */
{
	HANDLE hJobData;
	JM_JOB jobRecord;
	DWORD nXfer, fileSizeLow;
	TCHAR jobMgtFileName[MAX_PATH+1];
	OVERLAPPED regionStart;
	LARGE_INTEGER fileSize;

	if ( !GetJobMgtFileName (jobMgtFileName) ) return 0;
	hJobData = CreateFile (jobMgtFileName, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hJobData == INVALID_HANDLE_VALUE) return 0;
	/* Position to the correct record, but not past the end of file */
	/* As a variation, use GetFileSize to demonstrate its operation. */
	if (!GetFileSizeEx (hJobData, &fileSize) ||
		(fileSize.HighPart != 0 || SJM_JOB * (jobNumber - 1) > fileSize.LowPart 
		 || fileSize.LowPart > SJM_JOB * MAX_JOBS_ALLOWED))
			return 0;
	fileSizeLow = fileSize.LowPart;
	/* SetFilePoiner is more convenient here than SetFilePointerEx since the the file is known to be "short" ( < 4 GB). */
	SetFilePointer (hJobData, SJM_JOB * (jobNumber - 1), NULL, FILE_BEGIN);
	
	/* Get a shared lock on the record. */
	
	regionStart.Offset = SJM_JOB * (jobNumber - 1);
	regionStart.OffsetHigh = 0; /* Assume a "short" file. */
	regionStart.hEvent = (HANDLE)0;
	LockFileEx (hJobData, 0, 0, SJM_JOB, 0, &regionStart);
	
	if (!ReadFile (hJobData, &jobRecord, SJM_JOB, &nXfer, NULL))
		ReportError (_T ("JobData file error"), 0, TRUE);

	UnlockFileEx (hJobData, 0, SJM_JOB, 0, &regionStart);
	CloseHandle (hJobData);

	return jobRecord.ProcessId;
}

BOOL GetJobMgtFileName (LPTSTR jobMgtFileName)

/* Create the name of the job management file in the temporary directory. */
{
	TCHAR UserName[MAX_PATH], TempPath[MAX_PATH];
	DWORD UNSize = MAX_PATH, TPSize = MAX_PATH;

 	if (!GetUserName (UserName, &UNSize))
		return FALSE;
	if (GetTempPath (TPSize, TempPath) > TPSize)
		return FALSE;
	_stprintf_s (jobMgtFileName, MAX_PATH, _T ("%s%s%s"), TempPath, UserName, _T (".JobMgt"));
	return TRUE;
}
