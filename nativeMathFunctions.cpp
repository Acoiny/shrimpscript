#include "nativeMathFunctions.hpp"

#include "nativeFunctions.hpp"

#include <cmath>

static value nativeMath_Rand(int arity, value* args, bool& success) {
	static bool firstUse = true;
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::erro("Math.rand expects 0 arguments");
	}
	if (firstUse) {
		firstUse = false;
		srand(time(NULL));
	}
	return value(((double)rand() / (double)RAND_MAX));
}

static value nativeMath_Sqrt(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::erro("Math.sqrt expects 1 argument");
	}

	if (args->getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("Math.sqrt argument must be a number");
	}

	return value(sqrt(args->as.number));
}

static value nativeMath_Floor(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::erro("Math.floor expects 1 argument");
	}

	if (args->getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("Math.floor argument must be a number");
	}

	return value(floor(args->as.number));
}

static value nativeMath_Ceil(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::erro("Math.sqrt expects 1 argument");
	}

	if (args->getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("Math.sqrt argument must be a number");
	}

	return value(ceil(args->as.number));
}

void nativeMathFunctions(VM& vm, objNativeInstance* math)
{
	math->tableSet(objString::copyString("sqrt", 4), value(objNativeFunction::createNativeFunction(nativeMath_Sqrt)));
	math->tableSet(objString::copyString("floor", 5), value(objNativeFunction::createNativeFunction(nativeMath_Floor)));
	math->tableSet(objString::copyString("ceil", 4), value(objNativeFunction::createNativeFunction(nativeMath_Ceil)));
	math->tableSet(objString::copyString("rand", 4), value(objNativeFunction::createNativeFunction(nativeMath_Rand)));
}
