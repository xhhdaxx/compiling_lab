%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// token types
enum {
    SYM_NULL = 0,
    SYM_IDENTIFIER = 1,
    SYM_NUMBER = 2,
    SYM_PLUS = 3,
    SYM_MINUS = 4,
    SYM_TIMES = 5,
    SYM_SLASH = 6,
    SYM_EQU = 7,
    SYM_GTR = 8,
    SYM_LES = 9,
    SYM_NEQ = 10,
    SYM_LEQ = 11,
    SYM_GEQ = 12,
    SYM_LPAREN = 13,
    SYM_RPAREN = 14,
    SYM_LBRACE = 15,
    SYM_RBRACE = 16,
    SYM_SEMICOLON = 17,
    SYM_COMMA = 18,
    SYM_ASSIGN = 20,
    SYM_VAR = 21,
    SYM_IF = 22,
    SYM_THEN = 23,
    SYM_ELSE = 24,
    SYM_WHILE = 25,
    SYM_FOR = 26,
    SYM_BEGIN = 27,
    SYM_WRITELN = 28,
    SYM_PROCEDURE = 29,
    SYM_END = 30,
    SYM_CONST = 31,
    SYM_CALL = 32,
    SYM_DO = 33,
    SYM_WRITE = 34,
    SYM_PERIOD = 35,
    SYM_STRING = 36,
    SYM_ERROR = 100
};

// 字符串缓冲区
char string_buf[1000];
int string_len = 0;

%}

DIGIT    [0-9]
LETTER   [a-zA-Z]
ID       {LETTER}({LETTER}|{DIGIT})*
INVALID_NUMBER {DIGIT}+{LETTER}({LETTER}|{DIGIT})*

%%

"var"           { printf("(21, \"%s\")\n", yytext); }
"if"            { printf("(22, \"%s\")\n", yytext); }
"then"          { printf("(23, \"%s\")\n", yytext); }
"else"          { printf("(24, \"%s\")\n", yytext); }
"while"         { printf("(25, \"%s\")\n", yytext); }
"for"           { printf("(26, \"%s\")\n", yytext); }
"begin"         { printf("(27, \"%s\")\n", yytext); }
"writeln"       { printf("(28, \"%s\")\n", yytext); }
"procedure"     { printf("(29, \"%s\")\n", yytext); }
"end"           { printf("(30, \"%s\")\n", yytext); }
"const"         { printf("(31, \"%s\")\n", yytext); }
"call"          { printf("(32, \"%s\")\n", yytext); }
"do"            { printf("(33, \"%s\")\n", yytext); }
"write"         { printf("(34, \"%s\")\n", yytext); }

{ID}            { printf("(1, \"%s\")\n", yytext); }
{INVALID_NUMBER} { printf("(100, \"%s\")\n", yytext); }  /* 数字后接字母的错误 */
{DIGIT}+        { printf("(2, %s)\n", yytext); }

":="            { printf("(20, \":=\")\n"); }
"<>"            { printf("(10, \"<>\")\n"); }
"<="            { printf("(11, \"<=\")\n"); }
">="            { printf("(12, \">=\")\n"); }

"+"             { printf("(3, \"+\")\n"); }
"-"             { printf("(4, \"-\")\n"); }
"*"             { printf("(5, \"*\")\n"); }
"/"             { printf("(6, \"/\")\n"); }
"="             { printf("(7, \"=\")\n"); }
">"             { printf("(8, \">\")\n"); }
"<"             { printf("(9, \"<\")\n"); }
"("             { printf("(13, \"(\")\n"); }
")"             { printf("(14, \")\")\n"); }
"{"             { 
                  // 注释处理
                  int c;
                  int comment_closed = 0;
                  while ((c = input()) != EOF && c != '\n') {
                      if (c == '}') {
                          comment_closed = 1;
                          break;
                      }
                  }
                  if (!comment_closed) {
                      printf("(100, \"=== Unclosed comment ===\")\n");
                  }
                }
"}"             { printf("(100, \"%s\")\n", yytext); }  // 单独的 } 应该报错
";"             { printf("(17, \";\")\n"); }
","             { printf("(18, \",\")\n"); }
"."             { printf("(35, \".\")\n"); }

\"              {
                  // 字符串处理
                  string_len = 0;
                  int c;
                  int string_closed = 0;
                  while ((c = input()) != EOF && c != '\n') {
                      if (c == '"') {
                          string_closed = 1;
                          break;
                      }
                      if (string_len < sizeof(string_buf) - 1) {
                          string_buf[string_len++] = c;
                      }
                  }
                  if (!string_closed) {
                      printf("(100, \"=== Unclosed string ===\")\n");
                  } else {
                      string_buf[string_len] = '\0';
                      printf("(36, \"%s\")\n", string_buf);
                  }
                }

[ \t\r\n]+      { /* 跳过分隔符 */ }

.               { printf("(100, \"%s\")\n", yytext); }

%%

int yywrap(void) {
    return 1;
}

int main(void) {
    yylex();
    return 0;
}