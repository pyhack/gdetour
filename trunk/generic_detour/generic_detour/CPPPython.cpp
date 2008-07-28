#include "stdafx.h"
#include "CPPPython.h"

namespace CPPPython {
	CPPPythonException::CPPPythonException(char* err) {
		if (err == NULL) {
			this->errmsg = "";
			return;
		}
		size_t l = strlen(err) + 1;
		this->errmsg = new char[l];
		memset(this->errmsg, 0, l);
		strcpy_s(this->errmsg, l, err);
		OutputDebugString(this->errmsg);
		MessageBoxA(0, this->errmsg, "CPPPython Exception", 0);
		__asm { INT 3 }
	}
	CPPPythonException::~CPPPythonException() {
		if (this->errmsg != NULL) {
			delete[] this->errmsg;
		}
	}
	CPPPythonException::operator const char *() {
		return this->errmsg;
	}
	NULLPyObjectException::NULLPyObjectException() {
		CPPPythonException::CPPPythonException("NULL PyObject pointer exception");
	}

	P_GIL::P_GIL() {
		this->state = PyGILState_Ensure();
	}
	P_GIL::~P_GIL() {
		if (this->state == NULL) {
			return;
		}
		PyGILState_Release(state);
	}

	PObject::PObject() {
		this->del_on_destruct = true;
		this->myObject = NULL;
	}
	PObject::PObject(PyObject* obj, bool stealReferance) {
		this->del_on_destruct = true;
		if (!stealReferance && obj) {
			Py_INCREF(obj); //This is a borrowed ref, so we should take our own ref
		}
		this->myObject = obj;
	}
	PObject::PObject(const PObject& old) {
		//copy constructor
		this->del_on_destruct = true; 
		Py_INCREF(old.myObject); //This new class always copies the ref
		this->myObject = old.myObject;
	}
	PObject::~PObject() {
		if (this->del_on_destruct) {
			Py_XDECREF(this->myObject);
		}
	}
	PObject& PObject::operator =(const PObject& p) {
		if (this == &p) {
			return *this; //Same object
		}
		this->del_on_destruct = true;
		this->myObject = p.myObject;
		if (p.del_on_destruct && p.myObject) {
			//take ownership here, the old object will die and drop it's ref
			Py_INCREF(p.myObject);
		} else {
			//don't addref it, the old is the lifetime manager
			//XXX: is this true? should we addref it?
		}
		return *this;
	}
	PObject::operator bool() {
		return (this->myObject != NULL);
	}
	PObject::operator PyObject*() {
		if (this->myObject == NULL) {
			throw new CPPPythonException("Implicit conversion to a NULL PyObject prevented");
		}
		return this->myObject;
	}
	PyObject* PObject::getObject() {
		return this->myObject;
	}

	//-----------
	bool PObject::hasAttr(PyObject* attr) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (::PyObject_HasAttr(this->myObject, attr) == 0) {
			return false;
		}
		return true;
	}
	bool PObject::hasAttr(char* attr) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (::PyObject_HasAttrString(this->myObject, attr) == 0) {
			return false;
		}
		return true;
	}
	bool PObject::isCallable() {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (::PyCallable_Check(this->myObject) == 0) {
			return false;
		}
		return true;
	}
	PObject PObject::PObject_Call(PyObject* args, ...) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		//A null argument MUST BE PASSED at the end!
		PyObject* ret = PyObject_CallFunctionObjArgs(this->myObject, args);
		return PObject(ret, true);
	}

//--------------------
//--------------------
//--------------------
	PNumber::PNumber(PyObject* obj, bool stealReferance) {
		if (!obj) { this->myObject = NULL; return; }
		if (PyNumber_Check(obj) == 0) {
			throw new CPPPythonException("PNumber objects must have valid numbers to wrap");
		}
		if (!stealReferance) {
			Py_INCREF(obj);
		}
		this->myObject = obj;
	}
	PNumber::PNumber(long num) {
		this->myObject = PyInt_FromLong(num); //returns new ref
	}
//---------------------
	PBool::PBool(PyObject* obj, bool stealReferance) {
		if (!obj) { this->myObject = NULL; return; }
		if (PyBool_Check(obj) == 0) {
			throw new CPPPythonException("PBool objects must wrap real Python Booleans.");
		}
		if (!stealReferance) {
			Py_INCREF(obj);
		}
		this->myObject = obj;
	}
	PBool PBool::getTrue() {
		PBool x = (Py_True);
		return x;
	}
	PBool PBool::getFalse() {
		PBool x = (Py_True);
		return x;
	}
	PString::PString(PyObject* obj, bool stealReferance) {
		if (!obj) { this->myObject = NULL; return; }
		if (PyString_Check(obj) == 0) {
			throw new CPPPythonException("PString objects must have valid strings to wrap");
		}
		if (!stealReferance && obj) {
			Py_INCREF(obj);
		}
		this->myObject = obj;
	}
	PString::PString(const char* obj) {
		if (!obj) { this->myObject = NULL; return; }
		this->myObject = PyString_FromString(obj);
	}
	PModule::PModule(PyObject* obj, bool stealReferance) {
		if (!obj) { this->myObject = NULL; return; }
		if (!obj) {
			throw new CPPPythonException("PModule objects cannot wrap NULL objects");
		}
		if (PyModule_Check(obj) == 0) {
			throw new CPPPythonException("PModule objects must have valid strings to wrap");
		}
		if (!stealReferance && obj) {
			Py_INCREF(obj);
		}
		this->myObject = obj;
	}
	PModule PModule::getNewModule(const char *name) {
		PModule ret = PModule(::PyModule_New(name), true);
		return ret;
	}
	PModule PModule::importModule(char* name, PyObject* locals, PyObject* globals) {
		PyObject* ret;
		if (locals == globals == NULL) {
			ret = ::PyImport_ImportModule(name);
			return PModule(ret, true);
		}
		if (::PyMapping_Check(locals) == 0) {
			throw new CPPPythonException("PModule::importModule: locals must be a valid mapping");
		}
		if (::PyMapping_Check(globals) == 0) {
			throw new CPPPythonException("PModule::importModule: globals must be a valid mapping");
		}
		ret = ::PyImport_ImportModuleEx(name, globals, locals, NULL);
		return PModule(ret, true);
	}
	bool PModule::AddConstant(const char *name, long value) {
		if (!this->myObject) {
			throw new NULLPyObjectException();
		}
		if (::PyModule_AddIntConstant(this->myObject, name, value) == -1) {
			return false;
		}
		return true;
	}
	bool PModule::AddConstant(const char *name, const char* value) {
		if (!this->myObject) {
			throw new NULLPyObjectException();
		}
		if (::PyModule_AddStringConstant(this->myObject, name, value) == -1) {
			return false;
		}
		return true;
	}
	bool PModule::AddObject(const char *name, PyObject *value) {
		if (!this->myObject) {
			throw new NULLPyObjectException();
		}
		Py_INCREF(value); //will steal a ref
		if (::PyModule_AddObject(this->myObject, name, value) == -1) {
			return false;
		}
		return true;
	}
	PMapping::PMapping(PyObject* obj, bool stealReferance) {
		if (!obj) { this->myObject = NULL; return; }
		if (PyMapping_Check(obj) == 0) {
			throw new CPPPythonException("PMapping objects must have valid mappings to wrap");
		}
		if (!stealReferance && obj) {
			Py_INCREF(obj);
		}
		this->myObject = obj;
	}
	bool PMapping::HasKey(char* key) {
		if (!this->myObject) {
			throw new NULLPyObjectException();
		}
		if (::PyMapping_HasKeyString(this->myObject, key) == 0) {
			return false;
		}
		return true;
	}
	bool PMapping::HasKey(PyObject* key) {
		if (!this->myObject) {
			throw new NULLPyObjectException();
		}
		if (::PyMapping_HasKey(this->myObject, key) == 0) {
			return false;
		}
		return true;
	}
	bool PMapping::DelItem(char* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (PyMapping_DelItemString(this->myObject, key) == -1) {
			return false;
		}
		return true;
	}
	bool PMapping::DelItem(PyObject* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (PyMapping_DelItem(this->myObject, key) == -1) {
			return false;
		}
		return true;
	}
	PObject PMapping::GetItem(char* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		PyObject* ret = ::PyMapping_GetItemString(this->myObject, key);
		return PObject(ret, true);
	}
	bool PMapping::SetItem(char *key, PyObject *value) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (::PyMapping_SetItemString(this->myObject, key, value) == -1) {
			return false;
		}
		return true;
	}

	PDict PDict::getNewDict() {
		return PDict(PyDict_New(), true);
	}
	PDict::PDict(PyObject* obj, bool stealReferance) {
		if (!obj) { this->myObject = NULL; return; }
		if (PyDict_Check(obj) == 0) {
			throw new CPPPythonException("PDict objects must have valid mappings to wrap");
		}
		if (!stealReferance) {
			Py_INCREF(obj);
		}
		this->myObject = obj;
	}
	bool PDict::Contains(PyObject* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (::PyDict_Contains(this->myObject, key) == 1) {
			return true;
		}
		return false;
	}
	bool PDict::Contains(char* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		PString str = PString(key);
		return this->Contains(str);
	}
	bool PDict::DelItem(char* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (PyDict_DelItemString(this->myObject, key) == -1) {
			return false;
		}
		return true;
	}
	bool PDict::DelItem(PyObject* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (PyDict_DelItem(this->myObject, key) == -1) {
			return false;
		}
		return true;
	}
	PObject PDict::GetItem(char* key) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		PyObject* ret = ::PyDict_GetItemString(this->myObject, key);
		return PObject(ret, true);
	}
	bool PDict::SetItem(char *key, PyObject *value) {
		if (!this->myObject) { throw new NULLPyObjectException(); }
		if (::PyDict_SetItemString(this->myObject, key, value) == -1) {
			return false;
		}
		return true;
	}

};