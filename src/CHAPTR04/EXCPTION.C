/* Chapter 4.  Excption.c
	Generate exceptions intentionally and respond to them. */

#include "Everything.h"
#include <float.h>

DWORD Filter (LPEXCEPTION_POINTERS, LPDWORD);
double x = 1.0, y = 0.0;
int _tmain (int argc, LPTSTR argv [])
{
	DWORD eCategory, i = 0, ix, iy = 0;
	LPDWORD pNull = NULL;
	BOOL done = FALSE;
	DWORD fpOld, fpNew, fpOldDummy;
	//__try { /* Try-Finally block. */
						/* Save old control mask. */
	_controlfp_s (&fpOld, 0, 0);
						/* Enable floating-point exceptions. */
	fpNew = fpOld & ~(EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT
			| EM_ZERODIVIDE | EM_DENORMAL | EM_INVALID);
						/* Set new control mask. */
	_controlfp_s (&fpOldDummy, fpNew, MCW_EM);
	while (!done) __try {
		_tprintf (_T("Enter exception type:\n"));
		_tprintf (_T("1: Mem, 2: Int, 3: Flt 4: User 5: _leave 6: return\n"));
		_tscanf_s (_T("%d"), &i);
		__try { /* Try-Except block. */
			switch (i) {
			case 1: /* Memory reference. */
				ix = *pNull;
				*pNull = 5;
				break;
			case 2: /* Integer arithmetic. */
				ix = ix / iy;
				break;
			case 3: /* floating-point exception. */
				x = x / y;
				_tprintf (_T("x = %20e\n"), x);
				break;
			case 4: /* User generated exception. */
				ReportException (_T("Raising user exception.\n"), 1);
				break;
			case 5: /* Use the _leave statement to terminate. */
				done = TRUE;
				__leave;
			case 6: /* Use the return statement to terminate. */
				return 1;
			default: done = TRUE;
			}
		} /* End of inner __try. */

		__except (Filter (GetExceptionInformation (), &eCategory)){
			switch (eCategory) {
			case 0:	_tprintf (_T("Unknown exception.\n"));
				break;
			case 1:	_tprintf (_T("Memory ref exception.\n"));
				break;
			case 2:	_tprintf (_T("Integer arithmetic exception.\n"));
				break;
			case 3:	
	 			_tprintf (_T("floating-point exception.\n"));
				break;
			case 10: _tprintf (_T("User generated exception.\n"));
				break; 
			default: _tprintf (_T("Unknown exception.\n"));
				break;
			}
			_tprintf (_T("End of handler.\n"));
		} /* End of inner __try __except block. */


	
	//} /* End of exception generation loop. */

	//return; /* Cause an abnormal termination. */

	} /* End of outer __try __finally */
	__finally {
		BOOL AbTerm; /* Restore the old mask value. */
		_controlfp_s (&fpOldDummy, fpOld, MCW_EM);
		AbTerm = AbnormalTermination();
		_tprintf (_T("Abnormal Termination?: %d\n"), AbTerm);
 	}
	return 0;
}

static DWORD Filter (LPEXCEPTION_POINTERS pExP, LPDWORD eCategory)

/*	Categorize the exception and decide whether to continue execution or
	execute the handler or to continue the search for a handler that
	can process this exception type. The exception category is only used
	by the exception handler. */
{
	DWORD exCode;
	DWORD_PTR readWrite, virtAddr;
	exCode = pExP->ExceptionRecord->ExceptionCode;
	_tprintf (_T("Filter. exCode: %x\n"), exCode);
	if ((0x20000000 & exCode) != 0) {
				/* User Exception. */
		*eCategory = 10;
		return EXCEPTION_EXECUTE_HANDLER;
	}

	switch (exCode) {
		case EXCEPTION_ACCESS_VIOLATION:
				/* Determine whether it was a read, write, or execute
					and give the virtual address. */
			readWrite =
				(DWORD)(pExP->ExceptionRecord->ExceptionInformation [0]);
			virtAddr =
				(DWORD)(pExP->ExceptionRecord->ExceptionInformation [1]);
			_tprintf
				(_T("Access Violation. Read/Write/Execute: %d. Address: %x\n"),
				readWrite, virtAddr);
			*eCategory = 1;
			return EXCEPTION_EXECUTE_HANDLER;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			*eCategory = 1;
			return EXCEPTION_EXECUTE_HANDLER;
					/* Integer arithmetic exception. Halt execution. */
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_OVERFLOW:
			*eCategory = 2;
			return EXCEPTION_EXECUTE_HANDLER;
					/* Float exception. Attempt to continue execution. */
					/* Return the maximum floating value. */
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_OVERFLOW:
			_tprintf (_T("Flt Exception - Large result.\n"));
			*eCategory = 3;
			_clearfp();
			return EXCEPTION_EXECUTE_HANDLER;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
		case EXCEPTION_FLT_INEXACT_RESULT:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_FLT_STACK_CHECK:
			_tprintf (_T("Flt Exception - Unknown result.\n"));
			*eCategory = 3;
			return EXCEPTION_CONTINUE_EXECUTION;
					/* Return the minimum floating value. */
		case EXCEPTION_FLT_UNDERFLOW:
			_tprintf (_T("Flt Exception - Small result.\n"));
			*eCategory = 3;
			return EXCEPTION_CONTINUE_EXECUTION;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			*eCategory = 4;
			return EXCEPTION_CONTINUE_SEARCH;
		case STATUS_NONCONTINUABLE_EXCEPTION:
			*eCategory = 5;
			return EXCEPTION_EXECUTE_HANDLER;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_PRIV_INSTRUCTION:
			*eCategory = 6;
			return EXCEPTION_EXECUTE_HANDLER;
		case STATUS_NO_MEMORY:
			*eCategory = 7;
			return EXCEPTION_EXECUTE_HANDLER;
		default:
			*eCategory = 0;
			return EXCEPTION_CONTINUE_SEARCH;
	}
}
