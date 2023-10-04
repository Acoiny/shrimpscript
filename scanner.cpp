#include "scanner.hpp"

#include <cstring>


token scanner::errorToken(const char *msg) {
    return token{TOKEN_ERROR, msg, (int)strlen(msg)};
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool isNumber(char c) {
    return (c >= '0' && c <= '9');
}

scanner::scanner() : source(nullptr), current(nullptr), line(1) {}

void scanner::init(char *str) {
    source = str;
    current = str;
}

token scanner::scanToken() {
    skipWhiteSpaces();
    switch (*(current)) {
        case '.':
            return makeToken(TOKEN_DOT);
        case ':':
            return makeToken(TOKEN_COLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case '{':
            return makeToken(TOKEN_BRACE_OPEN);
        case '}':
            return makeToken(TOKEN_BRACE_CLOSE);
        case '"':
            return stringToken('"');
        case '\'':
            return stringToken('\'');
        case '+':
            return makeToken(TOKEN_PLUS);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '*':
            return makeToken(TOKEN_TIMES);
        case '/':
            return makeToken(TOKEN_DIVIDE);
        case '%':
            return makeToken(TOKEN_MODULO);
        case '&': {
            if (peek(1) == '&') {
                advance();
                return makeToken(TOKEN_AND);
            }
            return makeToken(TOKEN_ERROR);
        }
        case '|': {
            if (peek(1) == '|') {
                advance();
                return makeToken(TOKEN_OR);
            }
            return makeToken(TOKEN_ERROR);
        }
        case '(':
            return makeToken(TOKEN_PAREN_OPEN);
        case ')':
            return makeToken(TOKEN_PAREN_CLOSE);
        case '[':
            return makeToken(TOKEN_SQUARE_OPEN);
        case ']':
            return makeToken(TOKEN_SQUARE_CLOSE);
        case '=': {
            if(peek(1) == '=') {
                advance();
                return makeToken(TOKEN_EQUALS_EQUALS);
            } else {
                return makeToken(TOKEN_EQUALS);
            }
        }
        case '<': {
            if(peek(1) == '=') {
                advance();
                return makeToken(TOKEN_LESS_EQUALS);
            }
            return makeToken(TOKEN_LESS);
        }
        case '>': {
            if(peek(1) == '=') {
                advance();
                return makeToken(TOKEN_GREATER_EQUALS);
            }
            return makeToken(TOKEN_GREATER);
        }
        case '!': {
            if(peek(1) == '=') {
                advance();
                return makeToken(TOKEN_BANG_EQUALS);
            } else {
                return makeToken(TOKEN_BANG);
            }
        }
        case '\0':
            return makeToken(TOKEN_EOF);

        default: {
            if (isNumber(*current)) {
                return numberToken();
            }
            if (isAlpha(*current) || *current == '_') {
                return identifierToken();
            }
            return makeToken(TOKEN_ERROR);
        }
    }
}

token scanner::numberToken() {
    token res{TOKEN_NUMBER, (current), 0, line};

    int len = 0;

    while (isNumber(*current)) {
        advance();
        len++;
    }
    if (*current == '.' && isNumber(peek(1))) {
        advance();
        len++;
        while (isNumber(*current)) {
            advance();
            len++;
        }
    }

    res.len = len;

    return res;
}

token scanner::stringToken(char endAt) {
    advance();
    token str{TOKEN_STRING, current};
    int len = 0;
    while(*current != endAt) {
        if(*current == '\0') {
            return errorToken("unterminated string");
        }
        len++;
        advance();
    }
    advance();
    str.len = len;
    return str;
}

token scanner::confirmNext(const char *name, int len, tokenType type, token tk) {
    tk.type = type;
    int tkLen = 0;
    bool isKeyWord = true;
    advance();

    for (int i = 0; i < len; ++i) {
        if(!(isAlpha(peek(0)) || isNumber(peek(0)) || peek(0) == '_')) {
            isKeyWord = false;
            break;
        }
        if(name[i] != peek(0)) {
            isKeyWord = false;
        }
        advance();
        tkLen++;
    }
    while(isAlpha(peek(0)) || isNumber(peek(0)) || peek(0) == '_') {
        isKeyWord = false;
        tkLen++;
        advance();
    }

    if(!isKeyWord)
        tk.type = TOKEN_IDENTIFIER;

    tk.len += tkLen;
    return tk;
}

token scanner::scanRestIdentifier(token tk) {
    advance();
    int adLen = 0;
    while(isAlpha(peek(0)) || isNumber(peek(0)) || peek(0) == '_') {
        adLen++;
        advance();
    }
    tk.len += adLen;
    return tk;
}

token scanner::identifierToken() {
    token tk{TOKEN_IDENTIFIER, current, 1, line};
    switch(peek(0)) {
    case 'a':
        return confirmNext("nd", 2, TOKEN_AND, tk);
    case 'b':
        return confirmNext("reak", 4, TOKEN_BREAK, tk);
        case 'c':
            switch (peek(1)) {
            case 'l':
                return confirmNext("lass", 4, TOKEN_CLASS, tk);
            case 'o': {
                advance();
                if (peek(1) == 'n') {
                    advance();
                    if (peek(1) == 's') {
                        return confirmNext("st", 2, TOKEN_CONST, tk);
                    }
                    return confirmNext("tinue", 5, TOKEN_CONTINUE, tk);
                }
                return scanRestIdentifier(tk);
            }
            default:
                return scanRestIdentifier(tk);
            }
        case 'i':
            return confirmNext("f", 1, TOKEN_IF, tk);
        case 'e':
            return confirmNext("lse", 3, TOKEN_ELSE, tk);
        case 'l':
            return confirmNext("et", 2, TOKEN_LET, tk);
        case 't':
            switch (peek(1)) {
            case 'r':
                return confirmNext("rue", 3, TOKEN_TRUE, tk);
            case 'h':
                return confirmNext("his", 3, TOKEN_THIS, tk);
            default:
                return scanRestIdentifier(tk);
            }
        case 'f':{
            switch(peek(1)) {
                case 'u':
                    return confirmNext("un", 2, TOKEN_FUN, tk);
                case 'o':
                    return confirmNext("or", 2, TOKEN_FOR, tk);
                case 'a':
                    return confirmNext("alse", 4, TOKEN_FALSE, tk);
                default:
                    return scanRestIdentifier(tk);
            }
        }
        case 'n':
            return confirmNext("il",2,TOKEN_NIL, tk);
        case 'o':
            return confirmNext("r", 1, TOKEN_OR, tk);
        case 'r':
            return confirmNext("eturn", 5, TOKEN_RETURN, tk);
        case 'w':
            return confirmNext("hile", 4, TOKEN_WHILE, tk);
        default:{
            int adLen = 0;
            while(isAlpha(peek(0)) || peek(0) == '_' || isNumber(peek(0))) {
                advance();
                adLen++;
            }
            tk.len = adLen;
            return tk;
        }
    }
}

char scanner::peek(int dist) {
    return *(current + dist);
}

char scanner::advance() {
    return *current++;
}

void scanner::skipWhiteSpaces() {
    for(;;) {
        switch (*current) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                advance();
                line++;
                break;
            case '/':
                if(peek(1) == '/') {
                    while(peek(0) != '\n')
                        advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

token scanner::makeToken(tokenType type) {
    if (type == TOKEN_ERROR) {
        token err{ type, "unknown character", 17, line };
        advance();
        return err;
    }
    token res{type, (current), 1, line};
    advance();
    return res;
}
