#include "nativeStringFunctions.hpp"

#include "nativeFunctions.hpp"

#include <unordered_map>
#include "VM.hpp"

static value nativeString_Len(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::erro("len: expects 0 arguments");
	}

	double len = ((objString*)(args->as.object))->getLen();

	return value(len);
}


static value nativeString_Slice(int arity, value* args, bool& success) {
	if (arity < 1 || arity > 2) {
		
		success = false;
		return nativeFunctions::erro("cut: expects 1 or 2 arguments");
	}

	if (args[1].getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("cut: arguments must be numbers");
	}
	if (arity == 2 && args[2].getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("cut: arguments must be numbers");
	}
	objString* str = ((objString*)(args[0].as.object));
	double len = str->getLen();
	double indexOne = args[1].as.number;

	double indexTwo;
	if (arity == 2) {
		indexTwo = args[2].as.number;
	}
	else {
		indexTwo = len;
	}

	if (indexOne < 0 || indexTwo < 0) {
		
		success = false;
		return nativeFunctions::erro("slice: substring can't be created with negativ indices");
	}

	if (indexTwo >= len) {
		
		success = false;
		return nativeFunctions::erro("slice: second index must be lesser than string length");
	}

	if (indexOne > indexTwo) {
		
		success = false;
		return nativeFunctions::erro("slice: first index must be lesser than second index");
	}


	return value(objString::copyString(str->getChars() + uint64_t(indexOne), uint64_t(indexTwo)));
}

static value nativeString_Chr(int arity, value* args, bool& success) {
	if (arity > 1) {
		
		success = false;
		return nativeFunctions::erro("chr: expects 1 argument");
	}
	objString* str = (objString*)args[0].as.object;
	unsigned int index = 0;

	if (arity == 1) {
		if (args[1].getType() != VAL_NUM || args[1].as.number >= (str->getLen())) {
			
			success = false;
			return nativeFunctions::erro("chr: invalid index");
		}
		index = args[1].as.number;
	}

	unsigned char res = str->getChars()[index];

	return value(double(res));
}


static value nativeString_To_Chr(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::erro("to_chr: expects 1 argument");
	}

	if (args->getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("to_chr: argument must be a number");
	}

	char chr[1];
	chr[0] = args->as.number;

	return value(objString::copyString(chr, 1));
}

static value nativeString_At(int arity, value* args, bool& success) {
	if (arity != 1) {
		success = false;
		return nativeFunctions::erro("at: expects 1 argument");
	}
	if (args[1].getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("at: index must be a number");
	}

	int index = args[1].as.number;

	objString* str = (objString*)args[0].as.object;

	if (index >= (int)str->getLen()) {
		
		success = false;
		return nativeFunctions::erro("at: invalid index");
	}

	while (index < 0) {
		index += (str->getLen() - 1);
	}

	char atChar[1] = { str->getChars()[index] };

	return value(objString::copyString(atChar, 1));
}

void nativeStringFunctions(std::unordered_map<objString*, value>& globals, std::unordered_map<objString*, value>& stringFunTable)
{
	//standalone
	globals.insert_or_assign(objString::copyString("to_chr", 6), value(objNativeFunction::createNativeFunction(nativeString_To_Chr)));

	//tied to string objects
	stringFunTable.insert_or_assign(objString::copyString("len", 3), value(objNativeFunction::createNativeFunction(nativeString_Len)));
	stringFunTable.insert_or_assign(objString::copyString("slice", 5), value(objNativeFunction::createNativeFunction(nativeString_Slice)));
	stringFunTable.insert_or_assign(objString::copyString("chr", 3), value(objNativeFunction::createNativeFunction(nativeString_Chr)));
	stringFunTable.insert_or_assign(objString::copyString("at", 2), value(objNativeFunction::createNativeFunction(nativeString_At)));
}
