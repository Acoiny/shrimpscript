#include "../../header/nativeFunctions/nativeFileFunctions.hpp"

#include "../../header/virtualMachine/VM.hpp"
#include "../../header/virtualMachine/obj.hpp"
#include "../../header/virtualMachine/value.hpp"
#include "../../header/nativeFunctions/nativeFunctions.hpp"

#include <string>

static value nativeFile_Init(int arity, value* args, bool& success) {
	if (arity > 2 || arity == 0) {
		success = false;
		return nativeFunctions::error("expects filename and optional mode");
	}

	bool readMode = true;

	if (!IS_STR(args[0])) {
		success = false;
		return nativeFunctions::error("filename must be a string");
	}
	if (arity == 2) {
		if (!(IS_STR(args[1]) && AS_STR(args[1])->len == 1 && (AS_CSTR(args[1])[0] == 'w' || AS_CSTR(args[1])[0] == 'r'))) {
			success = false;
			return nativeFunctions::error("invalid mode");
		}

		if (AS_CSTR(args[1])[0] == 'w')
			readMode = false;
	}

	objFile* newFile = nullptr;
	if (readMode)
		newFile = objFile::createReadFile(AS_STR(args[0]));
	else
		newFile = objFile::createWriteFile(AS_STR(args[0]));

	return OBJ_VAL(newFile);
}

static value nativeFile_read(int arity, value* args, bool& success) {
	if (arity != 0) {

		success = false;
		return nativeFunctions::error("read: expected 0 arguments");
	}

	value _this = NAT_THIS;
	if (!IS_FILE(_this)) {
		success = false;
		return nativeFunctions::error("read: can only be used on files");
	}

	auto* file = (objFile*)AS_OBJ((_this));

	if (!file->isOpen()) {

		success = false;
		return nativeFunctions::error("read: file is closed");
	}

	char* tmp = new char[file->fileSize];
	file->file.read(tmp, file->fileSize);
	objString* fileStr = objString::copyString(tmp, file->fileSize);
	delete[] tmp;

	return OBJ_VAL(fileStr);
}

static value nativeFile_getline(int arity, value* args, bool& success) {
	if (arity != 0) {
		success = false;
		return nativeFunctions::error("getline: expected 0 arguments");
	}
	value _this = NAT_THIS;
	if (!IS_FILE(_this)) {
		success = false;
		return nativeFunctions::error("getline: can only be used on files");
	}
	auto* file = (objFile*)AS_OBJ((_this));

	if (!file->isOpen()) {

		success = false;
		return nativeFunctions::error("getline: file is closed");
	}

	std::string tmp;
	if (!std::getline(file->file, tmp)) {
		return NIL_VAL;
	}

	return OBJ_VAL(objString::copyString(tmp.data(), tmp.size()));
}

static value nativeFile_close(int arity, value* args, bool& success) {
	if (arity != 0) {

		success = false;
		return nativeFunctions::error("close: expected 0 arguments");
	}
	value _this = NAT_THIS;
	if (!IS_FILE(_this)) {
		success = false;
		return nativeFunctions::error("close: can only be used on files");
	}
	auto* file = (objFile*)AS_OBJ((_this));

	if (file->isOpen()) {
		file->close();
		return NIL_VAL;
	}
	else {

		success = false;
		return nativeFunctions::error("close: file is already closed");
	}
}

static value nativeFile_write(int arity, value* args, bool& success) {
	if (arity != 1) {

		success = false;
		return nativeFunctions::error("write: expected 1 argument");
	}
	if (!(IS_OBJ(args[0]) && AS_OBJ(args[0])->getType() == OBJ_STR)) {

		success = false;
		return nativeFunctions::error("write: can only write strings");
	}
	value _this = NAT_THIS;
	if (!IS_FILE(_this)) {
		success = false;
		return nativeFunctions::error("write: can only be used on files");
	}
	auto* file = (objFile*)AS_OBJ(_this);
	auto* str = (objString*)AS_OBJ(args[0]);

	if (!file->isOpen()) {

		success = false;
		return nativeFunctions::error("write: file is closed");
	}


	file->file << str->getChars();

	return NIL_VAL;
}

void nativeFileClass::nativeFileFunctions(VM& vm)
{
	objString* name = objString::copyString("File", 4);
	objClass* fileClass = objClass::createObjClass(name);

	fileClass->tableSet(vm.memory.initString, OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_Init)));
	fileClass->tableSet(objString::copyString("read", 4), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_read)));
	fileClass->tableSet(objString::copyString("close", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_close)));
	fileClass->tableSet(objString::copyString("write", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_write)));
	fileClass->tableSet(objString::copyString("getline", 7), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_getline)));

	vm.fileClass = fileClass;
	vm.globals.insert_or_assign(name, OBJ_VAL(fileClass));
	vm.constVector.emplace_back("File");
}
