#ifndef SHRIMPP_VM_HPP
#define SHRIMPP_VM_HPP

#include <functional>
#include <unordered_map>
#include <cstring>

#include "../defines.hpp"
#include "chunk.hpp"
#include "../compiler.hpp"

#include "memoryManager.hpp"

#ifdef DEBUG_TRACE_EXECUTION

#include "../commandLineOutput/debug.hpp"

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
    //all scripts are stored in vector, so GC can reach them when in import script
    std::vector<objFunction*> scriptFuncs;
    size_t currentScript = 0;

    //size_t stackSize = 0;
    value *stack;

    value *activeCallFrameBottom;
    value *stackTop;

    callFrame callFrames[FRAMES_MAX];
    size_t callDepth = 0;

    std::unordered_map<objString*, value> globals;

    // the compiler uses this vector to store all the consts names
    // if import is used, the new compiler gets the same vector,
    // so consts are still known as const during the compilation time
    std::vector<std::string> constVector;

    //tables with methods for objects like lists & strings
    std::unordered_map<objString*, value> stringFunctions;
    std::unordered_map<objString*, value> arrayFunctions;
    std::unordered_map<objString*, value> fileFunctions;

    char *ip;

    template<typename ...Ts>
    bool runtimeError(const char *msg, Ts... args);

    inline unsigned char readByte() { return *(ip++); }

    inline short readShort() {
        short res = short(readByte()) << 8;
        res |= readByte();
        return res;
    }
    // optimize this function
    inline size_t readSizeT() {
        size_t res = size_t(*(ip++));
        res <<= 8;
        res |= size_t(*(ip++));
    	res <<= 8;
        res |= size_t(*(ip++));
        res <<= 8;
        res |= size_t(*(ip++));
        res <<= 8;
        res |= size_t(*(ip++));
        res <<= 8;
        res |= size_t(*(ip++));
        res <<= 8;
        res |= size_t(*(ip++));
        res <<= 8;
        res |= size_t(*(ip++));
        return res;
    }

    void push(value val);

    value pop();

    inline const value peek(int dist) const { return *(stackTop - 1 - dist); }

    inline void peek_set(int dist, const value val) { *(stackTop - 1 - dist) = val; }

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

    bool getObjectIndex(obj* object, value index);

    bool setObjectIndex(obj* object, value index, value val);

    bool importFile(const char* filename);

public:

    // the path that gets added to imported files, so they are always relative to the executed file
    std::string currentPath;

    ~VM();

    VM();

    exitCodes interpret(char *str);

    void interpretImportFile(const char* name, char* str);

    memoryManager &memory;
    compiler *currentCompiler;

    exitCodes run();

    value replGetLast();
};


#endif //SHRIMPP_VM_HPP
