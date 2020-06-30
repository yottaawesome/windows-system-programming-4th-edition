/*	Synchronize.h - header file to go with Synchize.c */

typedef struct _SYNCH_HANDLE_STRUCTURE {
	HANDLE hEvent;
	HANDLE hMutex;
	HANDLE hMutexa;
	LONG MaxCount;
	volatile LONG CurCount;			/*  Multiple threads will alter this value */
	LONG RefCount;					/*  Number of references to a shared handle */
	BOOL fFirstComeFirstServed;		/*  For multiple wait semaphores */
	DWORD dwFlags;					/*  Used as requred */
	LPVOID ViewOfFile;				/*	For named objects, this is the mapped file
										view containing the handle */
	struct _SYNCH_HANDLE_STRUCTURE *SharedHandle; /*  For named objects, this is the 
										   shared part of the handle, containing
										   the handle state */
	TCHAR lpName[MAX_PATH];
} SYNCH_HANDLE_STRUCTURE, *SYNCHHANDLE;

#define SYNCH_HANDLE_SIZE sizeof (SYNCH_HANDLE_STRUCTURE)
#define SYNCH_MAX_NUMBER 1000
#define SIZE_SYNCH_DB SYNCH_MAX_NUMBER*SYNCH_HANDLE_SIZE

#define SYNCH_OBJECT_MUTEX _T("SynchObjectMutex")
#define SYNCH_FM_NAME _T("SynchSharedMem")

#define SYNCH_DEFAULT_TIMEOUT 50   /* required to avoid a lost signal */

/*	Error codes - all must have bit 29 set */
#define SYNCH_ERROR 0X20000000		/* EXERCISE - REFINE THE ERROR NUMBERS */

SYNCHHANDLE CreateSemaphoreCE (LPSECURITY_ATTRIBUTES, LONG, LONG, LPCTSTR);

BOOL ReleaseSemaphoreCE (SYNCHHANDLE, LONG, LPLONG);

DWORD WaitForSemaphoreCE (SYNCHHANDLE, DWORD);

BOOL CloseSynchHandle (SYNCHHANDLE);

SYNCHHANDLE CreateSemaphoreMW (LPSECURITY_ATTRIBUTES, LONG, LONG,BOOL, LPCTSTR);

BOOL ReleaseSemaphoreMW (SYNCHHANDLE, LONG, LPLONG);

DWORD WaitForSemaphoreMW (SYNCHHANDLE, LONG, DWORD);
