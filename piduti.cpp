
#pragma hdrstop
/******************************************************************************\
*	   This is a part of the Microsoft Source Code Samples.
*	   Copyright (C) 1994-1996 Microsoft Corporation.
*	   All rights reserved.
*	   This source code is only intended as a supplement to
*	   Microsoft Development Tools and/or WinHelp documentation.
*	   See these sources for detailed information regarding the
*	   Microsoft samples programs.
\******************************************************************************/


/*++

Module Name:

    piduti.c

Abstract:

    This module contains common apis used by tlist & kill.

--*/

#define KEYGEN
#include <windows.h>
#include <winperf.h>   // for Windows NT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>  // for Windows 95
//#include <classes.hpp>



#include "piduti.h"


#if defined(KEYGEN) ||defined(TOOLS_BUILD)
#define STACKINFO_BEGIN
#define STACKINFO_END
#define TOTDebugInitFunc() 1
#define STACKINFO_BEGIN_EX
#define STACKINFO_END_EX
#define TOTDebugError(a) 1
#define TOTDebug(a) 1

#else
#include "debug.h"

#endif



int fGetLastError(AnsiString &aErrorMsg)
{
TOTDebugInitFunc();

			int lLastError=GetLastError();
			LPVOID lpMsgBuf;

			FormatMessage(
										FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
										NULL,
										lLastError,
										MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
										(LPTSTR) &lpMsgBuf,
										0,
										NULL
										);

			// Display the string.
			//Application->MessageBox((char*)lpMsgBuf,MESSAGE_BOX_TITLE, MB_OK|MB_ICONWARNING );
      aErrorMsg=(char *)lpMsgBuf ;


			// Free the buffer.
			LocalFree( lpMsgBuf );
      return lLastError;
 }

#include <except.h>

        LPGetTaskList     GetTaskList;
        LPEnableDebugPriv EnableDebugPriv;
        OSVERSIONINFO     mVerInfo ;



//
// manafest constants
//
#define INITIAL_SIZE	   51200
#define EXTEND_SIZE 	   25600
#define REGKEY_PERF 	   "software\\microsoft\\windows nt\\currentversion\\perflib"
#define REGSUBKEY_COUNTERS  "Counters"
#define PROCESS_COUNTER	   "process"
#define PROCESSID_COUNTER   "id process"
#define UNKNOWN_TASK	   "unknown"

#ifndef max

   int max(int value1, int value2)
   {
      return ( (value1 > value2) ? value1 : value2);
   }
   int min(int value1, int value2)
   {
      return ( (value1 < value2) ? value1 : value2);
   }

#endif

//
// Function pointer types for accessing Toolhelp32 functions dynamically.
// By dynamically accessing these functions, we can do so only on Windows
// 95 and still run on Windows NT, which does not have these functions.
//
typedef BOOL (WINAPI *PROCESSWALK)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef HANDLE (WINAPI *CREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);


//
// prototypes
//
BOOL CALLBACK
EnumWindowsProc(
    HWND	  hwnd,
    DWORD   lParam
    );



DWORD
GetTaskList95(
    TList *apListe,
    DWORD 	 dwNumTasks
    )

/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.	This function uses Toolhelp32to get the task list and is
    therefore straight WIN32 calls that anyone can call.

Arguments:

    dwNumTasks 	 - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

{
TOTDebugInitFunc();

   CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
   PROCESSWALK    pProcess32First		    = NULL;
   PROCESSWALK    pProcess32Next		    = NULL;

   HANDLE 	   hKernel	   = NULL;
   HANDLE 	   hProcessSnap   = NULL;
   PROCESSENTRY32 pe32		   = {0};
   DWORD		   dwTaskCount    = 0;
   TASK_LIST *pTask=NULL;

   // Guarantee to the code later on that we'll enum at least one task.
   if (dwNumTasks == 0)
	 return 0;

    // Obtain a module handle to KERNEL so that we can get the addresses of
    // the 32-bit Toolhelp functions we need.
    hKernel = GetModuleHandle("KERNEL32.DLL");

    if (hKernel)
    {
	   pCreateToolhelp32Snapshot =
		(CREATESNAPSHOT)GetProcAddress(
			(HMODULE)hKernel,
			"CreateToolhelp32Snapshot");

		 pProcess32First = (PROCESSWALK)GetProcAddress((HMODULE)hKernel,
												"Process32First");
	   pProcess32Next  = (PROCESSWALK)GetProcAddress((HMODULE)hKernel,
										    "Process32Next");
    }

    // make sure we got addresses of all needed Toolhelp functions.
    if (!(pProcess32First && pProcess32Next && pCreateToolhelp32Snapshot))
	  return 0;


    // Take a snapshot of all processes currently in the system.
    hProcessSnap = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == (HANDLE)-1)
	   return 0;

    // Walk the snapshot of processes and for each process, get information
    // to display.
    dwTaskCount = 0;
    pe32.dwSize = sizeof(PROCESSENTRY32);   // must be filled out before use
    if (pProcess32First(hProcessSnap, &pe32))
    {
	   do
	   {
		  LPSTR pCurChar;

		  // strip path and leave executabe filename splitpath
		  for (pCurChar = (pe32.szExeFile + lstrlen (pe32.szExeFile));
			  *pCurChar != '\\' && pCurChar != pe32.szExeFile;
			  --pCurChar)
      pTask = new TASK_LIST;
      apListe->Add((void *) pTask); 
		  lstrcpy(pTask -> ProcessName, pCurChar);
		  pTask -> flags = 0;
		  pTask -> WindowCount = 0;
		  pTask -> dwProcessId = pe32.th32ProcessID;
      TOTDebug(("Task95 %d=%s[%d]",dwTaskCount,pCurChar,pTask -> dwProcessId));

		  ++dwTaskCount;   // keep track of how many tasks we've got so far
		  //++pTask;	    // get to next task info block.
	   }
	   while (dwTaskCount < dwNumTasks && pProcess32Next(hProcessSnap, &pe32));
    }
    else
	   dwTaskCount = 0;    // Couldn't walk the list of processes.

    // Don't forget to clean up the snapshot object...
    CloseHandle (hProcessSnap);

    return dwTaskCount;
}


DWORD
GetTaskListNT(
    TList *apListe,
    DWORD 	 dwNumTasks
    )

/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.	This function uses the registry performance data to get the
    task list and is therefore straight WIN32 calls that anyone can call.

Arguments:

    dwNumTasks 	 - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

{
    DWORD 				   rc;
    HKEY					   hKeyNames;
    DWORD 				   dwType;
    DWORD 				   dwSize;
    LPBYTE				   buf = NULL;
    CHAR					   szSubKey[1024];
    LANGID				   lid;
    LPSTR 				   p;
    LPSTR 				   p2;
    PPERF_DATA_BLOCK		   pPerf;
    PPERF_OBJECT_TYPE		   pObj;
    PPERF_INSTANCE_DEFINITION    pInst;
    PPERF_COUNTER_BLOCK		   pCounter;
    PPERF_COUNTER_DEFINITION	   pCounterDef;
    DWORD 				   i;
    DWORD 				   dwProcessIdTitle;
    DWORD 				   dwProcessIdCounter;
    CHAR					   szProcessName[MAX_PATH];
    DWORD 				   dwLimit = dwNumTasks - 1;
   TASK_LIST *pTask=NULL;



    //
    // Look for the list of counters.  Always use the neutral
    // English version, regardless of the local language.  We
    // are looking for some particular keys, and we are always
    // going to do our looking in English.  We are not going
    // to show the user the counter names, so there is no need
    // to go find the corresponding name in the local language.
    //
    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    snprintf( szSubKey,1024, "%s\\%03x", REGKEY_PERF, lid );
    //JC 21/07/2003 : Voir la doc du snprintf !!
    szSubKey[1023]=0;
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				   szSubKey,
				   0,
				   KEY_READ,
				   &hKeyNames
				 );
    if (rc != ERROR_SUCCESS) {
	   goto exit;
    }

    //
    // get the buffer size for the counter names
    //
    rc = RegQueryValueEx( hKeyNames,
					 REGSUBKEY_COUNTERS,
					 NULL,
					 &dwType,
					 NULL,
					 &dwSize
				    );

    if (rc != ERROR_SUCCESS) {
	   goto exit;
    }

    //
    // allocate the counter names buffer
    //
    buf = (LPBYTE) malloc( dwSize );
    if (buf == NULL) {
	   goto exit;
    }
    memset( buf, 0, dwSize );

    //
    // read the counter names from the registry
    //
    rc = RegQueryValueEx( hKeyNames,
					 REGSUBKEY_COUNTERS,
					 NULL,
					 &dwType,
					 buf,
					 &dwSize
				    );

    if (rc != ERROR_SUCCESS) {
	   goto exit;
    }

    //
    // now loop thru the counter names looking for the following counters:
    //
    //	  1.  "Process"           process name
    //	  2.  "ID Process"        process id
    //
    // the buffer contains multiple null terminated strings and then
    // finally null terminated at the end.  the strings are in pairs of
    // counter number and counter name.
    //

    p = (LPSTR)buf;
    while (*p) {
	   if (p > (LPSTR)buf) {
		  for( p2=p-2; isdigit(*p2); p2--) ;
		  }
	   if (stricmp(p, PROCESS_COUNTER) == 0) {
		  //
		  // look backwards for the counter number
		  //
		  for( p2=p-2; isdigit(*p2); p2--) ;
		  strcpy( szSubKey, p2+1 );
	   }
	   else
	   if (stricmp(p, PROCESSID_COUNTER) == 0) {
		  //
		  // look backwards for the counter number
		  //
		  for( p2=p-2; isdigit(*p2); p2--) ;
		  dwProcessIdTitle = atol( p2+1 );
	   }
	   //
	   // next string
	   //
	   p += (strlen(p) + 1);
    }

    //
    // free the counter names buffer
    //
    free( buf );


    //
    // allocate the initial buffer for the performance data
    //
    dwSize = INITIAL_SIZE;
    buf = (unsigned char *)malloc( dwSize );
    if (buf == NULL) {
	   goto exit;
    }
    memset( buf, 0, dwSize );


    while (TRUE) {

	   rc = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
						szSubKey,
						NULL,
						&dwType,
						buf,
						&dwSize
					   );

	   pPerf = (PPERF_DATA_BLOCK) buf;

	   //
	   // check for success and valid perf data block signature
	   //
	   if ((rc == ERROR_SUCCESS) &&
		  (dwSize > 0) &&
		  (pPerf)->Signature[0] == (WCHAR)'P' &&
		  (pPerf)->Signature[1] == (WCHAR)'E' &&
		  (pPerf)->Signature[2] == (WCHAR)'R' &&
		  (pPerf)->Signature[3] == (WCHAR)'F' ) {
		  break;
	   }

	   //
	   // if buffer is not big enough, reallocate and try again
	   //
	   if (rc == ERROR_MORE_DATA) {
		  dwSize += EXTEND_SIZE;
		  buf = (unsigned char *)realloc( buf, dwSize );
		  memset( buf, 0, dwSize );
	   }
	   else {
		  goto exit;
	   }
    }

    //
    // set the perf_object_type pointer
    //
    pObj = (PPERF_OBJECT_TYPE) ((DWORD)pPerf + pPerf->HeaderLength);

    //
    // loop thru the performance counter definition records looking
    // for the process id counter and then save its offset
    //
    pCounterDef = (PPERF_COUNTER_DEFINITION) ((DWORD)pObj + pObj->HeaderLength);
    for (i=0; i<(DWORD)pObj->NumCounters; i++) {
	   if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) {
		  dwProcessIdCounter = pCounterDef->CounterOffset;
		  break;
	   }
	   pCounterDef++;
    }

    dwNumTasks = min( dwLimit, (DWORD)pObj->NumInstances );

    pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD)pObj + pObj->DefinitionLength);

    //
    // loop thru the performance instance data extracting each process name
    // and process id
    //
    for (i=0; i<dwNumTasks; i++) {
	   //
	   // pointer to the process name
	   //
	   p = (LPSTR) ((DWORD)pInst + pInst->NameOffset);

	   //
	   // convert it to ascii
	   //
	   rc = WideCharToMultiByte( CP_ACP,
						    0,
						    (LPCWSTR)p,
						    -1,
						    szProcessName,
						    sizeof(szProcessName),
						    NULL,
						    NULL
						  );

     pTask = new TASK_LIST;
     apListe->Add((void *) pTask);

	   if (!rc) {
		  //
	    // if we cant convert the string then use a default value
		  //
		  strcpy( pTask->ProcessName, UNKNOWN_TASK );
	   }

	   if (strlen(szProcessName)+4 <= sizeof(pTask->ProcessName)) {
		  strcpy( pTask->ProcessName, szProcessName );
		  strcat( pTask->ProcessName, ".exe" );
	   }

	   //
	   // get the process id
	   //
	   pCounter = (PPERF_COUNTER_BLOCK) ((DWORD)pInst + pInst->ByteLength);
	   pTask->flags = 0;
     pTask -> WindowCount = 0;

	   pTask->dwProcessId = *((LPDWORD) ((DWORD)pCounter + dwProcessIdCounter));
	   if (pTask->dwProcessId == 0) {
		  pTask->dwProcessId = (DWORD)-2;
	   }

	   //
	   // next process
	   //
	   //pTask++;
	   pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD)pCounter + pCounter->ByteLength);
    }

exit:
    if (buf) {
	   free( buf );
    }

    RegCloseKey( hKeyNames );
    RegCloseKey( HKEY_PERFORMANCE_DATA );

    return dwNumTasks;
}



BOOL
EnableDebugPriv95(
     char * aPriv
    )

/*++

Routine Description:

    Changes the process's privilege so that kill works properly.

Arguments:


Return Value:

    TRUE			 - success
    FALSE 		 - failure

Comments:
    Always returns TRUE

--*/

{
   return TRUE;
}



BOOL
EnableDebugPrivNT(
    char * aPriv
    )

/*++

Routine Description:

    Changes the process's privilege so that kill works properly.

Arguments:


Return Value:

    TRUE			 - success
    FALSE 		 - failure

--*/

{
TOTDebugInitFunc();

    HANDLE hToken;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;


    //
    // Retrieve a handle of the access token
    //
    if (!OpenProcessToken(GetCurrentProcess(),
		  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		  &hToken))
    {
      AnsiString lMsg;
      auto lErrNum=fGetLastError(lMsg);
	    printf("OpenProcessToken failed with %d:%s\n", (int)lErrNum,lMsg.c_str());
	   return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege
    //
    if (!LookupPrivilegeValue((LPSTR) NULL,
		  aPriv,
		  &DebugValue)) {
	   printf("LookupPrivilegeValue failed with %d\n", GetLastError());
	   return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
	   FALSE,
	   &tkp,
	   sizeof(TOKEN_PRIVILEGES),
	   (PTOKEN_PRIVILEGES) NULL,
	   (PDWORD) NULL);

    //
    // The return value of AdjustTokenPrivileges can't be tested
    //
    if (GetLastError() != ERROR_SUCCESS) {
	   printf("AdjustTokenPrivileges failed with %d\n", GetLastError());
	   return FALSE;
    }

    return TRUE;
}

BOOL
TOTProcessInformation::KillProcess(
    PTASK_LIST tlist,
    BOOL		fForce

    )
{
    HANDLE		  hProcess;


    if (fForce || !tlist->hwnd) {
	   hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, tlist->dwProcessId );
	   if (hProcess) {
		  hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, tlist->dwProcessId );
		  if (hProcess == NULL) {
			 return FALSE;
		  }

		  if (!TerminateProcess( hProcess, 1 )) {
			 CloseHandle( hProcess );
			 return FALSE;
		  }

		  CloseHandle( hProcess );
		  return TRUE;
	   }
    }

    //
    // kill the process
    //
    for(int li=0;li<mpWindowList->Count;li++)
    {
      WINDOW_LIST *lpTask=(WINDOW_LIST *) mpWindowList->Items[li];
      int lPid= lpTask->dwProcessId;
      if(lPid==tlist->dwProcessId)
        PostMessage( (HWND)lpTask->hwnd, WM_CLOSE, 0, 0 );
    }
    return TRUE;
}


int TOTProcessInformation::GetWindowTitles()
{
    //
    // enumerate all windows
    //
    EnumWindows( (WNDENUMPROC )EnumWindowsProc, (LPARAM) this );
  return 1;
}


void TOTProcessInformation::AddWindow(PWINDOW_LIST aWindow)
{
  mpWindowList->Add((void *) aWindow);
}

BOOL CALLBACK
EnumWindowsProc(
    HWND	  hwnd,
    DWORD   lParam
    )

/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    hwnd			 - window handle
    lParam		 - ** not used **

Return Value:

    TRUE	- continues the enumeration

--*/

{
TOTDebugInitFunc();

    DWORD 		  pid = 0;
    DWORD 		  i=0;
    CHAR			  buf[TITLE_SIZE];
    //PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    TOTProcessInformation *lpProc = (TOTProcessInformation *)lParam;
    WINDOW_LIST *lpWindow =NULL;
    DWORD 		  numTasks = 0;
    bool lAdd=false;
    PTASK_LIST 	  lpTask =NULL;
    //TList *lpListe = (TList *) lParam;
    //PTASK_LIST 	  tlist = (PTASK_LIST )lpListe->Object;

  try
  {
    buf[0] =0;
    numTasks = lpProc->GetTaskCount();


    //
    // get the processid for this window
    //
    try
    {
      if (!GetWindowThreadProcessId( hwnd, &pid )) {
	     return TRUE;
      }
    }
    catch(...)
    {
      1;
      return TRUE;
    }
    try
    {
      lpWindow = new WINDOW_LIST;
      if (lpWindow==NULL)
      {
        return TRUE;
      }

      lpWindow->dwProcessId=pid;
      lpWindow->hwnd=hwnd;
      lpWindow->WindowTitle[0]=0;
      lpWindow->ClassName[0]=0;

      if (GetWindowText( hwnd, buf, sizeof(buf) ))
      {
			 //
		  // got it, so lets save it
			   //
  			 strcpy( lpWindow->WindowTitle, buf );
      }
      if (GetClassName( hwnd, buf, sizeof(buf) ))
      {
			 strcpy( lpWindow->ClassName, buf );
      }
      lpProc->AddWindow(lpWindow);
    }
    catch(...)
    {
          TOTDebugError(("Exception in EnumWindowsProc0"));
    }

    lAdd=true;
    //!!!!!!!!!!!

    //
    // look for the task in the task list for this window
    //
    for (i=0; i<numTasks; i++)
    {
      lpTask = (PTASK_LIST ) lpProc->GetTaskNum(i);
	   if (lpTask->dwProcessId == pid)
     {
      try
      {
  		  lpTask->hwnd = hwnd;
        lpTask->WindowCount++;
		    //
  	    // we found the task so lets try to get the
	  	  // window text
		    //
        strcpy( lpTask->WindowTitle, lpWindow->WindowTitle );
        strcpy( lpTask->ClassName, lpWindow->ClassName );

        strcpy( lpWindow->ProcessName, lpTask->ProcessName );
      }
      catch(...)
      {
          TOTDebugError(("Exception in EnumWindowsProc"));
      }
      break;
     }
    }

    //
    // continue the enumeration
    //
    return TRUE;
  }
  catch(Exception &Ex)
  {

    TOTDebugError(("Exception in EnumWindowsProc : %s",Ex.Message.c_str()));
    return FALSE;
  }

  catch(...)
  {
    TOTDebugError(("Exception in EnumWindowsProc"));
    return FALSE;
  }
}


BOOL
MatchPattern(
    PUCHAR String,
    PUCHAR Pattern
    )
{
    UCHAR   c, p, l;

    for (; ;) {
	   switch (p = *Pattern++) {
		  case 0: 					   // end of pattern
			 return *String ? FALSE : TRUE;  // if end of string TRUE

		  case '*':
			 while (*String) {			   // match zero or more char
				if (MatchPattern (String++, Pattern))
				    return TRUE;
			 }
			 return MatchPattern (String, Pattern);

		  case '?':
			 if (*String++ == 0)		   // match any one char
				return FALSE;				  // not end of string
			 break;

		  case '[':
			 if ( (c = *String++) == 0)	   // match char set
				return FALSE;				  // syntax

			 c = toupper(c);
			 l = 0;
			 while (p = *Pattern++) {
				if (p == ']')               // if end of char set, then
				    return FALSE;		   // no match found

				if (p == '-') {             // check a range of chars?
				    p = *Pattern;		   // get high limit of range
				    if (p == 0  ||	p == ']')
					   return FALSE;		  // syntax

				    if (c >= l  &&	c <= p)
					   break; 		   // if in range, move on
				}

				l = p;
				if (c == p)			   // if char matches this element
				    break;			   // move on
			 }

			 while (p  &&	p != ']')         // got a match in char set
				p = *Pattern++;		   // skip to end of set

			 break;

		  default:
			 c = *String++;
			 if (toupper(c) != p)		   // check for exact char
				return FALSE;				  // not a match

			 break;
	   }
    }
}

void InitPidUti()
{

    mVerInfo ;
    mVerInfo.dwOSVersionInfoSize = sizeof (mVerInfo);
    GetVersionEx(&mVerInfo);

    switch (mVerInfo.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
       GetTaskList     = GetTaskListNT;
       EnableDebugPriv = EnableDebugPrivNT;
       break;

    case VER_PLATFORM_WIN32_WINDOWS:
       GetTaskList = GetTaskList95;
       EnableDebugPriv = EnableDebugPriv95;
       break;

    default:
       //printf ("tlist requires Windows NT or Windows 95\n");
       return ;
    }
    EnableDebugPriv(SE_DEBUG_NAME);
    EnableDebugPriv(SE_SHUTDOWN_NAME);
 }

#define MAX_TASKS           256


TOTProcessInformation::TOTProcessInformation()
{
TOTDebugInitFunc();

  mpTaskList = new TList;
  mpWindowList = new TList;
  int numTasks = GetTaskList( mpTaskList, MAX_TASKS );
  GetWindowTitles();
}
TOTProcessInformation::~TOTProcessInformation()
{
  TOTDebugInitFunc();

  try
  {
    while(mpTaskList->Count>0)
    {
      TASK_LIST * lpTask=(TASK_LIST *)mpTaskList->Items[0];
      delete lpTask;
      mpTaskList->Delete(0);
    }
    while(mpWindowList->Count>0)
    {
      WINDOW_LIST *lpWindow =(WINDOW_LIST *)mpWindowList->Items[0];
      delete lpWindow;
      mpWindowList->Delete(0);
    }
  }
  catch(...)
  {
    TOTDebugError(("Error Deleting Process Information"));
  }
  delete mpTaskList;
  delete mpWindowList;
}


int __fastcall TOTProcessInformation::fGetPid(char *  aNameCh,
  char *aWindowName,char *aClassName,bool aSelf)
{
TOTDebugInitFunc();

        //
    // get the task list for the system
    //
 //TASK_LIST   lpTList[MAX_TASKS];
  int ThisPid = GetCurrentProcessId();

  if (aWindowName!=NULL)
  {
    aWindowName[0]=0;
  }
  if (aClassName!=NULL)
  {
    aClassName[0]=0;
  }

 int lLigne;
 int li;
    for (li=0;li<mpWindowList->Count;li++)
    {
        WINDOW_LIST *lpTask=(WINDOW_LIST *) mpWindowList->Items[li];
        char * lName=lpTask->ProcessName ;
        TOTDebug(("GetPid %s ? %s",aNameCh,lName));

        //strupr(lName);
        if (!strncmpi(lName,aNameCh,strlen(aNameCh)))
        {
          int lPid= lpTask->dwProcessId;
          if(aSelf | (lPid!=ThisPid))
          {
            TOTDebug(("GetPid %s  found %s : %d",aNameCh,lName,lPid));
            if (aWindowName!=NULL)
            {
              strncpy(aWindowName,lpTask->WindowTitle, TITLE_SIZE);
              //JC 21/07/2003 : Voir la doc de strncpy !!
              aWindowName[TITLE_SIZE-1]=0;
              TOTDebug(("Windows Title : %s",aWindowName));
            }
            if (aClassName!=NULL)
            {
              strncpy(aClassName,lpTask->ClassName,CLASS_SIZE);
              //JC 21/07/2003 : Voir la doc de strncpy !!
              aClassName[CLASS_SIZE-1]=0;
              TOTDebug(("Class Name : %s",aClassName));
            }

            return  lPid ;
          }
        }
    }
    TOTDebug(("GetPid %s not found",aNameCh));

    return PID_NOTFOUND;
 }


 int TOTProcessInformation::GetTaskCount()
 {
    return mpTaskList->Count;
 }
 PTASK_LIST  TOTProcessInformation::GetTaskNum(int aNum)
 {
    return (TASK_LIST *) mpTaskList->Items[aNum];
 }
 int TOTProcessInformation::GetWindowCount()
 {
   return mpWindowList->Count;
 }
 PWINDOW_LIST  TOTProcessInformation::GetWindowNum(int aNum)
 {
  return (WINDOW_LIST *) mpWindowList->Items[aNum];

 }
PTASK_LIST  TOTProcessInformation::GetTask(char *processname)
{
    for (int li=0;li<mpTaskList->Count;li++)
    {
        TASK_LIST *ptask=(TASK_LIST *) mpTaskList->Items[li];
        if(stricmp(ptask->ProcessName,processname)==0)return ptask;
    }
    return NULL;
}

TOTServiceInformation::TOTServiceInformation(AnsiString aServiceName)
{
TOTDebugInitFunc();
  mServiceName=aServiceName;
  mStatus=-1;
  AnsiString lMsg;
  int lErrNum;

  //
  schService=NULL;
	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager==0)
	{
    lErrNum=fGetLastError(lMsg);

		TOTDebugError(( "OpenSCManager failed, error code = %X : %s\n", lErrNum,lMsg.c_str()));
    return;
	}
  schService = OpenService( schSCManager, mServiceName.c_str(), SERVICE_ALL_ACCESS);
  if (schService==0)
  {
      lErrNum=fGetLastError(lMsg);

			TOTDebugError(( "OpenService failed, error code = %X : %s \n", lErrNum,lMsg.c_str()));
      return;
  }
  UpdateStatus();

}
TOTServiceInformation::~TOTServiceInformation()
{
TOTDebugInitFunc();
  if(schService!=0)
    CloseServiceHandle(schService);
  if(schSCManager!=0)
    CloseServiceHandle(schSCManager);

}
bool TOTServiceInformation::UpdateStatus()
{
TOTDebugInitFunc();

  QueryServiceStatus(schService,&mInternalStatus);
  mStatus=mInternalStatus.dwCurrentState;
  return true;
}

bool TOTServiceInformation::KillService()
{
TOTDebugInitFunc();
  if(schService==0)
    return false;
  SERVICE_STATUS status;
  if(ControlService(schService,SERVICE_CONTROL_STOP,&status))
  {
    return true;
  }
  else
  {
    AnsiString lMsg;
    int lErrNum;
    lErrNum=fGetLastError(lMsg);

    TOTDebugError(( "ControlService failed, error code = %X\n", lErrNum,lMsg.c_str()));
  }
	return false;
}

bool TOTServiceInformation::RunService( int nArg, char** pArg)
{
TOTDebugInitFunc();
  if(schService==0)
  {
    return false;
  }
  // Service deja lancé
  if(mStatus==SERVICE_RUNNING	)
  {
    return true;
  }


  if(StartService(schService,nArg,(const char**)pArg))
  {
    return true;
  }
  else
  {
      AnsiString lMsg;
      int lErrNum;
    lErrNum=fGetLastError(lMsg);

    TOTDebugError(( "StartService failed, error code = %X\n", lErrNum,lMsg.c_str()));
  }

	return false;
}

bool TOTServiceInformation::WaitRun(int aTimeOut)
{
  TOTDebugInitFunc();
  do
  {
    UpdateStatus();
    if(mStatus!=SERVICE_RUNNING)
    {
      Sleep(100);
      aTimeOut-=100;
    }
    if (aTimeOut<=0)
    {
      return false;
    }

  }
  while(mStatus==SERVICE_START_PENDING);

  return (mStatus==SERVICE_RUNNING);
}

bool TOTServiceInformation::WaitStop(int aTimeOut)
{
  TOTDebugInitFunc();
  do
  {
    UpdateStatus();
    if(mStatus!=SERVICE_STOPPED)
    {
      Sleep(100);
      aTimeOut-=100;
    }
    if (aTimeOut<=0)
    {
      return false;
    }

  }
  while(mStatus==SERVICE_STOP_PENDING);

  return (mStatus==SERVICE_STOPPED);
}

bool fIsProcessPresent(int aPid)
{
  HANDLE		  hProcess;
  hProcess = OpenProcess( PROCESS_QUERY_INFORMATION	, FALSE, aPid );
  int lRet=(hProcess!=NULL);
  if (lRet)
  {
    DWORD lExitCode;


    if(GetExitCodeProcess(hProcess,&lExitCode))
    {
      if(lExitCode!=STILL_ACTIVE)
      {
        int lInt=lExitCode;
        // Le Process a terminé
        lRet=0;
      }
    }

    CloseHandle( hProcess );
  }

  return lRet;

}
