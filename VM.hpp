#ifndef SHRIMPP_VM_HPP
#define SHRIMPP_VM_HPP

#include <functional>
#include <unordered_map>
#include <cstring>

#include "defines.hpp"
#include "chunk.hpp"
#include "compiler.hpp"

#include "memoryManager.hpp"

#ifdef DEBUG_TRACE_EXECUTION

#include "debug.hpp"

#endif


#define FRAMES_MAX 128
#define STACK_MAX (256 * FRAMES_MAX)


enum exitCodes {
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR
};


struct callFrame{
    value* bottom;
    objFunction* func;
    value* top;
};

class VM {
    friend class objString;
    friend class objFunction;
    friend class memoryManager;
    friend class nativeFunctions;

    bool gcReady = false;

    objFunction* activeFunc;
    objFunction* scriptFunc;

    //size_t stackSize = 0;
    value *stack;

    value *activeCallFrameBottom;
    value *stackTop;

    callFrame callFrames[FRAMES_MAX];
    size_t callDepth = 0;

    std::unordered_map<objString *, value> globals;

    //tables with methods for objects like lists & strings
    std::unordered_map<objString*, value> stringFunctions;
    std::unordered_map<objString*, value> arrayFunctions;
    std::unordered_map<objString*, value> fileFunctions;

    char *ip;

    template<typename ...Ts>
    bool runtimeError(const char *msg, Ts... args);

    inline char readByte() { return *(ip++); }

    inline short readShort() {
        short res = *(ip++) << 8;
        res |= *(ip++);
        return res;
    }

    inline size_t readSizeT() {
        size_t res = *(ip++) << 56;
        res |= *(ip++) << 48;
        res |= *(ip++) << 40;
        res |= *(ip++) << 32;
        res |= *(ip++) << 24;
        res |= *(ip++) << 16;
        res |= *(ip++) << 8;
        res |= *(ip++);
        return res;
    }

    void push(value val);

    value pop();

    value& peek(int dist);

    ///The functions responsible for executing OP-codes
    void concatenateTwoStrings();

    bool add();

    bool sub();

    bool mul();

    bool div();

    bool modulo();

    bool isFalsey(value a);

    bool areEqual(value b, value a);

    bool callValue(value callee, int arity);
    bool call(value callee, int arity);

    void defineMethod();

    bool defineMemberVar();

    bool invoke();

    bool superInvoke();

    bool getObjectIndex(obj* object, value &index);

    bool setObjectIndex(obj* object, value &index, value& val);

    bool importFile(const char* filename);

public:
    ~VM();

    VM();

    void interpret(char *str);

    void interpretImportFile(char* str);

    memoryManager &memory;
    compiler *currentCompiler;

    exitCodes run();
};


#endif //SHRIMPP_VM_HPP
