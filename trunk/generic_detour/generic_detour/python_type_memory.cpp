#include "stdafx.h"
#include "python_type_memory.h"

#include "structmember.h"

#include <windows.h>
#include "CPPPython.h"
using namespace CPPPython;
typedef struct {
    PyObject_HEAD
	unsigned long rwsize;
} memory;

static void memory_dealloc(memory* self) {
}

static PyObject * memory_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    memory *self;
    self = (memory *)type->tp_alloc(type, 0);
    if (self != NULL) {
		self->rwsize = 1;
	}
    return (PyObject *)self;
}

static int memory_init(memory *self, PyObject *args, PyObject *kwds) {
	//PyArg_ParseTuple(args, "l", &self->rwsize);
	self->rwsize = 1;
    return 0;
}

static PyMemberDef memory_members[] = {
	//Comment to hide impl
	//{"rwsize", T_ULONG, offsetof(memory, rwsize), 0, "rwsize"},
    {NULL}  /* Sentinel */
};

static PyGetSetDef memory_getseters[] = {
    {NULL}  /* Sentinel */
};

static PyObject* memory_repr(memory* self) {
	return PyString_FromString("<memory object>");
}

static PyMethodDef memory_methods[] = {
    {NULL}  /* Sentinel */
};

static PyObject* memory_getItem(memory* self, Py_ssize_t i) {
	if (IsBadReadPtr((LPVOID)i, self->rwsize)) {
		goto err_ptr;
	}
	return PyString_FromStringAndSize((char*)i, self->rwsize);
err_ptr:
	return PyErr_Format(PyExc_IndexError, "%p is an invalid (nonreadable) memory address", i);
}
static PyObject* memory_getSlice(memory* self, Py_ssize_t i, Py_ssize_t len) {
	if (IsBadReadPtr((LPVOID)i, len)) {
		goto err_ptr;
	}
	return PyString_FromStringAndSize((char*)i, len);
err_ptr:
	return PyErr_Format(PyExc_IndexError, "%p is an invalid (nonreadable) memory address", i);
}
static int memory_setItem(memory* self, Py_ssize_t i, PyObject *v) {
	unsigned long objsize = self->rwsize;
	unsigned long objval = 0;

	if (PyString_Check(v) == FALSE) {
		PyErr_Format(PyExc_TypeError, "Item must be a sequence of bytes");
		return -1;
	}
	PString s = PString(v);

	objsize = s.getLength();
	char* st = PyString_AsString(v);
	
	if (IsBadWritePtr((LPVOID)i, objsize)) {
		goto err_ptr;
	}
	memcpy((void*)i, st, objsize);
	return 0;
err_ptr:
	PyErr_Format(PyExc_IndexError, "%p is an invalid (nonwriteable) memory address", i);
	return -1;
err_args:
	//PyErr_Format(PyExc_TypeError, "You must use an integer from 0 to 255 or a tuple (bytes, value from 0 to 2^(8*bytes)-1)", i);
	return -1;

}

static PySequenceMethods memory_seq_methods[] = {
	0, /*lenfunc sq_length;*/
	0, /*binaryfunc sq_concat;*/
	0, /*ssizeargfunc sq_repeat;*/
	(ssizeargfunc) memory_getItem, //ssizeargfunc /*sq_item;*/
	(ssizessizeargfunc) memory_getSlice, //ssizessizeargfunc /*sq_slice;*/
	(ssizeobjargproc) memory_setItem, //ssizeobjargproc /*sq_ass_item;*/
	0, /*ssizessizeobjargproc sq_ass_slice;*/
	0, /*objobjproc sq_contains;*/
	/* Added in release 2.0 */
	0, /*binaryfunc sq_inplace_concat;*/
	0, /*ssizeargfunc sq_inplace_repeat;*/
};
static PyTypeObject memoryType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "gdetour.memory",             /*tp_name*/
    sizeof(memory),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)memory_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    (reprfunc)memory_repr,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    memory_seq_methods,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Instances of memory allow indexing into the memory of the current process. Use memory[0x1234] to return the single byte at that location. Use memory[0x1234] = 23 to set the single byte. Alternatively, to set larger sized values, use memory[0x1234] = (4, 23456) (That is, set it to a tuple of (bytesize, value). Note that bytesize can be at most 4.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    memory_methods,             /* tp_methods */
    memory_members,             /* tp_members */
    memory_getseters,           /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)memory_init,      /* tp_init */
    0,                         /* tp_alloc */
    memory_new,                 /* tp_new */
};

bool add_module_type_memory(PyObject* m) {
	if (PyType_Ready(&memoryType) < 0) {
        return false;
	}

	if (m == NULL){ 
		return false;
	}

    Py_INCREF(&memoryType);
	PyModule_AddObject(m, "memoryType", (PyObject *)&memoryType); //steals ref

	PObject mT((PyObject*)&memoryType);

	PNumber pn4 = 4;
	PNumber pn1 = 1;

	//PyModule_AddObject(m, "mem_dword", mT.call(pn4, NULL)); //steals ref
	PyModule_AddObject(m, "memory", mT.call(NULL)); //steals ref

	return true;
}