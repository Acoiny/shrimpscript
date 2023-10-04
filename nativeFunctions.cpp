#include "nativeFunctions.hpp"

#include "nativeStringFunctions.hpp"
#include "nativeArrayFunctions.hpp"
#include "nativeMathFunctions.hpp"
#include "nativeFileFunctions.hpp"

#include "value.hpp"
#include "obj.hpp"
#include "VM.hpp"
#include <ctime>


typedef value(nativeFunctionType)(obj* object);


static value nativeClock(int arity, value* args, bool& success) {
	if (arity != 0) {
		success = false;
		return nativeFunctions::erro("clock: expect 0 arguments");
	}

	double time = double(std::clock()) / CLOCKS_PER_SEC;

	return value(time);
}

static value nativePrint(int arity, value* args, bool& success) {
	if (arity < 1) {
		success = false;
		return nativeFunctions::erro("print: expects at least 1 argument");
	}

	for (int i = 0; i < arity; ++i) {
		std::cout << args[i];
	}
	return value();
}

static value nativePrintLine(int arity, value* args, bool& success) {
	if (arity < 1) {
		success = false;
		return nativeFunctions::erro("println: expects at least 1 argument");
	}

	for (int i = 0; i < arity; ++i) {
		std::cout << args[i];
	}
	std::cout << std::endl;
	return value();
}

static value native_Type(int argc, value* args, bool& success) {
	if (argc != 1) {
		success = false;
		return nativeFunctions::erro("type: expects 1 argument");
	}

	switch (args->getType()) {
	case VAL_NIL:
		return value(objString::copyString("nil", 3));
	case VAL_NUM:
		return value(objString::copyString("number", 6));
	case VAL_BOOL:
		return value(objString::copyString("bool", 4));
	case VAL_OBJ: {
		switch (args->as.object->getType()) {
		case OBJ_STR:
			return value(objString::copyString("string", 6));
		case OBJ_NAT_FUN:
		case OBJ_FUN:
			return value(objString::copyString("function", 8));
		case OBJ_CLASS:
			return value(objString::copyString("class", 5));
		case OBJ_THIS:
		case OBJ_NAT_INSTANCE:
		case OBJ_INSTANCE:
			return value(objString::copyString("instance", 8));
		case OBJ_LIST:
			return value(objString::copyString("list", 4));
		case OBJ_MAP:
			return value(objString::copyString("dictionary", 10));
		}
		break;
	}
	default:
		return value(objString::copyString("unknown", 7));
	}

	switch (args->as.object->getType()) {
	case OBJ_STR:
		return value(objString::copyString("string", 6));
	case OBJ_FUN:
		return value(objString::copyString("function", 8));
	case OBJ_CLASS:
		return value(objString::copyString("class", 5));
	case OBJ_INSTANCE:
		return value(objString::copyString("object", 6));
	case OBJ_LIST:
		return value(objString::copyString("list", 4));
	case OBJ_NAT_FUN:
		return value(objString::copyString("native_function", 15));
	case OBJ_NAT_INSTANCE:
		return value(objString::copyString("native_object", 13));
	default:
		return value(objString::copyString("unknown", 7));
	}
}

static value native_Input(int arity, value* args, bool& success) {
	if (arity > 1) {
		success = false;
		return nativeFunctions::erro("input: 1 or less arguments");
	}

	if (arity == 1) {
		std::cout << args[0];
	}

	std::string tmp;
	std::getline(std::cin, tmp);

	if (tmp == "nil") {
		return value();
	}
	if (tmp == "true") {
		return value(true);
	}
	if (tmp == "false") {
		return value(false);
	}

	size_t numLen = 0;
	bool isNum = true;
	bool foundDot = false;

	if (tmp.empty())
		return value(objString::copyString("", 0));

	for (size_t i = 0; i < tmp.size(); i++)
	{
		if (tmp.at(i) == '.') {
			if (!foundDot) {
				foundDot = true;
				continue;
			}
			else {
				break;
			}
		}
		if (!(tmp.at(i) >= '0' && tmp.at(i) <= '9')) {
			isNum = false;
			break;
		}
	}

	if (isNum) {
		return value(strtod(tmp.data(), nullptr));
	}

	if(tmp.at(0) == '"' && tmp.at(tmp.size()-1) == '"')
		return value(objString::copyString(tmp.data() + 1, tmp.size() - 2));

	return value(objString::copyString(tmp.data(), tmp.size()));
}

static value native_Open(int arity, value* args, bool& success) {
	value a = args[0];
	value b = args[1];
	if (arity > 2) {
		success = false;
		return nativeFunctions::erro("open: expects 1 or 2 arguments");
	}
	if (!(args[0].getType() == VAL_OBJ && args[0].as.object->getType() == OBJ_STR)) {
		success = false;
		return nativeFunctions::erro("open: expects filename");
	}
	objFile* file;
	if (arity == 2) {
		if (!(args[1].getType() == VAL_OBJ && args[1].as.object->getType() == OBJ_STR)) {
			success = false;
			return nativeFunctions::erro("open: mode must be a string");
		}

		objString* mode = (objString*)(args[1].as.object);
		if (mode->getLen() != 1) {
			success = false;
			return nativeFunctions::erro("open: invalid mode");
		}
		if (mode->getChars()[0] == 'r') {
			file = objFile::createReadFile((objString*)args[0].as.object);
		}
		else if (mode->getChars()[0] == 'w') {
			file = objFile::createWriteFile((objString*)args[0].as.object);
		}
		else {
			success = false;
			return nativeFunctions::erro("open: invalid mode");
		}
	}
	else {
		file = objFile::createReadFile((objString*)args[0].as.object);
	}

	if (!file->isOpen()) {
		success = false;
		return nativeFunctions::erro("open: error opening file");
	}

	return value(file);
}


void nativeFunctions::initNatives(VM& vm) {
	vm.globals.insert_or_assign(objString::copyString("clock", 5), value(objNativeFunction::createNativeFunction(nativeClock)));
	vm.globals.insert_or_assign(objString::copyString("type", 4), value(objNativeFunction::createNativeFunction(native_Type)));
	vm.globals.insert_or_assign(objString::copyString("println", 7), value(objNativeFunction::createNativeFunction(nativePrintLine)));
	vm.globals.insert_or_assign(objString::copyString("print", 5), value(objNativeFunction::createNativeFunction(nativePrint)));
	vm.globals.insert_or_assign(objString::copyString("input", 5), value(objNativeFunction::createNativeFunction(native_Input)));
	vm.globals.insert_or_assign(objString::copyString("open", 4), value(objNativeFunction::createNativeFunction(native_Open)));


	nativeStringFunctions(vm.globals, vm.stringFunctions);
	nativeArrayFunctions(vm, vm.arrayFunctions);
	nativeFileFunctions(vm, vm.fileFunctions);

	auto* math = objNativeInstance::createNativeInstance();
	nativeMathFunctions(vm, math);
	vm.globals.insert_or_assign(objString::copyString("Math", 4), value(math));
}

value nativeFunctions::erro(const char* msg)
{
	return value(objString::copyString(msg, strlen(msg)));
}
