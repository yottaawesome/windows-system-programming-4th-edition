#ifndef __SYNCH_OBJ_H
#define __SYNCH_OBJ_H
/* CRITICAL_SECTION Version */

#define CV_TIMEOUT 25  /* tunable parameter for the CV model */

/* THRESHOLD oBarrier - TYPE DEFINITION AND FUNCTIONS */
typedef struct THRESHOLD_BARRIER_TAG { 	/* Threshold oBarrier */
	HANDLE bGuard;	/* mutex for the object */
	HANDLE bEvent;	/* auto reset event: bCount >= bThreshold */
	DWORD bCount;		/* number of threads that have reaached the oBarrier */
	DWORD bThreshold;	/* oBarrier threshold */
} THRESHOLD_BARRIER, *THB_OBJECT;

DWORD CreateThresholdoBarrier (THB_OBJECT *, DWORD /* threshold */);
DWORD WaitThresholdoBarrier (THB_OBJECT);
DWORD CloseThresholdoBarrier (THB_OBJECT);

/*  SEMAPHORE - TYPE DEFINITION AND FUNCTIONS */
typedef struct ethread_semaphore_tag { 	/* Semaphore */
	CRITICAL_SECTION s_guard;	/* CRITICAL_SECTION for the object */
	HANDLE s_fcfs;		/* Used for fcfs semaphores */
	HANDLE s_broadcast;	/* s_count has increased */
	DWORD s_max;	/* Maximum semaphore count */
	DWORD s_count;	/* Current semaphore count */
	int f_fcfs;		/* First come, first served, or first fit */
} ethread_semaphore_t;

/* Definitions of a sychronized, general bounded queue structure. */
/* Queues are implemented as arrays with indices to youngest */
/* and oldest messages, with wrap around. 					*/
/* Each queue also contains a guard mutex and			*/
/* "not empty" and "not full" condition variables.		*/
/* Finally, there is a pointer to an array of messages of	*/
/* arbitrary type						*/

#if (defined(USE_WINDOWS_CV))
typedef struct QUEUE_OBJECT_TAG { 	/* General purpose queue 	*/
	CRITICAL_SECTION	qGuard;/* Guard the message block	*/
	CONDITION_VARIABLE	qNe;	/* Queue is not empty		*/
	CONDITION_VARIABLE	qNf;	/* Queue is not full		*/
	DWORD	qSize;	/* Queue max size size		*/
	DWORD	qFirst;	/* Index of oldest message	*/
	DWORD	qLast;	/* Index of youngest msg	*/
	DWORD	QueueDestroyed;/* Q receiver has terminated	*/
	char	*msgArray;	/* array of qSize messages	*/
} QUEUE_OBJECT;
#else
typedef struct QUEUE_OBJECT_TAG_T { 	/* General purpose queue 	*/
	CRITICAL_SECTION	qGuard;/* Guard the message block	*/
	HANDLE	qNe;	/* Queue is not empty		*/
	HANDLE	qNf;	/* Queue is not full		*/
	DWORD qSize;	/* Queue max size size		*/
	DWORD qFirst;	/* Index of oldest message	*/
	DWORD qLast;	/* Index of youngest msg	*/
	DWORD QueueDestroyed;/* Q receiver has terminated	*/
	char	*msgArray;	/* array of qSize messages	*/
} QUEUE_OBJECT;
#endif

/* Queue management functions */
DWORD QueueInitialize (QUEUE_OBJECT *, DWORD, DWORD);
DWORD QueueDestroy (QUEUE_OBJECT *);
DWORD QueueDestroyed (QUEUE_OBJECT *);
DWORD QueueEmpty (QUEUE_OBJECT *);
DWORD QueueFull (QUEUE_OBJECT *);
DWORD QueueGet (QUEUE_OBJECT *, PVOID, DWORD, DWORD);
DWORD QueuePut (QUEUE_OBJECT *, PVOID, DWORD, DWORD);
DWORD QueueRemove (QUEUE_OBJECT *, PVOID, DWORD);
DWORD QueueInsert (QUEUE_OBJECT *, PVOID, DWORD);

#endif
