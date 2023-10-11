#include "memoryManager.hpp"

#include "VM.hpp"


memoryManager::memoryManager(VM& v) : vm(&v), initString(objString::copyString("init",4)) {}

memoryManager::memoryManager() : vm(nullptr), initString(objString::copyString("init",4)) {}

memoryManager::~memoryManager() {
	obj* object = allObjects;
	while (object != nullptr) {
		obj* next = object->getNext();
		freeObject(object);
		object = next;
	}
}

void memoryManager::setVM(VM* v) {
	this->vm = v;
}

void memoryManager::addToObjects(obj* o) {
	o->setNext(allObjects);
	allObjects = o;
}

void memoryManager::freeObject(obj* el) {
	switch (el->getType()) {
	case OBJ_STR: {
		auto* str = (objString*)el;
		bytesAllocated -= sizeof(objString);
		freeArray(str->getChars(), str->getLen() + 1);
		delete str;
		break;
	}
	case OBJ_FUN: {
		auto* fun = (objFunction*)el;
		delete fun->funChunk;
		bytesAllocated -= sizeof(objFunction);
		delete el;
		break;
	}
	case OBJ_NAT_FUN: {
		bytesAllocated -= sizeof(objNativeFunction);
		delete el;
		break;
	}
	case OBJ_CLASS: {
		bytesAllocated -= sizeof(objClass);
		auto* object = (objClass*)el;
		object->table.clear();
		delete el;
		break;
	}
	case OBJ_THIS: {
		bytesAllocated -= sizeof(objThis);
		delete el;
		break;
	}
	case OBJ_INSTANCE: {
		bytesAllocated -= sizeof(objInstance);
		auto* object = (objInstance*)el;
		object->table.clear();
		delete el;
		break;
	}
	case OBJ_NAT_INSTANCE: {
		bytesAllocated -= sizeof(objNativeInstance);
		auto* object = (objNativeInstance*)el;
		object->table.clear();
		delete el;
		break;
	}
	case OBJ_LIST: {
		bytesAllocated -= sizeof(objList);
		auto* list = (objList*)el;
		list->data.clear();
		delete el;
		break;
	}
	case OBJ_FILE: {
		bytesAllocated -= sizeof(objFile);
		auto* file = (objFile*)el;
		if(file->isOpen())
			file->file.close();
		delete el;
		break;
	}
	case OBJ_MAP: {
		bytesAllocated -= sizeof(objMap);
		auto* map = (objMap*)el;
		map->data.clear();
		delete el;
		break;
	}
	}
}

void memoryManager::markObject(obj* obj) {
	if (obj == nullptr) return;
	if (obj->isMark()) return;

	obj->mark();
	grayStack.push_back(obj);
#ifdef DEBUG_LOG_GC
	std::cout << "marked : " << obj << std::endl;
#endif
}

void memoryManager::markValue(value& val) {
	if (val.getType() == VAL_OBJ) {
		markObject(val.as.object);
	}
}

/*
* this overload is only necessary because of the dictionary object
*/
void memoryManager::markValue(const value& val) {
	if (val.type == VAL_OBJ) {
		markObject(val.as.object);
	}
}

void memoryManager::markRoots() {
	for (int i = 0; i < vm->stackTop - vm->stack; ++i) {
		markValue(vm->stack[i]);
	}
	for (auto& el : vm->globals) {
		markObject(el.first);
		markValue(el.second);
	}

	for (auto& el : vm->scriptChunk->constants) {
		markValue(el);
	}

	for (auto& el : vm->stringFunctions) {
		markObject(el.first);
		markValue(el.second);
	}

	for (auto& el : vm->arrayFunctions) {
		markObject(el.first);
		markValue(el.second);
	}

	for (auto& el : vm->fileFunctions) {
		markObject(el.first);
		markValue(el.second);
	}

	markObject(initString);
}

void memoryManager::blackenObject(obj* obj) {
	switch (obj->getType()) {
	case OBJ_CLASS: {
		auto* cl = (objClass*)obj;
		markObject(cl->getName());
		for (auto& el : cl->table) {
			markObject(el.first);
			markValue(el.second);
		}
		break;
	}
	case OBJ_FUN: {
		auto* fn = (objFunction*)obj;
		for (auto& el : fn->funChunk->constants) {
			markValue(el);
		}
		break;
	}
	case OBJ_INSTANCE: {
		auto* cl = (objInstance*)obj;
		for (auto& el : cl->table) {
			markObject(el.first);
			markValue(el.second);
		}
		break;
	}
	case OBJ_NAT_INSTANCE: {
		auto* ins = (objNativeInstance*)obj;
		for (auto& el : ins->table) {
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
	case OBJ_THIS:
	case OBJ_NAT_FUN:
	case OBJ_STR:
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

safetyMark:

	for (auto& el : internedStrings) {
		if (!el.second->isMark()) {
			internedStrings.erase(el.first);
			goto safetyMark;
		}
	}
}

void memoryManager::sweep() {
	obj* previous = nullptr;
	obj* object = allObjects;
	while (object != nullptr) {
		if (object->isMark()) {
			object->unmark();
			previous = object;
			object = object->getNext();
		}
		else {
			obj* unreached = object;
			object = object->getNext();
			if (previous == nullptr) {
				allObjects = object;
			}
			else {
				previous->setNext(object);
			}

#ifdef DEBUG_LOG_GC
			std::cout << "freed:" << unreached << std::endl;
#endif

			freeObject(unreached);
		}
	}
}

void memoryManager::collectGarbage() {
#ifdef DEBUG_LOG_GC
	std::cout << " -- GC started:" << std::endl;
	size_t before = bytesAllocated;
#endif
	
	//stopping if vm is not created yet
	if (!(vm != nullptr && vm->gcReady)) {
		return;
	}


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
