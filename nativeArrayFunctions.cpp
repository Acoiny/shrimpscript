#include "nativeArrayFunctions.hpp"

#include "nativeFunctions.hpp"

static value nativeArray_Len(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::erro("len: expected 0 arguments");
	}

	double len = ((objList*)args->as.object)->getSize();

	return value(len);
}

static value nativeArray_Append(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::erro("append: expected 1 argument");
	}

	auto *arr = ((objList*)args[0].as.object);
	arr->appendValue(args[1]);

	return value(arr->getSize());
}

static value nativeArray_Insert(int arity, value* args, bool& success) {
	if (arity != 2) {
		
		success = false;
		return nativeFunctions::erro("insert: expected 2 arguments");
	}

	if (args[1].getType() != VAL_NUM) {
		
		success = false;
		return nativeFunctions::erro("insert: expected index as first argument");
	}

	auto* arr = ((objList*)args[0].as.object);

	if (args[1].as.number > arr->getSize() || args[1].as.number < 0) {
		
		success = false;
		return nativeFunctions::erro("insert: invalid index");
	}

	std::vector<value>::iterator it = arr->data.begin() + args[1].as.number;
	value element = args[2];
	arr->data.insert(it, element);

	return value(arr->getSize());
}

void nativeArrayFunctions(VM& vm, std::unordered_map<objString*, value>& arrayFunTable)
{
	arrayFunTable.insert_or_assign(objString::copyString("len", 3), value(objNativeFunction::createNativeFunction(nativeArray_Len)));
	arrayFunTable.insert_or_assign(objString::copyString("append", 6), value(objNativeFunction::createNativeFunction(nativeArray_Append)));
	arrayFunTable.insert_or_assign(objString::copyString("insert", 6), value(objNativeFunction::createNativeFunction(nativeArray_Insert)));
}
