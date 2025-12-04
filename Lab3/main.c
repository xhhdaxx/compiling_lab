#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// --- 1. 定义符号与数据结构 ---
// 终结符: i, +, *, (, ), #
typedef enum
{
    SYM_i = 0,      // i
    SYM_PLUS = 1,   // +
    SYM_STAR = 2,   // *
    SYM_LPAREN = 3, // (
    SYM_RPAREN = 4, // )
    SYM_EOF = 5,    // #
    SYM_UNKNOWN = 6 // ?
} Terminal;

// 非终结符: E, E', T, T', F
typedef enum
{
    VN_E = 0,  // E
    VN_Ep = 1, // E'
    VN_T = 2,  // T
    VN_Tp = 3, // T'
    VN_F = 4   // F
} NonTerminal;

// 符号类型
typedef struct
{
    int is_terminal; // 1: 终结符, 0: 非终结符
    int index;       // 对应的枚举值
} Symbol;

// 产生式结构
typedef struct
{
    int len;         // 右部长度
    Symbol right[5]; // 右部符号数组
    char str[30];    // 字符串表示
} Production;

// Token 结构
typedef struct
{
    Terminal type;     // 映射后的文法符号
    int original_code; // 原始种别码 (如 1, 5, 17)
    char value[50];    // 属性值 (如 "x", "*")
} Token;

// 全局变量
#define VT_NUM 6                                         // 终结符数量
#define VN_NUM 5                                         // 非终结符数量
int LL1Table[VN_NUM][VT_NUM];                            // 预测分析表
Production rules[10];                                    // 产生式集合
const char *vt_names[] = {"i", "+", "*", "(", ")", "#"}; // 终结符集合
const char *vn_names[] = {"E", "E'", "T", "T'", "F"};    // 非终结符集合

// --- 2. 初始化文法与分析表 ---
// 终结符
Symbol S_VT(int id)
{
    Symbol s = {1, id};
    return s;
}
// 非终结符
Symbol S_VN(int id)
{
    Symbol s = {0, id};
    return s;
}

// 初始化文法
void init_grammar()
{
    // 0: E -> T E'
    rules[0].len = 2;
    rules[0].right[0] = S_VN(VN_T);
    rules[0].right[1] = S_VN(VN_Ep);
    strcpy(rules[0].str, "E -> T E'");

    // 1: E' -> + T E'
    rules[1].len = 3;
    rules[1].right[0] = S_VT(SYM_PLUS);
    rules[1].right[1] = S_VN(VN_T);
    rules[1].right[2] = S_VN(VN_Ep);
    strcpy(rules[1].str, "E' -> + T E'");

    // 2: E' -> ε
    rules[2].len = 0;
    strcpy(rules[2].str, "E' -> ε");

    // 3: T -> F T'
    rules[3].len = 2;
    rules[3].right[0] = S_VN(VN_F);
    rules[3].right[1] = S_VN(VN_Tp);
    strcpy(rules[3].str, "T -> F T'");

    // 4: T' -> * F T'
    rules[4].len = 3;
    rules[4].right[0] = S_VT(SYM_STAR);
    rules[4].right[1] = S_VN(VN_F);
    rules[4].right[2] = S_VN(VN_Tp);
    strcpy(rules[4].str, "T' -> * F T'");

    // 5: T' -> ε
    rules[5].len = 0;
    strcpy(rules[5].str, "T' -> ε");

    // 6: F -> ( E )
    rules[6].len = 3;
    rules[6].right[0] = S_VT(SYM_LPAREN);
    rules[6].right[1] = S_VN(VN_E);
    rules[6].right[2] = S_VT(SYM_RPAREN);
    strcpy(rules[6].str, "F -> ( E )");

    // 7: F -> i
    rules[7].len = 1;
    rules[7].right[0] = S_VT(SYM_i);
    strcpy(rules[7].str, "F -> i");

    // 初始化分析表
    // 行: 非终结符, 列: 终结符
    for (int i = 0; i < VN_NUM; i++)
        for (int j = 0; j < VT_NUM; j++)
            LL1Table[i][j] = -1;

    // 填表
    LL1Table[VN_E][SYM_i] = 0;
    LL1Table[VN_E][SYM_LPAREN] = 0;
    LL1Table[VN_Ep][SYM_PLUS] = 1;
    LL1Table[VN_Ep][SYM_RPAREN] = 2;
    LL1Table[VN_Ep][SYM_EOF] = 2;
    LL1Table[VN_T][SYM_i] = 3;
    LL1Table[VN_T][SYM_LPAREN] = 3;
    LL1Table[VN_Tp][SYM_STAR] = 4;
    LL1Table[VN_Tp][SYM_PLUS] = 5;
    LL1Table[VN_Tp][SYM_RPAREN] = 5;
    LL1Table[VN_Tp][SYM_EOF] = 5;
    LL1Table[VN_F][SYM_i] = 7;
    LL1Table[VN_F][SYM_LPAREN] = 6;
}

// --- 3. 新版序列解析器 ---
// 根据字符串值判断文法符号类型
Terminal identify_terminal(const char *value_str, int code)
{
    if (strcmp(value_str, "+") == 0)
        return SYM_PLUS;
    if (strcmp(value_str, "*") == 0)
        return SYM_STAR;
    if (strcmp(value_str, "(") == 0)
        return SYM_LPAREN;
    if (strcmp(value_str, ")") == 0)
        return SYM_RPAREN;
    if (strcmp(value_str, "#") == 0)
        return SYM_EOF;

    // 将分号 ; 视为结束符 #
    if (strcmp(value_str, ";") == 0)
        return SYM_EOF;

    // 其他情况视为标识符 i
    return SYM_i;
}

// 读取输入序列
int read_sequence(Token *tokens)
{
    int count = 0;  // 存储token数量
    char line[256]; // 存储输入行

    while (fgets(line, sizeof(line), stdin))
    {
        // 预处理
        char *ptr = line; // 清理行指针
        while (isspace(*ptr))
            ptr++; // 跳过空白字符
        size_t len = strlen(ptr);
        while (len > 0 && isspace(ptr[len - 1]))
            ptr[--len] = '\0'; // 清除空白字符
        if (!*ptr)
            continue; // 跳过空行

        // 检查END
        if (strcmp(ptr, "END") == 0)
            break;

        // 直接解析格式(code, "value")
        int code;       // 存储种别码
        char value[50]; // 存储属性值
        if (sscanf(ptr, "(%d, \"%49[^\"]\")", &code, value) != 2)
        {
            fprintf(stderr, "格式错误: %s\n", ptr);
            return -1;
        }

        // 存储token
        tokens[count].type = identify_terminal(value, code);              // 识别终结符-
        tokens[count].original_code = code;                               // 存储种别码
        strncpy(tokens[count].value, value, sizeof(tokens[0].value) - 1); // 存储属性值
        tokens[count].value[sizeof(tokens[0].value) - 1] = '\0';          // 确保字符串结束
        count++;                                                          // 增加token数量
    }

    // 自动添加结束标记
    tokens[count].type = SYM_EOF;     // 设置终结符
    tokens[count].original_code = -1; // 设置种别码
    strcpy(tokens[count].value, "#"); // 设置属性值
    return count + 1;                 // 返回token数量
}

// --- 4. LL(1) 驱动程序 ---
Symbol stack[100]; // 分析栈
int top = -1;      // 栈顶指针

// 压栈
void push(Symbol s) { stack[++top] = s; }
// 出栈
Symbol pop() { return stack[top--]; }
// 查看栈顶
Symbol peek() { return stack[top]; }
// 获取栈内容
void get_stack_content(char *buffer)
{
    buffer[0] = '\0';
    for (int i = 0; i <= top; i++)
    {
        if (stack[i].is_terminal)
            strcat(buffer, vt_names[stack[i].index]);
        else
            strcat(buffer, vn_names[stack[i].index]);
    }
}

// LL(1) 分析
void parse_LL1()
{
    Token tokens[100];                        // 存储输入的token
    int total_tokens = read_sequence(tokens); // 读取输入的token

    // 读取失败返回
    if (total_tokens == -1)
        return;

    // 初始化
    top = -1;
    push(S_VT(SYM_EOF)); // 将#压栈
    push(S_VN(VN_E));    // 将E压栈

    int ip = 0;          // 输入指针
    int step = 1;        // 步骤
    char stack_str[100]; // 分析栈字符串
    int success = 0;     // 是否成功

    printf("\n------------------------------------------------------------------------\n");
    printf("%-7s | %-15s | %-14s | %-13s | %-20s\n", "步骤", "分析栈", "当前种别", "当前值", "动作");
    printf("------------------------------------------------------------------------\n");

    while (1)
    {
        Symbol X = peek();    // 取栈顶符号
        Token a = tokens[ip]; // 当前输入的token

        get_stack_content(stack_str); // 获取分析栈内容

        // 格式化输出：显示原始种别码和值
        printf("%-5d | %-12s | %-10d | %-10s | ", step++, stack_str, a.original_code, a.value);

        if (X.is_terminal)
        {
            if (X.index == a.type)
            {
                if (X.index == SYM_EOF)
                {
                    printf("\033[32m分析成功 (Accept)\033[0m\n");
                    success = 1;
                    break;
                }
                else
                {
                    printf("匹配 '%s'\n", a.value);
                    pop();
                    ip++;
                }
            }
            else
            {
                printf("\033[31m错误: 栈顶 '%s' 不匹配输入\033[0m\n", vt_names[X.index]);
                break;
            }
        }
        else
        {
            int rule = LL1Table[X.index][a.type]; // 获取产生式
            if (rule != -1)
            {
                printf("%s\n", rules[rule].str); // 输出产生式
                pop();                           // 出栈
                for (int k = rules[rule].len - 1; k >= 0; k--)
                {
                    push(rules[rule].right[k]); // 压栈
                }
            }
            else
            {
                printf("\033[31m错误: 无产生式 M[%s, %s]\033[0m\n", vn_names[X.index], a.value);
                break;
            }
        }
    }
    // 最终输出结果
    printf("------------------------------------------------------------------------\n");
    if (success)
        printf("\033[32m结论: 输入串是该文法定义的算术表达式\033[0m\n\n");
    else
        printf("\033[31m结论: 输入串不是该文法定义的算术表达式\033[0m\n\n");
}

// --- 5. 主函数 ---
int main()
{
    init_grammar();

    printf("=============== LL(1) 分析器 ===============\n");
    printf("请输入多行二元序列，输入 END 结束：\n");

    parse_LL1();

    return 0;
}