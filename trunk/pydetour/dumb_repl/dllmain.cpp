// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <stdio.h>

#include "guiconsole.h"
#include "process_stuff.h"
#include <gdetour.h>

#include <string>
#include <sstream>

HMODULE hmod_k32 = NULL;

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
		hmod_k32 = GetModuleHandle("kernel32.dll");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		printf("Goodbye forever! (DLL_PROCESS_DETACH)\n");
		CloseHandle(hmod_k32);
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
	BYTE* lla = (BYTE*) GetProcAddress(hmod_k32, "LoadLibraryExA");
	DWORD tid = GetCurrentThreadId();

	if (!GetModuleHandleW((LPCWSTR)l.paramZero)) {
		wprintf(L"%p %p: %s (thread %d, first load, delaying .25s)\n", *(&l.paramZero-1), l.ret_addr, l.paramZero, tid);
	} else {
		wprintf(L"%p: %s (thread %d, already loaded)\n", *(&l.paramZero-1), l.paramZero, tid);
	}
}

PYTHON_DETOUR_API int run_python_file(char* filename, bool debugging) {
	RedirectIOToConsole();
	printf("run_python_file(%s, %d);\n", filename, debugging);

	HMODULE* modules = GetLoadedModules(GetCurrentProcess());
	delete[] modules;


	printf("Detouring LoadLibraryExW\n");
	ll_detour = add_detour(
		(BYTE*) GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryExW"),
		7,
		0x0C,
		&ll_hook,
		0
	);
	ll_detour->gateway_opt.call_original_on_return = true;
	GDetour_Apply(ll_detour);

	return 0; //0 is success
}


