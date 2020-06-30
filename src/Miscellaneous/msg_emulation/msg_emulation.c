/*  msg_emulation.c													*/
/*  Library to emulate UNIX msg functions in Win32 environments.	*/
/*																	*/
/*	IMPLEMENTATION NOTE: The implementation is source code 			*/
/*		portable to any Pthreads platform and Win32					*/
/*		All thread management and synchronization is performed		*/
/*		with macros defined in thread_emulation.h. These macros		*/
/*		correspond to the Pthreads model							*/
/*		Pthreads is assumed unless _WINDOWS is defined				*/
/*		On Windows, you can also replace thread_emulation.h with	*/
/*		the open source Pthreads libarry at the RedHat site			*/
/*																	*/
/*		msg_emulation.h redefines the Posix Message Queue (MQ)		*/
/*		functions, so this is not recommended for Posix (mostly		*/
/*		UNIX and LINUX) systems. Pthreads is a subset of POSIX.		*/
/*																	*/
/*		Author: John Hart,  July 27, 2000							*/
/*		Open issues: 1) Process sharing								*/
/*					 2) Security (full msgctl() functionality)		*/
/*																	*/
#ifdef _WINDOWS
//#define _WIN32_WINNT 0x400  // WINBASE.H
#include "thread_emulation.h"
#else
#include "pthread.h"
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "msg_emulation.h"
#include <time.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Internally defined and used functions									*/

static int msgq_lookup (key_t);
static int msgq_access (int, int, int);
static int msgq_add (key_t, int);
static int	msgq_remove (int);
static int msgq_valid (int);
static void empty_table();
static void SetErrno (int);
int  GetErrno ();
static void ThreadIdInit();

#ifndef _WINDOWS
#include <unistd.h>
#endif 

/* Emulate UNIX user and group Ids, and process Id. */
static int em_getuid();
static int em_geteuid();
static int em_getgid();
static int em_getegid();

/* Thread Id structure */
typedef struct _MQ_THREAD_ID {
	uid_t	uid;
	uid_t	euid;
	gid_t	gid;
	gid_t	egid;
	int		th_errno;
} MQ_THREAD_ID;

/*	Global message queue structure																*/
typedef struct _MSG_QUEUES {
	volatile size_t			q_max;		/* Max of number of queues								*/
	volatile size_t			q_current;	/* System wide queue count								*/
	volatile size_t			b_max;		/* System wide max number of message bytes				*/
	volatile size_t			b_current;	/* System wide current number of message bytes			*/
	pthread_mutex_t		q_guard;				/* Mutex to guard the entire message queue structure	*/
	pthread_condvar_t	e_qne;					/* Event indicating at least one queue is not empty		*/
	pthread_condvar_t	e_qnf;					/* Event indicating at least one queue is not full		*/
	pthread_condvar_t	e_q_not_busy;			/* Event indicating that some q has become not busy		*/
	pthread_t			d_sender;				/* Daemon thread to write msgsnd messages to the socket */
	pthread_t			d_receive;				/* Daemon thread to receive incoming socket messages	*/
	msg_t				mq[MSG_MAX_NUMBER_QUEUES]; 
} MSG_QUEUES;	

static MSG_QUEUES mq_table = 
		{ MSG_MAX_NUMBER_QUEUES, 0, MQ_MAX_BYTES_IN_SYSTEM, 0 };


#ifdef _WINDOWS
static int OnceFlag = 0;
static void InitializeMqTable(int *);
#define IntiializeMQSystem InitializeMqTable(&OnceFlag);
#else
static void once_init_routine (void);
static pthread_once_t one_time = PTHREAD_ONCE_INIT;
#define IntiializeMQSystem pthread_once (&one_time, once_init_routine);
#endif


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int not_full (size_t msgsz, int msqid, int hysteresis)
{
	/*	CONDITION VARIABLE PREDICATE										*/
	/* Is there space in the specified Q for a message of the specified size */
	/* plus some slack (hysteresis). Using a positive hysteresis prevents    */
	/* excessive broadcasting on the e_qnf condition variable. A negative	 */
	/* value results in a default of one maximum sized message (limited to	 */
	/* 1/4 of the maximum message size					 */
	/* This must be called with the mutex locked				 */
	/* Remember that message Q numbering starts at 1.			 */
	/* Also, each message also requires a long int field for the type	 */

	int Result;
	unsigned int h;
	
	h = (hysteresis < 0) ? MQ_MAX_MESSAGE_SIZE : hysteresis;
	if (h >= mq_table.mq[msqid-1].msg_qbytes/4) h = mq_table.mq[msqid-1].msg_qbytes/4;
	
	Result = 
		(msgsz + h + sizeof(long) + mq_table.b_current < mq_table.b_max && 
		 msgsz + h + sizeof(long) + mq_table.mq[msqid-1].msg_cbytes 
					< mq_table.mq[msqid-1].msg_qbytes);
	return Result;
}

static int message_exists (msqid)
{
	/*  CONDITION VARIABLE PREDICATE										*/
	/* Is there a message in the specified quueue							*/
	/* or, is there any message at all when msqid == -1						*/
	/* This must be called with the mutex locked							*/
	return	(msqid > 0 && mq_table.mq[msqid-1].msg_qnum > 0);
}

static int update_status (int msgsz, int msqid)
{
	/* Update total byte counts and time of last modifications */
	/* msgsz is positive when sending a message and negative when receiving */
	int msqidm1 = msqid - 1, longsize = sizeof(long);
	
	if (msgsz < 0) longsize = - longsize;

	mq_table.mq[msqidm1].msg_cbytes	+= (msgsz+longsize); /* Q-specific */
	mq_table.b_current += (msgsz+longsize); /* System wide */
	
	if (msgsz > 0) { /* Add a message */
		mq_table.mq[msqidm1].msg_qnum++;
		mq_table.mq[msqidm1].msg_stime = time(NULL);
		mq_table.mq[msqidm1].msg_lspid = getpid();
	}
	else if (msgsz < 0) { /* Remove a message */
		mq_table.mq[msqidm1].msg_qnum--;
		mq_table.mq[msqidm1].msg_rtime = time(NULL);
		mq_table.mq[msqidm1].msg_lrpid = getpid();
	}
	return 1;
}


static int PlaceMessageInQueue (int msqid, const void *msgp, size_t msgsz)
{
	/* Utility function to place message in Q. Must be called with mutex locked */
	/* The Q id is known to be valid, and there is space in the Q				*/
	/* The message Q is maintained as a doubly linked list. Each list entry     */
	/* has a node, of type MESSAGE_T, and each node also conatins a message		*/
	/* whose size is determined from the message size (msgsz), allowing for		*/
	/* the type field. This function and GetMessageFromQueueu maintain			*/
	/* the list structure														*/

	int Result = 1; /* Assume success */
	struct _MESSAGE_T *pMsgElement, *pFirst, *pLast;
	void *msgpCopy;
	int msqidm1 = msqid - 1;

	/* Copy the message, including type field */
	pMsgElement = (void *)malloc (msgsz+sizeof(MESSAGE_T));	
	if (pMsgElement == NULL) return 0;
	msgpCopy = &(pMsgElement->msg_mtype);
	memcpy (msgpCopy, msgp, msgsz+sizeof(long));

	/* Fill in the message Q list element */
	pMsgElement->mlength = msgsz;
	pMsgElement->mtype = *((long *)msgp); /* Type is taken from message head */

	/* Insert the element at the end of the list */
	pFirst = mq_table.mq[msqidm1].msg_first;
	pLast  = mq_table.mq[msqidm1].msg_last;
	if (pFirst == NULL && pLast == NULL) { /* It is not possible for just one to be null */
		/* First and only message in the list */
		mq_table.mq[msqidm1].msg_first = mq_table.mq[msqidm1].msg_last = pMsgElement;
		pMsgElement->pNext = pMsgElement->pPrev = NULL;
	} else if (pFirst != NULL && pLast != NULL) {
		pMsgElement->pPrev = mq_table.mq[msqidm1].msg_last;
		pMsgElement->pNext = NULL;
		mq_table.mq[msqidm1].msg_last->pNext = pMsgElement;
		mq_table.mq[msqidm1].msg_last = pMsgElement;
	} else {
		/* The message Q must have been corrupted. Deallocate new list element and return */
		free (pMsgElement); 
		free (msgpCopy);
		SetErrno (EFAULT);
		Result = 0;
	}

	return Result;
}

static int GetMessageFromQueue (int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	/* Retrieve a message from the specified queue, which may be empty */
	/* On success, return the number of bytes transferred */

	int Result = 0;  /* Assume FALSE until a message is located */
	int msqidm1 = msqid - 1;
	struct _MESSAGE_T *pMsgCurrent;

	if (!msgq_valid(msqid)) { /* the message Q must have been removed */
		SetErrno (EIDRM);
		return 0;
	}
	
	pMsgCurrent = mq_table.mq[msqidm1].msg_first;
	while (pMsgCurrent != NULL) { /* Scan the list looking for a satisfactory message */
		if (msgtyp == 0) break;		/* First message */
		if (msgtyp > 0 && pMsgCurrent->mtype == msgtyp) break;
		pMsgCurrent = pMsgCurrent->pNext;
	}

	if (pMsgCurrent != NULL) { /* This is the message to return */
		Result = 1; /* Success, but this can be reset if the incoming message is too large */
		/* Remove the message from the list */
		/* Maintain the list structure so that the last node always has a NULL next pointer */
		/* and the first node always has a NULL previous pointer. Deallocate the node       */
		/* and the message after they are copied.                                           */
		if (pMsgCurrent->pPrev != NULL) 
			(pMsgCurrent->pPrev)->pNext = pMsgCurrent->pNext;
		else { /* This is the first element in the list - make its successor the first */
			mq_table.mq[msqidm1].msg_first = pMsgCurrent->pNext;
			if (pMsgCurrent->pNext != NULL) (pMsgCurrent->pNext)->pPrev = NULL;
		}
		if (pMsgCurrent->pNext != NULL) 
			(pMsgCurrent->pNext)->pPrev = pMsgCurrent->pPrev;
		else { /* This is the last element in the list. Make its predecssor the first */
			mq_table.mq[msqidm1].msg_last  = pMsgCurrent->pPrev;
			if (pMsgCurrent->pPrev) (pMsgCurrent->pPrev)->pNext = NULL;
		}

		/* Copy the message into the receiving buffer, truncating if necessary  */
		if (pMsgCurrent->mlength <= msgsz) {
			memcpy (msgp, &(pMsgCurrent->msg_mtype), pMsgCurrent->mlength+sizeof(size_t));
			Result = pMsgCurrent->mlength;
		}
		else {
			memcpy (msgp, &(pMsgCurrent->msg_mtype), msgsz+sizeof(size_t));
			Result = msgsz;
			if (!(msgflg & MSG_NOERROR)) SetErrno(E2BIG);
		}
		free (pMsgCurrent);
	} else {
		/*  No satisfactory message was found, and wait was not specified */
		if (msgflg & IPC_NOWAIT) SetErrno (ENOMSG);
		Result = 0;
	}
	return Result;
}

static int RemoveMessagesFromQueue (int msqid)
{
	/* Remove all messages from the queue and deallocates storage */
	struct _MESSAGE_T *pMsgCurrent, *pNext;
	int msqidm1 = msqid - 1;

	pMsgCurrent = mq_table.mq[msqidm1].msg_first;
	while (pMsgCurrent != NULL) { /* Scan the list freeing messages and listelements */
		pNext = pMsgCurrent->pNext;
		free (pMsgCurrent);
		pMsgCurrent = pNext;
	}

	mq_table.b_current -= mq_table.mq[msqidm1].msg_cbytes; /* System wide */
	/* Clear the message Q structure */
	memset (&mq_table.mq[msqidm1], 0, sizeof(struct _MSG_QUEUES));
	mq_table.mq[msqidm1].msg_cbytes = 0; /* Q-specific */
	mq_table.mq[msqidm1].msg_first = mq_table.mq[msqidm1].msg_last = NULL;
	return 1;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg)
{

/*

NAME
     msgop: msgsnd, msgrcv - message operations

SYNOPSIS
     #include <sys/types.h>
     #include <sys/ipc.h>
     #include <sys/msg.h>

     int msgsnd(int msqid, const void *msgp,
          size_t msgsz, int msgflg);


DESCRIPTION
     msgsnd sends a message to the queue associated with the message queue
     identifier specified by msqid.  msgp points to a user defined buffer that
     must contain first a field of type long integer that will specify the
     type of the message, and then a data portion that will hold the text of
     the message.  The following is an example of members that might be in a
     user defined buffer.

              long mtype;    /* message type 
              char mtext[];  /* message text 

     mtype is a positive integer that can be used by the receiving process for
     message selection.  mtext is any text of length msgsz bytes.  msgsz can
     range from 0 to a system imposed maximum.

     msgflg specifies the action to be taken if one or more of the following
     are true:

          The number of bytes already on the queue is equal to msg_qbytes [see
          intro(2)].

          The total number of messages on all queues system-wide is equal to
          the system-imposed limit.

     These actions are as follows:

          If (msgflg&IPC_NOWAIT) is true, the message is not sent and the
          calling process returns immediately. ?? should errno be set?

          If (msgflg&IPC_NOWAIT) is false, the calling process suspends
          execution until one of the following occurs:

                  The condition responsible for the suspension no longer
                  exists, in which case the message is sent.

                  msqid is removed from the system [see msgctl(2)].  When this
                  occurs, errno is set to EIDRM, and a value of -1 is
                  returned.

                  The calling process receives a signal that is to be caught.
                  In this case the message is not sent and the calling process
                  resumes execution in the manner prescribed in signal(2).

     msgsnd fails and sends no message if one or more of the following are true:

     EINVAL         msqid is not a valid message queue identifier.
     EACCES         Write permission is denied to the calling process [see intro(2)].

     EINVAL         mtype is less than 1.

     EAGAIN         The message cannot be sent for one of the reasons cited
                    above and (msgflg&IPC_NOWAIT) is true.

     EINVAL         msgsz is less than zero or greater than the system-imposed limit.

     EFAULT         msgp points to an illegal address.

     Upon successful completion, the following actions are taken with respect
     to the data structure associated with msqid [see intro (2)].

          msg_qnum is incremented by 1.

          msg_lspid is set to the process ID of the calling process.

          msg_stime is set to the current time.
*/

	int mtype, Result = 0, tstatus;
	int msqidm1 = msqid - 1;

	/*	Assure thread is initialized */
	ThreadIdInit();

	if (msgp == NULL) {
		SetErrno (EFAULT);
		return -1;
	}
	mtype = *((long *)msgp);

	tstatus = pthread_mutex_lock(mq_table.q_guard);
	mq_table.mq[msqidm1].msg_busy = 1; /* Mark this message Q busy (it can't be removed) */

	if (!msgq_valid (msqid) || mtype < 1 || msgsz > MQ_MAX_MESSAGE_SIZE) {
		SetErrno(EINVAL);
		Result = -1;
	} else if (!msgq_access (msqid, 0, 2)) { /* Is there write permission? */
		SetErrno(EACCES);
		Result = -1;
	} else if (!not_full (msgsz, msqid, 0) && (msgflg & IPC_NOWAIT)) {
		/* Full: No space and we should not wait */
		SetErrno(EAGAIN); 
		Result = -1;
	} else {
		/* Wait for there to be sufficient space, then place the message in the queue */
		while (!not_full (msgsz, msqid, 0)) {
			pthread_cond_wait(mq_table.e_qnf, mq_table.q_guard);
		}

		if (PlaceMessageInQueue (msqid, msgp, msgsz)) {
					/*  Update time stamps and counts */
			update_status ((int)msgsz, msqid);
			/* Broadcast the fact that the queues are not empty */
			pthread_cond_broadcast(mq_table.e_qne); 
		}
	}

	mq_table.mq[msqidm1].msg_busy = 0; /* Mark this message Q not busy (it can be removed) */
	pthread_cond_broadcast(mq_table.e_q_not_busy);

	tstatus = pthread_mutex_unlock(mq_table.q_guard);
	
	return Result;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{

/*
     msgrcv reads a message from the queue associated with the message queue
     identifier specified by msqid and places it in the user defined structure
     pointed to by msgp.  The structure must contain a message type field
     followed by the area for the message text (see the structure mymsg
     above).  mtype is the received message's type as specified by the sending
     process.  mtext is the text of the message.  msgsz specifies the size in
     bytes of mtext.  The received message is truncated to msgsz bytes if it
     is larger than msgsz and (msgflg&MSG_NOERROR) is true.  The truncated
     part of the message is lost and no indication of the truncation is given
     to the calling process.

     msgtyp specifies the type of message requested as follows:

          If msgtyp is 0, the first message on the queue is received.

          If msgtyp is greater than 0, the first message of type msgtyp is
          received.

          If msgtyp is less than 0, the first message of the lowest type that
          is less than or equal to the absolute value of msgtyp is received.
			NOTE: This is ambiguous. If msgtyp == -3 and the messge order is 2, 1,
			which do you take? I think you take the type 1 (this is what's implemented)

     msgflg specifies the action to be taken if a message of the desired type
     is not on the queue.  These are as follows:

          If (msgflg&IPC_NOWAIT) is true, the calling process returns
          immediately with a return value of -1 and sets errno to ENOMSG.

          If (msgflg&IPC_NOWAIT) is false, the calling process suspends
          execution until one of the following occurs:

                  A message of the desired type is placed on the queue.

                  msqid is removed from the system.  When this occurs, errno
                  is set to EIDRM, and a value of -1 is returned.

                  The calling process receives a signal that is to be caught.
                  In this case a message is not received and the calling
                  process resumes execution in the manner prescribed in
                  signal(2).

     msgrcv fails and receives no message if one or more of the following are true:

     EINVAL         msqid is not a valid message queue identifier.

     EACCES         Read permission is denied to the calling process.

     EINVAL         msgsz is less than 0.

     E2BIG          The length of mtext is greater than msgsz and
                    (msgflg&MSG_NOERROR) is false.

     ENOMSG         The queue does not contain a message of the desired type
                    and (msgflg&IPC_NOWAIT) is true.

     EFAULT         msgp points to an illegal address.

     Upon successful completion, the following actions are taken with respect
     to the data structure associated with msqid [see intro (2)].

          msg_qnum is decremented by 1.

          msg_lrpid is set to the process ID of the calling process.

          msg_rtime is set to the current time.

SEE ALSO
     intro(2), msgctl(2), msgget(2), signal(2).

DIAGNOSTICS
     If msgsnd or msgrcv return due to the receipt of a signal, a value of -1
     is returned to the calling process and errno is set to EINTR.  If they
     return due to removal of msqid from the system, a value of -1 is returned
     and errno is set to EIDRM.

     Upon successful completion, the return value is as follows:

          msgsnd returns a value of 0.

          msgrcv returns the number of bytes actually placed into mtext.

     Otherwise, a value of -1 is returned and errno is set to indicate the
     error.

*/
	long int Result = 0, BytesXfer = 0, tstatus;
	int msqidm1 = msqid - 1;

	/*	Assure thread is initialized */
	ThreadIdInit();

	tstatus = pthread_mutex_lock(mq_table.q_guard);
	mq_table.mq[msqidm1].msg_busy = 1; /* Mark this message Q busy (it can't be removed) */
	
	if (!msgq_valid (msqid)) {
		SetErrno(EINVAL); /* Not a valid Q identifier (it may have been removed) */
		Result = -1;
	} else if (msgsz > MQ_MAX_MESSAGE_SIZE) {
		SetErrno(EINVAL);
		Result = -1;
	} else if (!msgq_access (msqid, 0, 1)) { /* Is there read permission? */
		SetErrno(EACCES);
		Result = -1;
	} else if (!message_exists (msqid) && (msgflg & IPC_NOWAIT)) {
		/* No message and we should not wait */
		SetErrno(ENOMSG);
		Result = -1;
	} else {
		/* Wait for a message to exist, then get the message from the queue 
		 * if it is the correct type	*/
		/* Retry if there is no message of the sepcified type				*/

		while ( !(message_exists (msqid) /* Do not try to get a message until one exists */
			  && (BytesXfer = GetMessageFromQueue (msqid, msgp, msgsz, msgtyp, msgflg)) > 0 )) {
			if (msgflg & IPC_NOWAIT) break;
			pthread_cond_wait(mq_table.e_qne, mq_table.q_guard); 
		}

		/*  Update time stamps and counts (negative to denote receipt) */
		if (/* GetErrno() != ENOMSG && */ msgq_valid(msqid)) {
			update_status (-((int)BytesXfer), msqid);
			/* Broadcast the fact that the queues are not full */
			/* But, specify the default hysteresis to avoid    */
			/* a broadcast every time a message is removed     */
			if (not_full (0, msqid, -1)) pthread_cond_broadcast(mq_table.e_qnf);
		}

		Result = BytesXfer;
		
	}

	mq_table.mq[msqidm1].msg_busy = 0; /* Mark this message Q not busy (it can be removed) */
	pthread_cond_broadcast(mq_table.e_q_not_busy);

	tstatus = pthread_mutex_unlock(mq_table.q_guard);
	
	return Result;
}
 
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int msgget (key_t key, int msgflg)
{
/*
	NAME
     msgget - get message queue

SYNOPSIS
     #include <sys/types.h>
     #include <sys/ipc.h>
     #include <sys/msg.h>

     int msgget(key_t key, int msgflg);

DESCRIPTION
     msgget returns the message queue identifier associated with key.

     A message queue identifier and associated message queue and data
     structure [see intro(2)] are created for key if one of the following are
     true:

          key is IPC_PRIVATE.

          key does not already have a message queue identifier associated with
          it, and (msgflg&IPC_CREAT) is true (but, do not create a duplicate if
		  (msgflg&IPC_EXCL) is true.

     On creation, the data structure associated with the new message queue
     identifier is initialized as follows:

          msg_perm.cuid, msg_perm.uid, msg_perm.cgid, and msg_perm.gid are set
          to the effective user ID and effective group ID, respectively, of
          the calling process.

          The low-order 9 bits of msg_perm.mode are set to the low-order 9
          bits of msgflg.

          msg_qnum, msg_lspid, msg_lrpid, msg_stime, and msg_rtime are set to
          0.

          msg_ctime is set to the current time.

          msg_qbytes is set to the system limit.

     msgget fails if one or more of the following are true:

     EACCES         A message queue identifier exists for key, but operation
                    permission [see intro(2)] as specified by the low-order 9
                    bits of msgflg would not be granted.

     ENOENT         A message queue identifier does not exist for key and
                    (msgflg&IPC_CREAT) is false.

     ENOSPC         A message queue identifier is to be created but the
                    system-imposed limit on the maximum number of allowed
                    message queue identifiers system wide would be exceeded.

     EEXIST         A message queue identifier exists for key but
                    (msgflg&IPC_CREAT) and (msgflg&IPC_EXCL) are both true.

SEE ALSO
     intro(2), msgctl(2), msgop(2), stdipc(3C).

DIAGNOSTICS
     Upon successful completion, a non-negative integer, namely a message
     queue identifier, is returned.  Otherwise, a value of -1 is returned and
     errno is set to indicate the error.
	 COMMENT: How can this operate properly in a threaded environment???
*/
	int msgq_ident = 0, msgq_exists = 0;
	msg_t *pMsgq = NULL;
	pid_t userid, groupid;

	IntiializeMQSystem;			/* OS-specific macro. Once per process intitialization */

	/*	Assure thread is initialized */
	ThreadIdInit();

	userid = em_getuid();
	groupid = em_getgid();

	pthread_mutex_lock(mq_table.q_guard);

	if (key != IPC_PRIVATE) { /* See if a message queue with this key already exists */
		msgq_ident = msgq_lookup (key);
		msgq_exists = (msgq_ident > 0);
		if (msgq_exists) { /* An entry exists. Check for other possible errors */
			if ((msgflg & IPC_CREAT) && (msgflg & IPC_EXCL)) {
				SetErrno(EEXIST);
				msgq_ident = -1;
			} else if (!msgq_access (msgq_ident, msgflg, 0)) { /* Test for read access */
				SetErrno(EACCES);
				msgq_ident = -1;
			} else { /* The message q exists and can be used */ }
		} else if (!msgq_exists && (msgflg & IPC_CREAT == 0)) {
		/* Message q does not exist. Error if the create flag is not set */
			SetErrno(ENOENT);
			msgq_ident = -1;
		} 
	}

	if (key == IPC_PRIVATE || (!msgq_exists && !(msgflg & IPC_CREAT == 0))) {
		/* Either key is IPC_PRIVATE, or key does not already have	*/
		/* a message queue identifier								*/
		/* associated with it, and (msgflg & IPC_CREAT) is true.	*/
		/* In either case, create a message q with this key			*/
		msgq_ident = msgq_add (key, msgflg);
		if (msgq_ident <= 0) {
			SetErrno(ENOSPC);
			msgq_ident = -1;
		}
	}
	
	pthread_mutex_unlock(mq_table.q_guard);

	return msgq_ident;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int msgctl (int msqid, int cmd, .../* struct msqid_ds *buf */)
{
/*

NAME
     msgctl - message control operations

SYNOPSIS
     #include <sys/types.h>
     #include <sys/ipc.h>
     #include <sys/msg.h>

     int msgctl (int msqid, int cmd, .../* struct msqid_ds *buf );

DESCRIPTION
     msgctl provides a variety of message control operations as specified by
     cmd.  The following cmds are available:

     IPC_STAT    Place the current value of each member of the data structure
                 associated with msqid into the structure pointed to by buf.
                 The contents of this structure are defined in intro(2).

     IPC_SET     Set the value of the following members of the data structure
                 associated with msqid to the corresponding value found in the
                 structure pointed to by buf:
                       msg_perm.uid
                       msg_perm.gid
                       msg_perm.mode /* only access permission bits 
                       msg_qbytes

                 This cmd can only be executed by a process that has an
                 effective user ID equal to the value of msg_perm.cuid or
                 msg_perm.uid in the data structure associated with msqid, or
                 by a process that has the super-user privilege.

     A process with the super-user privilege can raise the value of
     msg_qbytes.

     IPC_RMID
          Remove the message queue identifier specified by msqid from the
          system and destroy the message queue and data structure associated
          with it.  This cmd can only be executed by a process that has an
          effective user ID equal to either that of super user, or to the
          value of msg_perm.cuid or msg_perm.uid in the data structure
          associated with msqid.

     msgctl fails if one or more of the following are true:

	 EACCES         cmd is IPC_STAT and operation permission is denied to the
                    calling process [see intro(2)].

     EFAULT         buf points to an illegal address.

     EINVAL         msqid is not a valid message queue identifier.

     EINVAL         cmd is not a valid command.

     EINVAL         cmd is IPC_SET and msg_perm.uid or msg_perm.gid is not
                    valid.

     EOVERFLOW      cmd is IPC_STAT and uid or gid is too large to be stored
                    in the structure pointed to by buf.

     EPERM          cmd is IPC_RMID or IPC_SET, the effective user ID of the
                    calling process is not equal to the value of msg_perm.cuid
                    or msg_perm.uid in the data structure associated with
                    msqid and the process does not have the super-user
                    privilege.

     EOVERFLOW      cmd is IPC_STAT and uid or gid is too large to be stored
                    in the structure pointed to by buf.

     EPERM          cmd is IPC_SET, an attempt is being made to increase to
                    the value of msg_qbytes, and the calling process does not
                    have the super-user privilege.

SEE ALSO
     intro(2), msgget(2), msgop(2 (this is msgsnd and msgrcf).

DIAGNOSTICS
     Upon successful completion, a value of 0 is returned. Otherwise, a value
     of -1 is returned and errno is set to indicate the error.
*/
	int Result = -1, msqidm1 = msqid-1;
	msg_t *pMsgq;
	va_list pArgList;	/* Pointer to extended arguments containing buf. */
	PMSQID_DS buf;
	pid_t userid, groupid;

	/*	Assure thread is initialized */
	ThreadIdInit();

	userid = em_getuid();
	groupid = em_getgid();

	pthread_mutex_lock(mq_table.q_guard);

	if (!msgq_valid (msqid)) {
		SetErrno(EINVAL);
	} else { /* Note: the rest of the code is intentionally not indented */

	pMsgq = (struct msg *)&mq_table.mq[msqidm1];
	va_start (pArgList, cmd);	/* Point to the buffer, if present. */

	switch (cmd) {
		case IPC_STAT:
			/*	Place the current value of each member of the data structure
                associated with msqid into the structure pointed to by buf.
                The contents of this structure are defined in intro(2). 
				Comment: No they aren't. We'll use the 4 values from IPC_SET
			*/
			buf = va_arg (pArgList, void *);
			va_end (pArgList);
			if (buf == NULL) { 
				SetErrno(EFAULT);
			} else if (!msgq_access (msqid, 0, 1)) { /* Read access denied */
				SetErrno(EACCES);
			}
			else {
				buf->msg_perm.uid  = pMsgq->msg_perm.uid;
				buf->msg_perm.cuid = pMsgq->msg_perm.cuid;
				buf->msg_perm.gid  = pMsgq->msg_perm.gid;
				buf->msg_perm.cgid = pMsgq->msg_perm.cgid;
				buf->msg_perm.mode = pMsgq->msg_perm.mode;
				buf->msg_qbytes = pMsgq->msg_qbytes;
				Result = 0;
			}
			break;

		case IPC_SET:
			if (em_geteuid() != pMsgq->msg_perm.cuid && 
				em_geteuid() != pMsgq->msg_perm.uid  &&
				em_geteuid() != 0) {
					SetErrno(EPERM);
				}
			else {
				buf = va_arg (pArgList, void *);
				va_end (pArgList);
				if (buf == NULL) {
					SetErrno(EFAULT);
				}
				else {
					pMsgq->msg_perm.uid  = buf->msg_perm.uid;
					pMsgq->msg_perm.cuid = buf->msg_perm.cuid;
					pMsgq->msg_perm.gid  = buf->msg_perm.gid;
					pMsgq->msg_perm.cgid = buf->msg_perm.cgid;
					pMsgq->msg_perm.mode = buf->msg_perm.mode;
					if (buf->msg_qbytes > pMsgq->msg_qbytes && em_getuid() != 0) {
						/* Not super user, and the q size is being increased */
						SetErrno(EPERM);
					} else {
						pMsgq->msg_qbytes = buf->msg_qbytes;
					}
					Result = 0;
				}
			}
			break;

		case IPC_RMID:
			/*	Remove the message queue identifier specified by msqid from the
				system and destroy the message queue and data structure associated
				with it.  This cmd can only be executed by a process that has an
				effective user ID equal to either that of super user, or to the
				value of msg_perm.cuid or msg_perm.uid in the data structure
				associated with msqid.
			*/
			if (em_geteuid() != pMsgq->msg_perm.cuid && 
				em_geteuid() != pMsgq->msg_perm.uid  &&
				em_geteuid() != 0) {
					SetErrno(EPERM);
				}
			else {
				/* The message queue must be marked unbusy before doing any of these 
				 * operations, however, as queue properties are about to be changed 
				 * We could get here while a msgsnd or msgrcv is waiting on a CV with 
				 * the mutex unlocked, but, in that case, the queue's msg_busy flag   
				 * will be set, so wait for a queue to become not busy and try again  
				 */
				while (mq_table.mq[msqidm1].msg_busy != 0) {
					pthread_cond_wait (mq_table.e_q_not_busy, mq_table.q_guard);
				}

				/* Free all messages associated with this queue */
				if (msgq_remove (msqid)) Result = 0;
			}
			break;

		default:
			SetErrno(EINVAL);
	}

	pthread_mutex_unlock(mq_table.q_guard);
	}

	return Result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static THREAD_SPECIFIC_INDEX thread_id_key;

#ifdef _WINDOWS
#define MQ_MUTEX_NAME _T("MessageQueueMutex")

void InitializeMqTable(int *pOnceFlag)
{
	/* One time initialization function. Guaranteed to operate only one time */
	/* assuming it is always called with the same argument					*/

	HANDLE hm;
	int Done = 0, One = 1;
	void * null_value = NULL;
	MQ_THREAD_ID *pThreadId;

	Done = InterlockedExchange (pOnceFlag, One);
	if (Done == 1) return;

	hm = CreateMutex (NULL, TRUE, MQ_MUTEX_NAME);
	if (hm == NULL) return;
	
	/* This is the first time; the mutex is newly created */
	/* The mutex is locked, so we can safely modify the mq table */
	mq_table.q_guard = hm;  /* Store its handle */

	/* Set up all the event handles for various message queue state changes */
	/* All events are manual reset and pulsed, so as to release multiple threads */
	/* Using a "broadcast model" */
	/* Note that these events/cvs are "course grained" in that threads are signaled */
	/* even if the queue that they are waiting on has not been affected. An improvement */
	/* would be to add a mutex and a set of events/cvs to each individual queue */
	/* Be awere, though, that global state changes need to be broadcast on any */
	/* state change */

	mq_table.e_qne = CreateEvent (NULL, TRUE, FALSE, NULL);
	mq_table.e_qnf = CreateEvent (NULL, TRUE, TRUE, NULL); /* initially set */
	mq_table.e_q_not_busy = CreateEvent (NULL, TRUE, FALSE, NULL);

	/* Create a thread specific key to hold this thread's identity and errno */
	ts_key_create(thread_id_key, destructor);
	pThreadId = calloc (1, sizeof(MQ_THREAD_ID));
	pthread_setspecific (thread_id_key, pThreadId);

	/* Initialize the table with empty entries */
	empty_table();
	ReleaseMutex (hm); 
	
	/*	The mutex exists, is unowned, and there is only one handle to it */
	/*	Assuming that this function is always called with the same argument */

	return;
}
#else

void destructor (void *p)
{
}

void once_init_routine (void)
{
	/*	Intitialize the mutex and the three condition variables */
	pthread_mutex_init (&mq_table.q_guard, NULL);
	pthread_cond_init (&mq_table.e_qne, NULL);
	pthread_cond_init (&mq_table.e_qnf, NULL);
	pthread_cond_init (&mq_table.e_q_not_busy, NULL);
	/* Initialize the table with empty entries */
	/* Access is serialized, so there is no need to lock the mutex */

	/* Create a thread specific key to hold this thread's identity and errno */
	ts_key_create(thread_id_key, destructor);

	empty_table();

}
#endif

void empty_table()
{
	/*  Initialization of the message queue table to indicate there are no queues */
	memset (mq_table.mq, 0, sizeof(mq_table.mq)); 
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*	Utility functions to manage the message queue table */
/*	The correctness depends on the fact that we know this function is always */
/*	called before msgq_access or msgq_add are called, assuring correct		*/
/*	initialization through the macro, one_time.								*/
/*	Must be called with the mutex locked									*/

int msgq_lookup (key_t key)
{
	int result = 0;
	size_t i;

	for (i = 0; i < mq_table.q_max; i++) {
		if (mq_table.mq[i].msg_perm.key == key) {
			result = i+1;
			break;
		}
	}
	if (result != 0) return result;
	return -1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int msgq_access (int msgq_ident, int msgflg, int op)
{
	/* Determine if the MQ can be accessed as requested by op 
		op_in values: 
		0: Use requested operations as specified in msgflg
		1: Read, 2: Write, 3: read/write) */

	int mqidm1 = msgq_ident - 1;
	uid_t p_uid, o_uid;
	gid_t p_gid, o_gid;
	int mode, ok = 0;

	/* Process Ids */
	p_uid  = em_getuid();
	p_gid = em_getgid();

	/* Queue owners */
	o_uid  = mq_table.mq[mqidm1].msg_perm.uid;
	o_gid  = mq_table.mq[mqidm1].msg_perm.gid;

	/* Access allowed (UNIX-style permission bits) */
	mode = mq_table.mq[mqidm1].msg_perm.mode;

	/* Determine if access is allowed via brute-force enumeration of cases */
	ok = ( (op == 1) && (((p_uid == o_uid) && (mode & 0400)) || 
						 ((p_gid == o_gid) && (mode & 0040)) ||
						                      (mode & 0004) )
	   ||  (op == 2) && (((p_uid == o_uid) && (mode & 0200)) || 
						 ((p_gid == o_gid) && (mode & 0020)) ||
						                      (mode & 0002) )
	   ||  (op == 3) && (((p_uid == o_uid) && (mode & 0600)) || 
						 ((p_gid == o_gid) && (mode & 0060)) ||
						                      (mode & 0006) ) )
		/* In the last case, the mode must permit all the operations in the flag */
	   ||  (op == 0) && (((mode & msgflg) == msgflg)  )  ;

/*
printf ("msgq_access. op: %x. p_uid: %d. p_gid: %d. o_uid: %d. o_gid: %d. mode: %o. ok: %d\n",
		              op,     p_uid,     p_gid,     o_uid,     o_gid,     mode,     ok);
*/

	return ok;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int msgq_add (key_t key, int msgflg)
{
	/* Must be called with the mutex locked */
	/* Message queue identifiers start at 1, not zero */
	int Result = -1;
	size_t i;
	msg_t *pMsgq;

	if (mq_table.q_current > mq_table.q_max) {
		/*	Error. Not enough space in the table */
		SetErrno(ENOSPC);
	} else {
		/*	Add the new message queue to the table and return the index */
		/*	Scan the table to look for an empty slot					*/
		for (i = 0; i < mq_table.q_max; i++) {
			if (mq_table.mq[i].msg_perm.key == 0) break; /* Empty slot */
		}
		/*  There has to be an empty slot, but test anyhow to be sure */
		if (i >= mq_table.q_max) {
			SetErrno(ENOSPC);
		} else {
			mq_table.mq[i].msg_perm.key = key;
			mq_table.mq[i].msg_busy = 0;
			pMsgq = (struct msg *)&mq_table.mq[i];
			Result = i + 1; /* Note: return index starts at 1 */
			mq_table.q_current++;
			/* Initialize all components as required */
			pMsgq->msg_perm.cuid = pMsgq->msg_perm.uid = em_getuid();
			pMsgq->msg_perm.cgid = pMsgq->msg_perm.gid = em_getgid();
			pMsgq->msg_perm.mode = (msgflg & 0x1FF); /* Low order 9 bits */
			pMsgq->msg_qnum = pMsgq->msg_lspid = pMsgq->msg_lrpid = 0;
			pMsgq->msg_stime = pMsgq->msg_rtime = 0;
			pMsgq->msg_ctime = time (NULL);
			pMsgq->msg_qbytes = MQ_MAX_BYTES_IN_QUEUE;
		}
	}

	return Result;
}

int msgq_remove (int msqid)
{
	/* Free all messages associated with this queue */
	/* The mutex must be locked, and this message queue can not be busy */
	int msqidm1 = msqid - 1;
	struct _MESSAGE_T *pMsg, *pNext;

	pMsg = mq_table.mq[msqidm1].msg_first;
	/* Delete all messages */
	while (pMsg != NULL) {
		pNext = pMsg->pNext;
		free (pMsg);
		pMsg = pNext;
	}

	mq_table.mq[msqidm1].msg_first = mq_table.mq[msqidm1].msg_last = NULL;
	mq_table.mq[msqidm1].msg_lrpid = mq_table.mq[msqidm1].msg_lspid = 0;
	mq_table.mq[msqidm1].msg_ctime = mq_table.mq[msqidm1].msg_rtime 
		= mq_table.mq[msqidm1].msg_stime = 0;
	mq_table.mq[msqidm1].msg_busy = mq_table.mq[msqidm1].msg_cbytes
		= mq_table.mq[msqidm1].msg_qnum = mq_table.mq[msqidm1].msg_qbytes = 0;
	memset (&(mq_table.mq[msqidm1].msg_perm), 0, sizeof(ipc_perm_t));
	return 1;
}


int msgq_valid (int msgid)
{
	/* Must be called with the mutex locked */
	int Valid = 0;
	/* Is the identified message q valid ? */
	/* Remember that messasge Q numbering starts at 1 */

	if (msgid <= 0 || msgid > MSG_MAX_NUMBER_QUEUES) return 0;
	
	Valid = (mq_table.mq[msgid-1].msg_perm.key > 0 && mq_table.mq[msgid-1].msg_ctime != 0);
	
	return Valid;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*	Assure thread is initialized - Once per thread */
void ThreadIdInit()
{
	MQ_THREAD_ID *pThId, *pNow;
	IntiializeMQSystem;			/* OS-specific macro. Once per process intitialization */

	pNow = pthread_getspecific(thread_id_key);
	if (pNow != NULL) return;
	pThId = calloc (1, sizeof(MQ_THREAD_ID));
	pthread_setspecific (thread_id_key, pThId);
}

/*  Set all thread-specific UIDs and group Ids */
void em_set_all_ids (int uid, int euid, int gid, int egid)
{
	MQ_THREAD_ID *pThId;
	/*	Assure thread is initialized */
	ThreadIdInit();

	pThId = pthread_getspecific (thread_id_key);
	pThId->uid = uid;
	pThId->euid = euid;
	pThId->gid = gid;
	pThId->egid = gid;
}
/*	Emulate UNIX user and group ids, obtaining the thread-specific values */
int em_getuid()
{
	MQ_THREAD_ID *pThId;
	pThId = pthread_getspecific (thread_id_key);
	return pThId->uid;
}

int em_geteuid()
{
	MQ_THREAD_ID *pThId;
	pThId = pthread_getspecific (thread_id_key);
	return pThId->euid;
}

int em_getgid()
{
	MQ_THREAD_ID *pThId;
	pThId = pthread_getspecific (thread_id_key);
	return pThId->gid;
}

int em_getegid()
{
	MQ_THREAD_ID *pThId;
	pThId = pthread_getspecific (thread_id_key);
	return pThId->egid;
}

/*	Thread safe error number management. errno is set, but is not reliable */
void SetErrno (int Value)
{
	/* Set the thread-specific error number */
	MQ_THREAD_ID *pThId;
	pThId = pthread_getspecific (thread_id_key);
	pThId->th_errno = Value;
	errno = Value;	/* this is not thread-safe, but is done anyhow to conform to the spec */
}

int  GetErrno ()
{
	/* Get the thread-specific error number */
	MQ_THREAD_ID *pThId;

	pThId = pthread_getspecific (thread_id_key);
	
	return pThId->th_errno;
}


