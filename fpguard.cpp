//---------------------------------------------------------------------------
#include "fpguard.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#include <tlhelp32.h>
#include <stdio.h>
#include <IniFiles.hpp>
#include "piduti.h"
#include "windows.h"
#include "stdint.h"


#ifdef _WIN64
typedef std::uint64_t ApiInt;
#else
typedef std::uint32_t ApiInt;
#endif

//#define GROSDEBUG
TProcessGuardService *ProcessGuardService;
//---------------------------------------------------------------------------
__fastcall TProcessGuardService::TProcessGuardService(TComponent* Owner)
	: TService(Owner)
{
  InitPidUti();
  mLastIniRead=-1;
  mpIniFile=NULL;
  mIniFileName="ProcessGuard.ini";
  char *lWinDir=getenv("windir");
  if(lWinDir==NULL)
  {
    lWinDir=getenv("WINDIR");
  }
  if(lWinDir)
  {
    mIniFileName=AnsiString(lWinDir)+AnsiString("\\")+mIniFileName;
  }
  if(mpIniFile==NULL)
  {
    mpIniFile = new TMemIniFile(mIniFileName);
    mpIniFile->CaseSensitive=false;
    mLastIniRead=FileAge(mIniFileName);
  }
  mpServices=new TStringList();
  mpIniFile->ReadSection("Services",mpServices);
  mCurTime=0;

}

void __stdcall ServiceController(unsigned CtrlCode)
{
	ProcessGuardService->Controller(CtrlCode);
}

TServiceController __fastcall TProcessGuardService::GetServiceController(void)
{
	return (TServiceController) ServiceController;
}

//---------------------------------------------------------------------------
void __fastcall TProcessGuardService::ServiceAfterInstall(TService *Sender)
{
  int aaa=1;
}

////////////////////////////////////////////////////////////////////
//
// Function: EnableTokenPrivilege()
//
// Added: 20/02/99
//
// Description: Enables a specific token privilege
//
///////////////////////////////////////////////////////////////////
static BOOL EnableTokenPrivilege ( LPTSTR privilege )
{
	HANDLE		hToken;				// process token
	TOKEN_PRIVILEGES tp;			// token provileges
	DWORD		dwSize;

	// initialize privilege structure
	ZeroMemory (&tp, sizeof (tp));
	tp.PrivilegeCount = 1;

	// open the process token
	if ( !OpenProcessToken ( GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken ) )
		return FALSE;

	// look up the privilege LUID and enable it
	if ( !LookupPrivilegeValue ( NULL, privilege, &tp.Privileges[0].Luid ) )
	{
		CloseHandle ( hToken);
		return FALSE;
	}

	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// adjust token privileges
	if ( !AdjustTokenPrivileges ( hToken, FALSE, &tp, 0, NULL, &dwSize ) )
	{
		CloseHandle ( hToken);
		return FALSE;
	}

	// clean up
	CloseHandle ( hToken );
	return TRUE;
}

// found by experimentation this is where the some
// process data block is found in an NT machine.
// On an Intel system, 0x00020000 is the 32
// memory page. At offset 0x0498 is the process
// current directory (or startup directory, not sure yet)
// followed by the system's PATH. After that is the
// process full command command line, followed by
// the exe name and the window
// station it's running on
#define BLOCK_ADDRESS	(LPVOID)0x00020498
// Additional comments:
// From experimentation I've found
// two notable exceptions where this doesn't seem to apply:
// smss.exe : the page is reserved, but not commited
//			which will get as an invalid memory address
//			error
// crss.exe : although we can read the memory, it's filled
//			  with 00 comepletely. No trace of command line
//			  information

// simple macro to handle errors
#define SIGNAL_ERROR() { bError = TRUE; return FALSE; }
// align pointer
#define ALIGN_DWORD(x) ( (x & 0xFFFFFFFC) ? (x & 0xFFFFFFFC) + sizeof(DWORD) : x )

BOOL GetProcessCmdLine ( HANDLE hProcess, LPWSTR lpszCmdLine )
{
	LPBYTE						lpBuffer = NULL;
	LPBYTE						lpPos = NULL; // offset from the start of the buffer
	ApiInt						dwBytesRead;
	MEMORY_BASIC_INFORMATION	mbi;
	SYSTEM_INFO					sysinfo;
	BOOL						bError = FALSE;

	__try {
		// Get the system page size by using GetSystemInfo()
		GetSystemInfo ( &sysinfo );
		// allocate one on the heap to retrieve a full page
		// of memory
		lpBuffer = (LPBYTE)malloc ( sysinfo.dwPageSize );
		if ( lpBuffer == NULL )
			SIGNAL_ERROR ();

		// first of all, use VirtualQuery to get the start of the memory
		// block
		if ( VirtualQueryEx ( hProcess, BLOCK_ADDRESS, &mbi, sizeof(mbi) ) == 0 )
			SIGNAL_ERROR ();

		// read memory begining at the start of the page
		// after that, we know that the env strings block
		// will be 0x498 bytes after the start of the page
		if ( !ReadProcessMemory ( hProcess, mbi.BaseAddress, (LPVOID)lpBuffer,
								  sysinfo.dwPageSize, &dwBytesRead ) )
			 SIGNAL_ERROR ();

		// now we've got the buffer on our side of the fence.
		// first, lpPos points to a string containing the current directory
		/// plus the path.
		lpPos = lpBuffer + ((DWORD)BLOCK_ADDRESS - (DWORD)mbi.BaseAddress);
		lpPos = lpPos + (wcslen ( (LPWSTR)lpPos ) + 1) * sizeof(WCHAR);
		// now goes full path an filename, aligned on a DWORD boundary
		// skip it
		lpPos = (LPBYTE)ALIGN_DWORD((DWORD)lpPos);
		lpPos = lpPos + (wcslen ( (LPWSTR)lpPos ) + 1) * sizeof(WCHAR);
		// hack: Sometimes, there will be another '\0' at this position
		// if that's so, skip it
		if ( *lpPos == '\0' ) lpPos += sizeof(WCHAR);
		// now we have the actual command line
		// copy it to the buffer
		wcsncpy  ( lpszCmdLine, (LPWSTR)lpPos, MAX_PATH );
		// make sure the path is null-terminted
		lpszCmdLine[MAX_PATH-1] = L'\0';

	}
	__finally  {
		// clean up
		if ( lpBuffer != NULL ) free ( lpBuffer );
		return bError ? FALSE : TRUE;
	}

}


//---------------------------------------------------------------------------
void __fastcall TProcessGuardService::ServiceStart(TService *Sender,
      bool &Started)
{
  Started=true;
  ProcessTimer->Enabled=true;
  EnableTokenPrivilege ( SE_DEBUG_NAME );
  //Sleep(20000);

}

static int fPriorityClass(AnsiString aClass)
{
  if(aClass.UpperCase()=="LOW")
  {
    return BELOW_NORMAL_PRIORITY_CLASS;
  }
  if(aClass.UpperCase()=="HIGH")
  {
    return HIGH_PRIORITY_CLASS;
  }
  if(aClass.UpperCase()=="ALOVE")
  {
    return BELOW_NORMAL_PRIORITY_CLASS;
  }
  if(aClass.UpperCase()=="IDLE")
  {
    return IDLE_PRIORITY_CLASS;
  }

  return NORMAL_PRIORITY_CLASS;
}



//---------------------------------------------------------------------------
void __fastcall TProcessGuardService::ProcessTimerTimer(TObject *Sender)
{
  int lAAA=1;
  mCurTime+=(ProcessTimer->Interval/1000);

  int lNewAge=FileAge(mIniFileName);
  if(mLastIniRead==-1 || (lNewAge>mLastIniRead))
  {
    delete   mpIniFile;
    mpIniFile = new TMemIniFile(mIniFileName);
	mpIniFile->CaseSensitive=false;
	mLastIniRead=lNewAge;
  }

#ifdef GROSDEBUG
  FILE *lpOut=fopen("processguard.log","at");
	//fprintf(lpOut,"Ages : %d/%d",mLastIniRead,lNewAge);
  if(lpOut)
  {
	fprintf(lpOut,"------\n");
	fflush(lpOut);
  }
  #endif

  if(mpServices->Count>0)
  {
	for(int li=0;li<mpServices->Count;li++)
	{
	  AnsiString lServiceName=mpServices->Strings[li];
	  int lTimeStart=mpIniFile->ReadInteger("Services",lServiceName,120);
	  if(mCurTime>=lTimeStart)
	  {
		TOTServiceInformation lInfo(lServiceName);
		if(lInfo.mStatus==TOTServiceStatus_Stopped)
		{
#ifdef GROSDEBUG
        fprintf(lpOut,"Starting Service  %s ...\n",lServiceName.c_str());
#endif

          char **lpArg={NULL,};
          lInfo.RunService(0,lpArg);
          mpServices->Delete(li);
          li--;
        }
      }
    }
  }


  HANDLE hSnapShot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
  PROCESSENTRY32* processInfo=new PROCESSENTRY32;
  processInfo->dwSize=sizeof(PROCESSENTRY32);
  int index=0;
  bool lNeedFlush=false;
  //TStringList *lpListe=new TStringList();
  try
  {
	//if(FileExists("processGuard.txt"))
	{
	  //lpListe->LoadFromFile("processGuard.txt");
	}
  }
  catch(...)
  {
  }
  while(Process32Next(hSnapShot,processInfo)!=FALSE)
  {
  /*
cout<<endl<<"***********************************************";
cout<<endl<<"\t\t\t"<<++index;
cout<<endl<<"***********************************************";
cout<<endl<<"Parent Process ID: "<<processInfo->th32ParentProcessID;
cout<<endl<<"Process ID: "<<processInfo->th32ProcessID;
cout<<endl<<"Name: "<<processInfo->szExeFile;
cout<<endl<<"Current Threads: "<<processInfo->cntThreads;
cout<<endl<<"Current Usage: "<<processInfo->cntUsage;
cout<<endl<<"Flags: "<<processInfo->dwFlags;
cout<<endl<<"Size: "<<processInfo->dwSize;
cout<<endl<<"Primary Class Base: "<<processInfo->pcPriClassBase;
cout<<endl<<"Default Heap ID: "<<processInfo->th32DefaultHeapID;
cout<<endl<<"Module ID: "<<processInfo->th32ModuleID;
*/
#ifdef GROSDEBUG
	fprintf(lpOut,"ExecName (PID=%d): %s (Prior=%d)\n",(int)processInfo->th32ProcessID,processInfo->szExeFile,(int)processInfo->pcPriClassBase);
#endif
	AnsiString lExe=processInfo->szExeFile;
	AnsiString lPrior=mpIniFile->ReadString("Priority",lExe,"");
	if(lExe=="notepad.exe")
	{
	  int toto=1;
	}
	unsigned long lAffinity=mpIniFile->ReadInteger("Affinity",lExe,0);
	int lKill=mpIniFile->ReadInteger("Kill",lExe,0);

	AnsiString lDate=mpIniFile->ReadString("History",lExe,"");
	AnsiString lCmdLine=mpIniFile->ReadString("CmdLine",lExe,"");
	if(lExe.Pos('[')>0)
	{
	  continue;
	}

	if(lDate.IsEmpty())
	{
	  lDate=TDateTime::CurrentDateTime().FormatString("yyyymmdd hhnnss");
	  mpIniFile->WriteString("History",lExe,lDate);
	  lNeedFlush=true;
	}


	if(lKill || (!lPrior.IsEmpty())|| (lAffinity!=0) || (lCmdLine.IsEmpty()))
    {
      //fprintf(lpOut,"OpenProcess ...\n");
      HANDLE hProcess=OpenProcess(PROCESS_ALL_ACCESS,TRUE,processInfo->th32ProcessID);
      if(hProcess==NULL)
      {
      //cout<<"Unable to get handle of process: "<<processID;
      //cout<<"Error is: "<<GetLastError();
      //return 1;
        continue;
      }
      if(lCmdLine.IsEmpty())
      {
        WCHAR	cmdline[MAX_PATH];

        GetProcessCmdLine(hProcess,cmdline);
        AnsiString lCmdLine=WideString(cmdline);
		if(!lCmdLine.IsEmpty())
        {
          mpIniFile->WriteString("CmdLine",lExe,lCmdLine);
          lNeedFlush=true;
        }
      }
      if(!lPrior.IsEmpty())
      {
		int lProcessClass=GetPriorityClass(hProcess);
        int lNewClass=fPriorityClass(lPrior);
        if(lNewClass!=lProcessClass)
        {
          SetPriorityClass(hProcess,lNewClass);
#ifdef GROSDEBUG
		  fprintf(lpOut,"Modify Priority Class %s\n",processInfo->szExeFile,lNewClass);
#endif
		}
	  }
	  if(lAffinity!=0)
	  {
		ApiInt lProcessAffinityMask=0;
		ApiInt lSystemAffinityMask=0;
		if(GetProcessAffinityMask(hProcess,&lProcessAffinityMask,&lSystemAffinityMask))
		{
		  lAffinity=lAffinity &lSystemAffinityMask;
		  if(lAffinity!=lProcessAffinityMask)
		  {
			SetProcessAffinityMask(hProcess,lAffinity);
#ifdef GROSDEBUG
			fprintf(lpOut,"Modify Affinity  %s ->%d\n",processInfo->szExeFile,lAffinity);
#endif
		  }
		}
	  }
	  if(lKill)
	  {
#ifdef GROSDEBUG
			fprintf(lpOut,"Terminate Process  %s (PID#%d)\n",processInfo->szExeFile,processInfo->th32ProcessID);
#endif

		TerminateProcess(hProcess,0);
	  }
	  CloseHandle(hProcess);

	}
  }
  delete processInfo;
  TOTProcessInformation lTOTProcessInfo;
  int lShowClass=mpIniFile->ReadInteger("Show","Class",-2);
  if(lShowClass==-2)
  {
	  mpIniFile->WriteInteger("Show","Class",0);
	  lNeedFlush=true;
  }
  for(int li=0;li<lTOTProcessInfo.GetWindowCount();li++)
  {
	  PWINDOW_LIST  lpWin=lTOTProcessInfo.GetWindowNum(li);
	  AnsiString lProcessName=lpWin->ProcessName;
	  AnsiString lClassName=lpWin->ClassName;
	  AnsiString lWindowTitle=lpWin->WindowTitle;
	  int lShow=mpIniFile->ReadInteger("Show",lProcessName,-1);
	  if(lShow==0)
	  {
		  ShowWindow((HWND)lpWin->hwnd,SW_HIDE);
	  }
	  if(lShow==1)
	  {
		  ShowWindow((HWND)lpWin->hwnd,SW_NORMAL);
	  }
	  if(lShow==2)
	  {
		  ShowWindow((HWND)lpWin->hwnd,SW_MINIMIZE);
	  }
	  if(lShowClass>0)
	  {
		  mpIniFile->WriteString("Show",lClassName,lWindowTitle);

      }
  }


  if(lNeedFlush)
	mpIniFile->UpdateFile();
#ifdef GROSDEBUG
  if(lpOut)
	fclose(lpOut);
#endif
  CloseHandle(hSnapShot);
}
//---------------------------------------------------------------------------

void __fastcall TProcessGuardService::ServicePause(TService *Sender,
      bool &Paused)
{
  ProcessTimer->Enabled=false;
  Paused=true;

}
//---------------------------------------------------------------------------

void __fastcall TProcessGuardService::ServiceContinue(TService *Sender,
      bool &Continued)
{
    ProcessTimer->Enabled=true;
    Continued=true;

}
//---------------------------------------------------------------------------

void __fastcall TProcessGuardService::ServiceStop(TService *Sender,
      bool &Stopped)
{
  ProcessTimer->Enabled=false;
  Stopped=true;
}
//---------------------------------------------------------------------------

void __fastcall TProcessGuardService::ServiceShutdown(TService *Sender)
{
    delete mpIniFile;
    mpIniFile=NULL;
    delete mpServices;
}
//---------------------------------------------------------------------------

