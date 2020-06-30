/*	MultiSem.c

	Copyright (c) 1998, Johnson M. Hart
	Permission is granted for any and all use providing that this copyright is
	properly acknowledged.
	There are no assurances of suitability for any use whatsosever.
		UPDATED Feb 5, 2004. Fixed several errors
			Thanks to Daniel Jiang for pointing out a number of issues.
	Library of synchronization "objects" to supplement the standard Win32
	events and mutexs. 
	
	For simplicity, this implementation shares a header file (Synchronize.h) with
	another library of synchronization objects (EvMxSem.c) that is designed
	to operate under Windows CE, which does not have semaphores.

	Implementation notes: 
	1.	All required internal data structures are allocated on the process's heap
	2.	Where appropriate, a new error code is returned (see the header
		file), or, if the error is a Win32 error, that code is unchanged.
	3.	Notice the new handle type, "SYNCHHANDLE" which has handles, counters,
		and	other information. This structure will grow as new objects are added
		to this set; some members are specific to only one or two of the objects;
		in particular, the structure is more general than is required just for
		some of the specific examples.
	4.	Mutexs are used for critical sections. These could be replaced with
		CRITICAL_SECTION objects if the objects are used totally within
		a process. Even better, if the object is not named, then you 
		know that it will be used only within the process and you could
		make the decision at create time. HOWEVER, you will loose the timeout
		option if you do so, AND you will also loose the naming and interprocess
		capabilities. 
	5.	These simulated semaphores can be named and hence can be shared
		between processes.
	6.	No Win32 semaphores are used, so this implementation (with null names)
		will operate under Windows CE.
	7.	The implementation shows serveral interesting aspect of synchronization, some
		of which are specific to Win32 and some of which are general. There are pointed
		out in the comments as appropriate.
	8.	These objects have a WaitForSingleObject equivalent. There is, however, no
		equivalent to WaitForMultipleObjects as this is very difficult, if not impossible
		to do efficiently outside of the kernel.
*/

#include "Everything.h"
#include "Synchronize.h"

static SYNCHHANDLE CleanUp (SYNCHHANDLE);
static SYNCHHANDLE AllocSynchHandle (LPCTSTR, LPTSTR, LPTSTR, LPTSTR, LPBOOL, LONG, LONG, BOOL);
static HANDLE hSynchDB;	/*  Mutex to protect the entire synchronization object database */


/*********************************************
	MULTIPLE WAIT SEMAPHORE function family.
	These semaphores allow a thread to wait atomically for several "units"
	rather than just one. The DEFAULT wait semantics are considered to be 
	"First Satisfiable". That is, a thread with a satisfiable request
	will be released even if a thread (regardless of priority) has an
	outstanding request that is not currently satisfiable.

	Optional semantics, specified at create time, are
	"First Come, First Served". A thread with request larger than the
	number of units currently available will block all subsequent
	threads, even those with satisfiable requests or those of
	higher priority. The solution on p. 235 is a First Come, First Served
	solution, but here we implement it within the general framework.
*/

SYNCHHANDLE CreateSemaphoreMW (

	LPSECURITY_ATTRIBUTES  lpSemaphoreAttributes,	/* pointer to security attributes */
    LONG  lInitialCount,	/* initial count */
    LONG  lMaximumCount,	/* maximum count */
	BOOL  fFirstComeFirstServed,	/*	First Satisfiable is the default */
    LPCTSTR  lpName )

/*	Multiple wait semaphore creation. 
	Requires a counter, a mutex to protect the semaphore state, and an
	autoreset event.

	Here are the rules that must always hold between the autoreset event
	and the mutex (any violation of these rules by the multiple wait semaphore
	functions will, in all liklihood, result in a defect):
		1.	No thread can set, pulse, or reset the event,
			nor can it access any part of the SYNCHHANDLE structure,
			without first gaining ownership of the mutex.
			BUT, a thread can wait on the event without owning the mutex
			(this is clearly necessary or else the event could never be set).
			This is a "signal" Condition Variable model (AR event, SetEvent)
		2.	The event is set to a signaled state if and only if the count is
			greater than zero. To assure this property, the count should
			be checked after every semaphore decrease.
		3.	The semaphore count is always >= 0 and <= the maximum count
*/
{
	SYNCHHANDLE hSynch = NULL, hShare = NULL;
	TCHAR MutexName [MAX_PATH] = _T(""), EventName [MAX_PATH] = _T(""),
		MutexaName[MAX_PATH] = _T("");
	BOOL NewObject;

	if (lInitialCount > lMaximumCount || lMaximumCount < 0 || lInitialCount < 0) {
		/* Bad parameters */
		SetLastError (SYNCH_ERROR);
		return NULL;
	}

	hSynch = AllocSynchHandle (lpName, MutexName, EventName, MutexaName, &NewObject,
			lMaximumCount, lInitialCount, fFirstComeFirstServed);
	if (hSynch == NULL) {
		SetLastError (SYNCH_ERROR);
		return NULL;
	}

	/*	Create the object handles. These are always created in the process's
		local handle. */
	
	hSynch->hMutex = CreateMutex (lpSemaphoreAttributes, FALSE, (LPCTSTR)MutexName);

	/*	Create the event. It is initially signalled if and only if the
		initial count is > 0 */
	hSynch->hEvent = CreateEvent (lpSemaphoreAttributes, FALSE /*  autoreset */, 
		lInitialCount > 0, (LPCTSTR)EventName);

	hSynch->hMutexa = NULL;
	hSynch->dwFlags = 6; /* An event and a mutex, but no secondary mutex
		unless it is a First Come, First Served Multiple Wait semaphore */
	if (fFirstComeFirstServed) {
		hSynch->hMutexa = CreateMutex (lpSemaphoreAttributes, FALSE, (LPCTSTR)MutexaName);
		hSynch->dwFlags = 7; /*  All three objects were created */
	}

	/*	Set the object state, always in the local handle (for quick reference)
		and in the shared handle if there is one. */

	hSynch->MaxCount = lMaximumCount;
	hSynch->CurCount = lInitialCount;  /* The local value is not maintaned. */
	hSynch->fFirstComeFirstServed = fFirstComeFirstServed;
	_tcscpy (hSynch->lpName, lpName);

	/*  Return with the handle, or, if there was any error, return
		a null after closing any open handles and freeing any allocated memory */
	return CleanUp (hSynch);	
}


BOOL ReleaseSemaphoreMW (SYNCHHANDLE hSemMW, LONG cReleaseCount, LPLONG lpPreviousCount)
/*	Multiple wait equivalent to ReleaseSemaphore */
{
	BOOL Result = TRUE;
	SYNCHHANDLE hState;

	/*	Gain access to the object to assure that the release count
		would not cause the total count to exceed the maximum */

	/*	The state is maintained locally if the object is unnamed and
		in shared memory for a named object */

	hState = (hSemMW->SharedHandle == NULL) ? hSemMW : hSemMW->SharedHandle;
	__try {
		WaitForSingleObject (hSemMW->hMutex, INFINITE);
		*lpPreviousCount = hState->CurCount;
		if (hState->CurCount + cReleaseCount > hState->MaxCount || cReleaseCount <= 0) {
			SetLastError (SYNCH_ERROR);
			Result = FALSE;
			_leave;
		}
		hState->CurCount += cReleaseCount;

		/*	Set the autoreset event. All threads currently waiting on the
			event will be released, and then the event will be reset. */
		SetEvent (hSemMW->hEvent);
	}
	__finally {
		ReleaseMutex (hSemMW->hMutex);
	}
	return Result;
}

DWORD WaitForSemaphoreMW (SYNCHHANDLE hSemMW, LONG cSemRequest, DWORD dwMilliseconds)
/*	Multiple wait semaphore equivalent of WaitForSingleObject.
	The second parameter is the number of units requested, and the waiting will be
	either first come, first served or first available, depending on the option
	selected at create time. */
{
	DWORD WaitResult, Timeout = SYNCH_DEFAULT_TIMEOUT;
	SYNCHHANDLE hState;

	/*	The state is maintained locally if the object is unnamed and
		in shared memory for a named object */

	if (dwMilliseconds != INFINITE) Timeout = dwMilliseconds;

	hState = (hSemMW->SharedHandle == NULL) ? hSemMW : hSemMW->SharedHandle;

	if (cSemRequest <= 0 || cSemRequest > hState->MaxCount) {
		SetLastError (SYNCH_ERROR);
		return WAIT_FAILED;
	}

	/*	If this is a First Come, First Served MW semaphore, then this thread
		seizes the secondary mutex to block all other threads.
		Do this BEFORE waiting on the mutex protecting the semaphore state. */
	if (hSemMW->fFirstComeFirstServed) {
		WaitResult = WaitForSingleObject (hSemMW->hMutexa, dwMilliseconds);
		if (WaitResult != WAIT_OBJECT_0 && WaitResult != WAIT_ABANDONED_0) return WaitResult;
	}

	WaitResult = WaitForSingleObject (hSemMW->hMutex, dwMilliseconds);
	if (WaitResult != WAIT_OBJECT_0 && WaitResult != WAIT_ABANDONED_0) return WaitResult;


	while (hState->CurCount < cSemRequest) { 
		/*	The count is less than the number requested.
			The thread must wait on the event (which, by the rules, is currently reset)
			for semaphore resources to become available. First, of course, the mutex
			must be released so that another thread will be capable of setting the event.
		*/
		/*	Wait for the event, indicating that some other thread has increased
			the semaphore count.
			The event is autoreset and signaled with a SetEvent (not PulseEvent)
			so only one thread (whether currently waiting or arriving in the future)
			will be released. An alternative would be to set a manual reset
			event and reset the event immediately after the event wait.
			A finite timeout is required in the following wait to avoid a lost
			signal. Win32 does not have the condition variable concept where the
			Release-Wait is atomic; it should. Notice that this timeout is not
			required if cSemRequest == 1.
		*/
		WaitResult = SignalObjectAndWait (hSemMW->hMutex, hSemMW->hEvent, INFINITE, FALSE);
//		ReleaseMutex (hSemMW->hMutex);
//		WaitResult = WaitForSingleObject (hSemMW->hEvent, Timeout);
		if (WaitResult != WAIT_OBJECT_0 && WaitResult != WAIT_TIMEOUT) return WaitResult;

		/*	Seize the semaphore so that the semaphore state can be retested
			at the top of the loop. Note that there may be other threads
			waiting at this same point, but only one at a time can test the
			semaphore count. */
		WaitResult = WaitForSingleObject (hSemMW->hMutex, dwMilliseconds);
		if (WaitResult != WAIT_OBJECT_0 && WaitResult != WAIT_ABANDONED_0) return WaitResult;
		if (hState->CurCount > 0) SetEvent (hSemMW->hEvent);  
	}
	
	/*	hState->CurCount >= cSemRequest (the request can be satisfied), and
		this thread owns the mutex */
	hState->CurCount -= cSemRequest;
	if (hState->CurCount > 0) SetEvent (hSemMW->hEvent);
	ReleaseMutex (hSemMW->hMutex);
	if (hSemMW->fFirstComeFirstServed) ReleaseMutex (hSemMW->hMutexa);
	
	return WaitResult;

}


BOOL CloseSynchHandle (SYNCHHANDLE hSynch)
/*	Close a synchronization handle. 
	Improvement: Test for a valid handle before closing the handle */
{
	BOOL Result = TRUE;

	if (hSynch->hEvent  != NULL) Result = Result && CloseHandle (hSynch->hEvent);
	if (hSynch->hMutex  != NULL) Result = Result && CloseHandle (hSynch->hMutex);
	if (hSynch->hMutexa != NULL) Result = Result && CloseHandle (hSynch->hMutexa);
	if (hSynch->SharedHandle != NULL) { 
		WaitForSingleObject (hSynchDB, INFINITE);  /* Lock the shared memory structure 
													* using a shared, named mutex */
		InterlockedDecrement (&(hSynch->SharedHandle->RefCount));
		if (hSynch->SharedHandle->RefCount == 0) {
			/* Free the slot for re-use */
			memset (hSynch->SharedHandle, 0, SYNCH_HANDLE_SIZE);
		}
		ReleaseMutex (hSynchDB);
	}
	if (hSynch->ViewOfFile != NULL) UnmapViewOfFile (hSynch->ViewOfFile);
	HeapFree (GetProcessHeap (), 0, hSynch);
	return (Result);
}


static SYNCHHANDLE CleanUp (SYNCHHANDLE hSynch)
{	/*	Prepare to return from a create of a synchronization handle.
		If there was any failure, free any allocated resouces
		"Flags" indicates which Win32 objects are required in the 
		synchronization handle */
	BOOL ok = TRUE;
	DWORD Flags;

	if (hSynch == NULL)	return NULL;
	Flags = hSynch->dwFlags;
	if ((Flags & 4) == 1 && hSynch->hEvent ==  NULL) ok = FALSE;
	if ((Flags & 2) == 1 && hSynch->hMutex ==  NULL) ok = FALSE;
	if ((Flags & 1) == 1 && hSynch->hMutexa == NULL) ok = FALSE;
	if (!ok) {
		CloseSynchHandle (hSynch);
		return NULL;
	}

	/*	Everything worked */
	return hSynch;
}


static SYNCHHANDLE AllocSynchHandle (LPCTSTR lpName, 
	LPTSTR MutexName, LPTSTR EventName, LPTSTR MutexaName,
	LPBOOL pfNewObject, LONG MaxCount, LONG InitCount, BOOL fFCFS)
/*	Allocate memory for a synchronization handle. Unnamed objects
	have their handles created directly in the process heap, whereas
	named objects have two handles, one allocated locally to
	contain the handles (which are local to this process) and
	out of a shared memory pool mapped to the paging file for
	the shared object state.
	Also, create the names for the three internal objects
	and determine if this is a new object that should be initialized. */
{
	HANDLE hSynchDB;	/*  Mutex to protect the entire synchronization object database */
	HANDLE hMap, hFile = (HANDLE)(-1);
						/*	Shared memory and file handle for maintaining synch objects */
	BOOL FirstTime;
	SYNCHHANDLE pView, pNew = NULL, pFirstFree, hLocal;

	hLocal = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, SYNCH_HANDLE_SIZE);
	if (hLocal == NULL) return NULL;
	*pfNewObject = TRUE;
	if (lpName == NULL || _tcscmp (lpName, _T("")) == 0) {
		/*  The object is not named */
		hLocal->SharedHandle = NULL;
		return hLocal;
	}

	/*	The object is named. Create names for the internal objects. */
	_stprintf (MutexName,  _T("%s%s"), lpName, _T(".mtx"));
	_stprintf (EventName,  _T("%s%s"), lpName, _T(".evt"));
	_stprintf (MutexaName, _T("%s%s"), lpName, _T(".mtxa"));
	
	/*	Lock access to the synchronization object data base to prevent other threads
		from concurrently creating another object of the same name.
		All processes and threads use this same well-known mutex name. */
	
	hSynchDB = CreateMutex (NULL, FALSE, SYNCH_OBJECT_MUTEX);
	WaitForSingleObject (hSynchDB, INFINITE);

	/*  Access the shared memory where the synchronization objects are maintained.
		It is necessary, however, first to check if this is the first time
		that an object has been created so that the shared memory-mapped 
		table can be initialized. 
		The test is achieved with an OpenFileMapping call.  */
	
	__try {
		hMap = OpenFileMapping (FILE_MAP_WRITE, FALSE, SYNCH_FM_NAME);
		FirstTime = (hMap == NULL);
		if (FirstTime)
			hMap = CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, SIZE_SYNCH_DB, SYNCH_FM_NAME);
		if (hMap == NULL) _leave;
		pView = (SYNCHHANDLE)MapViewOfFile (hMap, FILE_MAP_WRITE, 0, 0, SIZE_SYNCH_DB);
		if (pView == NULL) _leave;
		if (FirstTime) memset (pView, 0, SIZE_SYNCH_DB);

		/*	Search to see if an object of this name already exists.
			The entry the mapped record is used for bookkeeping, in
			case it is ever needed in the future. An empty slot
			is detected by a 0 reference count. */
		pFirstFree = NULL;
		for (pNew = pView+1; pNew < pView + SYNCH_MAX_NUMBER; pNew++) {
			if ((pFirstFree == NULL) && (pNew->RefCount <= 0)) pFirstFree = pNew;
			if ((pNew->lpName != NULL) 
				&& _tcscmp (pNew->lpName, lpName) == 0) break; /* Name exists */
		}

		if (pNew < pView + SYNCH_MAX_NUMBER) { /* The name exists */
			*pfNewObject = FALSE;
		} else if (pFirstFree != NULL) {
			/*	The name does not exist, but we have found and empty slot. */
			*pfNewObject = TRUE;
			pNew = pFirstFree;
		} else { /*  The name does not exist, but there is no free slot. */
			pNew = NULL;
			*pfNewObject = FALSE;  /* It's not possible to create a new object */
		}
		if (*pfNewObject && pNew != NULL ) { 
			/*  There is a new shared handle. Set the state if it is new */
			pNew->MaxCount = MaxCount;
			pNew->CurCount = InitCount;  /* The local value is not maintaned. */
			pNew->fFirstComeFirstServed = fFCFS;
			_tcscpy (pNew->lpName, lpName);
		}
	}

	__finally {
		if (pNew != NULL) hLocal->ViewOfFile = pView;
		hLocal->SharedHandle = pNew;
		if (hLocal->SharedHandle != NULL) 
			InterlockedIncrement (&(hLocal->SharedHandle->RefCount)); /* Interlocked is redundant */
		ReleaseMutex (hSynchDB);
	}
	return hLocal;
}

