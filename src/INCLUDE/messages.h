/* messages.h - Used with numerous Chapter 9, 10 programs */

#define DATA_SIZE 256
typedef struct MSG_BLOCK_TAG { /* Message block */
	HANDLE	mGuard;	/* Mutex for  the message block	*/
	HANDLE  mConsumed; /* Event: Message consumed;		*/
				/* Produce a new one or stop	*/
	HANDLE  mReady; /* Event: Message ready		*/
	/* Note: the mutex and events are not used by some programs, such
	 * as Program 10-3, 4, 5 (the multi-stage pipeline) as the messages
	 * are part of a protected quueue */
	int source; /* Creating producer identity 	*/
	int destination;/* Identity of receiving thread*/

	DWORD fReady;	 /* Set to 1 if a message is ready. Used by eventPC and simplePC */
	DWORD fStop;     /* Set to 1 for the last message */
		/* Consumed & ready state flags, stop flag	*/
	int sequence; /* Message block sequence number. Negative indicates final message	*/
	time_t timestamp;
	DWORD checksum; /* Message contents checksum		*/
	DWORD data[DATA_SIZE]; /* Message Contents		*/
} MSG_BLOCK;

VOID MessageFill (MSG_BLOCK *, DWORD, DWORD, DWORD);
VOID MessageDisplay (MSG_BLOCK *);
