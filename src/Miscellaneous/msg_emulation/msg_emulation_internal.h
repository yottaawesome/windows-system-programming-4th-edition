/*		msg_emulation_internal.h									*/
/*		Author: John Hart, July 27, 2000							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
#ifndef _MSG_EMULATION_INTERNALS
#define _MSG_EMULATION_INTERNALS

#include <time.h>
/*	System limits															*/
#define MSG_MAX_NUMBER_QUEUES		256	
#define MQ_MAX_MESSAGE_SIZE			0x10000		/* 65K */
#define MQ_MAX_BYTES_IN_QUEUE		MQ_MAX_MESSAGE_SIZE*16 /* arbitary Q upper bound  */
#define MQ_MAX_BYTES_IN_SYSTEM		MQ_MAX_BYTES_IN_QUEUE*MSG_MAX_NUMBER_QUEUES/16

/* Messages - Note that the message length and list pointers are added to the */
/* UNIX man page specification													*/

typedef struct _MESSAGE_T {
		struct _MESSAGE_T	*pNext;
		struct _MESSAGE_T	*pPrev;
		long				mtype;		/* message type */
		size_t				mlength;	/* Message Length; not counting type field */
		long				msg_mtype;
} MESSAGE_T;

typedef struct ipc_perm {
          uid_t   cuid;       /* creator user id */
          gid_t   cgid;       /* creator group id */
          uid_t   uid;        /* user id */
          gid_t   gid;        /* group id */
          mode_t  mode;       /* r/w permission */
          ulong_t seq;        /* slot usage sequence # */
          key_t   key;        /* key */
} ipc_perm_t;

typedef struct msg { /* This is the message Q structure, not the message itself */
          volatile int     msg_busy;	/* Set to indicate that the queue is in use */
          ipc_perm_t   msg_perm; 
          struct  _MESSAGE_T *msg_first;	/* pointer to the first message on the queue. */
          struct  _MESSAGE_T *msg_last;	/* pointer to the last message on the queue. */
          ulong_t msg_cbytes;		/* current number of bytes on the queue. */
          ulong_t msg_qnum;			/* number of messages currently on the queue. */
          ulong_t msg_qbytes;		/* maximum number of bytes allowed on the queue. */
          pid_t   msg_lspid;		/* process id of last process that performed a msgsnd */
          pid_t   msg_lrpid;		/* process id of last process that performed a msgrcv */
          time_t  msg_stime;		/* is the time of the last msgsnd operation. */
          time_t  msg_rtime;		/* time of the last msgrcv operation */
          time_t  msg_ctime;		/* time of last msgctl(2) that changed structure. */
} msg_t;

/* * * * * * * * * * * * * * * * * * * * * * * * */
/*
     Message Queue Identifier A message queue identifier (msqid) is a unique
     positive integer created by a msgget(2) system call.  Each msqid has a
     message queue and a data structure associated with it.  The data
     structure is referred to as msqid_ds and contains the following members:


     Message Operation Permissions In the msgop(2) and msgctl(2) system cal
     descriptions, the permission required for an operation is given as
     "{token}", where "token" is the type of permission needed, interpreted
     follows:

            00400          Read by user
            00200          Write by user
            00040          Read by group
            00020          Write by group
            00004          Read by others
            00002          Write by others

	Read and write permissions on a msqid are granted to a process if one
    more of the following are true:

          The effective user ID of the process is super-user.

          The effective user ID of the process matches msg_perm.cuid or
          msg_perm.uid in the data structure associated with msqid and the
          appropriate bit of the ``user'' portion (0600) of msg_perm.mode i
          set.

          The effective group ID of the process matches msg_perm.cgid or
          msg_perm.gid and the appropriate bit of the ``group'' portion (06
          of msg_perm.mode is set.

          The appropriate bit of the ``other'' portion (006) of msg_perm.mo
          is set.

  Otherwise, the corresponding permissions are denied.
*/

#endif
