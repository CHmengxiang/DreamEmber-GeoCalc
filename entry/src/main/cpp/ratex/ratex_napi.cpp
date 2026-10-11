// ratex_napi.cpp — RaTeX NAPI 桥：线性数学串 → LaTeX → DisplayList JSON
// 五十二包：RaTeX（纯 Rust KaTeX 兼容排版引擎，MIT）真机桥接原型。
// 链路：ArkTS 传内核线性串（f(x) = sqrt(x)/2 等）→ 本层转 LaTeX →
// libratex_ffi.so 排版 → DisplayList JSON 原样返回（ArkTS 解析绘制）。
// 五十八包起恒交 RaTeX 排版（ok|），排版失败由 ArkTS 回退旧 painter 兜底。
#include "napi/native_api.h"
#include <string>
#include <cstring>
#include "ratex/ratex.h"

namespace {

std::string NapiToStr(napi_env env, napi_value v)
{
    size_t len = 0;
    napi_get_value_string_utf8(env, v, nullptr, 0, &len);
    std::string s(len + 1, '\0');
    napi_get_value_string_utf8(env, v, &s[0], len + 1, &len);
    s.resize(len);
    return s;
}

// ---------- 线性串 → LaTeX ----------
// 只转显示相关的构造；认识的字符集之外的原文原样返回（isIdentity 判定，
// ArkTS 侧据此回退旧 painter，保证任何输入不出错）。
//  · sqrt(x) → \sqrt{x}；NthRoot(a, b) → \sqrt[b]{a}（顶层逗号才算模板）
//  · x^2 → x^{2}；x^(n+1) → x^{n+1}（^ 后含非单字符时整体括号化）
//  · a_n → a_{n}；a_(m+1) → a_{m+1}
//  · π → \pi、° → ^{\circ}、× → \times、≤/≥ → \leq / \geq、≠ → \neq
//  · α..ω 希腊字母 → \alpha 等（与内核 IsGreekAt 同字节判定）
//  · 分式 / 不转：内核 1/2x=(1/2)x 左结合语义与 \frac 可视分组有差，
//    v1 保持线性（RaTeX 渲染 1/2x 与内核求值一致）
bool GreekAt(const std::string &s, size_t i, std::string &out)
{
    // 内核规则：0xCE 开头（U+0370–03FF 希腊区）两字节；0xCF 0x80 = π 单列
    const unsigned char b0 = (unsigned char)s[i];
    if (i + 1 >= s.size()) return false;
    const unsigned char b1 = (unsigned char)s[i + 1];
    static const char *kLower[] = {
        "alpha", "beta", "gamma", "delta", "varepsilon", "zeta", "eta",
        "theta", "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron",
        "pi", "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega"
    };
    static const char *kUpper[] = {
        "Gamma", "Delta", "Theta", "Lambda", "Xi", "Pi", "Sigma",
        "Upsilon", "Phi", "Psi", "Omega"
    };
    if (b0 == 0xCE) {
        // 0xCE 0x91..0xA9 = α..ω（跳过 0xB6 ς 变体另行）；0xCE 0x91=U+03B1
        if (b1 >= 0xB1 && b1 <= 0xC9 && b1 != 0xB6) {
            // U+03B1..U+03C9：两字节 0xCE/0xCF 跨界，0xCE 段覆盖 0x91..0xBF
            // （α..ο），0xCF 段覆盖 π..ω——此分支只出 α..ο
            if (b1 <= 0xBF) {
                out = "\\" + std::string(kLower[b1 - 0xB1]);
                return true;
            }
        }
        if (b1 >= 0x93 && b1 <= 0xA9) {
            // Γ(0x93) Δ(0x94) Θ(0x98) Λ(0x9B) Ξ(0x9E) Π(0xA0) Σ(0xA3)
            // Υ(0xA5) Φ(0xA6) Ψ(0xA7) Ω(0xA9)
            int idx = -1;
            switch (b1) {
            case 0x93: idx = 0; break; case 0x94: idx = 1; break;
            case 0x98: idx = 2; break; case 0x9B: idx = 3; break;
            case 0x9E: idx = 4; break; case 0xA0: idx = 5; break;
            case 0xA3: idx = 6; break; case 0xA5: idx = 7; break;
            case 0xA6: idx = 8; break; case 0xA7: idx = 9; break;
            case 0xA9: idx = 10; break; default: break;
            }
            if (idx >= 0) {
                out = "\\" + std::string(kUpper[idx]);
                return true;
            }
        }
        return false;
    }
    if (b0 == 0xCF) {
        // π = 0xCF 0x80；0xCF 0x81..0xC9 = ρ..ω
        if (b1 == 0x80) {
            out = "\\pi";
            return true;
        }
        if (b1 >= 0x81 && b1 <= 0x89) {
            // ρ σ τ υ φ χ ψ ω（0x81..0x88）+ ϖ(0x89) 不转
            if (b1 <= 0x88) {
                out = "\\" + std::string(kLower[b1 - 0x81 + 15]);
                return true;
            }
            return false;
        }
        return false;
    }
    return false;
}

bool GreekAtRaw(const std::string &s, size_t i)
{
    std::string tmp;
    return GreekAt(s, i, tmp);
}

// 控制字分隔：LaTeX 词法里 \pi 后紧跟字母会读成 \pix（未知命令）。命令后
// 一个空格被 tokenizer 吞掉、不影响视觉，仅在紧邻字母/反斜杠时需要。
bool CmdNeedsSpace(char c)
{
    return isalpha((unsigned char)c) || c == '\\';
}

// 前向配对括号，返回 ')' 下标；不配对返回 npos
size_t MatchParen(const std::string &s, size_t open)
{
    int depth = 0;
    for (size_t i = open; i < s.size(); ++i) {
        if (s[i] == '(') ++depth;
        else if (s[i] == ')') {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::string::npos;
}

// 空参占位灰块（六十包②，看齐 GGB：TeXSerializer.PLACEHOLDER = 浅灰实
// 心块 \bgcolor{#E6E6EB}\scalebox{1}[1.6]{\phantom{g}}，编辑器里空参数
// 的可见提示；RaTeX 无 bgcolor/scalebox，以 \color+\rule 等效——rule 的
// em 尺寸相对当前字号，根号/上标/次根内自动缩放，DisplayList 里是粗
// Line 块，绘制端零改动。GGB 对「光标所在空 field」只画光标不画框，我
// 们光标自绘同理，故一律灰块即可）
const char kPlaceholder[] = "\\color{#E6E6EB}\\rule{0.5em}{0.9em}";
// 根次槽位（\sqrt[...]）的窄版灰块
const char kPlaceholderIdx[] = "\\color{#E6E6EB}\\rule{0.3em}{0.4em}";
// 脚本槽位（^/_）的窄版灰块
const char kPlaceholderScript[] = "\\color{#E6E6EB}\\rule{0.35em}{0.5em}";

// 从 from 起的脚本内容（无括号组时取连续字母数字字符）
size_t ScriptRunEnd(const std::string &s, size_t from)
{
    size_t j = from;
    while (j < s.size()
           && (isalnum((unsigned char)s[j]) || s[j] == '_' || GreekAtRaw(s, j))) {
        j += GreekAtRaw(s, j) ? 2 : 1;
    }
    return j;
}

std::string LinearToLatex(const std::string &s, bool &identity)
{
    identity = true;
    std::string out;
    out.reserve(s.size() + 16);
    size_t i = 0;
    while (i < s.size()) {
        // sqrt( → \sqrt{...}（不配对则原样）
        // 守卫只排除字母（标识符中段 asqrt(）：内核量名不含字母紧贴模板名
        // 的情况；数字/右括号后是并列乘法 2sqrt(2)=2·√2，必须转换——
        // 59 包修复：原 isalnum 把数字后跟的 sqrt 也拦下，identity 放行后
        // 渲染成字面 "2sqrt(2)"（真机 58 包反馈）
        if (s.compare(i, 5, "sqrt(") == 0
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 4);
            if (m != std::string::npos) {
                out += "\\sqrt{";
                // 空参（键盘 √ 模板 sqrt() 未填）：灰块占位（六十包②——
                // 原先空参走旧 painter 虚线框致整行退旧渲染）
                if (m == i + 5) {
                    out += kPlaceholder;
                } else {
                    out += s.substr(i + 5, m - i - 5);
                }
                out += "}";
                identity = false;
                i = m + 1;
                continue;
            }
        }
        // NthRoot(根次, 被开方数) → \sqrt[根次]{被开方数}（须有顶层逗号）
        if (s.compare(i, 8, "NthRoot(") == 0
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 7);
            if (m != std::string::npos) {
                size_t comma = std::string::npos;
                int depth = 0;
                for (size_t q = i + 8; q < m; ++q) {
                    if (s[q] == '(') ++depth;
                    else if (s[q] == ')') --depth;
                    else if (s[q] == ',' && depth == 0) { comma = q; break; }
                }
                if (comma != std::string::npos) {
                    out += "\\sqrt[";
                    if (comma == i + 8) {
                        out += kPlaceholderIdx;   // 根次未填（NthRoot(,x)）
                    } else {
                        out += s.substr(i + 8, comma - i - 8);
                    }
                    out += "]{";
                    if (m == comma + 1) {
                        out += kPlaceholder;      // 被开方数未填（NthRoot(3,)）
                    } else {
                        out += s.substr(comma + 1, m - comma - 1);
                    }
                    out += "}";
                    identity = false;
                    i = m + 1;
                    continue;
                }
            }
        }
        // abs(x) → \left|x\right|（五十八包：identity 放行后 RaTeX 成为
        // 缺省链路，纯字母直通会把 abs 渲染成斜体 a·b·s，倒退于旧 painter
        // 的 |·|；排版引擎不支持 \left| 时整串排版失败走回退，无回归）
        if (s.compare(i, 4, "abs(") == 0
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 3);
            if (m != std::string::npos) {
                out += "\\left|" + s.substr(i + 4, m - i - 4) + "\\right|";
                identity = false;
                i = m + 1;
                continue;
            }
        }
        // 常用函数名 → \sin 等（五十八包：仅调用形态——名字后紧 '(' 且前
        // 面非字母，避免命中标识符中段；长名在前防前缀误配 sinh/sin）
        if (s[i] >= 'a' && s[i] <= 'z'
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            static const char *kFns[] = {
                "arcsin", "arccos", "arctan", "sinh", "cosh", "tanh",
                "asin", "acos", "atan", "sin", "cos", "tan", "cot",
                "sec", "csc", "ln", "log", "exp", "min", "max",
                "floor", "ceil", "round", "sgn"
            };
            bool fnHit = false;
            for (const char *fn : kFns) {
                const size_t len = strlen(fn);
                if (s.compare(i, len, fn) == 0 && i + len < s.size()
                    && s[i + len] == '(') {
                    out += '\\';
                    out += fn;
                    i += len;
                    identity = false;
                    fnHit = true;
                    break;
                }
            }
            if (fnHit) {
                continue;
            }
        }
        // ^ / _ 脚本：^2 → ^{2}；^(n+1) → ^{n+1}
        if (s[i] == '^' || s[i] == '_') {
            const char op = s[i];
            if (i + 1 < s.size() && s[i + 1] == '(') {
                const size_t m = MatchParen(s, i + 1);
                if (m != std::string::npos) {
                    out += op;
                    out += "{" + s.substr(i + 2, m - i - 2) + "}";
                    identity = false;
                    i = m + 1;
                    continue;
                }
            }
            const size_t j = ScriptRunEnd(s, i + 1);
            if (j > i + 1) {
                out += op;
                out += "{" + s.substr(i + 1, j - i - 1) + "}";
                identity = false;
                i = j;
                continue;
            }
            // 悬空脚本（x^、x_ 输入中途态，六十包③）：出灰块占位进组——
            // 原样放行 ^/_ 时 RaTeX 报 "Expected group after" 整串排版失
            // 败退旧渲染；补灰块后排版成功且占位可见（GGB ^{\jlmcursor}
            // /^{灰块} 同语义）
            out += op;
            out += "{" + std::string(kPlaceholderScript) + "}";
            identity = false;
            ++i;
            continue;
        }
        // π（0xCF 0x80）/ 希腊字母 → \alpha 等
        {
            std::string g;
            if (GreekAt(s, i, g)) {
                out += g;
                if (i + 2 < s.size() && CmdNeedsSpace(s[i + 2])) {
                    out += ' ';
                }
                identity = false;
                i += 2;
                continue;
            }
        }
        // °（0xC2 0xB0）→ ^{\circ}
        if ((unsigned char)s[i] == 0xC2 && i + 1 < s.size()
            && (unsigned char)s[i + 1] == 0xB0) {
            out += "^{\\circ}";
            identity = false;
            i += 2;
            continue;
        }
        // ≤ ≥ ≠（E2 89 A4/A5/A0）与 ∞（E2 88 9E）、∈（E2 88 88）等
        // Unicode 算符
        if ((unsigned char)s[i] == 0xE2 && i + 2 < s.size()) {
            const unsigned char b1 = (unsigned char)s[i + 1];
            const unsigned char b2 = (unsigned char)s[i + 2];
            if (b1 == 0x89 && b2 == 0xA4) { out += "\\leq"; i += 3; identity = false; }
            else if (b1 == 0x89 && b2 == 0xA5) { out += "\\geq"; i += 3; identity = false; }
            else if (b1 == 0x89 && b2 == 0xA0) { out += "\\neq"; i += 3; identity = false; }
            else if (b1 == 0x88) {
                if (b2 == 0x9E) { out += "\\infty"; i += 3; identity = false; }
                else if (b2 == 0x88) { out += "\\in"; i += 3; identity = false; }
                else { out += s[i]; ++i; }
            }
            else { out += s[i]; ++i; }
            if (identity == false && i < s.size() && CmdNeedsSpace(s[i])) {
                out += ' ';
            }
            continue;
        }
        // ASCII ×（U+00D7 = 0xC3 0x97）→ \times、÷（U+00F7 = 0xC3 0xB7）→ \div
        if ((unsigned char)s[i] == 0xC3 && i + 1 < s.size()
            && ((unsigned char)s[i + 1] == 0x97 || (unsigned char)s[i + 1] == 0xB7)) {
            out += (unsigned char)s[i + 1] == 0x97 ? "\\times" : "\\div";
            identity = false;
            i += 2;
            if (i < s.size() && CmdNeedsSpace(s[i])) {
                out += ' ';
            }
            continue;
        }
        // ±（U+00B1 = 0xC2 0xB1）→ \pm
        if ((unsigned char)s[i] == 0xC2 && i + 1 < s.size()
            && (unsigned char)s[i + 1] == 0xB1) {
            out += "\\pm";
            identity = false;
            i += 2;
            if (i < s.size() && CmdNeedsSpace(s[i])) {
                out += ' ';
            }
            continue;
        }
        // LaTeX 特殊字符转义（五十八包）：identity 放行后这些字符若原样
        // 进排版引擎会改变语义（% 注释吞行、# & $ 宏参数/对齐/数学模式切
        // 换、{} 分组），转义后按字面渲染
        if (s[i] == '%' || s[i] == '#' || s[i] == '&' || s[i] == '$'
            || s[i] == '{' || s[i] == '}') {
            out += '\\';
            out += s[i];
            identity = false;
            ++i;
            continue;
        }
        // 其余字符原样放行（^_ 已处理；未知 Unicode 交给排版引擎，失败
        // 由 ArkTS 回退旧 painter 兜底）
        out += s[i];
        ++i;
    }
    return out;
}

} // namespace

// ratexLayout(latex: string, displayMode: number): string
// 返回 "ok|<json>" 或 "error:<msg>"。LaTeX 由调用方（ArkTS/cathode 转换层）
// 提供；本接口不做线性转换——线性转换在 ratexConvert。
// '#RRGGBB'（可带 #）→ RatexColor；非法输入返回 false（调用方保持缺省黑）
static bool ParseHexColor(const std::string &hex, RatexColor &out)
{
    size_t i = (!hex.empty() && hex[0] == '#') ? 1 : 0;
    if (hex.size() - i < 6) return false;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    int r = nib(hex[i]), g = nib(hex[i + 2]), b = nib(hex[i + 4]);
    // 中间两位
    r = r * 16 + nib(hex[i + 1]);
    g = g * 16 + nib(hex[i + 3]);
    b = b * 16 + nib(hex[i + 5]);
    if (r < 0 || g < 0 || b < 0) return false;
    out.r = r / 255.0f;
    out.g = g / 255.0f;
    out.b = b / 255.0f;
    out.a = 1.0f;
    return true;
}

static napi_value RatexLayout(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value argv[3] = {nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "ratexLayout requires (latex, displayMode?, color?)");
        return nullptr;
    }
    const std::string latex = NapiToStr(env, argv[0]);
    int displayMode = 0;
    if (argc >= 2) {
        napi_valuetype vt = napi_undefined;
        napi_typeof(env, argv[1], &vt);
        if (vt == napi_number) {
            napi_get_value_int32(env, argv[1], &displayMode);
        }
    }

    RatexColor color = {0.0f, 0.0f, 0.0f, 1.0f};
    if (argc >= 3) {
        napi_valuetype vt = napi_undefined;
        napi_typeof(env, argv[2], &vt);
        if (vt == napi_string) {
            ParseHexColor(NapiToStr(env, argv[2]), color);
        }
    }
    RatexOptions opts;
    opts.struct_size = sizeof(RatexOptions);
    opts.display_mode = displayMode != 0 ? 1 : 0;
    opts.color = &color;

    RatexResult res = ratex_parse_and_layout(latex.c_str(), &opts);
    if (res.error_code != 0 || res.data == nullptr) {
        const char *err = ratex_get_last_error();
        std::string msg = "error:" + std::string(err ? err : "unknown ratex error");
        if (res.data != nullptr) ratex_free_display_list(res.data);
        napi_value out;
        napi_create_string_utf8(env, msg.c_str(), msg.size(), &out);
        return out;
    }
    std::string ok = "ok|" + std::string(res.data);
    ratex_free_display_list(res.data);
    napi_value out;
    napi_create_string_utf8(env, ok.c_str(), ok.size(), &out);
    return out;
}

// ratexConvert(linear: string): string
// 线性数学串 → LaTeX，恒返回 "ok|<latex>"（五十八包：identity=串中无被
// 转换的构造，曾据此判回退——但简单式 y=2x+1、分式 1/2 等纯线性串因此
// 全部错过 RaTeX（真机 57 包反馈③④）。现放开：RaTeX 成为缺省链路，线
// 性渲染与内核求值一致；排版失败的兜底回退在 ArkTS 侧（ratexLayout 失
// 败 → 旧 painter），任何输入不出错。
static napi_value RatexConvert(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "ratexConvert requires (linear)");
        return nullptr;
    }
    const std::string linear = NapiToStr(env, argv[0]);
    bool identity = true;
    const std::string latex = LinearToLatex(linear, identity);
    (void)identity;
    const std::string out = "ok|" + latex;
    napi_value res;
    napi_create_string_utf8(env, out.c_str(), out.size(), &res);
    return res;
}

void DreamemberRegisterRatexApis(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "ratexLayout", nullptr, RatexLayout, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "ratexConvert", nullptr, RatexConvert, nullptr, nullptr, nullptr, napi_default, nullptr },
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
