#pragma once

#ifdef GENERIC_DETOUR_EXPORTS
#define GENERIC_DETOUR_API extern "C" __declspec(dllexport)
#else
#define GENERIC_DETOUR_API extern "C" __declspec(dllimport)
#endif

//extern PyThreadState* mainPythonThreadState;

extern PyObject* myPyGlobals;
extern PyObject* myPyLocals;

GENERIC_DETOUR_API void run_test();

GENERIC_DETOUR_API PyObject* run_python_string(char* pycode);
GENERIC_DETOUR_API int run_python_file(char* filename);

void Python_Initialize();
void Python_Unload();

PyGILState_STATE Python_GrabGIL();
void Python_ReleaseGIL(PyGILState_STATE);