// ratex_napi.cpp — RaTeX NAPI 桥：线性数学串 → LaTeX → DisplayList JSON
// 五十二包：RaTeX（纯 Rust KaTeX 兼容排版引擎，MIT）真机桥接原型。
// 链路：ArkTS 传内核线性串（f(x) = sqrt(x)/2 等）→ 本层转 LaTeX →
// libratex_ffi.so 排版 → DisplayList JSON 原样返回（ArkTS 解析绘制）。
// 五十八包起恒交 RaTeX 排版（ok|），排版失败由 ArkTS 回退旧 painter 兜底。
// 六十一包：光标魔色探针（ratexConvert 可选 rest 参，prefix/rest 边界插探
// 针）；六十二包：模板内容 span 递归（嵌套模板/希腊全转换）+ / → \frac
// （内核左结合语义保真）+ 探针分上下文三种（根号内 0.6em/根次 1.1em）。
#include "napi/native_api.h"
#include <string>
#include <cstring>
#include <vector>
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
size_t MatchParen(const std::string &s, size_t open, size_t to)
{
    int depth = 0;
    for (size_t i = open; i < to && i < s.size(); ++i) {
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
// 根次槽位（\sqrt[...]）的灰块——六十二包②加大（原 0.3/0.4em 经引擎
// ×0.5 后仅 2.5×3.3px 看不见）
const char kPlaceholderIdx[] = "\\color{#E6E6EB}\\rule{0.5em}{0.65em}";
// 脚本槽位（^/_）的灰块——六十二包③加大（下标位原尺寸过小难辨）
const char kPlaceholderScript[] = "\\color{#E6E6EB}\\rule{0.42em}{0.6em}";
// 光标探针（六十一包②，GGB CursorAtom 同源模型）：转换器在光标落点插入
// 的魔色 rule，排版后 DisplayList 里该色 Line 项的 x/y/thickness 即光标
// x 与上下文盒——rule 的 em 尺寸随当前 style 自动缩放（宿主 dump：上标内
// 0.8em→0.56em 且抬升、下标内下沉、根次内 ×0.5、根号内同尺寸贴内容基
// 线），ArkTS 据此自绘光标，二态 caretScript 不再参与 RaTeX 视觉路径。
// 外层 {} 成组：悬空 ^/^（ 切片后 ^ 后跟本探针时整组成为上标，语义正确
const char kCaretMarker[] = "{\\color{#010203}\\rule{0.05em}{0.8em}}";
// 根号内专用探针（六十二包①）：0.6em——根号内容为同尺寸 cramped style，
// 统一 0.8em 时内外变化不明显；根内收细后切换可见（宿主 dump 定标）
const char kCaretMarkerRoot[] = "{\\color{#010203}\\rule{0.05em}{0.6em}}";
// 根次槽位专用探针（六十二包②）：引擎 rootStyle ×0.5，0.8em→0.4em 基本
// 不可见；1.1em→0.55em 恢复可见
const char kCaretMarkerIdx[] = "{\\color{#010203}\\rule{0.05em}{1.1em}}";

// 字节级判定（/ 分式因子扫描用）：° U+00B0=0xC2 0xB0、× U+00D7=0xC3 0x97、
// ÷ U+00F7=0xC3 0xB7
bool DegreeAt(const std::string &s, size_t i)
{
    return i + 1 < s.size() && (unsigned char)s[i] == 0xC2
        && (unsigned char)s[i + 1] == 0xB0;
}
bool TimesAt(const std::string &s, size_t i)
{
    return i + 1 < s.size() && (unsigned char)s[i] == 0xC3
        && (unsigned char)s[i + 1] == 0x97;
}
bool DivAt(const std::string &s, size_t i)
{
    return i + 1 < s.size() && (unsigned char)s[i] == 0xC3
        && (unsigned char)s[i + 1] == 0xB7;
}

// 分母前扫的「名+括号组」整体吸收判定：名须为已知函数/模板（x(2) 是
// 并列乘法只取 x，sin(x) 是调用取整体）
bool KnownFn(const std::string &s, size_t from, size_t to)
{
    static const char *kNames[] = {
        "arcsin", "arccos", "arctan", "sinh", "cosh", "tanh",
        "asin", "acos", "atan", "sin", "cos", "tan", "cot",
        "sec", "csc", "ln", "log", "exp", "min", "max",
        "floor", "ceil", "round", "sgn", "sqrt", "NthRoot", "abs",
        "Floor", "Ceil", "Derivative", "Integral"
    };
    if (to <= from) return false;
    const size_t len = to - from;
    for (const char *nm : kNames) {
        if (strlen(nm) == len && s.compare(from, len, nm) == 0) return true;
    }
    return false;
}

// 从 from 起的脚本内容（限 to；无括号组时取连续字母数字字符）
size_t ScriptRunEnd(const std::string &s, size_t from, size_t to)
{
    size_t j = from;
    while (j < to && j < s.size()
           && (isalnum((unsigned char)s[j]) || s[j] == '_' || GreekAtRaw(s, j))) {
        j += GreekAtRaw(s, j) ? 2 : 1;
    }
    return j;
}

// 两参入口（显示链路+宿主回归）：不插探针，全串转换
std::string LinearToLatexCaret(const std::string &s, bool &identity,
                               size_t from, size_t to, size_t markerAt,
                               bool &caretDone, const char *marker, int depth);
std::string LinearToLatex(const std::string &s, bool &identity)
{
    bool caretDone = false;
    return LinearToLatexCaret(s, identity, 0, s.size(), std::string::npos,
                              caretDone, kCaretMarker, 0);
}

// 六十一包②/六十二包⑤：[from,to) 跨度转换（模板内容递归，嵌套模板/分式
// 全转换）；markerAt=光标落点（prefix 字节长）时——主循环走到该下标、或
// 子跨度尾==markerAt（span 端点即落点：模板槽尾/分式分子尾等）、或外层串
// 尾——插入 marker（根号内传 Root 版、根次槽传 Idx 版，探针随上下文变
// 尺寸）；caretDone 防双发。markerAt=npos（显示链路）全检查短路
std::string LinearToLatexCaret(const std::string &s, bool &identity,
                               size_t from, size_t to, size_t markerAt,
                               bool &caretDone, const char *marker, int depth)
{
    identity = true;
    std::string out;
    out.reserve(s.size() + 16);
    // 分式分子回扫需要「输出截断点」：loop-top 逐位记 out 长（ns 恒为
    // loop-top 位——因子只可能起于跨度首/运算符后）
    std::vector<size_t> outLen(to + 1, std::string::npos);
    size_t i = from;
    while (i < to) {
        outLen[i] = out.size();
        if (i == markerAt && !caretDone) {
            out += marker;
            caretDone = true;
        }
        // sqrt( → \sqrt{...}（不配对则原样）
        // 守卫只排除字母（标识符中段 asqrt(）：内核量名不含字母紧贴模板名
        // 的情况；数字/右括号后是并列乘法 2sqrt(2)=2·√2，必须转换——
        // 59 包修复：原 isalnum 把数字后跟的 sqrt 也拦下，identity 放行后
        // 渲染成字面 "2sqrt(2)"（真机 58 包反馈）
        if (s.compare(i, 5, "sqrt(") == 0
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 4, to);
            if (m != std::string::npos) {
                out += "\\sqrt{";
                const size_t cs = i + 5;
                // 空参（键盘 √ 模板 sqrt() 未填）：灰块占位（六十包②）；
                // 落点在空槽内探针插灰块前（键入字符将出现的位置）
                if (m == cs) {
                    if (markerAt == cs && !caretDone) {
                        out += kCaretMarkerRoot;
                        caretDone = true;
                    }
                    out += kPlaceholder;
                } else {
                    // 内容递归转换（六十二包⑤：嵌套模板/分式/希腊字母全
                    // 转换——61 包为切片直出，嵌套 sqrt 不转）；根号内传
                    // Root 探针（①：0.6em，根号内外变化明显）
                    out += LinearToLatexCaret(s, identity, cs, m, markerAt,
                                              caretDone, kCaretMarkerRoot,
                                              depth + 1);
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
            const size_t m = MatchParen(s, i + 7, to);
            if (m != std::string::npos) {
                size_t comma = std::string::npos;
                int dep = 0;
                for (size_t q = i + 8; q < m; ++q) {
                    if (s[q] == '(') ++dep;
                    else if (s[q] == ')') --dep;
                    else if (s[q] == ',' && dep == 0) { comma = q; break; }
                }
                if (comma != std::string::npos) {
                    // 六十三包②：整个根次槽包成单组——RaTeX \sqrt[..] 可选
                    // 参解析只吃单个顶层组（宿主 dump：{探针组}{灰块} 在探
                    // 针组闭括号处报 Expected EOF 整串失败退旧 painter，根
                    // 次槽光标因此消失）；包组后探针/灰块/递归内容的花括号
                    // 都在组内，任意光标位不再触发边界
                    out += "\\sqrt[{";
                    const size_t cs0 = i + 8;
                    if (comma == cs0) {
                        // 根次未填（NthRoot(,x)）；落点在空槽内探针插灰块前
                        //（六十二包②：根次专用 1.1em 探针——引擎 rootStyle
                        // ×0.5 后 0.55em 仍可见）
                        if (markerAt == cs0 && !caretDone) {
                            out += kCaretMarkerIdx;
                            caretDone = true;
                        }
                        out += kPlaceholderIdx;
                    } else {
                        out += LinearToLatexCaret(s, identity, cs0, comma,
                                                  markerAt, caretDone,
                                                  kCaretMarkerIdx, depth + 1);
                    }
                    out += "}]{";
                    const size_t cs1 = comma + 1;
                    if (m == cs1) {
                        // 被开方数未填（NthRoot(3,)）；根号内 0.6em 探针
                        if (markerAt == cs1 && !caretDone) {
                            out += kCaretMarkerRoot;
                            caretDone = true;
                        }
                        out += kPlaceholder;
                    } else {
                        out += LinearToLatexCaret(s, identity, cs1, m,
                                                  markerAt, caretDone,
                                                  kCaretMarkerRoot, depth + 1);
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
            const size_t m = MatchParen(s, i + 3, to);
            if (m != std::string::npos) {
                out += "\\left|";
                const size_t cs = i + 4;
                if (m == cs) {
                    if (markerAt == cs && !caretDone) {
                        out += marker;
                        caretDone = true;
                    }
                } else {
                    out += LinearToLatexCaret(s, identity, cs, m, markerAt,
                                              caretDone, marker, depth + 1);
                }
                out += "\\right|";
                identity = false;
                i = m + 1;
                continue;
            }
        }
        // log(底数, 真数) → \log_{底数}(真数)（六十三包④，键盘 log□ 键与
        // 内核 Log(底,值) 的显示语义；无顶层逗号落通用 fn 分支按 \log(…)
        // 显示。空槽出灰块，光标探针走基线上下文）
        if (s.compare(i, 4, "log(") == 0
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 3, to);
            if (m != std::string::npos) {
                size_t comma = std::string::npos;
                int dep = 0;
                for (size_t q = i + 4; q < m; ++q) {
                    if (s[q] == '(') ++dep;
                    else if (s[q] == ')') --dep;
                    else if (s[q] == ',' && dep == 0) { comma = q; break; }
                }
                if (comma != std::string::npos) {
                    out += "\\log_{";
                    const size_t cs0 = i + 4;
                    if (comma == cs0) {
                        if (markerAt == cs0 && !caretDone) {
                            out += kCaretMarker;
                            caretDone = true;
                        }
                        out += kPlaceholderIdx;
                    } else {
                        out += LinearToLatexCaret(s, identity, cs0, comma,
                                                  markerAt, caretDone,
                                                  kCaretMarker, depth + 1);
                    }
                    out += "}(";
                    const size_t cs1 = comma + 1;
                    if (m == cs1) {
                        if (markerAt == cs1 && !caretDone) {
                            out += kCaretMarker;
                            caretDone = true;
                        }
                        out += kPlaceholder;
                    } else {
                        out += LinearToLatexCaret(s, identity, cs1, m,
                                                  markerAt, caretDone,
                                                  kCaretMarker, depth + 1);
                    }
                    out += ")";
                    identity = false;
                    i = m + 1;
                    continue;
                }
            }
        }
        // Floor(x)/Ceil(x) → \lfloor x\rfloor、\lceil x\rceil（六十三包④，
        // 键盘 ⌊⌋/⌈⌉ 键插入大写命令名，通用 fn 分支只认小写——不转会被
        // 渲染成斜体字母 F·l·o·o·r）
        if ((s.compare(i, 6, "Floor(") == 0 || s.compare(i, 5, "Ceil(") == 0)
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const bool isFloor = s[i] == 'F';
            const size_t m = MatchParen(s, i + (isFloor ? 5 : 4), to);
            if (m != std::string::npos) {
                out += isFloor ? "\\lfloor " : "\\lceil ";
                const size_t cs = i + (isFloor ? 6 : 5);
                if (m == cs) {
                    if (markerAt == cs && !caretDone) {
                        out += marker;
                        caretDone = true;
                    }
                    out += kPlaceholder;
                } else {
                    out += LinearToLatexCaret(s, identity, cs, m, markerAt,
                                              caretDone, marker, depth + 1);
                }
                out += isFloor ? " \\rfloor" : " \\rceil";
                identity = false;
                i = m + 1;
                continue;
            }
        }
        // Derivative(f) → \frac{d(f)}{dx}（六十五包②，键盘 d□/d□ 键插命
        // 令名——不转渲染斜体字面 D·e·r…）。二参：第二参全数字=求导阶
        // \frac{d^{n}(f)}{dx^{n}}，否则=变量 \frac{d(f)}{d(v)}。空槽灰块，
        // 探针走基线上下文（宿主 dump：d(x^2)/dx、d^2 形态、灰块全过）
        if (s.compare(i, 11, "Derivative(") == 0
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 10, to);
            if (m != std::string::npos) {
                size_t comma = std::string::npos;
                int dep = 0;
                for (size_t q = i + 11; q < m; ++q) {
                    if (s[q] == '(') ++dep;
                    else if (s[q] == ')') --dep;
                    else if (s[q] == ',' && dep == 0) { comma = q; break; }
                }
                bool orderForm = false;
                if (comma != std::string::npos && comma + 1 < m) {
                    orderForm = true;
                    for (size_t q = comma + 1; q < m; ++q) {
                        if (!isdigit((unsigned char)s[q])) {
                            orderForm = false;
                            break;
                        }
                    }
                }
                const size_t cs0 = i + 11;
                const size_t ce0 = comma != std::string::npos ? comma : m;
                out += "\\frac{d";
                if (orderForm) {
                    out += "^{";
                    out += s.substr(comma + 1, m - comma - 1);
                    out += "}";
                }
                out += "(";
                if (ce0 == cs0) {
                    if (markerAt == cs0 && !caretDone) {
                        out += kCaretMarker;
                        caretDone = true;
                    }
                    out += kPlaceholder;
                } else {
                    out += LinearToLatexCaret(s, identity, cs0, ce0,
                                              markerAt, caretDone,
                                              kCaretMarker, depth + 1);
                }
                out += ")}{d";
                if (orderForm) {
                    out += "x^{";
                    out += s.substr(comma + 1, m - comma - 1);
                    out += "}";
                } else if (comma != std::string::npos) {
                    const size_t cs1 = comma + 1;
                    out += "(";
                    if (m == cs1) {
                        if (markerAt == cs1 && !caretDone) {
                            out += kCaretMarker;
                            caretDone = true;
                        }
                        out += kPlaceholder;
                    } else {
                        out += LinearToLatexCaret(s, identity, cs1, m,
                                                  markerAt, caretDone,
                                                  kCaretMarker, depth + 1);
                    }
                    out += ")";
                } else {
                    out += "x";
                }
                out += "}";
                identity = false;
                i = m + 1;
                continue;
            }
        }
        // Integral(f) → \int f\,dx；Integral(f, v) → \int f\,dv（多字/非字
        // 母变量 d(v)）；Integral(f, a, b) → \int_{a}^{b} f\,dx（GGB 定积
        // 分形态）；≥4 参不转（落通用字面）。空槽灰块（限槽小灰块），探针
        // 走基线上下文（宿主 dump：单参/变量/定限/灰块全过）
        if (s.compare(i, 9, "Integral(") == 0
            && (i == 0 || !isalpha((unsigned char)s[i - 1]))) {
            const size_t m = MatchParen(s, i + 8, to);
            if (m != std::string::npos) {
                size_t c1 = std::string::npos;
                size_t c2 = std::string::npos;
                bool tooMany = false;
                int dep = 0;
                for (size_t q = i + 9; q < m; ++q) {
                    if (s[q] == '(') ++dep;
                    else if (s[q] == ')') --dep;
                    else if (s[q] == ',' && dep == 0) {
                        if (c1 == std::string::npos) {
                            c1 = q;
                        } else if (c2 == std::string::npos) {
                            c2 = q;
                        } else {
                            tooMany = true;
                            break;
                        }
                    }
                }
                if (!tooMany) {
                    const size_t fs = i + 9;
                    const size_t fe = c1 != std::string::npos ? c1 : m;
                    auto emitArg = [&](size_t as, size_t ae, const char *ph) {
                        if (ae == as) {
                            if (markerAt == as && !caretDone) {
                                out += kCaretMarker;
                                caretDone = true;
                            }
                            out += ph;
                        } else {
                            out += LinearToLatexCaret(s, identity, as, ae,
                                                      markerAt, caretDone,
                                                      kCaretMarker, depth + 1);
                        }
                    };
                    if (c2 != std::string::npos) {
                        out += "\\int_{";
                        emitArg(c1 + 1, c2, kPlaceholderIdx);
                        out += "}^{";
                        emitArg(c2 + 1, m, kPlaceholderIdx);
                        out += "} ";
                    } else {
                        out += "\\int ";
                    }
                    emitArg(fs, fe, kPlaceholder);
                    if (c2 != std::string::npos) {
                        out += "\\,dx";
                    } else if (c1 != std::string::npos) {
                        out += "\\,d";
                        if (c1 + 2 == m && isalpha((unsigned char)s[c1 + 1])) {
                            out += s.substr(c1 + 1, 1);
                        } else {
                            out += "(";
                            emitArg(c1 + 1, m, kPlaceholder);
                            out += ")";
                        }
                    } else {
                        out += "\\,dx";
                    }
                    identity = false;
                    i = m + 1;
                    continue;
                }
            }
        }
        // 循环小数 0.(3)/1.2(34) → 0.\overline{3}（六十五包③，键盘 □̄ 键
        // 插 0.()——旧 painter over 原子有上划线，RaTeX 链补齐同款）。判
        // 据与 ArkTS parseMathAtoms 同：'(' 前回溯「整数段.小数段」，整数
        // 位至少一位；不配对/中缀不转（2(3) 仍并列乘法字面）。空 () 出
        // (灰块)——与 ArkTS 空参分支（'(' 灰 ')') 同形
        if (s[i] == '(' && i > from
            && (isdigit((unsigned char)s[i - 1]) || s[i - 1] == '.')) {
            size_t k1 = i;
            while (k1 > from && isdigit((unsigned char)s[k1 - 1])) {
                --k1;
            }
            bool ok = false;
            size_t dotPos = i;
            if (k1 == i && s[i - 1] == '.') {
                // 0.() 形态：'(' 紧跟小数点，点前须有整数位
                size_t k2 = i - 1;
                while (k2 > from && isdigit((unsigned char)s[k2 - 1])) {
                    --k2;
                }
                if (k2 < i - 1) {
                    ok = true;
                }
            } else if (k1 < i && k1 > from && s[k1 - 1] == '.') {
                // 1.2(34) 形态
                size_t k2 = k1 - 1;
                while (k2 > from && isdigit((unsigned char)s[k2 - 1])) {
                    --k2;
                }
                if (k2 < k1 - 1) {
                    ok = true;
                }
            }
            if (ok) {
                const size_t m = MatchParen(s, i, to);
                if (m != std::string::npos) {
                    // 循环位必须全数字（与 ArkTS parseMathAtoms 同判据——
                    // 0.5(x) 是 0.5·(x) 并列乘法，字母循环位不转）
                    bool repOk = true;
                    for (size_t q = i + 1; q < m; ++q) {
                        if (!isdigit((unsigned char)s[q])) {
                            repOk = false;
                            break;
                        }
                    }
                    if (repOk) {
                        if (m == i + 1) {
                            out += "(";
                            if (markerAt == i + 1 && !caretDone) {
                                out += kCaretMarker;
                                caretDone = true;
                            }
                            out += kPlaceholder;
                            out += ")";
                        } else {
                            out += "\\overline{";
                            out += LinearToLatexCaret(s, identity, i + 1, m,
                                                      markerAt, caretDone,
                                                      kCaretMarker, depth + 1);
                            out += "}";
                        }
                        identity = false;
                        i = m + 1;
                        continue;
                    }
                }
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
            if (i + 1 < to && s[i + 1] == '(') {
                const size_t m = MatchParen(s, i + 1, to);
                if (m != std::string::npos) {
                    out += op;
                    out += "{";
                    const size_t cs = i + 2;
                    if (m == cs) {
                        if (markerAt == cs && !caretDone) {
                            out += marker;
                            caretDone = true;
                        }
                    } else {
                        out += LinearToLatexCaret(s, identity, cs, m, markerAt,
                                                  caretDone, marker,
                                                  depth + 1);
                    }
                    out += "}";
                    identity = false;
                    i = m + 1;
                    continue;
                }
            }
            const size_t j = ScriptRunEnd(s, i + 1, to);
            if (j > i + 1) {
                out += op;
                out += "{";
                out += LinearToLatexCaret(s, identity, i + 1, j, markerAt,
                                          caretDone, marker, depth + 1);
                out += "}";
                identity = false;
                i = j;
                continue;
            }
            // 悬空脚本（x^、x_ 输入中途态，六十包③）：出灰块占位进组——
            // 原样放行 ^/_ 时 RaTeX 报 "Expected group after" 整串排版失
            // 败退旧渲染；补灰块后排版成功且占位可见（GGB ^{\jlmcursor}
            // /^{灰块} 同语义）。光标落点紧随其上时探针插灰块前
            out += op;
            out += "{";
            if (markerAt == i + 1 && !caretDone) {
                out += marker;
                caretDone = true;
            }
            out += kPlaceholderScript;
            out += "}";
            identity = false;
            ++i;
            continue;
        }
        // 六十二包⑤：/ → \frac{分子}{分母}——内核左结合（2x/3y=((2x)/3)y、
        // 1/2/3=((1/2)/3)），跨距按同语义取：分子=向左吸收连续因子（字母
        // 数字/希腊/°/括号组+名/^_脚本//链/×），分母=恰一个因子（已知名+
        // 括号组、字母数字、希腊、°、脚本链）。分子前紧邻 ÷ 不转（a÷b/c
        // 内核=((a÷b)/c)，frac 化视觉成 a÷(b/c) 改值）。已输出区段截断重
        // 转（outLen 记账），跨度内嵌套模板/分式递归全转换
        if (s[i] == '/' && depth < 32) {
            size_t ns = i;
            while (ns > from) {
                const unsigned char c = (unsigned char)s[ns - 1];
                if (c == ')') {
                    int dep2 = 0;
                    size_t k = ns;
                    bool found = false;
                    while (k > from) {
                        --k;
                        if (s[k] == ')') ++dep2;
                        else if (s[k] == '(') {
                            if (--dep2 == 0) { found = true; break; }
                        }
                    }
                    if (!found) break;
                    ns = k;
                    while (ns > from && isalpha((unsigned char)s[ns - 1])) --ns;
                    continue;
                }
                if (isalnum(c)) {
                    while (ns > from && isalnum((unsigned char)s[ns - 1])) --ns;
                    continue;
                }
                if (ns >= from + 2 && (GreekAtRaw(s, ns - 2) || DegreeAt(s, ns - 2)
                                       || TimesAt(s, ns - 2))) {
                    ns -= 2;
                    continue;
                }
                if (c == '^' || c == '_' || c == '/') { --ns; continue; }
                break;
            }
            const bool divBefore = ns >= from + 2 && DivAt(s, ns - 2);
            size_t de = i + 1;
            bool hasDen = false;
            if (de < to) {
                const unsigned char c = (unsigned char)s[de];
                if (c == '(') {
                    const size_t m2 = MatchParen(s, de, to);
                    if (m2 != std::string::npos) { de = m2 + 1; hasDen = true; }
                } else if (isalpha(c)) {
                    while (de < to && isalpha((unsigned char)s[de])) ++de;
                    while (de < to && isdigit((unsigned char)s[de])) ++de;
                    if (de < to && s[de] == '(' && KnownFn(s, i + 1, de)) {
                        const size_t m2 = MatchParen(s, de, to);
                        if (m2 != std::string::npos) de = m2 + 1;
                    }
                    hasDen = true;
                } else if (isdigit(c)) {
                    while (de < to && isdigit((unsigned char)s[de])) ++de;
                    hasDen = true;
                } else if (GreekAtRaw(s, de) || DegreeAt(s, de)) {
                    de += 2;
                    hasDen = true;
                }
                if (hasDen) {
                    // 脚本链 ^/_（后随一个原子：组/字母数字/希腊）
                    while (de < to && (s[de] == '^' || s[de] == '_')) {
                        ++de;
                        if (de < to && s[de] == '(') {
                            const size_t m3 = MatchParen(s, de, to);
                            if (m3 == std::string::npos) break;
                            de = m3 + 1;
                        } else if (de < to && isalnum((unsigned char)s[de])) {
                            while (de < to && isalnum((unsigned char)s[de])) ++de;
                        } else if (de + 1 < to && GreekAtRaw(s, de)) {
                            de += 2;
                        } else {
                            --de;      // 悬空脚本符留给主循环，分母不含它
                            break;
                        }
                    }
                    if (de + 1 < to && DegreeAt(s, de)) de += 2;
                }
            }
            if (hasDen && ns < i && !divBefore
                && outLen[ns] != std::string::npos) {
                if (ns < i) {
                    out.resize(outLen[ns]);
                    if (caretDone && markerAt >= ns) {
                        caretDone = false;   // 截断区内已插探针一并撤，重插
                    }
                }
                out += "\\frac{";
                out += LinearToLatexCaret(s, identity, ns, i, markerAt,
                                          caretDone, marker, depth + 1);
                out += "}{";
                out += LinearToLatexCaret(s, identity, i + 1, de, markerAt,
                                          caretDone, marker, depth + 1);
                out += "}";
                identity = false;
                i = de;
                continue;
            }
            out += '/';
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
        // Unicode 算符/关系/集合/逻辑/大算符 → LaTeX 命令（六十三包④扩
        // 表：键盘符号页 ∧∨→¬⊗∥⊥∠⊂⊆≇′″ 与常用 ≈∑∏∫ 等此前未转，三字节
        // 原样进引擎整串失败退旧渲染；未识别序列三字节整体放行）
        if ((unsigned char)s[i] == 0xE2 && i + 2 < s.size()) {
            const unsigned char b1 = (unsigned char)s[i + 1];
            const unsigned char b2 = (unsigned char)s[i + 2];
            const char *cmd = nullptr;
            if (b1 == 0x89) {               // 关系
                if (b2 == 0xA4) cmd = "\\leq";
                else if (b2 == 0xA5) cmd = "\\geq";
                else if (b2 == 0xA0) cmd = "\\neq";
                else if (b2 == 0x88) cmd = "\\approx";
                else if (b2 == 0xA1) cmd = "\\equiv";
                else if (b2 == 0x85) cmd = "\\cong";
            } else if (b1 == 0x88) {
                if (b2 == 0x9E) cmd = "\\infty";
                else if (b2 == 0x88) cmd = "\\in";
                else if (b2 == 0x89) cmd = "\\notin";
                else if (b2 == 0x91) cmd = "\\sum";
                else if (b2 == 0x8F) cmd = "\\prod";
                else if (b2 == 0xAB) cmd = "\\int";
                else if (b2 == 0xAE) cmd = "\\oint";
                else if (b2 == 0xBC) cmd = "\\sim";
                else if (b2 == 0x9D) cmd = "\\propto";
                else if (b2 == 0xA7) cmd = "\\wedge";
                else if (b2 == 0xA8) cmd = "\\vee";
                else if (b2 == 0xA5) cmd = "\\parallel";
                else if (b2 == 0xAA) cmd = "\\cup";
                else if (b2 == 0xA9) cmd = "\\cap";
                else if (b2 == 0x85) cmd = "\\emptyset";
                else if (b2 == 0xA0) cmd = "\\angle";
                else if (b2 == 0x87) cmd = "\\nabla";
                else if (b2 == 0x82) cmd = "\\partial";
                else if (b2 == 0x80) cmd = "\\forall";
                else if (b2 == 0x83) cmd = "\\exists";
            } else if (b1 == 0x8A) {        // 集合/二元圈算符/几何
                if (b2 == 0x82) cmd = "\\subset";
                else if (b2 == 0x83) cmd = "\\supset";
                else if (b2 == 0x86) cmd = "\\subseteq";
                else if (b2 == 0x87) cmd = "\\supseteq";
                else if (b2 == 0x95) cmd = "\\oplus";
                else if (b2 == 0x97) cmd = "\\otimes";
                else if (b2 == 0xA5) cmd = "\\perp";
            } else if (b1 == 0x86) {        // 箭头
                if (b2 == 0x92) cmd = "\\to";
                else if (b2 == 0x90) cmd = "\\leftarrow";
                else if (b2 == 0x94) cmd = "\\leftrightarrow";
            } else if (b1 == 0x87) {
                if (b2 == 0x92) cmd = "\\Rightarrow";
                else if (b2 == 0x94) cmd = "\\Leftrightarrow";
            } else if (b1 == 0x80 && b2 == 0xB2) {  // ′ → '
                out += '\'';
                identity = false;
                i += 3;
                continue;
            } else if (b1 == 0x80 && b2 == 0xB3) {  // ″ → ''
                out += "''";
                identity = false;
                i += 3;
                continue;
            }
            if (cmd != nullptr) {
                out += cmd;
                identity = false;
                i += 3;
                if (i < s.size() && CmdNeedsSpace(s[i])) {
                    out += ' ';
                }
                continue;
            }
            out += s[i];
            out += s[i + 1];
            out += s[i + 2];
            i += 3;
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
        // ±（U+00B1 = 0xC2 0xB1）→ \pm、¬（U+00AC = 0xC2 0xAC）→ \neg
        if ((unsigned char)s[i] == 0xC2 && i + 1 < s.size()) {
            if ((unsigned char)s[i + 1] == 0xB1) {
                out += "\\pm";
                identity = false;
                i += 2;
                if (i < s.size() && CmdNeedsSpace(s[i])) {
                    out += ' ';
                }
                continue;
            }
            if ((unsigned char)s[i + 1] == 0xAC) {
                out += "\\neg ";
                identity = false;
                i += 2;
                continue;
            }
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
    // 落点在跨度尾：主循环顶部不再触发（i==to 时循环已退出）。子跨度尾
    // ==markerAt（模板槽尾/分式分子尾）与外层串尾在此统一补插
    if (markerAt != std::string::npos && !caretDone && markerAt >= to
        && (to == markerAt || to == s.size())) {
        out += marker;
        caretDone = true;
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

// ratexConvert(linear: string, rest?: string): string
// 线性数学串 → LaTeX，恒返回 "ok|<latex>"（五十八包：identity=串中无被
// 转换的构造，曾据此判回退——但简单式 y=2x+1、分式 1/2 等纯线性串因此
// 全部错过 RaTeX（真机 57 包反馈③④）。现放开：RaTeX 成为缺省链路，线
// 性渲染与内核求值一致；排版失败的兜底回退在 ArkTS 侧（ratexLayout 失
// 败 → 旧 painter），任何输入不出错。
// 六十一包②：rest=光标后剩余串时，两段拼接整体转换，并在 linear/rest 边
// 界（=光标落点，ArkTS 已按 UTF-16 切好串，边界即 UTF-8 字节下标）插入
// 光标探针——模板跨边界（sqrt(2|) 等）由槽位切分逻辑覆盖
static napi_value RatexConvert(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "ratexConvert requires (linear, rest?)");
        return nullptr;
    }
    const std::string linear = NapiToStr(env, argv[0]);
    size_t markerAt = std::string::npos;
    std::string s = linear;
    if (argc >= 2) {
        napi_valuetype vt = napi_undefined;
        napi_typeof(env, argv[1], &vt);
        if (vt == napi_string) {
            s = linear + NapiToStr(env, argv[1]);
            markerAt = linear.size();
        }
    }
    bool identity = true;
    bool caretDone = false;
    const std::string latex = LinearToLatexCaret(s, identity, 0, s.size(),
                                                 markerAt, caretDone,
                                                 kCaretMarker, 0);
    (void)identity;
    (void)caretDone;
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
