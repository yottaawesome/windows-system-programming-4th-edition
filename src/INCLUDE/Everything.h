#ifndef _NOEXCLUSIONS
#include "Exclude.h"	/* Define Preprocessor variables to */
			/* exclude un-wanted header files. */
#endif
#include "Environment.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <WinSock2.h>
#include "support.h"
#ifdef _MT
#include <process.h>
#endif
/* DWORD_PTR (pointer precision unsigned integer) is used for integers 
 * that are converted to handles or pointers
 * This eliminates Win64 warnings regarding conversion between
 * 32 and 64-bit data, as HANDLEs and pointers are 64 bits in 
 * Win64 (See Chapter 16). This is enable only if _Wp64 is defined.
 */
#if !defined(_Wp64)
#define DWORD_PTR DWORD
#define LONG_PTR LONG
#define INT_PTR INT
#endif
