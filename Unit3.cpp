//---------------------------------------------------------------------------
#include "Unit3.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TService3 *Service3;
//---------------------------------------------------------------------------
__fastcall TService3::TService3(TComponent* Owner)
	: TService(Owner)
{
}

void __stdcall ServiceController(unsigned CtrlCode)
{
	Service3->Controller(CtrlCode);
}

TServiceController __fastcall TService3::GetServiceController(void)
{
	return (TServiceController) ServiceController;
}

//---------------------------------------------------------------------------
