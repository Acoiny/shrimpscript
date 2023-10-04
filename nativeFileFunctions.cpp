#include "nativeFileFunctions.hpp"

#include "obj.hpp"
#include "value.hpp"
#include "nativeFunctions.hpp"

static value nativeFile_read(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::erro("read: expected 0 arguments");
	}

	auto* file = (objFile*)args->as.object;

	if (!file->isOpen()) {
		
		success = false;
		return nativeFunctions::erro("read: file is closed");
	}

	char* tmp = new char[file->fileSize];
	file->file.read(tmp, file->fileSize);

	return value(objString::copyString(tmp, file->fileSize));
}

static value nativeFile_close(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::erro("close: expected 0 arguments");
	}

	auto* file = (objFile*)args->as.object;

	if (file->isOpen()) {
		file->close();
		return value();
	}
	else {
		
		success = false;
		return nativeFunctions::erro("close: file is already closed");
	}
}

static value nativeFile_write(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::erro("write: expected 1 argument");
	}
	if (!(args[1].getType() == VAL_OBJ && args[1].as.object->getType() == OBJ_STR)) {
		
		success = false;
		return nativeFunctions::erro("write: can only write strings");
	}

	auto* file = (objFile*)args[0].as.object;
	auto* str = (objString*)args[1].as.object;

	if (!file->isOpen()) {
		
		success = false;
		return nativeFunctions::erro("write: file is closed");
	}

	file->file.write(str->getChars(), str->getLen());

	return value();
}

void nativeFileFunctions(VM& vm, std::unordered_map<objString*, value>& fileFunTable)
{
	fileFunTable.insert_or_assign(objString::copyString("read", 4), value(objNativeFunction::createNativeFunction(nativeFile_read)));
	fileFunTable.insert_or_assign(objString::copyString("close", 5), value(objNativeFunction::createNativeFunction(nativeFile_close)));
	fileFunTable.insert_or_assign(objString::copyString("write", 5), value(objNativeFunction::createNativeFunction(nativeFile_write)));

}
