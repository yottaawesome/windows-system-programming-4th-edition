/* Definitions for client/server communication. */
/* request and response messages. No Unicode as
	the request may be coming from a Windows 95 system. */
/* Message length fields are explicitely declared 32 bits 
   to avoid any ambiguity and to assure interoperability 
   between Win32 and future Win64 systems */

#define MAX_RQRS_LEN 0x1000
#define MAX_MESSAGE_LEN MAX_RQRS_LEN

typedef struct {	/* Generic message. */
	LONG32 msgLen;	/* Total length of request, not including this field */
	BYTE record [MAX_RQRS_LEN];
} MESSAGE;

typedef struct {	/* Same as a message; only the length field has a different name */
	LONG32 rqLen;	/* Total length of request, not including this field */
	BYTE record [MAX_RQRS_LEN];
} REQUEST;

typedef struct {	/* Same as a message; only the length field has a different name */
	LONG32 rsLen;	/* Total length of response, not including this field */
	BYTE record [MAX_RQRS_LEN];
} RESPONSE;

#define RQ_SIZE sizeof (REQUEST)
#define RQ_HEADER_LEN RQ_SIZE-MAX_RQRS_LEN
#define RS_SIZE sizeof (RESPONSE)
#define RS_HEADER_LEN RS_SIZE-MAX_RQRS_LEN

/* Mailslot message structure */
typedef struct {
	DWORD32 msStatus;
	DWORD32 msUtilization;
	TCHAR msName[MAX_PATH];
} MS_MESSAGE;

#define MSM_SIZE sizeof (MS_MESSAGE)

#define CS_TIMEOUT 5000
	/* Timeout period for named pipe connections and performance monitoring. */

#define MAX_CLIENTS  4 ///***10 /* Maximum number of clients for serverNP */
#define MAX_SERVER_TH 4 /* Maximum number of server threads for serverNPCP */
#define MAX_CLIENTS_CP 128 /* Maximum number of clients for serverNPCP */

/* Client and server pipe & mailslot names.
	See Table 3 in the book for alternatives to the DOT as the root name. */

#define SERVER_PIPE _T ("\\\\.\\PIPE\\SERVER")
#define CLIENT_PIPE _T ("\\\\.\\PIPE\\SERVER")
#define MS_SRVNAME _T ("\\\\.\\MAILSLOT\\CLS_MAILSLOT")
#define MS_CLTNAME _T ("\\\\.\\MAILSLOT\\CLS_MAILSLOT")

/*  Alternative names if the client and server systems are
	networked. Alternatively, the application server can
	find its name and create the name to be used.  */
//#define CLIENT_PIPE _T ("\\\\ServerName\\PIPE\\SERVER")
/*  Use the first form below to find any application server
	and the second form to find a specific one */
//#define MS_CLTNAME _T ("\\\\*\\MAILSLOT\\CLS_MAILSLOT")
//#define MS_CLTNAME _T ("\\\\MailslotServerName\\MAILSLOT\\CLS_MAILSLOT")

/*  Mutex and semaphore names to be used by clientCO and
	serverMT.								*/
#define MX_NAME _T ("ClientServerMutex")
#define SM_NAME _T ("ClientServerSemaphore")

/* Name of the program that broadcasts the pipe name. */

#define SERVER_BROADCAST _T ("SrvrBcst.exe")

/* Commands for the statistics maintenance function. */

#define CS_INIT			1
#define CS_RQSTART		2
#define CS_RQCOMPLETE	3
#define CS_REPORT		4
#define CS_TERMTHD		5

/* Client/Server support functions. */

BOOL LocateServer (LPTSTR, DWORD);

/* Required for sockets */
#define SERVER_PORT 50000   /* Well known port. */

