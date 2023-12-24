#include "../../header/virtualMachine/obj.hpp"

#include "../../header/virtualMachine/VM.hpp"
#include "../../header/virtualMachine/memoryManager.hpp"

extern memoryManager globalMemory;

///objString functions

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
	char* tmp = globalMemory.allocateArray<char>(len + 1);
	memcpy(tmp, chars, len);
	tmp[len] = '\0';

	if (globalMemory.internedStrings.count(tmp)) {
		objString* ret = globalMemory.internedStrings.at(tmp);
		globalMemory.freeArray(tmp, len + 1);
		return ret;
	}

	globalMemory.freeArray(tmp, len + 1);

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
	char* escapedStr = globalMemory.allocateArray<char>(newLen + 1);
	memcpy(escapedStr, tmp, newLen);
	escapedStr[newLen] = '\0';
	delete[] tmp;
	tmp = escapedStr;

	if (globalMemory.internedStrings.count(tmp)) {
		objString* ret = globalMemory.internedStrings.at(tmp);
		globalMemory.freeArray(tmp, newLen + 1);
		return ret;
	}


	auto* str = (objString*)globalMemory.allocateObject<objString>();
	str->mark();
	str->init(tmp, newLen);
	globalMemory.internedStrings.insert_or_assign(str->chars, str);
	str->unmark();

	globalMemory.freeArray(tmp, newLen + 1);
	return str;
}

objString* objString::takeString(char* chars, const unsigned int len) {

	auto interned = globalMemory.internedStrings.find(chars);

	if (interned != globalMemory.internedStrings.end()) {
		globalMemory.freeArray(chars, len + 1);
		return interned->second;
	}

	auto* str = (objString*)globalMemory.allocateObject<objString>();
	str->mark();
	str->chars = chars;
	str->len = len;
	globalMemory.internedStrings.insert_or_assign(str->chars, str);
	str->unmark();
	return str;
}

unsigned int objString::getLen() const {
	return len;
}

const char* objString::getChars() const {
	return chars;
}

obj* obj::getNext() const {
	return next;
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

objFunction* objFunction::createObjFunction(objString* name, chunk* ch, int arity) {
	auto* fun = (objFunction*)globalMemory.allocateObject<objFunction>();
	fun->name = name;
	fun->funChunk = ch;
	fun->arity = arity;
	return fun;
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

void objClass::tableSet(objString* name, value val) {
	table.insert_or_assign(name, val);
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

	if (IS_OBJ(el) && (AS_OBJ(el)->getType() == OBJ_FUN || AS_OBJ(el)->getType() == OBJ_NAT_FUN))
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



///objList methods
objList::objList() {
	type = OBJ_LIST;
}



void objList::appendValue(value val) {
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

value objMap::getValueAt(const value key) {
	if (data.count(key) == 0)
		return NIL_VAL;

	return data.at(key);
}

void objMap::insertElement(const value key, const value val) {
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