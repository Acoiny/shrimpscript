#include "value.hpp"

#include "obj.hpp"

#include <cstdint>

#ifdef NAN_BOXING

static std::ostream& printObject(std::ostream& os, obj* object) {
	switch (object->getType()) {
	case OBJ_STR:
		os << ((objString*)object)->getChars();
		break;
	case OBJ_FUN: {
		auto fn = (objFunction*)object;
		os << "<function " << fn->name->getChars() << ">";
		break;
	}
	case OBJ_NAT_FUN:
		os << "<native fn>";
		break;
	case OBJ_CLASS: {
		auto* cl = (objClass*)object;
		os << "<class " << cl->name->getChars() << ">";
		break;
	}
	case OBJ_NAT_INSTANCE:
	case OBJ_INSTANCE: {
		auto* cl = (objInstance*)object;
		os << "<" << cl->klass->getName()->getChars() << " instance>";
		break;
	}
	case OBJ_LIST: {
		auto* list = (objList*)object;
		os << "[";
		size_t len = list->getSize();
		if (len > 0) {
			os << list->data.at(0);
			for (size_t i = 1; i < len; i++)
			{
				os << ", " << list->data.at(i);
			}
		}
		os << "]";
		break;
	}
	case OBJ_MAP: {
		auto* map = (objMap*)object;
		os << "{";
		if (map->data.size() == 0) {
			os << "}";
			break;
		}

		bool setComma = false;

		for (auto& el : map->data) {
			if (setComma)
				os << ", ";
			else
				setComma = true;

			os << el.first << " : " << el.second;

		}
		os << "}";
		break;
	}
	case OBJ_THIS: {
		os << "this";
		break;
	}
	case OBJ_FILE: {
		os << "file";
		break;
	}
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const value val) {
	if (IS_NIL(val))
		os << "nil";
	else if (IS_BOOL(val))
		os << (AS_BOOL(val) ? "true" : "false");
	else if (IS_NUM(val))
		os << AS_NUM(val);
	else if (IS_RET(val))
		os << "<return address>";
	else if (IS_OBJ(val))
		printObject(os, AS_OBJ(val));
	else
		os << "ERROR";
	return os;
}

bool operator==(const value a, const value b) {
	return a.as_double == b.as_double;
}

#else
std::ostream& value::printObject(std::ostream& os) const {
	switch (as.object->getType()) {
	case OBJ_STR:
		os << ((objString*)as.object)->chars;
		break;
	case OBJ_FUN: {
		auto fn = (objFunction*)as.object;
		os << "<function " << fn->name->getChars() << ">";
		break;
	}
	case OBJ_NAT_FUN:
		os << "<native fn>";
		break;
	case OBJ_CLASS: {
		auto* cl = (objClass*)as.object;
		os << "<class " << cl->name->getChars() << ">";
		break;
	}
	case OBJ_INSTANCE: {
		auto* cl = (objInstance*)as.object;
		os << "<" << cl->klass->getName()->getChars() << " instance>";
		break;
	}
	case OBJ_LIST: {
		auto* list = (objList*)as.object;
		os << "[";
		size_t len = list->getSize();
		if (len > 0) {
			os << list->data.at(0);
			for (size_t i = 1; i < len; i++)
			{
				os << ", " << list->data.at(i);
			}
		}
		os << "]";
		break;
	}
	case OBJ_MAP: {
		auto* map = (objMap*)as.object;
		os << "{";
		if (map->data.size() == 0) {
			os << "}";
			break;
		}

		bool setComma = false;

		for (auto& el : map->data) {
			if (setComma)
				os << ", ";
			else
				setComma = true;

			os << el.first << " : " << el.second;
			
		}
		os << "}";
		break;
	}
	case OBJ_THIS: {
		os << "this";
		break;
	}
	case OBJ_FILE: {
		os << "file";
		break;
	}
	}
	return os;
}
/*
const valType value::getType() const {
	return type;
}
*/
value::value(bool boolean) {
	type = VAL_BOOL;
	as.boolean = boolean;
}

value::value(double num) {
	type = VAL_NUM;
	as.number = num;
}

value::value(obj* obj) {
	type = VAL_OBJ;
	as.object = obj;
}

value::value(uintptr_t address) {
	type = VAL_ADDRESS;
	as.address = address;
}

value::value() {
	type = VAL_NIL;
}


std::ostream& operator<<(std::ostream& os, const value& val) {
	switch (val.type) {
	case VAL_NIL:
		os << "nil";
		break;
	case VAL_BOOL:
		os << (val.as.boolean ? "true" : "false");
		break;
	case VAL_NUM:
		os << val.as.number;
		break;
	case VAL_OBJ:
		val.printObject(os);
		break;
	case VAL_ADDRESS:
		os << "<return address>";
		break;
	}
	return os;
}

bool value::operator==(const value& rhs) const {
	return this->type == rhs.type && this->as.address == rhs.as.address;
}
#endif