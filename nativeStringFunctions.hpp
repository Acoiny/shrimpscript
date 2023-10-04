#pragma once

#include <unordered_map>
#include "VM.hpp"


void nativeStringFunctions(std::unordered_map<objString*, value>& globals, std::unordered_map<objString*, value>& stringFunTable);
