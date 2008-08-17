// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GENERIC_DETOUR_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GENERIC_DETOUR_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once

#ifdef GENERIC_DETOUR_EXPORTS
#define GENERIC_DETOUR_API extern "C" __declspec(dllexport)
#else
#define GENERIC_DETOUR_API extern "C" __declspec(dllimport)
#endif

class GDetour;

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
	BYTE*		original_code;				//8
	int			call_original_on_return;	//4
	int			bytes_to_pop_on_ret;		//0
};
struct DETOUR_LIVE_SETTINGS {
	REGISTERS	registers; //10
	DWORD		flags; //c
	DWORD		ret_addr; //8
	DWORD		caller_ret; //4
	DWORD		paramZero; //0
};

typedef void (*gdetourCallback)(GDetour &d, DETOUR_LIVE_SETTINGS &l);

class GDetour {
	public:
		BYTE*		address;
		bool		Applied;

		BYTE		original_code[32];
		DWORD		original_code_len;

		//-----
		CRITICAL_SECTION my_critical_section;
		DETOUR_LIVE_SETTINGS live_settings;
		//------
		DETOUR_GATEWAY_OPTIONS gateway_opt;

		gdetourCallback callbackFunction;
		

		bool Apply();
		bool Unapply();
		
		GDetour(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type);
		~GDetour();
};
typedef std::map<BYTE*, GDetour*> detour_list_type;

extern detour_list_type detours;

GENERIC_DETOUR_API GDetour* add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type=0);
GENERIC_DETOUR_API bool remove_detour(BYTE* address);
GENERIC_DETOUR_API GDetour* getDetour(BYTE* address);


int detour_call_dest(); //ASM function that is the jump target
void detour_c_call_dest(DETOUR_GATEWAY_OPTIONS gateway_opt, DETOUR_LIVE_SETTINGS stack);


DWORD CalculateRelativeJMP(DWORD jmp_address, DWORD jmp_destination, int jmp_operand_length=1);
DWORD CalculateAbsoluteJMP(DWORD jmp_address, DWORD jmp_reldestination, int jmp_operand_length=1);

GENERIC_DETOUR_API int test_detour_func(int count=0);
GENERIC_DETOUR_API int stolen_detour_func(REGISTERS registers, DWORD flags, DWORD retaddr, DWORD params[]);

GENERIC_DETOUR_API GDetour* add_test_detour();