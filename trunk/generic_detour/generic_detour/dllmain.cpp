// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "generic_detour.h"
#include "python_funcs.h"




BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Python_Initialize();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		Python_Unload();
		break;
	}
	return TRUE;
}

