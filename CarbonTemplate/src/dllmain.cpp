#include <Windows.h>
#include <iostream>

#include "log.h"

// DllMain function which gets called when the DLL is loaded
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	// RAII object to handle console creation/destruction
	// Simply having this line here will allow you to print to the games console
	// Don't worry about it, but if you're interested feel free to look in `utils.h`
	// for the implementation
	Log_t logRAII;

	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		std::cout << "DLL_PROCESS_ATTACH" << std::endl;
		// Setup, hook, etc here
	}

	if (dwReason == DLL_PROCESS_DETACH) {
		std::cout << "DLL_PROCESS_DETACH" << std::endl;
		// Cleanup, unhooking, etc here
	}

	return TRUE;
}
