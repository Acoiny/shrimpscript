#ifndef SHRIMPP_VALUE_HPP
#define SHRIMPP_VALUE_HPP

#include <iostream>
#include <cstring>

#include "defines.hpp"

#ifdef NAN_BOXING

union value {
    uint64_t as_uint64;
    double as_double;
};


//set of quiet NaN bits
#define QNAN ((uint64_t)0x7ffc000000000000)

#define NANISH_MASK (0xffff000000000000)
#define BOOL_MASK (0x7ffe000000000002)
#define OBJ_MASK (0xfffc000000000000)
#define RET_MASK (0xfffe000000000000)

#define TRUE_VAL value{(BOOL_MASK | 3)}
#define FALSE_VAL value{(BOOL_MASK | 2)}
#define NIL_VAL value{(0x7ffe000000000000)}


//macros for creating the different values
#define OBJ_VAL(ptr) value{(uint64_t)(ptr) | OBJ_MASK}
#define RET_VAL(ptr) value{(uintptr_t)(ptr) | RET_MASK}

//TODO: add return address thingy

//macros for checking type of values
#define IS_NUMBER(val) ((val.as_uint64 & QNAN) != QNAN)

#define IS_NIL(val) (val.as_uint64 == 0x7ffe000000000000)
#define IS_BOOL(val) ((val.as_uint64 & BOOL_MASK) == BOOL_MASK)

#define IS_OBJ(val) ((val.as_uint64 & QNAN) == OBJ_MASK)
#define IS_RET(val) ((val.as_uint64 & QNAN) == RET_MASK)

//macros for pulling data out of values
#define AS_NUMBER(val) (v.as_double)
#define AS_BOOL(val) ((bool)(v.as_uint64 & 0x1))
#define AS_OBJ(val) ((obj*)(val.as_uint64 & 0xFFFFFFFFFFFF))
#define AS_RET(val) ((uintptr_t)(val.as_uint64 & 0xFFFFFFFFFFFF))


#else
class obj;

enum valType{
    VAL_NUM,
    VAL_BOOL,
    VAL_NIL,
    VAL_OBJ,
    VAL_ADDRESS
};

union trueVal{
    bool boolean;
    double number;
    uintptr_t address;
    obj* object;
};

class value {
    friend class memoryManager;

    valType type;

    friend std::ostream &operator<<(std::ostream& os, const value& val);

    std::ostream &printObject(std::ostream& os) const;

public:
    trueVal as{};

    explicit value();
    explicit value(bool boolean);
    explicit value(double num);
    explicit value(obj* obj);
    explicit value(uintptr_t address);
    inline valType getType() const { return type; };

    bool operator==(const value& rhs) const;
};
#endif

#endif //SHRIMPP_VALUE_HPP
