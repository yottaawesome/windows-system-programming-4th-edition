/*	DataMgmt.h
	All definitions required for the data management functions
*/

typedef struct _DM_STATISTICS {
	DWORD NumberGets;
	DWORD NumberDeletes;
	DWORD NumberRecords;
	DWORD NumberAdds;
	DWORD NumberXctions;
	DWORD NumberUpdates;
	DWORD Status;
	DWORD MaxClients;
	DWORD CurClients;
} DM_STATISTICS, *PDM_STATISTICS;
#define DM_STAT_SIZE sizeof (DM_STATISTICS)

typedef struct _DM_SYNCHHANDLE_STRUCT {
	HANDLE hLimitSem;
	HANDLE hLimitMutex;
	CRITICAL_SECTION CsSrvrStats;
} DM_SYNCHHANDLE_STRUCT, *DM_SYNCHHANDLE;
#define DM_SYNCH_SIZE sizeof (DM_SYNCHHANDLE_STRUCT)

typedef struct _DMHANDLE_STRUCTURE {
	TCHAR FileName [MAX_PATH];
	HANDLE FileHandle;
	HANDLE HelperProc;
	DWORD RecSize;
	DWORD KeyLen;
	DWORD KeyPos;
	DWORD TableSize;
	DM_STATISTICS DmStat;
	DM_SYNCHHANDLE_STRUCT Synch;

} DMHANDLE_STRUCTURE, * DMHANDLE;

#define DMHANDLE_SIZE sizeof (DMHANDLE_STRUCTURE)
#define DMHANDLE_MAX_NUMBER 1000
#define SIZE_DMHANDLE_TABLE DMHANDLE_MAX_NUMBER*DMHANDLE_SIZE

#define DMHANDLE_NAME _T("DataManagementHandle")

/*	Error codes - all must have bit 29 set */
#define DMGMT_ERROR 0X20000001		/* EXERCISE - REFINE THE ERROR NUMBERS */

#define C_CREATE         1
#define C_TERM           2
#define C_GET            3
#define C_ADD            4
#define C_INFO           5
#define C_OPEN_EXIST     6
#define C_DELETE         7
#define C_UPDATE         8
#define C_SCAN           9
#define C_QUIT          10
#define C_ENTER         11
#define C_LIST          12
#define C_REMOVE        13
#define C_UNKNOWN       14
#define DMGMT_ADD          20
#define DMGMT_DELETE       21
#define DMGMT_UPDATE       22
#define DMGMT_NOTFOUND     23
#define DMGMT_ADDEXISTING  24
#define DMGMT_BADPARAMTERS 25
#define DMGMT_NOMEMORY     26
#define DMGMT_FULL         27

typedef struct _DATES {
	TCHAR StartDate [8];
	TCHAR EndDate   [8];
} DATES, *PDATES;

#define KEY_LEN 8

typedef struct _EMPLOYEE_STRUCTURE {
	TCHAR BadgeNo [KEY_LEN];
	TCHAR EmpName [MAX_PATH];
	TCHAR crlf [2];
	DATES Dates;

} EMPLOYEE, * PEMPLOYEE;

#define EMP_SIZE sizeof (EMPLOYEE)

DMHANDLE DmCreate (LPCTSTR, DWORD, DWORD, DWORD, DWORD, DWORD);

static DMHANDLE FatalError (LPCTSTR, HANDLE, DMHANDLE, LPBYTE);

BOOL DmClose (DMHANDLE);

BOOL DmRemove (DMHANDLE); 

BOOL DmGetRecord (DMHANDLE, LPBYTE, HANDLE hFile);

BOOL DmGetInfo (DMHANDLE, LPBYTE);

BOOL DmChangeRecord (DMHANDLE, LPBYTE, HANDLE, DWORD);

static BOOL LocateRecord (DMHANDLE, HANDLE, LPBYTE, LPDWORD,
	LPDWORD, LPBYTE, LPDWORD, LPDWORD, BOOL);

static BOOL LocateRecordUn (DMHANDLE, HANDLE, DWORD, DWORD);

BOOL DmScan (DMHANDLE, HANDLE);

static BOOL membcmp (LPBYTE, BYTE, DWORD);

static BOOL GetServerResource (DMHANDLE, DWORD);

static BOOL FreeServerResource (DMHANDLE, DWORD);


