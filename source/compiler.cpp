#include "../header/compiler.hpp"

#include <iostream>
#include <cstring>

#include "../header/defines.hpp"

#include "../header/virtualMachine/VM.hpp"

#ifdef DEBUG_PRINT_CODE
#include "../header/commandLineOutput/debug.hpp"
#endif

static bool identifiersEqual(token& a, token& b) {
	if (a.len != b.len) return false;

	return memcmp(a.start, b.start, a.len) == 0;
}

precFuncTableEntry* compiler::getRule(token tk) {
	return &precedenceTable[tk.type];
}

compiler::compiler(VM& vm, std::vector<std::string>& constVec)
	: scanr(), currentFunction(nullptr), vm(vm), globalConsts(constVec), upvalues(new std::vector<upvalue>) {}

compiler::~compiler() {
	delete upvalues;
}

void compiler::errorAt(token tk, const char* msg) {
	if (panicMode) return;

	if (tk.type == TOKEN_EOF) {
		std::cerr << "at end -> " << msg << std::endl;
	}
	else {
		std::cerr << "[line " << tk.line << "] -> " << msg << std::endl;
	}
	hadError = true;
	panicMode = true;
}

void compiler::error(const char* msg) {
	errorAt(prevToken, msg);
}

void compiler::synchronize() {
	panicMode = false;
	while (currentToken.type != TOKEN_EOF) {
		if (prevToken.type == TOKEN_SEMICOLON) return;
		switch (currentToken.type) {
		case TOKEN_BRACE_CLOSE:
			// testing open braces
		case TOKEN_BRACE_OPEN:
			// case TOKEN_PAREN_CLOSE:
		case TOKEN_CLASS:
		case TOKEN_WHILE:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_LET:
		case TOKEN_CONST:
		case TOKEN_FUN:
			// case TOKEN_SEMICOLON:
			// advance();
			return;
		default:
			advance();
		}
	}
}

void compiler::advance() {

	prevToken = currentToken;
	for (;;) {
		currentToken = scanr.scanToken();

		if (currentToken.type != TOKEN_ERROR) break;

		errorAt(currentToken, currentToken.start);
	}
}

bool compiler::match(tokenType type) {
	if (currentToken.type == type) {
		advance();
		return true;
	}
	return false;
}

bool compiler::check(tokenType type) const {
	return currentToken.type == type;
}

void compiler::consume(const char* msg, tokenType type) {
	if (currentToken.type == type) {
		advance();
		return;
	}

	if (scanr.checkASIfoundIllegal()) {
		token tmp = prevToken;
		prevToken = { TOKEN_SEMICOLON, NULL, tmp.len, tmp.line };
		if (prevToken.type == type) return;
	}

	error(msg);
}

void compiler::emitByte(opCodes op) {
	currentFunction->funChunk->addByte(op, prevToken.line);
}

void compiler::emitBytes(char a, char b) {
	currentFunction->funChunk->addByte(a, prevToken.line);
	currentFunction->funChunk->addByte(b, prevToken.line);
}

size_t compiler::emitJump(opCodes jmpCode) {
	emitByte(jmpCode);
	emitBytes(0xff, 0xff);
	return currentFunction->funChunk->getSize() - 2;
}

void compiler::emitLoop(size_t loopStart) {
	emitByte(OP_LOOP);

	int dest = currentFunction->funChunk->getSize() - loopStart + 2;

	if (dest > UINT16_MAX)
		error("loop body too big");

	emitBytes((short(dest >> 8)) & 0xff, (dest) & 0xff);
}

//expression parsing functions

void compiler::number(bool canAssign, compiler& cmp) {
	double num = std::strtod(cmp.prevToken.start, nullptr);
	cmp.currentFunction->funChunk->writeConstant(NUM_VAL(num), cmp.prevToken.line);
}

void compiler::unary(bool canAssign, compiler& cmp) {
	token opType = cmp.prevToken;
	precFuncTableEntry* rule = cmp.getRule(opType);
	cmp.parsePrec(precedence(rule->prec + 1));

	switch (opType.type) {
	case TOKEN_MINUS:
		cmp.emitByte(OP_NEGATE);
		break;
	case TOKEN_BANG:
		cmp.emitByte(OP_NOT);
		break;
	case TOKEN_BIT_NOT:
		cmp.emitByte(OP_BIT_NOT);
		break;
	}
}

void compiler::binary(bool canAssign, compiler& cmp) {
	token opType = cmp.prevToken;
	precFuncTableEntry* rule = cmp.getRule(opType);
	cmp.parsePrec(precedence(rule->prec + 1));

	switch (opType.type) {
	case TOKEN_PLUS:
		cmp.emitByte(OP_ADD);
		break;
	case TOKEN_MINUS:
		cmp.emitByte(OP_SUB);
		break;
	case TOKEN_TIMES:
		cmp.emitByte(OP_MUL);
		break;
	case TOKEN_DIVIDE:
		cmp.emitByte(OP_DIV);
		break;
	case TOKEN_MODULO:
		cmp.emitByte(OP_MODULO);
		break;
	case TOKEN_EQUALS_EQUALS:
		cmp.emitByte(OP_EQUALS);
		break;
	case TOKEN_BANG_EQUALS:
		cmp.emitByte(OP_NOT_EQUALS);
		break;
	case TOKEN_LESS:
		cmp.emitByte(OP_LESSER);
		break;
	case TOKEN_LESS_EQUALS:
		cmp.emitByte(OP_LESSER_OR_EQUALS);
		break;
	case TOKEN_GREATER:
		cmp.emitByte(OP_GREATER);
		break;
	case TOKEN_GREATER_EQUALS:
		cmp.emitByte(OP_GREATER_OR_EQUALS);
		break;

		// bitwise operations
	case TOKEN_BIT_AND:
		cmp.emitByte(OP_BIT_AND);
		break;
	case TOKEN_BIT_OR:
		cmp.emitByte(OP_BIT_OR);
		break;
	case TOKEN_BIT_SHIFT_LEFT:
		cmp.emitByte(OP_BIT_SHIFT_LEFT);
		break;
	case TOKEN_BIT_SHIFT_RIGHT:
		cmp.emitByte(OP_BIT_SHIFT_RIGHT);
		break;
	case TOKEN_BIT_XOR:
		cmp.emitByte(OP_BIT_XOR);
		break;
	default:
		cmp.errorAt(opType, "unknown operator");
	}
}

void compiler::literal(bool canAssign, compiler& cmp) {
	switch (cmp.prevToken.type) {
	case TOKEN_TRUE:
		cmp.emitByte(OP_TRUE);
		break;
	case TOKEN_FALSE:
		cmp.emitByte(OP_FALSE);
		break;
	case TOKEN_NIL:
		cmp.emitByte(OP_NIL);
		break;
	}
}

void compiler::and_f(bool canAssign, compiler& cmp) {
	size_t falseJump = cmp.emitJump(OP_JUMP_IF_FALSE);
	cmp.emitByte(OP_POP);

	cmp.parsePrec(PREC_AND);

	cmp.patchJump(falseJump);
}

void compiler::or_f(bool canAssign, compiler& cmp) {
	size_t falseJump = cmp.emitJump(OP_JUMP_IF_FALSE);
	size_t trueJump = cmp.emitJump(OP_JUMP);

	cmp.patchJump(falseJump);
	cmp.emitByte(OP_POP);

	cmp.parsePrec(PREC_OR);
	cmp.patchJump(trueJump);
}

void compiler::grouping(bool canAssign, compiler& cmp) {
	cmp.expression();
	cmp.consume("expect ')' after grouping", TOKEN_PAREN_CLOSE);
}

void compiler::string(bool canAssign, compiler& cmp) {
	auto* str = objString::copyString(cmp.prevToken.start, cmp.prevToken.len);
	cmp.currentFunction->funChunk->writeConstant(OBJ_VAL(str), cmp.prevToken.line);
}

void compiler::stringEscape(bool canAssign, compiler& cmp) {
	auto* str = objString::copyStringEscape(cmp.prevToken.start, cmp.prevToken.len);
	cmp.currentFunction->funChunk->writeConstant(OBJ_VAL(str), cmp.prevToken.line);
}

int compiler::resolveLocal(bool& isConst) {
	return resolveLocal(isConst, prevToken);
}

int compiler::resolveLocal(bool& isConst, token comp) {
	for (int i = locals.size() - 1; i >= 0; --i) {
		local* loc = &locals.at(i);
		// return if no local in current function
		if (loc->funcDepth < functionDepth)
			return -1;

		if (identifiersEqual(loc->name, comp)) {
			if (loc->depth == -1) {
				error("can't initialize local with itself");
			}

			if (loc->isConst)
				isConst = true;

			return i - functionLocalOffset;
		}
	}

	return -1;
}

int compiler::addUpvalue(int index, bool isLocal) {
	// check if upvalue already exists
	for (size_t i = 0; i < upvalues->size(); i++)
	{
		upvalue el = upvalues->at(i);
		if (el.index == index && el.isLocal)
			return i;
	}

	// check if too many, for runtime
	if (upvalues->size() == 255) {
		// TODO: give upvalues 2 bytes
		error("Too many upvalues");
		return 0;
	}

	// add new upvalue
	upvalue upVal;
	upVal.index = index;
	upVal.isLocal = isLocal;
	upvalues->push_back(upVal);
	return currentFunction->upvalueCount++;
}

int compiler::resolveUpvalueInFunctionScope(bool& isConst, token& name, int functionScope) {
	for (int i = locals.size() - 1; i >= 0; i--) {
		local &loc = locals.at(i);
		if (loc.funcDepth > functionScope) continue;
		if (loc.funcDepth < functionScope) return -1;

		if (identifiersEqual(loc.name, name)) {
			if (loc.depth == -1) {
				error("can't initialize local with itself");
			}

			if (loc.isConst)
				isConst = true;

			return i;
		}
	}

	return -1;
}

int compiler::resolveUpvalue(bool& isConst, token &name, int funcDepth) {
	if (funcDepth == 0) return -1;

	// look for upvalue in directly enclosing function
	int local = resolveUpvalueInFunctionScope(isConst, name, funcDepth - 1);

	// if found, add it
	if (local != -1) {
		locals.at(local).isCaptured = true;
		return addUpvalue(local, true);
	}

	// look for upvalue relative to enclosing function
	int upval = resolveUpvalue(isConst, name, funcDepth - 1);

	// if found add it, but NOT as local
	if (upval != -1) {
		return addUpvalue(upval, false);
	}

	return -1;
}

void compiler::checkConsts(bool isConst, opCodes setOP, const std::string& checkNameGlobal) {
	if (isConst)
		error("can't reassign const variables");

	//checking globals here, so memcmp isn't called as often
	if (setOP == OP_SET_GLOBAL ||
		setOP == OP_INCREMENT_GLOBAL ||
		setOP == OP_DECREMENT_GLOBAL
		) {
		for (auto& el : globalConsts) {
			if (checkNameGlobal == el) {
				error("can't reassign const variables");
			}
		}
	}
}

void compiler::namedVariable(token name, bool canAssign) {
	opCodes setOP, getOP, incOP, decOP;

	bool isConst = false;

	int arg = resolveLocal(isConst);
	if (arg != -1) {
		getOP = OP_GET_LOCAL;
		setOP = OP_SET_LOCAL;

		incOP = OP_INCREMENT_LOCAL;
		decOP = OP_DECREMENT_LOCAL;
	}
	// here add resolveUpvalue call
	else if ((arg = resolveUpvalue(isConst, name, functionDepth)) != -1) {
		getOP = OP_GET_UPVALUE;
		setOP = OP_SET_UPVALUE;

		// maybe add inc and dec upvalue?
	}
	else {
		arg = identifierConstant(name);
		getOP = OP_GET_GLOBAL;
		setOP = OP_SET_GLOBAL;

		incOP = OP_INCREMENT_GLOBAL;
		decOP = OP_DECREMENT_GLOBAL;
	}
	
	if (canAssign && match(TOKEN_PLUS_PLUS)) {	// currently prohibiting inc and decrement for upvalues
		if (getOP == OP_GET_UPVALUE)
			error("increment only works on directly local variables");

		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
		emitByte(incOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else if (canAssign && match(TOKEN_MINUS_MINUS)) {
		if (getOP == OP_GET_UPVALUE)
			error("increment only works on directly local variables");

		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
		emitByte(decOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else if (canAssign && match(TOKEN_EQUALS)) {
		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);

		expression();
		emitByte(setOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else if (canAssign && match(TOKEN_PLUS_EQUALS)) {
		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);

		//loading variable onto stack
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);

		//parsing number
		expression();
		//adding number and variable
		emitByte(OP_ADD);
		//setting variable to addition result
		emitByte(setOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else if (canAssign && match(TOKEN_MINUS_EQUALS)) {
		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);

		//same principle as with adding
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);

		expression();
		emitByte(OP_SUB);
		emitByte(setOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else if (canAssign && match(TOKEN_TIMES_EQUALS)) {
		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);

		//same principle as with adding
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);

		expression();
		emitByte(OP_MUL);
		emitByte(setOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else if (canAssign && match(TOKEN_DIVIDE_EQUALS)) {
		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);

		//same principle as with adding
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);

		expression();
		emitByte(OP_DIV);
		emitByte(setOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else if (canAssign && match(TOKEN_MODULO_EQUALS)) {
		std::string constNameStr(name.start, name.len);
		checkConsts(isConst, setOP, constNameStr);

		//same principle as with adding
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);

		expression();
		emitByte(OP_MODULO);
		emitByte(setOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}
	else {
		emitByte(getOP);
		emitBytes((arg >> 8) & 0xff, arg & 0xff);
	}

}

void compiler::variable(bool canAssign, compiler& cmp) {
	cmp.namedVariable(cmp.prevToken, canAssign);
}

void compiler::call(bool canAssign, compiler& cmp) {
	int arity = 0;
	if (!cmp.check(TOKEN_PAREN_CLOSE)) {
		do {
			cmp.expression();
			arity++;
			if (arity > UINT16_MAX) {
				cmp.error("cannot call more than 255 arguments");
			}
		} while (cmp.match(TOKEN_COMMA));
	}
	cmp.consume("expect ')'", TOKEN_PAREN_CLOSE);
	cmp.emitByte(OP_CALL);
	cmp.emitByte((opCodes)arity);
}

void compiler::dot(bool canAssign, compiler& cmp) {
	cmp.consume("expect property name", TOKEN_IDENTIFIER);
	int index = cmp.identifierConstant(cmp.prevToken);

	if (canAssign && cmp.match(TOKEN_EQUALS)) {
		cmp.expression();
		cmp.emitByte(OP_SET_PROPERTY);
		cmp.emitBytes((index >> 8) & 0xff, (index) & 0xff);
	}
	else if (cmp.match(TOKEN_PAREN_OPEN)) {
		int argc = 0;
		if (!cmp.check(TOKEN_PAREN_CLOSE)) {
			do {
				cmp.expression();
				argc++;
				if (argc > UINT8_MAX) {
					cmp.error("functions can have a maximum of 255 arguments");
				}
			} while (cmp.match(TOKEN_COMMA));
		}
		cmp.consume("expect ')' after function call", TOKEN_PAREN_CLOSE);
		cmp.emitByte(OP_INVOKE);
		cmp.emitBytes((index >> 8) & 0xff, (index) & 0xff);
		cmp.emitByte((opCodes)argc);
	}
	else {
		cmp.emitByte(OP_GET_PROPERTY);
		cmp.emitBytes((index >> 8) & 0xff, (index) & 0xff);
	}
}

void compiler::list(bool canAssign, compiler& cmp) {
	size_t len = 0;
	cmp.emitByte(OP_LIST);

	if (!cmp.check(TOKEN_SQUARE_CLOSE)) {
		do {
			cmp.expression();
			len++;
		} while (cmp.match(TOKEN_COMMA));
	}

	if (len != 0) {
		cmp.emitByte(OP_APPEND);
		cmp.emitBytes((len >> 56) & 0xff, (len >> 48) & 0xff);
		cmp.emitBytes((len >> 40) & 0xff, (len >> 32) & 0xff);
		cmp.emitBytes((len >> 24) & 0xff, (len >> 16) & 0xff);
		cmp.emitBytes((len >> 8) & 0xff, (len) & 0xff);
	}

	cmp.consume("expect ']' at end of list", TOKEN_SQUARE_CLOSE);
}

void compiler::map(bool canAssign, compiler& cmp) {
	size_t len = 0;

	cmp.emitByte(OP_MAP);

	if (!cmp.check(TOKEN_BRACE_CLOSE)) {
		do {
			cmp.expression();
			cmp.match(TOKEN_COLON);
			cmp.expression();
			len++;
		} while (cmp.match(TOKEN_COMMA));
	}

	if (len != 0) {
		cmp.emitByte(OP_MAP_APPEND);
		cmp.emitBytes((len >> 56) & 0xff, (len >> 48) & 0xff);
		cmp.emitBytes((len >> 40) & 0xff, (len >> 32) & 0xff);
		cmp.emitBytes((len >> 24) & 0xff, (len >> 16) & 0xff);
		cmp.emitBytes((len >> 8) & 0xff, (len) & 0xff);
	}

	cmp.consume("expect '}' at end of dictionary", TOKEN_BRACE_CLOSE);
}

void compiler::index(bool canAssign, compiler& cmp) {
	cmp.expression();
	cmp.consume("expect ']' after index", TOKEN_SQUARE_CLOSE);
	if (canAssign && cmp.match(TOKEN_EQUALS)) {
		cmp.expression();
		cmp.emitByte(OP_SET_INDEX);
	}
	else {
		cmp.emitByte(OP_GET_INDEX);
	}
}

void compiler::this_key(bool canAssign, compiler& cmp) {

	if (!(cmp.currentPosition == TYPE_METHOD || cmp.currentPosition == TYPE_INIT)) {
		cmp.error("can only use 'this' in class methods");
	}

	cmp.emitByte(OP_THIS);
}

void compiler::super_key(bool canAssign, compiler& cmp) {
	if (!(cmp.currentPosition == TYPE_METHOD || cmp.currentPosition == TYPE_INIT)) {
		cmp.error("can only use 'super' in class methods");
	}

	cmp.consume("expect '.' after 'super' keyword", TOKEN_DOT);
	cmp.consume("expect identifier", TOKEN_IDENTIFIER);
	int index = cmp.identifierConstant(cmp.prevToken);

	if (cmp.match(TOKEN_PAREN_OPEN)) {
		cmp.emitByte(OP_THIS);
		int argc = 0;
		if (!cmp.check(TOKEN_PAREN_CLOSE)) {
			do {
				cmp.expression();
				argc++;
				if (argc > UINT8_MAX) {
					cmp.error("functions can have a maximum of 255 arguments");
				}
			} while (cmp.match(TOKEN_COMMA));
		}
		cmp.consume("expect ')' after function call", TOKEN_PAREN_CLOSE);
		cmp.emitByte(OP_SUPER_INVOKE);
		cmp.emitBytes((index >> 8) & 0xff, (index) & 0xff);
		cmp.emitByte((opCodes)argc);
	}
	else {
		cmp.emitByte(OP_SUPER);
		cmp.emitBytes((index >> 8) & 0xff, (index) & 0xff);
	}
}

void compiler::ternary(bool canAssign, compiler& cmp) {
	size_t elseJump = cmp.emitJump(OP_JUMP_IF_FALSE);
	cmp.emitByte(OP_POP);

	cmp.expression();
	size_t exitJump = cmp.emitJump(OP_JUMP);

	cmp.patchJump(elseJump);
	cmp.emitByte(OP_POP);
	cmp.consume("expect ':' after first expression", TOKEN_COLON);

	cmp.expression();
	cmp.patchJump(exitJump);
}

void compiler::preCrement(bool canAssign, compiler& cmp) {
	token op = cmp.prevToken;

	bool isConst = false;

	int var = cmp.resolveLocal(isConst, cmp.currentToken);

	if (var == -1) {
		var = cmp.identifierConstant(cmp.currentToken);

		std::string constNameStr(cmp.currentToken.start, cmp.currentToken.len);

		cmp.checkConsts(isConst, OP_INCREMENT_GLOBAL, constNameStr);

		if (op.type == TOKEN_PLUS_PLUS) {
			cmp.emitByte(OP_INCREMENT_GLOBAL);
		}
		else {
			cmp.emitByte(OP_DECREMENT_GLOBAL);
		}
	}
	else {

		std::string constNameStr(cmp.currentToken.start, cmp.currentToken.len);
		cmp.checkConsts(isConst, OP_SET_LOCAL, constNameStr);

		if (op.type == TOKEN_PLUS_PLUS) {
			cmp.emitByte(OP_INCREMENT_LOCAL);
		}
		else {
			cmp.emitByte(OP_DECREMENT_LOCAL);
		}
	}

	cmp.emitBytes((var >> 8) & 0xff, var & 0xff);

	cmp.expression();
}


void compiler::expression() {
	parsePrec(PREC_ASSIGN);
}

void compiler::beginScope() {
	scopeDepth++;
}

void compiler::endScope() {
	scopeDepth--;
	while (!locals.empty() && locals.at(locals.size() - 1).depth > scopeDepth) {
		if (locals.at(locals.size() - 1).isCaptured) {
			emitByte(OP_CLOSE_UPVALUE);
		}
		else {
			emitByte(OP_POP);
		}
		locals.pop_back();
	}
}

void compiler::endFunctionScope() {
	scopeDepth--;

	while (!locals.empty() && locals.at(locals.size() - 1).depth > scopeDepth) {
		//emitByte(OP_POP);
		locals.pop_back();
	}
}

void compiler::block() {
	while (!check(TOKEN_BRACE_CLOSE) && !check(TOKEN_EOF)) {
		declaration();
	}

	consume("expected '}' to end block", TOKEN_BRACE_CLOSE);
}

void compiler::importStatement() {
	expression();
	consume("expect ';' after import statement", TOKEN_SEMICOLON);
	emitByte(OP_IMPORT);
}

void compiler::switchCase(uint64_t& prevCaseJump) {
	expression();	// parsing the case 'HERE':
	emitByte(OP_CASE_COMPARE);
	uint64_t falseJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	emitByte(OP_POP);	// popping the value from the switch
	if (prevCaseJump != 0) {		// prevCaseJump is 0 for the first case
		patchJump(prevCaseJump);	// if the previous case was true,
	}								// it jumps over this comparison and 'falls through' to here
	consume("expect ':' after case-expression", TOKEN_COLON);
	while (!(check(TOKEN_BRACE_CLOSE) || check(TOKEN_DEFAULT) || check(TOKEN_CASE))) {
		statement();
	}
	prevCaseJump = emitJump(OP_JUMP);
	patchJump(falseJump);
	emitByte(OP_POP);	// should pop 'false' from comparison result
}

void compiler::defaultCase(uint64_t& prevCaseJump) {
	consume("expected ':' after default-case", TOKEN_COLON);
	emitByte(OP_POP);	// popping the value if default is active/no case was right
	if (prevCaseJump != 0) {
		patchJump(prevCaseJump);
	}
	while (!(check(TOKEN_BRACE_CLOSE) || check(TOKEN_DEFAULT) || check(TOKEN_CASE))) {
		statement();
	}

	// if no break is found, must jump to avoid the extra pop statement
	prevCaseJump = emitJump(OP_JUMP);
}

void compiler::switchStatement() {
	consume("expected '(' after 'switch' keyword", TOKEN_PAREN_OPEN);
	expression();
	consume("expected ')' after 'switch' expression", TOKEN_PAREN_CLOSE);
	consume("expected '{' as switch-body", TOKEN_BRACE_OPEN);
	loopDepth++;	// entering loopdepth to enable break statements
	uint64_t previousLoopDepth = currentLoopDepth;
	currentLoopDepth = scopeDepth;
	uint64_t previousCaseJump = 0;
	while (match(TOKEN_CASE)) {
		switchCase(previousCaseJump);
	}
	if (match(TOKEN_DEFAULT)) {
		defaultCase(previousCaseJump);
	}
	if (match(TOKEN_CASE)) {
		error("default must be the last case");
	}
	// popping the value if no case was right
	emitByte(OP_POP);
	// if last case was right, and they fell through
	// they jump to this label
	if (previousCaseJump == 0) {
		patchJump(previousCaseJump);
	}
	consume("expected '}' after switch-statement", TOKEN_BRACE_CLOSE);
	currentLoopDepth = previousLoopDepth;
	loopDepth--;
	patchBreaks();
	// emitByte(OP_POP);	// only popping at the end
}

void compiler::expressionStatement() {
	if (match(TOKEN_SEMICOLON)) {
		return;
	}
	expression();

	consume("expect ';' after end of expression", TOKEN_SEMICOLON);
	emitByte(OP_POP);
}


void compiler::patchJump(size_t offset) {
	int jump = currentFunction->funChunk->getSize() - offset - 2;

	if (jump > UINT16_MAX)
		error("loop body too big");

	currentFunction->funChunk->accessAt(offset) = char(short(jump >> 8)) & 0xff;
	currentFunction->funChunk->accessAt(offset + 1) = char(jump) & 0xff;
}

void compiler::whileStatement() {
	size_t loopStart = currentFunction->funChunk->getSize();
	consume("expect '(' after while statement", TOKEN_PAREN_OPEN);
	expression();
	consume("expect ')' after while condition", TOKEN_PAREN_CLOSE);

	size_t exitJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);

	int64_t prevContinue = currentContinue;
	int64_t prevContinueDepth = currentLoopDepth;

	currentContinue = loopStart;
	currentLoopDepth = scopeDepth;
	loopDepth++;
	statement();
	loopDepth--;
	currentContinue = prevContinue;
	currentLoopDepth = prevContinueDepth;

	emitLoop(loopStart);

	patchJump(exitJump);
	emitByte(OP_POP);



	for (int64_t i = breakJumps.size() - 1; i >= 0; --i) {
		if (breakJumps.empty())
			break;

		if (breakJumps.at(i).depth == loopDepth + 1) {
			patchJump(breakJumps.at(i).pos);
			breakJumps.pop_back();
		}
		else {
			break;
		}
	}
}

void compiler::ifStatement() {
	consume("expect '(' after if statement", TOKEN_PAREN_OPEN);
	expression();
	consume("expect ')' after if condition", TOKEN_PAREN_CLOSE);

	size_t elseJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();
	size_t exitJump = emitJump(OP_JUMP);

	patchJump(elseJump);
	emitByte(OP_POP);
	if (match(TOKEN_ELSE)) {
		statement();
	}

	patchJump(exitJump);
}

void compiler::returnStatement() {

	if (currentPosition == TYPE_SCRIPT) {
		error("can't return from top level script");
	}
	if (check(TOKEN_SEMICOLON)) {
		// if method is a constructor, empty return returns the object
		if (currentPosition == TYPE_INIT) {
			emitByte(OP_THIS);
		}
		else {
			emitByte(OP_NIL);
		}
	}
	else {
		// if method is a constructor, return must be empty
		if (currentPosition == TYPE_INIT) {
			error("init method can't return any value - use 'return;' to return early");
		}
		expression();
	}
	emitByte(OP_RETURN);
	consume("expect ';' after return statement", TOKEN_SEMICOLON);
}

void compiler::breakStatement() {
	if (loopDepth < 1) {
		error("can only break out of loops");
		return;
	}

	consume("expect ';' after break statement", TOKEN_SEMICOLON);

	// poppint of all local variables when jumping out of loops
	for (int i = locals.size() - 1; i >= 0; --i)
	{
		local tmp = locals.at(i);
		if (tmp.depth <= currentLoopDepth)
			break;
		emitByte(OP_POP);
	}

	breakJumps.push_back({ emitJump(OP_JUMP) ,loopDepth });
}

void compiler::continueStatement() {
	if (currentContinue == -1) {
		error("can only use 'continue' inside of loops");
		return;
	}

	consume("expect ';' after continue statement", TOKEN_SEMICOLON);

	// trying popping all current local variables

	// endScope();
	for (int i = locals.size() - 1; i >= 0; --i)
	{
		local tmp = locals.at(i);
		if (tmp.depth <= currentLoopDepth)
			break;
		emitByte(OP_POP);
	}
	emitLoop(currentContinue);
}

void compiler::patchBreaks() {
	for (int64_t i = breakJumps.size() - 1; i >= 0; --i) {
		if (breakJumps.empty())
			break;

		if (breakJumps.at(i).depth == loopDepth + 1) {
			patchJump(breakJumps.at(i).pos);
			breakJumps.pop_back();
		}
		else {
			break;
		}
	}
}

void compiler::forStatement() {
	beginScope();
	consume("expect '(' after for statement", TOKEN_PAREN_OPEN);
	if (match(TOKEN_SEMICOLON)) {

	}
	else if (match(TOKEN_LET)) {
		letDeclaration();
	}
	else {
		expressionStatement();
	}

	size_t loopStart = currentFunction->funChunk->getSize();
	int exitJump = -1;

	if (!match(TOKEN_SEMICOLON)) {
		expression();
		consume("expect ';' after loop expression", TOKEN_SEMICOLON);

		exitJump = emitJump(OP_JUMP_IF_FALSE);
		emitByte(OP_POP);
	}

	if (!match(TOKEN_PAREN_CLOSE)) {
		size_t bodyJump = emitJump(OP_JUMP);
		size_t incrementStart = currentFunction->funChunk->getSize();
		expression();
		emitByte(OP_POP);
		consume("expect ')' after for clause", TOKEN_PAREN_CLOSE);

		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	//maybe make break a statement and hand jump back to here?
	int64_t prevContinue = currentContinue;
	int64_t prevLoopDepth = currentLoopDepth;

	currentContinue = loopStart;
	currentLoopDepth = scopeDepth;
	loopDepth++;
	statement();
	loopDepth--;
	currentContinue = prevContinue;
	currentLoopDepth = prevLoopDepth;

	emitLoop(loopStart);

	if (exitJump != -1) {
		patchJump(exitJump);
		emitByte(OP_POP);
	}


	patchBreaks();

	endScope();
}

void compiler::statement() {

	if (match(TOKEN_CONTINUE)) {
		continueStatement();
	}
	else if (match(TOKEN_BREAK)) {
		breakStatement();
	}
	else if (match(TOKEN_RETURN)) {
		returnStatement();
	}
	else if (match(TOKEN_BRACE_OPEN)) {
		beginScope();
		block();
		endScope();
	}
	else if (match(TOKEN_WHILE)) {
		whileStatement();
	}
	else if (match(TOKEN_IF)) {
		ifStatement();
	}
	else if (match(TOKEN_FOR)) {
		forStatement();
	}
	else if (match(TOKEN_IMPORT)) {
		importStatement();
	}
	else if (match(TOKEN_SWITCH)) {
		switchStatement();
	}
	else {
		expressionStatement();
	}
}

int compiler::identifierConstant(token& name) {
	return currentFunction->funChunk->addConstantGetLine(OBJ_VAL(objString::copyString(name.start, name.len)),
		name.line);
}

void compiler::addLocal(token name, bool isConst) {
	local tmp;
	tmp.depth = -1;
	tmp.name = name;
	tmp.isConst = isConst;
	tmp.funcDepth = functionDepth;
	tmp.isCaptured = false;

	locals.push_back(tmp);
}

void compiler::declareVariable(bool isConst) {
	if (scopeDepth == 0) return;

	token& name = prevToken;

	for (int i = locals.size() - 1; i >= 0; --i) {
		local* loc = &locals.at(i);
		if (loc->depth != -1 && loc->depth < scopeDepth) {
			break;
		}

		if (identifiersEqual(name, loc->name)) {
			error("already a variable with this name in this scope");
		}
	}

	addLocal(name, isConst);
}

unsigned int compiler::parseVariable(const char* msg, bool isConst) {
	consume(msg, TOKEN_IDENTIFIER);

	declareVariable(isConst);
	if (scopeDepth > 0) return 0;

	return identifierConstant(prevToken);
}

void compiler::markInitialized() {
	if (scopeDepth == 0) return;

	locals.at(locals.size() - 1).depth = scopeDepth;
}

void compiler::defineVariable(unsigned int global) {
	if (scopeDepth > 0) {
		markInitialized();
		return;
	}

	emitByte(OP_DEFINE_GLOBAL);
	emitBytes((global >> 8) & 0xff, global & 0xff);
}

void compiler::constDeclaration() {

multiDeclaration:

	token constIdentifier = currentToken;

	const bool constSuccess = varDeclaration(true);

	if (scopeDepth == 0 && constSuccess) {
		std::string constName(constIdentifier.start, constIdentifier.len);
		globalConsts.emplace_back(constName);
	}

	if (match(TOKEN_COMMA))
		goto multiDeclaration;

	consume("expect ';' after variable declaration", TOKEN_SEMICOLON);
}

void compiler::letDeclaration() {

multiDeclaration:

	varDeclaration();

	if (match(TOKEN_COMMA))
		goto multiDeclaration;
	consume("expect ';' after variable declaration", TOKEN_SEMICOLON);
}

bool compiler::varDeclaration(bool isConst) {
	unsigned int global = parseVariable("expect variable name", isConst);

	if (scopeDepth == 0) {
		std::string name(prevToken.start, prevToken.len);
		for (const auto& el : globalConsts) {
			if (el == name) {
				error("cannot redeclare previously declared const");
			}
		}
	}

	if (match(TOKEN_EQUALS)) {
		expression();
	}
	else {
		if (isConst) {
			error("can't implicitly initialize 'const' with nil");
			return false;
		}
		emitByte(OP_NIL);
	}

	defineVariable(global);

	return true;
}

int compiler::argumentList() {
	int argc = 0;
	if (check(TOKEN_IDENTIFIER)) {
		do {
			size_t para = parseVariable("expect parameter name", false);
			defineVariable(para);
			argc++;
			if (argc > UINT8_MAX) {
				error("functions can have a maximum of 255 arguments");
			}
		} while (match(TOKEN_COMMA));
	}

	return argc;
}

void compiler::function() {

	functionDepth++;

	auto prevLocalOffset = functionLocalOffset;
	functionLocalOffset = locals.size();

	auto* prevUpvalues = upvalues;

	/**
	 * TODO: check here, if upvalue vectors
	 * behave correctly
	 * 
	 */
	upvalues = new std::vector<upvalue>;

	objFunction* prevFunction = currentFunction;


	objString* name = objString::copyString(prevToken.start, prevToken.len);

	currentFunction = objFunction::createObjFunction(name, new chunk(), -1);

	int argc = 0;

	//auto *fun = objFunction::createObjFunction(0, 0);
	//currentChunk->writeConstant(value((obj*)fun), prevToken.line);

	beginScope();

	consume("expect '(' after function name", TOKEN_PAREN_OPEN);

	argc = argumentList();

	consume("expect ')' after function arguments", TOKEN_PAREN_CLOSE);

	consume("expected '{' before function body", TOKEN_BRACE_OPEN);
	block();
	if (currentPosition == TYPE_INIT) {
		// because init MUST return this
		emitByte(OP_THIS);
	}
	else {
		emitByte(OP_NIL);
	}
	emitByte(OP_RETURN);
	endFunctionScope();

	currentFunction->arity = argc;

	// adding function to previous chunk, with currentchunk containing its body
	unsigned int fun = prevFunction->funChunk->addConstantGetLine(OBJ_VAL(currentFunction), prevToken.line);



	functionDepth--;

	/**
	 * TODO: check the functions upvalue count, if 0 don't emit closure
	 * but use regular function
	 * 
	 */

	objFunction* newFunc = currentFunction;

	currentFunction = prevFunction;

	emitByte(OP_CLOSURE);
	emitBytes((fun >> 8) & 0xff, fun & 0xff);

	for (int i = 0; i < newFunc->upvalueCount; i++)
	{
		emitByte((opCodes)(upvalues->at(i).isLocal ? 1 : 0));
		emitByte((opCodes)(upvalues->at(i).index));
	}

	// deleting the current upvalues, if 
	delete upvalues;
	upvalues = prevUpvalues;
	functionLocalOffset = prevLocalOffset;

	//fun->setData(jump, argc);

#ifdef DEBUG_PRINT_CODE
	std::cout << " == " << name->getChars() << " == " << std::endl;
	debug::disassembleChunk(newFunc->funChunk);
	std::cout << std::endl;
#endif
}

void compiler::functionDeclaration() {
	if (scopeDepth != 0) {
		// error("functions can only be declared in top level code");
	}

	position prevPos = currentPosition;
	currentPosition = TYPE_FUNCTION;

	unsigned int var = parseVariable("expect function name", false);
	markInitialized();
	function();
	defineVariable(var);


	currentPosition = prevPos;
}



void compiler::method() {

	int constant = identifierConstant(prevToken);

	token tmpInitToken{ TOKEN_IDENTIFIER, "init", 4, prevToken.line };
	position prevPos = currentPosition;
	if (identifiersEqual(prevToken, tmpInitToken)) {
		currentPosition = TYPE_INIT;
	}

	function();
	currentPosition = prevPos;

	emitByte(OP_METHOD);
	emitBytes((constant >> 8) & 0xff, constant & 0xff);
}

void compiler::memberVar() {

	int constant = identifierConstant(prevToken);

	if (match(TOKEN_EQUALS)) {
		expression();
	}
	else {
		emitByte(OP_NIL);
	}

	consume("expect ';' after member variable", TOKEN_SEMICOLON);

	emitByte(OP_MEMBER_VARIABLE);
	emitBytes((constant >> 8) & 0xff, constant & 0xff);
}

void compiler::classDeclaration() {
	if (scopeDepth != 0) {
		error("classes can only be declared in top level code");
	}

	consume("expect class name", TOKEN_IDENTIFIER);
	token className = prevToken;
	unsigned int var = identifierConstant(prevToken);
	declareVariable(false);


	emitByte(OP_CLASS);
	emitBytes((var >> 8) & 0xff, var & 0xff);

	if (match(TOKEN_COLON)) {
		expression();
		emitByte(OP_INHERIT);
	}

	defineVariable(var);

	consume("expect '{' after class name", TOKEN_BRACE_OPEN);

	///inner methods
	namedVariable(className, false);
	beginScope();
	position prevPos = currentPosition;
	currentPosition = TYPE_METHOD;
	while (!check(TOKEN_BRACE_CLOSE) && !check(TOKEN_EOF) && !hadError) {
		if (match(TOKEN_FUN)) {
			consume("expect method name", TOKEN_IDENTIFIER);
			method();
		}
		else {
			consume("expect identifier", TOKEN_IDENTIFIER);
			if (check(TOKEN_PAREN_OPEN)) {
				method();
			}
			else {
				memberVar();
			}
		}
	}
	currentPosition = prevPos;

	consume("expect '}' after class body", TOKEN_BRACE_CLOSE);
	emitByte(OP_POP);

	endScope();
}

void compiler::declaration() {
	if (match(TOKEN_FUN)) {
		functionDeclaration();
	}
	else if (match(TOKEN_LET)) {
		letDeclaration();
	}
	else if (match(TOKEN_CONST)) {
		constDeclaration();
	}
	else if (match(TOKEN_CLASS)) {
		classDeclaration();
	}
	else {
		statement();
	}

	if (panicMode)
		synchronize();
}

void compiler::parsePrec(precedence prec) {
	advance();
	parseFn prefixRule = getRule(prevToken)->prefix;
	if (prefixRule == nullptr) {
		error("expected expression");
		return;
	}

	bool canAssign = prec <= PREC_ASSIGN;
	prefixRule(canAssign, *this);

	while (prec <= getRule(currentToken)->prec) {
		advance();
		parseFn infixRule = getRule(prevToken)->infix;
		if (infixRule == nullptr) {
			error("unexpected character");
			return;
		}
		infixRule(canAssign, *this);
	}

	if (canAssign && match(TOKEN_EQUALS)) {
		error("invalid assignment target");
	}
}

objFunction* compiler::compiling(const char* name, char* str) {

	scanr.init(str);


	currentFunction = objFunction::createObjFunction(objString::copyString(name, strlen(name)), new chunk(), 0);

	currentToken = scanr.scanToken();

	while (currentToken.type != TOKEN_EOF) {
		declaration();
	}
	emitByte(OP_RETURN);

#ifdef DEBUG_PRINT_CODE
	char* ptr = currentFunction->funChunk->getInstructionPointer();
	std::cout << " == " << name << " == " << std::endl;
	debug::disassembleChunk(currentFunction->funChunk);
#endif

	return currentFunction;
}

bool compiler::errorOccured() {
	if (hadError) {
		hadError = false;
		return true;
	}
	return hadError;
}
