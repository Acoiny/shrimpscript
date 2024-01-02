#ifndef SHRIMPP_VALUE_HPP
#define SHRIMPP_VALUE_HPP

#include <iostream>
#include <cstring>

#include <cstdint>

#include "../defines.hpp"

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
// mask can be resued? CHECK IF WORKING FIRST
// #define RET_MASK (0xfffe000000000000)

#define TRUE_VAL value{(BOOL_MASK | 3)}
#define FALSE_VAL value{(BOOL_MASK | 2)}
#define NIL_VAL value{(0x7ffe000000000000)}


//macros for creating the different values
#define OBJ_VAL(ptr) value{(uint64_t)(ptr) | OBJ_MASK}
#define NUM_VAL(num) value{ .as_double{num}}



//macros for checking type of values
#define IS_NUM(val) (((val.as_uint64) & QNAN) != QNAN)

#define IS_NIL(val) (val.as_uint64 == 0x7ffe000000000000)
#define IS_BOOL(val) ((val.as_uint64 & BOOL_MASK) == BOOL_MASK)

#define IS_OBJ(val) ((val.as_uint64 & NANISH_MASK) ==  OBJ_MASK)

#define IS_STR(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_STR)
#define IS_FUN(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_FUN)
#define IS_CLOSURE(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_CLOSURE)
#define IS_INSTANCE(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_INSTANCE)
#define IS_ARRAY(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_LIST)
#define IS_FILE(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_FILE)

// macro for testing if number is an integer

#define IS_INT(val) (IS_NUM(val) && ((val.as_double) == ((long long)(val.as_double))))

#define AS_INT(val) ((long long)(val.as_double))

//macros for pulling data out of values
#define AS_NUM(val) (val.as_double)
#define AS_BOOL(val) ((bool)(val.as_uint64 & 0x1))
#define AS_OBJ(val) ((obj*)(val.as_uint64 & 0xFFFFFFFFFFFF))
#define AS_STR(val) ((objString*)AS_OBJ(val))
#define AS_CSTR(val) (AS_STR(val)->chars)

#define AS_HASH(val) (val.as_uint64)

std::ostream& operator<<(std::ostream& os, const value val);

bool operator==(const value a, const value b);

const std::string stringify(value val);

#else
#define TRUE_VAL value(true)
#define FALSE_VAL value(false)
#define NIL_VAL value()
#define OBJ_VAL(ptr) value(ptr)
#define RET_VAL(ptr) value(ptr)
#define NUM_VAL(num) value(num)

//macros for checking type of values
#define IS_NUM(val) (val.getType() == VAL_NUM)
#define IS_INT(val) (IS_NUM(val) && AS_NUM(val) == AS_INT(val))

#define IS_NIL(val) (val.getType() == VAL_NIL)
#define IS_BOOL(val) (val.getType() == VAL_BOOL)

#define IS_OBJ(val) (val.getType() == VAL_OBJ)

#define IS_STR(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_STR)
#define IS_FUN(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_FUN)
#define IS_CLOSURE(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_CLOSURE)
#define IS_FILE(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_FILE)
#define IS_INSTANCE(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_INSTANCE)
#define IS_ARRAY(val) (IS_OBJ(val) && AS_OBJ(val)->getType() == OBJ_LIST)

#define AS_NUM(val) (val.as.number)
#define AS_BOOL(val) (val.as.boolean)
#define AS_OBJ(val) (val.as.object)
#define AS_STR(val) ((objString*)AS_OBJ(val))
#define AS_CSTR(val) (AS_STR(val)->chars)

#define AS_INT(val) ((long long)val.as.number)

#define AS_HASH(val) (val.as.integer)

// macro for testing if number is an integer

#define IS_INT(val) ((val.as.number) == ((long long)(val.as.number)))

class obj;

enum valType{
    VAL_NUM,
    VAL_BOOL,
    VAL_NIL,
    VAL_OBJ
};

union trueVal{
    bool boolean;
    double number;
    uint64_t integer;
    obj* object;
};

class value {
    friend class memoryManager;

    valType type;

    friend std::ostream &operator<<(std::ostream& os, const value& val);

public:
    trueVal as{};

    explicit value();
    explicit value(bool boolean);
    explicit value(double num);
    explicit value(obj* obj);
    inline valType getType() const { return type; };

    bool operator==(const value& rhs) const;
};


//for printing and type function
const std::string stringify(value val);

//nanbox define endif
#endif


#endif //SHRIMPP_VALUE_HPP
