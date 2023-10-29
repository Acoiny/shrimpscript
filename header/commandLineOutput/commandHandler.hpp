#pragma once

#include <string>

class commandHandler
{
	void printHelp();

	void printNativeFunctions();

	void printNativeStringFunctions();

	void printNativeListFunctions();

	void printNativeFileFunctions();

	void printNativeMathFunctions();

public:
	commandHandler(std::string command);
};
