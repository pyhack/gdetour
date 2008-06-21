// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "generic_detour.h"
#include "python_funcs.h"


DWORD WINAPI mainThread(void* lParam) {
	Py_Initialize();
	if (!Py_IsInitialized()) {
		OutputDebugString("Python could not be initilized\n");
	}

    PyGILState_STATE gstate = PyGILState_Ensure();

	InitilizePythonFuncs();

	//PyObject* s = run_python_string("import ctypes\nctypes.windll.user32.MessageBoxA(0, 'runcode is running', 'test.py', 0)");
	//Py_XDECREF(s);



	//Sleep(1000);

	//test_detour_func(2);

	//add_test_detour();

	//test_detour_func(2);

	//AllocConsole();

	//run_test();

	//test_detour_func(2);
	

	PyGILState_Release(gstate);
	return 0;
};

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		DWORD newId;
		CreateThread(NULL, 0, &mainThread,0, 0, &newId);

		//GDetour::initialize();

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		PyGILState_STATE gstate = PyGILState_Ensure();
		Py_Finalize();
		PyGILState_Release(gstate);
		break;
	}
	return TRUE;
}

