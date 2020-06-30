/* Chapter 9. eventPC.c											*/
/* Maintain two threads, a producer and a consumer				*/
/* The producer periodically creates checksummed mData buffers, 	*/
/* or "message block" signalling the consumer that a message	*/
/* is ready, and which the consumer displays when prompted		*/
/* by the user. The conusmer reads the next complete 			*/
/* set of mData and validates it before display					*/
/* This is a reimplementation of simplePC.c to use an event		*/

#include "Everything.h"
#include <time.h>
#define DATA_SIZE 256

typedef struct MSG_BLOCK_TAG { /* Message block */
	HANDLE	mGuard;	/* Mutex to uard the message block structure	*/
	HANDLE	mReady; /* "Message ready" auto-reset event			*/
	DWORD	fReady, fStop; 
		/* ready state flag, stop flag	*/
	volatile DWORD mSequence; /* Message block mSequence number	*/
	volatile DWORD nCons, nLost;
	time_t mTimestamp;
	DWORD	mChecksum; /* Message contents mChecksum		*/
	DWORD	mData[DATA_SIZE]; /* Message Contents		*/
} MSG_BLOCK;
/*	One of the following conditions holds for the message block 	*/
/*	  1)	!fReady || fStop										*/
/*			 nothing is assured about the mData		OR				*/
/*	  2)	fReady && mData is valid								*/
/*			 && mChecksum and mTimestamp are valid					*/ 
/*  Also, at all times, 0 <= nLost + nCons <= mSequence				*/

/* Single message block, ready to fill with a new message 	*/
MSG_BLOCK mBlock = { 0, 0, 0, 0, 0 }; 

DWORD WINAPI Produce (void *);
DWORD WINAPI Consume (void *);
void MessageFill (MSG_BLOCK *);
void MessageDisplay (MSG_BLOCK *);
	
int _tmain (int argc, LPTSTR argv[])
{
	DWORD status;
	HANDLE hProduce, hConsume;
	
	/* Initialize the message block mutex and event (auto-reset) */
	mBlock.mGuard = CreateMutex (NULL, FALSE, NULL);
	mBlock.mReady = CreateEvent (NULL, FALSE, FALSE, NULL);

	/* Create the two threads */
	hProduce = (HANDLE)_beginthreadex (NULL, 0, Produce, NULL, 0, NULL);
	if (hProduce == NULL) 
		ReportError (_T("Cannot create producer thread"), 1, TRUE);
	hConsume = (HANDLE)_beginthreadex (NULL, 0, Consume, NULL, 0, NULL);
	if (hConsume == NULL) 
		ReportError (_T("Cannot create consumer thread"), 2, TRUE);
	
	/* Wait for the producer and consumer to complete */
	
	status = WaitForSingleObject (hConsume, INFINITE);
	if (status != WAIT_OBJECT_0) 
		ReportError (_T("Failed waiting for consumer thread"), 3, TRUE);
	status = WaitForSingleObject (hProduce, INFINITE);
	if (status != WAIT_OBJECT_0) 
		ReportError (__T("Failed waiting for producer thread"), 4, TRUE);

	CloseHandle (mBlock.mGuard);
	CloseHandle (mBlock.mReady);

	_tprintf (_T("Producer and consumer threads have terminated\n"));
	_tprintf (_T("Messages produced: %d, Consumed: %d, Known Lost: %d\n"),
		mBlock.mSequence, mBlock.nCons, mBlock.mSequence - mBlock.nCons);
	return 0;
}

DWORD WINAPI Produce (void *arg)
/* Producer thread - Create new messages at random intervals */
{
	srand ((DWORD)time(NULL)); /* Seed the random # generator 	*/
	
	while (!mBlock.fStop) {
		/* Random Delay */
		Sleep(rand()/5); /* wait a long period for the next message */
			/* Adjust the divisor to change message generation rate */
		/* Get the buffer, fill it */
		
		WaitForSingleObject (mBlock.mGuard, INFINITE);
		__try {
			if (!mBlock.fStop) {
				mBlock.fReady = 0;
				MessageFill (&mBlock);
				mBlock.fReady = 1;
				InterlockedIncrement(&mBlock.mSequence);
				SetEvent(mBlock.mReady); /* Signal that a message is ready. */
			}
		} 
		__finally { ReleaseMutex (mBlock.mGuard); }
	}
	return 0;
}

DWORD WINAPI Consume (void *arg)
{
	DWORD ShutDown = 0;
	CHAR command[10];
	/* Consume the NEXT message when prompted by the user */
	while (!ShutDown) { /* This is the only thread accessing stdin, stdout */
		_tprintf (_T ("\n**Enter 'c' for Consume; 's' to stop: "));
		_tscanf_s (_T("%9s"), command, sizeof(command)-1);
		if (command[0] == _T('s')) {
			WaitForSingleObject (mBlock.mGuard, INFINITE);
			ShutDown = mBlock.fStop = 1;
			ReleaseMutex (mBlock.mGuard);
		} else if (command[0] == _T('c')) { /* Get a new buffer to consume */
			WaitForSingleObject (mBlock.mReady, INFINITE);
			WaitForSingleObject (mBlock.mGuard, INFINITE);
			__try {
				if (!mBlock.fReady) _leave; /* Don't process a message twice */
				/* Wait for the event indicating a message is ready */
				MessageDisplay (&mBlock);
				InterlockedIncrement(&mBlock.nCons);
				mBlock.nLost = mBlock.mSequence - mBlock.nCons;
				mBlock.fReady = 0; /* No new messages are ready */
			} 
			__finally { ReleaseMutex (mBlock.mGuard); }
		} else {
			_tprintf (_T("Illegal command. Try again.\n"));
		}
	}
	return 0;		
}

void MessageFill (MSG_BLOCK *msgBlock)
{
	/* Fill the message buffer, and include mChecksum and mTimestamp	*/
	/* This function is called from the producer thread while it 	*/
	/* owns the message block mutex					*/
	
	DWORD i;
	
	msgBlock->mChecksum = 0;	
	for (i = 0; i < DATA_SIZE; i++) {
		msgBlock->mData[i] = rand();
		msgBlock->mChecksum ^= msgBlock->mData[i];
	}
	msgBlock->mTimestamp = time(NULL);
	return;
}

void MessageDisplay (MSG_BLOCK *msgBlock)
{
	/* Display message buffer, mTimestamp, and validate mChecksum	*/
	/* This function is called from the consumer thread while it 	*/
	/* owns the message block mutex					*/
	DWORD i, tcheck = 0;
	TCHAR timeValue[26];
	
	for (i = 0; i < DATA_SIZE; i++) 
		tcheck ^= msgBlock->mData[i];

	_tctime_s (timeValue, 26, &(msgBlock->mTimestamp));
	_tprintf (_T("\nMessage number %d generated at: %s"), 
		msgBlock->mSequence, timeValue);
	_tprintf (_T("First and last entries: %x %x\n"),
		msgBlock->mData[0], msgBlock->mData[DATA_SIZE-1]);
	if (tcheck == msgBlock->mChecksum)
		_tprintf (_T("GOOD ->mChecksum was validated.\n"));
	else
		_tprintf (_T("BAD  ->mChecksum failed. message was corrupted\n"));
		
	return;
}