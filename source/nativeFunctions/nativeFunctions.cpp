#include "../../header/nativeFunctions/nativeFunctions.hpp"

#include "../../header/nativeFunctions/nativeStringFunctions.hpp"
#include "../../header/nativeFunctions/nativeArrayFunctions.hpp"
#include "../../header/nativeFunctions/nativeMathFunctions.hpp"
#include "../../header/nativeFunctions/nativeFileFunctions.hpp"

#include "../../header/virtualMachine/value.hpp"
#include "../../header/virtualMachine/obj.hpp"
#include "../../header/virtualMachine/VM.hpp"

#include <ctime>

//to allow initiating garbage collector
extern memoryManager globalMemory;

typedef value(nativeFunctionType)(obj* object);


static value nativeClock(int arity, value* args, bool& success) {
	if (arity != 0) {
		success = false;
		return nativeFunctions::error("clock: expect 0 arguments");
	}

	double time = double(std::clock()) / CLOCKS_PER_SEC;

	return NUM_VAL(time);
}

static value nativePrint(int arity, value* args, bool& success) {
	if (arity < 1) {
		success = false;
		return nativeFunctions::error("print: expects at least 1 argument");
	}

	for (int i = 0; i < arity; ++i) {
		std::cout << args[i];
	}
	return NIL_VAL;
}

static value nativePrintLine(int arity, value* args, bool& success) {
	if (arity < 1) {
		success = false;
		return nativeFunctions::error("println: expects at least 1 argument");
	}

	for (int i = 0; i < arity; ++i) {
		std::cout << args[i];
	}
	std::cout << std::endl;
	return NIL_VAL;
}

static value native_Type(int argc, value* args, bool& success) {
	if (argc != 1) {
		success = false;
		return nativeFunctions::error("type: expects 1 argument");
	}

	std::string str;

	if (IS_NUM((*args))) {
		str = "number";
	}
	else if (IS_BOOL((*args))) {
		str = "bool";
	}
	else if (IS_NIL((*args))) {
		str = "nil";
	}
	else if (IS_RET((*args))) {
		str = "retAdd";
	}
	else if (IS_OBJ((*args))) {
		switch (AS_OBJ((*args))->getType()) {
		case OBJ_STR:
			str = "string";
			break;
		case OBJ_FUN:
		case OBJ_NAT_FUN:
			str = "function";
			break;
		case OBJ_CLASS:
			str = "class";
			break;
		case OBJ_THIS:
		case OBJ_INSTANCE:
			str = "object";
			break;
		case OBJ_LIST:
			str = "list";
			break;
		case OBJ_MAP:
			str = "dict";
			break;
		case OBJ_FILE:
			str = "file";
			break;
		default:
			str = "unknown";
			break;
		}
	}
	else {
		str = "unknown";
	}

	return OBJ_VAL(objString::copyString(str.data(), str.size()));
}

static value native_Input(int arity, value* args, bool& success) {
	if (arity > 1) {
		success = false;
		return nativeFunctions::error("input: 1 or less arguments");
	}

	if (arity == 1) {
		std::cout << args[0];
	}

	std::string tmp;
	std::getline(std::cin, tmp);

	if (tmp == "nil") {
		return NIL_VAL;
	}
	if (tmp == "true") {
		return TRUE_VAL;
	}
	if (tmp == "false") {
		return FALSE_VAL;
	}

	size_t numLen = 0;
	bool isNum = true;
	bool foundDot = false;

	if (tmp.empty())
		return OBJ_VAL(objString::copyString("", 0));

	for (size_t i = 0; i < tmp.size(); i++)
	{
		if (i == 0 && tmp.at(i) == '-')
			continue;

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
		return NUM_VAL(strtod(tmp.data(), nullptr));
	}

	if(tmp.at(0) == '"' && tmp.at(tmp.size()-1) == '"')
		return OBJ_VAL(objString::copyString(tmp.data() + 1, tmp.size() - 2));

	return OBJ_VAL(objString::copyString(tmp.data(), tmp.size()));
}

static value native_Open(int arity, value* args, bool& success) {
	value a = args[0];
	value b = args[1];
	if (arity > 2 || arity < 1) {
		success = false;
		return nativeFunctions::error("open: expects 1 or 2 arguments");
	}
	if (!(IS_OBJ(args[0]) && AS_OBJ(args[0])->getType() == OBJ_STR)) {
		success = false;
		return nativeFunctions::error("open: expects filename");
	}
	objFile* file;
	if (arity == 2) {
		if (!(IS_OBJ(args[1]) && AS_OBJ(args[1])->getType() == OBJ_STR)) {
			success = false;
			return nativeFunctions::error("open: mode must be a string");
		}

		objString* mode = (objString*)(AS_OBJ(args[1]));
		if (mode->getLen() != 1) {
			success = false;
			return nativeFunctions::error("open: invalid mode");
		}
		if (mode->getChars()[0] == 'r') {
			file = objFile::createReadFile((objString*)AS_OBJ(args[0]));
		}
		else if (mode->getChars()[0] == 'w') {
			file = objFile::createWriteFile((objString*)AS_OBJ(args[0]));
		}
		else {
			success = false;
			return nativeFunctions::error("open: invalid mode");
		}
	}
	else {
		file = objFile::createReadFile((objString*)AS_OBJ(args[0]));
	}

	if (!file->isOpen()) {
		success = false;
		return nativeFunctions::error("open: error opening file");
	}

	return OBJ_VAL(file);
}

static value native_GC(int arity, value* args, bool& success) {
	if (arity != 0) {
		nativeFunctions::error("collectGarbage: expects 0 arguments");
		return NIL_VAL;
	}

	size_t prevSize = globalMemory.getHeapSize();

	globalMemory.collectGarbage();

	size_t newSize = globalMemory.getHeapSize();

	return NUM_VAL(double(prevSize - newSize));
}

void nativeFunctions::initNatives(VM& vm) {
	vm.globals.insert_or_assign(objString::copyString("clock", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativeClock)));
	vm.globals.insert_or_assign(objString::copyString("type", 4), OBJ_VAL(objNativeFunction::createNativeFunction(native_Type)));
	vm.globals.insert_or_assign(objString::copyString("println", 7), OBJ_VAL(objNativeFunction::createNativeFunction(nativePrintLine)));
	vm.globals.insert_or_assign(objString::copyString("print", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativePrint)));
	vm.globals.insert_or_assign(objString::copyString("input", 5), OBJ_VAL(objNativeFunction::createNativeFunction(native_Input)));
	vm.globals.insert_or_assign(objString::copyString("open", 4), OBJ_VAL(objNativeFunction::createNativeFunction(native_Open)));
	vm.globals.insert_or_assign(objString::copyString("collectGarbage", 15), OBJ_VAL(objNativeFunction::createNativeFunction(native_GC)));


	nativeStringFunctions(vm.globals, vm.stringFunctions);
	nativeArrayFunctions(vm, vm.arrayFunctions);
	nativeFileFunctions(vm, vm.fileFunctions);

	auto math = objClass::createObjClass(objString::copyString("Math", 4));
	nativeMathFunctions(vm, math);
	vm.globals.insert_or_assign(objString::copyString("Math", 4), OBJ_VAL(math));
}

value nativeFunctions::error(const char* msg)
{
	return OBJ_VAL(objString::copyString(msg, strlen(msg)));
}
