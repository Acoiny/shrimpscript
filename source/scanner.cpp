#include "../header/scanner.hpp"

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
    scanToken();
    previousToken = {};
    currentToken = {};
}

token scanner::scanToken() {
    previousToken = currentToken;
    lineBreakBeforeCurrent = lineBreakAfterCurrent;
    currentToken = lookoutToken;
    lookoutToken = advanceTokens();


    return currentToken;
}

token scanner::advanceTokens() {
    lineBreakAfterCurrent = false;
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
        case '+': {
            if (peek(1) == '+') {
                advance();
                return makeToken(TOKEN_PLUS_PLUS);
            }
            if (peek(1) == '=') {
                advance();
                return makeToken(TOKEN_PLUS_EQUALS);
            }
            return makeToken(TOKEN_PLUS);
        }
        case '-': {
            if (peek(1) == '-') {
                advance();
                return makeToken(TOKEN_MINUS_MINUS);
            }
            if (peek(1) == '=') {
                advance();
                return makeToken(TOKEN_MINUS_EQUALS);
            }
            return makeToken(TOKEN_MINUS);
        }
        case '*': {
            if (peek(1) == '=') {
                advance();
                return makeToken(TOKEN_TIMES_EQUALS);
            }
            return makeToken(TOKEN_TIMES);
        }
        case '/': {
            if (peek(1) == '=') {
                advance();
                return makeToken(TOKEN_DIVIDE_EQUALS);
            }
            return makeToken(TOKEN_DIVIDE);
        }
        case '%': {
            if (peek(1) == '=') {
                advance();
                return makeToken(TOKEN_MODULO_EQUALS);
            }
            return makeToken(TOKEN_MODULO);
        }
        case '&': {
            if (peek(1) == '&') {
                advance();
                return makeToken(TOKEN_AND);
            }
            return makeToken(TOKEN_BIT_AND);
        }
        case '|': {
            if (peek(1) == '|') {
                advance();
                return makeToken(TOKEN_OR);
            }
            return makeToken(TOKEN_BIT_OR);
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
            else if (peek(1) == '<') {
                advance();
                return makeToken(TOKEN_BIT_SHIFT_LEFT);
            }
            return makeToken(TOKEN_LESS);
        }
        case '>': {
            if(peek(1) == '=') {
                advance();
                return makeToken(TOKEN_GREATER_EQUALS);
            }
            else if (peek(1) == '>') {
                advance();
                return makeToken(TOKEN_BIT_SHIFT_RIGHT);
            }
            return makeToken(TOKEN_GREATER);
        }
        case '~':
            return makeToken(TOKEN_BIT_NOT);
        case '^':
            return makeToken(TOKEN_BIT_XOR);
        case '!': {
            if(peek(1) == '=') {
                advance();
                return makeToken(TOKEN_BANG_EQUALS);
            } else {
                return makeToken(TOKEN_BANG);
            }
        }
        case '?':
            return makeToken(TOKEN_QUESTIONMARK);
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
    token str{TOKEN_STRING, current, 0, line};
    if (endAt == '\"')
        str.type = TOKEN_STRING_ESCAPE;
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
            case 'a':
                return confirmNext("ase", 3, TOKEN_CASE, tk);
            case 'l':
                return confirmNext("lass", 4, TOKEN_CLASS, tk);
            case 'o': {
                tk.len++;
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
        case 'i': {
            switch (peek(1)) {
            case 'f':
                return confirmNext("f", 1, TOKEN_IF, tk);
            default:
                return confirmNext("mport", 5, TOKEN_IMPORT, tk);
            }
        }
        case 'd':
            return confirmNext("efault", 6, TOKEN_DEFAULT, tk);
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
        case 's': {
            switch (peek(1)) {
            case 'u':
                return confirmNext("uper", 4, TOKEN_SUPER, tk);
            case 'w':
                return confirmNext("witch", 5, TOKEN_SWITCH, tk);
            default:
                return scanRestIdentifier(tk);
            }
        }
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
                lineBreakAfterCurrent = true;
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

bool scanner::currentIsOnNewLine() {
    return lineBreakBeforeCurrent;
}

tokenType scanner::viewNextType() {
    return lookoutToken.type;
}

bool scanner::checkASIfoundIllegal() {
    // currentToken because compiler mostly works on prevToken
    if (lineBreakBeforeCurrent || currentToken.type == TOKEN_BRACE_CLOSE ||
        currentToken.type == TOKEN_EOF) {
        return true;
    }

    return false;
}