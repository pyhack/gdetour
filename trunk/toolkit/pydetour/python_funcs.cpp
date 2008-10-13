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
GENERIC_DETOUR_API int run_python_file(char* filename, bool debugging) {
	char fnbuf[1024];
	memset(&fnbuf, 0, sizeof(fnbuf));
	if (filename[1] != ':') {
		//assume that this is a relative path from this dll
		HMODULE h = GetModuleHandle("pydetour.pyd");
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

	PyObject* pyfile = PyFile_FromString(fnbuf,"r"); //new ref




	if (PyErr_Occurred()) { 
		PyErr_Print(); 
		OutputDebugString("pyerror occured reading file");
	}
	if(pyfile==NULL){
		OutputDebugString("pyfile is null");
		Py_XDECREF(pyfile);
		return 0;
	}

	FILE* f = PyFile_AsFile(pyfile); //no ref

	if (PyErr_Occurred()) {
		//TODO: bail
		PyErr_Clear();
	}

	PyRun_File(f,fnbuf, Py_file_input, pyGlobals, pyLocals);

	if (PyErr_Occurred()) {
		
		if (debugging) {
			PyObject *a, *b, *c;
			PyErr_Fetch(&a, &b, &c);
			PModule traceback = PModule::importModule("traceback", pyGlobals, pyLocals);
			PObject traceback_print = traceback.getAttr("print_tb");
			traceback_print.call(c, NULL);
			printf("\n\nBreaking into debugger...\n");
			PModule dbg = PModule::importModule("pdb", pyGlobals, pyLocals);
			PObject dbg_run = dbg.getAttr("post_mortem");
			dbg_run.call(c, NULL);
			Py_XDECREF(a);
			Py_XDECREF(b);
			Py_XDECREF(c);
		} else {
			PyErr_Clear();
		}
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

static char python_path[MAX_PATH];

void Python_Initialize() {

#ifdef _DEBUG
	//Here we set up PythonHome to our debugging version a few directories up
	HMODULE pyhandle = GetModuleHandle("python26_d.dll");

	char pythonhome[MAX_PATH];
	ZeroMemory(python_path, sizeof(python_path));
	ZeroMemory(python_path, sizeof(pythonhome));

	GetModuleFileName(pyhandle, python_path, MAX_PATH-50);

	for(int i = MAX_PATH - 1; i > 0; i--) {
		char* cur = (char*)python_path + i;
		if (*cur == '\\') {
			break;
		}
		*cur = '\0';
	}

	memcpy(pythonhome, "PYTHONHOME=", 12);
	strcat_s(pythonhome, python_path);
	pythonhome[strlen(pythonhome)-1] = 0;
	_putenv(pythonhome);

#else
	//Here we used the installed version of Python at C:\Python26 (we assume)
	//HMODULE pyhandle = GetModuleHandle("python26.dll");
#endif
	Py_Initialize();
	if (!Py_IsInitialized()) {
		OutputDebugString("Python could not be initialized\n");
	}
	PyEval_InitThreads();


	PModule mainmod = PModule::getModule("__main__");
	PDict d = PyModule_GetDict(mainmod); //borrowed ref

	pyGlobals = d;
	pyLocals = d;

	//PyRun_SimpleString(tempbuf);

	//These three do import gdetour
	//initgdetour();
	//PModule gd = PModule::importModule("gdetour", pyGlobals, pyLocals);
	//mainmod.AddObject("gdetour", gd);


}
void Python_Unload() {
	P_GIL gil;



	pyGlobals = NULL;
	pyLocals = NULL;

	printf("python about to unload\n");

	Py_Finalize();

	printf("python unloaded\n");
}
