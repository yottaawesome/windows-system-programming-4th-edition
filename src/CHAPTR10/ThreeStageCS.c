/* Chapter 10. ThreeStage.c										*/
/* Three-stage Producer Consumer system							*/
/* Other files required in this project, either directly or		*/
/* in the form of libraries (DLLs are preferable)				*/
/*		QueueObj.c												*/
/*		Messages.c												*/
/*																*/
/*  Usage: ThreeStage npc goal  								*/
/* start up "npc" paired producer  and consumer threads.		*/
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

#include "Everything.h"
#include "SynchObjCS.h" /* This is the only change to ThreeStage.c */
#include "messages.h"
#include <time.h>

#define DELAY_COUNT 1000
#define MAX_THREADS 1024

/* Queue lengths and blocking factors. These numbers are arbitrary and 	*/
/* can be adjusted for performance tuning. The current values are	*/
/* not well balanced.							*/

#define TBLOCK_SIZE 5  	/* Transmitter combines this many messages at at time */
#define TBLOCK_TIMEOUT 50 /* Transmiter timeout waiting for messages to block */
#define P2T_QLEN 10 	/* Producer to Transmitter queue length */
#define T2R_QLEN 4	/* Transmitter to Receiver queue length */
#define R2C_QLEN 4	/* Receiver to Consumer queue length - there is one
			 * such queue for each consumer */

DWORD WINAPI producer (PVOID);
DWORD WINAPI consumer (PVOID);
DWORD WINAPI transmitter (PVOID);
DWORD WINAPI receiver (PVOID);

typedef struct THARG_TAG {
	DWORD threadNumber;
	DWORD workGoal;    /* used by producers */
	DWORD workDone;    /* Used by producers and consumers */
} THARG;

/* Grouped messages sent by the transmitter to receiver		*/
typedef struct T2R_MSG_OBJ_TAG {
	DWORD numMessages; /* Number of messages contained	*/
	MSG_BLOCK messages [TBLOCK_SIZE];
} T2R_MSG_OBJ;

/* Argument for transmitter and receiver threads */
typedef struct TR_ARG_TAG {
	DWORD nProducers;  /* Number of active producers */
} TR_ARG;

QUEUE_OBJECT p2tq, t2rq, *r2cqArray;

static volatile DWORD ShutDown = 0;
static DWORD EventTimeout = 50;
DWORD trace = 0;


int _tmain (int argc, LPTSTR argv[])
{
	DWORD tStatus, nThread, iThread, goal;
	HANDLE *producerThreadArray, *consumerThreadArray, transmitterThread, receiverThread;
	THARG *producerArg, *consumerArg;
	TR_ARG transmitterArg, receiverArg;

#if (defined (REQUIRES_NT6))
	if (!WindowsVersionOK (6, 0)) 
		ReportError (_T("This program requires Windows NT 6.0 or greater"), 1, TRUE);
#elif (!defined (NO_SOAW))
	if (!WindowsVersionOK (4, 0)) 
		ReportError (_T("This program requires Windows NT 4.0 or greater"), 1, FALSE);
#endif

	if (argc < 3) {
		_tprintf (_T("Usage: ThreeStage npc goal [trace]\n"));
		return 1;
	}
	srand ((int)time(NULL));	/* Seed the RN generator */
	
	nThread = _ttoi(argv[1]);
	if (nThread > MAX_THREADS) {
		_tprintf (_T("Maximum number of producers or consumers is %d.\n"), MAX_THREADS);
		return 2;
	}
	receiverArg.nProducers = transmitterArg.nProducers = nThread;

	goal = _ttoi(argv[2]);
	if (argc >= 4) trace = _ttoi(argv[3]);

	producerThreadArray = malloc (nThread * sizeof(HANDLE));
	producerArg = calloc (nThread, sizeof (THARG));
	consumerThreadArray = malloc (nThread * sizeof(HANDLE));
	consumerArg = calloc (nThread, sizeof (THARG));
	
	if (producerThreadArray == NULL || producerArg == NULL
	 || consumerThreadArray == NULL || consumerArg == NULL)
		ReportError (_T("Cannot allocate working memory for threads."), 1, FALSE);
	
	QueueInitialize (&p2tq, sizeof(MSG_BLOCK), P2T_QLEN);
	QueueInitialize (&t2rq, sizeof(T2R_MSG_OBJ), T2R_QLEN);
	/* Allocate and initialize Receiver to Consumer queue for each consumer */
	r2cqArray = calloc (nThread, sizeof(QUEUE_OBJECT));
	if (r2cqArray == NULL) ReportError (_T("Cannot allocate memory for r2c queues"), 
					20, FALSE);
	
	for (iThread = 0; iThread < nThread; iThread++) {
		/* Initialize r2c queue for this consumer thread */
		QueueInitialize (&r2cqArray[iThread], sizeof(MSG_BLOCK), R2C_QLEN);
		/* Fill in the thread arg */
		consumerArg[iThread].threadNumber = iThread;
		consumerArg[iThread].workGoal = goal;
		consumerArg[iThread].workDone = 0;

		consumerThreadArray[iThread] = (HANDLE)_beginthreadex (NULL, 0,
				consumer, &consumerArg[iThread], 0, NULL);
		if (consumerThreadArray[iThread] == NULL) 
			ReportError (_T("Cannot create consumer thread"), 2, TRUE);

		producerArg[iThread].threadNumber = iThread;
		producerArg[iThread].workGoal = goal;
		producerArg[iThread].workDone = 0;
		producerThreadArray[iThread] = (HANDLE)_beginthreadex (NULL, 0,
			producer, &producerArg[iThread], 0, NULL);
		if (producerThreadArray[iThread] == NULL) 
			ReportError (_T("Cannot create producer thread"), 3, TRUE);
	}

	transmitterThread = (HANDLE)_beginthreadex (NULL, 0, transmitter, &transmitterArg, 0, NULL);
	if (transmitterThread == NULL) 
		ReportError (_T("Cannot create tranmitter thread"), 4, TRUE);
	receiverThread = (HANDLE)_beginthreadex (NULL, 0, receiver, &receiverArg, 0, NULL);
	if (receiverThread == NULL) 
		ReportError (_T("Cannot create receiver thread"), 5, TRUE);
	
	_tprintf (_T("BOSS: All threads are running\n"));	
	/* Wait for the producers to complete */
	/* The implementation allows too many threads for WaitForMultipleObjects */
	/* although you could call WFMO in a loop */
	for (iThread = 0; iThread < nThread; iThread++) {
		tStatus = WaitForSingleObject (producerThreadArray[iThread], INFINITE);
		if (tStatus != 0) 
			ReportError (_T("Cannot wait for producer thread"), 6, TRUE);
		if (trace >= 1) _tprintf (_T("BOSS: Producer %d produced %d work units\n"),
			iThread, producerArg[iThread].workDone);
	}
	/* Producers have completed their work. */
	_tprintf (_T("BOSS: All producers have completed their work.\n"));

	/* Wait for the consumers to complete */
	for (iThread = 0; iThread < nThread; iThread++) {
		tStatus = WaitForSingleObject (consumerThreadArray[iThread], INFINITE);
		if (tStatus != 0) 
			ReportError (_T("Cannot wait for consumer thread"), 7, TRUE);
		if (trace >= 1) _tprintf (_T("BOSS: consumer %d consumed %d work units\n"),
			iThread, consumerArg[iThread].workDone);
	}
	_tprintf (_T("BOSS: All consumers have completed their work.\n"));	

	ShutDown = 1; /* Set a shutdown flag. */
	tStatus = WaitForSingleObject (transmitterThread, INFINITE);
	if (tStatus != WAIT_OBJECT_0) 
		ReportError (_T("Failed waiting for transmitter"), 8, TRUE);
	tStatus = WaitForSingleObject (receiverThread, INFINITE);

	if (tStatus != WAIT_OBJECT_0) 
		ReportError (_T("Failed waiting for transmitter"), 9, TRUE);

	QueueDestroy (&p2tq);
	QueueDestroy (&t2rq);
	for (iThread = 0; iThread < nThread; iThread++) {
		QueueDestroy (&r2cqArray[iThread]);
		CloseHandle(consumerThreadArray[iThread]);
		CloseHandle(producerThreadArray[iThread]);
	}
	free (r2cqArray);
	free (producerThreadArray); free (consumerThreadArray);
	free (producerArg); free(consumerArg);
	CloseHandle(transmitterThread); CloseHandle(receiverThread);
	_tprintf (_T("System has finished. Shutting down\n"));
	return 0;
}

DWORD WINAPI producer (PVOID arg)
{
	THARG * parg;
	DWORD iThread;
	MSG_BLOCK msg;
	
	parg = (THARG *)arg;	
	iThread = parg->threadNumber;

	while (parg->workDone < parg->workGoal) { 
		/* Periodically produce work units until the goal is satisfied */
		/* messages receive a source and destination address which are */
		/* the same in this case but could, in general, be different. */
		delay_cpu (DELAY_COUNT * rand() / RAND_MAX);
		MessageFill (&msg, iThread, iThread, parg->workDone);
		
		/* put the message in the queue */
		QueuePut (&p2tq, &msg, sizeof(msg), INFINITE);
				
		parg->workDone++;		
	}
	
	/*Send a final "done" message (negative sequence number) */
	MessageFill (&msg, iThread, iThread, -1);
	QueuePut (&p2tq, &msg, sizeof(msg), INFINITE);
	return 0;		
}


DWORD WINAPI transmitter (PVOID arg)
{
	/* Obtain multiple producer messages, combining into a single	*/
	/* compound message for the receiver */

	DWORD tStatus, im;
	T2R_MSG_OBJ t2r_msg = {0};
	TR_ARG * tArg = (TR_ARG *)arg;

	while (!ShutDown) {
		t2r_msg.numMessages = 0;
		/* pack the messages for transmission to the receiver */
		for (im = 0; im < TBLOCK_SIZE; im++) {
			tStatus = QueueGet (&p2tq, &t2r_msg.messages[im], sizeof(MSG_BLOCK), INFINITE);
			if (tStatus != 0) break;
			t2r_msg.numMessages++;
			/* Decrement the number of active consumers for each negative sequence number */
			if (t2r_msg.messages[im].sequence < 0) {
				tArg->nProducers--;
				if (tArg->nProducers <= 0) break;
			}
		}

		/* Transmit the block of messages */
		tStatus = QueuePut (&t2rq, &t2r_msg, sizeof(t2r_msg), INFINITE);
		if (tStatus != 0) return tStatus;
		/* Terminate the transmitter if there are no active consumers */
		if (tArg->nProducers <=0) return 0;
	}
	return 0;
}


DWORD WINAPI receiver (PVOID arg)
{
	/* Obtain compund messages from the transmitter and unblock them	*/
	/* and transmit to the designated consumer.				*/

	DWORD tStatus, im, ic;
	T2R_MSG_OBJ t2r_msg;
	TR_ARG * tArg = (TR_ARG *)arg;
	
	while (!ShutDown) {
		tStatus = QueueGet (&t2rq, &t2r_msg, sizeof(t2r_msg), INFINITE);
		if (tStatus != 0) return tStatus;
		/* Distribute the messages to the proper consumer */
		for (im = 0; im < t2r_msg.numMessages; im++) {
			ic = t2r_msg.messages[im].destination; /* Destination consumer */
			tStatus = QueuePut (&r2cqArray[ic], &t2r_msg.messages[im], sizeof(MSG_BLOCK), INFINITE);
			if (tStatus != 0) return tStatus;
			if (t2r_msg.messages[im].sequence < 0) {
				tArg->nProducers--;
				if (tArg->nProducers <= 0) break;
			}
		}
		/* Terminate the transmitter if there are no active consumers */
		if (tArg->nProducers <=0) return 0;
	}
	return 0;
}

DWORD WINAPI consumer (PVOID arg)
{
	THARG * carg;
	DWORD tStatus, iThread;
	MSG_BLOCK msg;
	QUEUE_OBJECT *pr2cq;

	carg = (THARG *) arg;
	iThread = carg->threadNumber;
	
	carg = (THARG *)arg;	
	pr2cq = &r2cqArray[iThread];

	while (carg->workDone < carg->workGoal) { 
		/* Receive and display messages */
		tStatus = QueueGet (pr2cq, &msg, sizeof(msg), INFINITE);
		if (tStatus != 0) return tStatus;
		if (msg.sequence < 0) return 0;  /* Last Message */
				
		if (trace >= 1) _tprintf (_T("Message received by consumer #: %d. Message #: %d.\n"), iThread, msg.sequence);
		if (trace >= 2) MessageDisplay (&msg);

		carg->workDone++;		
	}

	return 0;
}
