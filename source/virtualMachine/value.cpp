#include "../../header/virtualMachine/value.hpp"

#include "../../header/virtualMachine/obj.hpp"

#include <string>
#include <sstream>

const std::string stringify(value val) {
	if (IS_NUM(val)) {
		if (((long long)AS_NUM(val)) == AS_NUM(val)) {
			return std::to_string((long long)AS_NUM(val));
		}
		return std::to_string(AS_NUM(val));
	}
	if (IS_NIL(val)) {
		return "nil";
	}
	if (IS_BOOL(val)) {
		return AS_BOOL(val) ? "true" : "false";
	}
	if (IS_RET(val)) {
		return std::to_string(AS_RET(val));
	}
	if (IS_OBJ(val)) {
		obj* object = AS_OBJ(val);
		switch (object->getType()) {
		case OBJ_STR:
			return ((objString*)object)->chars;
		case OBJ_FUN:
			return ("<function " + std::string(((objFunction*)object)->name->chars) + ">");
		case OBJ_NAT_FUN:
			return ("<nativeFunction>");
		case OBJ_CLASS:
			return ("<class " + std::string(((objClass*)object)->name->chars) + ">");
		case OBJ_INSTANCE:
			return ("<" + std::string(((objInstance*)object)->klass->name->chars) + " object>");
		case OBJ_LIST: {
			auto* list = (objList*)object;
			std::stringstream buffer;
			buffer << "[";
			size_t len = list->getSize();
			if (len > 0) {
				buffer << list->data.at(0);
				for (size_t i = 1; i < len; i++)
				{
					buffer << ", " << list->data.at(i);
				}
			}
			buffer << "]";

			return buffer.str();
		}
		case OBJ_MAP: {
			auto* map = (objMap*)object;
			std::stringstream buffer;
			buffer << "{";
			if (map->data.size() == 0) {
				buffer << "}";
				break;
			}

			bool setComma = false;

			for (auto& el : map->data) {
				if (setComma)
					buffer << ", ";
				else
					setComma = true;

				buffer << el.first << " : " << el.second;

			}
			buffer << "}";

			return buffer.str();
		}
		case OBJ_FILE:
			return "<file>";
		case OBJ_THIS:
			return ("<" + std::string(((objThis*)object)->this_instance->klass->name->chars) + " object>");

		}
}
	return "<unknown object>";
}

#ifdef NAN_BOXING

std::ostream& operator<<(std::ostream& os, const value val) {
	return os << stringify(val);
}

bool operator==(const value a, const value b) {
	return a.as_uint64 == b.as_uint64;
}

#else
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
	return os << stringify(val);
}

bool value::operator==(const value& rhs) const {
	return this->type == rhs.type && this->as.address == rhs.as.address;
}
#endif