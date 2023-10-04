#pragma once

class VM;
class value;

class nativeFunctions {
public:
    static void initNatives(VM &vm);

    static value erro(const char* msg);
};
