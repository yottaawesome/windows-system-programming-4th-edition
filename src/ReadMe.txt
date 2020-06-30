Windows System Programming, Edition 4 by Johnson (John) Hart

    Version 1.18 January 4, 2016    Fix too touch (Prog 3.3) logic, update presdients.txt,
                                    inlcude several accumulated changes.

    Version 1.17 July 25, 2014      Fix to serverSK.c, some minor adjustments and comments elsewhere    

    Version 1.16 April 5, 2013      Fix to lsW.c (Chapter 3)

    Version 1.15 March 24, 2012     Fix to JobMgmt.c (Chapter 6)

    Version 1.14 January 22, 2012   Several programs, such as cci_fmm.c and cci_fmmDll.c were corrected to test CreateFileMapping return
                                    values properly. NULL, not INVALID_HANDLE_VALUE, indicates an error.

    Version 1.13 November 20, 2011  Minor changes in some programs and comments, mostly those suggested by readers.

    Version 1.12 January 24, 2011   Fixed a bug in Utility\SkipArg.c (part of Utility_4_0.dll) that is manifested
                                    in timep and JobShell when you execute from within Visual Studio and the executable
                                    pathname contains spaces.
                                    I also rebuilt all the VS 2008 project (both 32 and 64-bit)
                                    I did NOT rebuild the VS 2005 projects (VS 2005 users should rebuild all dlls and
                                    applications).
                                    VS 2010 Users: The VS 2008 projects all convert easily.

    Version 1.11 Decembber 10, 2010 Changed compiler optimation in cci* projects to "full optimization" for better performance.    
    Version 1.10 Sept 8, 2010       Improved performance in all cci* examples by removing explicit % 256 operations (not needed)
                                    This can improve performance considerably.
                                    For more information, see " Sequential File Processing: A Fast Path from Native Code to .NET
                                    with Visits to Parallelism and Memory Mapped Files" at http://www.jmhartsoftware.com/SequentialFileProcessingSupport.html
    Version 1.09 Aug 27, 2010       Added geonames project (Projects2008 and Projects2008_64 only). Source file is in CHAPTR14.
                                    See " File Searching: Parallelism and Performance PLINQ, C#/.NET, and Native Code" at http://www.jmhartsoftware.com/SequentialFileProcessingSupport.html
    Version 1.08 July 8, 2010       Additional lsW (Chapter 3) bug fix.
    Version 1.07.1 June 23, 2010    Additional compMP updates, simplifying code. 
    Version 1.07 June 14, 2010.     I added compMP, a fast file comparison utility, to Projects8_64. The source file is in CHAPTR14.
                                    See the article "Windows Parallelism, Fast File Searching, and Speculative Processing" for more
                                    information. UPDATED again June 17 to speed the program up. Updated again June 22 to improve some more.

    Version 1.06 May 21, 2010.      Bug fix in cciMTMM (multithreaded, memory mapped version, faster than other solutions)
	                                Plus other minor fixes and changes. 

    Version 1.05 April 13, 2010.    Bug fixes in tail, RecordAccessMM, RecordAccess, and randfile
                                    Converted several projects to use "safe" C runtime library functions.
                                    See: http://msdn.microsoft.com/en-us/library/8ef0s5kh%28VS.80%29.aspx and Section 3 below.

    Version 1.04 March 30, 2010.    Additional minor fixups, such as changing "atoi" to "_ttoi". In general, I've endeavored
                                    to be consistent with generic characters. Previously, there were many inconsistencies, such
                                    as not using generic functions or not using the _T macro where required. I think that most
                                    such situations are fixed, but I'll continue to be on the lookout for more and will also
                                    appreciate any that are brought to my attention.
                                    See Bullet 5 ("Notes on Generic Characters and Strings") below.
    Version 1.03 March 28, 2010.    Tuned buffer size and thread count in cci* programs (for a 4 CPU system)
									Fixed lsW to eliminate a buffer/stack overflow threat (also see comments in lsReg and lsFP)
									Other minor adjustments
    Version 1.02 March 10, 2010.    Fixed a bug in cciMT. Enhanced resource management and error reporting in several programs
    Version 1.01 February 28, 2010. Enhanced Version program (Chapter 6). Enhnaced Chapter 2 copy programs.
    Version 1.0  February 25, 2010. fixed bug in cciMT_VTP
    Beta 1.3  February 21, 2010 (principal changes: fixed projects in Debug mode, fixed wcMTMM)
    Beta 1.2. January 20, 2010  
    
ReadMe.txt for the "EXAMPLES FILE": WSP4_Examples.zip

CONTENTS
    1.  OVERVIEW
    2.  EXAMPLES FILE ORGANIZATION
    3.  ABOUT THE C RUN-TIME LIBRARY AND SECURITY & SAFETY
    4.  KNOWN SIGNIFICANT ISSUES WITH THIS BETA VERSION
    5.  NOTES ON GENERIC CHARACTERS AND STRINGS
    6.  CONTACT INFORMATION
    
1.  OVERVIEW

This directory (WSP_Examples) contains the source code for all the sample programs as well as 
the include files, utility functions, projects, and executables.

IMPORTANT: If you want to use the example code for your own applications, copy the code from
these Examples, not directly from the book. The book code is streamlined, does not test
thoroughly for errors, and may not be up-to-date in other ways.

A number of programs illustrate additional features and solve specific exercises, although the
Examples file does not include solutions for all exercises or show every alternative implementation.

All programs have been tested on Windows 7, Vista, XP, Server 2008, and Server 2003 on a wide
variety of systems, ranging from laptops to servers. Where appropriate, they have also been
tested at one time or another under Windows 9x and earlier NT versions, although many programs,
especially those from later chapters, will not run on Windows 9x or even on NT 4.0 (which is also
obsolete).

With a few minor exceptions, nearly all programs compile without warning messages under Microsoft
Visual Studio 2005 and 2008 using warning level 3. Visual Studio 2010 (Beta 2 and RC) easily converted
several projects.

Distinct project directories are provided for Microsoft Visual Studio 2005 and 2008 (32- and 64-bit).
The three project directories are:
 - Projects2005 (Does not include projects which require NT 6 [Vista, Windows 7, Server 2008])
 - Projects2008
 - Projects2008_64.
 
The projects build the executable programs into the directories:
 - run2005
 - run2008
 - run2008_64
respectively.

NOTE: While I generally keep the executable files up-to-date and consistent with the source files,
it's a good idea to rebuild projects before using these executables to be certain that you are using the 
latest version. 

There is a separate zip file with Visual Studio C++ 6.0 and 7.0 projects; some readers may find
these projects convenient, but they are not up to date.

The generic C library functions are used extensively, as are compiler-specific keywords such as
__try, __except, and __leave. The multithreaded C run-time library, _beginthreadex, and _endthreadex
are essential starting with Chapter 7.

The projects are in both release and debug form; Release is the active configuration.
The projects are all very simple, with minimal dependencies, and can also be created quickly with
the desired configuration and as either debug or release versions. The .exe and .dll files
will be built into the appropriate run directory.

The projects are defined to build all programs, with the exception of static or dynamic libraries,
as console applications.

You can also build the programs using open source development tools, such as gcc and g++ in the
Gnu Compiler Collection (http://gcc.gnu.org/). Readers interested in these tools should look at
the MinGW open source project (http://www.mingw.org), which describes MinGW as “a port of the
GNU Compiler Collection (GCC), and GNU Binutils, for use in the development of native Microsoft
Windows applications.” I have tested only a few of the programs using these tools, but I have
had considerable success using MinGW and have even been able to cross-build, constructing
Windows executable programs and DLLs on a Linux system. Furthermore, I’ve found that gcc and
g++ provide very useful 64-bit warning and error messages.


2.  EXAMPLES FILE ORGANIZATION

The primary directory is named WSP4_Examples (“Windows System Programming, Edition 4 Examples”),
and this directory can be copied directly to your hard disk at a convenient location.

 - There is a source file subdirectory for each chapter (CHAPTR01, ..., CHAPTR15).
 - All include files are in the INCLUDE directory
 - The UTILITY directory contains the common functions such as ReportError.
 - Complete projects are in the project directories.
 - Executables and DLLs for all projects are in the run directories. 

Download WindowsSmpEd3 (“Windows Sample Programs, Edition 3”) if you want to use Visual Studio 6
or Visual Studio 7.

3.  ABOUT THE C RUN-TIME LIBRARY AND SECURITY & SAFETY

Microsoft has done a nice job enhancing the C Runtime Library Security and Safety.
See: http://msdn.microsoft.com/en-us/library/8ef0s5kh%28VS.80%29.aspx
	
I have not, in general, used the enhanced functions. I've taken some care
to prevent the normal CRTL risks, but there are undoubtedly some oversights
(let me know if you find any). As appropriate, I've taken the easy way out and
defined _CRT_SECURE_NO_WARNINGS in the projects to supress warnings.
**** Verion 1.05, April 12, 2010. I've fixed numerous projects and source files and will do
     more. For a good example, see the ServerNP and ClientNP projects for examples of the changes and the
     impact on the code logic. See the use of _tfopen_s in ServerNP.c and _tcsncpy_s in
     LocSrver.c (both are in Chapter 11).

The more I think about it, the more I feel I really should update these Examples
programs to use the enhanced library. Even if you have reasons not to use the library,
its use in the examples will be a good reminder to take care.

I'll be doing this work soon and will post the updated zip file.

4.  KNOWN SIGNIFICANT ISSUES WITH THIS BETA VERSION

 - C Runtime library uasge - buffer overflow risks (see the previous section); this is fixed in many of the examples.

5.  NOTES ON GENERIC CHARACTERS AND STRINGS

Throughout the examples and the book, I've generally used generic characters (TCHAR) and
strings (LPTSTR and LPTCSTR), along with the appropriate generic functions, such as 
_tcscpy, _tcslen, etc.

However, I have not been entirely consistent or careful. Version 1.04 (March 30, 2010) fixed
many inconsistencies.

In addtion, please note:

    a.  I've left the grep (pattern search) and wc (word count) implementations (grepMP, wcMT, etc.)
        in 8-bit mode. The intent was to demonstrate the underlying threading and memory mapped file
        logic. The upgrade to generic characters and strings should be an easy task for anyone interested.
    b.  Client/Server messages use 8-bit characters. This is important in the client and server programs,
        even though the programs themselves use generic characters. The same is true of the "commandIP" DLL.
    c.  Some of the X programs (that is, intentionally buggy) are not consistent.
    d.  Some experimental programs do not properly use generic characters and strings.
        
**  DO WE REALLY NEED GENERIC STRINGS AND CHARACTERS?
	Answer: It depends. Several people have suggested that I could just use UNICODE throughout as is
	generally done in new programs. I tend to agree, and it was a difficult decision as to what to do
	in Edition 4. I finally decided to stick with the current policy, even though it makes the code
	messy (all those _T macros and _t functions are annoying, IMHO). Nonetheless, I still see a lot
	of legacy code in this form, and I also see a lot of legacy code being ported from UNIX/Linux or which
	must be capable of interoperability between plaforms and must be source code compatible (as in Appendix B).
	       

6.  CONTACT INFORMATION

Reader feedback is welcome and appreciated. Contact me at:

 - jmhart62@gmail.com or jmh_assoc@hotmail.com

I'll respond to all messages as quickly as possible. Even if I'm busy, I'll send
a quick response to acknowledge your message.

Significant bug fixes, contributions, etc. will be acknowledged on the book's support site:
	www.jmhartsoftware.com 
	
Good luck, and best wishes. I've enjoyed writing Edition 4, and, over the years, I've enjoyed hearing from
readers of the previous editions. I'll look forward to hearing from you in the future.

John
jmhart62@gmail.com or jmh_assoc@hotmail.com
