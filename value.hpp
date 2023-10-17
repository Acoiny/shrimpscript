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
#define NUM_VAL(num) value{ .as_double{num}}

//TODO: add return address thingy

//macros for checking type of values
#define IS_NUM(val) (((val.as_uint64) & QNAN) != QNAN)

#define IS_NIL(val) (val.as_uint64 == 0x7ffe000000000000)
#define IS_BOOL(val) ((val.as_uint64 & BOOL_MASK) == BOOL_MASK)

#define IS_OBJ(val) ((val.as_uint64 & NANISH_MASK) ==  OBJ_MASK)
#define IS_RET(val) ((val.as_uint64 & NANISH_MASK) == RET_MASK)

//macros for pulling data out of values
#define AS_NUM(val) (val.as_double)
#define AS_BOOL(val) ((bool)(val.as_uint64 & 0x1))
#define AS_OBJ(val) ((obj*)(val.as_uint64 & 0xFFFFFFFFFFFF))
#define AS_RET(val) ((uintptr_t)(val.as_uint64 & 0xFFFFFFFFFFFF))

std::ostream& operator<<(std::ostream& os, const value val);

bool operator==(const value a, const value b);

#else
#define TRUE_VAL value(true)
#define FALSE_VAL value(false)
#define NIL_VAL value()
#define OBJ_VAL(ptr) value(ptr)
#define RET_VAL(ptr) value(ptr)
#define NUM_VAL(num) value(num)

//macros for checking type of values
#define IS_NUM(val) (val.getType() == VAL_NUM)

#define IS_NIL(val) (val.getType() == VAL_NIL)
#define IS_BOOL(val) (val.getType() == VAL_BOOL)

#define IS_OBJ(val) (val.getType() == VAL_OBJ)
#define IS_RET(val) (val.getType() == VAL_ADDRESS)

#define AS_NUM(val) (val.as.number)
#define AS_BOOL(val) (val.as.boolean)
#define AS_OBJ(val) (val.as.object)
#define AS_RET(val) (val.as.address)

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
