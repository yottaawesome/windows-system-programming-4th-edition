/* Chapter 9. simplePCx.c	DEFECTIVE							*/
/* Maintain two threads, a producer and a consumer				*/
/* The producer periodically creates checksummed data buffers, 	*/
/* or "message block" which the consumer displays when prompted	*/
/* by the user. The conusmer reads the most recent complete 	*/
/* set of data and validates it before display					*/

#include "Everything.h"
#include <time.h>
#define DATA_SIZE 256

typedef struct msg_block_tag { /* Message block */
	DWORD		f_ready, f_stop; 
		/* ready state flag, stop flag	*/
	DWORD sequence; /* Message block sequence number	*/
	DWORD nCons, nLost;
	time_t timestamp;
	CRITICAL_SECTION	mguard;	/* Guard the message block structure	*/
	DWORD	checksum; /* Message contents checksum		*/
	DWORD	data[DATA_SIZE]; /* Message Contents		*/
} MSG_BLOCK;
/*	One of the following conditions holds for the message block 	*/
/*	  1)	!f_ready || f_stop										*/
/*			 nothing is assured about the data		OR				*/
/*	  2)	f_ready && data is valid								*/
/*			 && checksum and timestamp are valid					*/ 
/*  Also, at all times, 0 <= nLost + nCons <= sequence				*/

/* Single message block, ready to fill with a new message 	*/
MSG_BLOCK mblock = { 0, 0, 0, 0, 0 }; 

DWORD WINAPI produce (void *);
DWORD WINAPI consume (void *);
void MessageFill (MSG_BLOCK *);
void MessageDisplay (MSG_BLOCK *);
	
int _tmain (int argc, LPTSTR argv[])
{
	DWORD Status, ThId;
	HANDLE produce_h, consume_h;
	
	/* Initialize the message block CRITICAL SECTION */
	InitializeCriticalSection (&mblock.mguard);

	/* Create the two threads */
	produce_h = (HANDLE)_beginthreadex (NULL, 0, produce, NULL, 0, &ThId);
	if (produce_h == NULL) 
		ReportError (_T("Cannot create producer thread"), 1, TRUE);
	consume_h = (HANDLE)_beginthreadex (NULL, 0, consume, NULL, 0, &ThId);
	if (consume_h == NULL) 
		ReportError (_T("Cannot create consumer thread"), 2, TRUE);
	
	/* Wait for the producer and consumer to complete */
	
	Status = WaitForSingleObject (consume_h, INFINITE);
	if (Status != WAIT_OBJECT_0) 
		ReportError (_T("Failed waiting for consumer thread"), 3, TRUE);
	Status = WaitForSingleObject (produce_h, INFINITE);
	if (Status != WAIT_OBJECT_0) 
		ReportError (__T("Failed waiting for producer thread"), 4, TRUE);

	DeleteCriticalSection (&mblock.mguard);

	_tprintf (_T("Producer and consumer threads have terminated\n"));
	_tprintf (_T("Messages produced: %d, Consumed: %d, Known Lost: %d\n"),
		mblock.sequence, mblock.nCons, mblock.nLost);
	return 0;
}

DWORD WINAPI produce (void *arg)
/* Producer thread - Create new messages at random intervals */
{
	
	srand ((DWORD)time(NULL)); /* Seed the random # generator 	*/
	
	while (!mblock.f_stop) {
		/* Random Delay */
		Sleep(rand()/100);
		
		/* Get the buffer, fill it */
		EnterCriticalSection (&mblock.mguard);
		if (!mblock.f_stop) {
			EnterCriticalSection (&mblock.mguard);
			mblock.f_ready = 0;
			mblock.f_ready = 1;
			mblock.sequence++;
			LeaveCriticalSection (&mblock.mguard);
			MessageFill (&mblock);
		}
	}
	return 0;
}

DWORD WINAPI consume (void *arg)
{
	DWORD ShutDown = 0;
	CHAR command, extra;
	/* Consume the NEXT message when prompted by the user */
	while (!ShutDown) { /* This is the only thread accessing stdin, stdout */
		_tprintf (_T("\n**Enter 'c' for consume; 's' to stop: "));
		_tscanf (_T("%c%c"), &command, &extra);
		if (command == _T('s')) {
			ShutDown = mblock.f_stop = 1;
		} else if (command == _T('c')) { /* Get a new buffer to consume */
			EnterCriticalSection (&mblock.mguard);
			__try {
				if (mblock.f_ready == 0) 
					_tprintf (_T("No new messages. Try again later\n"));
				else {
					mblock.nCons++;
					mblock.nLost = mblock.sequence - mblock.nCons;
					mblock.f_ready = 0; /* No new messages are ready */
					LeaveCriticalSection (&mblock.mguard);
					MessageDisplay (&mblock);
				}
			} 
			__finally { LeaveCriticalSection (&mblock.mguard); }
		} else {
			_tprintf (_T("Illegal command. Try again.\n"));
		}
	}
	return 0;		
}

void MessageFill (MSG_BLOCK *mblock)
{
	/* Fill the message buffer, and include checksum and timestamp	*/
	/* This function is called from the producer thread while it 	*/
	/* owns the message block mutex					*/
	
	DWORD i;
	
	mblock->checksum = 0;	
	for (i = 0; i < DATA_SIZE; i++) {
		mblock->data[i] = rand();
		mblock->checksum ^= mblock->data[i];
	}
	mblock->timestamp = time(NULL);
	return;
}

void MessageDisplay (MSG_BLOCK *mblock)
{
	/* Display message buffer, timestamp, and validate checksum	*/
	/* This function is called from the consumer thread while it 	*/
	/* owns the message block mutex					*/
	DWORD i, tcheck = 0;
	
	for (i = 0; i < DATA_SIZE; i++) 
		tcheck ^= mblock->data[i];
	_tprintf (_T("\nMessage number %d generated at: %s"), 
		mblock->sequence, _tctime (&(mblock->timestamp)));
	_tprintf (_T("First and last entries: %x %x\n"),
		mblock->data[0], mblock->data[DATA_SIZE-1]);
	if (tcheck == mblock->checksum)
		_tprintf (_T("GOOD ->Checksum was validated.\n"));
	else
		_tprintf (_T("BAD  ->Checksum failed. message was corrupted\n"));
		
	return;

}