// ratex_napi.cpp — RaTeX NAPI 桥：线性数学串 → LaTeX → DisplayList JSON
// 五十二包：RaTeX（纯 Rust KaTeX 兼容排版引擎，MIT）真机桥接原型。
// 链路：ArkTS 传内核线性串（f(x) = sqrt(x)/2 等）→ 本层转 LaTeX →
// libratex_ffi.so 排版 → DisplayList JSON 原样返回（ArkTS 解析绘制）。
// 转换失败（不认识的构造）返回原文，由 ArkTS 判定回退旧 painter。
#include "napi/native_api.h"
#include <string>
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
        if (s.compare(i, 5, "sqrt(") == 0
            && (i == 0 || !isalnum((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 4);
            if (m != std::string::npos) {
                out += "\\sqrt{" + s.substr(i + 5, m - i - 5) + "}";
                identity = false;
                i = m + 1;
                continue;
            }
        }
        // NthRoot(根次, 被开方数) → \sqrt[根次]{被开方数}（须有顶层逗号）
        if (s.compare(i, 8, "NthRoot(") == 0
            && (i == 0 || !isalnum((unsigned char)s[i - 1]))) {
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
                    out += "\\sqrt[" + s.substr(i + 8, comma - i - 8) + "]{"
                         + s.substr(comma + 1, m - comma - 1) + "}";
                    identity = false;
                    i = m + 1;
                    continue;
                }
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
            out += op;
            ++i;
            continue;
        }
        // π（0xCF 0x80）/ 希腊字母 → \alpha 等
        {
            std::string g;
            if (GreekAt(s, i, g)) {
                out += g;
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
        // ≤ ≥ ≠（E2 89 A4/A5/0x88 0x80）与 × ÷ ± ∞ ∈ 等 Unicode 算符
        if ((unsigned char)s[i] == 0xE2 && i + 2 < s.size()) {
            const unsigned char b1 = (unsigned char)s[i + 1];
            const unsigned char b2 = (unsigned char)s[i + 2];
            if (b1 == 0x89 && b2 == 0xA4) { out += "\\leq"; identity = false; i += 3; continue; }
            if (b1 == 0x89 && b2 == 0xA5) { out += "\\geq"; identity = false; i += 3; continue; }
            if (b1 == 0x89 && b2 == 0xA0) { out += "\\neq"; identity = false; i += 3; continue; }
            if (b1 == 0x88) {
                if (b2 == 0xAB) { out += "\\times"; identity = false; i += 3; continue; }
                if (b2 == 0xB1) { out += "\\pm"; identity = false; i += 3; continue; }
                if (b2 == 0x88) { out += "\\infty"; identity = false; i += 3; continue; }
            }
        }
        // ASCII ×（键盘 '×' 键实为 U+00D7 = 0xC3 0x97）→ \times
        if ((unsigned char)s[i] == 0xC3 && i + 1 < s.size()
            && (unsigned char)s[i + 1] == 0x97) {
            out += "\\times";
            identity = false;
            i += 2;
            continue;
        }
        // LaTeX 保留字符原样放行（{} 之外的 ^_ 已处理；& % # # 等 v1
        // 不出现于内核输出，出现则走回退，不在此转义）
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
// 线性数学串 → LaTeX；可完整转换返回 "ok|<latex>"，含未知构造返回
// "raw|<原文>"（ArkTS 回退旧 painter）。
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
    const std::string out = (identity ? "raw|" : "ok|") + latex;
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
