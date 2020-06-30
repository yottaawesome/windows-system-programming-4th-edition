/* environment.h  define UNICODE and _STATICLIB, if required */
#if (WIN32_WINNT >= 0x0600)
#define WIN32_WINNT 0x0600   /* Enable use of NT 5 (XP, 2000, 2003) functions */
#else
#if (WIN32_WINNT >= 0x0500)
#define WIN32_WINNT 0x0500   /* Enable use of NT 5 (XP, 2000, 2003) functions */
#endif
#endif

//#define UNICODE

/* It is best and easiest to define UNICODE within the project. */
/* Use Project...Settings...C/C++. Then, down in the "Project Options */
/* window on the bottom, add   /D "UNICODE"     */
/* The same is true for DLLLIB */
//#define UNICODE
//#undef UNICODE

#ifdef UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#undef _UNICODE
#endif
#define LANG_DFLT LANG_ENGLISH
#define SUBLANG_DFLT SUBLANG_ENGLISH_US

/* Define _MT when running in a threaded environment
Better Alternative: Set from the
	Project...Settings...C/C++...CodeGeneration category  . */
//#define _MT

/* Define UTILITY_EXPORTS only if you build a DLL. */
/* It is best and easiest to define _DLLLIB within the project. */
/* Use Project...Settings...C/C++. Then, down in the "Project Options */
/* window on the bottom, add   /D "UTILITY_EXPORTS"     */
//#undef UTILITY_EXPORTS
//#define UTILITY_EXPORTS
/* Define _STATICLIB if you are building a static library
	or if you are linking with static libaries. 
	Undefine if you are builing a DLL or linking with a DLL. */
/* Again, it is best and easiest to define this in the project */
//#define _STATICLIB
//#undef _STATICLIB

//#define STRICT
