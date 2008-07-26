// generic_detour.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "generic_detour.h"
#include "python_module_gdetour.h"


detour_list_type detours;


GDetour::GDetour(BYTE* address, int overwrite_length, int bytes_to_pop, int type) {
	//type is unused. provided for forward compat.
	if (overwrite_length < 5) {
		return; //need 5 bytes at minimum for JMP in
	}
	if (overwrite_length > sizeof(this->original_code)-5) {
		return; //need 5 bytes in backed up code for JMP return
	}
	this->Applied = false;
	this->address = address;
	memset(&this->live_settings, 0x0, sizeof(this->live_settings)); //Zero out live settings
	memset((BYTE*)&this->original_code, 0xCC, sizeof(this->original_code)); //fill with breakpoint
	this->original_code_len = overwrite_length;

	this->gateway_opt.bytes_to_pop_on_ret = bytes_to_pop;
	this->gateway_opt.call_original_on_return = false;

	this->gateway_opt.original_code = (BYTE*)&this->original_code;

	InitializeCriticalSection(&this->my_critical_section);

	memcpy(this->original_code, this->address, this->original_code_len);

	this->original_code[this->original_code_len] = 0xE9; //JMP
	DWORD* retJmp = (DWORD*)&this->original_code[this->original_code_len+1];
	*retJmp = CalculateRelativeJMP((DWORD)&this->original_code[this->original_code_len], (DWORD) (this->address + this->original_code_len));


}

bool GDetour::Apply() {
	if (this->Applied) {
		return true;
	}

	DWORD oldProt = 0;
	DWORD dummy = 0;

	VirtualProtect(this->address, this->original_code_len, PAGE_EXECUTE_READWRITE, &oldProt);

	BYTE* addr;
	DWORD* jaddr;
	addr = (BYTE*) this->address;
	jaddr = (DWORD*) (addr + 1);
	addr[0] = 0xE8; //A Call!

	jaddr[0] = CalculateRelativeJMP((DWORD) addr, (DWORD) &detour_call_dest);
	for(DWORD i = 5; i < this->original_code_len; i++) {
		addr[i] = 0x90; //Set extra bytes to NOP
	}
	VirtualProtect(this->address, this->original_code_len, oldProt, &dummy);

	this->Applied = true;
}
bool GDetour::Unapply() {
	if (!this->Applied) {
		return true;
	}
	DWORD oldProt = 0;
	DWORD dummy = 0;
	VirtualProtect(this->address, this->original_code_len, PAGE_EXECUTE_READWRITE, &oldProt);
	memcpy(this->address, &this->original_code, this->original_code_len);
	VirtualProtect(this->address, this->original_code_len, oldProt, &dummy);
	this->Applied = false;
}
GDetour::~GDetour() {
	if (this->Applied) {
		this->Unapply();
	}
}



GENERIC_DETOUR_API bool remove_detour(BYTE* address) {
	detour_list_type::iterator dl = detours.find(address);
	if (dl == detours.end()) {
		return false;
	}
	int overwrite_length = dl->second.original_code_len;
	DWORD oldProt = 0;
	DWORD dummy = 0;
	VirtualProtect(address, overwrite_length, PAGE_EXECUTE_READWRITE, &oldProt);
	memcpy(address, &dl->second.original_code, overwrite_length);
	VirtualProtect(address, overwrite_length, oldProt, &dummy);
	detours.erase(dl);
	return true;
}
GENERIC_DETOUR_API bool add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, int type) {

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
//TODO: 
	__asm {
		PUSHFD
		PUSHAD
		SUB ESP, 12 //sizeof(DETOUR_GATEWAY_OPTIONS)
		CALL detour_c_call_dest //returns nothing, all options are in above struct
		ADD ESP, 12 //skip sizeof(DETOUR_GATEWAY_OPTIONS)
		POPAD //32 bytes
		CMP [ESP-40], 1 //check DETOUR_GATEWAY_OPTIONS[1] for 1
		JE do_return_to_orig

		POPFD //4 bytes
		ADD ESP, [ESP-40] //move stack by DETOUR_GATEWAY_OPTIONS[0] bytes
		RET

do_return_to_orig:
		POPFD // 4 bytes
		ADD ESP, 4 //knock out our return address
		JMP DWORD PTR [ESP-52] //jmp to DETOUR_GATEWAY_OPTIONS[2]
	}
}
void detour_c_call_dest(
	DETOUR_GATEWAY_OPTIONS gateway_opt,
	DETOUR_LIVE_SETTINGS stack
	) {

	char tempstring[512];

	memset(&gateway_opt, 0, sizeof(gateway_opt));

	detour_list_type::iterator dl = detours.find((BYTE*)(stack.ret_addr-5));
	if (dl == detours.end()) {
		sprintf_s(tempstring, sizeof(tempstring), "Called detour from function 0x%x and could not find a registered handler. Crash is likely.", (stack.ret_addr-5));
		OutputDebugString(tempstring);
		return;
	}

	GDetour &d = dl->second;

	EnterCriticalSection(&d.my_critical_section);

	gateway_opt = d.gateway_opt; //copy options to the stack

	stack.registers.esp = stack.registers.esp + 4; //ESP is ignored on POPAD anyway, and its up 4 due to PUSHFD. Lets just correct it for simplicity.

	d.live_settings = stack;

	sprintf_s(tempstring, sizeof(tempstring),"func: 0x%x, ret: 0x%x,\n R[EAX] = 0x%x,\n R[ECX] = 0x%x,\n R[EDX] = 0x%x,\n R[EBX] = 0x%x,\n R[ESP] = 0x%x,\n R[EBP] = 0x%x,\n R[ESI] = 0x%x,\n R[EDI] = 0x%x,\n flags = 0x%x,\n arg0 = 0x%x,\n arg1 = 0x%x\n",
		d.live_settings.ret_addr-5,
		d.live_settings.caller_ret,
		d.live_settings.registers.eax,
		d.live_settings.registers.ecx,
		d.live_settings.registers.edx,
		d.live_settings.registers.ebx,
		d.live_settings.registers.esp,
		d.live_settings.registers.ebp,
		d.live_settings.registers.esi,
		d.live_settings.registers.edi,
		d.live_settings.flags,
		d.live_settings.paramZero,
		(DWORD*)(&d.live_settings.paramZero)[1]
		);
	//MessageBox(0,tempstring, "",0);
	OutputDebugString(tempstring);

	CallPythonDetour(d);

	stack = d.live_settings; //Copy the temporary live settings back to the stack
	
	LeaveCriticalSection(&dl->second.my_critical_section);

}

GENERIC_DETOUR_API GDetour* getDetour(BYTE* address) {
	detour_list_type::iterator dl = detours.find((BYTE*)(address));
	if (dl == detours.end()) {
		return NULL;
	}
	return &dl->second;
}
GENERIC_DETOUR_API int test_detour_func(int count) {
	MessageBox(0, "I'm the real test_detour_func!", "Caption", 0);
	if (count < 2) {
		test_detour_func(count+1);
	}
	return count;
}

GENERIC_DETOUR_API int stolen_detour_func(REGISTERS registers, DWORD flags, DWORD retaddr, DWORD params[]) {
	MessageBox(0, "I'm the fake test_detour_func!", "Caption", 0);
	return 42;
}

GENERIC_DETOUR_API int add_test_detour() {
	return add_detour((BYTE*) &test_detour_func, 5, 4);
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