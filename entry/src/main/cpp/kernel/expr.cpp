// expr.cpp — 双精度迷你表达式求值器：tokenizer + 递归下降解析
#include "kernel/expr.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace dreamember {

struct Expr::Node {
    enum class Kind { Num, Var, YVar, Param, Ref, Bin, Call };
    Kind kind = Kind::Num;
    double num = 0.0;
    char op = 0;
    std::string name;   // Var/YVar/Param 无用；Ref=对象标签；Call=函数名
    std::unique_ptr<Node> l, r;
    std::vector<std::unique_ptr<Node>> args;   // Call 的实参表（1~2 参）
};

namespace {

// ---------- 键盘线性输入预解析（expr.h 声明） ----------

constexpr double kPi = 3.14159265358979323846;

// 引号串内（Text 的文本内容）不动
std::string StripDollar(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    bool inStr = false;
    for (char c : s) {
        if (c == '"') inStr = !inStr;
        if (!inStr && c == '$') continue;   // $A$1 → A1（绝对引用标记）
        out.push_back(c);
    }
    return out;
}

bool DigitsULL(const std::string &s, size_t from, size_t to, unsigned long long &out)
{
    if (from >= to || to - from > 15) return false;
    unsigned long long v = 0;
    for (size_t i = from; i < to; ++i) {
        if (!isdigit((unsigned char)s[i])) return false;
        v = v * 10 + (unsigned long long)(s[i] - '0');
    }
    out = v;
    return true;
}

// 循环小数：数字.数字(数字) → 精确分数表达式。
// 0.(3) → (3/9)；1.2(34) → (1+232/990)；10.2(34) → (10+234/990)。
// 函数调用/隐式乘法的括号（f(3)、2(3)）因点分隔要求不会误伤。
std::string ExpandRepeating(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    bool inStr = false;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '"') { inStr = !inStr; out.push_back(c); ++i; continue; }
        if (inStr || c != '(') { out.push_back(c); ++i; continue; }
        // '(' 前须为「数字.数字」或「.」紧随数字的小数点结构
        size_t dot = std::string::npos;
        size_t nonrepFrom = i, nonrepTo = i;
        if (i > 0 && s[i - 1] == '.') {
            dot = i - 1;                        // 0.() 空非循环位
            nonrepFrom = nonrepTo = i;
        } else if (i > 0 && isdigit((unsigned char)s[i - 1])) {
            size_t k = i;
            while (k > 0 && isdigit((unsigned char)s[k - 1])) --k;
            if (k > 0 && s[k - 1] == '.') {
                dot = k - 1;
                nonrepFrom = k;
                nonrepTo = i;
            }
        }
        size_t repEnd = (dot != std::string::npos) ? s.find(')', i + 1)
                                                   : std::string::npos;
        unsigned long long rep = 0, nonrep = 0, intPart = 0;
        // 非循环位可为空（0.(3) 形态）
        const bool nonrepOk = nonrepTo == nonrepFrom
            || DigitsULL(s, nonrepFrom, nonrepTo, nonrep);
        if (repEnd == std::string::npos
            || !DigitsULL(s, i + 1, repEnd, rep)
            || !nonrepOk) {
            out.push_back(c);
            ++i;
            continue;
        }
        size_t j = dot;
        while (j > 0 && isdigit((unsigned char)s[j - 1])) --j;
        if (j == dot || !DigitsULL(s, j, dot, intPart)) {
            out.push_back(c);
            ++i;
            continue;
        }
        const size_t nDig = nonrepTo - nonrepFrom;
        const size_t rDig = repEnd - i - 1;
        unsigned long long pow10n = 1, pow10r = 1;
        for (size_t d = 0; d < nDig; ++d) pow10n *= 10;
        for (size_t d = 0; d < rDig; ++d) pow10r *= 10;
        const unsigned long long concat = nonrep * pow10r + rep;
        // 循环部分值 = (concat - 非循环部分) / (10^|n|·(10^|r|-1))
        const unsigned long long num = concat - nonrep;
        const unsigned long long den = pow10n * (pow10r - 1);
        if (den == 0 || concat < nonrep) {   // 0 次循环或溢出守卫：不展开
            out.push_back(c);
            ++i;
            continue;
        }
        // 括号策略：默认裸写（"3/9"、"1+232/990"，优先级天然正确，且整串
        // 替换结果不会以 '(' 开头撞点语法）；乘性上下文（前缀 * / ^ 或后缀
        // ^ ! % °）或负整数部分才整体加括号
        const bool negPrefix = j > 0 && s[j - 1] == '-';
        bool needParen = j > 0
            && (s[j - 1] == '*' || s[j - 1] == '/' || s[j - 1] == '^');
        if (repEnd + 1 < s.size()) {
            const char nx = s[repEnd + 1];
            if (nx == '^' || nx == '!' || nx == '%'
                || (unsigned char)nx == 0xC2) needParen = true;
        }
        // 替换串回退：从整数段起点到 '(' 之间已逐字发出的字符
        out.resize(out.size() - (i - j));
        const std::string frac = std::to_string(num) + "/" + std::to_string(den);
        if (intPart == 0) {
            out += needParen ? "(" + frac + ")" : frac;
        } else if (negPrefix) {
            if (!out.empty() && out.back() == '-') out.pop_back();
            out += "-(" + std::to_string(intPart) + "+" + frac + ")";
        } else {
            const std::string body = std::to_string(intPart) + "+" + frac;
            out += needParen ? "(" + body + ")" : body;
        }
        i = repEnd + 1;
    }
    return out;
}

// 带分数：整数 空白+ 分子/分母 → 整数+分子/分母（须在去空白前做）
std::string ExpandMixedNumbers(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    bool inStr = false;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '"') { inStr = !inStr; out.push_back(c); ++i; continue; }
        if (inStr || !isdigit((unsigned char)c)) { out.push_back(c); ++i; continue; }
        size_t q = i;
        while (q < s.size() && isdigit((unsigned char)s[q])) ++q;   // 整数段
        size_t w = q;
        while (w < s.size() && isspace((unsigned char)s[w])) ++w;
        size_t n2 = w;
        while (n2 < s.size() && isdigit((unsigned char)s[n2])) ++n2;   // 分子段
        if (w > q && n2 > w && n2 < s.size() && s[n2] == '/'
            && n2 + 1 < s.size() && isdigit((unsigned char)s[n2 + 1])) {
            size_t d2 = n2 + 1;
            while (d2 < s.size() && isdigit((unsigned char)s[d2])) ++d2;
            // 负带分数：-1 1/2 语义为 -(1+1/2)=-1.5（上游 同款），须整体加括号，
            // 否则改写成 -1+1/2 会变成 -0.5；前导 '-' 已按普通字符发出，回退之。
            // 乘性上下文（前缀 * / ^、后缀 ^ ! % °）同样需要整体括号
            const bool neg = i > 0 && s[i - 1] == '-';
            bool needParen = i > 0
                && (s[i - 1] == '*' || s[i - 1] == '/' || s[i - 1] == '^');
            if (d2 < s.size()) {
                const char nx = s[d2];
                if (nx == '^' || nx == '!' || nx == '%'
                    || (unsigned char)nx == 0xC2) needParen = true;
            }
            if (neg && !out.empty() && out.back() == '-') out.pop_back();
            const std::string body = s.substr(i, q - i) + "+" + s.substr(w, d2 - w);
            if (neg) {
                out += "-(" + body + ")";
            } else {
                out += needParen ? "(" + body + ")" : body;
            }
            i = d2;
            continue;
        }
        out += s.substr(i, q - i);
        i = q;
    }
    return out;
}

} // namespace

std::string PreprocessInput(const std::string &in)
{
    return ExpandMixedNumbers(ExpandRepeating(StripDollar(in)));
}

// 上一答案（上游 ans）：静态存储，供 input 提交流程与 CAS 求值回填
namespace {
double g_ansValue = 0.0;
}

double GetAnsValue()
{
    return g_ansValue;
}

void SetAnsValue(double v)
{
    g_ansValue = v;
}

// 希腊字母（U+0370–U+03FF）UTF-8 双字节判定：首字节 0xCE（U+0380–03BF）
// 或 0xCF（U+03C0–03FF）+ 合法续字节；0xCF 0x80 = π 是常量，排除在外。
bool IsGreekAt(const std::string &s, size_t p)
{
    if (p + 1 >= s.size()) return false;
    const unsigned char b0 = (unsigned char)s[p];
    const unsigned char b1 = (unsigned char)s[p + 1];
    if (b0 == 0xCE) return b1 >= 0x80 && b1 <= 0xBF;
    if (b0 == 0xCF) return b1 >= 0x81 && b1 <= 0xBF;   // 0x80 = π
    return false;
}

std::string ReplaceAns(const std::string &in)
{
    static const char kKey[] = "ans";
    std::string out;
    out.reserve(in.size() + 16);
    size_t i = 0;
    bool inStr = false;
    while (i < in.size()) {
        const char c = in[i];
        if (c == '"') { inStr = !inStr; out.push_back(c); ++i; continue; }
        if (!inStr && in.compare(i, 3, kKey) == 0
            && (i == 0 || !(isalnum((unsigned char)in[i - 1]) || in[i - 1] == '_'
                || (unsigned char)in[i - 1] >= 0x80))
            && (i + 3 >= in.size()
                || !(isalnum((unsigned char)in[i + 3]) || in[i + 3] == '_'
                    || (unsigned char)in[i + 3] >= 0x80))
            // 赋值左侧的 "ans=" 不替换：留给保留名检查报错（上游 同款禁占）
            && !(i + 3 < in.size() && in[i + 3] == '=')) {
            char buf[48];
            snprintf(buf, sizeof(buf), "(%.17g)", GetAnsValue());
            out += buf;
            i += 3;
            continue;
        }
        out.push_back(c);
        ++i;
    }
    return out;
}

namespace {

// 函数名 → 参数个数上限/下限（N6 扩充多参：mod/min/max/gcd/lcm/atan2/log(x,b)）
bool FnArity(const std::string &n, int &lo, int &hi)
{
    if (n == "mod" || n == "gcd" || n == "lcm" || n == "atan2") { lo = 2; hi = 2; return true; }
    if (n == "log") { lo = 1; hi = 2; return true; }
    if (n == "min" || n == "max") { lo = 2; hi = 2; return true; }
    lo = 1;
    hi = 1;
    static const char *kFns[] = { "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh",
                                  "tanh", "exp", "ln", "log10", "sqrt", "abs", "sign",
                                  "floor", "ceil", "round" };
    for (const char *f : kFns) {
        if (n == f) return true;
    }
    return false;
}

long long GcdLL(long long a, long long b)
{
    a = a < 0 ? -a : a;
    b = b < 0 ? -b : b;
    while (b != 0) {
        long long t = a % b;
        a = b;
        b = t;
    }
    return a;
}

double CallFnN(const std::string &n, const double *v, int argc)
{
    if (argc == 1) {
        double x = v[0];
        if (n == "sin") return sin(x);
        if (n == "cos") return cos(x);
        if (n == "tan") return tan(x);
        if (n == "asin") return asin(x);
        if (n == "acos") return acos(x);
        if (n == "atan") return atan(x);
        if (n == "sinh") return sinh(x);
        if (n == "cosh") return cosh(x);
        if (n == "tanh") return tanh(x);
        if (n == "exp") return exp(x);
        if (n == "ln") return log(x);
        if (n == "log") return log10(x);   // 上游：log 为常用对数（ln 自然）
        if (n == "log10") return log10(x);
        if (n == "sqrt") return sqrt(x);
        if (n == "abs") return fabs(x);
        if (n == "sign") return x > 0 ? 1.0 : (x < 0 ? -1.0 : 0.0);
        if (n == "floor") return floor(x);
        if (n == "ceil") return ceil(x);
        if (n == "round") return x >= 0 ? floor(x + 0.5) : ceil(x - 0.5);
        return NAN;
    }
    // 双参（N6）
    double a = v[0], b = v[1];
    if (n == "mod") {
        // 上游/欧几里得语义：结果与除数同号（fmod 与被除数同号需校正）
        if (b == 0) return NAN;
        double m = fmod(a, b);
        if (m != 0 && ((m < 0) != (b < 0))) m += b;
        return m;
    }
    if (n == "min") return a < b ? a : b;
    if (n == "max") return a > b ? a : b;
    if (n == "atan2") return atan2(a, b);
    if (n == "gcd" || n == "lcm") {
        long long ia = llround(a), ib = llround(b);
        if (fabs(a - ia) > 1e-9 || fabs(b - ib) > 1e-9) return NAN;
        if (n == "gcd") return (double)GcdLL(ia, ib);
        if (ia == 0 || ib == 0) return 0;
        long long g = GcdLL(ia, ib);
        return (double)(llabs(ia / g * ib));
    }
    if (n == "log") {
        // 上游 Log(base, value)：底数在前（Log(2, 8) = 3）
        if (a <= 0 || a == 1 || b <= 0) return NAN;
        return log(b) / log(a);
    }
    return NAN;
}

double EvalNode(const Expr::Node *n, double x, double y, double param,
                const std::function<double(const std::string &)> *resolve)
{
    switch (n->kind) {
    case Expr::Node::Kind::Num: return n->num;
    case Expr::Node::Kind::Var: return x;
    case Expr::Node::Kind::YVar: return y;
    case Expr::Node::Kind::Param: return param;
    case Expr::Node::Kind::Ref:
        // 对象标签引用（轮 D 表格公式）：无 resolver 或查不到一律 NaN
        return resolve != nullptr ? (*resolve)(n->name) : NAN;
    case Expr::Node::Kind::Call: {
        double v[2] = { 0, 0 };
        int argc = 0;
        for (auto &arg : n->args) {
            if (argc < 2) v[argc] = EvalNode(arg.get(), x, y, param, resolve);
            ++argc;
        }
        return CallFnN(n->name, v, argc);
    }
    case Expr::Node::Kind::Bin: {
        double a = EvalNode(n->l.get(), x, y, param, resolve);
        // 后缀一元算符（! % °）：r 为空，先于二元求值分支处理
        switch (n->op) {
        case '!': {   // 阶乘：仅非负整数（≤170 防溢出）
            const long long v = llround(a);
            if (std::fabs(a - (double)v) > 1e-9 || v < 0 || v > 170) return NAN;
            double fact = 1.0;
            for (long long i2 = 2; i2 <= v; ++i2) fact *= (double)i2;
            return fact;
        }
        case '%':
            return a / 100.0;
        case (char)0xB0:   // 度 → 弧度
            return a * kPi / 180.0;
        default:
            break;
        }
        double b = EvalNode(n->r.get(), x, y, param, resolve);
        switch (n->op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
        case '^': return pow(a, b);
        }
        break;
    }
    }
    return NAN;
}

struct Tok {
    enum class K { Num, Name, Op, End };
    K k = K::End;
    double num = 0.0;
    std::string name;
    char op = 0;
};

bool Tokenize(const std::string &s, std::vector<Tok> &out, std::string &err)
{
    size_t i = 0;
    while (i < s.size()) {
        char c = s[i];
        if (isdigit((unsigned char)c) || (c == '.' && i + 1 < s.size() && isdigit((unsigned char)s[i + 1]))) {
            size_t j = i;
            while (j < s.size() && (isdigit((unsigned char)s[j]) || s[j] == '.')) ++j;
            // 指数部分：仅当 e/E 后跟数字或符号+数字，否则 e 按常量名处理（如 2e = 2*e）
            if (j < s.size() && (s[j] == 'e' || s[j] == 'E')) {
                size_t k2 = j + 1;
                if (k2 < s.size() && (s[k2] == '+' || s[k2] == '-')) ++k2;
                if (k2 < s.size() && isdigit((unsigned char)s[k2])) {
                    ++k2;
                    while (k2 < s.size() && isdigit((unsigned char)s[k2])) ++k2;
                    j = k2;
                }
            }
            Tok t;
            t.k = Tok::K::Num;
            t.num = strtod(s.substr(i, j - i).c_str(), nullptr);
            out.push_back(std::move(t));
            i = j;
            continue;
        }
        // 标识符：ASCII 字母或希腊字母开头，后接字母/数字/下划线/希腊字母
        //（第八包：α β γ 等作对象名/变量名，上游 希腊键盘同款）
        if (isalpha((unsigned char)c) || IsGreekAt(s, i)) {
            size_t j = isalpha((unsigned char)c) ? i + 1 : i + 2;
            while (j < s.size()) {
                if (isalnum((unsigned char)s[j]) || s[j] == '_') {
                    ++j;
                    continue;
                }
                if (IsGreekAt(s, j)) {
                    j += 2;
                    continue;
                }
                break;
            }
            Tok t;
            t.k = Tok::K::Name;
            t.name = s.substr(i, j - i);
            out.push_back(std::move(t));
            i = j;
            continue;
        }
        // π（UTF-8: 0xCF 0x80）
        if ((unsigned char)c == 0xCF && i + 1 < s.size() && (unsigned char)s[i + 1] == 0x80) {
            Tok t;
            t.k = Tok::K::Name;
            t.name = "pi";
            out.push_back(std::move(t));
            i += 2;
            continue;
        }
        // °（UTF-8: 0xC2 0xB0）→ 度数后缀算符
        if ((unsigned char)c == 0xC2 && i + 1 < s.size() && (unsigned char)s[i + 1] == 0xB0) {
            Tok t;
            t.k = Tok::K::Op;
            t.op = (char)0xB0;
            out.push_back(std::move(t));
            i += 2;
            continue;
        }
        // , ! % 为后缀/分隔算符（逗号用于多参函数 log(b,x) 等；!'% 为后缀）
        if (strchr("+-*/^(),!%", c) != nullptr) {
            Tok t;
            t.k = Tok::K::Op;
            t.op = c;
            out.push_back(std::move(t));
            ++i;
            continue;
        }
        err = "无法识别的字符";
        return false;
    }
    Tok end;
    out.push_back(end);
    return true;
}

class Parser {
public:
    explicit Parser(const std::vector<Tok> &t) : toks_(t) {}
    std::string err;
    bool hasX = false;
    bool hasY = false;
    std::string param;   // 非空时该标识符解析为参数节点（Sequence 循环变量等）
    bool hasParam = false;
    // refs 模式（轮 D）：未知标识符收进 refs（Ref 节点）而非报错
    bool allowRefs = false;
    std::vector<std::string> refs;

    std::unique_ptr<Expr::Node> run()
    {
        if (toks_.empty() || toks_[0].k == Tok::K::End) {
            err = "表达式为空";
            return nullptr;
        }
        auto n = parseExpr();
        if (!n) return nullptr;
        if (toks_[pos_].k != Tok::K::End) {
            err = "表达式末尾有多余内容";
            return nullptr;
        }
        return n;
    }

private:
    const std::vector<Tok> &toks_;
    size_t pos_ = 0;

    const Tok &peek() const { return toks_[pos_]; }
    bool eatOp(char c)
    {
        if (peek().k == Tok::K::Op && peek().op == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    std::unique_ptr<Expr::Node> mkNum(double v)
    {
        auto n = std::make_unique<Expr::Node>();
        n->kind = Expr::Node::Kind::Num;
        n->num = v;
        return n;
    }
    std::unique_ptr<Expr::Node> mkVar()
    {
        auto n = std::make_unique<Expr::Node>();
        n->kind = Expr::Node::Kind::Var;
        return n;
    }
    std::unique_ptr<Expr::Node> mkYVar()
    {
        auto n = std::make_unique<Expr::Node>();
        n->kind = Expr::Node::Kind::YVar;
        return n;
    }
    std::unique_ptr<Expr::Node> mkParam()
    {
        auto n = std::make_unique<Expr::Node>();
        n->kind = Expr::Node::Kind::Param;
        return n;
    }
    std::unique_ptr<Expr::Node> mkRef(const std::string &name)
    {
        auto n = std::make_unique<Expr::Node>();
        n->kind = Expr::Node::Kind::Ref;
        n->name = name;
        return n;
    }
    std::unique_ptr<Expr::Node> mkBin(char op, std::unique_ptr<Expr::Node> l, std::unique_ptr<Expr::Node> r)
    {
        auto n = std::make_unique<Expr::Node>();
        n->kind = Expr::Node::Kind::Bin;
        n->op = op;
        n->l = std::move(l);
        n->r = std::move(r);
        return n;
    }
    std::unique_ptr<Expr::Node> mkCall(const std::string &name, std::vector<std::unique_ptr<Expr::Node>> args)
    {
        auto n = std::make_unique<Expr::Node>();
        n->kind = Expr::Node::Kind::Call;
        n->name = name;
        n->args = std::move(args);
        return n;
    }

    std::unique_ptr<Expr::Node> parseExpr()   // + -
    {
        auto l = parseTerm();
        if (!l) return nullptr;
        while (peek().k == Tok::K::Op && (peek().op == '+' || peek().op == '-')) {
            char op = peek().op;
            ++pos_;
            auto r = parseTerm();
            if (!r) return nullptr;
            l = mkBin(op, std::move(l), std::move(r));
        }
        return l;
    }

    std::unique_ptr<Expr::Node> parseTerm()   // * / 与隐式乘法
    {
        auto l = parseUnary();
        if (!l) return nullptr;
        for (;;) {
            char op = 0;
            if (peek().k == Tok::K::Op && (peek().op == '*' || peek().op == '/')) {
                op = peek().op;
                ++pos_;
            } else if (peek().k == Tok::K::Num || peek().k == Tok::K::Name ||
                       (peek().k == Tok::K::Op && peek().op == '(')) {
                op = '*';   // 隐式乘法：2x、2sin(x)、(1+x)(2-x)
            } else {
                break;
            }
            auto r = parseUnary();
            if (!r) return nullptr;
            l = mkBin(op, std::move(l), std::move(r));
        }
        return l;
    }

    std::unique_ptr<Expr::Node> parseUnary()
    {
        if (peek().k == Tok::K::Op && (peek().op == '-' || peek().op == '+')) {
            char op = peek().op;
            ++pos_;
            auto r = parseUnary();
            if (!r) return nullptr;
            if (op == '-') return mkBin('-', mkNum(0.0), std::move(r));
            return r;
        }
        return parsePower();
    }

    std::unique_ptr<Expr::Node> parsePower()   // ^ 右结合，允许 2^-3
    {
        auto base = parsePrimary();
        if (!base) return nullptr;
        // 后缀 ! % ° 紧绑底数（阶乘优先级高于 ^：2^3! 的阶乘属于指数）
        for (;;) {
            if (peek().k == Tok::K::Op && peek().op == '!') {
                ++pos_;
                base = mkBin('!', std::move(base), nullptr);
                continue;
            }
            if (peek().k == Tok::K::Op && peek().op == '%') {
                ++pos_;
                base = mkBin('%', std::move(base), nullptr);
                continue;
            }
            if (peek().k == Tok::K::Op && peek().op == (char)0xB0) {
                ++pos_;
                base = mkBin((char)0xB0, std::move(base), nullptr);
                continue;
            }
            break;
        }
        if (peek().k == Tok::K::Op && peek().op == '^') {
            ++pos_;
            auto ex = parseUnary();
            if (!ex) return nullptr;
            return mkBin('^', std::move(base), std::move(ex));
        }
        return base;
    }

    std::unique_ptr<Expr::Node> parsePrimary()
    {
        const Tok &t = peek();
        if (t.k == Tok::K::Num) {
            ++pos_;
            return mkNum(t.num);
        }
        if (t.k == Tok::K::Name) {
            std::string name = t.name;
            ++pos_;
            if (peek().k == Tok::K::Op && peek().op == '(') {
                ++pos_;
                // 实参表：逗号分隔（N6 多参函数；单参仍是常见形）
                std::vector<std::unique_ptr<Expr::Node>> args;
                auto first = parseExpr();
                if (!first) return nullptr;
                args.push_back(std::move(first));
                while (peek().k == Tok::K::Op && peek().op == ',') {
                    ++pos_;
                    auto next = parseExpr();
                    if (!next) return nullptr;
                    args.push_back(std::move(next));
                }
                if (!eatOp(')')) {
                    err = "缺少右括号";
                    return nullptr;
                }
                int lo = 0, hi = 0;
                if (!FnArity(name, lo, hi)) {
                    err = "未知函数 " + name;
                    return nullptr;
                }
                if ((int)args.size() < lo || (int)args.size() > hi) {
                    err = name + " 的参数个数不符";
                    return nullptr;
                }
                return mkCall(name, std::move(args));
            }
            if (name == "x" || name == "X") {
                hasX = true;
                return mkVar();
            }
            // 批次⑥：y 为第二变量（两变量不等式区域用；含 y 的表达式进
            // 单变量函数路径由调用方拒绝）
            if (name == "y" || name == "Y") {
                hasY = true;
                return mkYVar();
            }
            if (name == "pi" || name == "PI" || name == "Pi") return mkNum(3.14159265358979323846);
            if (name == "e") return mkNum(2.71828182845904523536);
            if (!param.empty() && name == param) {
                hasParam = true;
                return mkParam();
            }
            // refs 模式（轮 D 表格公式）：未知标识符 → 对象引用节点，
            // 存在性/类型由调用方校验；按首次出现序去重记录
            if (allowRefs) {
                if (std::find(refs.begin(), refs.end(), name) == refs.end()) {
                    refs.push_back(name);
                }
                return mkRef(name);
            }
            err = "未知符号 " + name;
            return nullptr;
        }
        if (t.k == Tok::K::Op && t.op == '(') {
            ++pos_;
            auto n = parseExpr();
            if (!n) return nullptr;
            if (!eatOp(')')) {
                err = "缺少右括号";
                return nullptr;
            }
            return n;
        }
        err = "表达式不完整";
        return nullptr;
    }
};

} // namespace

bool Expr::parse(const std::string &src, std::string &err)
{
    return parse(src, err, std::string(), false);
}

bool Expr::parse(const std::string &src, std::string &err, const std::string &param)
{
    return parse(src, err, param, false);
}

bool Expr::parse(const std::string &src, std::string &err, const std::string &param,
                 bool allowRefs)
{
    root_ = nullptr;
    hasX_ = false;
    hasY_ = false;
    text_.clear();
    refs_.clear();
    std::string t;
    t.reserve(src.size());
    for (char c : src) {
        if (!isspace((unsigned char)c)) t.push_back(c);
    }
    if (t.empty()) {
        err = "表达式为空";
        return false;
    }
    std::vector<Tok> toks;
    if (!Tokenize(t, toks, err)) return false;
    Parser p(toks);
    p.param = param;
    p.allowRefs = allowRefs;
    auto n = p.run();
    if (!n) {
        err = p.err;
        return false;
    }
    root_ = std::move(n);
    hasX_ = p.hasX;
    hasY_ = p.hasY;
    refs_ = p.refs;
    text_ = t;
    return true;
}

double Expr::eval(double x) const
{
    if (!root_) return NAN;
    return EvalNode(root_.get(), x, 0.0, 0.0, nullptr);
}

double Expr::eval(double x, double param) const
{
    if (!root_) return NAN;
    return EvalNode(root_.get(), x, 0.0, param, nullptr);
}

double Expr::evalXY(double x, double y) const
{
    if (!root_) return NAN;
    return EvalNode(root_.get(), x, y, 0.0, nullptr);
}

double Expr::evalResolved(double x,
                          const std::function<double(const std::string &)> &resolve) const
{
    if (!root_) return NAN;
    // param 复用 x 槽位：refs 模式表达式不含 Var 节点（调用侧 containsX/Y 把关），
    // param 命名节点直接取 x 求值
    return EvalNode(root_.get(), x, 0.0, x, &resolve);
}

// 递归判定子树是否为一次式 k*x+m；常数式 k=0 也算。
// 仅放行 Num / Var / 加减 / 数乘 / 除常数——含任何函数调用、x^2、x*x 等直接判否。
static bool LinearCoefNode(const Expr::Node *n, double &k, double &m)
{
    if (!n) return false;
    if (n->kind == Expr::Node::Kind::Num) { k = 0.0; m = n->num; return true; }
    if (n->kind == Expr::Node::Kind::Var) { k = 1.0; m = 0.0; return true; }
    if (n->kind != Expr::Node::Kind::Bin) return false;
    double aK = 0, aM = 0, bK = 0, bM = 0;
    bool aLin = false, bLin = false;
    switch (n->op) {
    case '+':
        aLin = LinearCoefNode(n->l.get(), aK, aM);
        bLin = LinearCoefNode(n->r.get(), bK, bM);
        if (aLin && bLin) { k = aK + bK; m = aM + bM; return true; }
        return false;
    case '-':
        aLin = LinearCoefNode(n->l.get(), aK, aM);
        bLin = LinearCoefNode(n->r.get(), bK, bM);
        if (aLin && bLin) { k = aK - bK; m = aM - bM; return true; }
        return false;
    case '*':
        aLin = LinearCoefNode(n->l.get(), aK, aM);
        bLin = LinearCoefNode(n->r.get(), bK, bM);
        // 数乘：一侧纯常数 × 另一侧一次式（k=0 表示该侧无 x）
        if (aLin && aK == 0.0 && bLin) { k = aM * bK; m = aM * bM; return true; }
        if (bLin && bK == 0.0 && aLin) { k = bM * aK; m = bM * aM; return true; }
        return false;
    case '/':
        aLin = LinearCoefNode(n->l.get(), aK, aM);
        bLin = LinearCoefNode(n->r.get(), bK, bM);
        // 除以常数（分母无 x 且非零）
        if (aLin && bLin && bK == 0.0 && bM != 0.0) { k = aK / bM; m = aM / bM; return true; }
        return false;
    default:
        return false;
    }
}

bool Expr::linearCoef(double &k, double &m) const
{
    k = 0.0;
    m = 0.0;
    return LinearCoefNode(root_.get(), k, m);
}

} // namespace dreamember
