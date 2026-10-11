// 宿主验证：ratex_napi.cpp 的 LinearToLatex 希腊/算符控制字后跟字母时
// 必须补空格（\pix 修复回归）。stub/napi 桩头文件屏蔽 NAPI 依赖，只测纯字符串逻辑。
#include <string>
#include <cstdio>
#include "../../entry/src/main/cpp/ratex/ratex_napi.cpp"

// FFI 桩：本测试只覆盖 LinearToLatex 字符串逻辑，不排版
extern "C" {
RatexResult ratex_parse_and_layout(const char *, const RatexOptions *)
    { RatexResult r; r.data = nullptr; r.error_code = 1; return r; }
void ratex_free_display_list(char *) {}
const char *ratex_get_last_error() { return "stub"; }
}

static int g_fail = 0;
static void expect(const std::string &in, const std::string &want)
{
    bool identity = true;
    const std::string got = LinearToLatex(in, identity);
    const bool ok = got == want;
    if (!ok) g_fail++;
    printf("%s in=%s got=%s want=%s\n", ok ? "PASS" : "FAIL",
        in.c_str(), got.c_str(), want.c_str());
}

// 六十一包②：光标探针——prefix+rest 拼接转换，markerAt=prefix 字节长
static void expectCaret(const std::string &prefix, const std::string &rest,
    const std::string &want)
{
    bool identity = true;
    bool caretDone = false;
    const std::string got = LinearToLatexCaret(prefix + rest, identity,
        prefix.size(), caretDone);
    const bool ok = got == want && caretDone;
    if (!ok) g_fail++;
    printf("%s in=%s|%s got=%s want=%s\n", ok ? "PASS" : "FAIL",
        prefix.c_str(), rest.c_str(), got.c_str(), want.c_str());
}

int main()
{
    expect("y(x) = \xCF\x80x", "y(x) = \\pi x");          // πx → \pi x（修复目标）
    expect("y=\xCF\x80", "y=\\pi");                        // 行尾不补
    expect("y=\xCE\xB1x+\xCE\xB2", "y=\\alpha x+\\beta");  // αx+β
    expect("2\xC3\x97x", "2\\times x");                    // 2×x → \times x
    expect("6\xC3\xB7" "2", "6\\div2");                    // 6÷2（数字终止命令名，不补空格）
    expect("x\xE2\x89\xA4y", "x\\leq y");                  // x≤y
    expect("y=\xCF\x80x+\xE2\x88\x9E", "y=\\pi x+\\infty");// πx+∞
    expect("x\xE2\x88\x88" "A", "x\\in A");                // x∈A
    expect("1\xC2\xB1" "2", "1\\pm2");                     // 1±2（数字终止命令名，不补空格）
    // ---- 五十八包：identity 放行 + 特殊字符转义 + abs/函数名 ----
    expect("y=2x+1", "y=2x+1");                            // 简单式直通（RaTeX 缺省链路）
    expect("1/2x", "1/2x");                                // 分式线性直通（与内核求值一致）
    expect("50%", "50\\%");                                // % 转义（防 LaTeX 注释吞行）
    expect("a#b", "a\\#b");
    expect("a&b", "a\\&b");
    expect("a$b", "a\\$b");
    expect("abs(x)", "\\left|x\\right|");                  // abs → 绝对值
    expect("sin(x)+cos(y)", "\\sin(x)+\\cos(y)");          // 函数名直立体
    expect("sinh(x)", "\\sinh(x)");                        // 长名优先不误配 sin
    expect("asin(x)", "\\asin(x)");
    expect("xsin(x)", "xsin(x)");                          // 标识符中段不命中
    // ---- 五十九包：模板守卫 isalnum→isalpha（数字/右括号后=并列乘法） ----
    expect("2sqrt(2)", "2\\sqrt{2}");                      // 2·√2（58 包真机回归）
    expect("2abs(x)", "2\\left|x\\right|");
    expect("2NthRoot(3,x)", "2\\sqrt[3]{x}");
    expect("asqrt(2)", "asqrt(2)");                        // 标识符中段仍不命中
    // ---- 六十包：空参灰块占位（GGB PLACEHOLDER 同款浅灰块）----
    expect("sqrt()", "\\sqrt{\\color{#E6E6EB}\\rule{0.5em}{0.9em}}");
    expect("2sqrt(2)", "2\\sqrt{2}");          // 有参不变（回归）
    expect("NthRoot(3,)", "\\sqrt[3]{\\color{#E6E6EB}\\rule{0.5em}{0.9em}}");
    expect("NthRoot(,x)", "\\sqrt[\\color{#E6E6EB}\\rule{0.3em}{0.4em}]{x}");
    expect("x^", "x^{\\color{#E6E6EB}\\rule{0.35em}{0.5em}}");   // 悬空指数灰块
    expect("x_", "x_{\\color{#E6E6EB}\\rule{0.35em}{0.5em}}");
    expect("x^2", "x^{2}");                            // 有参不变（回归）
    expect("()", "()");                                // 空括号对直通
    expect("sin()", "\\sin()");              // 函数空参括号紧贴
    // ---- 六十一包②：光标探针（M=魔色 rule，GGB CursorAtom 同源模型）----
    const std::string M = "{\\color{#010203}\\rule{0.05em}{0.8em}}";
    const std::string G =
        "\\color{#E6E6EB}\\rule{0.5em}{0.9em}";        // 空参灰块
    const std::string GS = "\\color{#E6E6EB}\\rule{0.35em}{0.5em}";
    expectCaret("", "", M);                            // 空串首
    expectCaret("x", "", "x" + M);                     // 单字符尾
    expectCaret("y=2x+1", "", "y=2x+1" + M);           // 串尾
    expectCaret("y=2x+", "1", "y=2x+" + M + "1");      // 二目后=真实落点
    expectCaret("sqrt(2)", "", "\\sqrt{2}" + M);       // 落点在根号后=组外
    expectCaret("sqrt(2", ")", "\\sqrt{2" + M + "}");  // 根号内尾（')' 之前）
    expectCaret("sqrt(", "2)", "\\sqrt{" + M + "2}");  // 根号内首
    expectCaret("sqrt()", "", "\\sqrt{" + G + "}" + M);      // 空根号后=组外
    expectCaret("sqrt(", ")", "\\sqrt{" + M + G + "}");      // 空根号内探针在灰块前
    expectCaret("x^", "", "x^{" + M + GS + "}");       // 悬空指数探针在灰块前
    expectCaret("x^2", "", "x^{2" + M + "}");          // 指数尾（箭头不出组）
    expectCaret("x^", "2", "x^{" + M + "2}");          // 指数首
    expectCaret("x_", "2", "x_{" + M + "2}");          // 下标首
    expectCaret("x^(n", "+1)", "x^{n" + M + "+1}");    // ^( 组内
    expectCaret("NthRoot(3", ",x)", "\\sqrt[3" + M + "]{x}"); // 根次槽尾（跨逗号）
    expectCaret("NthRoot(3,", "x)", "\\sqrt[3]{" + M + "x}"); // 被开方数首
    expectCaret("abs(", "x)", "\\left|" + M + "x\\right|");   // abs 内首
    expectCaret("y=\xCE\xB1", "", "y=\\alpha" + M);    // 希腊字母尾（UTF-8 字节落点）
    return g_fail;
}
