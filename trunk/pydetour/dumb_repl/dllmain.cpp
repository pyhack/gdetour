// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <stdio.h>

#include "guiconsole.h"
#include "process_stuff.h"
#include <gdetour.h>

GDetour* ll_detour = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		printf("O Hello! (DLL_PROCESS_ATTACH)\n");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		//printf("Goodbye forever! (DLL_PROCESS_DETACH)\n");
		break;
	}
	return TRUE;
}


PYTHON_DETOUR_API void run_test() {
	printf("run_test();\n");
	return;
}

PYTHON_DETOUR_API int* run_python_string(char* pycode) {
	printf("run_python_string(%s);\n", pycode);
	return NULL;
}


void ll_hook(GDetour &d, DETOUR_LIVE_SETTINGS &l) {
	wprintf(L"%s", l.paramZero);
	Sleep(1000);
}

PYTHON_DETOUR_API int run_python_file(char* filename, bool debugging) {
	RedirectIOToConsole();
	printf("run_python_file(%s, %d);\n", filename, debugging);

	HMODULE* modules = GetLoadedModules(GetCurrentProcess());
	delete[] modules;

	__asm { int 3 }

	printf("detouring loadlibraryexw\n");
	ll_detour = GDetour_Create(
		(BYTE*) GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryExW"),
		7,
		20,
		&ll_hook,
		0x0C
	);
	ll_detour->gateway_opt.call_original_on_return = true;
	GDetour_Apply(ll_detour);

	__asm { int 3 }
	return 0; //0 is success
}


