#include <SysUtils.hpp>
#include <SvcMgr.hpp>
#pragma hdrstop
#include <tchar.h>
USEFORM("fpguard.cpp", ProcessGuardService); /* TService: File Type */
//---------------------------------------------------------------------------
#define Application Svcmgr::Application
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	try
	{
		// Windows 2003 Server n�cessite que StartServiceCtrlDispatcher soit
		// appel� avant CoRegisterClassObject, qui peut �tre appel� indirectement
		// par Application.Initialize. TServiceApplication->DelayInitialize permet
		// l'appel de Application->Initialize depuis TService->Main (apr�s
		// l'appel de StartServiceCtrlDispatcher).
		//
		// L'initialisation diff�r�e de l'objet Application peut affecter
		// les �v�nements qui surviennent alors avant l'initialisation, tels que
		// TService->OnCreate. Elle est seulement recommand�e si le ServiceApplication
		// enregistre un objet de classe avec OLE et est destin�e � une utilisation
		// avec Windows 2003 Server.
		//
		// Application->DelayInitialize = true;
		//
		if ((!Application->DelayInitialize) || (Application->Installing()))
		{
			Application->Initialize();
		}
		Application->CreateForm(__classid(TProcessGuardService), &ProcessGuardService);

		Application->Run();
	}
	catch (Exception &exception)
	{
		Sysutils::ShowException(&exception, System::ExceptAddr());
	}
	catch(...)
	{
		try
		{
			throw Exception("");
		}
		catch(Exception &exception)
		{
			Sysutils::ShowException(&exception, System::ExceptAddr());
		}
        }
	return 0;
}
