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

namespace GDetour {
	struct DETOUR_PARAMS {
		BYTE		original_code[32];
		DWORD		original_code_len;
		BYTE*		address;
		//-----
		CRITICAL_SECTION my_critical_section;
		REGISTERS	registers;
		DWORD		flags;
		DWORD		caller_ret;
		DWORD*		params;
		//------
		int			bytes_to_pop_on_ret;
		int			call_original_on_return;
	};
	struct DETOUR_GATEWAY_OPTIONS {
		BYTE*		original_code;				//8
		int			call_original_on_return;	//4
		int			bytes_to_pop_on_ret;		//0
	};


	extern std::map<BYTE*, DETOUR_PARAMS> detour_list;

	GENERIC_DETOUR_API bool add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, int type=0);

	GENERIC_DETOUR_API DETOUR_PARAMS* get_detour_settings(BYTE* address);

	int detour_call_dest();
	void detour_c_call_dest(DETOUR_GATEWAY_OPTIONS gateway_opt, REGISTERS registers, DWORD flags, DWORD ret_addr, DWORD caller_ret, DWORD param_zero);


};

DWORD CalculateRelativeJMP(DWORD jmp_address, DWORD jmp_destination, int jmp_operand_length=1);
DWORD CalculateAbsoluteJMP(DWORD jmp_address, DWORD jmp_reldestination, int jmp_operand_length=1);


GENERIC_DETOUR_API int test_detour_func(int count=0);
GENERIC_DETOUR_API int stolen_detour_func(REGISTERS registers, DWORD flags, DWORD retaddr, DWORD params[]);

GENERIC_DETOUR_API int add_test_detour();