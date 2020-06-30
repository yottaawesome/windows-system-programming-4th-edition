/* Chapter 6. version. Display system version information  */
#include "Everything.h"

int _tmain (int argc, LPTSTR argv[])
{
	OSVERSIONINFO OSVer;
	SYSTEM_INFO SysInfo;
	DWORD SysAffinMask = 0, ProcAffinMask = 0;
	DWORD sectorsPerCluster, bytesPerSector, numberOfFreeClusters, totalNumberOfClusters;
/* For refereence -- this is what the OSVERSIONINFO structure looks like.
typedef struct _OSVERSIONINFO{ 
    DWORD dwOSVersionInfoSize; 
    DWORD dwMajorVersion; 
    DWORD dwMinorVersion; 
    DWORD dwBuildNumber; 
    DWORD dwPlatformId; 
    TCHAR szCSDVersion[ 128 ]; 
} OSVERSIONINFO; 
*/ 

	/* Determine version information.  */
	OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx (&OSVer)) 
		_tprintf (_T("Cannot get OS Version info. %d"), GetLastError());

	_tprintf (_T("Major Version:      %d\n"), OSVer.dwMajorVersion);
	_tprintf (_T("Minor Version:      %d\n"), OSVer.dwMinorVersion);
	_tprintf (_T("Build Number:       %d\n"), OSVer.dwBuildNumber);
	_tprintf (_T("Platform ID:        %d\n"), OSVer.dwPlatformId);
	_tprintf (_T("Platorm is NT?(0/1) %d\n"), (OSVer.dwPlatformId == VER_PLATFORM_WIN32_NT));
	_tprintf (_T("Service Pack:       %s\n"), OSVer.szCSDVersion);
	_tprintf (_T("\n"));

/* For reference -- Here is the SYSTEM_INFO structure.
typedef struct _SYSTEM_INFO { // sinf 
    union { 
        DWORD  dwOemId; 
        struct { 
            WORD wProcessorArchitecture; 
            WORD wReserved; 
        }; 
    }; 
    DWORD  dwPageSize; 
    LPVOID lpMinimumApplicationAddress; 
    LPVOID lpMaximumApplicationAddress; 
    DWORD  dwActiveProcessorMask; 
    DWORD  dwNumberOfProcessors; 
    DWORD  dwProcessorType; 
    DWORD  dwAllocationGranularity; 
    WORD  wProcessorLevel; 
    WORD  wProcessorRevision; 
} SYSTEM_INFO; 
*/
	GetSystemInfo (&SysInfo);
	_tprintf (_T("OEM Id:             %d\n"), SysInfo.dwOemId);
	_tprintf (_T("Processor Arch:     %d\n"), SysInfo.wProcessorArchitecture);
	_tprintf (_T("Page Size:          %x\n"), SysInfo.dwPageSize);
	_tprintf (_T("Min appl addr:      %p\n"), SysInfo.lpMinimumApplicationAddress);
	_tprintf (_T("Max appl addr:      %p\n"), SysInfo.lpMaximumApplicationAddress);
	_tprintf (_T("ActiveProcMask:     %x\n"), SysInfo.dwActiveProcessorMask);
	_tprintf (_T("Number processors:  %d\n"), SysInfo.dwNumberOfProcessors);
	_tprintf (_T("Processor type:     %d\n"), SysInfo.dwProcessorType);
	_tprintf (_T("Alloc grnrty:       %x\n"), SysInfo.dwAllocationGranularity);
	_tprintf (_T("Processor level:    %d\n"), SysInfo.wProcessorLevel);
	_tprintf (_T("Processor rev:      %d\n"), SysInfo.wProcessorRevision);
	_tprintf (_T("OEM Id:             %d\n"), SysInfo.dwOemId);

	_tprintf (_T("\n"));
	GetProcessAffinityMask (GetCurrentProcess(), (PDWORD_PTR)&ProcAffinMask, (PDWORD_PTR)&SysAffinMask);
	_tprintf (_T("Sys  Affinity Mask  %x\n"), SysAffinMask);
	_tprintf (_T("Proc Affinity Mask  %x\n"), ProcAffinMask);

	/* Now get Disk information (for the current disk) */
	/* Caution: the MSND page suggests using GetDiskFreeSpaceEx for volumes larger than 2GB, which would be nearly any */
	/* hard disk volume. However, the Ex function does not return the same cluster and sector information. */
	/* Regardless, this seems to work properly under Vista and W7 on a large disk (640 GB was the largest I tried). */
	GetDiskFreeSpace(NULL, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);
	_tprintf (_T("\n"));
	_tprintf (_T("Information about current disk.\n"));
	_tprintf (_T("Sectors per cluster:      %d.\n"), sectorsPerCluster);
	_tprintf (_T("Bytes per sector:         %d.\n"), bytesPerSector);
	_tprintf (_T("Number of free clusters:  %d.\n"), numberOfFreeClusters);
	_tprintf (_T("Total number of clusters: %d.\n"), totalNumberOfClusters);

	return 0;
}
