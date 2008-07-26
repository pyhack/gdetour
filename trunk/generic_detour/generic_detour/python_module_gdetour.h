#include <map>
#include "generic_detour.h"

void CallPythonDetour(std::map<BYTE*, GDetour::DETOUR_PARAMS>::iterator &dl, DWORD ret_Addr, DWORD caller_ret);