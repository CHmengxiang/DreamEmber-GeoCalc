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

// 六十一包②/六十二包：光标探针——prefix+rest 拼接转换，markerAt=prefix 字节长
static void expectCaret(const std::string &prefix, const std::string &rest,
    const std::string &want)
{
    bool identity = true;
    bool caretDone = false;
    const std::string s = prefix + rest;
    const std::string got = LinearToLatexCaret(s, identity, 0, s.size(),
        prefix.size(), caretDone, kCaretMarker, 0);
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
    expect("1/2x", "\\frac{1}{2}x");                       // 分式转 \frac（62 包⑤，与内核 (1/2)x 一致）
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
    expect("2NthRoot(3,x)", "2\\sqrt[{3}]{x}");
    expect("asqrt(2)", "asqrt(2)");                        // 标识符中段仍不命中
    // ---- 六十包：空参灰块占位（GGB PLACEHOLDER 同款浅灰块）----
    expect("sqrt()", "\\sqrt{\\color{#E6E6EB}\\rule{0.5em}{0.9em}}");
    expect("2sqrt(2)", "2\\sqrt{2}");          // 有参不变（回归）
    // 六十三包②：根次槽整槽包单组（RaTeX \sqrt[..] 可选参只吃单个顶层组）
    expect("NthRoot(3,)", "\\sqrt[{3}]{\\color{#E6E6EB}\\rule{0.5em}{0.9em}}");
    expect("NthRoot(,x)", "\\sqrt[{\\color{#E6E6EB}\\rule{0.5em}{0.65em}}]{x}");
    expect("NthRoot(3,x)", "\\sqrt[{3}]{x}");
    expect("x^", "x^{\\color{#E6E6EB}\\rule{0.42em}{0.6em}}");   // 悬空指数灰块
    expect("x_", "x_{\\color{#E6E6EB}\\rule{0.42em}{0.6em}}");
    expect("x^2", "x^{2}");                            // 有参不变（回归）
    expect("()", "()");                                // 空括号对直通
    expect("sin()", "\\sin()");              // 函数空参括号紧贴
    // ---- 六十二包⑤：/ → \frac（内核左结合语义保真）+ 模板递归 ----
    expect("1/2", "\\frac{1}{2}");
    expect("1/2x", "\\frac{1}{2}x");                   // 内核 (1/2)·x
    expect("2x/3y", "\\frac{2x}{3}y");                 // 内核 ((2x)/3)·y
    expect("1/2/3", "\\frac{\\frac{1}{2}}{3}");        // 左结合嵌套
    expect("1+2/3", "1+\\frac{2}{3}");
    expect("x^2/3", "\\frac{x^{2}}{3}");
    expect("sqrt(2)/3", "\\frac{\\sqrt{2}}{3}");
    expect("2sqrt(2)/3", "\\frac{2\\sqrt{2}}{3}");
    expect("sin(x)/2", "\\frac{\\sin(x)}{2}");
    expect("1/(2+3)", "\\frac{1}{(2+3)}");
    expect("(a+b)/(c+d)", "\\frac{(a+b)}{(c+d)}");
    expect("1/2^2", "\\frac{1}{2^{2}}");
    expect("y(x) = \xCF\x80/2", "y(x) = \\frac{\\pi}{2}");
    expect("1/", "1/");                                // 悬空分母不转
    expect("/2", "/2");                                // 悬空分子不转
    expect("a\xC3\xB7" "b/c", "a\\div b/c");           // ÷ 前邻不转（保内核值）
    expect("sqrt(sqrt(2))", "\\sqrt{\\sqrt{2}}");      // 嵌套模板（递归修复）
    expect("sqrt(1/2)", "\\sqrt{\\frac{1}{2}}");       // 根内分式
    // ---- 六十二包①②：探针分上下文（根号内 0.6em/根次 1.1em）----
    const std::string M = "{\\color{#010203}\\rule{0.05em}{0.8em}}";
    const std::string MR = "{\\color{#010203}\\rule{0.05em}{0.6em}}";
    const std::string MI = "{\\color{#010203}\\rule{0.05em}{1.1em}}";
    const std::string G =
        "\\color{#E6E6EB}\\rule{0.5em}{0.9em}";        // 空参灰块
    const std::string GS = "\\color{#E6E6EB}\\rule{0.42em}{0.6em}";
    expectCaret("", "", M);                            // 空串首
    expectCaret("x", "", "x" + M);                     // 单字符尾
    expectCaret("y=2x+1", "", "y=2x+1" + M);           // 串尾
    expectCaret("y=2x+", "1", "y=2x+" + M + "1");      // 二目后=真实落点
    expectCaret("sqrt(2)", "", "\\sqrt{2}" + M);       // 落点在根号后=组外
    expectCaret("sqrt(2", ")", "\\sqrt{2" + MR + "}"); // 根号内尾（')' 之前）
    expectCaret("sqrt(", "2)", "\\sqrt{" + MR + "2}"); // 根号内首
    expectCaret("sqrt()", "", "\\sqrt{" + G + "}" + M);      // 空根号后=组外
    expectCaret("sqrt(", ")", "\\sqrt{" + MR + G + "}");     // 空根号内探针在灰块前
    expectCaret("x^", "", "x^{" + M + GS + "}");       // 悬空指数探针在灰块前
    expectCaret("x^2", "", "x^{2" + M + "}");          // 指数尾（箭头不出组）
    expectCaret("x^", "2", "x^{" + M + "2}");          // 指数首
    expectCaret("x_", "2", "x_{" + M + "2}");          // 下标首
    expectCaret("x^(n", "+1)", "x^{n" + M + "+1}");    // ^( 组内
    expectCaret("NthRoot(3", ",x)", "\\sqrt[{3" + MI + "}]{x}"); // 根次槽探针 1.1em
    expectCaret("NthRoot(3,", "x)", "\\sqrt[{3}]{" + MR + "x}"); // 被开方数首 0.6em
    expectCaret("NthRoot(", ",x)", "\\sqrt[{" + MI
        + "\\color{#E6E6EB}\\rule{0.5em}{0.65em}}]{x}"); // 空根次槽探针在灰块前
    expectCaret("NthRoot(23", ",8)", "\\sqrt[{23" + MI + "}]{8}"); // 根次内容尾
    expectCaret("NthRoot(,2)", "", "\\sqrt[{\\color{#E6E6EB}\\rule{0.5em}{0.65em}}]{2}"
        + M); // 串尾=根号外（探针不出模板）
    expectCaret("abs(", "x)", "\\left|" + M + "x\\right|");   // abs 内首
    expectCaret("y=\xCE\xB1", "", "y=\\alpha" + M);    // 希腊字母尾（UTF-8 字节落点）
    // ---- 六十二包⑤：分式内探针 ----
    expectCaret("1+", "2/3", "1+\\frac{" + M + "2}{3}");     // 分子首
    expectCaret("1/2", "", "\\frac{1}{2" + M + "}");         // 串尾=分母尾
    expectCaret("1/", "2", "\\frac{1}{" + M + "2}");         // 分母首
    expectCaret("1", "/2", "\\frac{1" + M + "}{2}");         // 落点在 / 处=分子尾
    expectCaret("sqrt(1", "/2)", "\\sqrt{\\frac{1" + MR + "}{2}}"); // 根内分式
    // ---- 六十三包④：符号表扩充 + log(底,真数) 下标 + Floor/Ceil ----
    expect("x\xE2\x89\x88y", "x\\approx y");               // ≈
    expect("a\xE2\x88\xA7" "b", "a\\wedge b");             // ∧
    expect("a\xE2\x88\xA8" "b", "a\\vee b");               // ∨
    expect("\xC2\xAC" "a", "\\neg a");                     // ¬
    expect("A\xE2\x86\x92" "B", "A\\to B");                // →
    expect("A\xE2\x87\x92" "B", "A\\Rightarrow B");        // ⇒
    expect("A\xE2\x8A\x82" "B", "A\\subset B");            // ⊂
    expect("A\xE2\x8A\x86" "B", "A\\subseteq B");          // ⊆
    expect("A\xE2\x88\xAA" "B", "A\\cup B");               // ∪
    expect("A\xE2\x8A\xA5" "B", "A\\perp B");              // ⊥
    expect("\xE2\x88\xA0" "ABC", "\\angle ABC");           // ∠
    expect("x\xE2\x80\xB2", "x'");                         // ′ → '
    expect("\xE2\x88\x91i", "\\sum i");                    // ∑
    expect("\xE2\x88\xAB_0^1", "\\int_{0}^{1}");           // ∫ 带上下限
    expect("log(2,x)", "\\log_{2}(x)");                    // 底数下标
    expect("log(,x)", "\\log_{\\color{#E6E6EB}\\rule{0.5em}{0.65em}}(x)");
    expect("log(x)", "\\log(x)");                          // 无逗号=普通 \log
    expect("Floor(x)", "\\lfloor x \\rfloor");             // ⌊⌋ 键
    expect("Ceil(2.5)", "\\lceil 2.5 \\rceil");            // ⌈⌉ 键
    expect("Floor()", "\\lfloor " + G + " \\rfloor");      // 空参灰块
    return g_fail;
}
