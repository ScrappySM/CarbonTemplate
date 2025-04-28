#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

// DllMain function which gets called when the DLL is loaded
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
	}

	if (dwReason == DLL_PROCESS_DETACH) {
		// Cleanup, unhooking, etc here
	}

	return TRUE;
}
