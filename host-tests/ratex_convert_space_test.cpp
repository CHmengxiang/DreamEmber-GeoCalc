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
    return g_fail;
}
