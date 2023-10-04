#include <iostream>

#include <fstream>

#include "VM.hpp"
#include "defines.hpp"

static void repl() {
    VM vm;
    std::string input;
    std::cout << ">";
    while(std::getline(std::cin, input)) {
        vm.interpret(input.data());
        
        std::cout << ">";
    }
}

static char* getFileContents(const char* path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    std::streamsize len = file.tellg();
    file.seekg(0, std::ios::beg);
    char* content = new char[len + 1];
    if(!file.good()) {
        std::cerr << "couldn't find file" << std::endl;
        exit(1);
    }
    if(file.is_open()) {
        file.read(content, len);
        file.close();
        content[len] = '\0';
        return content;
    } else {
        std::cerr << "couldn't open file" << std::endl;
        exit(1);
    }
}

static bool verifyFileName(const char* name) {
    bool isValid = false;
    size_t len = strlen(name);
    if (memcmp(name + (len - 7), ".shrimp", 7) == 0) {
        isValid = true;
    }
    else if (memcmp(name + (len - 5), ".🦐", 5) == 0) {
        isValid = true;
    }

    return isValid;
}

static void runFile(const char* path) {

    if (!verifyFileName(path)) {
        std::cout << "file must end in '.shrimp' or '.🦐'" << std::endl;
        return;
    }

    char* cont = getFileContents(path);

    VM vm;
    vm.interpret(cont);

    delete[] cont;
}

int main(int argc, char** argv) {

#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "DEBUG_TRACE_EXECUTION\n";
#endif
#ifdef DEBUG_PRINT_CODE
    std::cout << "DEBUG_PRINT_CODE\n";
#endif
#ifdef DEBUG_STRESS_GC
    std::cout << "DEBUG_STRESS_GC\n";
#endif
#ifdef DEBUG_LOG_GC
    std::cout << "DEBUG_LOG_GC\n";
#endif


    if(argc == 1) {
#ifdef DEBUG_TESTFILE
        runFile("shrimpcode.🦐");
#else
        repl();
#endif
    } else if(argc == 2){
        runFile(argv[1]);
    } else {
        std::cerr << "usage: shrimpp [path]" << std::endl;
    }
    return 0;
}
