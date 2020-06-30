/* Chapter 9. simplePC.c										*/
/* Maintain two threads, a Producer and a Consumer				*/
/* The Producer periodically creates checksummed mData buffers, 	*/
/* or "message block" which the Consumer displays when prompted	*/
/* by the user. The conusmer reads the most recent complete 	*/
/* set of mData and validates it before display					*/

#include "Everything.h"
#include <time.h>
#define DATA_SIZE 256

typedef struct MSG_BLOCK_TAG { /* Message block */
	CRITICAL_SECTION mGuard;	/* Guard the message block structure	*/
	DWORD fReady, fStop; 
		/* ready state flag, stop flag	*/
	volatile DWORD nCons, mSequence; /* Message block mSequence number	*/
	DWORD nLost;
	time_t mTimestamp;
	DWORD mChecksum; /* Message contents mChecksum		*/
	DWORD mData[DATA_SIZE]; /* Message Contents		*/
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
	
	/* Initialize the message block CRITICAL SECTION */
	InitializeCriticalSection (&mBlock.mGuard);

	/* Create the two threads */
	hProduce = (HANDLE)_beginthreadex (NULL, 0, Produce, NULL, 0, NULL);
	if (hProduce == NULL) 
		ReportError (_T("Cannot create Producer thread"), 1, TRUE);
	hConsume = (HANDLE)_beginthreadex (NULL, 0, Consume, NULL, 0, NULL);
	if (hConsume == NULL) 
		ReportError (_T("Cannot create Consumer thread"), 2, TRUE);
	
	/* Wait for the Producer and Consumer to complete */
	
	status = WaitForSingleObject (hConsume, INFINITE);
	if (status != WAIT_OBJECT_0) 
		ReportError (_T("Failed waiting for Consumer thread"), 3, TRUE);
	status = WaitForSingleObject (hProduce, INFINITE);
	if (status != WAIT_OBJECT_0) 
		ReportError (__T("Failed waiting for Producer thread"), 4, TRUE);

	DeleteCriticalSection (&mBlock.mGuard);

	_tprintf (_T("Producer and Consumer threads have terminated\n"));
	_tprintf (_T("Messages Produced: %d, Consumed: %d, Lost: %d.\n"),
		mBlock.mSequence, mBlock.nCons, mBlock.mSequence - mBlock.nCons);
	return 0;
}

DWORD WINAPI Produce (void *arg)
/* Producer thread - Create new messages at random intervals */
{
	srand ((DWORD)time(NULL)); /* Seed the random # generator 	*/
	
	while (!mBlock.fStop) {
		/* Random Delay */
		Sleep(rand()/100);
		
		/* Get the buffer, fill it */
		
		EnterCriticalSection (&mBlock.mGuard);
		__try {
			if (!mBlock.fStop) {
				mBlock.fReady = 0;
				MessageFill (&mBlock);
				mBlock.fReady = 1;
				InterlockedIncrement (&mBlock.mSequence);
			}
		} 
		__finally { LeaveCriticalSection (&mBlock.mGuard); }
	}
	return 0;
}

DWORD WINAPI Consume (void *arg)
{
	CHAR command, extra;
	/* Consume the NEXT message when prompted by the user */
	while (!mBlock.fStop) { /* This is the only thread accessing stdin, stdout */
		_tprintf (_T("\n**Enter 'c' for Consume; 's' to stop: "));
		_tscanf (_T("%c%c"), &command, &extra);
		if (command == _T('s')) {
			/* ES not needed here. This is not a read/modify/write. 
			 * The Producer will see the new value after the Consumer returns */
			mBlock.fStop = 1;
		} else if (command == _T('c')) { /* Get a new buffer to Consume */
			EnterCriticalSection (&mBlock.mGuard);
			__try {
				if (mBlock.fReady == 0) 
					_tprintf (_T("No new messages. Try again later\n"));
				else {
					MessageDisplay (&mBlock);
					mBlock.nLost = mBlock.mSequence - mBlock.nCons + 1;
					mBlock.fReady = 0; /* No new messages are ready */
					InterlockedIncrement(&mBlock.nCons);
				}
			} 
			__finally { LeaveCriticalSection (&mBlock.mGuard); }
		} else {
			_tprintf (_T("Illegal command. Try again.\n"));
		}
	}
	return 0;		
}

void MessageFill (MSG_BLOCK *msgBlock)
{
	/* Fill the message buffer, and include mChecksum and mTimestamp	*/
	/* This function is called from the Producer thread while it 	*/
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
	/* This function is called from the Consumer thread while it 	*/
	/* owns the message block mutex					*/
	DWORD i, tcheck = 0;
	
	for (i = 0; i < DATA_SIZE; i++) 
		tcheck ^= msgBlock->mData[i];
	_tprintf (_T("\nMessage number %d generated at: %s"), 
		msgBlock->mSequence, _tctime (&(msgBlock->mTimestamp)));
	_tprintf (_T("First and last entries: %x %x\n"),
		msgBlock->mData[0], msgBlock->mData[DATA_SIZE-1]);
	if (tcheck == msgBlock->mChecksum)
		_tprintf (_T("GOOD ->mChecksum was validated.\n"));
	else
		_tprintf (_T("BAD  ->mChecksum failed. message was corrupted\n"));
		
	return;
}