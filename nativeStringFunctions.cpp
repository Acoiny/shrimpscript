#include "nativeStringFunctions.hpp"

#include "nativeFunctions.hpp"

#include <unordered_map>
#include <string>

#include "VM.hpp"

static value nativeString_Len(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::error("len: expects 0 arguments");
	}

	double len = ((objString*)(AS_OBJ((*args))))->getLen();

	return NUM_VAL(len);
}


static value nativeString_Slice(int arity, value* args, bool& success) {
	if (arity < 1 || arity > 2) {
		
		success = false;
		return nativeFunctions::error("cut: expects 1 or 2 arguments");
	}

	if (!IS_NUM(args[1])) {
		
		success = false;
		return nativeFunctions::error("cut: arguments must be numbers");
	}
	if (arity == 2 && !IS_NUM(args[2])) {
		
		success = false;
		return nativeFunctions::error("cut: arguments must be numbers");
	}
	objString* str = ((objString*)(AS_OBJ(args[0])));
	double len = str->getLen();
	double indexOne = AS_NUM(args[1]);

	double indexTwo;
	if (arity == 2) {
		indexTwo = AS_NUM(args[2]);
	}
	else {
		indexTwo = len;
	}

	if (indexOne < 0 || indexTwo < 0) {
		
		success = false;
		return nativeFunctions::error("slice: substring can't be created with negativ indices");
	}

	if (indexTwo >= len) {
		
		success = false;
		return nativeFunctions::error("slice: second index must be lesser than string length");
	}

	if (indexOne > indexTwo) {
		
		success = false;
		return nativeFunctions::error("slice: first index must be lesser than second index");
	}


	return OBJ_VAL(objString::copyString(str->getChars() + uint64_t(indexOne), uint64_t(indexTwo)));
}

static value nativeString_Chr(int arity, value* args, bool& success) {
	if (arity > 1) {
		
		success = false;
		return nativeFunctions::error("chr: expects 1 argument");
	}
	objString* str = (objString*)AS_OBJ(args[0]);
	unsigned int index = 0;

	if (arity == 1) {
		if (!IS_NUM(args[1]) || AS_NUM(args[1]) >= (str->getLen())) {
			
			success = false;
			return nativeFunctions::error("chr: invalid index");
		}
		index = AS_NUM(args[1]);
	}

	unsigned char res = str->getChars()[index];

	return NUM_VAL(double(res));
}

static value nativeString_To_String(int arity, value* args, bool& success) {
	if (arity != 1) {
		success = false;
		return nativeFunctions::error("to_string: expects 1 argument");
	}

	if (IS_NUM((*args))) {
		std::string result;
		if (AS_NUM((*args)) == long(AS_NUM((*args)))) {
			result = std::to_string(long(AS_NUM((*args))));
		}
		else {
			result = std::to_string(AS_NUM((*args)));
		}
		return OBJ_VAL(objString::copyString(result.data(), result.size()));
	}
	if(IS_NIL((*args))) {
		return OBJ_VAL(objString::copyString("nil", 3));
	}
	if (IS_BOOL((*args))) {
		if (AS_BOOL((*args))) {
			return OBJ_VAL(objString::copyString("true", 4));
		}
		return OBJ_VAL(objString::copyString("false", 5));
	}

	return nativeFunctions::error("to_string: invalid type");
}

static value nativeString_To_Chr(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::error("to_chr: expects 1 argument");
	}

	if (!IS_NUM((*args))) {
		
		success = false;
		return nativeFunctions::error("to_chr: argument must be a number");
	}

	char chr[1];
	chr[0] = AS_NUM((*args));

	return OBJ_VAL(objString::copyString(chr, 1));
}

static value nativeString_At(int arity, value* args, bool& success) {
	if (arity != 1) {
		success = false;
		return nativeFunctions::error("at: expects 1 argument");
	}
	if (!IS_NUM(args[1])) {
		
		success = false;
		return nativeFunctions::error("at: index must be a number");
	}

	int index = AS_NUM(args[1]);

	objString* str = (objString*)AS_OBJ(args[0]);

	if (index >= (int)str->getLen()) {
		
		success = false;
		return nativeFunctions::error("at: invalid index");
	}

	while (index < 0) {
		index += (str->getLen() - 1);
	}

	char atChar[1] = { str->getChars()[index] };

	return OBJ_VAL(objString::copyString(atChar, 1));
}

static value nativeString_Number(int arity, value* args, bool& success) {
	if (arity != 0) {
		success = false;
		return nativeFunctions::error("number: expects 1 argument");
	}

	auto str = (objString*)AS_OBJ((*args));
	
	const char* start = str->getChars();

	if (start[0] >= '0' && start[0] <= '9') {
		double res = strtod(start, nullptr);
		return NUM_VAL(res);
	}

	return NIL_VAL;
}

void nativeStringFunctions(std::unordered_map<objString*, value>& globals, std::unordered_map<objString*, value>& stringFunTable)
{
	//standalone
	globals.insert_or_assign(objString::copyString("to_chr", 6), OBJ_VAL(objNativeFunction::createNativeFunction(nativeString_To_Chr)));
	globals.insert_or_assign(objString::copyString("to_string", 9), OBJ_VAL(objNativeFunction::createNativeFunction(nativeString_To_String)));


	//tied to string objects
	stringFunTable.insert_or_assign(objString::copyString("len", 3), OBJ_VAL(objNativeFunction::createNativeFunction(nativeString_Len)));
	stringFunTable.insert_or_assign(objString::copyString("slice", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativeString_Slice)));
	stringFunTable.insert_or_assign(objString::copyString("chr", 3), OBJ_VAL(objNativeFunction::createNativeFunction(nativeString_Chr)));
	stringFunTable.insert_or_assign(objString::copyString("at", 2), OBJ_VAL(objNativeFunction::createNativeFunction(nativeString_At)));
	stringFunTable.insert_or_assign(objString::copyString("number", 6), OBJ_VAL(objNativeFunction::createNativeFunction(nativeString_Number)));
}
