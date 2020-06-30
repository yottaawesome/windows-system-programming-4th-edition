/* Job management utility functions - Enhanced from Chapter 6. */
/* These functions illustrate file locking. */
/* NOTE AND SUGGESTION: Compare to Chapter 6's JobMgt.c to see:
 *   1) How the named pape is secured
 *   2) How this program has not been updated to take advantage of newer features such
 *		as GetFileSizeEx.
 *	In general, if you are going to use this program, it would be best to upgrade it to conform
 *  to JobMgt.c
 */

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
	JM_JOB JobRecord;
	DWORD JobNumber = 0, nXfer, ExitCode;
	DWORD FsLow, FsHigh;
	TCHAR JobMgtFileName [MAX_PATH];
	OVERLAPPED RegionStart;
	
	if (!GetJobMgtFileName (JobMgtFileName)) return -1;
	hJobData = CreateFile (JobMgtFileName, GENERIC_READ | GENERIC_WRITE,
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

	RegionStart.Offset = 0;
	RegionStart.OffsetHigh = 0;
	RegionStart.hEvent = (HANDLE)0;
	FsLow = GetFileSize (hJobData, &FsHigh);
	LockFileEx (hJobData, LOCKFILE_EXCLUSIVE_LOCK,
		0, FsLow + SJM_JOB, FsHigh, &RegionStart);

	__try { /* Assure that the region is unlocked. */

	/* Read records to find empty slot (corresponding to the job number).
		Extend the file to create a new job number if required. */

	while (JobNumber < MAX_JOBS_ALLOWED &&
		ReadFile (hJobData, &JobRecord, SJM_JOB, &nXfer, NULL) && (nXfer > 0)) {
		if (JobRecord.ProcessId == 0) break;

		/* Also use this slot if corresponding job has ended. */

		hProcess = OpenProcess (PROCESS_ALL_ACCESS, FALSE, JobRecord.ProcessId);
		if (hProcess == NULL) break;
		if (GetExitCodeProcess (hProcess, &ExitCode)
			&& CloseHandle (hProcess) && (ExitCode != STILL_ACTIVE))
			break;

		JobNumber++;
	}

	/* Either an empty slot has been found, or we are at end
		of the file and need to create a new one, or we've
	    exceeded the max number of jobs. */
	if (JobNumber >= MAX_JOBS_ALLOWED) return -1;

	if (nXfer != 0)		/* Not at end of file. Back up a record. */
		SetFilePointer (hJobData, -(LONG) SJM_JOB, NULL, FILE_CURRENT);
					/* Enter the new Job record. */
	JobRecord.ProcessId = pProcessInfo->dwProcessId;
	_tcsnccpy (JobRecord.CommandLine, Command, MAX_PATH);
	WriteFile (hJobData, &JobRecord, SJM_JOB, &nXfer, NULL);
	}

	__finally {		/* Release the lock on the file. */
		UnlockFileEx (hJobData, 0, FsLow + SJM_JOB, FsHigh, &RegionStart);
		CloseHandle (hJobData);
	}
	return JobNumber + 1;
}

BOOL DisplayJobs (void)

/* Scan the job database file, reporting on the status of all jobs.
	In the process remove all jobs that no longer exist in the system. */
{
	HANDLE hJobData, hProcess;
	JM_JOB JobRecord;
	DWORD JobNumber = 0, nXfer, ExitCode;
	TCHAR JobMgtFileName [MAX_PATH];
	OVERLAPPED RegionStart;
	DWORD FsLow, FsHigh;
	
	if (GetJobMgtFileName (JobMgtFileName) < 0)
		return FALSE;
	hJobData = CreateFile (JobMgtFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hJobData == INVALID_HANDLE_VALUE)
		return FALSE;

	/* Read records and report on each job. */
	/*  First, create an exclusive lock on the entire
		file as entries will be changed. */

	RegionStart.Offset = 0;
	RegionStart.OffsetHigh = 0;
	RegionStart.hEvent = (HANDLE)0;
	FsLow = GetFileSize (hJobData, &FsHigh);
	LockFileEx (hJobData, LOCKFILE_EXCLUSIVE_LOCK, 0, FsLow, FsHigh, &RegionStart);

	__try {
	while (ReadFile (hJobData, &JobRecord, SJM_JOB, &nXfer, NULL) && (nXfer > 0)) {
		JobNumber++; /* JobNumber won't exceed MAX_JOBS_ALLOWED as the file is
					    not allowed to grow that long */
		if (JobRecord.ProcessId == 0) continue;
					/* Obtain process status. */
		hProcess = OpenProcess (PROCESS_ALL_ACCESS, FALSE, JobRecord.ProcessId);
		if (hProcess != NULL) {
			GetExitCodeProcess (hProcess, &ExitCode);
			CloseHandle (hProcess);
		}
					/* Show the job status. */
		_tprintf (_T (" [%d] "), JobNumber);
		if (hProcess == NULL)
			_tprintf (_T (" Done"));
		else if (ExitCode != STILL_ACTIVE)
			_tprintf (_T ("+ Done"));
		else _tprintf (_T ("      "));
					/* Show the command. */
		_tprintf (_T (" %s\n"), JobRecord.CommandLine);

		/* Remove processes that are no longer in system. */

		if (hProcess == NULL) { /* Back up one record. */
			SetFilePointer (hJobData, -(LONG)nXfer, NULL, FILE_CURRENT);
					/* Set the job number to 0. */
			JobRecord.ProcessId = 0;
			if (!WriteFile (hJobData, &JobRecord, SJM_JOB, &nXfer, NULL))
				ReportError (_T ("Rewrite error."), 1, TRUE);
		}
	}  /* End of while. */
	}  /* End of try.   */
	
	__finally {		/* Release the lock on the file. */
		UnlockFileEx (hJobData, 0, FsLow, FsHigh, &RegionStart);
		CloseHandle (hProcess);
	}

	CloseHandle (hJobData);
	return TRUE;
}

DWORD FindProcessId (DWORD JobNumber)

/* Obtain the process ID of the specified job number. */
{
	HANDLE hJobData;
	JM_JOB JobRecord;
	DWORD nXfer, FileSizeLow, FileSizeHigh;
	TCHAR JobMgtFileName [MAX_PATH];
	OVERLAPPED RegionStart;

	GetJobMgtFileName (JobMgtFileName);
	hJobData = CreateFile (JobMgtFileName, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hJobData == INVALID_HANDLE_VALUE) return 0;
	/* Position to the correct record, but not past the end of file */
	FileSizeLow = GetFileSize (hJobData, &FileSizeHigh);
	if (FileSizeHigh != 0 || SJM_JOB * (JobNumber - 1) > FileSizeLow 
		|| FileSizeLow > SJM_JOB * MAX_JOBS_ALLOWED)
		return 0;

	SetFilePointer (hJobData, SJM_JOB * (JobNumber - 1), NULL, FILE_BEGIN);
	
	/* Get a shared lock on the record. */
	
	RegionStart.Offset = SJM_JOB * (JobNumber - 1);
	RegionStart.OffsetHigh = 0; /* Assume a "short" file. */
	RegionStart.hEvent = (HANDLE)0;
	LockFileEx (hJobData, 0, 0, SJM_JOB, 0, &RegionStart);
	
	if (!ReadFile (hJobData, &JobRecord, SJM_JOB, &nXfer, NULL))
		ReportError (_T ("JobData file error"), 0, TRUE);

	UnlockFileEx (hJobData, 0, SJM_JOB, 0, &RegionStart);
	CloseHandle (hJobData);

	return JobRecord.ProcessId;
}

BOOL GetJobMgtFileName (LPTSTR JobMgtFileName)

/* Create the name of the job management file in the temporary directory. */
{
	TCHAR UserName [MAX_PATH], TempPath [MAX_PATH];
	DWORD UNSize = MAX_PATH, TPSize = MAX_PATH;

 	if (!GetUserName (UserName, &UNSize))
		return FALSE;
	if (GetTempPath (TPSize, TempPath) > TPSize)
		return FALSE;
	_stprintf (JobMgtFileName, _T ("%s%s%s"), TempPath, UserName, _T (".JobMgt"));
	return TRUE;
}
