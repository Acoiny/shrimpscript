#include "chunk.hpp"

void chunk::addByte(char byte, unsigned int line) {
    lines.push_back(line);
    code.push_back(byte);
}

unsigned int chunk::addConstantGetLine(value constant, unsigned int line) {
    constants.push_back(constant);
    size_t index = constants.size() - 1;
    if(index > UINT16_MAX) {
        //TODO: error if too much constants
        return -1;
    }

    return constants.size() - 1;
}

void chunk::writeConstant(value constant, unsigned int line) {
    constants.push_back(constant);
    size_t index = constants.size() - 1;
    if(index > UINT16_MAX) {
        //TODO: error if too much constants
        return;
    }
    addByte(OP_CONSTANT, line);
    addByte(char(index >> 8) & 0xff, line);
    addByte(char(index) & 0xff, line);
}

char& chunk::accessAt(size_t pos) {
    return code.at(pos);
}