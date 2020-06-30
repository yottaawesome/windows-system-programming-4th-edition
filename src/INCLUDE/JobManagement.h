/* JobManagement.h - Definitions required for job management. */

/* Job management exit code for killed jobs. */

#define JM_EXIT_CODE 0x1000
#define MAX_JOBS_ALLOWED 10000
typedef struct _JM_JOB
{
	DWORD ProcessId;
	TCHAR CommandLine [MAX_PATH];
} JM_JOB;

#define SJM_JOB sizeof (JM_JOB)

/* Job management functions. */

LONG GetJobNumber (PROCESS_INFORMATION *, LPCTSTR);
BOOL DisplayJobs (void);
DWORD FindProcessId (DWORD);
BOOL GetJobMgtFileName (LPTSTR);

