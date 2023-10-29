#pragma once

#include <unordered_map>

#include "../virtualMachine/value.hpp"

class VM;
class objString;


void nativeFileFunctions(VM& vm, std::unordered_map<objString*, value>& fileFunTable);