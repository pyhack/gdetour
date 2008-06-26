// gdetour_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gdetour_api.h"

bool __stdcall test_detour_here(char* myparam) {
	printf("test_detour_here('%s') called!\n", myparam);
	return false;
}

int _tmain(int argc, _TCHAR* argv[])
{
	gdetour g = gdetour();

	//g.add_detour((BYTE*) &test_detour_here, 5, 4);

	printf("test_detour_here is at 0x%08x...\n\n", &test_detour_here);

	int ret = g.run_python_file("..\\testcode.py");
	if (ret == 0) {
		printf("\nImporting python file probably just died for some reason. Exiting test application.\n");
		return 1;
	}




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
	bool a = test_detour_here("test number 1, lol");
	bool b = test_detour_here("test number 2, ftw");

	if (a) {
		printf("You detoured the first call to return true! You sneaky bastard!\n");
	} else {
		printf("You detoured the first call return false as usual.\n");
	}
	if (b) {
		printf("You detoured the second call to return true! You sneaky bastard!\n");
	} else {
		printf("You detoured the second call return false as usual.\n");
	}
	return 0;
}

