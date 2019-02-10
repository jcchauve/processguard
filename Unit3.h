//---------------------------------------------------------------------------
#ifndef Unit3H
#define Unit3H
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <SvcMgr.hpp>
#include <vcl.h>
//---------------------------------------------------------------------------
class TService3 : public TService
{
__published:    // Composants g�r�s par l'EDI
private:        // D�clarations utilisateur
public:         // D�clarations utilisateur
	__fastcall TService3(TComponent* Owner);
	TServiceController __fastcall GetServiceController(void);

	friend void __stdcall ServiceController(unsigned CtrlCode);
};
//---------------------------------------------------------------------------
extern PACKAGE TService3 *Service3;
//---------------------------------------------------------------------------
#endif
