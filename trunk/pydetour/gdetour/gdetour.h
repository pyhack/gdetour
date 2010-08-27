#pragma once

#if defined GENERIC_DETOUR_EXPORTS
#print Compiling gdetour as DLL
#pragma message( "gdetour being compiled with exports" )
#define GENERIC_DETOUR_API extern "C" __declspec(dllexport)
#elif defined GDETOUR_INTERNAL
#pragma message( "gdetour being compiled" )
#define GENERIC_DETOUR_API extern "C"
#elif defined GENERIC_DETOUR_STATIC
#pragma message( "gdetour being compiled as a LIB" )
#define GENERIC_DETOUR_API extern "C"
#else
#pragma message( "gdetour.h api client (will require DLL)" )
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
struct DETOUR_GATEWAY_OPTIONS {
	int guard_top;// = 0xcccccc;//10
	BYTE*			original_code;				//c
	int				call_original_on_return;	//8 //Can be set directly. 1 or 0.
	int				bytes_to_pop_on_ret;		//4
	BYTE*			address_of_new_retn;
	unsigned int	int3_after_call;
	int guard_bottom;// = 0xcccccc; //0
};

//Structure to represent what is on the stack at the time of the detour call
struct DETOUR_LIVE_SETTINGS {
	REGISTERS	registers; //10
	DWORD		flags; //c
	DWORD		ret_addr; //8
	DWORD		caller_ret; //4
	DWORD		paramZero; //0
};

struct GDetour;
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



#ifdef GDETOUR_INTERNAL
//These functions are internal to GDetour, and shouldn't be called.

GDetour* GDetour_Create(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type);
void GDetour_Destroy(GDetour* detour);
	
bool GDetour_Apply(GDetour* detour);
bool GDetour_Unapply(GDetour* detour);

#include <map>
typedef std::map<BYTE*, GDetour*> detour_list_type;
extern detour_list_type detours;

int detour_call_dest(); //ASM function that is the jump target
void detour_c_call_dest(DETOUR_GATEWAY_OPTIONS gateway_opt, DETOUR_LIVE_SETTINGS stack);

#endif

//You can specify the functions used for GDetour to allocate and free memory.
//Just set these function pointers before making any calls.
//By default, gdetour uses malloc() and free()
typedef GDetour* (*gdetour_malloc_func_type)(unsigned long bytes);
typedef void (*gdetour_free_func_type)(GDetour*);
GENERIC_DETOUR_API extern gdetour_malloc_func_type gdetour_malloc_func;
GENERIC_DETOUR_API extern gdetour_free_func_type gdetour_free_func;

//Utility functions
DWORD CalculateRelativeJMP(DWORD jmp_address, DWORD jmp_destination, int jmp_operand_length=1);
DWORD CalculateAbsoluteJMP(DWORD jmp_address, DWORD jmp_reldestination, int jmp_operand_length=1);

//Main API
#ifndef GDETOUR_INTERNAL
	GENERIC_DETOUR_API GDetour* gdetour_create(BYTE* address, int overwrite_length, int bytes_to_pop, gdetourCallback callback, int type=0);
	GENERIC_DETOUR_API void gdetour_destroy(GDetour* detour);

	GENERIC_DETOUR_API GDetour* gdetour_get(BYTE* address);

	GENERIC_DETOUR_API int gdetour_unapply(GDetour* detour);
	GENERIC_DETOUR_API int gdetour_apply(GDetour* detour);
#endif

//Utility functions that probably don't quite work yet
GENERIC_DETOUR_API int __cdecl   call_cdecl_func_with_registers(REGISTERS r, int dest, ...);
GENERIC_DETOUR_API int __stdcall call_stdcall_func_with_registers(REGISTERS r, int dest, ...);