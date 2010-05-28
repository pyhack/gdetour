#pragma once

#ifdef GENERIC_DETOUR_EXPORTS
#define GENERIC_DETOUR_API extern "C" __declspec(dllexport)
#else
#ifndef GENERIC_DETOUR_STATIC
#define GENERIC_DETOUR_API extern "C" __declspec(dllimport)
#else
#define GENERIC_DETOUR_API extern "C"
#endif
#endif

#include <map>

struct GDetour;

struct REGISTERS {
	//in order of the pushad call
	DWORD edi;
	DWORD esi;
	DWORD ebp;
	DWORD esp;
	DWORD ebx;
	DWORD edx;
	DWORD ecx;
	DWORD eax;
};
struct DETOUR_GATEWAY_OPTIONS {
	int guard_top;// = 0xcccccc;//10
	BYTE*			original_code;				//c
	int				call_original_on_return;	//8
	int				bytes_to_pop_on_ret;		//4
	BYTE*			address_of_new_retn;
	unsigned int	int3_after_call;
	int guard_bottom;// = 0xcccccc; //0
};
struct DETOUR_LIVE_SETTINGS {
	REGISTERS	registers; //10
	DWORD		flags; //c
	DWORD		ret_addr; //8
	DWORD		caller_ret; //4
	DWORD		paramZero; //0
};

typedef void (*gdetourCallback)(GDetour &d, DETOUR_LIVE_SETTINGS &l);

struct GDetour {
	BYTE*		address;
	bool		Applied;

	BYTE		retn_code[5]; //overallocation
	BYTE		original_code[32];
	DWORD		original_code_len;

	//-----
	CRITICAL_SECTION my_critical_section;
	DETOUR_LIVE_SETTINGS live_settings;
	//------
	DETOUR_GATEWAY_OPTIONS gateway_opt;

	gdetourCallback callbackFunction;
};

GDetour* GDetour_Create(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type);
void GDetour_Destroy(GDetour* detour);
	
bool GDetour_Apply(GDetour* detour);
bool GDetour_Unapply(GDetour* detour);


typedef std::map<BYTE*, GDetour*> detour_list_type;
extern detour_list_type detours;

#ifdef GDETOUR_INTERNAL



int detour_call_dest(); //ASM function that is the jump target
void detour_c_call_dest(DETOUR_GATEWAY_OPTIONS gateway_opt, DETOUR_LIVE_SETTINGS stack);

#endif

DWORD CalculateRelativeJMP(DWORD jmp_address, DWORD jmp_destination, int jmp_operand_length=1);
DWORD CalculateAbsoluteJMP(DWORD jmp_address, DWORD jmp_reldestination, int jmp_operand_length=1);


GENERIC_DETOUR_API GDetour* add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type=0);
GENERIC_DETOUR_API bool remove_detour(BYTE* address);
GENERIC_DETOUR_API GDetour* getDetour(BYTE* address);

GENERIC_DETOUR_API int __cdecl   call_cdecl_func_with_registers(REGISTERS r, int dest, ...);
GENERIC_DETOUR_API int __stdcall call_stdcall_func_with_registers(REGISTERS r, int dest, ...);