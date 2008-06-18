// generic_detour.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "generic_detour.h"
#include "python_funcs.h"

namespace GDetour {
	std::map<BYTE*, DETOUR_PARAMS> detour_list;

GENERIC_DETOUR_API void initialize() {
	//detour_list = new std::map<BYTE*, BYTE*>;
}
GENERIC_DETOUR_API bool add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, int type) {
	//type is unused. provided for forward compat.
	if (overwrite_length < 5) {
		return false; //need 5 bytes at minimum
	}
	DETOUR_PARAMS dl = detour_list[address];
	dl.address = address;
	dl.bytes_to_pop_on_ret = bytes_to_pop;
	if (overwrite_length > sizeof(dl.original_code)-5) {
		detour_list.erase(address);
		return false; //need 5 bytes in backed up code
	}
	dl.original_code_len = overwrite_length;
	for(int i = 0; i < sizeof(dl.original_code); i++) {
		dl.original_code[i] = 0x90; //NOP
	}

	InitializeCriticalSection(&dl.my_critical_section);

	
	memcpy(dl.original_code, address, overwrite_length);
	dl.original_code[overwrite_length] = 0xE9; //JMP
	DWORD cont = CalculateRelativeJMP(dl.original_code[overwrite_length], (DWORD) address + overwrite_length);
	memcpy(&dl.original_code[overwrite_length+1], (void*) cont, 4);

	DWORD oldProt = 0;
	DWORD dummy = 0;

	VirtualProtect(address, 7, PAGE_EXECUTE_READWRITE, &oldProt);

	BYTE* addr;
	DWORD* jaddr;
	addr = (BYTE*) address;
	jaddr = (DWORD*) (addr + 1);
	addr[0] = 0xE8; //A Call!
	addr[0] = 0xE8; //A Call!


	jaddr[0] = CalculateRelativeJMP((DWORD) addr, (DWORD) &detour_call_dest);
	addr[5] = 90;
	addr[6] = 90;
	VirtualProtect(address, 7, oldProt, &dummy);

	detour_list[address] = dl;
	return true;
};

__declspec(naked) int detour_call_dest() {
/*
When we come in, we look like:
ESP:   [address of function we meant to call, plus 5]
ESP+4: [return address of the caller]
ESP+8: [arg 1]
ESP+C: [arg 2]
*/
	__asm {
		PUSHFD
		PUSHAD
		CALL detour_c_call_dest
		PUSH EAX //store number of bytes to pop right above everything
		ADD ESP, 4 //skip what we just pushed
		POPAD
		POPFD
		ADD ESP, [ESP-40] //move stack appropriately
		RET
	}
}
int detour_c_call_dest(
	DWORD register_1, 
	DWORD register_2, 
	DWORD register_3, 
	DWORD register_4, 
	DWORD register_5, 
	DWORD register_6, 
	DWORD register_7, 
	DWORD register_8,  
	DWORD flags, 
	DWORD ret_addr, 
	DWORD caller_ret, 
	DWORD param_zero) {

	char tempstring[512];


	std::map<BYTE*, DETOUR_PARAMS>::iterator dl = detour_list.find((BYTE*)(ret_addr-5));
	if (dl == detour_list.end()) {
		sprintf_s(tempstring, sizeof(tempstring), "Called detour from function 0x%x and could not find a registered handler. Crash is likely.", (ret_addr-5));
		OutputDebugString(tempstring);
		return 0;
	}

	

	EnterCriticalSection(&dl->second.my_critical_section);

	dl->second.registers[7] = register_1;
	dl->second.registers[6] = register_2;
	dl->second.registers[5] = register_3;
	dl->second.registers[4] = register_4 + 4; //ESP is ignored on POPAD anyway, and its up 4 due to PUSHFD. Lets just correct it for simplicity.
	dl->second.registers[3] = register_5;
	dl->second.registers[2] = register_6;
	dl->second.registers[1] = register_7;
	dl->second.registers[0] = register_8;
	dl->second.flags = flags;
	dl->second.caller_ret = caller_ret;

	dl->second.params = &param_zero;

	sprintf_s(tempstring, sizeof(tempstring),"func: 0x%x, ret: 0x%x,\n R[EAX] = 0x%x,\n R[ECX] = 0x%x,\n R[EDX] = 0x%x,\n R[EBX] = 0x%x,\n R[ESP] = 0x%x,\n R[EBP] = 0x%x,\n R[ESI] = 0x%x,\n R[EDI] = 0x%x,\n flags = 0x%x,\n arg0 = 0x%x,\n arg1 = 0x%x\n",
		ret_addr-5,
		caller_ret,
		dl->second.registers[0], //EAX
		dl->second.registers[1], //ECX
		dl->second.registers[2], //EDX
		dl->second.registers[3], //EBX
		dl->second.registers[4], //ESP
		dl->second.registers[5], //EBP
		dl->second.registers[6], //ESI
		dl->second.registers[7], //EDI
		dl->second.flags,
		dl->second.params[0],
		dl->second.params[1]
		);
	//MessageBox(0,tempstring, "",0);
	OutputDebugString(tempstring);

	OutputDebugString("Trying to communicate with Python...\n");

	::PyGILState_STATE gstate = PyGILState_Ensure();

	//PyObject* m = PyDict_GetItemString(myPyLocals, "detour");
	PyObject* m = PyImport_AddModule("detour");

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
		ret_addr-5,
		dl->second.registers[0], //EAX
		dl->second.registers[1], //ECX
		dl->second.registers[2], //EDX
		dl->second.registers[3], //EBX
		dl->second.registers[4], //ESP
		dl->second.registers[5], //EBP
		dl->second.registers[6], //ESI
		dl->second.registers[7], //EDI
		dl->second.flags,
		caller_ret - 5
	);
	if (PyErr_Occurred()) { PyErr_Print(); }
	OutputDebugString("Done.\n");
	Py_XDECREF(detour_pyfunc);
	pastPython:

	::PyGILState_Release(gstate);
	
	
	LeaveCriticalSection(&dl->second.my_critical_section);

	return dl->second.bytes_to_pop_on_ret;
}

}


GENERIC_DETOUR_API int test_detour_func(int count) {
	MessageBox(0, "I'm the real test_detour_func!", "Caption", 0);
	if (count < 2) {
		test_detour_func(count+1);
	}
	return count;
}

GENERIC_DETOUR_API int stolen_detour_func(DWORD registers[8], DWORD flags, DWORD retaddr, DWORD params[]) {
	MessageBox(0, "I'm the fake test_detour_func!", "Caption", 0);
	return 42;
}

GENERIC_DETOUR_API int add_test_detour() {
	return GDetour::add_detour((BYTE*) &test_detour_func, 5, 4);
}














DWORD CalculateRelativeJMP(DWORD jmp_address, DWORD jmp_destination, int jmp_operand_length) {
	//jmp_operand_length is the number of BYTEs the JMP opcode takes up.
	//JMP == 1 BYTE
	//JE == 2 BYTEs, etc
	//this function assumes that you will be specifying the absolute offset as a DWORD (4 BYTEs)
	if (jmp_address > jmp_destination) {
		//2s complement for a negitive JMP
		DWORD a = ((jmp_address + jmp_operand_length + 4) - jmp_destination);
		a = ~a;
		a = a + 1;
		return a;
	} else {
		return (jmp_destination - (jmp_address + jmp_operand_length + 4));
	}

}
DWORD CalculateAbsoluteJMP(DWORD jmp_address, DWORD jmp_reldestination, int jmp_operand_length) {
	//jmp_operand_length is the number of BYTEs the JMP opcode takes up.
	//JMP == 1 BYTE
	//JE == 2 BYTEs, etc
	//this function assumes that you will be specifying the relative offset as a DWORD (4 BYTEs)
	if ((signed) jmp_reldestination < 0) {
		//undo 2s complement for a negitive JMP
		DWORD a = jmp_reldestination;
		a = a - 1;
		a = ~a;
		DWORD b = ((jmp_address + jmp_operand_length + 4) - a);
		//g_console.sprintf("Calculate Absolute JMP - From %X, to rel %X (%i), is abs %X.\n", jmp_address, (signed) jmp_reldestination, a, b);
		return b;
	} else {
		//g_console.sprintf("Calculate Absolute JMP - From %X, to rel %X (%i), is abs %X.\n", jmp_address, jmp_reldestination, jmp_reldestination, (jmp_address + jmp_reldestination));
		return ((jmp_address + jmp_operand_length + 4) + jmp_reldestination);
	}
}