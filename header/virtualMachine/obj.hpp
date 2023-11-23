#ifndef SHRIMPP_OBJ_HPP
#define SHRIMPP_OBJ_HPP

#include <unordered_map>
#include <fstream>
#include <vector>

#include "value.hpp"

class VM;
class chunk;

typedef value(*nativeFn)(int arity, value* args, bool& success);

enum objType : char {
	OBJ_STR,
	OBJ_FUN,
	OBJ_NAT_FUN,
	OBJ_CLASS,
	OBJ_INSTANCE,
	OBJ_LIST,
	OBJ_MAP,
	OBJ_THIS,
	OBJ_FILE
};

class obj {
	friend class memoryManager;
protected:
	objType type;
	bool isMarked = false;
	obj* next = nullptr;

	~obj() = default;
public:

	obj* getNext() const;
	inline objType getType() const {
		return type;
	}

	void mark();
	void unmark();

	bool isMark();
};

class objString : public obj {
public:
	friend class memoryManager;

	char* chars;
	unsigned int len;


	void init(const char* ch, unsigned int strlen);

	objString();

	~objString() = default;

	[[nodiscard]] unsigned int getLen() const;

	const char* getChars() const;

	static objString* copyString(const char* chars, const unsigned int len);
	static objString* copyStringEscape(const char* chars, const unsigned int len);
	static objString* takeString(char* chars, const unsigned int len);
};

//forward declaring objClass

class objClass;

class objFunction : public obj {
public:
	friend class memoryManager;

	objString* name;

	chunk* funChunk;

	objClass* klass;

	unsigned char arity;

	objFunction();

	~objFunction() = default;

	inline chunk* getChunkPtr() const { return funChunk; }

	inline int getArity() const { return arity; };

	void setClass(objClass* cl);

	bool isMethod() const;

	objClass* getClass() const;

	static objFunction* createObjFunction(objString* name, chunk* ch, int arity);
};

class objNativeFunction : public obj {
	friend class memoryManager;

public:

	nativeFn fun;

	objNativeFunction();

	~objNativeFunction() = default;

	static objNativeFunction* createNativeFunction(nativeFn fn);
};

class objClass : public obj {
public:
	friend class memoryManager;

	friend class objThis;

	objString* name;

	objClass* superClass;

	std::unordered_map<objString*, value> table;

	objClass();

	~objClass() = default;

	objString* getName() const;

	void tableSet(objString* n, value val);

	inline value tableGet(objString* k) {
		if (table.count(k) == 0) {
			if (superClass != nullptr) {
				return superClass->tableGet(k);
			}

			return NIL_VAL;
		}

		return table.at(k);
	}

	value superTableGet(objString* k);

	bool hasInitFunction();

	static objClass* createObjClass(objString* name);

	void setSuperClass(objClass* cl);
};

class objInstance : public obj {
public:
	friend class memoryManager;

	friend class objThis;

	objClass* klass;

	std::unordered_map<objString*, value> table;

	objInstance();

	~objInstance() = default;

	inline void tableSet(objString* n, value val) {
		table.insert_or_assign(n, val);
	}

	inline value tableGet(objString* k) {
		if (table.count(k) == 0) {
			return klass->tableGet(k);
		}
		return table.at(k);
	}

	inline void tableDelete(objString* k) {
		table.erase(k);
	}

	static objInstance* createInstance(objClass* kl);
};

//return this-object
class objThis : public obj {
	friend class memoryManager;

public:
	objInstance* this_instance;
	uintptr_t retAddress;

	objThis();

	~objThis() = default;

	static objThis* createObjThis(objInstance* ins);
};


/*
//native object instance
class objNativeInstance : public obj {
	friend class memoryManager;

	std::unordered_map<objString*, value> table;

public:
	objNativeInstance();

	void tableSet(objString* n, value val);

	value tableGet(objString* k);

	static objNativeInstance* createNativeInstance();
};
*/
class objList : public obj {
	friend class memoryManager;

public:
	std::vector<value> data;

	objList();

	~objList() = default;

	inline size_t getSize() { return data.size(); }

	inline value getValueAt(size_t index) { return data.at(index); }
	inline void setValueAt(size_t index, value val) { data.at(index) = val; }

	void appendValue(value val);

	static objList* createList();
};

struct myMapHash {
	size_t operator()(const value& rhs) const {
		return AS_RET(rhs);
	}
};


class objMap : public obj {
	friend class memoryManager;

public:
	std::unordered_map<value, value, myMapHash> data;

	objMap();

	~objMap() = default;

	size_t getSize();

	value getValueAt(const value key);

	void insertElement(const value key, const value val);

	static objMap* createMap();
};

class objFile : public obj {
	friend class memoryManager;

public:
	size_t fileSize;
	std::fstream file;

	objFile();

	~objFile() = default;

	void close();

	bool isOpen();

	static objFile* createReadFile(objString* path);
	static objFile* createWriteFile(objString* path);
};

#endif //SHRIMPP_OBJ_HPP
