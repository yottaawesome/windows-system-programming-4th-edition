/*	msg_emulation.h											*/
/*	Emulate the UNIX msg functions using SF_INET sockets	*/
/*		Author: John Hart, July 27, 2000					*/
#ifndef _MSG_EMULATION
#define _MSG_EMULATION

#ifdef _WINDOWS
/* Definitions required for Windows */
#include <tchar.h>
#include <windows.h>
#else
/* Definitions required for OpenVMS */
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#endif

#define size_t	unsigned long int
#define key_t	unsigned long int
#define uid_t	int
#define gid_t	int
#define mode_t	int
#define pid_t	int
#define ulong_t	unsigned long int

#define IPC_CREAT       0x1000
#define IPC_EXCL        0x2000
#define IPC_STAT		0x4000
#define IPC_PRIVATE     0x8000
#define IPC_SET			0x10000
#define IPC_RMID		0x20000
#define IPC_NOWAIT		0x40000
#define IPC_WAIT		0x0

#ifndef S_IRUSR 
#define S_IRUSR			0400
#define S_IWUSR			0200
#endif

#define MSG_NOERROR		0x2000

/*	UNIX error numbers.	Reference: man 2 intro on a Solaris system */
#define EPERM		1
#define ENOENT      2
#define E2BIG		7
#define EAGAIN		11
#define	ENOMEM		12
#define EACCES      13
#define	EFAULT		14
#define EBUSY		16
#define EEXIST      17
#define EINVAL		22
#define ENOSPC		28
#ifndef ENOMSG
#define ENOMSG		35
#endif
#ifndef EIDRM
#define EIDRM		36
#endif
#define EOVERFLOW	79

#include "msg_emulation_internal.h"

#ifdef _WINDOWS
/*  Two libraries export these interfaces: 
		1)  The actual implementation (msg_emulation_dll)
		2)  The client stub (msg_client_dll
*/
#ifdef MSG_EMULATION_DLL_EXPORTS
#define _MSG_MODIFIER __declspec (dllexport)
#else
#define _MSG_MODIFIER __declspec (dllimport)
#endif

#else 
#define _MSG_MODIFIER 
#endif /* _WINDOWS */

/*	The four UNIX message functions */
_MSG_MODIFIER int msgsnd (int msqid, const void *msgp, size_t msgsz, int msgflg);
_MSG_MODIFIER int msgrcv (int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
_MSG_MODIFIER int msgget (key_t key, int msgflg);
_MSG_MODIFIER int msgctl (int msqid, int cmd, .../* struct msqid_ds *buf */);

/* Functions to control thread-specific errno and ids */
_MSG_MODIFIER int  GetErrno ();
static void SetErrno (int);
_MSG_MODIFIER void em_set_all_ids (int, int, int, int);

/*	Message structure definition											*/
typedef struct _MSQID_DS {
		ipc_perm_t	msg_perm;
        ulong_t	msg_qbytes;
} MSQID_DS, *PMSQID_DS;


#endif
