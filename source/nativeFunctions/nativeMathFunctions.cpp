#include "../../header/nativeFunctions/nativeMathFunctions.hpp"

#include "../../header/nativeFunctions/nativeFunctions.hpp"

#include <cmath>

static value nativeMath_Rand(int arity, value* args, bool& success) {
	static bool firstUse = true;
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::error("Math.rand expects 0 arguments");
	}
	if (firstUse) {
		firstUse = false;
		srand(time(NULL));
	}
	return NUM_VAL(((double)rand() / (double)RAND_MAX));
}

static value nativeMath_Sqrt(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::error("Math.sqrt expects 1 argument");
	}

	if (!IS_NUM((*args))) {
		
		success = false;
		return nativeFunctions::error("Math.sqrt argument must be a number");
	}

	return NUM_VAL(sqrt(AS_NUM((*args))));
}

static value nativeMath_Floor(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::error("Math.floor expects 1 argument");
	}

	if (!IS_NUM((*args))) {
		
		success = false;
		return nativeFunctions::error("Math.floor argument must be a number");
	}

	return NUM_VAL(floor(AS_NUM((*args))));
}

static value nativeMath_Ceil(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::error("Math.sqrt expects 1 argument");
	}

	if (!IS_NUM((*args))) {
		
		success = false;
		return nativeFunctions::error("Math.sqrt argument must be a number");
	}

	return NUM_VAL(ceil(AS_NUM((*args))));
}

void nativeMathClass::nativeMathFunctions(VM& vm)
{
	objString* mathName = objString::copyString("Math", 4);
	objClass* math = objClass::createObjClass(mathName);
	math->tableSet(objString::copyString("sqrt", 4), OBJ_VAL(objNativeFunction::createNativeFunction(nativeMath_Sqrt)));
	math->tableSet(objString::copyString("floor", 5), OBJ_VAL(objNativeFunction::createNativeFunction(nativeMath_Floor)));
	math->tableSet(objString::copyString("ceil", 4), OBJ_VAL(objNativeFunction::createNativeFunction(nativeMath_Ceil)));
	math->tableSet(objString::copyString("rand", 4), OBJ_VAL(objNativeFunction::createNativeFunction(nativeMath_Rand)));

	vm.globals.insert_or_assign(mathName, OBJ_VAL(math));
	vm.constVector.emplace_back("Math");
}
