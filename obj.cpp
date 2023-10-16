#include "obj.hpp"

#include "VM.hpp"
#include "memoryManager.hpp"

extern memoryManager globalMemory;

///objString functions
objString::~objString() = default;


objString::objString() : len(0), chars(nullptr) {
	type = OBJ_STR;
}

void objString::init(const char* ch, unsigned int strlen) {
	len = strlen;

	char* str = globalMemory.allocateArray<char>(len + 1);
	memcpy(str, ch, len);
	str[len] = '\0';

	chars = str;
}

objString* objString::copyString(const char* chars, const unsigned int len) {
	//TODO: eliminate unnecessary new & delete
	char* tmp = new char[len + 1];
	memcpy(tmp, chars, len);
	tmp[len] = '\0';

	if (globalMemory.internedStrings.count(tmp)) {
		objString* ret = globalMemory.internedStrings.at(tmp);
		delete[] tmp;
		return ret;
	}

	delete[] tmp;

	auto* str = (objString*)globalMemory.allocateObject<objString>();
	str->mark();
	str->init(chars, len);
	globalMemory.internedStrings.insert_or_assign(str->chars, str);
	str->unmark();
	return str;
}

objString* objString::copyStringEscape(const char* chars, const unsigned int len) {
	char* tmp = new char[len + 1];
	
	size_t newLen = 0;
	size_t pos = 0;

	for (size_t i = 0; pos < len; i++)
	{
		if (chars[pos] == '\\') {
			newLen++;
			switch (chars[pos + 1]) {
			case 'n':
				pos += 2;
				tmp[i] = '\n';
				break;
			case 'r':
				pos += 2;
				tmp[i] = '\r';
				break;
			case 't':
				pos += 2;
				tmp[i] = '\t';
				break;
			case 'a':
				pos += 2;
				tmp[i] = '\a';
				break;
			default:
				newLen++;
				tmp[i] = chars[pos];
				pos++;
			}
		}
		else {
			newLen++;
			tmp[i] = chars[pos];
			pos++;
		}
	}
	char* escapedStr = new char[newLen + 1];
	memcpy(escapedStr, tmp, newLen);
	escapedStr[newLen] = '\0';
	delete[] tmp;
	tmp = escapedStr;

	if (globalMemory.internedStrings.count(tmp)) {
		objString* ret = globalMemory.internedStrings.at(tmp);
		delete[] tmp;
		return ret;
	}


	auto* str = (objString*)globalMemory.allocateObject<objString>();
	str->mark();
	str->init(tmp, len);
	globalMemory.internedStrings.insert_or_assign(str->chars, str);
	str->unmark();

	delete[] tmp;
	return str;
}

objString* objString::takeString(const char* chars, const unsigned int len) {

	auto interned = globalMemory.internedStrings.find(chars);
	
	if (interned != globalMemory.internedStrings.end()) {
		globalMemory.freeArray(chars, len + 1);
		return interned->second;
	}

	auto* str = (objString*)globalMemory.allocateObject<objString>();
	str->mark();
	str->init(chars, len);
	globalMemory.internedStrings.insert_or_assign(str->chars, str);
	str->unmark();
	return str;
}

unsigned int objString::getLen() const {
	return len;
}

char* objString::getChars() const {
	return chars;
}

obj* obj::getNext() {
	return next;
}

void obj::setNext(obj* ne) {
	next = ne;
}

void obj::mark() {
	isMarked = true;
}

void obj::unmark() {
	isMarked = false;
}

bool obj::isMark() {
	return isMarked;
}

///object function
objFunction::objFunction() : funChunk(nullptr), arity(0), klass(nullptr) {
	type = OBJ_FUN;
}

chunk* objFunction::getChunkPtr() {
	return funChunk;
}

objFunction* objFunction::createObjFunction(objString* name, chunk* ch, int arity) {
	auto* fun = (objFunction*)globalMemory.allocateObject<objFunction>();
	fun->name = name;
	fun->funChunk = ch;
	fun->arity = arity;
	return fun;
}

int objFunction::getArity() const {
	return arity;
}

void objFunction::setClass(objClass* cl) {
	klass = cl;
}

bool objFunction::isMethod() const {
	return klass != nullptr;
}

objClass* objFunction::getClass() const {
	return klass;
}

//objNativeFunction functions
objNativeFunction::objNativeFunction() : fun(nullptr) {
	type = OBJ_NAT_FUN;
}

objNativeFunction* objNativeFunction::createNativeFunction(nativeFn fn) {
	auto* natFun = (objNativeFunction*)globalMemory.allocateObject<objNativeFunction>();

	natFun->fun = fn;

	return natFun;
}

//objClass functions
objClass::objClass() : name(nullptr), superClass(nullptr) {
	type = OBJ_CLASS;
}

objString* objClass::getName() const {
	return name;
}

void objClass::tableSet(objString* name, value& val) {
	table.insert_or_assign(name, val);
}

value objClass::tableGet(objString* k) {
	if (table.count(k) == 0) {
		if (superClass != nullptr) {
			return superClass->tableGet(k);
		}

		return value();
	}

	return table.at(k);
}

value objClass::superTableGet(objString* k) {
	return superClass->tableGet(k);
}

objClass* objClass::createObjClass(objString* name) {
	auto* cl = (objClass*)globalMemory.allocateObject<objClass>();
	cl->name = name;
	return cl;
}

void objClass::setSuperClass(objClass* cl) {
	superClass = cl;
}

bool objClass::hasInitFunction() {
	objString* name = globalMemory.initString;
	
	auto el = tableGet(name);

	if (el.getType() == VAL_OBJ && el.as.object->getType() == OBJ_FUN)
		return true;

	return false;
}

//objInstance functions
objInstance::objInstance() : klass(nullptr) {
	type = OBJ_INSTANCE;
}

objInstance* objInstance::createInstance(objClass* kl) {
	auto* ins = (objInstance*)globalMemory.allocateObject<objInstance>();

	ins->klass = kl;
	//ins->table = kl->getTable();

	return ins;
}

void objInstance::tableSet(objString* n, value val) {
	table.insert_or_assign(n, val);
}

value objInstance::tableGet(objString* k) {
	if (table.count(k) == 0) {
		return klass->tableGet(k);
	}
	return table.at(k);
}

void objInstance::tableDelete(objString* k) {
	table.erase(k);
}


//this-object functions
objThis::objThis() : this_instance(nullptr), retAddress(0) {
	type = OBJ_THIS;
}

objThis* objThis::createObjThis(objInstance* ins) {
	auto* th = (objThis*)globalMemory.allocateObject<objThis>();

	th->this_instance = ins;

	return th;
}

//objNativeInstance functions
objNativeInstance::objNativeInstance() {
	type = OBJ_NAT_INSTANCE;
}

void objNativeInstance::tableSet(objString* name, value val) {
	table.insert_or_assign(name, val);
}

value objNativeInstance::tableGet(objString* k) {
	if (table.find(k) == table.end()) {
		return value();
	}
	return table.at(k);
}

objNativeInstance* objNativeInstance::createNativeInstance() {
	auto* ins = (objNativeInstance*)globalMemory.allocateObject<objNativeInstance>();
	return ins;
}

///objList methods
objList::objList() {
	type = OBJ_LIST;
}

size_t objList::getSize() {
	return data.size();
}

value& objList::getValueAt(size_t index) {
	return data.at(index);
}

void objList::appendValue(value& val) {
	data.push_back(val);
}

objList* objList::createList() {
	return (objList*)globalMemory.allocateObject<objList>();
}


//objMap functions
objMap::objMap() {
	type = OBJ_MAP;
}

size_t objMap::getSize() {
	return data.size();
}

value objMap::getValueAt(const value& key) {
	if (data.count(key) == 0)
		return value();

	return data.at(key);
}

void objMap::insertElement(const value& key, const value& val) {
	data.insert_or_assign(key, val);
}

objMap* objMap::createMap() {
	return (objMap*)globalMemory.allocateObject<objMap>();
}

//objFile functions

objFile::objFile() : file(), fileSize(0) {
	type = OBJ_FILE;
}

void objFile::close() {
	file.close();
}

bool objFile::isOpen() {
	return file.is_open();
}

objFile* objFile::createReadFile(objString* path) {
	auto* fl = (objFile*)globalMemory.allocateObject<objFile>();
	
	fl->file = std::fstream(path->getChars(), std::ios::in | std::ios::ate);
	std::streamsize len = fl->file.tellg();
	fl->file.seekg(0, std::ios::beg);

	fl->fileSize = len;

	return fl;
}

objFile* objFile::createWriteFile(objString* path) {
	auto* fl = (objFile*)globalMemory.allocateObject<objFile>();

	fl->file = std::fstream(path->getChars(), std::ios::out | std::ios::ate);
	std::streamsize len = fl->file.tellg();
	fl->file.seekg(0, std::ios::beg);

	fl->fileSize = len;

	return fl;
}