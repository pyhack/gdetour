
typedef	bool (__cdecl *add_detour_func)(BYTE* address, int overwrite_length, int bytes_to_pop, int type);

typedef int (__cdecl *run_python_file_func)(char* filename);



class gdetour {


	add_detour_func imp_add_detour;
	run_python_file_func imp_run_python_file;




public:
	gdetour();
	bool add_detour(BYTE* address, int overwrite_length, int bytes_to_pop, int type=0);
	int run_python_file(char* filename);
};