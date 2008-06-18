#include "stdafx.h"
#include "python_funcs.h"

PyObject* myPyGlobals;
PyObject* myPyLocals;


GENERIC_DETOUR_API void run_test() {
	run_python_file("c:\\test.py");
}

GENERIC_DETOUR_API PyObject* run_python_string(char* pycode)
{
	PyObject* ret = PyRun_String(pycode, Py_eval_input, myPyGlobals, myPyLocals);	
	if (PyErr_Occurred()) { PyErr_Print(); }
	return ret;
}
GENERIC_DETOUR_API int run_python_file(char* filename)
{

	PyObject* pyfile = PyFile_FromString(filename,"r");
	if (PyErr_Occurred()) { PyErr_Print(); }
	if(pyfile==NULL){
		OutputDebugString("pyfile is null");
		return 1;
	}

	FILE* f = PyFile_AsFile(pyfile);
	PyRun_File(f,filename, Py_file_input, myPyGlobals, myPyLocals);

	if (PyErr_Occurred()) { PyErr_Print(); }

	Py_DECREF(pyfile);
	
	return 0;
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
	{"callback", (PyCFunction)detour_callback, METH_VARARGS, "Default callback function"},
};

void InitilizePythonFuncs() {
	PyObject* mainmod = PyImport_AddModule("__main__"); //borrowed ref
	PyObject* d = PyModule_GetDict(mainmod); //borrowed red

	Py_INCREF(d);
	Py_INCREF(d);

	myPyGlobals = d; //PyDict_New();
	myPyLocals = d; //PyDict_New();


    /* Create the module and add the functions */
    PyObject* m = Py_InitModule3("detour", detour_funcs, "Generic Process Detour");

	PyImport_ImportModuleEx("detour", myPyGlobals, myPyLocals, NULL);

}

