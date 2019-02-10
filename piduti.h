
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*		  Copyright (C) 1994-1996 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/


#ifndef PIDUTI_H
#define PIDUTI_H

#define TITLE_SIZE          64
#define PROCESS_SIZE        MAX_PATH
#define CLASS_SIZE          64
#define PID_NOTFOUND   -1
#ifdef WIN32
#include <classes.hpp>
#endif

int fGetLastError(AnsiString &aErrorMsg);


//
// task list structure
//
typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    DWORD       dwInheritedFromProcessId;
    BOOL        flags;
    HANDLE      hwnd;
    CHAR        ProcessName[PROCESS_SIZE];
    CHAR        WindowTitle[TITLE_SIZE];
    CHAR        ClassName[CLASS_SIZE];
    int         WindowCount;

} TASK_LIST, *PTASK_LIST;

typedef struct _WINDOW_LIST {
    DWORD       dwProcessId;
    HANDLE      hwnd;
    CHAR        ProcessName[PROCESS_SIZE];
    CHAR        WindowTitle[TITLE_SIZE];
    CHAR        ClassName[CLASS_SIZE];

} WINDOW_LIST, *PWINDOW_LIST;

/*
typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    PWINDOW_LIST  w4list;
    DWORD       numtasks;
    DWORD       numwindows;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;
*/

//
// Function pointer types for accessing platform-specific functions
//
typedef DWORD (*LPGetTaskList)(TList *, DWORD);
typedef BOOL  (*LPEnableDebugPriv)(char *);

void InitPidUti() ;

//extern LPGetTaskList     GetTaskList;


//
// Function prototypes
//
DWORD
GetTaskList95(
    TList *apListe,
    DWORD       dwNumTasks
    );

DWORD
GetTaskListNT(
    TList *apListe,
    DWORD       dwNumTasks
    );


BOOL
EnableDebugPriv95(
    char *
    );

BOOL
EnableDebugPrivNT(
    char *
    );

BOOL
KillProcess(
    PTASK_LIST tlist,
    BOOL       fForce
    );

    /*
VOID
GetWindowTitles(
    PTASK_LIST_ENUM te
    );
*/
BOOL
MatchPattern(
    PUCHAR String,
    PUCHAR Pattern
    );

//int __fastcall fGetPid(char *  aNameCh,bool aSelf=false);

class TOTProcessInformation
{
  private :
    //TASK_LIST   *mpTList;
   TList *mpTaskList;
   TList *mpWindowList;
  public :
    TOTProcessInformation();
    ~TOTProcessInformation();
    int GetWindowTitles();

    int __fastcall fGetPid(char *  aNameCh,char *aWindowName,char * aClassName,bool aSelf);
    int GetTaskCount() ;
    PTASK_LIST  GetTask(char *processname);
    PTASK_LIST  GetTaskNum(int aNum);
    int GetWindowCount() ;
    PWINDOW_LIST  GetWindowNum(int aNum);
    void AddWindow(PWINDOW_LIST aWindow);
    BOOL KillProcess(PTASK_LIST tlist,BOOL fForce);
};

enum TOTServiceStatus { TOTServiceStatus_Error,TOTServiceStatus_Stopped,TOTServiceStatus_Running,TOTServiceStatus_Starting,TOTServiceStatus_Stopping}; 

class TOTServiceInformation
{
  private :
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

  public :
  _SERVICE_STATUS mInternalStatus;
    int mStatus;
    AnsiString mServiceName;
    TOTServiceInformation(AnsiString aServiceName);
    ~TOTServiceInformation();
   bool UpdateStatus();
   bool KillService();
   bool RunService( int nArg, char** pArg);
   bool WaitRun(int aTimeOut);
   bool WaitStop(int aTimeOut);

   inline bool  isOk(void) const { return schService != NULL; }

};

bool fIsProcessPresent(int aPid);

#endif


