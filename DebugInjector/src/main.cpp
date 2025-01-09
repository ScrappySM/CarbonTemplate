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
static void UninjectModule(HANDLE hProcess, const std::string& moduleName);
static void InjectModule(HANDLE hProcess, const std::string& modulePath);

void CopyDLLs() {
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
		CopyFileA(dll.c_str(), dest.c_str(), FALSE);
	}
}

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
		CopyFileA(dll.c_str(), dest.c_str(), FALSE);
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

	for (auto& dll : dlls) {
		std::string filename = std::filesystem::path(dll).filename().string();
		if (IsModuleInjected(hProcess, filename)) {
			std::cout << "Module " << dll << " is already injected" << std::endl;
			UninjectModule(hProcess, filename);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	CopyDLLs();

	for (auto& dll : dlls) {
		std::string filename = std::filesystem::path(dll).filename().string();
		std::string tempPath = path + "\\temp\\" + filename;
		InjectModule(hProcess, tempPath);
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

static void UninjectModule(HANDLE hProcess, const std::string& moduleName) {
	MODULEENTRY32 me32 = { 0 };
	me32.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hProcess));

	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return;
	}

	if (!Module32First(hSnapshot, &me32)) {
		CloseHandle(hSnapshot);
		return;
	}

	do {
		if (std::string(me32.szModule) == moduleName) {
			HANDLE hModule = me32.hModule;
			CloseHandle(hSnapshot); // Close ref
			//CloseHandle(hModule); // Close ref

			LPVOID pFreeLibrary = GetProcAddress(GetModuleHandleA("Kernel32"), "FreeLibrary");
			if (pFreeLibrary == nullptr) {
				std::cout << "Failed to get address of FreeLibrary in target process" << std::endl;
				CloseHandle(hSnapshot);
				return;
			}

			HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pFreeLibrary), hModule, 0, nullptr);
			if (hThread == nullptr) {
				std::cout << "Failed to create remote thread in target process" << std::endl;
				CloseHandle(hSnapshot);
				return;
			}

			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);

			return;
		}
	} while (Module32Next(hSnapshot, &me32));

	CloseHandle(hSnapshot);
}

static void InjectModule(HANDLE hProcess, const std::string& modulePath) {
	// Allocate memory in the target process for the module path
	LPVOID pModulePath = VirtualAllocEx(hProcess, nullptr, modulePath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (pModulePath == nullptr) {
		std::cout << "Failed to allocate memory in target process" << std::endl;
		return;
	}
	// Write the module path to the target process
	if (!WriteProcessMemory(hProcess, pModulePath, modulePath.c_str(), modulePath.size() + 1, nullptr)) {
		std::cout << "Failed to write module path to target process" << std::endl;
		VirtualFreeEx(hProcess, pModulePath, 0, MEM_RELEASE);
		return;
	}
	// Get the address of LoadLibraryA in the target process
	LPVOID pLoadLibraryA = GetProcAddress(GetModuleHandleA("Kernel32"), "LoadLibraryA");
	if (pLoadLibraryA == nullptr) {
		std::cout << "Failed to get address of LoadLibraryA in target process" << std::endl;
		VirtualFreeEx(hProcess, pModulePath, 0, MEM_RELEASE);
		return;
	}
	// Create a remote thread in the target process to load the module
	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibraryA), pModulePath, 0, nullptr);
	if (hThread == nullptr) {
		std::cout << "Failed to create remote thread in target process" << std::endl;
		VirtualFreeEx(hProcess, pModulePath, 0, MEM_RELEASE);
		return;
	}
	// Wait for the thread to finish
	WaitForSingleObject(hThread, INFINITE);
	// Clean up
	CloseHandle(hThread);
	VirtualFreeEx(hProcess, pModulePath, 0, MEM_RELEASE);

	std::cout << "Injected " << modulePath << " into " << targetProcess << std::endl;
}
