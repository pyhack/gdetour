#include "stdafx.h"
#include "python_funcs.h"

#include "python_type_registers.h"



PyObject* myPyGlobals;
PyObject* myPyLocals;

GENERIC_DETOUR_API PyObject* run_python_string(char* pycode) {
	PyGILState_STATE state = Python_GrabGIL();
	PyObject* ret = PyRun_String(pycode, Py_single_input, myPyGlobals, myPyLocals);	
	if (PyErr_Occurred()) {
		OutputDebugString("error occured running pystring");
		PyErr_Print();
	}
	Python_ReleaseGIL(state);
	return ret;
}
GENERIC_DETOUR_API int run_python_file(char* filename) {
	char fnbuf[1024];
	memset(&fnbuf, 0, sizeof(fnbuf));
	if (filename[1] != ':') {
		//assume that this is a relative path from this dll
		HMODULE h = GetModuleHandle("gdetour.dll");
		GetModuleFileName(h, fnbuf, 1022 - strlen(filename));
		int l = strlen(fnbuf);
		for(int i = l; i > 0; i--) {
			if (fnbuf[i] == '\\') {
				strncpy(&fnbuf[i+1], filename, 1023 - i);
				break;
			}
		}
	} else {
		strncpy(fnbuf, filename, 1023);
	}

	PyGILState_STATE state = Python_GrabGIL();
	PyObject* pyfile = PyFile_FromString(fnbuf,"r");
	if (PyErr_Occurred()) { 
		PyErr_Print(); 
		OutputDebugString("pyerror occured reading file");
	}
	if(pyfile==NULL){
		OutputDebugString("pyfile is null");
		Py_XDECREF(pyfile);
		Python_ReleaseGIL(state);
		return 0;
	}

	FILE* f = PyFile_AsFile(pyfile);
	PyRun_File(f,fnbuf, Py_file_input, myPyGlobals, myPyLocals);

	if (PyErr_Occurred()) {
		//probably an exception
		OutputDebugString("pyerror occured running file");
		PyErr_Print();
		Py_XDECREF(pyfile);
		Python_ReleaseGIL(state);
		return 0;
	}

	Py_DECREF(pyfile);
	Python_ReleaseGIL(state);
	return 1;
}

////////////////////////////////////////////////////////









//------------------------------------------------------------------------------

//PyThreadState* mainPythonThreadState = NULL;

void Python_Initialize() {
	Py_Initialize();
	if (!Py_IsInitialized()) {
		OutputDebugString("Python could not be initilized\n");
	}
	PyEval_InitThreads();
	//mainPythonThreadState = PyThreadState_Get();

	PyObject* mainmod = PyImport_AddModule("__main__"); //borrowed ref
	PyObject* d = PyModule_GetDict(mainmod); //borrowed ref

	Py_INCREF(d);
	Py_INCREF(d);

	myPyGlobals = d;
	myPyLocals = d;

	//Init_gdetour();

	//PyImport_ImportModuleEx("gdetour", myPyGlobals, myPyLocals, NULL);

	//PyEval_ReleaseLock();
}
void Python_Unload() {
	PyGILState_STATE state = Python_GrabGIL();

	Py_XDECREF(myPyGlobals);
	Py_XDECREF(myPyLocals);

	//Py_XDECREF(Detour_Exception_AccessViolation);
	//Py_XDECREF(Detour_Exception_WindowsException);
	//Py_XDECREF(Detour_Exception);


	myPyGlobals = NULL;
	myPyLocals = NULL;

	Py_Finalize();

}


PyGILState_STATE Python_GrabGIL() {
	PyGILState_STATE state = PyGILState_Ensure();
	return state;
/*
	PyEval_AcquireLock();
	PyInterpreterState* s = mainPythonThreadState->interp;

	PyThreadState* myThreadState = PyThreadState_New(s);

	PyThreadState_Swap(myThreadState);
	*/
}
void Python_ReleaseGIL(PyGILState_STATE state) {
	if (state == NULL) {
		return;
	}
	PyGILState_Release(state);
}
