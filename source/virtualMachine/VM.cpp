#include "../../header/virtualMachine/VM.hpp"

#include "../../header/nativeFunctions/nativeFunctions.hpp"

#include "../../header/runFile.hpp"

#ifdef DEBUG_TRACE_EXECUTION
#include <iomanip>
#endif

memoryManager globalMemory;

VM::VM() : ip(), activeFunc(nullptr), memory(globalMemory), callFrames() {
	memory.setVM(this);
	currentCompiler = new compiler(*this, constVector);

	stack = new value[STACK_MAX];
	activeCallFrameBottom = stack;
	stackTop = stack;

	nativeFunctions::initNatives(*this);
}

exitCodes VM::interpret(char* str) {
	activeFunc = currentCompiler->compiling("script", str);
	scriptFuncs.push_back(activeFunc);
	gcReady = true;

	exitCodes exited = INTERPRET_RUNTIME_ERROR;

	if (!currentCompiler->errorOccured()) {
		exited = run();
	}

	return exited;
}

void VM::interpretImportFile(const char* name, char* str) {
	compiler* prevCompiler = currentCompiler;

	currentCompiler = new compiler(*this, constVector);

	//no GC when compiling
	gcReady = false;
	activeFunc = currentCompiler->compiling(name, str);
	scriptFuncs.push_back(activeFunc);
	currentScript++;
	gcReady = true;

	if (!currentCompiler->errorOccured()) {
		run();
	}


	//mustn't delete function here, because of linked list of all objects in GC
	//globalMemory.freeObject(scriptFuncs.at(currentScript--));
	activeFunc = scriptFuncs.at(--currentScript);
	scriptFuncs.pop_back();

	delete currentCompiler;
	currentCompiler = prevCompiler;
}

VM::~VM() {
	memory.internedStrings.clear();
	globals.clear();

	delete currentCompiler;

	delete[] stack;
	//std::cout << memory.bytesAllocated;
}

void VM::push(value val) {
	*stackTop = val;
	stackTop++;
}

value VM::pop() {
	stackTop--;
	return *stackTop;
}

bool VM::concatenateTwoStrings() {
	auto* b = (objString*)AS_OBJ(peek(0));
	auto* a = (objString*)AS_OBJ(peek(1));

	unsigned long long len = a->getLen() + b->getLen();


	if (len == 0 && a->getLen() != 0) {
		runtimeError("string concatenation exceeds maximum size");
		return false;
	}

	char* res = memory.allocateArray<char>(len + 1);
	memcpy(res, a->getChars(), a->getLen());
	memcpy(res + a->getLen(), b->getChars(), b->getLen());
	res[len] = '\0';

	objString* strObj = nullptr;
	// strObj->mark();
	// push(OBJ_VAL(strObj));
	strObj = objString::takeString(res, len);
	// pop();
	// strObj->unmark();
	pop();
	pop();
	push(OBJ_VAL(strObj));

	return true;
}

bool VM::add() {
	value b = peek(0);
	value a = peek(1);
	if (IS_OBJ(a) && IS_OBJ(b)) {
		if (IS_STR(a) && IS_STR(b)) {
			return concatenateTwoStrings();
		}
		else {
			return runtimeError("can't add '", a, "' and '", b, "'");
		}
	}
	/*if (IS_STR(a) && IS_NUM(b)) {
		std::string bStr = stringify(b);
		peek_set(0, OBJ_VAL(objString::copyString(bStr.data(), bStr.size())));
		concatenateTwoStrings();
		return true;
	}*/
	if (!(IS_NUM(a) && IS_NUM(b)))
		return runtimeError("can't add '", a, "' and '", b, "'");
	b = pop();
	a = pop();
	push(NUM_VAL(AS_NUM(a) + AS_NUM(b)));
	return true;
}

bool VM::sub() {
	value b = pop();
	value a = pop();
	if (!(IS_NUM(a) && IS_NUM(b)))
		return runtimeError("can't subtract '", b, "' from '", a, "'");
	push(NUM_VAL(AS_NUM(a) - AS_NUM(b)));
	return true;
}

bool VM::mul() {
	value b = pop();
	value a = pop();
	if (!(IS_NUM(a) && IS_NUM(b)))
		return runtimeError("can't multiply '", a, "' and '", b, "'");
	push(NUM_VAL(AS_NUM(a) * AS_NUM(b)));
	return true;
}

bool VM::div() {
	value b = pop();
	value a = pop();
	if (!(IS_NUM(a) && IS_NUM(b)))
		return runtimeError("can't divide '", a, "' by '", b, "'");
	push(NUM_VAL(AS_NUM(a) / AS_NUM(b)));
	return true;
}

bool VM::modulo() {
	value b = pop();
	value a = pop();
	if (!(IS_NUM(a) && IS_NUM(b)))
		return runtimeError("can't get modulo of '", a, "' and '", b, "'");

	//checking if numbers are ints, and use normal modulo
	long long aInt = AS_NUM(a);
	long long bInt = AS_NUM(b);

	if ((AS_NUM(a) == aInt) && (AS_NUM(b) == bInt)) {
		push(NUM_VAL(double(aInt % bInt)));
	}
	else {
		//floating modulo operation
		long long tmp = AS_NUM(a) / AS_NUM(b);

		double stepMul = tmp * AS_NUM(b);

		double res = AS_NUM(a) - stepMul;

		push(NUM_VAL(double(res)));
	}
	return true;
}

bool VM::isFalsey(value a) {
	if (IS_NIL(a))
		return false;

	if (IS_BOOL(a))
		return AS_BOOL(a);

	if (IS_NUM(a))
		return AS_NUM(a) != 0;

	if (IS_OBJ(a)) {
		if (AS_OBJ(a)->getType() == OBJ_STR) {
			auto str = (objString*)AS_OBJ(a);
			return str->getLen() != 0;
		}
	}
	/*
	switch (a.getType())
	{
	case VAL_NUM:
		return a.as.number != 0;
	case VAL_BOOL:
		return a.as.boolean;
	case VAL_NIL:
		return false;
	case VAL_OBJ: {
		if (a.as.object->getType() == OBJ_STR) {
			auto* str = (objString*)a.as.object;
			return str->getLen() != 0;
		}
		break;
	}
	default:
		break;
	}*/
	return false;
}

bool VM::areEqual(value b, value a) {
	if (IS_NIL(a) && IS_NIL(b))
		return true;

	if (IS_NUM(a) && IS_NUM(b))
		return AS_NUM(a) == AS_NUM(b);

	if (IS_BOOL(a) && IS_BOOL(b))
		return AS_BOOL(a) == AS_BOOL(b);

	if (IS_OBJ(a) && IS_OBJ(b))
		return AS_OBJ(a) == AS_OBJ(b);

	/*
	switch (a.getType()) {
	case VAL_NIL:
		return b.getType() == VAL_NIL;
	case VAL_NUM:
		if (b.getType() == VAL_NUM) {
			return a.as.number == b.as.number;
		}
		break;
	case VAL_BOOL:
		if (b.getType() == VAL_BOOL) {
			return a.as.boolean == b.as.boolean;
		}
		break;
	default:
		return a.as.object == b.as.object;
	}
	*/
	return false;
}

bool VM::call(value callee, int arity) {
	auto* fun = (objFunction*)AS_OBJ(callee);
	callFrames[callDepth].bottom = activeCallFrameBottom;
	callFrames[callDepth].func = activeFunc;
	callFrames[callDepth].top = stackTop - arity;
	callFrames[callDepth].returnPtr = ip;
	callDepth++;

	if (callDepth >= FRAMES_MAX)
		return runtimeError("stack overflow");

	if (arity != fun->getArity())
		return runtimeError("expected ", fun->getArity(), " arguments, but got ", arity);

	/*auto retAdr = reinterpret_cast<uintptr_t>(ip);
	//fun->retAddress = retAdr;
	value replacing = peek(arity);
	if (AS_OBJ(replacing)->getType() == OBJ_THIS) {
		((objThis*)AS_OBJ(replacing))->retAddress = retAdr;
	}
	else {
		value tmp = RET_VAL(retAdr);
		peek_set(arity, tmp);
	}*/
	activeCallFrameBottom = stackTop - arity;
	activeFunc = fun;
	ip = activeFunc->getChunkPtr()->getInstructionPointer();

	return true;
}

bool VM::callValue(value callee, int arity) {
	if (IS_OBJ(callee)) {
		obj* cal = AS_OBJ(callee);

		if (cal->getType() == OBJ_FUN)
			return call(callee, arity);

		if (cal->getType() == OBJ_NAT_FUN) {
			bool success = true;
			value result = ((objNativeFunction*)cal)->fun(arity, stackTop - arity, success);
			stackTop -= arity + 1;
			push(result);
			if (success) {
				return true;
			}
			return runtimeError(((objString*)AS_OBJ(result))->getChars());
		}

		if (cal->getType() == OBJ_CLASS) {
			auto* cl = (objClass*)AS_OBJ(peek(arity));
			auto instance = objInstance::createInstance(cl);

			bool success = true;
			value initFunc = cl->tableGet(globalMemory.initString);
			if (IS_FUN(initFunc)) {
				//marking instance, so GC doesnt delete it
				instance->mark();
				peek_set(arity, OBJ_VAL(instance));
				//popping instance again
				instance->unmark();
				success = call(initFunc, arity);
			}
			else if (IS_OBJ(initFunc) && AS_OBJ(initFunc)->getType() == OBJ_NAT_FUN) {
				instance->mark();
				peek_set(arity, OBJ_VAL(instance));
				//popping instance again
				instance->unmark();
				success = callValue(initFunc, arity);
			}
			else {
				if (arity > 0) {
					return runtimeError("can't call default constructor with arguments");
				}
				pop();
				push(OBJ_VAL(instance));
			}
			return success;
		}
	}
	return runtimeError("can only call functions or classes");
}

void VM::defineMethod() {
	value method = peek(0);
	objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));
	objClass* cl = (objClass*)AS_OBJ(peek(1));

	((objFunction*)AS_OBJ(method))->setClass(cl);

	cl->tableSet(name, method);
	pop();
}

bool VM::defineMemberVar() {
	value val = peek(0);

	if (IS_NIL(val))
		return runtimeError("cannot initialize member variables with 'nil'");

	//value tmp = peek(1);
	objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));
	objClass* cl = (objClass*)AS_OBJ(peek(1));

	cl->tableSet(name, val);
	pop();

	return true;
}

bool VM::invoke() {
	objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));
	int argc = readByte();
	value callee = peek(argc);

	if (!IS_OBJ(callee)) {
		return runtimeError("can only invoke methods on objects");
	}

	switch (AS_OBJ(callee)->getType()) {
	case OBJ_INSTANCE: {
		auto* instance = (objInstance*)AS_OBJ(callee);

		value method = instance->tableGet(name);

		// instance should already be at position of 'this'
		// peek_set(argc, OBJ_VAL(instance));

		if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_FUN) {
			return call(method, argc);
		}
		else if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_NAT_FUN) {
			return callValue(method, argc);
		}
		return runtimeError("no function with name '", name->getChars(), "' on instance");
		break;
	}
	case OBJ_STR: {
		value method = stringClass->tableGet(name);

		// instance should already be at position of 'this'
		// peek_set(argc, OBJ_VAL(instance));

		if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_FUN) {
			return call(method, argc);
		}
		else
			if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_NAT_FUN) {
				bool success = true;
				value result = ((objNativeFunction*)(AS_OBJ(method)))->fun(argc, stackTop - argc, success);
				stackTop -= argc + 1;
				push(result);
				if (!success)
					return runtimeError(((objString*)AS_OBJ(result))->getChars());
				return success;
			}
			else {
				return runtimeError("no function with name '", name->getChars(), "' on string class");
			}
		break;
	}
	case OBJ_LIST: {
		value method = arrayClass->tableGet(name);

		// instance should already be at position of 'this'
		// peek_set(argc, OBJ_VAL(instance));

		if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_FUN) {
			return call(method, argc);
		}
		else
			if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_NAT_FUN) {
				bool success = true;
				value result = ((objNativeFunction*)(AS_OBJ(method)))->fun(argc, stackTop - argc, success);
				stackTop -= argc + 1;
				push(result);
				if (!success)
					return runtimeError(((objString*)AS_OBJ(result))->getChars());
				return success;
			}
			else {
				return runtimeError("no function with name '", name->getChars(), "' on array class");
			}
		break;;
	}
	case OBJ_FILE: {
		value method = fileClass->tableGet(name);

		// instance should already be at position of 'this'
		// peek_set(argc, OBJ_VAL(instance));

		if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_FUN) {
			return call(method, argc);
		}
		else
			if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_NAT_FUN) {
				bool success = true;
				value result = ((objNativeFunction*)(AS_OBJ(method)))->fun(argc, stackTop - argc, success);
				stackTop -= argc + 1;
				push(result);
				if (!success)
					return runtimeError(((objString*)AS_OBJ(result))->getChars());
				return success;
			}
			else {
				return runtimeError("no function with name '", name->getChars(), "' on file class");
			}
		break;
	}
	case OBJ_CLASS: {
		auto klass = (objClass*)AS_OBJ(callee);

		value method = klass->tableGet(name);

		if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_NAT_FUN) {
			bool success = true;
			//popping class
			value result = ((objNativeFunction*)AS_OBJ(method))->fun(argc, stackTop - argc, success);
			stackTop -= argc + 1;
			push(result);
			if (!success)
				return runtimeError(((objString*)AS_OBJ(result))->getChars());
			return success;
		}

		if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_FUN) {
			return call(method, argc);
		}
		return runtimeError("no function with name '", name->getChars(), "' on class");
		break;
		break;
	}
	default:
		return runtimeError("unknown error");
	}
	return runtimeError("unknown error");
}

bool VM::superInvoke() {
	//TODO: profile super invoke (and normal super call)
	objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));
	int argc = readByte();
	value callee = peek(argc);

	// auto* instance = (objInstance*)AS_OBJ(callee);

	// value this_val = OBJ_VAL(objThis::createObjThis(instance));

	//TODO: access activeFunc
	value method = activeFunc->getClass()->superTableGet(name);
	//value method = ((objThis*)this_val.as.object)->accessSuperClassVariable(name);

	// instance should be at correct position
	// peek_set(argc, this_val);

	if (IS_OBJ(method) && AS_OBJ(method)->getType() == OBJ_FUN) {
		return call(method, argc);
	}

	return runtimeError("no function with name '", name->getChars(), "' on superclass");
}

static inline bool validateIndex(double& index, size_t len) {
	if (index >= len) {
		return false;
	}

	if (index < 0) {
		index = len + index;
		if (index >= len) {
			return false;
		}
	}

	return true;
}

bool VM::getObjectIndex(obj* object, value index) {
	switch (object->getType()) {
	case OBJ_LIST: {
		if (!IS_NUM(index))
			return runtimeError("list index must be a number");

		auto* list = (objList*)object;
		size_t len = list->getSize();
		if (!validateIndex(AS_NUM(index), len)) {
			return runtimeError("invalid index, list has size '", len, "'");
		}
		pop();
		push(list->getValueAt(AS_NUM(index)));
		return true;
		break;
	}
	case OBJ_STR: {
		//TODO: get char at
		return runtimeError("TODO: get char at");
		break;
	}
	case OBJ_MAP: {
		auto* map = (objMap*)object;
		value res = map->getValueAt(index);

		pop();
		push(res);
		return true;
		break;
	}
	default:
		return runtimeError("can only index list or string objects");
	}

	return false;
}

bool VM::setObjectIndex(obj* object, value index, value val) {
	switch (object->getType()) {
	case OBJ_LIST: {
		if (!IS_NUM(index))
			return runtimeError("list index must be a number");

		auto* list = (objList*)object;
		size_t len = list->getSize();
		if (!validateIndex(AS_NUM(index), len)) {
			return runtimeError("invalid index, list has size '", len, "'");
		}
		list->setValueAt(AS_NUM(index), val);
		return true;
	}
	case OBJ_MAP: {
		auto* map = (objMap*)object;
		map->insertElement(index, val);
		return true;
	}
	default:
		return runtimeError("can only set index of lists");
	}
}

bool VM::importFile(const char* filename) {

	std::string path = this->currentPath + filename;
	// path = path.substr(0, path.size() - 1);
	return runImportFile(path.data(), *this);
}

exitCodes VM::run() {
	ip = activeFunc->getChunkPtr()->getInstructionPointer();
	char c;
	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		std::cout << "\t";
		for (int i = 0; i < stackTop - stack; ++i) {
			std::cout << " [" << std::setw(6) << std::left << stack[i] << "] ";
		}
		std::cout << std::endl;
		debug::disassembleInstruction(*ip, activeFunc->getChunkPtr(), (ip - activeFunc->getChunkPtr()->getInstructionPointer()));
#endif
		switch (c = readByte()) {
		case OP_CONSTANT: {
			short ind = readShort();
			push(activeFunc->getChunkPtr()->getConstant(ind));
			break;
		}
		case OP_INCREMENT_GLOBAL: {
			objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));

			if (!globals.count(name)) {
				runtimeError("no variable with name '", name->getChars(), "'");
				return INTERPRET_RUNTIME_ERROR;
			}

			value var = globals.at(name);

			if (!IS_NUM(var)) {
				runtimeError("can only increment numbers");
				return INTERPRET_RUNTIME_ERROR;
			}

			AS_NUM(var)++;
			globals.insert_or_assign(name, var);
			break;
		}
		case OP_DECREMENT_GLOBAL: {
			objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));

			if (!globals.count(name)) {
				runtimeError("no variable with name '", name->getChars(), "'");
				return INTERPRET_RUNTIME_ERROR;
			}

			value var = globals.at(name);

			if (!IS_NUM(var)) {
				runtimeError("can only increment numbers");
				return INTERPRET_RUNTIME_ERROR;
			}

			AS_NUM(var)--;
			globals.insert_or_assign(name, var);
			break;
		}
		case OP_INCREMENT_LOCAL: {
			int index = readShort();
			if (!IS_NUM(activeCallFrameBottom[index])) {
				runtimeError("can only increment numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			AS_NUM(activeCallFrameBottom[index])++;
			break;
		}
		case OP_DECREMENT_LOCAL: {
			int index = readShort();
			if (!IS_NUM(activeCallFrameBottom[index])) {
				runtimeError("can only increment numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			AS_NUM(activeCallFrameBottom[index])--;
			break;
		}
		case OP_ADD:
			if (!add()) {
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		case OP_SUB:
			if (!sub())
				return INTERPRET_RUNTIME_ERROR;
			break;
		case OP_MUL:
			if (!mul())
				return INTERPRET_RUNTIME_ERROR;
			break;
		case OP_DIV:
			if (!div())
				return INTERPRET_RUNTIME_ERROR;
			break;
		case OP_MODULO: {
			if (!modulo()) {
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_TRUE:
			push(TRUE_VAL);
			break;
		case OP_FALSE:
			push(FALSE_VAL);
			break;
		case OP_NIL:
			push(NIL_VAL);
			break;
		case OP_NEGATE: {
			if (!IS_NUM((*(stackTop - 1)))) {
				runtimeError("can't negate ", *(stackTop - 1));
				return INTERPRET_RUNTIME_ERROR;
			}
			AS_NUM((*(stackTop - 1))) *= -1;
			break;
		}
		case OP_NOT:
			if (isFalsey(pop()))
				push(FALSE_VAL);
			else
				push(TRUE_VAL);
			break;
		case OP_EQUALS:
			if (areEqual(pop(), pop()))
				push(TRUE_VAL);
			else
				push(FALSE_VAL);
			break;
		case OP_CASE_COMPARE: {
			value caseVal = pop();
			if (areEqual(caseVal, peek(0)))
				push(TRUE_VAL);
			else
				push(FALSE_VAL);
			break;
		}
		case OP_NOT_EQUALS:
			if (areEqual(pop(), pop()))
				push(FALSE_VAL);
			else
				push(TRUE_VAL);
			break;
		case OP_LESSER: {
			if (IS_NUM(peek(0)) && IS_NUM(peek(1))) {
				double b = AS_NUM(pop());
				double a = AS_NUM(pop());
				if (a < b)
					push(TRUE_VAL);
				else
					push(FALSE_VAL);
			}
			else {
				runtimeError("can only use '<' on numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_LESSER_OR_EQUALS: {
			if (IS_NUM(peek(0)) && IS_NUM(peek(1))) {
				double b = AS_NUM(pop());
				double a = AS_NUM(pop());
				if (a <= b)
					push(TRUE_VAL);
				else
					push(FALSE_VAL);
			}
			else {
				runtimeError("can only use '<=' on numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_GREATER: {
			if (IS_NUM(peek(0)) && IS_NUM(peek(1))) {
				double b = AS_NUM(pop());
				double a = AS_NUM(pop());
				if (a > b)
					push(TRUE_VAL);
				else
					push(FALSE_VAL);
			}
			else {
				runtimeError("can only use '>' on numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_GREATER_OR_EQUALS: {
			if (IS_NUM(peek(0)) && IS_NUM(peek(1))) {
				double b = AS_NUM(pop());
				double a = AS_NUM(pop());
				if (a >= b)
					push(TRUE_VAL);
				else
					push(FALSE_VAL);
			}
			else {
				runtimeError("can only use '>=' on numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
								 // bitwise operations
		case OP_BIT_AND: {
			value b = pop();
			value a = pop();
			if (!IS_INT(a) || !IS_INT(b)) {
				runtimeError("bitwise operations can only be done on whole numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			long long result = AS_INT(a) & AS_INT(b);
			push(NUM_VAL(double(result)));
			break;
		}
		case OP_BIT_OR: {
			value b = pop();
			value a = pop();
			if (!IS_INT(a) || !IS_INT(b)) {
				runtimeError("bitwise operations can only be done on whole numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			long long result = AS_INT(a) | AS_INT(b);
			push(NUM_VAL(double(result)));
			break;
		}
		case OP_BIT_SHIFT_LEFT: {
			value b = pop();
			value a = pop();
			if (!IS_INT(a) || !IS_INT(b)) {
				runtimeError("bitwise operations can only be done on whole numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			long long result = AS_INT(a) << AS_INT(b);
			push(NUM_VAL(double(result)));
			break;
		}
		case OP_BIT_SHIFT_RIGHT: {
			value b = pop();
			value a = pop();
			if (!IS_INT(a) || !IS_INT(b)) {
				runtimeError("bitwise operations can only be done on whole numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			long long result = AS_INT(a) >> AS_INT(b);
			push(NUM_VAL(double(result)));
			break;
		}
		case OP_BIT_NOT: {
			value a = pop();
			if (!IS_INT(a)) {
				runtimeError("bitwise operations can only be done on whole numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			long long result = ~AS_INT(a);
			push(NUM_VAL(double(result)));
			break;
		}
		case OP_BIT_XOR: {
			value b = pop();
			value a = pop();
			if (!IS_INT(a) || !IS_INT(b)) {
				runtimeError("bitwise operations can only be done on whole numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			long long result = AS_INT(a) ^ AS_INT(b);
			push(NUM_VAL(double(result)));
			break;
		}
		case OP_DEFINE_GLOBAL: {
			objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));
			globals.insert_or_assign(name, peek(0));
			pop();
			break;
		}
		case OP_GET_GLOBAL: {
			chunk* cPtr = activeFunc->getChunkPtr();
			short index = readShort();
			objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(index));

			if (!globals.count(name)) {
				runtimeError("no variable with name '", name->getChars(), "'");
				return INTERPRET_RUNTIME_ERROR;
			}

			push(globals.at(name));
			break;
		}
		case OP_SET_GLOBAL: {
			objString* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));
			if (!globals.count(name)) {
				runtimeError("no global variable with name '", name->getChars(), "'");
				return INTERPRET_RUNTIME_ERROR;
			}
			globals.insert_or_assign(name, peek(0));
			break;
		}
		case OP_GET_LOCAL: {
			push(activeCallFrameBottom[readShort()]);
			break;
		}
		case OP_SET_LOCAL: {
			activeCallFrameBottom[readShort()] = peek(0);
			break;
		}
		case OP_SET_PROPERTY: {
			short ind = readShort();
			if (IS_OBJ(peek(1))) {
				switch (AS_OBJ(peek(1))->getType()) {
				case OBJ_INSTANCE: {
					auto* instance = (objInstance*)AS_OBJ(peek(1));
					auto* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(ind));
					value val = pop();
					if (IS_NIL(val)) {
						instance->tableDelete(name);
					}
					else {
						instance->tableSet(name, val);
					}
					break;
				}
				case OBJ_CLASS: {
					runtimeError("classes can't be modified");
					return INTERPRET_RUNTIME_ERROR;
					break;
				}
				default:
					runtimeError("only instances have properties");
					return INTERPRET_RUNTIME_ERROR;
					break;
				}
			}
			else {
				runtimeError("only instances have properties");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_GET_PROPERTY: {
			short ind = readShort();
			if (IS_OBJ(peek(0))) {
				switch (AS_OBJ(peek(0))->getType()) {
				case OBJ_INSTANCE: {
					auto* instance = (objInstance*)AS_OBJ(peek(0));
					auto* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(ind));
					value val = instance->tableGet(name);

					pop();
					push(val);
					break;
				}
				case OBJ_CLASS: {
					auto* klass = (objClass*)AS_OBJ(peek(0));
					auto* name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(ind));
					value val = klass->tableGet(name);

					pop();
					push(val);
					break;
				}
				default:
					runtimeError("can only access objects");
					return INTERPRET_RUNTIME_ERROR;
				}
			}
			else {
				runtimeError("only instances have properties");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_JUMP_IF_FALSE: {
			int offset = readShort();
			if (!isFalsey(peek(0))) {
				ip += offset;
			}
			break;
		}
		case OP_JUMP: {
			int offset = readShort();
			ip += offset;
			break;
		}
		case OP_LOOP: {
			int offset = readShort();
			ip -= offset;
			break;
		}
		case OP_FOR_EACH_INIT: {
			value counter = peek(2);
			//value var = peek(1);
			value itOver = peek(0);

			if (!(IS_OBJ(itOver) && AS_OBJ(itOver)->getType() == OBJ_LIST)) {
				runtimeError("cannot iterate over variable");
				return INTERPRET_RUNTIME_ERROR;
			}

			objList* arr = ((objList*)AS_OBJ(itOver));

			peek_set(2, NUM_VAL(0.0));

			if (arr->getSize() > 0) {
				peek_set(1, arr->getValueAt(0));
			}
			break;
		}
		case OP_FOR_ITER: {
			value counter = peek(2);
			//value x_in = peek(1);
			objList* arr = (objList*)AS_OBJ(peek(0));

			if (AS_NUM(counter) >= arr->getSize()) {
				push(FALSE_VAL);
			}
			else {
				AS_NUM(counter)++;
				peek_set(1, arr->getValueAt(AS_NUM(counter) - 1));
				peek_set(2, counter);
				push(TRUE_VAL);
			}

			break;
		}
		case OP_CALL: {
			int arity = (unsigned char)readByte();
			if (!callValue(peek(arity), arity)) {
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_CLASS: {
			objString* klassName = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));
			objClass* klass = objClass::createObjClass(klassName);
			push(OBJ_VAL(klass));
			break;
		}
		case OP_INHERIT: {
			value superKlass = pop();
			if (!(IS_OBJ(superKlass) && AS_OBJ(superKlass)->getType() == OBJ_CLASS)) {
				runtimeError("can only inherit from other classes");
				return INTERPRET_RUNTIME_ERROR;
			}
			value klass = peek(0);
			((objClass*)AS_OBJ(klass))->setSuperClass((objClass*)AS_OBJ(superKlass));
			break;
		}
		case OP_MEMBER_VARIABLE:
			if (!defineMemberVar())
				return INTERPRET_RUNTIME_ERROR;
			break;
		case OP_METHOD: {
			defineMethod();
			break;
		}
		case OP_INVOKE:
			if (!invoke())
				return INTERPRET_RUNTIME_ERROR;
			break;
		case OP_POP: {
			pop();
			break;
		}
		case OP_LIST: {
			push(OBJ_VAL(objList::createList()));
			break;
		}
		case OP_APPEND: {
			size_t len = readSizeT();
			objList* list = (objList*)AS_OBJ(peek(len));

			for (size_t i = 1; i <= len; i++)
			{
				value val = peek(len - i);
				list->appendValue(val);
			}

			for (size_t i = 0; i < len; i++) {
				pop();
			}

			break;
		}
		case OP_MAP: {
			push(OBJ_VAL(objMap::createMap()));
			break;
		}
		case OP_MAP_APPEND: {
			size_t len = readSizeT();
			objMap* map = (objMap*)AS_OBJ(peek(len * 2));

			for (size_t i = 1; i <= len * 2; i += 2)
			{
				value key = peek(len * 2 - i);

				value val = peek(len * 2 - i - 1);

				map->insertElement(key, val);
			}

			for (size_t i = 0; i < len; i++) {
				pop();
				pop();
			}

			break;
		}
		case OP_GET_INDEX: {
			value index = pop();
			value list = peek(0);
			if (!(IS_OBJ(list))) {
				runtimeError("can only index '", list, "'");
				return INTERPRET_RUNTIME_ERROR;
			}
			if (!getObjectIndex(AS_OBJ(list), index))
				return INTERPRET_RUNTIME_ERROR;
			break;
		}
		case OP_SET_INDEX: {

			value val = pop();
			value index = pop();
			value list = peek(0);
			if (!(IS_OBJ(list))) {
				runtimeError("can only index list and string objects");
				return INTERPRET_RUNTIME_ERROR;
			}
			if (!setObjectIndex(AS_OBJ(list), index, val))
				return INTERPRET_RUNTIME_ERROR;
			break;
		}
		case OP_THIS: {
			value this_val = activeCallFrameBottom[-1];
			if (!(IS_OBJ(this_val) && AS_OBJ(this_val)->getType() != OBJ_FUN)) {
				runtimeError("no valid 'this' object");
				return INTERPRET_RUNTIME_ERROR;
			}
			push(this_val);
			break;
		}
		case OP_SUPER: {
			auto name = (objString*)AS_OBJ(activeFunc->getChunkPtr()->getConstant(readShort()));

			//TODO: access activeFuncs superclass -- add class to methods
			if (!activeFunc->isMethod()) {
				runtimeError("no superclass");
				return INTERPRET_RUNTIME_ERROR;
			}
			push(activeFunc->getClass()->superTableGet(name));
			break;
		}
		case OP_SUPER_INVOKE: {
			if (!superInvoke())
				return INTERPRET_RUNTIME_ERROR;
			break;
		}
		case OP_IMPORT: {
			value fileName = pop();
			if (!(IS_OBJ(fileName) && AS_OBJ(fileName)->getType() == OBJ_STR)) {
				runtimeError("importname must be a string");
				return INTERPRET_RUNTIME_ERROR;
			}
			auto fileNameStr = (objString*)AS_OBJ(fileName);
			auto prevIP = ip;
			if (!importFile(fileNameStr->getChars()))
				return INTERPRET_RUNTIME_ERROR;
			ip = prevIP;

			break;
		}
		case OP_RETURN:
			if (callDepth == 0) {
				return INTERPRET_OK;
			}
			else {
				value retVal = pop();
				callDepth--;
				activeCallFrameBottom = callFrames[callDepth].bottom;
				stackTop = callFrames[callDepth].top;
				activeFunc = callFrames[callDepth].func;
				pop(); // popping the callee
				//uintptr_t returnAddress = ((objFunction*)(tmpRetAdd.as.object))->retAddress;
				ip = callFrames[callDepth].returnPtr;

				push(retVal);
			}
			break;
		}
	}
}

template<typename... Ts>
bool VM::runtimeError(const char* msg, Ts... args) {

	unsigned int line = activeFunc->getChunkPtr()->getLine((ip - activeFunc->getChunkPtr()->getInstructionPointer()));

	std::cerr << "[line " << line << "] -> " << msg;
	(std::cerr << ... << args);
	std::cerr << std::endl;

	//simple stack traceback
	if (callDepth > 0) {
		std::cerr << "in function -> " << activeFunc->name->getChars() << std::endl;

		auto prevName = activeFunc->name->getChars();

		for (size_t i = callDepth - 1; i > 0; i--) {
			std::cerr << prevName << " <- " << callFrames[i].func->name->getChars() << std::endl;
			prevName = callFrames[i].func->name->getChars();
		}
	}
	return false;
}

value VM::replGetLast() {
	// resettin stack, in case of errors
	stackTop = stack;
	return stack[0];
}
