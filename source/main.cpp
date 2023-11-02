#include <iostream>

#include <fstream>

#include "../header/runFile.hpp"
#include "../header/commandLineOutput/commandHandler.hpp"

#include "../header/virtualMachine/VM.hpp"
#include "../header/defines.hpp"

static void repl() {
    VM vm;
    std::string input;

    std::cout << "Shrimpscript (pre-release) - created by Acoiny (https:\\\github.com\\Acoiny)\n";

    std::cout << ">";
    while(std::getline(std::cin, input)) {

        vm.interpret(input.data());
        
        std::cout << ">";
    }
}


int main(int argc, char** argv) {

#ifdef DEBUG_TESTFILE
    std::cout << "DEBUG_TESTFILE\n";
#endif
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
        runFile(DEBUG_TESTFILE);
#else
        repl();
#endif
    } else if(argc == 2){
        if (argv[1][0] == '-') {
            commandHandler print(argv[1]);
            return 0;
        }
        runFile(argv[1]);
    } else {
        std::cerr << "usage: shrimpp [path]" << std::endl;
    }
    return 0;
}
