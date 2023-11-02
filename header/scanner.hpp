#ifndef SHRIMPP_SCANNER_HPP
#define SHRIMPP_SCANNER_HPP

enum tokenType {
    TOKEN_ERROR,
    TOKEN_SEMICOLON,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_STRING_ESCAPE,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_PLUS,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS,
    TOKEN_MINUS_MINUS,
    TOKEN_TIMES,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
    TOKEN_BANG,

    TOKEN_QUESTIONMARK,

    TOKEN_BANG_EQUALS,
    TOKEN_EQUALS_EQUALS,
    TOKEN_LESS,
    TOKEN_LESS_EQUALS,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUALS,

    TOKEN_AND,
    TOKEN_OR,

    TOKEN_PAREN_OPEN,
    TOKEN_PAREN_CLOSE,
    TOKEN_NIL,

    TOKEN_BRACE_OPEN,
    TOKEN_BRACE_CLOSE,

    TOKEN_SQUARE_OPEN,
    TOKEN_SQUARE_CLOSE,

    //assign tokens
    TOKEN_EQUALS,
    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_TIMES_EQUALS,
    TOKEN_DIVIDE_EQUALS,
    TOKEN_MODULO_EQUALS,

    TOKEN_IDENTIFIER,
    TOKEN_LET,
    TOKEN_CONST,

    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,

    TOKEN_FUN,
    TOKEN_COMMA,
    TOKEN_RETURN,
    TOKEN_CLASS,
    TOKEN_THIS,
    TOKEN_SUPER,

    TOKEN_BREAK,
    TOKEN_CONTINUE,

    TOKEN_IMPORT,

    TOKEN_EOF
};

struct token{
    tokenType type;
    const char* start;
    int len;
    int line;
};

class scanner {
    char* source;
    char* current;
    int line = 0;

    char peek(int dist);

    char advance();

    void skipWhiteSpaces();

    token makeToken(tokenType);

    token numberToken();

    token stringToken(char endAt);

    token confirmNext(const char* name, int len, tokenType type, token tk);
    token identifierToken();
    token scanRestIdentifier(token tk);

    token errorToken(const char* msg);

public:

    explicit scanner();

    void init(char *str);

    token scanToken();
};


#endif //SHRIMPP_SCANNER_HPP
