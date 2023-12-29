#include "../../header/virtualMachine/memoryManager.hpp"

#include "../../header/virtualMachine/VM.hpp"


memoryManager::memoryManager(VM& v) : vm(&v), initString(objString::copyString("init", 4)) {}

memoryManager::memoryManager() : vm(nullptr), initString(objString::copyString("init", 4)) {}

memoryManager::~memoryManager() {
	obj* object = allObjects;

	// std::cout << " -- before memory manager destroyed: " << bytesAllocated << std::endl;

	while (object != nullptr) {
		obj* next = object->next;
		freeObject(object);
		object = next;
	}

	// std::cout << " -- memory manager destroyed: " << bytesAllocated << std::endl;
}

void memoryManager::setVM(VM* v) {
	this->vm = v;
}

void memoryManager::addToObjects(obj* o) {
	o->next = allObjects;
	allObjects = o;
}

void memoryManager::freeObject(obj* el) {
	switch (el->type) {
	case OBJ_STR: {
		auto str = (objString*)el;
		// const char* tmpArr = str->getChars();
		bytesAllocated -= sizeof(objString);
		freeArray(str->getChars(), str->getLen() + 1);
		delete str;
		break;
	}
	case OBJ_FUN: {
		auto* fun = (objFunction*)el;
		delete fun->funChunk;
		bytesAllocated -= sizeof(objFunction);
		delete fun;
		break;
	}
	case OBJ_NAT_FUN: {
		bytesAllocated -= sizeof(objNativeFunction);
		auto* fun = (objNativeFunction*)el;
		delete fun;
		break;
	}
	case OBJ_CLASS: {
		bytesAllocated -= sizeof(objClass);
		auto* klass = (objClass*)el;
		delete klass;
		break;
	}
	case OBJ_INSTANCE: {
		bytesAllocated -= sizeof(objInstance);
		auto* ins = (objInstance*)el;
		delete ins;
		break;
	}
	case OBJ_LIST: {
		bytesAllocated -= sizeof(objList);
		auto* list = (objList*)el;
		delete list;
		break;
	}
	case OBJ_FILE: {
		bytesAllocated -= sizeof(objFile);
		auto* file = (objFile*)el;
		if (file->isOpen())
			file->file.close();
		delete file;
		break;
	}
	case OBJ_MAP: {
		bytesAllocated -= sizeof(objMap);
		auto* map = (objMap*)el;
		delete map;
		break;
	}
	case OBJ_UPVALUE: {
		bytesAllocated -= sizeof(objUpvalue);
		auto* upval = (objUpvalue*)el;
		delete upval;
		break;
	}
	case OBJ_CLOSURE: {
		bytesAllocated -= sizeof(objClosure);
		auto* clos = (objClosure*)el;
		delete clos;
		break;
	}
	}
}

void memoryManager::markObject(obj* obj) {
	if (obj == nullptr) return;
	if (obj->isMarked) return;

	obj->mark();
	grayStack.push_back(obj);
#ifdef DEBUG_LOG_GC
	std::cout << "marked : " << obj << std::endl;
#endif
}

void memoryManager::markValue(value val) {
	if (IS_OBJ(val)) {
		markObject(AS_OBJ(val));
	}
}


void memoryManager::markRoots() {
	// marking the stack
	for (value* slot = vm->stack; slot < vm->stackTop; slot++) {
		markValue(*slot);
	}

	// marking all openUpvalues
	objUpvalue* upval = vm->openUpvalues;
	while (upval) {
		markObject(upval);
		upval = upval->next;
	}

	// marking the callstack
	for (size_t i = 0; i < vm->callDepth; ++i) {
		markObject(vm->callFrames[i].closure);
	}

	// marking all globals
	for (auto& el : vm->globals) {
		markObject(el.first);
		markValue(el.second);
	}

	markObject(vm->activeClosure);

	for (auto& el : vm->scriptClosures) {
		markObject(el);
	}

	markObject(vm->stringClass);

	//markObject(vm->arrayClass);

	//markObject(vm->fileClass);

	markObject(initString);
}

void memoryManager::blackenObject(obj* obj) {
	switch (obj->getType()) {
	case OBJ_CLASS: {
		auto* cl = (objClass*)obj;
		markObject(cl->name);
		markObject(cl->superClass);
		for (auto& el : cl->table) {
			markObject(el.first);
			markValue(el.second);
		}
		break;
	}
	case OBJ_FUN: {
		auto* fn = (objFunction*)obj;
		markObject(fn->name);
		markObject(fn->klass);
		for (auto& el : fn->funChunk->constants) {
			markValue(el);
		}
		break;
	}
	case OBJ_INSTANCE: {
		auto* cl = (objInstance*)obj;
		markObject(cl->klass);
		for (auto& el : cl->table) {
			markObject(el.first);
			markValue(el.second);
		}
		break;
	}
	case OBJ_LIST: {
		objList* list = (objList*)obj;
		for (auto& el : list->data) {
			markValue(el);
		}
		break;
	}
	case OBJ_MAP: {
		objMap* map = (objMap*)obj;
		for (auto& el : map->data) {
			markValue(el.first);
			markValue(el.second);
		}
		break;
	}
	case OBJ_UPVALUE: {
		objUpvalue* upval = (objUpvalue*)obj;
		markValue(upval->closed);
		break;
	}
	case OBJ_CLOSURE: {
		objClosure* clos = (objClosure*)obj;
		markObject(clos->function);
		for (auto el : clos->upvalues) {
			markObject(el);
		}
		break;
	}
	case OBJ_NAT_FUN:
	case OBJ_STR:
	case OBJ_FILE:
		break;
	}
}

void memoryManager::traceReferences() {
	do {
		obj* object = grayStack.at(grayStack.size() - 1);
		grayStack.pop_back();
		blackenObject(object);
	} while (grayStack.size() > 0);


	/*
	long long graySize = grayStack.size();
	for (long long i = graySize - 1; i >= 0; --i) {
		obj* current = grayStack.at(i);
		grayStack.pop_back();
		blackenObject(current);
	}
	*/
}

void memoryManager::stringsRemoveWhite() {
	std::vector<const char*> tmp;

	for (const auto& el : internedStrings) {
		if (!el.second->isMark()) {
			tmp.push_back(el.first);
		}
	}

	for (const auto& el : tmp) {
		internedStrings.erase(el);
	}
}

void memoryManager::sweep() {

#ifdef DEBUG_LOG_GC
	size_t freedNum = 0;
#endif

	obj* previous = nullptr;
	obj* object = allObjects;
	while (object != nullptr) {
		//std::cout << value(object) << std::endl;
		if (object->isMarked) {
			object->isMarked = false;
			previous = object;
			object = object->next;
		}
		else {
			obj* unreached = object;
			object = object->next;
			if (previous == nullptr) {
				allObjects = object;
			}
			else {
				previous->next = object;
			}

#ifdef DEBUG_LOG_GC
			std::cout << "freed: " << unreached << std::endl;
			freedNum++;
#endif

			freeObject(unreached);
		}
	}

#ifdef DEBUG_LOG_GC
	std::cout << "objects freed: " << freedNum << std::endl;
#endif
}

void memoryManager::collectGarbage() {
	//stopping if vm is not created yet
	if (!(vm != nullptr && vm->gcReady)) {
		return;
	}


	size_t before = bytesAllocated;
#ifdef DEBUG_LOG_GC
	std::cout << " -- GC started:" << std::endl;
	size_t before = bytesAllocated;
#endif


	markRoots();
	traceReferences();
	stringsRemoveWhite();
	sweep();

	nextGC = bytesAllocated * GC_GROWTH_FACTOR;


#ifdef DEBUG_LOG_GC
	std::cout << " -- end GC: from " << before << " to " << bytesAllocated <<
		" next at " << nextGC << std::endl;
#endif
}

size_t memoryManager::getHeapSize() {
	return bytesAllocated;
}