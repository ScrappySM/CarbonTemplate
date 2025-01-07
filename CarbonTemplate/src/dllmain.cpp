#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>

static HANDLE threadHandle = nullptr;
static DWORD threadId = 0;

static DWORD WINAPI MainThread(LPVOID lpReserved) {
    BOOL console = AllocConsole();

    std::cout << "Hello, World!" << std::endl;

    while (!GetAsyncKeyState(VK_END)) {
        Sleep(100);
    }

    std::cout << "Goodbye, World!" << std::endl;

    if (console) FreeConsole();

    FreeLibraryAndExitThread(static_cast<HMODULE>(lpReserved), 0);
    return 0;
}

BOOL WINAPI DllMain(
    _In_ HINSTANCE hModule,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved
) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        std::cout << "DLL_PROCESS_ATTACH" << std::endl;
        DisableThreadLibraryCalls(hModule);
        threadHandle = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread), hModule, 0, &threadId);
    }

    if (fdwReason == DLL_PROCESS_DETACH) {
        std::cout << "DLL_PROCESS_DETACH" << std::endl;
        if (threadHandle != nullptr) {
            // Signal the thread to exit
            PostThreadMessage(threadId, WM_QUIT, 0, 0);
            // Wait for the thread to exit
            WaitForSingleObject(threadHandle, INFINITE);
            CloseHandle(threadHandle);
        }
    }

    return TRUE;
}
