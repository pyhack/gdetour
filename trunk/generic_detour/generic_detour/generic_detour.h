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
	DWORD eax;
	DWORD ecx;
	DWORD edx;
	DWORD ebx;
	DWORD esp;
	DWORD ebp;
	DWORD esi;
	DWORD edi;
};

namespace GDetour {
	struct DETOUR_PARAMS {
		BYTE		original_code[32];
		DWORD		original_code_len;
		BYTE*		address;
		int			bytes_to_pop_on_ret;
		CRITICAL_SECTION my_critical_section;
		REGISTERS	registers;
		DWORD		flags;
		DWORD		caller_ret;
		DWORD*		params;
	};


	extern std::map<BYTE*, DETOUR_PARAMS> detour_list;

	GENERIC_DETOUR_API void initialize();

	GENERIC_DETOUR_API bool add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, int type=0);

	int detour_call_dest();
	int detour_c_call_dest(REGISTERS registers, DWORD flags, DWORD ret_addr, DWORD caller_ret, DWORD param_zero);


};

DWORD CalculateRelativeJMP(DWORD jmp_address, DWORD jmp_destination, int jmp_operand_length=1);
DWORD CalculateAbsoluteJMP(DWORD jmp_address, DWORD jmp_reldestination, int jmp_operand_length=1);


GENERIC_DETOUR_API int test_detour_func(int count=0);
GENERIC_DETOUR_API int stolen_detour_func(REGISTERS registers, DWORD flags, DWORD retaddr, DWORD params[]);

GENERIC_DETOUR_API int add_test_detour();