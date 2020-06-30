/* Session 6, Chapter 10. QueueObj.c										*/
/* Queue functions												*/

#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SynchObj.h"
#if (!defined INFINITE)
#define INFINITE 0xFFFFFFFF
#endif

/* Finite bounded queue management functions */
/* q_get, q_put timeouts (max_wait) are in ms - convert to sec, rounding up */
unsigned int q_get (queue_t *q, void * msg, unsigned int msize, unsigned int MaxWait)
{
	int tstatus = 0, got_msg = 0, time_inc = (MaxWait + 999) /1000;
	struct timespec timeout;
	timeout.tv_nsec = 0;

	if (q_destroyed(q)) return 1;
	pthread_mutex_lock (&q->q_guard);
	while (q_empty (q) && 0 == tstatus) {
		if (MaxWait != INFINITE) {
			timeout.tv_sec = time(NULL) + time_inc;
			tstatus = pthread_cond_timedwait (&q->q_ne, &q->q_guard, &timeout);
		} else {
			tstatus = pthread_cond_wait (&q->q_ne, &q->q_guard);
		}
	}
	/* remove the message, if any, from the queue */
	if (0 == tstatus && !q_empty (q)) { 
		q_remove (q, msg, msize);
		got_msg = 1;
		/* Signal that the queue is not full as we've removed a message */
		pthread_cond_broadcast (&q->q_nf);
	}
	pthread_mutex_unlock (&q->q_guard);
	return (0 == tstatus && got_msg == 1 ? 0 : max(1, tstatus));   /* 0 indictates success */
}

unsigned int q_put (queue_t *q, void * msg, unsigned int msize, unsigned int MaxWait)
{
	int tstatus = 0, put_msg = 0, time_inc = (MaxWait + 999) /1000;
	struct timespec timeout;
	timeout.tv_nsec = 0;

	if (q_destroyed(q)) return 1;
	pthread_mutex_lock (&q->q_guard);
	while (q_full (q) && 0 == tstatus) {
		if (MaxWait != INFINITE) {
			timeout.tv_sec = time(NULL) + time_inc;
			tstatus = pthread_cond_timedwait (&q->q_nf, &q->q_guard, &timeout);
		} else {
			tstatus = pthread_cond_wait (&q->q_nf, &q->q_guard);
		}	
	}
	/* Insert the message into the queue if there's room */
	if (0 == tstatus && !q_full (q)) {
		q_insert (q, msg, msize);
	    put_msg = 1;
		/* Signal that the queue is not empty as we've inserted a message */
		pthread_cond_broadcast (&q->q_ne);
	}
	pthread_mutex_unlock (&q->q_guard);
	return (0 == tstatus && put_msg == 1 ? 0 : max(1, tstatus));   /* 0 indictates success */
}

unsigned int q_initialize (queue_t *q, unsigned int msize, unsigned int nmsgs)
{
	/* Initialize queue, including its mutex and events */
	/* Allocate storage for all messages. */
	
	q->q_first = q->q_last = 0;
	q->q_size = nmsgs;
	q->q_destroyed = 0;

	pthread_mutex_init (&q->q_guard, NULL);
	pthread_cond_init (&q->q_ne, NULL);
	pthread_cond_init (&q->q_nf, NULL);

	if ((q->msg_array = calloc (nmsgs, msize)) == NULL) return 1;
	return 0; /* No error */
}

unsigned int q_destroy (queue_t *q)
{
	if (q_destroyed(q)) return 1;
	/* Free all the resources created by q_initialize */
	pthread_mutex_lock (&q->q_guard);
	q->q_destroyed = 1;
	free (q->msg_array);
	pthread_cond_destroy (&q->q_ne);
	pthread_cond_destroy (&q->q_nf);
	pthread_mutex_unlock (&q->q_guard);
	pthread_mutex_destroy (&q->q_guard);

	return 0;
}

unsigned int q_destroyed (queue_t *q)
{
	return (q->q_destroyed);
}

unsigned int q_empty (queue_t *q)
{
	return (q->q_first == q->q_last);
}

unsigned int q_full (queue_t *q)
{
	return ((q->q_first - q->q_last) == 1 ||
		    (q->q_last == q->q_size-1 && q->q_first == 0));
}


unsigned int q_remove (queue_t *q, void * msg, unsigned int msize)
{
	char *pm;
	
	pm = (char *)q->msg_array;
	/* Remove oldest ("first") message */
	memcpy (msg, pm + (q->q_first * msize), msize);
	// Invalidate the message
	q->q_first = ((q->q_first + 1) % q->q_size);
	return 0; /* no error */
}

unsigned int q_insert (queue_t *q, void * msg, unsigned int msize)
{
	char *pm;
	
	pm = (char *)q->msg_array;
	/* Add a new youngest ("last") message */
	if (q_full(q)) return 1; /* Error - Q is full */
	memcpy (pm + (q->q_last * msize), msg, msize);
	q->q_last = ((q->q_last + 1) % q->q_size);

	return 0;
}
