#pragma once

#include <unordered_map>
#include "../virtualMachine/VM.hpp"

class nativeStringClass {
public:
	static void nativeStringFunctions(VM& globals);
};
