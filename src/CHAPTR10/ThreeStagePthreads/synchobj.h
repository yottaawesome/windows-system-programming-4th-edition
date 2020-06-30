#ifndef __synch_obj_h
#define __synch_obj_h

#define CV_TIMEOUT 5  /* tunable parameter for the CV model */


/* THRESHOLD BARRIER - TYPE DEFINITION AND FUNCTIONS */
typedef struct THRESHOLD_BARRIER_TAG { 	/* Threshold barrier */
	pthread_mutex_t b_guard;	/* mutex for the object */
	pthread_cond_t b_broadcast;	/* b_count >= b_threshold */
	volatile unsigned int b_destroyed;	/* Is it valid */
	volatile unsigned int b_count;		/* number of threads that have reaached the barrier */
	volatile unsigned int b_threshold;	/* barrier threshold */
} THRESHOLD_BARRIER, *THB_HANDLE;

unsigned int CreateThresholdBarrier (THB_HANDLE *, unsigned int /* threshold */);
unsigned int WaitThresholdBarrier (THB_HANDLE);
unsigned int CloseThresholdBarrier (THB_HANDLE);

/*  SEMAPHORE - TYPE DEFINITION AND FUNCTIONS */
typedef struct ethread_semaphore_tag { 	/* Semaphore */
	pthread_mutex_t s_guard;	/* mutex for the object */
	pthread_cond_t s_fcfs;		/* Used for fcfs semaphores */
	pthread_cond_t s_broadcast;	/* s_count has increased */
	volatile unsigned int s_max;	/* Maximum semaphore count */
	volatile unsigned int s_count;	/* Current semaphore count */
	volatile int f_fcfs;		/* First come, first served, or first fit */
} ethread_semaphore_t;

/* Definitions of a sychronized, general bounded queue structure. */
/* Queues are implemented as arrays with indices to youngest */
/* and oldest messages, with wrap around. 					*/
/* Each queue also contains a guard mutex and			*/
/* "not empty" and "not full" condition variables.		*/
/* Finally, there is a pointer to an array of messages of	*/
/* arbitrary type						*/

typedef struct queue_tag { 	/* General purpose queue 	*/
	pthread_mutex_t	q_guard;/* Guard the message block	*/
	pthread_cond_t	q_ne;	/* Event: Queue is not empty		*/
	pthread_cond_t	q_nf;	/* Event: Queue is not full			*/
					/* These two events are manual-reset for the broadcast model
					 * and auto-reset for the signal model */
	volatile unsigned int q_size;	/* Queue max size size		*/
	volatile unsigned int q_first;	/* Index of oldest message	*/
	volatile unsigned int q_last;	/* Index of youngest msg	*/
	volatile unsigned int q_destroyed;/* Q receiver has terminated	*/
	void *	msg_array;	/* array of q_size messages	*/
} queue_t;

/* Queue management functions */
unsigned int q_initialize (queue_t *, unsigned int, unsigned int);
unsigned int q_destroy (queue_t *);
unsigned int q_destroyed (queue_t *);
unsigned int q_empty (queue_t *);
unsigned int q_full (queue_t *);
unsigned int q_get (queue_t *, void *, unsigned int, unsigned int);
unsigned int q_put (queue_t *, void *, unsigned int, unsigned int);
unsigned int q_remove (queue_t *, void *, unsigned int);
unsigned int q_insert (queue_t *, void *, unsigned int);

#endif
