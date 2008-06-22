#include "stdafx.h"
#include "python_funcs.h"

PyObject* myPyGlobals;
PyObject* myPyLocals;

GENERIC_DETOUR_API PyObject* run_python_string(char* pycode)
{
	PyGILState_STATE state = Python_GrabGIL();
	PyObject* ret = PyRun_String(pycode, Py_single_input, myPyGlobals, myPyLocals);	
	if (PyErr_Occurred()) { PyErr_Print(); }
	Python_ReleaseGIL(state);
	return ret;
}
GENERIC_DETOUR_API int run_python_file(char* filename)
{
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
	if (PyErr_Occurred()) { PyErr_Print(); }
	if(pyfile==NULL){
		OutputDebugString("pyfile is null");
		return 0;
	}

	FILE* f = PyFile_AsFile(pyfile);
	PyRun_File(f,fnbuf, Py_file_input, myPyGlobals, myPyLocals);

	if (PyErr_Occurred()) { PyErr_Print(); }

	Py_DECREF(pyfile);
	Python_ReleaseGIL(state);
	return 1;
}
PyObject* detour_loadPythonFile(PyObject* self, PyObject* args) {
	char* filename;
	if (!PyArg_ParseTuple(args, "s", &filename)) {
		return NULL;
	}
	int ret = run_python_file(filename);

	return Py_BuildValue("i", ret);
}
PyObject* detour_WriteMemory(PyObject* self, PyObject* args) {
	char* address;
	char* bytes;
	int bytenum;

	if (!PyArg_ParseTuple(args, "is#", &address, &bytes, &bytenum)) {
		return NULL;
	}
	DWORD oldProt;
	DWORD dummy;
	VirtualProtect(address, bytenum, PAGE_EXECUTE_READWRITE, &oldProt);
	memcpy(address, bytes, bytenum);
	VirtualProtect(address, bytenum, oldProt, &dummy);

	return Py_BuildValue("i", true);
}
PyObject* detour_WriteByte(PyObject* self, PyObject* args) {
	char* address;
	char bytes;
	if (!PyArg_ParseTuple(args, "ic", &address, &bytes)) {
		return NULL;
	}
	DWORD oldProt;
	DWORD dummy;
	VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProt);
	memcpy(address, &bytes, 1);
	VirtualProtect(address, 1, oldProt, &dummy);

	return Py_BuildValue("i", true);
}
PyObject* detour_WriteDWORD(PyObject* self, PyObject* args) {
	char* address;
	DWORD bytes;
	if (!PyArg_ParseTuple(args, "ii", &address, &bytes)) {
		return NULL;
	}
	DWORD oldProt;
	DWORD dummy;
	VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &oldProt);
	memcpy(address, &bytes, 4);
	VirtualProtect(address, 4, oldProt, &dummy);

	return Py_BuildValue("i", true);
}
PyObject* detour_ReadMemory(PyObject* self, PyObject* args) {
	char* address;
	int bytes;

	if (!PyArg_ParseTuple(args, "ii", &address, &bytes)) {
		return NULL;
	}
	
	return Py_BuildValue("s#", address, bytes);
}
PyObject* detour_ReadByte(PyObject* self, PyObject* args) {
	char* address;

	if (!PyArg_ParseTuple(args, "i", &address)) {
		return NULL;
	}
	
	return Py_BuildValue("s#", address, 1);
}
PyObject* detour_ReadDWORD(PyObject* self, PyObject* args) {
	char* address;

	if (!PyArg_ParseTuple(args, "i", &address)) {
		return NULL;
	}
	
	return Py_BuildValue("i", address);
}
PyObject* detour_callback(PyObject* self, PyObject* args) {

	char* address;
	int detoured_addr;
	int registers[8];
	int flags;
	int caller;
	if (!PyArg_ParseTuple(args, "i(iiiiiiii)ii", &address, &detoured_addr, &registers[0], &registers[1], &registers[2], &registers[3], &registers[4], &registers[5], &registers[6], &registers[7], &flags, &caller)) {
		return NULL;
	}
	OutputDebugString("Default Python -> C++ callback called\n");
	
	return Py_BuildValue("s", 0);
}

static PyMethodDef detour_funcs[] = {
	{"read", (PyCFunction)detour_ReadMemory, METH_VARARGS, "Reads memory, None on NULL pointer"},
	{"readByte", (PyCFunction)detour_ReadByte, METH_VARARGS, "Reads memory, None on NULL pointer"},
	{"readDWORD", (PyCFunction)detour_ReadDWORD, METH_VARARGS, "Reads memory"},
	{"write", (PyCFunction)detour_WriteMemory, METH_VARARGS, "Writes memory"},
	{"writeByte", (PyCFunction)detour_WriteByte, METH_VARARGS, "Writes memory"},
	{"writeDWORD", (PyCFunction)detour_WriteDWORD, METH_VARARGS, "Writes memory"},
	{"callback", (PyCFunction)detour_callback, METH_VARARGS, "Default callback function"},
	{"loadPythonFile", (PyCFunction)detour_loadPythonFile, METH_VARARGS, "Loads and executes a python file"},
	{NULL, NULL, 0, NULL}
};

void Init_detour() {
    /* Create the module and add the functions */
    PyObject* m = Py_InitModule3("detour", detour_funcs, "Generic Process Detour");
}








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

	Init_detour();

	PyImport_ImportModuleEx("detour", myPyGlobals, myPyLocals, NULL);

	//PyEval_ReleaseLock();
}
void Python_Unload() {
	PyGILState_STATE state = Python_GrabGIL();

	Py_XDECREF(myPyGlobals);
	Py_XDECREF(myPyLocals);
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
