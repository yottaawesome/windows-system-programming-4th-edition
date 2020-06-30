/* Session 6, Chapter 10. ThreeStage.c										*/
/* Three-stage Producer Consumer system							*/
/* Other files required in this project, either directly or		*/
/* in the form of libraries (DLLs are preferable)				*/
/*		QueueObj.c												*/
/*		Messages.c												*/
/*																*/
/* Usage: ThreeStage npc goal [display]							*/
/* start up "npc" paired producer  and consumer threads.		*/
/*          Display messages if "display" is non-zero			*/
/* Each producer must produce a total of						*/
/* "goal" messages, where each message is tagged				*/
/* with the consumer that should receive it						*/
/* Messages are sent to a "transmitter thread" which performs	*/
/* additional processing before sending message groups to the	*/
/* "receiver thread." Finally, the receiver thread sends		*/
/* the messages to the consumer threads.						*/

/* Transmitter: Receive messages one at a time from producers,	*/
/* create a trasmission message of up to "TBLOCK_SIZE" messages	*/
/* to be sent to the Receiver. (this could be a network xfer	*/
/* Receiver: Take message blocks sent by the Transmitter		*/
/* and send the individual messages to the designated consumer	*/

#if defined(WIN32)
#include <WINDOWS.H>
#define sleep(i) Sleep(i*1000)
#endif 

#include "pthread.h"
#include "messages.h"
#include "SynchObj.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define DELAY_COUNT 1000
#define MAX_THREADS 1024

/* Queue lengths and blocking factors. These numbers are arbitrary and 	*/
/* can be adjusted for performance tuning. The current values are	*/
/* not well balanced.							*/

#define TBLOCK_SIZE 5  	/* Transmitter combines this many messages at at time */
#define Q_TIMEOUT 2000 /* Transmiter and receiver timeout (ms) waiting for messages */
//#define Q_TIMEOUT INFINITE
#define MAX_RETRY 5  /* Number of q_get retries before quitting */
#define P2T_QLEN 10 	/* Producer to Transmitter queue length */
#define T2R_QLEN 4	/* Transmitter to Receiver queue length */
#define R2C_QLEN 4	/* Receiver to Consumer queue length - there is one
			 * such queue for each consumer */

void * producer (void *);
void * consumer (void *);
void * transmitter (void *);
void * receiver (void *);


typedef struct _THARG {
	volatile unsigned int thread_number;
	volatile unsigned int work_goal;    /* used by producers */
	volatile unsigned int work_done;    /* Used by producers and consumers */
} THARG;


/* Grouped messages sent by the transmitter to receiver		*/
typedef struct T2R_MSG_TYPEag {
	volatile unsigned int num_msgs; /* Number of messages contained	*/
	msg_block_t messages [TBLOCK_SIZE];
} T2R_MSG_TYPE;

queue_t p2tq, t2rq, *r2cq_array;

/* ShutDown, AllProduced are global flags to shut down the system & transmitter */
static volatile unsigned int ShutDown = 0;
static volatile unsigned int AllProduced = 0;
static unsigned int DisplayMessages = 0;

unsigned int main (unsigned int argc, char * argv[])
{
	unsigned int tstatus = 0, nthread, ithread, goal, thid;
	pthread_t *producer_th, *consumer_th, transmitter_th, receiver_th;
	THARG *producer_arg, *consumer_arg;
	
	if (argc < 3) {
		printf ("Usage: ThreeStage npc goal \n");
		return 1;
	}
	if (argc >= 4) DisplayMessages = atoi(argv[3]);

	srand ((int)time(NULL));	/* Seed the RN generator */
	
	nthread = atoi(argv[1]);
	if (nthread > MAX_THREADS) {
		printf ("Maximum number of producers or consumers is %d.\n", MAX_THREADS);
		return 2;
	}
	goal = atoi(argv[2]);
	producer_th = malloc (nthread * sizeof(pthread_t));
	producer_arg = calloc (nthread, sizeof (THARG));
	consumer_th = malloc (nthread * sizeof(pthread_t));
	consumer_arg = calloc (nthread, sizeof (THARG));
	
	if (producer_th == NULL || producer_arg == NULL
	 || consumer_th == NULL || consumer_arg == NULL)
		perror ("Cannot allocate working memory for threads.");
	
	q_initialize (&p2tq, sizeof(msg_block_t), P2T_QLEN);
	q_initialize (&t2rq, sizeof(T2R_MSG_TYPE), T2R_QLEN);
	/* Allocate and initialize Receiver to Consumer queue for each consumer */
	r2cq_array = calloc (nthread, sizeof(queue_t));
	if (r2cq_array == NULL) perror ("Cannot allocate memory for r2c queues");
	
	for (ithread = 0; ithread < nthread; ithread++) {
		/* Initialize r2c queue for this consumer thread */
		q_initialize (&r2cq_array[ithread], sizeof(msg_block_t), R2C_QLEN);
		/* Fill in the thread arg */
		consumer_arg[ithread].thread_number = ithread;
		consumer_arg[ithread].work_goal = goal;
		consumer_arg[ithread].work_done = 0;

		tstatus = pthread_create (&consumer_th[ithread], NULL,
				consumer, (void *)&consumer_arg[ithread]);
		if (tstatus != 0) 
			perror ("Cannot create consumer thread");

		producer_arg[ithread].thread_number = ithread;
		producer_arg[ithread].work_goal = goal;
		producer_arg[ithread].work_done = 0;
		tstatus = pthread_create (&producer_th[ithread], NULL,
			producer, (void *)&producer_arg[ithread]);
		if (tstatus != 0) 
			perror ("Cannot create producer thread");
	}

	tstatus = pthread_create (&transmitter_th, NULL, transmitter, &thid);
	if (tstatus != 0) 
		perror ("Cannot create tranmitter thread");
	tstatus = pthread_create (&receiver_th, NULL, receiver, &thid);
	if (tstatus != 0) 
		perror ("Cannot create receiver thread");
	

	printf ("BOSS: All threads are running\n");	
	/* Wait for the producers to complete */
	/* The implementation allows too many threads for WaitForMultipleObjects */
	/* although you could call WFMO in a loop */
	for (ithread = 0; ithread < nthread; ithread++) {
		tstatus = pthread_join (producer_th[ithread], NULL);
		if (tstatus != 0) 
			perror ("Cannot wait for producer thread");
		printf ("BOSS: Producer %d produced %d work units\n",
			ithread, producer_arg[ithread].work_done);
	}
	/* Producers have completed their work. */
	printf ("BOSS: All producers have completed their work.\n");
	AllProduced = 1;

	/* Wait for the consumers to complete */
	for (ithread = 0; ithread < nthread; ithread++) {
		tstatus = pthread_join (consumer_th[ithread], NULL);
		if (tstatus != 0) 
			perror ("Cannot wait for consumer thread");
		printf ("BOSS: consumer %d consumed %d work units\n",
			ithread, consumer_arg[ithread].work_done);
	}
	printf ("BOSS: All consumers have completed their work.\n");	

	ShutDown = 1; /* Set a shutdown flag - All messages have been consumed */

	/* Wait for the transmitter and receiver */

	tstatus = pthread_join (transmitter_th, NULL);
	if (tstatus != 0) 
		perror ("Failed waiting for transmitter");
	tstatus = pthread_join (receiver_th, NULL);
	if (tstatus != 0) 
		perror ("Failed waiting for receiver");

	q_destroy (&p2tq);
	q_destroy (&t2rq);
	for (ithread = 0; ithread < nthread; ithread++)
		q_destroy (&r2cq_array[ithread]);
	free (r2cq_array);
	free (producer_th); free (consumer_th);
	free (producer_arg); free(consumer_arg);
	printf ("System has finished. Shutting down\n");
	return 0;
}

void * producer (void * arg)
{
	THARG * parg;
	unsigned int ithread, tstatus = 0, AllProduced = 0;
	msg_block_t msg;
	
	parg = (THARG *)arg;	
	ithread = parg->thread_number;

	while (parg->work_done < parg->work_goal && !ShutDown) { 
		/* Periodically produce work units until the goal is satisfied */
		/* messages receive a source and destination address which are */
		/* the same in this case but could, in general, be different. */
		sleep (rand()/100000000);
		message_fill (&msg, ithread, ithread, parg->work_done);
		
		/* put the message in the queue - Use an infinite timeout to assure
		 * that the message is inserted, even if consumers are delayed */
		tstatus = q_put (&p2tq, &msg, sizeof(msg), INFINITE);
		if (0 == tstatus) {
			parg->work_done++;		
		}
	}

	return 0;		
}

void * consumer (void * arg)
{
	THARG * carg;
	unsigned int tstatus = 0, ithread, Retries = 0;
	msg_block_t msg;
	queue_t *pr2cq;

	carg = (THARG *) arg;
	ithread = carg->thread_number;
	
	carg = (THARG *)arg;	
	pr2cq = &r2cq_array[ithread];

	while (carg->work_done < carg->work_goal && Retries < MAX_RETRY && !ShutDown) { 
		/* Receive and display/process messages */
		/* Try to receive the requested number of messages,
		 * but allow for early system shutdown */
		
		tstatus = q_get (pr2cq, &msg, sizeof(msg), Q_TIMEOUT);
		if (0 == tstatus) {				
			if (DisplayMessages > 0) message_display (&msg);
			carg->work_done++;		
			Retries = 0;
		} else {
			Retries++;
		}
	}

	return NULL;
}

void * transmitter (void * arg)
{

	/* Obtain multiple producer messages, combining into a single	*/
	/* compound message for the receiver */

	unsigned int tstatus = 0, im, Retries = 0;
	T2R_MSG_TYPE t2r_msg = {0};
	msg_block_t p2t_msg;

	while (!ShutDown && !AllProduced) {
		t2r_msg.num_msgs = 0;
		/* pack the messages for transmission to the receiver */
		im = 0;
		while (im < TBLOCK_SIZE && !ShutDown && Retries < MAX_RETRY && !AllProduced) {
			tstatus = q_get (&p2tq, &p2t_msg, sizeof(p2t_msg), Q_TIMEOUT);
			if (0 == tstatus) {
				memcpy (&t2r_msg.messages[im], &p2t_msg, sizeof(p2t_msg));
				t2r_msg.num_msgs++;
				im++;
				Retries = 0;
			} else { /* Timed out.  */
				Retries++;
			}
		}
		tstatus = q_put (&t2rq, &t2r_msg, sizeof(t2r_msg), INFINITE);
		if (tstatus != 0) return NULL;
	}
	return NULL;
}


void * receiver (void * arg)
{
	/* Obtain compound messages from the transmitter and unblock them	*/
	/* and transmit to the designated consumer.				*/

	unsigned int tstatus = 0, im, ic, Retries = 0;
	T2R_MSG_TYPE t2r_msg;
	msg_block_t r2c_msg; 
	
	while (!ShutDown && Retries < MAX_RETRY) {
		tstatus = q_get (&t2rq, &t2r_msg, sizeof(t2r_msg), Q_TIMEOUT);
		if (tstatus != 0) { /* Timeout - Have the producers shut down? */
			Retries++;
			continue;
		}
		Retries = 0;
		/* Distribute the packaged messages to the proper consumer */
		im = 0;
		while (im < t2r_msg.num_msgs) {
			memcpy (&r2c_msg, &t2r_msg.messages[im], sizeof(r2c_msg));
			ic = r2c_msg.destination; /* Destination consumer */
			tstatus = q_put (&r2cq_array[ic], &r2c_msg, sizeof(r2c_msg), INFINITE);
			if (0 == tstatus) im++;
		}
	}
	return NULL;
}

