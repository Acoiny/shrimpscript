#include "nativeFileFunctions.hpp"

#include "obj.hpp"
#include "value.hpp"
#include "nativeFunctions.hpp"

#include <string>

static value nativeFile_read(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::error("read: expected 0 arguments");
	}

	auto* file = (objFile*)AS_OBJ((*args));

	if (!file->isOpen()) {
		
		success = false;
		return nativeFunctions::error("read: file is closed");
	}

	char* tmp = new char[file->fileSize];
	file->file.read(tmp, file->fileSize);

	return OBJ_VAL(objString::copyString(tmp, file->fileSize));
}

static value nativeFile_getline(int arity, value* args, bool& success) {
	if (arity != 0) {
		success = false;
		return nativeFunctions::error("getline: expected 0 arguments");
	}

	auto* file = (objFile*)AS_OBJ((*args));

	if (!file->isOpen()) {

		success = false;
		return nativeFunctions::error("read: file is closed");
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

	auto* file = (objFile*)AS_OBJ((*args));

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
	if (!(IS_OBJ(args[1]) && AS_OBJ(args[1])->getType() == OBJ_STR)) {
		
		success = false;
		return nativeFunctions::error("write: can only write strings");
	}

	auto* file = (objFile*)AS_OBJ(args[0]);
	auto* str = (objString*)AS_OBJ(args[1]);

	if (!file->isOpen()) {
		
		success = false;
		return nativeFunctions::error("write: file is closed");
	}

	
	file->file << str->getChars();

	return NIL_VAL;
}

void nativeFileFunctions(VM& vm, std::unordered_map<objString*, value>& fileFunTable)
{
	fileFunTable.insert_or_assign(objString::copyString("read", 4), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_read)));
	fileFunTable.insert_or_assign(objString::copyString("close", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_close)));
	fileFunTable.insert_or_assign(objString::copyString("write", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_write)));
	fileFunTable.insert_or_assign(objString::copyString("getline", 7), OBJ_VAL(objNativeFunction::createNativeFunction(nativeFile_getline)));

}
