#pragma once
/*This extra cpp file was created to make reading files accessable to the VM itself,
so the import statement can easily execute extra files
*/

class VM;

bool runFile(const char* path);

bool runImportFile(const char* path, VM& vm);