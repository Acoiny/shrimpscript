#pragma once

#include <unordered_map>

class VM;
class objString;
class value;

void nativeFileFunctions(VM& vm, std::unordered_map<objString*, value>& fileFunTable);