#pragma once

#include <unordered_map>

#include "../virtualMachine/value.hpp"

class VM;
class objString;

class nativeFileClass {
public:
	static void nativeFileFunctions(VM& vm);
};
