#ifndef SHRIMPP_DEBUG_HPP
#define SHRIMPP_DEBUG_HPP

#include "chunk.hpp"

namespace debug {
    int constantInstruction(const char* name, chunk* ch, int offset);
    int appendInstruction(const char* name, chunk* ch, int offset);
    int byteInstruction(const char* name, chunk* ch, int offset);
    int simpleInstruction(const char* name, chunk* ch, int offset);
    int jumpInstruction(const char* name, int sign, chunk *ch, int offset);
    int callInstruction(const char* name, chunk* ch, int offset);
    int invokeInstructione(const char* name, chunk* ch, int offset);

    int disassembleInstruction(char inst, chunk* ch, int offset);
    void disassembleChunk(chunk *ch);
}

#endif //SHRIMPP_DEBUG_HPP
