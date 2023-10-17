#ifndef SHRIMPP_MEMORYMANAGER_HPP
#define SHRIMPP_MEMORYMANAGER_HPP

#include "obj.hpp"
#include "defines.hpp"
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <vector>

#define GC_GROWTH_FACTOR 2


template<typename Tp>
struct my_equal_to : public std::equal_to<Tp>
{
    constexpr bool operator()(const Tp& _x, const Tp& _y) const
    {
        return std::strcmp(_x, _y) == 0;
    }
};

struct Hash_Func {
    //BKDR hash algorithm
    int operator()(const char* str)const
    {
        int seed = 131;//31  131 1313 13131131313 etc//
        int hash = 0;
        while (*str)
        {
            hash = (hash * seed) + (*str);
            str++;
        }

        return hash & (0x7FFFFFFF);
    }
};




class memoryManager {
    VM *vm;

    std::vector<obj*> grayStack;
    obj* allObjects = nullptr;

    size_t bytesAllocated = 0;

    size_t nextGC = 1024 * 256;

    void markRoots();

    void markObject(obj *obj);

    void markValue(value val);
#ifndef NAN_BOXING
    void markValue(const value &val);
#endif
    void traceReferences();

    void blackenObject(obj *obj);

    void stringsRemoveWhite();

    void sweep();

    void addToObjects(obj* o);

public:

    void collectGarbage();

    size_t getHeapSize();

    std::unordered_map<const char*, objString*, Hash_Func, my_equal_to<const char*>> internedStrings;

    objString* initString;

    explicit memoryManager(VM &v);

    ~memoryManager();

    memoryManager();

    void setVM(VM* v);

    template<typename T>
    obj *allocateObject();

    template<typename T>
    T* allocateArray(unsigned long long len);

    void freeObject(obj *el);

    template<typename T>
    void freeArray(T *arr, unsigned int len);
};

template<typename T>
obj *memoryManager::allocateObject() {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#else
    if (bytesAllocated > nextGC) {
        collectGarbage();
    }
#endif
    obj *res;

    res = new T;
    addToObjects(res);

    bytesAllocated += sizeof(T);

    //std::cout << "allocated:" << bytesAllocated << std::endl;
    return res;
}


template<typename T>
void memoryManager::freeArray(T *arr, unsigned int len) {
    bytesAllocated -= len * sizeof(T);

    delete[] arr;
}


template<typename T>
T *memoryManager::allocateArray(unsigned long long int len) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
    if (bytesAllocated > nextGC) {
        collectGarbage();
    }

    T* res = new T[len];

    bytesAllocated += len * sizeof(T);

    return res;
}






#endif //SHRIMPP_MEMORYMANAGER_HPP
