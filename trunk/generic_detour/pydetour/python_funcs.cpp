#include "stdafx.h"
#include "python_funcs.h"
#include "python_module_pydetour.h"
#include "python_type_registers.h"

using namespace CPPPython;

PDict pyLocals = NULL;
PDict pyGlobals = NULL;
//PyObject* myPyGlobals;
//PyObject* myPyLocals;

GENERIC_DETOUR_API PyObject* run_python_string(char* pycode) {
	P_GIL gil;

	PyObject* ret = PyRun_String(pycode, Py_single_input, pyGlobals, pyLocals);	
	if (PyErr_Occurred()) {
		OutputDebugString("error occured running pystring");
		PyErr_Print();
	}
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
				//strncpy(&fnbuf[i+1], filename, 1023 - i);
				strncpy_s(&fnbuf[i+1], 1023 - i, filename, 1023 - i);
				break;
			}
		}
	} else {
		//strncpy(fnbuf, filename, 1023);
		strncpy_s(fnbuf, 1023, filename, 1023);
	}

	P_GIL gil;

	PyObject* pyfile = PyFile_FromString(fnbuf,"r");
	if (PyErr_Occurred()) { 
		PyErr_Print(); 
		OutputDebugString("pyerror occured reading file");
	}
	if(pyfile==NULL){
		OutputDebugString("pyfile is null");
		Py_XDECREF(pyfile);
		return 0;
	}

	FILE* f = PyFile_AsFile(pyfile);
	PyRun_File(f,fnbuf, Py_file_input, pyGlobals, pyLocals);

	if (PyErr_Occurred()) {
		//probably an exception
		OutputDebugString("pyerror occured running file");
		PyErr_Print();
		Py_XDECREF(pyfile);
		return 0;
	}

	Py_DECREF(pyfile);
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

	PModule mainmod = PModule::getModule("__main__");
	PDict d = PyModule_GetDict(mainmod); //borrowed ref

	pyGlobals = d;
	pyLocals = d;

	//These three do import gdetour
	//initgdetour();
	//PModule gd = PModule::importModule("gdetour", pyGlobals, pyLocals);
	//mainmod.AddObject("gdetour", gd);


}
void Python_Unload() {
	P_GIL gil;

	pyGlobals = NULL;
	pyLocals = NULL;

	Py_Finalize();

}
