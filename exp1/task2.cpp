#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
using namespace std;
// 栈类
template<typename T>
class Stack {
vector<T> elems;
public:
void push(const T& val) { elems.push_back(val); }
void pop() { if (!empty()) elems.pop_back(); }
T& top() { return elems.back(); }
bool empty() const { return elems.empty(); }
size_t size() const { return elems.size(); }
};
// 运算符优先级
int getPrior(char op) {
if (op == '+' || op == '-') return 1;
if (op == '*' || op == '/') return 2;
if (op == '^') return 3;
if (op == 's' || op == 'c' || op == 't' || op == 'l') return 4; // 函
数
return 0;
}
// 判断是否左结合
bool isLeft(char op) {
return op != '^'; // 乘方是右结合
}
// 函数映射
map<string, char> funcMap = {
{"sin", 's'}, {"cos", 'c'}, {"tan", 't'}, {"log", 'l'}
};
map<char, string> symToFunc = {
{'s', "sin"}, {'c', "cos"}, {'t', "tan"}, {'l', "log"}
};
// 应用数学函数
double applyFunc(double val, char type) {
if (type == 's') return sin(val * M_PI / 180.0); // 角度转弧度
if (type == 'c') return cos(val * M_PI / 180.0);
if (type == 't') return tan(val * M_PI / 180.0);
if (type == 'l') {
if (val <= 0) throw runtime_error("对数参数必须为正");
return log10(val);
}
throw runtime_error("未知函数");
}
// 计算表达式并生成逆波兰式
pair<string, double> calcExpr(const string& expr) {
Stack<double> numStk;
Stack<char> opStk;
string rpn;
string numStr;
// 处理数字
auto procNum = [&]() {
if (!numStr.empty()) {
double num = stod(numStr);
numStk.push(num);
// 数字格式化为 2 位小数
ostringstream oss;
oss << fixed << setprecision(2) << num;
rpn += oss.str();
numStr.clear();
}
};
// 应用运算符
auto procOp = [&]() {
if (opStk.empty()) return;
char op = opStk.top();
opStk.pop();
// 处理函数（一元运算符）
if (op == 's' || op == 'c' || op == 't' || op == 'l') {
if (numStk.empty()) throw runtime_error("函数缺少参数");
double arg = numStk.top();
numStk.pop();
double res = applyFunc(arg, op);
numStk.push(res);
rpn += symToFunc[op];
}
// 处理普通运算符
else {
if (numStk.size() < 2) throw runtime_error("操作数不足");
double right = numStk.top(); numStk.pop();
double left = numStk.top(); numStk.pop();
double res;
switch (op) {
case '+': res = left + right; break;
case '-': res = left - right; break;
case '*': res = left * right; break;
case '/':
if (right == 0) throw runtime_error("除数不能为零");
res = left / right;
break;
case '^': res = pow(left, right); break;
default: throw runtime_error("未知运算符");
}
numStk.push(res);
rpn += op;
}
};
// 遍历表达式
for (size_t i = 0; i < expr.length(); ++i) {
char c = expr[i];
if (isspace(c)) continue;
// 处理数字和小数点
if (isdigit(c) || c == '.') {
numStr += c;
continue;
}
// 处理函数名
if (isalpha(c)) {
procNum(); // 先处理可能存在的数字
string funcName;
while (i < expr.length() && isalpha(expr[i])) {
funcName += tolower(expr[i]);
i++;
}
if (funcMap.count(funcName)) {
// 检查后面是否有左括号
while (i < expr.length() && isspace(expr[i])) i++;
if (i >= expr.length() || expr[i] != '(') {
throw runtime_error("函数缺少左括号");
}
// 跳过左括号
i++;
// 处理函数参数
string param;
int pCount = 1; // 已处理一个左括号
while (i < expr.length() && pCount > 0) {
if (expr[i] == '(') pCount++;
else if (expr[i] == ')') pCount--;
if (pCount > 0) {
param += expr[i];
}
i++;
}
if (pCount != 0) {
throw runtime_error("括号不匹配");
}
// 递归计算参数
auto [pRpn, pVal] = calcExpr(param);
numStk.push(pVal);
rpn += pRpn;
// 应用函数
opStk.push(funcMap[funcName]);
procOp();
// 回退索引
i--;
} else {
throw runtime_error("未知函数: " + funcName);
}
continue;
}
// 处理已读取的数字
procNum();
// 处理左括号
if (c == '(') {
opStk.push(c);
}
// 处理右括号
else if (c == ')') {
while (!opStk.empty() && opStk.top() != '(') {
procOp();
}
if (opStk.empty() || opStk.top() != '(') {
throw runtime_error("括号不匹配");
}
opStk.pop(); // 弹出'('
}
// 处理运算符
else if (getPrior(c) > 0) {
while (!opStk.empty() && opStk.top() != '(' &&
((getPrior(opStk.top()) > getPrior(c)) ||
(getPrior(opStk.top()) == getPrior(c) && isLeft(c)))) {
procOp();
}
opStk.push(c);
}
else {
throw runtime_error("无效字符: " + string(1, c));
}
}
// 处理最后一个数字
procNum();
// 处理剩余的运算符
while (!opStk.empty()) {
if (opStk.top() == '(') {
throw runtime_error("括号不匹配");
}
procOp();
}
if (numStk.size() != 1) {
throw runtime_error("表达式无效");
}
return {rpn, numStk.top()};
}
int main() {
cout << fixed << setprecision(2);
vector<string> tests = {
"3 + 4 * 5",
"2 * sin(45) + 3",
"sin(30) * 4 + log(10)",
"2 ^ sin(30)",
"cos(60) + tan(45)",
"log(100) * 2"
};
for (const auto& expr : tests) {
cout << "中缀表达式: " << expr << endl;
try {
auto [rpn, res] = calcExpr(expr);
cout << "逆波兰式: " << rpn << endl;
cout << "计算结果: " << res << endl;
} catch (const exception& e) {
cout << "计算结果: 表达式无效 (" << e.what() << ")" << endl;
}
}
// 交互式计算器
cout << "交互式计算器 (输入 'quit' 退出):" << endl;
string input;
while (true) {
cout << "> ";
getline(cin, input);
if (input == "quit") break;
if (input.empty()) continue;
try {
auto [rpn, res] = calcExpr(input);
cout << "= " << res << " (逆波兰式: " << rpn << ")" << endl;
} catch (const exception& e) {
cout << "错误: " << e.what() << endl;
}
}