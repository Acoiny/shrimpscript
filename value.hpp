#include <iostream>


#ifndef SHRIMPP_VALUE_HPP
#define SHRIMPP_VALUE_HPP

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
    valType getType();

};


#endif //SHRIMPP_VALUE_HPP
