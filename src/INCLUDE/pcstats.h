/* pcstats.h. Definitions for Chapter 10.				*/

typedef struct statistics_tag { /* Thread-specific statistics */
	MSG_BLOCK *pmblock;
	DWORD th_number;		/* Sequential thread id		*/
	DWORD nmsgs;		/* Number of messages consumed	*/
	DWORD ngood;		/* Number of good messages	*/
	DWORD nbad;		/* Number of bad messages	*/
	double data_sum; 	/* Sum of all data		*/
	time_t firstmsg;	/* Time stamp of first message	*/
	time_t lastmsg;		/* Time stamp of last message	*/
} statistics_t;

DWORD ts_index;		/* Index of thread specific key */

static DWORD thread_number = 0;

#if defined(PCSTATSDLL_EXPORTS)
#define LIBSPEC_STATS __declspec (dllexport)
#elif defined(__cplusplus)
#define LIBSPEC_STATS extern "C" __declspec (dllimport)
#else
#define LIBSPEC_STATS __declspec (dllimport)
#endif

LIBSPEC_STATS VOID report_statistics (VOID);
LIBSPEC_STATS VOID accumulate_statistics (VOID);