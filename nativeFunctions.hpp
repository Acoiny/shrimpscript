#pragma once

#include "value.hpp"

class VM;

class nativeFunctions {
public:
    static void initNatives(VM &vm);

    static value error(const char* msg);
};
