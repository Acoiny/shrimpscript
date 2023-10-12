#ifndef SHRIMPP_COMPILER_HPP
#define SHRIMPP_COMPILER_HPP

#include "scanner.hpp"
#include "chunk.hpp"

class compiler;
class VM;
class objFunction;

enum position {
	TYPE_SCRIPT,
	TYPE_FUNCTION,
	TYPE_METHOD,
	TYPE_INIT
};

enum precedence {
	PREC_NONE,
	PREC_ASSIGN,
	PREC_CONDITIONAL,
	PREC_OR,
	PREC_AND,
	PREC_EQUALITY,
	PREC_COMPARISON,
	PREC_TERM,
	PREC_FACTOR,
	PREC_UNARY,
	PREC_INCREMENT,
	PREC_CALL,
	PREC_PRIMARY
};

typedef void (*parseFn)(bool, compiler&);

struct precFuncTableEntry {
	parseFn prefix;
	parseFn infix;
	precedence precedence;
};

struct local {
	int depth;
	token name;
	bool isConst;
};

struct breaks {
	size_t pos;
	unsigned int depth;
};

class compiler {
	VM& vm;

	scanner scanr;

	chunk* currentChunk;

	bool hadError = false;
	bool panicMode = false;

	token prevToken{};
	token currentToken{};

	unsigned int scopeDepth = 0;

	position currentPosition = TYPE_SCRIPT;

	unsigned int loopDepth = 0;

	int64_t currentContinue = -1;

	std::vector<breaks> breakJumps;

	void errorAt(token tk, const char* msg);

	void error(const char* msg);

	void synchronize();

	void advance();

	bool match(tokenType type);

	[[nodiscard]] bool check(tokenType type) const;

	void consume(const char* msg, tokenType type);

	void parsePrec(precedence prec);

	void emitByte(opCodes op);

	void emitBytes(char a, char b);

	size_t emitJump(opCodes jmpCode);

	void emitLoop(size_t loopStart);

	precFuncTableEntry* getRule(token tk);

	void declaration();

	void markInitialized();

	int identifierConstant(token& name);

	void declareVariable(bool isConst);

	int resolveLocal(bool &isConst);

	//overload to use in preCrement, which can't consume the identifier
	int resolveLocal(bool& isConst, token comp);

	void checkConsts(bool isConst, opCodes setOP, token& checkNameGlobal);

	void namedVariable(token& name, bool canAssign);

	void addLocal(token name, bool isConst);

	unsigned int parseVariable(const char* msg, bool isConst);

	void defineVariable(unsigned int global);

	void constDeclaration();

	void letDeclaration();

	int argumentList();

	void function();

	void functionDeclaration();

	void classDeclaration();

	void method();

	void memberVar();

	void returnStatement();

	void breakStatement();

	void continueStatement();

	void statement();

	void importStatement();

	void expressionStatement();

	void ifStatement();

	//NOT FINISHED!!
	void forEachLoop();

	void forStatement();

	void whileStatement();

	void patchJump(size_t offset);

	void beginScope();

	void block();

	void endScope();

	void endFunctionScope();

	void expression();

	static void number(bool canAssign, compiler& cmp);

	static void binary(bool canAssign, compiler& cmp);

	static void unary(bool canAssign, compiler& cmp);

	static void literal(bool canAssign, compiler& cmp);

	static void and_f(bool canAssign, compiler& cmp);

	static void or_f(bool canAssign, compiler& cmp);

	static void grouping(bool canAssign, compiler& cmp);

	static void string(bool canAssign, compiler& cmp);

	static void variable(bool canAssign, compiler& cmp);

	static void call(bool canAssign, compiler& cmp);

	static void dot(bool canAssign, compiler& cmp);

	static void list(bool canAssign, compiler& cmp);

	static void map(bool canAssign, compiler& cmp);

	static void index(bool canAssign, compiler& cmp);

	static void this_key(bool canAssign, compiler& cmp);

	static void super_key(bool canAssign, compiler& cmp);

	static void ternary(bool canAssign, compiler& cmp);

	static void preCrement(bool canAssign, compiler& cmp);

	precFuncTableEntry precedenceTable[50] = {
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_ERROR] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_SEMICOLON] = 
			{nullptr, dot, PREC_CALL}, //[TOKEN_DOT] =
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_COLON]
			{number, nullptr, PREC_NONE}, //[TOKEN_NUMBER] = 
			{string, nullptr, PREC_NONE}, //[TOKEN_STRING] = 
			{literal, nullptr, PREC_PRIMARY}, //[TOKEN_TRUE] = 
			{literal, nullptr, PREC_PRIMARY}, //[TOKEN_FALSE] = 
			{nullptr, binary, PREC_TERM}, //[TOKEN_PLUS] =
			//TODO: increment and decrement MUST change variable itself
			{preCrement, nullptr, PREC_INCREMENT}, //[TOKEN_PLUS_PLUS]
			{unary, binary, PREC_TERM},//[TOKEN_MINUS] = 
			{preCrement, nullptr, PREC_INCREMENT}, //[TOKEN_MINUS_MINUS]
			{nullptr, binary, PREC_FACTOR}, //[TOKEN_TIMES] = 
			{nullptr, binary, PREC_FACTOR}, //[TOKEN_DIVIDE] = 
			{nullptr, binary, PREC_FACTOR}, //[TOKEN_MODULO] = 
			{unary, nullptr, PREC_UNARY}, //[TOKEN_BANG] = 
			{nullptr, ternary, PREC_CONDITIONAL}, //[TOKEN_QUESTIONMARK]
			{nullptr, binary, PREC_COMPARISON}, //[TOKEN_BANG_EQUALS] = 
			{nullptr, binary, PREC_COMPARISON}, //[TOKEN_EQUALS_EQUALS] = 
			{nullptr, binary, PREC_COMPARISON}, //[TOKEN_LESS] = 
			{nullptr, binary, PREC_COMPARISON}, //[TOKEN_LESS_EQUALS] = 
			{nullptr, binary, PREC_COMPARISON}, //[TOKEN_GREATER] = 
			{nullptr, binary, PREC_COMPARISON}, //[TOKEN_GREATER_EQUALS] = 
			{nullptr, and_f, PREC_AND}, //[TOKEN_AND]
			{nullptr, or_f, PREC_OR}, //[TOKEN_OR]
			{grouping, call, PREC_CALL}, //[TOKEN_PAREN_OPEN] = 
			{nullptr, nullptr, PREC_NONE},//[TOKEN_PAREN_CLOSE] = 
			{literal, nullptr, PREC_PRIMARY},//[TOKEN_NIL] = 
			{map, nullptr, PREC_NONE}, //[TOKEN_BRACE_OPEN] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_BRACE_CLOSE] = 
			{list, index, PREC_CALL}, //[TOKEN_SQUARE_OPEN]
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_SQUARE_CLOSE]
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_EQUALS] = 
			{variable, nullptr, PREC_NONE}, //[TOKEN_IDENTIFIER] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_LET] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_CONST]
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_IF] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_ELSE] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_FOR] = 
			{nullptr, nullptr, PREC_NONE},//[TOKEN_WHILE] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_FUN] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_COMMA] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_RETURN] = 
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_CLASS] = 
			{this_key, nullptr, PREC_NONE}, //[TOKEN_THIS]
			{super_key, nullptr, PREC_NONE}, //[TOKEN_SUPER]
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_BREAK]
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_CONTINUE]
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_IMPORT]
			{nullptr, nullptr, PREC_NONE}, //[TOKEN_EOF] = 
	};

	std::vector<local> locals;

	std::vector<token> globalConsts;

public:
	explicit compiler(VM& vm);

	~compiler();

	objFunction* compiling(char* str);

	bool errorOccured();
};


#endif //SHRIMPP_COMPILER_HPP
