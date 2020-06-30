#ifndef __errors_h
#define __errors_h

#ifndef _WINDOWS
#include <unistd.h>
#include <errno.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* From PWPT (Programming with POSIX Threads, by David Butenhof, 
 * Addison-Wesley, 1997) pp 33-34.
 * Define a macro that can be used for diagnostic output from
 * examples. When compiled -DDEBUG, it results in calling printf
 * with the specified argument list. When DEBUG is not defined, it
 * expands to nothing.
 */
#ifdef DEBUG
#define DPRINTF(arg) printf arg
#else
#define DPRINTF(arg)
#endif

/*
 * NOTE: the "do {" ... "} while(o);" bracketing around the macros
 * allows the err_abort and errno_abort macros to be used as if they
 * were function calls, even in contexts where a trailing ";" would
 * genrate a null statement. For example,
 *
 *	if (status != 0)
 *		err_abort (status, "message");
 *	else
 *		return status;
 *
 * will not comple if err_abort is a macro ending with "}", because
 * C does not expect a ";" to follow the "}". Because C does expect
 * a ";" following the "}" in the do...while constuct, err_abort and
 * errno_abort cah be used as if they were function calls.
 */

#ifdef _WINDOWS
#define errno_get (FormatMessage ( \
				FORMAT_MESSAGE_FROM_SYSTEM, NULL, \
				GetLastError(), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), \
				Msg, 256, NULL),Msg)

#else
#define errno_get strerror(errno)
#endif

#define err_abort(code,text) do {\
 	fprintf (stderr, "%s at \"%s\":%d: %s\n", \
 		text, __FILE__, __LINE__, strerror (code)); \
 	abort (); \
 	} while (0)
#define errno_abort(text) do {char Msg[256]; \
 	fprintf (stderr, "%s at \"%s\":%d: %s\n", \
 		text, __FILE__, __LINE__, errno_get); \
 	abort (); \
 	} while (0)
#define err_stop(text) do {char Msg[256]; \
 	fprintf (stderr, "%s at %s: %d. %s\n", \
 		text, __FILE__, __LINE__, errno_get); \
 	return 1; \
 	} while (0)
#define err_stop0(text) do {\
 	fprintf (stderr, "%s at %s: %d.\n", \
 		text, __FILE__, __LINE__); \
 	return ; \
 	} while (0)

#endif
