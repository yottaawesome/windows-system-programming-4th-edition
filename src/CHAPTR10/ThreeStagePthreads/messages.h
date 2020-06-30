/* messages.h - Used with numerous Chapter 9, 10 programs */

#define DATA_SIZE 256
typedef struct msg_block_tag { /* Message block */
	pthread_mutex_t	mguard;	/* Mutex for  the message block	*/
	pthread_cond_t  mconsumed; /* Event: Message consumed;		*/
				/* Produce a new one or stop	*/
	pthread_cond_t  mready; /* Event: Message ready		*/
	/* Note: the mutex and events are not used by some programs, such
	 * as Program 10-3, 4, 5 (the multi-stage pipeline) as the messages
	 * are part of a protected quueue */
	volatile unsigned int source; /* Creating producer identity 	*/
	volatile unsigned int destination;/* Identity of receiving thread*/

	volatile unsigned int f_consumed;
	volatile unsigned int f_ready;
	volatile unsigned int f_stop; 
		/* Consumed & ready state flags, stop flag	*/
	volatile unsigned int sequence; /* Message block sequence number	*/
	time_t timestamp;
	unsigned int checksum; /* Message contents checksum		*/
	unsigned int data[DATA_SIZE]; /* Message Contents		*/
} msg_block_t;

void message_fill (msg_block_t *, unsigned int, unsigned int, unsigned int);
void message_display (msg_block_t *);
