#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>

class Log_t {
public:
	Log_t();
	~Log_t();

private:
	bool bConsole;
};

Log_t::Log_t() {
	bConsole = AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	std::cout.rdbuf(std::cout.rdbuf());
	std::cerr.rdbuf(std::cout.rdbuf());
	std::clog.rdbuf(std::cout.rdbuf());
	std::cout << "Console initialized" << std::endl;
}

Log_t::~Log_t() {
	std::cout << "Console destroyed" << std::endl;
	if (bConsole) FreeConsole();
}

