// generic_detour.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "pydetour.h"
#include "python_module_pydetour.h"

char* guard_top = "Guard - Top";
char* guard_bottom = "Guard - Bottom";

detour_list_type detours;

//#define DETOUR_DEBUG

GDetour::GDetour(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type) {
	//type is unused. provided for forward compat.
#ifdef DETOUR_DEBUG
	if (overwrite_length < 6) {
		return; //need 6 bytes at minimum for JMP in
	}
	if (overwrite_length > sizeof(this->original_code)-6) {
		return; //need 6 bytes in backed up code for JMP return
	}
#else
	if (overwrite_length < 5) {
		return; //need 5 bytes at minimum for JMP in
	}
	if (overwrite_length > sizeof(this->original_code)-5) {
		return; //need 5 bytes in backed up code for JMP return
	}
#endif
	this->Applied = false;
	this->address = address;
	memset(&this->live_settings, 0x0, sizeof(this->live_settings)); //Zero out live settings
	memset((BYTE*)&this->original_code, 0xCC, sizeof(this->original_code)); //fill with breakpoint
	memset((BYTE*)&this->retn_code, 0x00, sizeof(this->retn_code)); //fill with zeros

	this->original_code_len = overwrite_length;

	this->gateway_opt.address_of_new_retn = (BYTE*)&this->retn_code;
	this->gateway_opt.call_original_on_return = false;
	this->gateway_opt.bytes_to_pop_on_ret = bytes_to_pop;
	this->gateway_opt.original_code = (BYTE*)&this->original_code;

	InitializeCriticalSection(&this->my_critical_section);

	memcpy(this->original_code, this->address, this->original_code_len);

	DWORD dummy = 0;
	VirtualProtect(this->retn_code, sizeof(this->retn_code), PAGE_EXECUTE_READWRITE, &dummy);
	VirtualProtect(this->original_code, sizeof(this->original_code), PAGE_EXECUTE_READWRITE, &dummy);
	this->retn_code[0] = 0xC2; //retn
	*(&this->retn_code[1]) = bytes_to_pop;
	this->retn_code[3] = 0xCC;
	this->retn_code[4] = 0xCC;


	this->original_code[this->original_code_len] = 0xE9; //JMP
	DWORD* retJmp = (DWORD*)&this->original_code[this->original_code_len+1]; //retJmp is pointer to place to write jmp address
	*retJmp = CalculateRelativeJMP((DWORD)&this->original_code[this->original_code_len], (DWORD) (this->address + this->original_code_len));
	
	this->callbackFunction = callback;

	this->Apply();
}

bool GDetour::Apply() {
	if (this->Applied) {
		return false;
	}

	DWORD oldProt = 0;
	DWORD dummy = 0;

	VirtualProtect(this->address, this->original_code_len, PAGE_EXECUTE_READWRITE, &oldProt);

	BYTE* addr;
	DWORD* jaddr;
	addr = (BYTE*) this->address;
	jaddr = (DWORD*) (addr + 1);


#ifdef DETOUR_DEBUG
	addr[0] = 0xCC; //INT 3
	addr++;
	jaddr = (DWORD*) ((BYTE*) jaddr + 1); //ah, the joy of pointers
#endif
	addr[0] = 0xE8; //A Call!
	jaddr[0] = CalculateRelativeJMP((DWORD) addr, (DWORD) &detour_call_dest);
	for(DWORD i = 5; i < this->original_code_len; i++) {
		addr[i] = 0x90; //Set extra bytes to NOP
	}

	VirtualProtect(this->address, this->original_code_len, oldProt, &dummy);

	this->Applied = true;

	return true;
}
bool GDetour::Unapply() {
	if (!this->Applied) {
		return false;
	}
	DWORD oldProt = 0;
	DWORD dummy = 0;
	VirtualProtect(this->address, this->original_code_len, PAGE_EXECUTE_READWRITE, &oldProt);
	memcpy(this->address, &this->original_code, this->original_code_len);
	VirtualProtect(this->address, this->original_code_len, oldProt, &dummy);
	this->Applied = false;
	return true;
}
GDetour::~GDetour() {
	if (this->Applied) {
		this->Unapply();
	}
}



GENERIC_DETOUR_API bool remove_detour(BYTE* address) {
	GDetour* d = getDetour(address);
	if (d == NULL) {
		return false;
	}
	d->Unapply();
	detours.erase(address);
	return true;
}
GENERIC_DETOUR_API GDetour* add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type) {
	GDetour* gd = new GDetour(address, overwrite_length, bytes_to_pop, callback, type);
	detours.insert(std::pair<BYTE*,GDetour*>(address, gd));
	return gd;
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
		//INT 3
		PUSHFD //-4 bytes [-4]
		PUSHAD //-32 bytes [-36]
		SUB ESP, SIZE DETOUR_GATEWAY_OPTIONS //- sizeof(DETOUR_GATEWAY_OPTIONS)
		CALL detour_c_call_dest //returns nothing, all options are in above struct
		//INT 3
		ADD ESP, SIZE DETOUR_GATEWAY_OPTIONS //+ skip sizeof(DETOUR_GATEWAY_OPTIONS)
		POPAD //+32 bytes [-4]
		CMP [ESP-32-SIZE DETOUR_GATEWAY_OPTIONS+(4*5)], 1
		JNE skip_int3
		INT 3
skip_int3:
		CMP [ESP-32-SIZE DETOUR_GATEWAY_OPTIONS+(4*2)], 1 //32 + 4popad + 12 check DETOUR_GATEWAY_OPTIONS[2] for 1
		JE do_return_to_orig

		POPFD //4 bytes [0]
		ADD ESP, 4 //knock out our return address
		JMP DWORD PTR [ESP-32-4-4-SIZE DETOUR_GATEWAY_OPTIONS+(4*4)] //jmp to new RETN command, which fixes up stack by DETOUR_GATEWAY_OPTIONS[3] bytes

do_return_to_orig:
		POPFD // 4 bytes
		ADD ESP, 4 //knock out our return address
		JMP DWORD PTR [ESP-32-4-4-SIZE DETOUR_GATEWAY_OPTIONS+(4*1)] //jmp to DETOUR_GATEWAY_OPTIONS[1]
	}
}




void default_callback(GDetour &d, DETOUR_LIVE_SETTINGS &stack_live_data) {
	char tempstring[512];
	sprintf_s(tempstring, sizeof(tempstring),"func: 0x%x, ret: 0x%x,\n R[EAX] = 0x%x,\n R[ECX] = 0x%x,\n R[EDX] = 0x%x,\n R[EBX] = 0x%x,\n R[ESP] = 0x%x,\n R[EBP] = 0x%x,\n R[ESI] = 0x%x,\n R[EDI] = 0x%x,\n flags = 0x%x\n",
#ifdef DETOUR_DEBUG
		stack_live_data.ret_addr-6,
#else
		stack_live_data.ret_addr-5,
#endif
		stack_live_data.caller_ret,
		stack_live_data.registers.eax,
		stack_live_data.registers.ecx,
		stack_live_data.registers.edx,
		stack_live_data.registers.ebx,
		stack_live_data.registers.esp,
		stack_live_data.registers.ebp,
		stack_live_data.registers.esi,
		stack_live_data.registers.edi,
		stack_live_data.flags
	);
	//MessageBox(0,tempstring, "",0);
	OutputDebugString(tempstring);
}

void detour_c_call_dest(
	DETOUR_GATEWAY_OPTIONS stack_gateway_opt,
	DETOUR_LIVE_SETTINGS stack_live_data
	) {

	char tempstring[512];

	memset(&stack_gateway_opt, 0, sizeof(stack_gateway_opt));

#ifdef DETOUR_DEBUG
	detour_list_type::iterator dl = detours.find((BYTE*)(stack_live_data.ret_addr-6));
#else
	detour_list_type::iterator dl = detours.find((BYTE*)(stack_live_data.ret_addr-5));
#endif

	if (dl == detours.end()) {
		sprintf_s(tempstring, sizeof(tempstring), "Called detour from function 0x%x and could not find a registered handler. Crash is likely.", (stack_live_data.ret_addr-5));
		OutputDebugString(tempstring);
		return;
	}

	GDetour &d = *dl->second;

	EnterCriticalSection(&d.my_critical_section);



	stack_live_data.registers.esp += 8; //ESP is ignored on POPAD anyway, and its up 4 due to PUSHFD. It's also up 4 due to the detour CALL. Lets just correct it for simplicity.

	d.live_settings = stack_live_data;

	//default_callback(d, stack_live_data);
	if (d.callbackFunction) {
		d.callbackFunction(d, stack_live_data);
	}

	stack_gateway_opt = d.gateway_opt; //copy options to the stack
	stack_live_data = d.live_settings; //Copy the temporary live settings back to the stack
	
	LeaveCriticalSection(&d.my_critical_section);


	stack_gateway_opt.guard_top = (int) guard_bottom; //0xCCCCCCCC;
	/*
	stack_gateway_opt.bytes_to_pop_on_ret = 0x04040404;
	stack_gateway_opt.call_original_on_return = 0x08080808;
	stack_gateway_opt.original_code = (BYTE*) 0x0c0c0c0c;
	*/
	stack_gateway_opt.guard_bottom = (int) guard_top; //0xCCCCCCCC;
}



GENERIC_DETOUR_API GDetour* getDetour(BYTE* address) {
	detour_list_type::iterator dl = detours.find((BYTE*)(address));
	if (dl == detours.end()) {
		return NULL;
	}
	return dl->second;
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

GENERIC_DETOUR_API GDetour* add_test_detour() {
	return add_detour((BYTE*) &test_detour_func, 5, 4, NULL);
}

//cdecl tells the compiler to do it's own argument removal. However, because we call the stdcall version, we need to sub the stack up before the compiler adds it back down.
GENERIC_DETOUR_API __declspec(naked) int __cdecl call_cdecl_func_with_registers(REGISTERS r, int dest, ...) {
	__asm {
		CALL call_stdcall_func_with_registers
		SUB ESP, (8*4)+4 //move stack back by the size of the first two params here
	}
}

GENERIC_DETOUR_API __declspec(naked) int __stdcall call_stdcall_func_with_registers(REGISTERS r, int dest, ...) {
	/*
	Calling a function and setting up all the registers (except ESP)

	1. Call helper function a(REGISTERS, DEST, [arg 1], [arg 2], ...)
	2. a() needs to set all the registers from the stack, and put RETADDR just before the first arg

	Stack:

	ESP+00: 0x_RETADDR to helper function

	ESP+04: 0x_EDI
	ESP+08: 0x_ESI
	ESP+0C: 0x_EBP
	ESP+10: 0x_ESP
	ESP+14: 0x_EBX
	ESP+18: 0x_EDX
	ESP+1C: 0x_ECX
	ESP+20: 0x_EAX			//gets overwritten with target dest

	ESP+24: TARGET DEST		//gets overwritten with retaddr

	ESP+28: arg 1
	ESP+2C: arg ...
	---------------------------------------------------------------------------

	Calling Convention Preserved Registers:

	EDI, ESI, EBP, EBX
	http://blogs.msdn.com/oldnewthing/archive/2004/01/08/48616.aspx

	CTypes assumes all four of these are preserved. So, yeah, I'll allow it.

	*/
	__asm {
		//INT 3
		ADD ESP, 4		//										esp at +04
		POP EAX //POP EDI //These 8 POPs mimic POPAD, with the extra four ignored as per calling convention
		POP EAX //POP ESI
		POP EAX //POP EBP
		POP EAX //POP ESP - Ignored because POPAD ignores it, and it'd really fuck stuff up :)
		POP EAX //POP EBX
		POP EDX
		POP ECX
		POP EAX
		//POPAD			//										esp at +24
		PUSH [ESP]		//copies dest addr 						esp at +20
		ADD ESP, 8		//skips both dest addrs					esp at +28
		PUSH [ESP-10*4]	//copies ret addr to just before arg 1	esp at +24
		//INT 3
		JMP DWORD PTR[ESP-4]	//we now rely on the target to pop off any arguments passed to us. this assumes we're talking about stdcall here.
	}
}








DWORD CalculateRelativeJMP(DWORD jmp_address, DWORD jmp_destination, int jmp_operand_length) {
	//jmp_operand_length is the number of BYTEs the JMP opcode takes up.
	//JMP == 1 BYTE
	//JE == 2 BYTEs, etc
	//this function assumes that you will be specifying the absolute offset as a DWORD (4 BYTEs)
	if (jmp_address > jmp_destination) {
		//2s complement for a negative JMP
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