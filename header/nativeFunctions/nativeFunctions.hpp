#pragma once

#include "../virtualMachine/value.hpp"

#define NAT_THIS (*(args - 1))

class VM;

class nativeFunctions {
public:
	static void initNatives(VM& vm);

	static value error(const char* msg);
};
