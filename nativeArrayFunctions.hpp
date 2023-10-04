#pragma once

#include <unordered_map>

#include "VM.hpp"


void nativeArrayFunctions(VM& vm, std::unordered_map<objString*, value>& arrayFunTable);
