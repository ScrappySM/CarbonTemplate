#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <TlHelp32.h>

#include <filesystem>
#include <shellapi.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

static const char* targetProcess = "ScrapMechanic.exe";

// Forward declaration of helper functions
static std::string GetModuleDir();
static bool IsProcessRunning(const std::string& processName);
static bool IsModuleInjected(HANDLE hProcess, const std::string& moduleName);
static HANDLE GetProcessHandle(const std::string& processName);

int main(void) {
	std::string path = GetModuleDir();

	// Look for .dll files in the CWD and copy them to the temp directory
	std::vector<std::string> dlls = {};

	std::string tempPath = path + "\\temp";
	CreateDirectoryA(tempPath.c_str(), nullptr);

	for (auto& file : std::filesystem::directory_iterator(path)) {
		if (file.path().extension() == ".dll") {
			dlls.emplace_back(file.path().string());
		}
	}

	for (const auto& dll : dlls) {
		std::string dest = tempPath + "\\" + std::filesystem::path(dll).filename().string();
	}

	bool hadToWait = false;
	while (!IsProcessRunning(targetProcess)) {
		static bool hasAsked = false;
		if (!hasAsked) {
			std::cout << "Start Scrap Mechanic? (Y/n) " << std::endl;

			std::string input;
			std::getline(std::cin, input);

			std::transform(input.begin(), input.end(), input.begin(), ::tolower);

			char firstChar = input.empty() ? 'y' : input[0];

			if (firstChar == 'y') {
				ShellExecuteA(nullptr, "open", "steam://rungameid/387990", nullptr, nullptr, SW_SHOWNORMAL);
			}

			hasAsked = true;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		std::cout << "Waiting for " << targetProcess << " to start..." << std::endl;
		hadToWait = true;
	}

	// Wait a bit longer to make sure the process is fully loaded
	// Just bit of safety
	if (hadToWait) {
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	HANDLE hProcess = GetProcessHandle(targetProcess);

	// Inject the DLLs
	for (const auto& dll : dlls) {
		std::string dest = tempPath + "\\" + std::filesystem::path(dll).filename().string();

		// Check if any of the DLLs are already injected
		if (IsModuleInjected(hProcess, std::filesystem::path(dll).filename().string())) {
			std::cerr << "DLL " << std::filesystem::path(dll).filename().string() << " is already injected" << std::endl;

			// Get a handle to the DLL in the target process
			HANDLE hModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hProcess));
			MODULEENTRY32 me32 = { 0 };
			me32.dwSize = sizeof(MODULEENTRY32);
			Module32First(hModule, &me32);

			do {
				if (std::string(me32.szModule) == std::filesystem::path(dll).filename().string()) {
					break;
				}
			} while (Module32Next(hModule, &me32));

			CloseHandle(hModule);

			// Unload the DLL
			HMODULE hKernel = GetModuleHandleA("Kernel32");
			if (hKernel == nullptr) {
				std::cerr << "Failed to get the handle of Kernel32" << std::endl;
				continue;
			}

			auto l_FreeLibraryA = GetProcAddress(hKernel, "FreeLibrary");
			if (l_FreeLibraryA == nullptr) {
				std::cerr << "Failed to get the address of FreeLibrary" << std::endl;
				continue;
			}

			HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(l_FreeLibraryA), me32.modBaseAddr, 0, nullptr);

			auto dllName = std::filesystem::path(dll).filename().string();
			std::cout << "Detachment of " << dllName << " requested" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}

		LPVOID pLibRemote = VirtualAllocEx(hProcess, nullptr, dest.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (pLibRemote == nullptr) {
			std::cerr << "Failed to allocate memory in the target process" << std::endl;
			continue;
		}

		WriteProcessMemory(hProcess, pLibRemote, dest.c_str(), dest.size() + 1, nullptr);

		auto hKernel = GetModuleHandleA("Kernel32");
		if (hKernel == nullptr) {
			std::cerr << "Failed to get the handle of Kernel32" << std::endl;
			continue;
		}

		auto l_LoadLibraryA = GetProcAddress(hKernel, "LoadLibraryA");
		if (l_LoadLibraryA == nullptr) {
			std::cerr << "Failed to get the address of LoadLibraryA" << std::endl;
			continue;
		}

		HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(l_LoadLibraryA), pLibRemote, 0, nullptr);
		if (hThread == nullptr) {
			std::cerr << "Failed to create remote thread" << std::endl;
			continue;
		}
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, pLibRemote, 0, MEM_RELEASE);

		std::cout << "Injected " << std::filesystem::path(dll).filename().string() << std::endl;
	}

	CloseHandle(hProcess);

	return 0;
}

static std::string GetModuleDir() {
	char szPath[MAX_PATH];
	GetModuleFileNameA(nullptr, szPath, MAX_PATH);
	std::string path(szPath);
	return path.substr(0, path.find_last_of("\\/"));
}

static bool IsProcessRunning(const std::string& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return false;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnapshot, &pe32)) {
		CloseHandle(hSnapshot);
		return false;
	}

	do {
		if (std::string(pe32.szExeFile) == processName) {
			CloseHandle(hSnapshot);
			return true;
		}
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return false;
}

static bool IsModuleInjected(HANDLE hProcess, const std::string& moduleName) {
	MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hProcess));
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return false;
	}
	if (!Module32First(hSnapshot, &me32)) {
		CloseHandle(hSnapshot);
		return false;
	}
	do {
		if (std::string(me32.szModule) == moduleName) {
			CloseHandle(hSnapshot);
			return true;
		}
	} while (Module32Next(hSnapshot, &me32));
	CloseHandle(hSnapshot);
	return false;
}

static HANDLE GetProcessHandle(const std::string& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe32)) {
		CloseHandle(hSnapshot);
		return nullptr;
	}
	do {
		if (std::string(pe32.szExeFile) == processName) {
			CloseHandle(hSnapshot);
			return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		}
	} while (Process32Next(hSnapshot, &pe32));
	CloseHandle(hSnapshot);
	return nullptr;
}
