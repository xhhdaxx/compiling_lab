#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// --- 1. 测试文本信息 ---
/*
    test_corr.txt   // 正确文法信息
    test_err1.txt   // 错误文法信息 -- 缺少运算量1
    test_err2.txt   // 错误文法信息 -- 缺少封闭括号
    test_err3.txt   // 错误文法信息 -- 缺少运算量2
    test_err4.txt   // 错误文法信息 -- 缺少运算符
    test_err5.txt   // 错误文法信息 -- 多个错误检查
*/

// --- 2. 定义变量 ---
#define MAX_BUF 2048        // 最大输入长度

// 全局变量
char buffer[MAX_BUF];       // 存储当前行的输入
int pos = 0;                // 当前字符位置
int current_code = 0;       // 当前Token的数值编号
char sym;                   // 映射到简易文法的字符 (+, *, (, ), i, ;, #)
char lexeme[100];           // Token的文本值
int error_pos = -1;         // 错误位置
char error_sym = 0;         // 错误符号


// --- 3. 定义 Token 类型 ---
enum {
    SYM_NULL = 0,
    SYM_IDENTIFIER = 1,
    SYM_NUMBER = 2,
    SYM_PLUS = 3,
    SYM_TIMES = 5,
    SYM_LPAREN = 13,
    SYM_RPAREN = 14,
    SYM_SEMICOLON = 17,
    SYM_ERROR = 100
};

// --- 4. 函数声明 ---
void advance();
int E();
int E_prime();
int T();
int T_prime();
int F();
void error(const char *msg);

// 词法分析器
void advance() {
    int token_start = pos;   // 记录本 token 开始位置
    current_code = 0;
    lexeme[0] = '\0';

    while (buffer[pos] != '\0' &&
           (isspace(buffer[pos]) || buffer[pos] == ',')) {
        pos++;
    }

    if (buffer[pos] == '\0') {
        sym = '#';
        current_code = SYM_NULL;
        strcpy(lexeme, "EOF");
        return;
    }

    if (buffer[pos] != '(') {
        sym = '?';
        lexeme[0] = buffer[pos];
        lexeme[1] = '\0';
        pos++;
        return;
    }

    pos++;  // 吃掉 '('

    // --- A. 读取整数 code ---
    while (isspace(buffer[pos])) pos++;

    char num_buf[32];
    int k = 0;

    if (!isdigit(buffer[pos])) {
        sym = '?';
        return;
    }

    while (isdigit(buffer[pos])) {
        num_buf[k++] = buffer[pos++];
    }
    num_buf[k] = '\0';
    current_code = atoi(num_buf);

    // --- B. 寻找左引号 ---
    while (buffer[pos] != '\0' && buffer[pos] != '"') pos++;
    if (buffer[pos] == '"') pos++;

    // --- C. 读取 value ---
    k = 0;
    while (buffer[pos] != '\0' && buffer[pos] != '"') {
        if (k < 99) lexeme[k++] = buffer[pos];
        pos++;
    }
    lexeme[k] = '\0';

    if (buffer[pos] == '"') pos++;

    // --- D. 跳到 ')' ---
    while (buffer[pos] != '\0' && buffer[pos] != ')') pos++;
    if (buffer[pos] == ')') pos++;

    // Token -> 文法字符映射
    switch (current_code) {
        case SYM_IDENTIFIER:
        case SYM_NUMBER:
            sym = 'i';
            break;
        case SYM_PLUS:
            sym = '+';
            break;
        case SYM_TIMES:
            sym = '*';
            break;
        case SYM_LPAREN:
            sym = '(';
            break;
        case SYM_RPAREN:
            sym = ')';
            break;
        case SYM_SEMICOLON:
            sym = ';';
            break;
        default:
            sym = '?';
            break;
    }

    error_pos = token_start;
    error_sym = sym;

    printf("   [Token] Code=%-2d Val=\"%-4s\" -> 识别为: %c\n",
           current_code, lexeme, sym);
}

// 语法规则实现
// E -> TE'
int E() {
    if (T()) {
        if (E_prime()) return 1;
    }
    return 0;
}

// E' -> +TE' | ε
int E_prime() {
    if (sym == '+') {
        advance();
        if (T()) return E_prime();
        else {
            error("【E' -> +TE' | ε】 ==> 预期 T (因子错误)");
            return 0;
        }
    }
    else if (sym == ')' || sym == ';' || sym == '#') {
        return 1;
    }
    return 1;
}

// T -> FT'
int T() {
    if (F()) {
        if (T_prime()) return 1;
    }
    return 0;
}

// T' -> *FT' | ε
int T_prime() {
    if (sym == '*') {
        advance();
        if (F()) return T_prime();
        else {
            error("【T' -> *FT' | ε】 ==> 预期 F (因子错误)");
            return 0;
        }
    }
    else if (sym == '+' || sym == ')' || sym == ';' || sym == '#') {
        return 1;
    }
    return 1;
}

// F -> (E) | i
int F() {
    if (sym == '(') {
        advance();
        if (E()) {
            if (sym == ')') {
                advance();
                return 1;
            } else {
                error("【F -> (E) | i】 ==> 缺少闭括号 ')'");
                return 0;
            }
        }
        return 0;
    }
    else if (sym == 'i') {
        advance();
        return 1;
    }
    else {
        return 0;
    }
}

// 错误处理
char error_msg[256] = {0};
int error_detected = 0;

void error(const char *msg) {
    error_detected = 1;
    snprintf(error_msg, sizeof(error_msg),
             "   \033[31m[Error]\033[0m 错误内容'%s': %s\n",
             lexeme, msg);
}

// 错误分类
const char* classify_error() {
    // 1. 缺少封闭括号 -> 最高优先级
    int left = 0, right = 0;
    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == '(') left++;
        if (buffer[i] == ')') right++;
    }
    if (left > right)
        return "缺少封闭括号";

    // 2. 若出错符号为 ';' 或 ')', 多为缺少运算量
    if (error_sym == ';' || error_sym == ')' || error_sym == '+' || error_sym == '*')
        return "缺少运算量";

    // 3. 若出错符号为 'i' 或 '('，但规则失败，多为缺少运算符
    if (error_sym == 'i' || error_sym == '(')
        return "缺少运算符";

    return "其他语法错误";
}

// 单行分析入口
void analyze_line(int line_num) {
    pos = 0;
    
    // 先检查是否为空行，如果是则直接返回不输出
    int temp_pos = 0;
    while (buffer[temp_pos] != '\0' && 
           (isspace(buffer[temp_pos]) || buffer[temp_pos] == ',')) {
        temp_pos++;
    }
    if (buffer[temp_pos] == '\0') {
        return;  // 空行，直接返回不输出
    }
    
    printf("\n====================================================\n");
    printf("Line %d 分析: %s\n", line_num, buffer);

    advance();

    if (sym == '#') {
        return;  // 空行，直接返回不输出
    }

    int success = E();

    if (success && (sym == '#' || sym == ';')) {
        printf("----------------------------------------------------\n");
        if (sym == '#') {
            printf("结果: \033[32m正确 (Accept) - 无分号结尾\033[0m");
        }
        if (sym == ';') {
            printf("结果: \033[32m正确 (Accept) - 分号结尾\033[0m");
        }
    }
    else {
        printf("%s", error_msg);

        printf("------------------- 具体情况分析 -------------------\n");
        printf("\033[31m错误位置: \033[0m\n");

        printf("%s\n", buffer);

        // 画出 ^~~~~
        for (int i = 0; i < error_pos; i++)
            printf(" ");
        printf("^~~~~~~\n");

        printf("\033[31m错误原因: \033[0m%s\n", classify_error());
        printf("----------------------------------------------------\n");
        printf("结果: \033[31m错误 (Error)\033[0m");
    }


    printf("\n");
}

// 自动拆分并逐句分析
void split_and_analyze(const char* bigbuf) {
    int line_num = 1;
    const char* p = bigbuf;

    // 跳过开头的空白字符
    while (*p && (isspace(*p) || *p == ',')) {
        p++;
    }

    while (*p) {
        const char* end = strstr(p, "(17,\";\")");

        if (end == NULL) {
            // 最后一段（可能没有分号）
            // 检查是否为空或只包含空白字符
            int has_content = 0;
            for (const char* check = p; *check; check++) {
                if (!isspace(*check) && *check != ',') {
                    has_content = 1;
                    break;
                }
            }
            if (has_content) {
                strcpy(buffer, p);
                analyze_line(line_num++);
            }
            break;
        }

        // 拆出一句（包含分号 token）
        int len = end - p + strlen("(17,\";\")");
        strncpy(buffer, p, len);
        buffer[len] = '\0';

        analyze_line(line_num++);

        p = end + strlen("(17,\";\")");  // 前进到下一句开始
        
        // 跳过空白字符，避免处理空内容
        while (*p && (isspace(*p) || *p == ',')) {
            p++;
        }
    }
}

// --- 5. 主程序 ---
int main() {
    int choice;
    char filename[100];
    FILE *fp;

    printf("=== 递归下降语法分析程序 (多句独立分析) ===\n");
    printf("1. 终端输入\n");
    printf("2. 文件读取\n");
    printf("选择: ");
    scanf("%d", &choice);
    getchar();

    char bigbuf[MAX_BUF];
    bigbuf[0] = '\0';

    if (choice == 1) {
        printf("请输入多行二元序列，输入 END 结束：\n");

        char line[256];
        while (1) {
            if (fgets(line, sizeof(line), stdin) == NULL) break;
            line[strcspn(line, "\n")] = 0;

            if (strcmp(line, "END") == 0) break;

            strcat(bigbuf, line);
            strcat(bigbuf, " ");
        }
    }
    else if (choice == 2) {
        printf("文件名: ");
        scanf("%s", filename);

        fp = fopen(filename, "r");
        if (!fp) {
            printf("无法打开文件。\n");
            return 1;
        }

        char line[256];
        while (fgets(line, sizeof(line), fp) != NULL) {
            line[strcspn(line, "\n")] = 0;
            strcat(bigbuf, line);
            strcat(bigbuf, " ");
        }
        fclose(fp);
    }

    // ---- 调用封装好的函数 ----
    split_and_analyze(bigbuf);
    printf("\n");

    return 0;
}
