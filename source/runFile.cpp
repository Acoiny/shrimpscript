#include "../header/runFile.hpp"

#include <iostream>
#include <fstream>

#include "../header/virtualMachine/VM.hpp"

static char* getFileContents(const char* path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    std::streamsize len = file.tellg();
    file.seekg(0, std::ios::beg);
    char* content = new char[len + 1];
    if (!file.good()) {
        std::cerr << "couldn't find file '" << path << "'" << std::endl;
        return nullptr;
    }
    if (file.is_open()) {
        file.read(content, len);
        file.close();
        content[len] = '\0';
        return content;
    }
    else {
        std::cerr << "couldn't open file '" << path << "'" << std::endl;
        return nullptr;
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

bool runFile(const char* path) {

    //TODO: use 'bool imported' to make automatic search for .shrimp and .🦐 files
    if (!verifyFileName(path)) {
        std::cout << "file must end in '.shrimp' or '.🦐'" << std::endl;
        return false;
    }

    char* cont = getFileContents(path);

    if (cont == nullptr)
        return false;

    std::string pathWithoutFile(path);

    size_t indexOfPathEnd = pathWithoutFile.find_last_of('\\');
    if (indexOfPathEnd == std::string::npos) {
        indexOfPathEnd = pathWithoutFile.find_last_of('/');
    }
    if (indexOfPathEnd != std::string::npos) {
        pathWithoutFile = pathWithoutFile.substr(0, indexOfPathEnd + 1);
    }
    else {
        pathWithoutFile = "";
    }

    VM vm;
    vm.currentPath = pathWithoutFile;
    vm.interpret(cont);

    delete[] cont;

    return true;
}

bool runImportFile(const char* path, VM& vm) {
    if (!verifyFileName(path)) {
        std::cout << "file must end in '.shrimp' or '.🦐'" << std::endl;
        return false;
    }

    char* cont = getFileContents(path);

    if (cont == nullptr)
        return false;

    vm.interpretImportFile(path, cont);

    delete[] cont;

    return true;
}