#include "stdafx.h"

#define PSAPI_VERSION 1
#pragma comment(lib, "psapi.lib")
#include <psapi.h>
#include <stdio.h>

#include "process_stuff.h"

HMODULE WaitForModule(HANDLE hProcess, char* module_name) {
	HMODULE glue_mod = 0;
	while(!glue_mod) {
		HMODULE* mod_list = 0;
		DWORD modsize = 0;
		EnumProcessModules(hProcess, mod_list, 0, &modsize);
		mod_list = new HMODULE[modsize / sizeof(HMODULE)];
		EnumProcessModules(hProcess, mod_list, modsize, &modsize);
		//printf("%i\n", modsize);
		for (unsigned int i=0; i < modsize / sizeof(HMODULE); i++) {
			char buf[255];
			ZeroMemory(buf, sizeof(buf));
			GetModuleBaseName(hProcess, mod_list[i], buf, 255);
			//printf("%s\n", buf);
			if (strcmp(buf, module_name) == 0) {
				glue_mod = mod_list[i];
				break;
			}
		}
		delete[] mod_list;
	}
	return glue_mod;
}

HMODULE* GetLoadedModules(HANDLE hProcess) {
	HMODULE* mod_list = 0;
	DWORD modsize = 0;
	EnumProcessModules(hProcess, mod_list, 0, &modsize);
	mod_list = new HMODULE[modsize / sizeof(HMODULE)];
	EnumProcessModules(hProcess, mod_list, modsize, &modsize);
	printf("%i\n", modsize);
	for (unsigned int i=0; i < modsize / sizeof(HMODULE); i++) {
		char buf[255];
		ZeroMemory(buf, sizeof(buf));
		GetModuleBaseName(hProcess, mod_list[i], buf, 255);
		printf("%s\n", buf);
	}
	return mod_list;
	//delete[] mod_list;
}

int WriteMemory(HANDLE hProcess, BYTE* target, BYTE* source, size_t bytes) {
	if (bytes < 1) {
		return -4;
	}
	DWORD old_prot = 0;
	if (!VirtualProtectEx(hProcess, target, bytes, PAGE_EXECUTE_READWRITE, &old_prot)) {
		return -1;
	}
	SIZE_T retbytes = 0;
	WriteProcessMemory(hProcess, target, source, bytes, &retbytes);
	if (retbytes < bytes) {
		return -2;
	}
	if (!VirtualProtectEx(hProcess, target, bytes, old_prot, &old_prot)) {
		return -3;
	}
	return retbytes;
}