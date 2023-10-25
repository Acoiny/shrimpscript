#include "nativeArrayFunctions.hpp"

#include "nativeFunctions.hpp"

static value nativeArray_Len(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::error("len: expected 0 arguments");
	}

	double len = ((objList*)AS_OBJ((*args)))->getSize();

	return NUM_VAL(len);
}

static value nativeArray_Append(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::error("append: expected 1 argument");
	}

	auto *arr = ((objList*)AS_OBJ(args[0]));
	arr->appendValue(args[1]);

	return NUM_VAL(double(arr->getSize()));
}

static value nativeArray_Pop(int arity, value* args, bool& success) {
	if (arity != 1) {
		success = false;
		return nativeFunctions::error("pop: expected 1 argument");
	}
	if (!IS_NUM((args[1]))) {
		success = false;
		return nativeFunctions::error("pop: argument must be a number");
	}

	auto arr = ((objList*)AS_OBJ(args[0]));

	if (AS_NUM(args[1]) >= arr->getSize() || AS_NUM(args[1]) < 0) {
		success = false;

		return nativeFunctions::error("pop: invalid index");
	}

	value res = arr->data.at(AS_NUM(args[1]));

	arr->data.erase(arr->data.begin() + AS_NUM(args[1]));

	return res;
}

static value nativeArray_Insert(int arity, value* args, bool& success) {
	if (arity != 2) {
		
		success = false;
		return nativeFunctions::error("insert: expected 2 arguments");
	}

	if (!IS_NUM(args[1])) {
		
		success = false;
		return nativeFunctions::error("insert: expected index as first argument");
	}

	auto* arr = ((objList*)AS_OBJ(args[0]));

	if (AS_NUM(args[1]) > arr->getSize() || AS_NUM(args[1]) < 0) {
		
		success = false;
		return nativeFunctions::error("insert: invalid index");
	}

	auto it = arr->data.begin() + AS_NUM(args[1]);
	value element = args[2];
	arr->data.insert(it, element);

	return NUM_VAL(double(arr->getSize()));
}

void nativeArrayFunctions(VM& vm, std::unordered_map<objString*, value>& arrayFunTable)
{
	arrayFunTable.insert_or_assign(objString::copyString("len", 3), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Len)));
	arrayFunTable.insert_or_assign(objString::copyString("append", 6), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Append)));
	arrayFunTable.insert_or_assign(objString::copyString("insert", 6), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Insert)));
	arrayFunTable.insert_or_assign(objString::copyString("pop", 3), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Pop)));
}
