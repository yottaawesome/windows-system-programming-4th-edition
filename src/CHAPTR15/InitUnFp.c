/* PROGRAM 15-3 STARTS HERE AND ENDS WHERE NOTED BELOW. */

/* InitUnFp.c */

/* These functions maintain UNIX-style permissions
		in a Win32 (Windows NT only) SECURITY_ATTRIBUTES structure.
	There are three entries:
		InitializeUnixSA: Allocate & initialize structure.
		ChangeFilePermissions: Modify file's security.
		ReadFilePermissions: Interrogate a file's security.

	There are a number of assumptions and limitations:
		You can only modify and read a structure previously
			created by InitializeUnixSA.
		All permissions are the UNIX style:
			[R,W,X] -[User, Group, Other].
		The group is taken from the Group SID of the process.
			This may really represent several groups.
			You can, however, apply generic rights to ANY securable object.
		The change and read entries require the HANDLE of the
			object and obtain its SA. */

#include "Everything.h"

#define ACL_SIZE 1024
#define SID_SIZE SECURITY_MAX_SID_SIZE
#define DOM_SIZE LUSIZE

static VOID FindGroup (DWORD, LPTSTR, DWORD);

LPSECURITY_ATTRIBUTES InitializeUnixSA (DWORD unixPerms,
		LPTSTR usrName, LPTSTR grpName, LPDWORD allowedAceMasks, 
		LPDWORD deniedAceMasks, LPHANDLE pSaHeap)

/* Allocate a structure and set the UNIX style permissions
	as specified in unixPerms, which is 9-bits
	(low-order end) giving the required[R,W,X] settings
	for[User,Group,Other] in the familiar UNIX form. 
	Return a pointer to a security attributes structure. */

{
	/*  Several memory allocations are necessary to build the SA.
		The structures are:
			1.	The SA itself
			2.	The SD
			3.	Three SIDs (for user, group, and everyone)
			4.  The group name if it is not supplied as the "grpName" parameter
		This memory MUST be available to the calling program and cannot
		be allocated on the stack of this function.	*/

	LPSECURITY_ATTRIBUTES pSA = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	PACL pAcl = NULL;
	BOOL success, ok = TRUE;
	DWORD iBit, iSid;
	/* Various tables of User, Group, and Everyone Names, SIDs,
		and so on for use first in LookupAccountName and SID creation. */
	LPTSTR groupNames[3] = {EMPTY, EMPTY, _T ("Everyone")};
	PSID pSidTable[3] = {NULL, NULL, NULL};
	SID_NAME_USE sNamUse[] = 
		{SidTypeUser, SidTypeGroup, SidTypeWellKnownGroup};
	TCHAR refDomain[3][DOM_SIZE];
	DWORD refDomainCount[3] = {DOM_SIZE, DOM_SIZE, DOM_SIZE};
	DWORD sidCount[3] = {SID_SIZE, SID_SIZE, SID_SIZE};

	*pSaHeap = HeapCreate (HEAP_GENERATE_EXCEPTIONS, 0, 0);
	__try {
	/* This is in a try-except block so as to
		free resources in case of any subsequent failure. */
	/*  The security structures must be allocated from the
		stack as it is used outside the function. */
	pSA = HeapAlloc (*pSaHeap, 0, sizeof(SECURITY_ATTRIBUTES));
	pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
	pSA->bInheritHandle = FALSE; /* Programmer can set this later. */
	
	pSD = HeapAlloc (*pSaHeap, 0, sizeof(SECURITY_DESCRIPTOR));
	pSA->lpSecurityDescriptor = pSD;
	if (!InitializeSecurityDescriptor (pSD, SECURITY_DESCRIPTOR_REVISION))
		ReportException (_T ("I.S.D. Error"), 21);

	/* Set up the table names for the user and group.
		Then get a SID for User, Group, and Everyone. */
	groupNames[0] = usrName;
	if (grpName == NULL || _tcslen(grpName) == 0) { 
		/*  No group name specified. Get the user's primary group. */
		/*  Allocate a buffer for the group name */
		groupNames[1] = HeapAlloc (*pSaHeap, 0, ACCT_NAME_SIZE);
		FindGroup (2, groupNames[1], ACCT_NAME_SIZE);
	} else groupNames[1] = grpName;

	/* Look up the three names, creating the SIDs. */
	for (iSid = 0; iSid < 3; iSid++) {
		pSidTable[iSid] = HeapAlloc (*pSaHeap, 0, SID_SIZE);
		ok = ok && LookupAccountName (NULL, groupNames[iSid],
				pSidTable[iSid], &sidCount[iSid],
				refDomain[iSid], &refDomainCount[iSid], &sNamUse[iSid]);
	}
	if (!ok)
		ReportException (_T("LookupAccntName Error"), 22);

	/* Set the security descriptor owner & group SIDs. */

	if (!SetSecurityDescriptorOwner (pSD, pSidTable[0], FALSE))
		ReportException (_T ("S.S.D.O. Error"), 23);
	if (!SetSecurityDescriptorGroup (pSD, pSidTable[1], FALSE))
		ReportException (_T ("S.S.D.G. Error"), 24);

	/* Allocate a structure for the ACL. */
	pAcl = HeapAlloc (*pSaHeap, 0, ACL_SIZE);

	/* Initialize an ACL. */
	if (!InitializeAcl (pAcl, ACL_SIZE, ACL_REVISION))
		ReportException (_T ("Initialize ACL Error"), 25);

	/* Add all the ACEs. Scan the permission bits, adding an allowed ACE when
		the bit is set and a denied ACE when the bit is reset. */

	success = TRUE;
	for (iBit = 0; iBit < 9; iBit++) {
		if ((unixPerms >> (8 - iBit) & 0x1) != 0 && allowedAceMasks[iBit % 3] != 0)
			success = success && AddAccessAllowedAce (pAcl, ACL_REVISION,
					allowedAceMasks[iBit % 3], pSidTable[iBit / 3]);
		else if (deniedAceMasks[iBit % 3] != 0)
			success = success && AddAccessDeniedAce (pAcl, ACL_REVISION,
					deniedAceMasks[iBit % 3], pSidTable[iBit / 3]);
	}

	if (!success)
		ReportException (_T ("AddAce error"), 26);
	if (!IsValidAcl (pAcl))
		ReportException (_T ("Created bad ACL"), 27);

	/* The ACL is now complete. Associate it with the security descriptor. */

	if (!SetSecurityDescriptorDacl (pSD, TRUE, pAcl, FALSE))
		ReportException (_T ("S.S.D.Dacl error"), 28);
	if (!IsValidSecurityDescriptor (pSD))
		ReportException (_T ("Created bad SD"), 29);
}	/* End of __try-except block. */

__except ((GetExceptionCode() != STATUS_NO_MEMORY) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
{
	/*  An exception occurred and was reported. All memory allocated to
		create the security descriptor and attributes is freed. */
	if (pSaHeap != NULL)
		HeapDestroy (pSaHeap);
	pSA = NULL;
}
	return pSA;
}

void DestroyUnixSA (LPSECURITY_ATTRIBUTES pSa, LPHANDLE pHeap)
{
	/* Deallocate a SA structure, which contains absolute SDs */
	if (pHeap != NULL) HeapDestroy (pHeap);
}

LPSECURITY_ATTRIBUTES InitializeAccessOnlySA (DWORD unixPerms,
		LPTSTR usrName, LPTSTR grpName, LPDWORD aceMasks, LPHANDLE pHeap)
/* To be implemented */
{
	return NULL;
}


/* PROGRAM 7-3 ENDS HERE. */


/* PROGRAM 7-4 STARTS HERE AND ENDS WHERE NOTED BELOW. */

DWORD ReadFilePermissions (LPTSTR lpfileName, LPTSTR userName, LPTSTR groupName)


/* Return the UNIX style permissions for a file. */
{
	PSECURITY_DESCRIPTOR pSD = NULL;
	DWORD lenNeeded, permissionBits, iAce;
	BOOL fileDacl, aclDefaulted, ownerDaclDefaulted, groupDaclDefaulted;
	DWORD dacl[ACL_SIZE/sizeof(DWORD)];	/* ACLS need DWORD alignment */
	PACL pAcl = (PACL) &dacl;
	ACL_SIZE_INFORMATION aclSizeInfo;
	PACCESS_ALLOWED_ACE pAce;
	BYTE aclType;
	PSID pOwnerSid, pGroupSid;
	TCHAR refDomain[2][DOM_SIZE];
	DWORD refDomainCount[2] = {DOM_SIZE, DOM_SIZE};
	DWORD accountNameSize[2] = {ACCT_NAME_SIZE, ACCT_NAME_SIZE};
	SID_NAME_USE sNamUse[] = {SidTypeUser, SidTypeGroup};

	/* Get the required size for the security descriptor. */
	GetFileSecurity (lpfileName, OWNER_SECURITY_INFORMATION |
			GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
			NULL, 0, &lenNeeded);

	/* Create a security descriptor. */
	pSD = malloc (lenNeeded);
	if ((pSD != NULL) && !GetFileSecurity (lpfileName, OWNER_SECURITY_INFORMATION |
			GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
			pSD, lenNeeded, &lenNeeded))
		ReportError (_T ("GetFileSecurity error"), 30, TRUE);
	
	if (!GetSecurityDescriptorDacl (pSD, &fileDacl, &pAcl, &aclDefaulted))
		ReportError (_T ("GetSecurityDescriptorDacl error"), 31, TRUE);

	/* Get the number of ACEs in the ACL. */

	if (!GetAclInformation (pAcl, &aclSizeInfo,
			sizeof (ACL_SIZE_INFORMATION), AclSizeInformation))
		ReportError (_T ("GetAclInformation error"), 32, TRUE);


	/* Get Each Ace. We know that this ACL was created by
		our InitializeUnixSA function, so the ACES are in the
		same order as the UNIX permission bits. */

	permissionBits = 0;
	for (iAce = 0; iAce < aclSizeInfo.AceCount; iAce++) {
		GetAce (pAcl, iAce, &pAce);
		aclType = pAce->Header.AceType;
		if (aclType == ACCESS_ALLOWED_ACE_TYPE)
			permissionBits |= (0x1 << (8-iAce));
	}

	/* Find the name of the owner and owning group. */
	/* Find the SIDs first. */

	if (!GetSecurityDescriptorOwner (pSD, &pOwnerSid, &ownerDaclDefaulted))
		ReportError (_T ("GetSecurityDescOwner error"), 33, TRUE);
	if (!GetSecurityDescriptorGroup (pSD, &pGroupSid, &groupDaclDefaulted))
		ReportError (_T ("GetSecurityDescGrp error"), 34, TRUE);
	if (!LookupAccountSid (NULL, pOwnerSid, userName, &accountNameSize[0], refDomain[0],
			&refDomainCount[0], &sNamUse[0]))
		ReportError (_T ("LookUpAccountSid error"), 35, TRUE);
	if (!LookupAccountSid (NULL, pGroupSid, groupName, &accountNameSize[1], refDomain[1],
			&refDomainCount[1], &sNamUse[1]))
		ReportError (_T ("LookUpAccountSid error"), 36, TRUE);
	free (pSD);	
	return permissionBits;
}

/* PROGRAM 7-4 ENDS HERE.  */


/* PROGRAM 7-5 STARTS HERE AND ENDS WHERE NOTED BELOW */

BOOL ChangeFilePermissions (DWORD fPerm, LPTSTR fileName, 
							LPDWORD allowedAceMasks, LPDWORD deniedAceMasks)

/* Change permissions in an existing file. The group is left unchanged. */

/* Strategy:
	1.	Obtain the existing security descriptor using
		the internal function ReadFilePermissions.
	2.	Create a security attribute for the owner and permission bits.
	3.	Extract the security descriptor.
	4.	Set the file security with the new descriptor. */
{
	TCHAR userName[ACCT_NAME_SIZE], groupName[ACCT_NAME_SIZE];
	LPSECURITY_ATTRIBUTES pSA;
	PSECURITY_DESCRIPTOR pSD = NULL;
	HANDLE heap;

	/* Assure that the file exists.	*/
	if (_taccess (fileName, 0) != 0)	/* File does not exist. */
		return FALSE;
	ReadFilePermissions (fileName, userName, groupName); /* Return value not required */
	pSA = InitializeUnixSA (fPerm, userName, groupName, allowedAceMasks, deniedAceMasks, &heap);
	pSD = pSA->lpSecurityDescriptor;

	if (!SetFileSecurity (fileName, DACL_SECURITY_INFORMATION, pSD))
		ReportError (_T ("SetFileSecurity error"), 37, TRUE);

	DestroyUnixSA (pSA, heap);
	return TRUE;
}

/* PROGRAM 13-5 ENDS HERE  */


/* This code is a hint as to how to solve Exercise 8-2. */

static VOID FindGroup (DWORD GroupNumber, LPTSTR GroupName, DWORD cGroupName)
/*	Find a group name associated with the owning user
	of the current process. */

{
	TCHAR refDomain[DOM_SIZE];
	DWORD refDomainCount = DOM_SIZE;
	SID_NAME_USE GroupSidType  = SidTypeGroup;
	HANDLE tHandle;
	TOKEN_GROUPS TokenG[20]; /* You need some space for this. */
	DWORD TISize, accountNameSize = cGroupName;

	if (!OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &tHandle))
		ReportError (_T ("OpenProcessToken error"), 0, TRUE);
	if (!GetTokenInformation (tHandle, TokenGroups,
			&TokenG, sizeof (TokenG), &TISize)) {
		ReportError (_T ("GetTokenInfo error"), 0, TRUE);
	}

	/* Experimentation shows that the groups entered are
		as follows:
		0	 -	None
		1	 -	Everyone
		2	 -	The first non-trivial group
	 	3,.. -	Keep looking up to the count, which is part
				of the structure - see the documentation! */

	if (!LookupAccountSid (NULL, TokenG[0].Groups[GroupNumber].Sid,
			GroupName, &accountNameSize, refDomain, &refDomainCount, &GroupSidType))
			ReportError (_T("Error looking up Account Name"), 0, TRUE);
	return;
}
