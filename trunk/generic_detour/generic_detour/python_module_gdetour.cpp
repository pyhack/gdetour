#include "stdafx.h"
#include "python_module_gdetour.h"
#include "python_funcs.h"
#include "python_type_registers.h"
#include "python_type_detourconfig.h"
#include "structmember.h"


#include "generic_detour.h"

PyObject* Detour_Exception;
PyObject* Detour_Exception_AlreadyInitilized;
PyObject* Detour_Exception_WindowsException;
PyObject* Detour_Exception_AccessViolation;

PyObject* detour_loadPythonFile(PyObject* self, PyObject* args) {
	char* filename;
	if (!PyArg_ParseTuple(args, "s", &filename)) {
		return NULL;
	}
	int ret = run_python_file(filename);
	if (ret == 0) {
		return PyErr_Format(PyExc_IOError, "%s is an invalid filename", filename);
	}
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
	__try {
		VirtualProtect(address, bytenum, PAGE_EXECUTE_READWRITE, &oldProt);
		memcpy(address, bytes, bytenum);
		VirtualProtect(address, bytenum, oldProt, &dummy);
	} __except(1) {
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			return PyErr_Format(Detour_Exception_AccessViolation, "%p is an invalid memory address (Access Violation)", address);
		}
		return PyErr_Format(Detour_Exception_WindowsException, "%p is an invalid memory address (Windows Exception %d)", address, GetExceptionCode());
	}

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
	__try {
		VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProt);
		memcpy(address, &bytes, 1);
		VirtualProtect(address, 1, oldProt, &dummy);
	} __except(1) {
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			return PyErr_Format(Detour_Exception_AccessViolation, "%p is an invalid memory address (Access Violation)", address);
		}
		return PyErr_Format(Detour_Exception_WindowsException, "%p is an invalid memory address (Windows Exception %d)", address, GetExceptionCode());
	}

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
	__try {
		VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &oldProt);
		memcpy(address, &bytes, 4);
		VirtualProtect(address, 4, oldProt, &dummy);
	} __except(1) {
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			return PyErr_Format(Detour_Exception_AccessViolation, "%p is an invalid memory address (Access Violation)", address);
		}
		return PyErr_Format(Detour_Exception_WindowsException, "%p is an invalid memory address (Windows Exception %d)", address, GetExceptionCode());
	}
	return Py_BuildValue("i", true);
}
PyObject* detour_ReadASCIIZ(PyObject* self, PyObject* args) {
	char* address;

	if (!PyArg_ParseTuple(args, "i", &address)) {
		return NULL;
	}
	__try {
		if (address == NULL) {
			int x = *(int*)0x0; //Python won't cause the access violation, it'll return None instead
		}
		return Py_BuildValue("s", address);
	} __except(1) {
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			return PyErr_Format(Detour_Exception_AccessViolation, "%p is an invalid memory address (Access Violation)", address);
		}
		return PyErr_Format(Detour_Exception_WindowsException, "%p is an invalid memory address (Windows Exception %d)", address, GetExceptionCode());
	}
}
PyObject* detour_ReadMemory(PyObject* self, PyObject* args) {
	char* address;
	int bytes;

	if (!PyArg_ParseTuple(args, "ii", &address, &bytes)) {
		return NULL;
	}
	__try {
		if (address == NULL) {
			int x = *(int*)0x0; //Python won't cause the access violation, it'll return None instead
		}
		return Py_BuildValue("s#", address, bytes);
	} __except(1) {
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			return PyErr_Format(Detour_Exception_AccessViolation, "%p is an invalid memory address (Access Violation)", address);
		}
		return PyErr_Format(Detour_Exception_WindowsException, "%p is an invalid memory address (Windows Exception %d)", address, GetExceptionCode());
	}

}
PyObject* detour_ReadByte(PyObject* self, PyObject* args) {
	char* address;

	if (!PyArg_ParseTuple(args, "i", &address)) {
		return NULL;
	}
	__try {
		if (address == NULL) {
			int x = *(int*)0x0; //Python won't cause the access violation, it'll return None instead
		}
		return Py_BuildValue("s#", address, 1);
	} __except(1) {
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			return PyErr_Format(Detour_Exception_AccessViolation, "%p is an invalid memory address (Access Violation)", address);
		}
		return PyErr_Format(Detour_Exception_WindowsException, "%p is an invalid memory address (Windows Exception %d)", address, GetExceptionCode());
	}
}
PyObject* detour_ReadDWORD(PyObject* self, PyObject* args) {
	int* address;

	if (!PyArg_ParseTuple(args, "i", &address)) {
		return NULL;
	}
	__try {
		return Py_BuildValue("i", *address);
	} __except(1) {
		if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
			return PyErr_Format(Detour_Exception_AccessViolation, "%p is an invalid memory address (Access Violation)", address);
		}
		return PyErr_Format(Detour_Exception_WindowsException, "%p is an invalid memory address (Windows Exception %d)", address, GetExceptionCode());
	}
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
PyObject* detour_setRegisters(PyObject* self, PyObject* args) {

	BYTE* address;
	REGISTERS registers;
	int flags;
	int caller;
	if (!PyArg_ParseTuple(args, "i(iiiiiiii)ii", 
			&address, 
			&registers.eax, 
			&registers.ecx, 
			&registers.edx, 
			&registers.ebx,
			&registers.esp, 
			&registers.ebp, 
			&registers.esi, 
			&registers.edi, 
			&flags, 
			&caller)) {
		return NULL;
	}
	GDetour* dt = getDetour(address);
	if (dt == NULL) {
		return PyErr_Format(PyExc_LookupError, "%p is an invalid detoured address", address);
	}

	GDetour &d = *dt;

	d.live_settings.registers.eax = registers.eax;
	d.live_settings.registers.ecx = registers.ecx;
	d.live_settings.registers.edx = registers.edx;
	d.live_settings.registers.ebx = registers.ebx;
	d.live_settings.registers.esp = registers.esp;
	d.live_settings.registers.ebp = registers.ebp;
	d.live_settings.registers.esi = registers.esi;
	d.live_settings.registers.edi = registers.edi;
	d.live_settings.flags = flags;
	//d.live_settings.caller_ret = caller;


	return Py_BuildValue("i", true);
}
PyObject* detour_createDetour(PyObject* self, PyObject* args) {
	BYTE* address;
	int overwrite_length;
	int bytes_to_pop;
	int type = 0;

	if (!PyArg_ParseTuple(args, "iiii", &address, &overwrite_length, &bytes_to_pop, &type)) {
		return NULL;
	}

	bool ret = add_detour(address, overwrite_length, bytes_to_pop, type);
	
	return Py_BuildValue("i", ret);

}
PyObject* detour_removeDetour(PyObject* self, PyObject* args) {
	BYTE* address;
	if (!PyArg_ParseTuple(args, "i", &address)) {
		return NULL;
	}
	detour_list_type::iterator dl = detours.find(address);
	if (dl == detours.end()) {
		return PyErr_Format(PyExc_LookupError, "Detour not found at %p", address);
	}

	bool r = remove_detour(address);
	if (r == false) {
		return PyErr_Format(Detour_Exception, "Unable to remove detour at %p", address);
	}
	return Py_BuildValue("i", true);
}
PyObject* detour_getDetourSettings(PyObject* self, PyObject* args) {
	
	
	BYTE* address;
	if (!PyArg_ParseTuple(args, "i", &address)) {
		return NULL;
	}

	GDetour* d = getDetour(address);
	if (d == NULL) {
		return PyErr_Format(PyExc_LookupError, "%p is an invalid detoured address", address);
	}
	return Py_BuildValue("(ii)",
		d->gateway_opt.bytes_to_pop_on_ret,
		d->gateway_opt.call_original_on_return
	);
}
PyObject* detour_setDetourSettings(PyObject* self, PyObject* args) {
	DETOUR_GATEWAY_OPTIONS n;
	BYTE* address;
	if (!PyArg_ParseTuple(args, "i(ii)", &address, 
			&n.bytes_to_pop_on_ret,
			&n.call_original_on_return)
			) {
		return NULL;
	}
	GDetour* d = getDetour(address);
	if (d == NULL) {
		return PyErr_Format(PyExc_LookupError, "%p is an invalid detoured address", address);
	}
	d->gateway_opt.bytes_to_pop_on_ret = n.bytes_to_pop_on_ret;
	d->gateway_opt.call_original_on_return = n.call_original_on_return;

	return Py_BuildValue("i", true);
}
////////////////////////////////////////////////////////////////////
static PyMethodDef detour_funcs[] = {
	{"readASCIIZ", (PyCFunction)detour_ReadASCIIZ, METH_VARARGS, "Reads memory, None on NULL pointer"},
	{"read", (PyCFunction)detour_ReadMemory, METH_VARARGS, "Reads memory"},
	{"readByte", (PyCFunction)detour_ReadByte, METH_VARARGS, "Reads memory"},
	{"readDWORD", (PyCFunction)detour_ReadDWORD, METH_VARARGS, "Reads memory"},
	{"write", (PyCFunction)detour_WriteMemory, METH_VARARGS, "Writes memory"},
	{"writeByte", (PyCFunction)detour_WriteByte, METH_VARARGS, "Writes memory"},
	{"writeDWORD", (PyCFunction)detour_WriteDWORD, METH_VARARGS, "Writes memory"},

	{"callback", (PyCFunction)detour_callback, METH_VARARGS, "Default callback function"},
	{"setRegisters", (PyCFunction)detour_setRegisters, METH_VARARGS, "Sets registers"},
	
	{"loadPythonFile", (PyCFunction)detour_loadPythonFile, METH_VARARGS, "Loads and executes a python file"},

	{"createDetour", (PyCFunction)detour_createDetour, METH_VARARGS, "Creates a detour"},
	{"removeDetour", (PyCFunction)detour_removeDetour, METH_VARARGS, "Removes a detour"},

	{"getDetourSettings", (PyCFunction)detour_getDetourSettings, METH_VARARGS, "Retreives a detour's settings"},
	{"setDetourSettings", (PyCFunction)detour_setDetourSettings, METH_VARARGS, "Modifies a detour's settings"},
	{NULL, NULL, 0, NULL}
};



PyMODINIT_FUNC initgdetour() {
	//this function is automagically called on import

    /* Create the module and add the functions */
    PyObject* m = Py_InitModule3("gdetour", detour_funcs, "Generic Process Detour");

	Detour_Exception = PyErr_NewException("gdetour.DetourException", NULL, NULL);
	Detour_Exception_AlreadyInitilized = PyErr_NewException("gdetour.DetourExceptionAlreadyInitilized", Detour_Exception, NULL);
	Detour_Exception_WindowsException = PyErr_NewException("gdetour.DetourWindowsException", Detour_Exception, NULL);
	Detour_Exception_AccessViolation = PyErr_NewException("gdetour.DetourAccessViolationException", Detour_Exception_WindowsException, NULL);


	Py_INCREF(Detour_Exception);
	Py_INCREF(Detour_Exception_AlreadyInitilized);
	Py_INCREF(Detour_Exception_WindowsException);
	Py_INCREF(Detour_Exception_AccessViolation);

	PyModule_AddObject(m, "DetourException", Detour_Exception);
	PyModule_AddObject(m, "DetourExceptionAlreadyInitilized", Detour_Exception_AlreadyInitilized);
	PyModule_AddObject(m, "DetourWindowsException", Detour_Exception_WindowsException);
	PyModule_AddObject(m, "DetourAccessViolationException", Detour_Exception_AccessViolation);

	add_module_type_registers(m);
	add_module_type_detourconfig(m);

}














void CallPythonDetour(GDetour* d) {
		/* ensure we hold the lock */

	PyGILState_STATE state = Python_GrabGIL();

	PyObject* m = PyImport_AddModule("gdetour");

	if (m == NULL) {
		OutputDebugString("Can't call detour! module not in locals!\n");
		goto pastPython;
	}

	PyObject* detour_pyfunc = PyObject_GetAttrString(m, "callback");

	if (detour_pyfunc == NULL) {
		OutputDebugString("Can't call detour! detour_pyfunc is null!\n");
		goto pastPython;
	}
	OutputDebugString("Calling Function...\n");
	PyObject* ret = PyEval_CallFunction(detour_pyfunc, 
		"i(iiiiiiii)ii", 
		d->live_settings.ret_addr-5,
		d->live_settings.registers.eax,
		d->live_settings.registers.ecx,
		d->live_settings.registers.edx,
		d->live_settings.registers.ebx,
		d->live_settings.registers.esp,
		d->live_settings.registers.ebp,
		d->live_settings.registers.esi,
		d->live_settings.registers.edi,
		d->live_settings.flags,
		d->live_settings.caller_ret - 5
	);
	if (PyErr_Occurred()) { PyErr_Print(); }


	OutputDebugString("Done.\n");
	Py_XDECREF(detour_pyfunc);
	pastPython:

	/* Restore the state of Python */
	Python_ReleaseGIL(state);

}