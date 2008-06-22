// gdetour_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gdetour_api.h"

int __stdcall test_detour_here(char* myparam) {
	printf("test_detour_here('%s') called!\n", myparam);
	return -1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	gdetour g = gdetour();

	//g.add_detour((BYTE*) &test_detour_here, 5, 4);

	g.run_python_file("..\\testcode.py");


	printf("test_detour_here is at 0x%08x if you know, you wanted to change something...\n\n", &test_detour_here);

	__asm {
		mov eax, 1
		mov ecx, 2
		mov edx, 3
		mov ebx, 4
		//mov esp, 5
		//mov ebp, 6
		mov esi, 7
		mov edi, 8
	}
	test_detour_here("test");
	test_detour_here("omg, lol, test ftw");

	return 0;
}

