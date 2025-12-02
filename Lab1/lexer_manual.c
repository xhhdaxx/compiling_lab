#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_ID_LEN  50
#define MAX_NUM_LEN 50
#define MAX_STR_LEN 200

// token types
enum {
    SYM_NULL = 0,            // 空
    SYM_IDENTIFIER = 1,      // 标识符
    SYM_NUMBER = 2,          // 整数

    SYM_PLUS = 3,            // +
    SYM_MINUS = 4,           // -
    SYM_TIMES = 5,           // *
    SYM_SLASH = 6,           // /
    SYM_EQU = 7,             // =
    SYM_GTR = 8,             // >
    SYM_LES = 9,             // <
    SYM_NEQ = 10,            // <>
    SYM_LEQ = 11,            // <=
    SYM_GEQ = 12,            // >=

    SYM_LPAREN = 13,         // (
    SYM_RPAREN = 14,         // )
    SYM_LBRACE = 15,         // {
    SYM_RBRACE = 16,         // }
    SYM_SEMICOLON = 17,      // ;
    SYM_COMMA = 18,          // , 
    SYM_ASSIGN = 20,         // :=

    SYM_VAR = 21,            // var
    SYM_IF = 22,             // if
    SYM_THEN = 23,           // then
    SYM_ELSE = 24,           // else
    SYM_WHILE = 25,          // while
    SYM_FOR = 26,            // for
    SYM_BEGIN = 27,          // begin
    SYM_WRITELN = 28,        // writeln
    SYM_PROCEDURE = 29,      // procedure
    SYM_END = 30,            // end

    SYM_CONST = 31,          // const
    SYM_CALL = 32,           // call
    SYM_DO = 33,             // do
    SYM_WRITE = 34,          // write
    SYM_PERIOD = 35,         // .
    SYM_STRING = 36,         // "string"

    SYM_LBRACKET = 37,       // [
    SYM_RBRACKET = 38,       // ]

    SYM_ERROR = 100          // 出错
};

// state types
typedef enum {
    STATE_START,
    STATE_INID,        // 标识符状态
    STATE_INNUM,       // 数字状态  
    STATE_INASSIGN,    // 遇到 :
    STATE_INCOMMENT,   // 注释状态
    STATE_INSTRING,    // 字符串状态（你原来的 STATE_INCHAR 改名更准确）
    STATE_DONE,        // 完成状态
    STATE_ERROR        // 出错状态
} DFA_State;

FILE *fin;
int ch;

// 读取下一个字符
void getch() {
    ch = fgetc(fin);
}

// 判断保留字
int check_reserved(const char *s) {
    if (!strcmp(s, "var")) return SYM_VAR;
    if (!strcmp(s, "if")) return SYM_IF;
    if (!strcmp(s, "then")) return SYM_THEN;
    if (!strcmp(s, "else")) return SYM_ELSE;
    if (!strcmp(s, "while")) return SYM_WHILE;
    if (!strcmp(s, "for")) return SYM_FOR;
    if (!strcmp(s, "begin")) return SYM_BEGIN;
    if (!strcmp(s, "writeln")) return SYM_WRITELN;
    if (!strcmp(s, "procedure")) return SYM_PROCEDURE;
    if (!strcmp(s, "end")) return SYM_END;

    if (!strcmp(s, "const")) return SYM_CONST;
    if (!strcmp(s, "call")) return SYM_CALL;
    if (!strcmp(s, "do")) return SYM_DO;
    if (!strcmp(s, "write")) return SYM_WRITE;
    return 0;
}

// 输出 token
void print_token(int sym, const char *text) {
    if (sym == SYM_IDENTIFIER)
        printf("(1, \"%s\")\n", text);
    else if (sym == SYM_NUMBER)
        printf("(2, %s)\n", text);
    else if (sym == SYM_STRING)
        printf("(36, \"%s\")\n", text);
    else if (sym == SYM_ERROR)
        printf("(100, \"%s\")\n", text);
    else
        printf("(%d, \"%s\")\n", sym, text);
}

int getsym() {
    DFA_State state = STATE_START;
    char buf[MAX_STR_LEN] = {0};
    int k = 0;

    while (state != STATE_DONE && state != STATE_ERROR) {

        switch (state) {

            //=========================
            //     初始状态
            //=========================
            case STATE_START:

                // 空白
                while (isspace(ch)) getch();

                // 标识符
                if (isalpha(ch)) {
                    state = STATE_INID;
                    buf[k++] = ch;
                    getch();
                    break;
                }

                // 数字
                if (isdigit(ch)) {
                    state = STATE_INNUM;
                    buf[k++] = ch;
                    getch();
                    break;
                }

                // :=
                if (ch == ':') {
                    state = STATE_INASSIGN;
                    buf[k++] = ':';
                    getch();
                    break;
                }

                // 注释 { ... }
                if (ch == '{') {
                    state = STATE_INCOMMENT;
                    getch(); // 跳过 {
                    break;
                }

                // 字符串
                if (ch == '"') {
                    state = STATE_INSTRING;
                    getch(); // 跳过开头 "
                    break;
                }

                // EOF
                if (ch == EOF) return SYM_NULL;

                // <, <=, <>
                if (ch == '<') {
                    getch();
                    if (ch == '=') {
                        getch();
                        print_token(SYM_LEQ, "<=");
                        return SYM_LEQ;
                    }
                    if (ch == '>') {
                        getch();
                        print_token(SYM_NEQ, "<>");
                        return SYM_NEQ;
                    }
                    print_token(SYM_LES, "<");
                    return SYM_LES;
                }
    
                // >, >=
                if (ch == '>') {
                    getch();
                    if (ch == '=') {
                        getch();
                        print_token(SYM_GEQ, ">=");
                        return SYM_GEQ;
                    }
                    print_token(SYM_GTR, ">");
                    return SYM_GTR;
                }

                // 单字符符号 + 错误符号
                int cur = ch;
                getch();

                // 单字符符号
                {
                    switch (cur) {
                        case '+': print_token(SYM_PLUS, "+"); return SYM_PLUS;
                        case '-': print_token(SYM_MINUS, "-"); return SYM_MINUS;
                        case '*': print_token(SYM_TIMES, "*"); return SYM_TIMES;
                        case '/': print_token(SYM_SLASH, "/"); return SYM_SLASH;
                        case '(': print_token(SYM_LPAREN, "("); return SYM_LPAREN;
                        case ')': print_token(SYM_RPAREN, ")"); return SYM_RPAREN;
                        case ';': print_token(SYM_SEMICOLON, ";"); return SYM_SEMICOLON;
                        case '[': print_token(SYM_LBRACKET, "["); return SYM_LBRACKET;
                        case ']': print_token(SYM_RBRACKET, "]"); return SYM_RBRACKET;
                        case '=': print_token(SYM_EQU, "="); return SYM_EQU;
                        case ',': print_token(SYM_COMMA, ","); return SYM_COMMA;
                        case '.': print_token(SYM_PERIOD, "."); return SYM_PERIOD;
                    }
                }

                // 错误符号
                {   // 非法字符 —— 使用 cur（原始字符）
                    char illegal[2];
                    illegal[0] = (char)cur;
                    illegal[1] = '\0';
                    print_token(SYM_ERROR, illegal);
                    return SYM_ERROR;
                }


            //=========================
            //     标识符状态
            //=========================
            case STATE_INID:
                while (isalnum(ch) && k < MAX_ID_LEN - 1) {
                    buf[k++] = ch;
                    getch();
                }
                buf[k] = '\0';
                {
                    int reserved = check_reserved(buf);
                    if (reserved) {
                        print_token(reserved, buf);
                        return reserved;
                    }
                    print_token(SYM_IDENTIFIER, buf);
                    return SYM_IDENTIFIER;
                }

            //=========================
            //     数字状态
            //=========================
            case STATE_INNUM:
                while (isdigit(ch) && k < MAX_NUM_LEN - 1) {
                    buf[k++] = ch;
                    getch();
                }
                // 数字后接字母 -> 错误
                if (isalpha(ch)) {
                    while (isalnum(ch) && k < MAX_NUM_LEN - 1) {
                        buf[k++] = ch;
                        getch();
                    }
                    buf[k] = '\0';
                    print_token(SYM_ERROR, buf);
                    return SYM_ERROR;
                }
                buf[k] = '\0';
                print_token(SYM_NUMBER, buf);
                return SYM_NUMBER;

            //=========================
            //     :=
            //=========================
            case STATE_INASSIGN:
                if (ch == '=') {
                    getch();
                    print_token(SYM_ASSIGN, ":=");
                    return SYM_ASSIGN;
                }
                buf[k] = '\0';
                print_token(SYM_ERROR, buf);
                return SYM_ERROR;
            
            //=========================
            //     注释 { ... }
            //=========================
            case STATE_INCOMMENT:
                while (ch != '}' && ch != EOF && ch != '\n') {
                    getch();
                }
                if (ch != '}') {
                    print_token(SYM_ERROR, "=== Unclosed comment ===");
                    return SYM_ERROR;
                }
                getch(); // 跳过 }
                state = STATE_START;
                break;

            //=========================
            //     字符串
            //=========================
            case STATE_INSTRING:
                while (ch != '"' && ch != EOF && ch != '\n' && k < MAX_STR_LEN - 1) {
                    buf[k++] = ch;
                    getch();
                }
                if (ch != '"') {    // 未闭合
                    print_token(SYM_ERROR, "=== Unclosed string ===");
                    return SYM_ERROR;
                }
                getch(); // 跳过 "
                buf[k] = '\0';
                print_token(SYM_STRING, buf);
                return SYM_STRING;

            default:
                state = STATE_ERROR;
                break;
        } // end switch
    } // end while

    print_token(SYM_ERROR, "unknown");
    return SYM_ERROR;
}


int main(int argc, char *argv[]) {
    fin = fopen(argv[1], "r");
    if (!fin) {
        printf("Cannot open file: %s\n", argv[1]);
        return 1;
    }

    getch();

    while (1) {
        int sym = getsym();
        if (sym == SYM_NULL) break;
    }

    fclose(fin);
    return 0;
}
