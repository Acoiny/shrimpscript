#include "nativeFunctions.hpp"

#include "nativeStringFunctions.hpp"
#include "nativeArrayFunctions.hpp"
#include "nativeMathFunctions.hpp"
#include "nativeFileFunctions.hpp"

#include "value.hpp"
#include "obj.hpp"
#include "VM.hpp"
#include <ctime>

//to allow initiating garbage collector
extern memoryManager globalMemory;

typedef value(nativeFunctionType)(obj* object);


static value nativeClock(int arity, value* args, bool& success) {
	if (arity != 0) {
		success = false;
		return nativeFunctions::erro("clock: expect 0 arguments");
	}

	double time = double(std::clock()) / CLOCKS_PER_SEC;

	return NUM_VAL(time);
}

static value nativePrint(int arity, value* args, bool& success) {
	if (arity < 1) {
		success = false;
		return nativeFunctions::erro("print: expects at least 1 argument");
	}

	for (int i = 0; i < arity; ++i) {
		std::cout << args[i];
	}
	return NIL_VAL;
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
	return NIL_VAL;
}

static value native_Type(int argc, value* args, bool& success) {
	if (argc != 1) {
		success = false;
		return nativeFunctions::erro("type: expects 1 argument");
	}

	if(IS_NIL((*args)))
		return OBJ_VAL(objString::copyString("nil", 3));
	if(IS_NUM((*args)))
		return OBJ_VAL(objString::copyString("number", 6));
	if(IS_BOOL((*args)))
		return OBJ_VAL(objString::copyString("bool", 4));
	if(IS_OBJ((*args))) {
		switch (AS_OBJ((*args))->getType()) {
		case OBJ_STR:
			return OBJ_VAL(objString::copyString("string", 6));
		case OBJ_NAT_FUN:
		case OBJ_FUN:
			return OBJ_VAL(objString::copyString("function", 8));
		case OBJ_CLASS:
			return OBJ_VAL(objString::copyString("class", 5));
		case OBJ_THIS:
		case OBJ_NAT_INSTANCE:
		case OBJ_INSTANCE:
			return OBJ_VAL(objString::copyString("instance", 8));
		case OBJ_LIST:
			return OBJ_VAL(objString::copyString("list", 4));
		case OBJ_MAP:
			return OBJ_VAL(objString::copyString("dictionary", 10));
		}
	}
	return OBJ_VAL(objString::copyString("unknown", 7));

	/*
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
		return value(objString::copyString("native function", 15));
	case OBJ_NAT_INSTANCE:
		return value(objString::copyString("native object", 13));
	default:
		return value(objString::copyString("unknown", 7));
	}
	*/
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
	if (arity > 2) {
		success = false;
		return nativeFunctions::erro("open: expects 1 or 2 arguments");
	}
	if (!(IS_OBJ(args[0]) && AS_OBJ(args[0])->getType() == OBJ_STR)) {
		success = false;
		return nativeFunctions::erro("open: expects filename");
	}
	objFile* file;
	if (arity == 2) {
		if (!(IS_OBJ(args[1]) && AS_OBJ(args[1])->getType() == OBJ_STR)) {
			success = false;
			return nativeFunctions::erro("open: mode must be a string");
		}

		objString* mode = (objString*)(AS_OBJ(args[1]));
		if (mode->getLen() != 1) {
			success = false;
			return nativeFunctions::erro("open: invalid mode");
		}
		if (mode->getChars()[0] == 'r') {
			file = objFile::createReadFile((objString*)AS_OBJ(args[0]));
		}
		else if (mode->getChars()[0] == 'w') {
			file = objFile::createWriteFile((objString*)AS_OBJ(args[0]));
		}
		else {
			success = false;
			return nativeFunctions::erro("open: invalid mode");
		}
	}
	else {
		file = objFile::createReadFile((objString*)AS_OBJ(args[0]));
	}

	if (!file->isOpen()) {
		success = false;
		return nativeFunctions::erro("open: error opening file");
	}

	return OBJ_VAL(file);
}

static value native_GC(int arity, value* args, bool& success) {
	if (arity != 0) {
		nativeFunctions::erro("collectGarbage: expects 0 arguments");
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

	auto math = objNativeInstance::createNativeInstance();
	nativeMathFunctions(vm, math);
	vm.globals.insert_or_assign(objString::copyString("Math", 4), OBJ_VAL(math));
}

value nativeFunctions::erro(const char* msg)
{
	return OBJ_VAL(objString::copyString(msg, strlen(msg)));
}
