#include <iostream>

#include "../../header/commandLineOutput/debug.hpp"

#include "../../header/virtualMachine/obj.hpp"

using namespace std;

int debug::constantInstruction(const char *name, chunk *ch, int offset) {
    short index = (ch->peekByte(offset + 1)) << 8 | ch->peekByte(offset + 2);
    cout.width(4);
    cout << offset;
    cout.width(20);
    cout << name << " '";
    cout << ch->getConstant(index) << "'" << endl;
    return offset + 3;
}

int debug::appendInstruction(const char* name, chunk* ch, int offset) {
    size_t numElements = ((size_t(ch->peekByte(offset + 1)) << 56) | (size_t(ch->peekByte(offset + 2)) << 48) |
        (size_t(ch->peekByte(offset + 3)) << 40) | (size_t(ch->peekByte(offset + 4)) << 32) |
        (size_t(ch->peekByte(offset + 5)) << 24) | (size_t(ch->peekByte(offset + 6)) << 16) |
        (size_t(ch->peekByte(offset + 7)) << 8) | size_t(ch->peekByte(offset + 8)));
    cout.width(4);
    cout << offset;
    cout.width(20);
    cout << name;
    cout << numElements << endl;
    return offset + 9;
}

int debug::byteInstruction(const char *name, chunk *ch, int offset) {
    short index = (ch->peekByte(offset + 1)) << 8 | ch->peekByte(offset + 2);
    cout.width(4);
    cout << offset;
    cout.width(20);
    cout << name << " ";
    cout << index << endl;
    return offset + 3;
}

int debug::simpleInstruction(const char *name, chunk *ch, int offset) {
    cout.width(4);
    cout << offset;
    cout.width(20);
    cout << name << endl;
    return offset + 1;
}

int debug::jumpInstruction(const char *name, int sign, chunk *ch, int offset) {
    short jmpOffset = (ch->peekByte(offset + 1)) << 8 | ch->peekByte(offset + 2);
    cout.width(4);
    cout << offset;
    cout.width(20);
    cout << name << " ";
    cout << offset << " -> " << offset + 3 + jmpOffset * sign << endl;
    return offset + 3;
}

int debug::callInstruction(const char *name, chunk *ch, int offset) {
    int args = ch->peekByte(offset + 1);
    cout.width(4);
    cout << offset;
    cout.width(20);
    cout << name << endl;
    return offset + 2;
}

int debug::invokeInstruction(const char* name, chunk* ch, int offset) {
    short jmpOffset = (ch->peekByte(offset + 1)) << 8 | ch->peekByte(offset + 2);
    int args = ch->peekByte(offset + 1);
    cout.width(4);
    cout << offset;
    cout.width(20);
    cout << name << endl;
    return offset + 4;
}

int debug::disassembleInstruction(char inst, chunk *ch, int offset) {
    switch (inst) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", ch, offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", ch, offset);
        case OP_SUB:
            return simpleInstruction("OP_SUB", ch, offset);
        case OP_MUL:
            return simpleInstruction("OP_MUL", ch, offset);
        case OP_DIV:
            return simpleInstruction("OP_DIV", ch, offset);
        case OP_MODULO:
            return simpleInstruction("OP_MODULO", ch, offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", ch, offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", ch, offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", ch, offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", ch, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", ch, offset);
        case OP_EQUALS:
            return simpleInstruction("OP_EQUALS", ch, offset);
        case OP_CASE_COMPARE:
            return simpleInstruction("OP_CASE_COMP", ch, offset);
        case OP_NOT_EQUALS:
            return simpleInstruction("OP_NOT_EQUALS", ch, offset);
        case OP_LESSER:
            return simpleInstruction("OP_LESSER", ch, offset);
        case OP_LESSER_OR_EQUALS:
            return simpleInstruction("OP_LESSER_OR_EQUALS", ch, offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", ch, offset);
        case OP_GREATER_OR_EQUALS:
            return simpleInstruction("OP_GREATER_OR_EQUALS", ch, offset);
        // bitwise operations
        case OP_BIT_AND:
            return simpleInstruction("OP_BIT_AND", ch, offset);
        case OP_BIT_OR:
            return simpleInstruction("OP_BIT_OR", ch, offset);
        case OP_BIT_SHIFT_LEFT:
            return simpleInstruction("OP_BIT_SHIFT_LEFT", ch, offset);
        case OP_BIT_SHIFT_RIGHT:
            return simpleInstruction("OP_BIT_SHIFT_RIGHT", ch, offset);
        case OP_BIT_NOT:
            return simpleInstruction("OP_BIT_NOT", ch, offset);
        case OP_BIT_XOR:
            return simpleInstruction("OP_BIT_XOR", ch, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", ch, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", ch, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", ch, offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", ch, offset);
        case OP_CLOSURE: {
            offset++;
            short index = (ch->peekByte(offset++)) << 8 | ch->peekByte(offset++);
            objFunction* func = (objFunction*)AS_OBJ(ch->getConstant(index));
            cout.width(4);
            cout << offset - 3;
            cout.width(20);
            cout << "OP_CLOSURE" << " '";
            cout << ch->getConstant(index) << "'" << endl;
            for (int i = 0; i < func->upvalueCount; i++) {
                int isLocal = ch->accessAt(offset++);
                int index = ch->accessAt(offset++);
                cout.width(4);
                cout << offset - 2;
                cout.width(20); cout << "|";
                cout << " " << (isLocal ? "local" : "upvalue");
                cout << " " << index << std::endl;
            }
            return offset;
        }
        case OP_SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", ch, offset);
        case OP_GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", ch, offset);
        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", ch, offset);
        case OP_SET_PROPERTY:
            return constantInstruction("OP_SET_PROPERTY", ch, offset);
        case OP_GET_PROPERTY:
            return constantInstruction("OP_GET_PROPERTY", ch, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", ch, offset);
        case OP_POP:
            return simpleInstruction("OP_POP", ch, offset);
        case OP_LIST:
            return simpleInstruction("OP_LIST", ch, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, ch, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, ch, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, ch, offset);
        // case OP_FUNCTION:
            // return constantInstruction("OP_FUNCTION", ch, offset);
        case OP_CALL:
            return callInstruction("OP_CALL", ch, offset);
        case OP_CLASS:
            return constantInstruction("OP_CLASS", ch, offset);
        case OP_METHOD:
            return constantInstruction("OP_METHOD", ch, offset);
        case OP_MEMBER_VARIABLE:
            return constantInstruction("OP_MEMBER_VAR", ch, offset);
        case OP_INVOKE:
            return invokeInstruction("OP_INVOKE", ch, offset);
        case OP_APPEND:
            return appendInstruction("OP_APPEND", ch, offset);
        case OP_GET_INDEX:
            return simpleInstruction("OP_GET_INDEX", ch, offset);
        case OP_SET_INDEX:
            return simpleInstruction("OP_SET_INDEX", ch, offset);
        case OP_THIS:
            return simpleInstruction("OP_THIS", ch, offset);
        case OP_SUPER:
            return constantInstruction("OP_SUPER", ch, offset);
        case OP_INHERIT:
            return simpleInstruction("OP_INHERIT", ch, offset);
        case OP_SUPER_INVOKE:
            return invokeInstruction("OP_SUPER_INVOKE", ch, offset);
        case OP_IMPORT:
            return simpleInstruction("OP_IMPORT", ch, offset);
        case OP_INCREMENT_GLOBAL:
            return byteInstruction("OP_INCREMENT_GLOBAL", ch, offset);
        case OP_DECREMENT_GLOBAL:
            return byteInstruction("OP_DECREMENT_GLOBAL", ch, offset);
        case OP_INCREMENT_LOCAL:
            return byteInstruction("OP_INCREMENT_LOCAL", ch, offset);
        case OP_DECREMENT_LOCAL:
            return byteInstruction("OP_DECREMENT_LOCAL", ch, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", ch, offset);
        default: {
            return simpleInstruction("UNKNOWN OPCODE", ch, offset);
        }
    }
}

void debug::disassembleChunk(chunk *ch) {
    for (int offset = 0; offset < ch->getSize();) {
        offset = disassembleInstruction(ch->accessAt(offset), ch, offset);
    }
}
