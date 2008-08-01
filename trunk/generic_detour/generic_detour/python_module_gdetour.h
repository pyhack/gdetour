#include <map>
#include "generic_detour.h"

void CallPythonDetour(GDetour*);

PyMODINIT_FUNC initgdetour();

extern PyObject* Detour_Exception;
extern PyObject* Detour_Exception_AlreadyInitilized;
extern PyObject* Detour_Exception_WindowsException;
extern PyObject* Detour_Exception_AccessViolation;