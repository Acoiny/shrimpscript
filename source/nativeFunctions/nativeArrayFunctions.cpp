#include "../../header/nativeFunctions/nativeArrayFunctions.hpp"

#include "../../header/nativeFunctions/nativeFunctions.hpp"

static value nativeArray_Init(int arity, value* args, bool& success) {
	objList* newArray = objList::createList();
	if (arity == 1 && IS_INT(args[0])) {
		// if only argument is a number init array with this many elements
		newArray->data.resize(AS_INT(args[0]), NIL_VAL);
		return OBJ_VAL(newArray);
	}
	for (size_t i = 0; i < arity; i++) {
		newArray->appendValue(args[i]);
	}

	return OBJ_VAL(newArray);
}

static value nativeArray_Len(int arity, value* args, bool& success) {
	if (arity != 0) {
		
		success = false;
		return nativeFunctions::error("len: expected 0 arguments");
	}

	value _this = NAT_THIS;
	if (!IS_ARRAY(_this)) {
		success = false;
		return nativeFunctions::error("len: can only be used on arrays");
	}

	double len = ((objList*)AS_OBJ((_this)))->getSize();

	return NUM_VAL(len);
}

static value nativeArray_Append(int arity, value* args, bool& success) {
	if (arity != 1) {
		
		success = false;
		return nativeFunctions::error("append: expected 1 argument");
	}

	value _this = NAT_THIS;
	objList* tmp = (objList*)(AS_OBJ(_this));
	if (!IS_ARRAY(_this)) {
		success = false;
		return nativeFunctions::error("append: can only be used on arrays");
	}

	auto *arr = ((objList*)AS_OBJ(_this));
	arr->appendValue(args[0]);

	return NUM_VAL(double(arr->getSize()));
}

static value nativeArray_Pop(int arity, value* args, bool& success) {
	if (arity != 1) {
		success = false;
		return nativeFunctions::error("pop: expected 1 argument");
	}
	if (!IS_NUM((args[0]))) {
		success = false;
		return nativeFunctions::error("pop: argument must be a number");
	}

	value _this = NAT_THIS;
	if (!IS_ARRAY(_this)) {
		success = false;
		return nativeFunctions::error("pop: can only be used on arrays");
	}
	auto arr = ((objList*)AS_OBJ(_this));

	if (AS_NUM(args[0]) >= arr->getSize() || AS_NUM(args[0]) < 0) {
		success = false;

		return nativeFunctions::error("pop: invalid index");
	}

	value res = arr->data.at(AS_NUM(args[0]));

	arr->data.erase(arr->data.begin() + AS_NUM(args[0]));

	return res;
}

static value nativeArray_Insert(int arity, value* args, bool& success) {
	if (arity != 2) {
		
		success = false;
		return nativeFunctions::error("insert: expected 2 arguments");
	}

	if (!IS_NUM(args[0])) {
		
		success = false;
		return nativeFunctions::error("insert: expected index as first argument");
	}

	value _this = NAT_THIS;
	if (!IS_ARRAY(_this)) {
		success = false;
		return nativeFunctions::error("insert: can only be used on arrays");
	}
	auto* arr = ((objList*)AS_OBJ(_this));

	if (AS_NUM(args[0]) > arr->getSize() || AS_NUM(args[0]) < 0) {
		
		success = false;
		return nativeFunctions::error("insert: invalid index");
	}

	auto it = arr->data.begin() + AS_NUM(args[0]);
	value element = args[1];
	arr->data.insert(it, element);

	return NUM_VAL(double(arr->getSize()));
}

void nativeArrayClass::nativeArrayFunctions(VM& vm)
{
	objString* name = objString::copyString("Array", 5);
	objClass* arrayClass = objClass::createObjClass(name);
	
	arrayClass->tableSet(vm.memory.initString, OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Init)));
	arrayClass->tableSet(objString::copyString("len", 3), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Len)));
	arrayClass->tableSet(objString::copyString("append", 6), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Append)));
	arrayClass->tableSet(objString::copyString("insert", 6), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Insert)));
	arrayClass->tableSet(objString::copyString("pop", 3), OBJ_VAL(objNativeFunction::createNativeFunction(nativeArray_Pop)));

	vm.arrayClass = arrayClass;
	vm.constVector.emplace_back("Array");
	vm.globals.insert_or_assign(name, OBJ_VAL(arrayClass));
}
