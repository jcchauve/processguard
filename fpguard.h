//---------------------------------------------------------------------------
#ifndef fpguardH
#define fpguardH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <SvcMgr.hpp>
#include <vcl.h>
#include <ExtCtrls.hpp>
#include <IniFiles.hpp>

//class DELPHICLASS TIniFile;
//---------------------------------------------------------------------------
class TProcessGuardService : public TService
{
__published:    // IDE-managed Components
  TTimer *ProcessTimer;
  void __fastcall ServiceAfterInstall(TService *Sender);
  void __fastcall ServiceStart(TService *Sender, bool &Started);
  void __fastcall ProcessTimerTimer(TObject *Sender);
  void __fastcall ServicePause(TService *Sender, bool &Paused);
  void __fastcall ServiceContinue(TService *Sender, bool &Continued);
  void __fastcall ServiceStop(TService *Sender, bool &Stopped);
  void __fastcall ServiceShutdown(TService *Sender);
protected:        // User declarations
  AnsiString mIniFileName;
  int mLastIniRead;
  TMemIniFile *mpIniFile;
  TStringList *mpServices;
  int mCurTime;
public:         // User declarations
	__fastcall TProcessGuardService(TComponent* Owner);
	TServiceController __fastcall GetServiceController(void);

	friend void __stdcall ServiceController(unsigned CtrlCode);
};
//---------------------------------------------------------------------------
extern PACKAGE TProcessGuardService *ProcessGuardService;
//---------------------------------------------------------------------------
#endif
