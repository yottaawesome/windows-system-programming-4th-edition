/*  Thread_emulation.h		*/
/*		Author: John Hart, July 27, 2000							*/
/*	Emulate the Pthreads model for both Win32 and Pthreads platforms*/
/*	The emulation is not complete, but it does provide a subset		*/
/*	required for the message Q implementation						*/
/*  IF YOU ARE REALLY SERIOUS ABOUT THIS, USE THE OPEN SOURCE       */
/*  PTHREAD LIBRARY. YOU'LL FIND IT ON THE RED HAT SITE             */
#ifndef _THREAD_EMULATION
#define _THREAD_EMULATION

/*	Thread management macros		*/
#ifdef _WINDOWS
/*		Win32		*/
#include <process.h>
#include <windows.h>
#define		THREAD_FUNCTION		DWORD WINAPI
#define		THREAD_FUNCTION_RETURN DWORD
#define		THREAD_SPECIFIC_INDEX DWORD
#define		pthread_t	HANDLE
#define		pthread_attr_t DWORD
#define		pthread_create(thhandle,attr,thfunc,tharg) ((thhandle=(HANDLE)_beginthreadex(NULL,0,thfunc,tharg,0, &ThId))==NULL)
#define		pthread_join(thread) ((WaitForSingleObject((thread),INFINITE)!=WAIT_OBJECT_0) || !CloseHandle(thread))
#define		pthread_detach(thread) if(thread!=NULL)CloseHandle(thread)
#define		thread_sleep(nms)	Sleep(nms)
#define		pthread_cancel(thread)	TerminateThread(thread,0)
#define		ts_key_create(ts_key, destructor) {ts_key = TlsAlloc();}
#define		pthread_getspecific(ts_key) TlsGetValue(ts_key)
#define		pthread_setspecific(ts_key, value) TlsSetValue(ts_key, (void *)value)
#define		pthread_self() GetCurrentThreadId()
#else
/*		pthreads	*/
/*		Nearly everything is already defined */
#define		THREAD_FUNCTION		void *
#define		THREAD_FUNCTION_RETURN void *
#define		THREAD_SPECIFIC_INDEX pthread_key_t
#define		thread_sleep(nms)	sleep((nms+500)/1000)
#define		ts_key_create(ts_key, destructor) pthread_key_create (&(ts_key), destructor);
#endif

/*	Syncrhronization macros: Win32 and Pthreads						*/

#ifdef _WINDOWS
//#define _WIN32_WINNT 0x400  // WINBASE.H
#define pthread_mutex_t					HANDLE
#define pthread_condvar_t				HANDLE
#define pthread_mutex_lock(object)		WaitForSingleObject(object, INFINITE)
#define pthread_mutex_unlock(object)		ReleaseMutex(object)
#define CV_TIMEOUT			100		/* Arbitrary value */
/* USE THE FOLLOWING FOR WINDOWS 9X */
/* For addtional explanation of the condition varialbe emulation and the use of the
 * timeout, see the paper "Batons: A Sequential Synchronization Object" 
 * by Andrew Tucker and Johnson M Hart. (Windows Developer’s Journal, 
 * July, 2001, pp24 ff. www.wdj.com). */
#define pthread_cond_wait(cv,mutex) 	{ReleaseMutex(mutex);WaitForSingleObject(cv,CV_TIMEOUT);WaitForSingleObject(mutex,INFINITE);};
/* You can use the following on Windows NT/2000/XP and avoid the timeout */
//#define pthread_cond_wait(cv,mutex) 	{SignalObjectAndWait(mutex,cv,INFINITE,FALSE);WaitForSingleObject(mutex,INFINITE);};
#define pthread_cond_broadcast(cv)		PulseEvent(cv)
static int						OnceFlag;
static DWORD ThId;  /* This is ugly, but is required on Win9x for _beginthreadex */
#else
/*	Not Windows. Assume pthreads */

#endif

#endif
