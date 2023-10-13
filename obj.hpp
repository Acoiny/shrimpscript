#ifndef SHRIMPP_OBJ_HPP
#define SHRIMPP_OBJ_HPP

#include <unordered_map>
#include <fstream>

#include "value.hpp"

class VM;
class chunk;

typedef value(*nativeFn)(int arity, value* args, bool &success);

enum objType {
    OBJ_STR,
    OBJ_FUN,
    OBJ_NAT_FUN,
    OBJ_NAT_INSTANCE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_LIST,
    OBJ_MAP,
    OBJ_THIS,
    OBJ_FILE
};

class obj {
protected:
    objType type;
    bool isMarked = false;
    obj *next = nullptr;
public:

    obj* getNext();
    void setNext(obj* ne);
    virtual ~obj() = default;
    inline objType getType() const {
        return type;
    }

    void mark();
    void unmark();

    bool isMark();
};

class objString : public obj {
    friend class memoryManager;
    friend class value;

    char* chars;
    unsigned int len;


    void init(const char* ch, unsigned int strlen);

public:
    objString();

    ~objString() override;


    [[nodiscard]] unsigned int getLen() const;

    char* getChars();

    static objString* copyString(const char* chars, const unsigned int len);
    static objString* copyStringEscape(const char* chars, const unsigned int len);
    static objString* takeString(const char* chars, const unsigned int len);
};

//forward declaring objClass

class objClass;

class objFunction : public obj {
    friend class memoryManager;
    friend class value;

    objString* name;

    chunk* funChunk;

    objClass* klass;

    unsigned char arity;

public:

    objFunction();

    ~objFunction() override = default;

    chunk* getChunkPtr();

    int getArity() const;

    void setClass(objClass* cl);

    bool isMethod() const;

    objClass* getClass() const;

    static objFunction* createObjFunction(objString* name, chunk* ch, int arity);
};

class objNativeFunction : public obj {
    friend class memoryManager;
    friend class value;

public:

    nativeFn fun;

    objNativeFunction();
    ~objNativeFunction() override = default;


    static objNativeFunction* createNativeFunction(nativeFn fn);
};

class objClass : public obj {
    friend class memoryManager;
    friend class value;

    friend class objThis;

    objString* name;

    objClass* superClass;

    std::unordered_map<objString *, value> table;

public:
    objClass();

    objString* getName() const;

    void tableSet(objString* n, value &val);

    value tableGet(objString* k);

    value superTableGet(objString* k);

    bool hasInitFunction();

    static objClass* createObjClass(objString* name);

    void setSuperClass(objClass* cl);
};

class objInstance : public obj {
    friend class memoryManager;
    friend class value;

    friend class objThis;

    objClass* klass;

    std::unordered_map<objString *, value> table;

public:
    objInstance();

    void tableSet(objString* n, value val);
    value tableGet(objString* k);
    void tableDelete(objString* k);

    static objInstance* createInstance(objClass* kl);
};

//return this-object
class objThis : public obj {
    friend class memoryManager;
    friend class value;

    objInstance* this_instance;
    uintptr_t retAddress;

public:
    objThis();

    static objThis* createObjThis(objInstance* ins);

    void setAddress(uintptr_t ad);

    uintptr_t getAddress();

    objInstance* getThis();
};



//native object instance
class objNativeInstance : public obj {
    friend class memoryManager;
    friend class value;

    std::unordered_map<objString*, value> table;

public:
    objNativeInstance();

    void tableSet(objString* n, value val);

    value tableGet(objString* k);

    static objNativeInstance* createNativeInstance();
};

class objList : public obj {
    friend class memoryManager;
    friend class value;

public:
    std::vector<value> data;
    
    objList();

    size_t getSize();

    value& getValueAt(size_t index);

    void appendValue(value& val);

    static objList* createList();
};

struct myMapHash {
    size_t operator()(const value& rhs) const {
        return rhs.as.address;
    }
};


class objMap : public obj{
    friend class memoryManager;
    friend class value;

public:
    std::unordered_map<value, value, myMapHash> data;

    objMap();

    size_t getSize();

    value getValueAt(const value& key);

    void insertElement(const value& key, const value& val);

    static objMap* createMap();
};

class objFile : public obj {
    friend class memoryManager;
    friend class value;


public:
    size_t fileSize;
    std::fstream file;

    objFile();

    void close();

    bool isOpen();

    static objFile* createReadFile(objString* path);
    static objFile* createWriteFile(objString* path);
};

#endif //SHRIMPP_OBJ_HPP
