#include <vector>
#include <string>

#include "value.hpp"

#ifndef SHRIMPP_CHUNK_HPP
#define SHRIMPP_CHUNK_HPP

enum opCodes : char {
    OP_CONSTANT,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MODULO,

    OP_NEGATE,
    OP_NOT,

    OP_INCREMENT_GLOBAL,
    OP_DECREMENT_GLOBAL,

    OP_INCREMENT_LOCAL,
    OP_DECREMENT_LOCAL,

    OP_EQUALS,
    OP_NOT_EQUALS,
    OP_LESSER,
    OP_LESSER_OR_EQUALS,
    OP_GREATER,
    OP_GREATER_OR_EQUALS,

    OP_TRUE,
    OP_FALSE,
    OP_NIL,

    OP_DEFINE_GLOBAL,
    OP_POP,

    OP_GET_GLOBAL,
    OP_SET_GLOBAL,

    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_JUMP,
    OP_JUMP_IF_FALSE,

    OP_LOOP,

    OP_FOR_EACH_INIT,
    OP_FOR_ITER,

    OP_FUNCTION,
    
    OP_CALL,

    OP_RETURN,

    OP_SET_PROPERTY,
    OP_GET_PROPERTY,

    OP_CLASS,
    OP_INHERIT,
    OP_MEMBER_VARIABLE,
    OP_METHOD,
    OP_INVOKE,
    OP_PULL_INSTANCE_FROM_THIS,

    OP_LIST,
    OP_APPEND,
    OP_GET_INDEX,
    OP_SET_INDEX,

    OP_MAP_APPEND,

    OP_THIS,
    OP_SUPER,
    OP_SUPER_INVOKE,

    OP_IMPORT
};

class chunk {
    std::string name;
    std::vector<char> code;
    int codePos = 0;
public:
    std::vector<value> constants;
    std::vector<unsigned int> lines;

    chunk() = default;

    void addByte(char byte, unsigned int line);

    unsigned int addConstantGetLine(value constant, unsigned int line);

    void writeConstant(value constant, unsigned int line);

    inline char peekByte(int index) const {
        return code.at(codePos + index);
    }

    inline char *getInstructionPointer() {
        return code.data();
    }

    inline unsigned int getLine(size_t index) const {
        return lines.at(index);
    }

    inline size_t getSize() const {
        return code.size();
    }

    char &accessAt(size_t pos);

    inline value getConstant(short index) const { return constants.at(index); }
};


#endif //SHRIMPP_CHUNK_HPP
