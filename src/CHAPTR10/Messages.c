/* Chapter 10. messages.c										*/
/* Queue functions												*/

#include "Everything.h"
#include "messages.h"
#include <time.h>

DWORD ComputeChecksum (PVOID msg, DWORD length)
{
	/* Computer an xor checksum on the entire message of "length" integers */
	DWORD i, cs = 0, *pInt;

	pInt = (DWORD *) msg;
	for (i = 0; i < length; i++) {
		cs = (cs ^ *pInt);
		pInt++;
	}
	return cs;		
}

VOID MessageFill (MSG_BLOCK *mBlock, DWORD src, DWORD dest, DWORD sequence)
{
	/* Fill the message buffer, and include checksum and timestamp	*/
	/* This function is called from the producer thread while it 	*/
	/* owns the message block mutex					*/

	DWORD i;
	
	mBlock->checksum = 0;	
	for (i = 0; i < DATA_SIZE; i++) {
		mBlock->data[i] = rand();
	}
	mBlock->source = src;
	mBlock->destination = dest;
	mBlock->sequence = sequence;
	mBlock->timestamp = time(NULL);
	mBlock->fReady = 0;
	mBlock->fStop = (sequence < 0);
	mBlock->checksum = ComputeChecksum (mBlock, sizeof(MSG_BLOCK)/sizeof(DWORD));
	return;
}

void MessageDisplay (MSG_BLOCK *mBlock)
{
	/* Display message buffer and timestamp, validate checksum	*/
	/* This function is called from the consumer thread while it 	*/
	/* owns the message block mutex					*/
	DWORD tcheck = 0;
	
	tcheck = ComputeChecksum (mBlock, sizeof(MSG_BLOCK)/sizeof(DWORD));
	_tprintf (_T("\nMessage number %d generated at: %s"), 
		mBlock->sequence, ctime (&(mBlock->timestamp)));
	_tprintf (_T("Source and destination: %d %d\n"), 
		mBlock->source, mBlock->destination);
	_tprintf (_T("First and last entries: %x %x\n"),
		mBlock->data[0], mBlock->data[DATA_SIZE-1]);
	if (tcheck == 0 /*mBlock->checksum was 0 when CS first computed */)
		_tprintf (_T("GOOD ->Checksum was validated.\n"));
	else
		_tprintf (_T("BAD  ->Checksum failed. message was corrupted\n"));
		
	return;

}
