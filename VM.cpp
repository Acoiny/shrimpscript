#include "VM.hpp"

#include "nativeFunctions.hpp"

#include "runFile.hpp"

#ifdef DEBUG_TRACE_EXECUTION
#include <iomanip>
#endif

memoryManager globalMemory;

VM::VM() : ip(), activeFunc(nullptr), memory(globalMemory), callFrames() {
    memory.setVM(this);
    currentCompiler = new compiler(*this);

    stack = new value[STACK_MAX];
    activeCallFrameBottom = stack;
    stackTop = stack;

    nativeFunctions::initNatives(*this);
}

void VM::interpret(char *str) {
    activeFunc = currentCompiler->compiling("script", str);
    scriptFuncs.push_back(activeFunc);
    gcReady = true;
    if(!currentCompiler->errorOccured()) {
        run();
    }
}

void VM::interpretImportFile(const char* name, char* str) {
    compiler* prevCompiler = currentCompiler;

    currentCompiler = new compiler(*this);

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


void VM::concatenateTwoStrings() {
    auto *b = (objString *) AS_OBJ(peek(0));
    auto *a = (objString *) AS_OBJ(peek(1));

    unsigned int len = a->getLen() + b->getLen();
    char *res = memory.allocateArray<char>(len + 1);
    memcpy(res, a->getChars(), a->getLen());
    memcpy(res + a->getLen(), b->getChars(), b->getLen());
    res[len] = '\0';

    objString *strObj = nullptr;
    push(OBJ_VAL(strObj));
    strObj = objString::takeString(res, len);
    pop();
    pop();
    pop();
    push(OBJ_VAL(strObj));
}

bool VM::add() {
    value b = peek(0);
    value a = peek(1);
    if (IS_OBJ(a) && b.getType() == VAL_OBJ) {
        if ((a.as.object)->getType() == OBJ_STR && (b.as.object)->getType() == OBJ_STR) {
            concatenateTwoStrings();
            return true;
        } else {
            return runtimeError("can't add '", a, "' and '", b, "'");
        }
    }
    if (!(a.getType() == VAL_NUM && b.getType() == VAL_NUM))
        return runtimeError("can't add '", a, "' and '", b, "'");
    b = pop();
    a = pop();
    push(value(a.as.number + b.as.number));
    return true;
}

bool VM::sub() {
    value b = pop();
    value a = pop();
    if (!(a.getType() == VAL_NUM && b.getType() == VAL_NUM))
        return runtimeError("can't subtract '", b, "' from '", a, "'");
    push(value(a.as.number - b.as.number));
    return true;
}

bool VM::mul() {
    value b = pop();
    value a = pop();
    if (!(a.getType() == VAL_NUM && b.getType() == VAL_NUM))
        return runtimeError("can't multiply '", a, "' and '", b, "'");
    push(value(a.as.number * b.as.number));
    return true;
}

bool VM::div() {
    value b = pop();
    value a = pop();
    if (!(a.getType() == VAL_NUM && b.getType() == VAL_NUM))
        return runtimeError("can't divide '", a, "' by '", b, "'");
    push(value(a.as.number / b.as.number));
    return true;
}

bool VM::modulo() {
    value b = pop();
    value a = pop();
    if (!(a.getType() == VAL_NUM && b.getType() == VAL_NUM))
        return runtimeError("can't get modulo of '", a, "' and '", b, "'");

    long tmp = a.as.number / b.as.number;

    push(value(a.as.number - (tmp * b.as.number)));
    return true;
}

bool VM::isFalsey(value a) {
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
            return str->getLen() == 0;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

bool VM::areEqual(value b, value a) {
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

    return false;
}

bool VM::call(value callee, int arity) {
    auto *fun = (objFunction*)callee.as.object;
    callFrames[callDepth].bottom = activeCallFrameBottom;
    callFrames[callDepth].func = activeFunc;
    callFrames[callDepth].top = stackTop - arity;
    callDepth++;

    if (callDepth > FRAMES_MAX)
        return runtimeError("stack overflow");

    if (arity != fun->getArity())
        return runtimeError("expected ", fun->getArity(), " arguments, but got ", arity);

    auto retAdr = reinterpret_cast<uintptr_t>(ip);
    //fun->retAddress = retAdr;
    value replacing = peek(arity);
    if (replacing.as.object->getType() == OBJ_THIS) {
        ((objThis*)replacing.as.object)->retAddress = retAdr;
    }
    else {
        value tmp = value(retAdr);
        peek_set(arity, tmp);
    }
    activeCallFrameBottom = stackTop - arity;
    activeFunc = fun;
    ip = activeFunc->getChunkPtr()->getInstructionPointer();

    return true;
}

bool VM::callValue(value callee, int arity) {
    if(callee.getType() == VAL_OBJ) {
        switch (callee.as.object->getType())
        {
        case OBJ_FUN:
            return call(callee, arity);
        case OBJ_NAT_FUN: {
            bool success = true;
            value result = ((objNativeFunction*)callee.as.object)->fun(arity, stackTop - arity, success);
            stackTop -= arity + 1;
            push(result);
            if (success) {
                return true;
            }
            return runtimeError(((objString*)result.as.object)->getChars());
        }
        case OBJ_CLASS: {
            auto* cl = (objClass*)peek(arity).as.object;
            auto instance = objInstance::createInstance(cl);

            bool success = true;
            if (cl->hasInitFunction()) {
                //marking instance, so GC doesnt delete it
                instance->mark();
                peek_set(arity, value(objThis::createObjThis(instance)));
                //popping instance again
                instance->unmark();
                success = call(instance->tableGet(globalMemory.initString), arity);
            }
            else {
                if (arity > 0) {
                    return runtimeError("can't call default constructor with arguments");
                }
                pop();
                push(value(instance));
            }
            return success;
        }
        default:
            break;
        }
    }
    return runtimeError("can only call functions or classes");
}

void VM::defineMethod() {
    value method = peek(0);
    objString* name = (objString*)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;
    objClass* cl = (objClass*)peek(1).as.object;

    ((objFunction*)method.as.object)->setClass(cl);

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
    objString* name = (objString*)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;
    int argc = readByte();
    value callee = peek(argc);
    
    if (callee.getType() != VAL_OBJ) {
        return runtimeError("can only invoke methods on objects");
    }

    switch (callee.as.object->getType()) {
    case OBJ_INSTANCE: {
        auto* instance = (objInstance*)callee.as.object;

        value method = instance->tableGet(name);

        peek_set(argc, value(objThis::createObjThis(instance)));

        if (method.getType() == VAL_OBJ && method.as.object->getType() == OBJ_FUN) {
            return call(method, argc);
        }
        return runtimeError("no function with name '", name->getChars(), "' on instance");
        break;
    }
    case OBJ_NAT_INSTANCE: {
        auto* instance = (objNativeInstance*)callee.as.object;

        value method = instance->tableGet(name);

        if (method.getType() == VAL_OBJ && method.as.object->getType() == OBJ_NAT_FUN) {
            bool success = true;
            value result = ((objNativeFunction*)method.as.object)->fun(argc, stackTop - argc, success);
            stackTop -= argc + 1;
            push(result);
            if (success) {
                return true;
            }
            return runtimeError(((objString*)result.as.object)->getChars());
        }
        break;
    }
    case OBJ_STR: {
        if (stringFunctions.find(name) != stringFunctions.end()) {
            bool success = true;
            value result = ((objNativeFunction*)(stringFunctions.at(name).as.object))->fun(argc, stackTop - argc - 1, success);
            stackTop -= argc + 1;
            push(result);
            if(!success)
                return runtimeError(((objString*)result.as.object)->getChars());
            return success;
        }
        else {
            return runtimeError("no function with name '", name->getChars(), "' for strings");
        }
        break;
    }
    case OBJ_LIST: {
        if (arrayFunctions.find(name) != arrayFunctions.end()) {
            bool success = true;
            value result = ((objNativeFunction*)(arrayFunctions.at(name).as.object))->fun(argc, stackTop - argc - 1, success);
            stackTop -= argc + 1;
            push(result);
            if (!success)
                return runtimeError(((objString*)result.as.object)->getChars());
            return success;
        }
        else {
            return runtimeError("no function with name '", name->getChars(), "' for lists");
        }
        break;
    }
    case OBJ_FILE: {
        if (fileFunctions.find(name) != fileFunctions.end()) {
            bool success = true;
            //objNativeFunction* funnn = (objNativeFunction*)fileFunctions.at(name).as.object;
            value result = ((objNativeFunction*)(fileFunctions.at(name).as.object))->fun(argc, stackTop - argc - 1, success);
            stackTop -= argc + 1;
            push(result);
            if (!success)
                return runtimeError(((objString*)result.as.object)->getChars());
            return success;
        }
        else {
            return runtimeError("no function with name '", name->getChars(), "' for files");
        }
        break;
    }
    case OBJ_CLASS: {
        auto klass = (objClass*)callee.as.object;

        value method = klass->tableGet(name);

        if (method.getType() == VAL_OBJ && method.as.object->getType() == OBJ_FUN) {
            return call(method, argc);
        }
        return runtimeError("no function with name '", name->getChars(), "' on instance");
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
    objString* name = (objString*)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;
    int argc = readByte();
    value callee = peek(argc);

    auto* instance = (objInstance*)callee.as.object;

    value this_val = value(objThis::createObjThis(instance));

    //TODO: access activeFunc
    value method = activeFunc->getClass()->superTableGet(name);
    //value method = ((objThis*)this_val.as.object)->accessSuperClassVariable(name);

    peek_set(argc, this_val);

    if (method.getType() == VAL_OBJ && method.as.object->getType() == OBJ_FUN) {
        return call(method, argc);
    }

    return runtimeError("no function with name '", name->getChars(), "' on superclass");
}

static bool validateIndex(double &index, size_t len) {
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

bool VM::getObjectIndex(obj* object, value &index) {
    switch (object->getType()) {
    case OBJ_LIST: {
        if (index.getType() != VAL_NUM)
            return runtimeError("list index must be a number");

        auto* list = (objList*)object;
        size_t len = list->getSize();
        if (!validateIndex(index.as.number, len)) {
            return runtimeError("invalid index, list has size '", len, "'");
        }
        pop();
        push(list->getValueAt(index.as.number));
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

bool VM::setObjectIndex(obj* object, value &index, value& val) {
    switch (object->getType()) {
    case OBJ_LIST: {
        if (index.getType() != VAL_NUM)
            return runtimeError("list index must be a number");

        auto* list = (objList*)object;
        size_t len = list->getSize();
        if (!validateIndex(index.as.number, len)) {
            return runtimeError("invalid index, list has size '", len, "'");
        }
        list->getValueAt(index.as.number) = val;
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

    return runImportFile(filename, *this);
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
                objString* name = (objString*)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;

                if (!globals.count(name)) {
                    runtimeError("no variable with name '", name->getChars(), "'");
                    return INTERPRET_RUNTIME_ERROR;
                }

                value var = globals.at(name);

                if (var.getType() != VAL_NUM) {
                    runtimeError("can only increment numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }

                var.as.number++;
                globals.insert_or_assign(name, var);
                break;
            }
            case OP_DECREMENT_GLOBAL: {
                objString* name = (objString*)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;

                if (!globals.count(name)) {
                    runtimeError("no variable with name '", name->getChars(), "'");
                    return INTERPRET_RUNTIME_ERROR;
                }

                value var = globals.at(name);

                if (var.getType() != VAL_NUM) {
                    runtimeError("can only increment numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }

                var.as.number--;
                globals.insert_or_assign(name, var);
                break;
            }
            case OP_INCREMENT_LOCAL: {
                int index = readShort();
                if (activeCallFrameBottom[index].getType() != VAL_NUM) {
                    runtimeError("can only increment numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                activeCallFrameBottom[index].as.number++;
                break;
            }
            case OP_DECREMENT_LOCAL: {
                int index = readShort();
                if (activeCallFrameBottom[index].getType() != VAL_NUM) {
                    runtimeError("can only increment numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                activeCallFrameBottom[index].as.number++;
                break;
            }
            case OP_ADD:
                if (!add())
                    return INTERPRET_RUNTIME_ERROR;
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
                push(value(true));
                break;
            case OP_FALSE:
                push(value(false));
                break;
            case OP_NIL:
                push(value());
                break;
            case OP_NEGATE: {
                if ((stackTop - 1)->getType() != VAL_NUM) {
                    runtimeError("can't negate ", *(stackTop - 1));
                    return INTERPRET_RUNTIME_ERROR;
                }
                (stackTop - 1)->as.number *= -1;
                break;
            }
            case OP_NOT:
                push(value(!isFalsey(pop())));
                break;
            case OP_EQUALS:
                push(value(areEqual(pop(), pop())));
                break;
            case OP_NOT_EQUALS:
                push(value(!areEqual(pop(), pop())));
                break;
            case OP_LESSER: {
                if (peek(0).getType() == VAL_NUM && peek(1).getType() == VAL_NUM) {
                    double b = pop().as.number;
                    double a = pop().as.number;
                    push(value(a < b));
                } else {
                    runtimeError("can only use '<' on numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_LESSER_OR_EQUALS: {
                if (peek(0).getType() == VAL_NUM && peek(1).getType() == VAL_NUM) {
                    double b = pop().as.number;
                    double a = pop().as.number;
                    push(value(a <= b));
                } else {
                    runtimeError("can only use '<=' on numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GREATER: {
                if (peek(0).getType() == VAL_NUM && peek(1).getType() == VAL_NUM) {
                    double b = pop().as.number;
                    double a = pop().as.number;
                    push(value(a > b));
                } else {
                    runtimeError("can only use '>' on numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GREATER_OR_EQUALS: {
                if (peek(0).getType() == VAL_NUM && peek(1).getType() == VAL_NUM) {
                    double b = pop().as.number;
                    double a = pop().as.number;
                    push(value(a >= b));
                } else {
                    runtimeError("can only use '>=' on numbers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DEFINE_GLOBAL: {
                objString *name = (objString *)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;
                globals.insert_or_assign(name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                chunk* cPtr = activeFunc->getChunkPtr();
                short index = readShort();
                objString *name = (objString *)activeFunc->getChunkPtr()->getConstant(index).as.object;

                if (!globals.count(name)) {
                    runtimeError("no variable with name '", name->getChars(), "'");
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(globals.at(name));
                break;
            }
            case OP_SET_GLOBAL: {
                objString *name = (objString *)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;
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
                if(peek(1).getType() == VAL_OBJ && peek(1).as.object->getType() == OBJ_INSTANCE) {
                    auto* instance = (objInstance*)peek(1).as.object;
                    auto* name = (objString*)activeFunc->getChunkPtr()->getConstant(ind).as.object;
                    value val = pop();
                    if(val.getType() == VAL_NIL) {
                        instance->tableDelete(name);
                    } else {
                        instance->tableSet(name, val);
                    }
                } else {
                    runtimeError("only instances have properties");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_PROPERTY: {
                short ind = readShort();
                if(peek(0).getType() == VAL_OBJ && peek(0).as.object->getType() == OBJ_INSTANCE) {
                    auto* instance = (objInstance*)peek(0).as.object;
                    auto* name = (objString*)activeFunc->getChunkPtr()->getConstant(ind).as.object;
                    value val = instance->tableGet(name);
                    if(val.getType() == VAL_NIL) {
                        runtimeError("no property of name ", name->getChars(), " on instance");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    pop();
                    push(val);
                } else {
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

                if (!(itOver.getType() == VAL_OBJ && itOver.as.object->getType() == OBJ_LIST)) {
                    runtimeError("cannot iterate over variable");
                    return INTERPRET_RUNTIME_ERROR;
                }

                objList* arr = ((objList*)itOver.as.object);
                
                peek_set(2, value(0.0));

                if (arr->getSize() > 0) {
                    peek_set(1, arr->getValueAt(0));
                }
                break;
            }
            case OP_FOR_ITER: {
                value counter = peek(2);
                //value x_in = peek(1);
                objList* arr = (objList*)(peek(0).as.object);

                if (counter.as.number >= arr->getSize()) {
                    push(value(false));
                }
                else {
                    counter.as.number++;
                    peek_set(1, arr->getValueAt(counter.as.number - 1));
                    peek_set(2, counter);
                    push(value(true));
                }

                break;
            }
            case OP_FUNCTION: {
                int constIndex = readShort();
                value fn = activeFunc->getChunkPtr()->getConstant(constIndex);

                push(fn);
                break;
            }
            case OP_CALL: {
                int arity = (unsigned char) readByte();
                if (!callValue(peek(arity), arity)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CLASS: {
                objString* klassName = (objString*)(activeFunc->getChunkPtr()->getConstant(readShort()).as.object);
                objClass* klass = objClass::createObjClass(klassName);
                push(value(klass));
                break;
            }
            case OP_INHERIT: {
                value superKlass = pop();
                if (!(superKlass.getType() == VAL_OBJ && superKlass.as.object->getType() == OBJ_CLASS)) {
                    runtimeError("can only inherit from other classes");
                    return INTERPRET_RUNTIME_ERROR;
                }
                value klass = peek(0);
                ((objClass*)(klass.as.object))->setSuperClass((objClass*)superKlass.as.object);
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
                push(value(objList::createList()));
                break;
            }
            case OP_APPEND: {
                size_t len = readSizeT();
                objList* list = (objList*)peek(len).as.object;

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
            case OP_MAP_APPEND: {
                size_t len = readSizeT();
                objMap* map = (objMap*)peek(len * 2).as.object;

                for (size_t i = 1; i <= len * 2; i+=2)
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
                if (!(list.getType() == VAL_OBJ)) {
                    runtimeError("can only index '", list,"'");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!getObjectIndex(list.as.object, index))
                    return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_SET_INDEX: {
                
                value val = pop();
                value index = pop();
                value list = peek(0);
                if (!(list.getType() == VAL_OBJ)) {
                    runtimeError("can only index list and string objects");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!setObjectIndex(list.as.object, index, val))
                    return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_THIS: {
                value this_val = activeCallFrameBottom[-1];
                if (!(this_val.getType() == VAL_OBJ && this_val.as.object->getType() == OBJ_THIS)) {
                    runtimeError("no valid 'this' object");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value(((objThis*)activeCallFrameBottom[-1].as.object)->this_instance));
                break;
            }
            case OP_SUPER: {
                auto name = (objString*)activeFunc->getChunkPtr()->getConstant(readShort()).as.object;

                //TODO: access activeFuncs superclass -- add class to methods
                if (!activeFunc->isMethod()) {
                    runtimeError("no superclass");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value(activeFunc->getClass()->superTableGet(name)));
                break;
            }
            case OP_SUPER_INVOKE: {
                if (!superInvoke())
                    return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_PULL_INSTANCE_FROM_THIS: {
                objThis* th = (objThis*)(pop().as.object);
                push(value(th->this_instance));
                break;
            }
            case OP_IMPORT: {
                value fileName = pop();
                if (!(fileName.getType() == VAL_OBJ && fileName.as.object->getType() == OBJ_STR)) {
                    runtimeError("importname must be a string");
                    return INTERPRET_RUNTIME_ERROR;
                }
                auto fileNameStr = (objString*)fileName.as.object;
                auto prevIP = ip;
                if (!importFile(fileNameStr->getChars()))
                    return INTERPRET_RUNTIME_ERROR;
                ip = prevIP;

                break;
            }
            case OP_RETURN:
                if (callDepth == 0) {
                    return INTERPRET_OK;
                } else {
                    value retVal = pop();
                    callDepth--;
                    activeCallFrameBottom = callFrames[callDepth].bottom;
                    stackTop = callFrames[callDepth].top;
                    activeFunc = callFrames[callDepth].func;
                    value tmpRetAdd = pop();
                    //uintptr_t returnAddress = ((objFunction*)(tmpRetAdd.as.object))->retAddress;
                    uintptr_t returnAddress;
                    if (tmpRetAdd.getType() == VAL_ADDRESS) {
                        returnAddress = tmpRetAdd.as.address;
                    }
                    else {
                        returnAddress = ((objThis*)tmpRetAdd.as.object)->retAddress;
                    }

                    ip = reinterpret_cast<char *>(returnAddress);
                    push(retVal);
                }
                break;
        }
    }
}

template<typename... Ts>
bool VM::runtimeError(const char *msg, Ts... args) {

    unsigned int line = activeFunc->getChunkPtr()->getLine((ip - activeFunc->getChunkPtr()->getInstructionPointer()));
    std::cerr << "[line " << line << "] -> " << msg;
    (std::cerr << ... << args);
    std::cerr << std::endl;
    return false;
}
