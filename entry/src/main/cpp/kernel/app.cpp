// app.cpp — 内核控制器实现：输入解析、命令分发、构造列表、绘图指令生成
#include "kernel/app.h"
#include "kernel/expr.h"
#include "cas/cas_wrapper.h"
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <algorithm>
#include <array>
#include <limits>

namespace dreamember {

namespace {

std::string ToLower(const std::string &s)
{
    std::string out(s);
    for (char &c : out) c = (char)tolower((unsigned char)c);
    return out;
}

// 去掉字符串两端空白
std::string Trim(const std::string &s)
{
    size_t b = 0, e = s.size();
    while (b < e && isspace((unsigned char)s[b])) ++b;
    while (e > b && isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}

// 深色画布下近黑对象反白（上游 深色主题的可读性处理）；其余颜色原样
std::string ObjectColor(const GeoColor &c, bool dark)
{
    if (!dark) return ColorHex(c);
    int lum = (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
    if (lum < 80) return "#E8EAED";
    return ColorHex(c);
}

// ---------- N4 .ggb XML 工具 ----------

// XML 属性文本转义（< > & "）
std::string XmlEsc(const std::string &s)
{
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '<': o += "&lt;"; break;
        case '>': o += "&gt;"; break;
        case '&': o += "&amp;"; break;
        case '"': o += "&quot;"; break;
        default: o.push_back(c);
        }
    }
    return o;
}

// 数值属性格式（%.10g 保证往返精度）
std::string NumA(double v)
{
    char b[40];
    snprintf(b, sizeof(b), "%.10g", v);
    return std::string(b);
}

// 属性实体解码（Text 文本等可含任意字符；&amp; 必须最后替换，
// 才能把 "&amp;lt;" 正确还原成 "&lt;"）
std::string XmlUnesc(const std::string &s)
{
    std::string o;
    o.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        if (s.compare(i, 4, "&lt;") == 0) { o.push_back('<'); i += 4; }
        else if (s.compare(i, 4, "&gt;") == 0) { o.push_back('>'); i += 4; }
        else if (s.compare(i, 6, "&quot;") == 0) { o.push_back('"'); i += 6; }
        else if (s.compare(i, 5, "&amp;") == 0) { o.push_back('&'); i += 5; }
        else o.push_back(s[i++]);
    }
    return o;
}

// 在标签原文里取属性值：要求 key 前是空白（避免 "scale" 误匹配 "yscale"）
std::string AttrVal(const std::string &tag, const std::string &key)
{
    size_t k = 0;
    for (;;) {
        k = tag.find(key, k);
        if (k == std::string::npos) return "";
        char prev = k > 0 ? tag[k - 1] : ' ';
        size_t after = k + key.size();
        bool prevOk = prev == ' ' || prev == '\t' || prev == '\n' || prev == '\r';
        if (prevOk && after < tag.size() && tag[after] == '=') {
            size_t q1 = tag.find('"', after);
            if (q1 == std::string::npos) return "";
            size_t q2 = tag.find('"', q1 + 1);
            if (q2 == std::string::npos) return "";
            return XmlUnesc(tag.substr(q1 + 1, q2 - q1 - 1));
        }
        k = after;
    }
}

double AttrNum(const std::string &tag, const std::string &key, double def)
{
    std::string v = AttrVal(tag, key);
    if (v.empty()) return def;
    return strtod(v.c_str(), nullptr);
}

// ---------- N5 几何辅助 ----------

// 点到线段距离（同一坐标系，像素/世界均可）
double PtSegDist(double px, double py, double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1, dy = y2 - y1;
    double len2 = dx * dx + dy * dy;
    double t = len2 > 1e-18 ? ((px - x1) * dx + (py - y1) * dy) / len2 : 0.0;
    t = std::max(0.0, std::min(1.0, t));
    double cx = x1 + t * dx, cy = y1 + t * dy;
    return sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
}

// 任意对象 → 所在直线 ax+by=c（线段/射线取所在直线；直线类以外返回 false）
bool LineCoefsOf(GeoElement *e, double &a, double &b, double &c)
{
    if (!e) return false;
    if (e->type() == GeoType::Line) {
        GeoLine *l = static_cast<GeoLine *>(e);
        a = l->a;
        b = l->b;
        c = l->c;
        return fabs(a) > 1e-15 || fabs(b) > 1e-15;
    }
    if (e->type() == GeoType::Segment) {
        GeoSegment *s = static_cast<GeoSegment *>(e);
        a = s->ey1() - s->ey2();
        b = s->ex2() - s->ex1();
        c = a * s->ex1() + b * s->ey1();
        return fabs(a) > 1e-15 || fabs(b) > 1e-15;
    }
    if (e->type() == GeoType::Ray) {
        GeoRay *r = static_cast<GeoRay *>(e);
        a = r->p1->y - r->p2->y;
        b = r->p2->x - r->p1->x;
        c = a * r->p1->x + b * r->p1->y;
        return fabs(a) > 1e-15 || fabs(b) > 1e-15;
    }
    return false;
}

// 点是否落在对象的有界范围（线段参数区间 / 射线半平面）；直线/圆/函数恒真
bool OnObject(GeoElement *e, double x, double y)
{
    if (e->type() == GeoType::Segment) {
        GeoSegment *s = static_cast<GeoSegment *>(e);
        double dx = s->ex2() - s->ex1(), dy = s->ey2() - s->ey1();
        double len2 = dx * dx + dy * dy;
        if (len2 < 1e-18) return false;
        double t = ((x - s->ex1()) * dx + (y - s->ey1()) * dy) / len2;
        return t >= -1e-9 && t <= 1 + 1e-9;
    }
    if (e->type() == GeoType::Ray) {
        GeoRay *r = static_cast<GeoRay *>(e);
        double dx = r->p2->x - r->p1->x, dy = r->p2->y - r->p1->y;
        double len2 = dx * dx + dy * dy;
        if (len2 < 1e-18) return false;
        double t = ((x - r->p1->x) * dx + (y - r->p1->y) * dy) / len2;
        return t >= -1e-9;
    }
    return true;
}

// ---- 四十五包 路径依附点：路径判定 / 参数取点 / 指针投影 ----

// 可附着的路径对象：线段/向量（t∈[0,1]）、直线（ℝ）、射线（[0,∞)）、
// 圆（θ 弧度）、函数图像（x，不等式区域不算）、多边形边（边序号+t）
bool IsPathType(GeoElement *e)
{
    if (!e) return false;
    const GeoType t = e->type();
    if (t == GeoType::Segment || t == GeoType::Vector || t == GeoType::Line
        || t == GeoType::Ray || t == GeoType::Circle || t == GeoType::Polygon) {
        return true;
    }
    if (t == GeoType::Function) {
        GeoFunction *f = static_cast<GeoFunction *>(e);
        return !f->ineqOp && !f->ineqTwoVar && f->ineqChain.empty()
            && f->ineqDNF.empty() && f->definable();
    }
    return false;
}

// 多边形第 i 个顶点（变换像取缓存）
bool PolyVert(GeoPolygon *pg, size_t i, double &x, double &y)
{
    const size_t n = pg->image ? pg->ivx.size() : pg->verts.size();
    if (n == 0 || i >= n) return false;
    if (pg->image) {
        x = pg->ivx[i];
        y = pg->ivy[i];
    } else {
        x = pg->verts[i]->x;
        y = pg->verts[i]->y;
    }
    return true;
}

// 路径参数 → 世界坐标。退化路径（零长线段/零半径圆/不可求值函数）返回 false
bool PathPointAt(GeoElement *e, double param, double &x, double &y)
{
    if (!IsPathType(e)) return false;
    switch (e->type()) {
    case GeoType::Segment: {
        GeoSegment *s = static_cast<GeoSegment *>(e);
        double t = std::max(0.0, std::min(1.0, param));
        x = s->ex1() + t * (s->ex2() - s->ex1());
        y = s->ey1() + t * (s->ey2() - s->ey1());
        return true;
    }
    case GeoType::Vector: {
        GeoVector *v = static_cast<GeoVector *>(e);
        double t = std::max(0.0, std::min(1.0, param));
        x = v->ex1() + t * (v->ex2() - v->ex1());
        y = v->ey1() + t * (v->ey2() - v->ey1());
        return true;
    }
    case GeoType::Line: {
        double a, b, c;
        if (!LineCoefsOf(e, a, b, c)) return false;
        // 单位方向 (-b,a)/|·|，锚点 = 直线上距原点最近点（参数=有向距离）
        const double n2 = a * a + b * b;
        x = a * c / n2 - (b / sqrt(n2)) * param;
        y = b * c / n2 + (a / sqrt(n2)) * param;
        return true;
    }
    case GeoType::Ray: {
        GeoRay *r = static_cast<GeoRay *>(e);
        double dx = r->p2->x - r->p1->x, dy = r->p2->y - r->p1->y;
        double len = sqrt(dx * dx + dy * dy);
        if (len < 1e-15) return false;
        double t = std::max(0.0, param);
        x = r->p1->x + (dx / len) * t;
        y = r->p1->y + (dy / len) * t;
        return true;
    }
    case GeoType::Circle: {
        GeoCircle *c = static_cast<GeoCircle *>(e);
        if (c->radius < 1e-15) return false;
        x = c->ccx() + c->radius * cos(param);
        y = c->ccy() + c->radius * sin(param);
        return true;
    }
    case GeoType::Function: {
        GeoFunction *f = static_cast<GeoFunction *>(e);
        double yv = f->evalAt(param);
        if (!std::isfinite(yv)) return false;
        x = param;
        y = yv;
        return true;
    }
    case GeoType::Polygon: {
        GeoPolygon *pg = static_cast<GeoPolygon *>(e);
        const size_t n = pg->image ? pg->ivx.size() : pg->verts.size();
        if (n == 0) return false;
        long i = static_cast<long>(floor(param));
        if (i < 0) i = 0;
        if (i >= static_cast<long>(n)) i = static_cast<long>(n) - 1;
        double t = param - floor(param);
        t = std::max(0.0, std::min(1.0, t));
        double ax, ay, bx, by;
        if (!PolyVert(pg, static_cast<size_t>(i), ax, ay)) return false;
        if (!PolyVert(pg, (static_cast<size_t>(i) + 1) % n, bx, by)) return false;
        x = ax + t * (bx - ax);
        y = ay + t * (by - ay);
        return true;
    }
    default:
        return false;
    }
}

// 世界坐标 → 最近路径参数与落点。prev 为该点当前参数（圆 unwrap 用，
// 传 1e18 表示无），保证拖动跨 ±π 分界时参数连续不跳变
bool PathProject(GeoElement *e, double wx, double wy, double prev,
                 double &param, double &x, double &y)
{
    if (!IsPathType(e)) return false;
    switch (e->type()) {
    case GeoType::Segment:
    case GeoType::Vector: {
        double ax, ay, bx, by;
        if (e->type() == GeoType::Segment) {
            GeoSegment *s = static_cast<GeoSegment *>(e);
            ax = s->ex1(); ay = s->ey1(); bx = s->ex2(); by = s->ey2();
        } else {
            GeoVector *v = static_cast<GeoVector *>(e);
            ax = v->ex1(); ay = v->ey1(); bx = v->ex2(); by = v->ey2();
        }
        double dx = bx - ax, dy = by - ay;
        double len2 = dx * dx + dy * dy;
        if (len2 < 1e-18) return false;
        double t = ((wx - ax) * dx + (wy - ay) * dy) / len2;
        t = std::max(0.0, std::min(1.0, t));
        param = t;
        x = ax + t * dx;
        y = ay + t * dy;
        return true;
    }
    case GeoType::Line: {
        double a, b, c;
        if (!LineCoefsOf(e, a, b, c)) return false;
        const double n2 = a * a + b * b;
        // 单位方向 d̂=(-b,a)/√n2，锚点（距原点最近点）A=(ac,bc)/n2；锚点在
        // d̂ 上分量恒 0，故 param = dot(P, d̂) 即有向距离
        param = (-b * wx + a * wy) / sqrt(n2);
        x = a * c / n2 - (b / sqrt(n2)) * param;
        y = b * c / n2 + (a / sqrt(n2)) * param;
        return true;
    }
    case GeoType::Ray: {
        GeoRay *r = static_cast<GeoRay *>(e);
        double dx = r->p2->x - r->p1->x, dy = r->p2->y - r->p1->y;
        double len2 = dx * dx + dy * dy;
        if (len2 < 1e-18) return false;
        double t = ((wx - r->p1->x) * dx + (wy - r->p1->y) * dy) / len2;
        t = std::max(0.0, t);
        param = t * sqrt(len2);   // 参数统一为有向距离
        x = r->p1->x + t * dx;
        y = r->p1->y + t * dy;
        return true;
    }
    case GeoType::Circle: {
        GeoCircle *c = static_cast<GeoCircle *>(e);
        if (c->radius < 1e-15) return false;
        double th = atan2(wy - c->ccy(), wx - c->ccx());
        if (th < 0) th += 2.0 * M_PI;
        if (prev < 1e17) {
            // unwrap：取与 prev 同圈最近的等价角
            th += 2.0 * M_PI * round((prev - th) / (2.0 * M_PI));
        }
        param = th;
        x = c->ccx() + c->radius * cos(th);
        y = c->ccy() + c->radius * sin(th);
        return true;
    }
    case GeoType::Function: {
        GeoFunction *f = static_cast<GeoFunction *>(e);
        double yv = f->evalAt(wx);
        if (!std::isfinite(yv)) return false;
        param = wx;
        x = wx;
        y = yv;
        return true;
    }
    case GeoType::Polygon: {
        GeoPolygon *pg = static_cast<GeoPolygon *>(e);
        const size_t n = pg->image ? pg->ivx.size() : pg->verts.size();
        if (n == 0) return false;
        double bestD = 1e18;
        size_t bestI = 0;
        double bestT = 0;
        for (size_t i = 0; i < n; ++i) {
            double ax, ay, bx, by;
            if (!PolyVert(pg, i, ax, ay)) continue;
            if (!PolyVert(pg, (i + 1) % n, bx, by)) continue;
            double dx = bx - ax, dy = by - ay;
            double len2 = dx * dx + dy * dy;
            double t = len2 > 1e-18
                ? ((wx - ax) * dx + (wy - ay) * dy) / len2 : 0.0;
            t = std::max(0.0, std::min(1.0, t));
            double cx = ax + t * dx, cy = ay + t * dy;
            double d = (wx - cx) * (wx - cx) + (wy - cy) * (wy - cy);
            if (d < bestD) {
                bestD = d;
                bestI = i;
                bestT = t;
            }
        }
        param = static_cast<double>(bestI) + bestT;
        double px, py;
        if (!PathPointAt(e, param, px, py)) return false;
        x = px;
        y = py;
        return true;
    }
    default:
        return false;
    }
}

} // namespace

// ---------- 输入解析 ----------

// 撤销打点在包装层：成功创建才留一步（失败输入不留）；setXml 重放与
// redefine 的内部 input（undoSuspend_ > 0）不打点
// 1.0.6.6 关系符号 ∥/⊥ 的方向向量活取（calc 闭包内调用——线类几何可变，
// 创建时缓存方向会失真）。直线 ax+by=c 方向 (-b,a)（verticalConst 同式）；
// 线段/射线/向量取端点差（image 缓存感知）；函数限一次式（斜率 k → (1,k)），
// 非线性图像无方向返回 false
static bool RelationDir(GeoElement *g, double &dx, double &dy)
{
    if (auto *l = dynamic_cast<GeoLine *>(g)) {
        dx = -l->b;
        dy = l->a;
        return true;
    }
    if (auto *s = dynamic_cast<GeoSegment *>(g)) {
        dx = s->ex2() - s->ex1();
        dy = s->ey2() - s->ey1();
        return true;
    }
    if (auto *r = dynamic_cast<GeoRay *>(g)) {
        if (!r->p1 || !r->p2) return false;
        dx = r->p2->x - r->p1->x;
        dy = r->p2->y - r->p1->y;
        return true;
    }
    if (auto *v = dynamic_cast<GeoVector *>(g)) {
        dx = v->ex2() - v->ex1();
        dy = v->ey2() - v->ey1();
        return true;
    }
    if (auto *f = dynamic_cast<GeoFunction *>(g)) {
        double k = 0, m = 0;
        if (!f->body.linearCoef(k, m)) return false;
        dx = 1;
        dy = k;
        return true;
    }
    return false;
}

// 1.0.6.6 弧度会话缩放判定：角对象（GeoAngle）与文件重建的角度值数值
//（isAngleVal）在表达式层出弧度值；普通数值不缩放
static bool IsAngleValue(const GeoElement *g)
{
    const GeoNumeric *n = dynamic_cast<const GeoNumeric *>(g);
    return n != nullptr && n->isAngleVal;
}

// —— 1.0.7.2 ∧/∨ 组合区域（文本级 DNF）——
// 顶层比较符扫描（引号外；≤≥ 按 E2 89 A4/A5 归并为带 eq 的 < >）。
// 1.0.7.1 链式切段与组合区域原子切段共用
struct TextCmpSeg {
    size_t pos;   // 算符起始字节
    char op;      // '<' / '>'
    bool eq;      // ≤/≥/<=/>=
    size_t len;   // 算符字节数
};

static std::vector<TextCmpSeg> ScanTextCmp(const std::string &s)
{
    std::vector<TextCmpSeg> segs;
    bool inStr = false;
    for (size_t i = 0; i < s.size();) {
        const char c = s[i];
        if (c == '"') { inStr = !inStr; ++i; continue; }
        if (!inStr && (unsigned char)c == 0xE2 && i + 2 < s.size()
            && (unsigned char)s[i + 1] == 0x89) {
            // Unicode ≤（E2 89 A4）/ ≥（E2 89 A5）：键盘键字符，闭边界
            const unsigned char tail = (unsigned char)s[i + 2];
            if (tail == 0xA4 || tail == 0xA5) {
                segs.push_back({ i, tail == 0xA4 ? '<' : '>', true, 3 });
                i += 3;
                continue;
            }
        }
        if (inStr || (c != '<' && c != '>')) { ++i; continue; }
        const bool eq = i + 1 < s.size() && s[i + 1] == '=';
        segs.push_back({ i, c, eq, eq ? (size_t)2 : (size_t)1 });
        i += eq ? 2 : 1;
    }
    return segs;
}

// 顶层逻辑连词扫描（引号外、括号深度 0）：∧（E2 88 A7 / & / &&）、
// ∨（E2 88 A8 / ||）。单 | 非连词（绝对值条不受扰），单 & 同 ∧
struct TextConnTok { size_t pos; bool isOr; size_t len; };

static std::vector<TextConnTok> ScanTextConn(const std::string &s)
{
    std::vector<TextConnTok> toks;
    bool inStr = false;
    int depth = 0;
    for (size_t i = 0; i < s.size();) {
        const char c = s[i];
        if (c == '"') { inStr = !inStr; ++i; continue; }
        if (!inStr && (c == '(' || c == '[' || c == '{')) { ++depth; ++i; continue; }
        if (!inStr && (c == ')' || c == ']' || c == '}')) { if (depth > 0) --depth; ++i; continue; }
        if (inStr || depth != 0) { ++i; continue; }
        if ((unsigned char)c == 0xE2 && i + 2 < s.size() && (unsigned char)s[i + 1] == 0x88
            && ((unsigned char)s[i + 2] == 0xA7 || (unsigned char)s[i + 2] == 0xA8)) {
            toks.push_back({ i, (unsigned char)s[i + 2] == 0xA8, 3 });
            i += 3;
            continue;
        }
        if (c == '&') {
            const bool two = i + 1 < s.size() && s[i + 1] == '&';
            toks.push_back({ i, false, two ? (size_t)2 : (size_t)1 });
            i += two ? 2 : 1;
            continue;
        }
        if (c == '|' && i + 1 < s.size() && s[i + 1] == '|') {
            toks.push_back({ i, true, 2 });
            i += 2;
            continue;
        }
        ++i;
    }
    return toks;
}

// 整体括号剥壳：'(x>1)∧(x<5)' 原子、'((1<x<3))' 链式外壳（引号感知；
// 顺带去两端空白）
static void StripOuterParens(std::string &t)
{
    for (;;) {
        const size_t b = t.find_first_not_of(" \t");
        if (b == std::string::npos) return;
        const size_t e = t.find_last_not_of(" \t");
        const std::string s = t.substr(b, e - b + 1);
        if (s.size() < 2 || s.front() != '(' || s.back() != ')') { t = s; return; }
        int d = 0;
        bool wrapped = false;
        bool inQ = false;
        for (size_t i = 0; i < s.size(); ++i) {
            const char c = s[i];
            if (c == '"') inQ = !inQ;
            else if (!inQ && c == '(') ++d;
            else if (!inQ && c == ')') { --d; if (d == 0) { wrapped = i == s.size() - 1; break; } }
        }
        if (!wrapped) { t = s; return; }
        t = s.substr(1, s.size() - 2);
    }
}

// 组合式规范化到 DNF：句内条件 AND（链式切段同构），句间 OR。∨ 优先级
// 低于 ∧（1.0.6.4 语义）→ 先按顶层 ∨ 分句、句内按顶层 ∧ 摊平、原子剥壳
// 后按比较段建条件；∧∨ 交叉嵌套按分配律展开（句数封顶 32 防失控）。
// false 时 err 给出文案
static bool NormIneqDnf(const std::string &t0,
                        std::vector<std::vector<GeoFunction::IneqCond>> &cls,
                        int depth, std::string &err)
{
    if (depth > 12) { err = "组合区域嵌套过深"; return false; }
    std::string t = t0;
    StripOuterParens(t);
    if (t.empty()) { err = "组合区域条件为空"; return false; }
    const std::vector<TextConnTok> toks = ScanTextConn(t);
    auto splitTop = [&](bool wantOr) {
        std::vector<std::string> parts;
        size_t start = 0;
        bool any = false;
        for (const TextConnTok &tk : toks) {
            if (tk.isOr != wantOr) continue;
            any = true;
            parts.push_back(t.substr(start, tk.pos - start));
            start = tk.pos + tk.len;
        }
        if (any) parts.push_back(t.substr(start));
        return parts;
    };
    const std::vector<std::string> orParts = splitTop(true);
    if (orParts.size() >= 2) {
        for (const std::string &p : orParts) {
            std::vector<std::vector<GeoFunction::IneqCond>> sub;
            if (!NormIneqDnf(p, sub, depth + 1, err)) return false;
            cls.insert(cls.end(), std::make_move_iterator(sub.begin()),
                       std::make_move_iterator(sub.end()));
        }
        return true;
    }
    const std::vector<std::string> andParts = splitTop(false);
    if (andParts.size() >= 2) {
        std::vector<std::vector<GeoFunction::IneqCond>> acc(1);
        for (const std::string &p : andParts) {
            std::vector<std::vector<GeoFunction::IneqCond>> sub;
            if (!NormIneqDnf(p, sub, depth + 1, err)) return false;
            std::vector<std::vector<GeoFunction::IneqCond>> next;
            for (std::vector<GeoFunction::IneqCond> &c1 : acc) {
                for (std::vector<GeoFunction::IneqCond> &c2 : sub) {
                    std::vector<GeoFunction::IneqCond> m = c1;
                    m.insert(m.end(), c2.begin(), c2.end());
                    next.push_back(std::move(m));
                }
            }
            if (next.size() > 32) { err = "组合区域过复杂（∧∨ 嵌套分配）"; return false; }
            acc = std::move(next);
        }
        cls = std::move(acc);
        return true;
    }
    // 原子：比较式（含链式切段）——所有比较段条件并入一条句
    const std::vector<TextCmpSeg> segs = ScanTextCmp(t);
    if (segs.empty()) {
        err = "组合区域条件须为比较式（如 x>1∧x<5、x<0∨x>1）";
        return false;
    }
    std::vector<std::string> operands;
    size_t start = 0;
    for (const TextCmpSeg &sg : segs) {
        operands.push_back(t.substr(start, sg.pos - start));
        start = sg.pos + sg.len;
    }
    operands.push_back(t.substr(start));
    std::vector<GeoFunction::IneqCond> clause;
    for (size_t k = 0; k + 1 < operands.size(); ++k) {
        Expr el, er;
        std::string e1, e2;
        if (!el.parse(operands[k], e1, std::string(), true)
            || !er.parse(operands[k + 1], e2, std::string(), true)
            || el.isBoolLike() || er.isBoolLike()) {
            err = "组合区域条件须为比较式（如 x>1∧x<5、x<0∨x>1）";
            return false;
        }
        GeoFunction::IneqCond c;
        c.l = std::move(el);
        c.r = std::move(er);
        c.lt = segs[k].op == '<';
        c.strict = !segs[k].eq;
        clause.push_back(std::move(c));
    }
    cls.push_back(std::move(clause));
    return true;
}

// 组合/链式区域符号函数 g（<0 在区域内）：句内条件归一化取 max（AND），
// 句间取 min（OR，并集）。max/min 连续，g 的符号翻转即区域边界——两变量
// 列扫描渲染机制不变。ineqChain 单句（1.0.7.1 链式）同构特例。引用对象
//（滑条）按 resolver 现值解析，随拖动联动
static double IneqComboG(const GeoFunction &f, double wx, double wy,
                         const std::function<double(const std::string &)> &resolver)
{
    auto condVal = [&](const GeoFunction::IneqCond &c) -> double {
        const double s = c.l.evalXYResolved(wx, wy, resolver)
                       - c.r.evalXYResolved(wx, wy, resolver);
        return c.lt ? s : -s;
    };
    if (f.ineqDNF.empty()) {
        double g = NAN;
        for (const GeoFunction::IneqCond &c : f.ineqChain) {
            const double s = condVal(c);
            if (!std::isfinite(s)) return NAN;
            g = std::isnan(g) ? s : std::max(g, s);
        }
        return g;
    }
    double g = NAN;
    for (const std::vector<GeoFunction::IneqCond> &cl : f.ineqDNF) {
        double h = NAN;
        bool fin = true;
        for (const GeoFunction::IneqCond &c : cl) {
            const double s = condVal(c);
            if (!std::isfinite(s)) { fin = false; break; }
            h = std::isnan(h) ? s : std::max(h, s);
        }
        if (!fin) continue;   // 未定义句按不满足处理（OR 下不吞噬其余句）
        g = std::isnan(g) ? h : std::min(g, h);
    }
    return g;
}

bool Kernel::input(const std::string &srcRaw)
{
    if (undoSuspend_ > 0) return inputImpl(srcRaw);
    const std::string pre = getXml();
    if (!inputImpl(srcRaw)) return false;
    commitUndoSnapshot(pre);
    return true;
}

// 表格单元格地址：单个大写字母 + 纯数字（A1、B12）。排除 L 开头（列表
// 命名空间 L1,L2…）。轮D 反馈③：依随公式里这类未知引用自动按空单元格
//（0 值）物化，填值后经重定义联动（上游 Spreadsheet 语义）。
static bool IsCellLabel(const std::string &s)
{
    if (s.size() < 2 || s[0] < 'A' || s[0] > 'Z' || s[0] == 'L') return false;
    for (size_t i = 1; i < s.size(); ++i) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    return true;
}

// 1.0.6.2 矩阵算术：exprVal 求值的引用回填——数值依赖读 value，矩阵依赖
// 拷贝 values/维度（GeoList 行主序与 ExprVal 同布局）。查不到返回 false
static bool FillExprVal(const std::string &nm, ExprVal &o,
                        const std::vector<std::pair<std::string, GeoNumeric *>> &deps,
                        const std::vector<std::pair<std::string, GeoList *>> &mats)
{
    for (const auto &d : deps) {
        if (d.first == nm) {
            o = ExprVal{};
            o.num = d.second->value;
            return true;
        }
    }
    for (const auto &m : mats) {
        if (m.first == nm) {
            o = ExprVal{};
            o.isMat = true;
            o.rows = m.second->mrows;
            o.cols = m.second->mcols;
            o.vals = m.second->values;
            return true;
        }
    }
    return false;
}

// 1.0.6.3 列表/图表命令：解析 {…} 内部为数值列表（元素为不含 x 的有限
// 数值表达式）。返回 false 时 err 已填。1.5b 平面列表分支与本 helper 共用
static bool ParseListValues(const std::string &inner, std::vector<double> &out,
                            std::string &err)
{
    size_t start = 0;
    for (;;) {
        const size_t comma = inner.find(',', start);
        const std::string part = inner.substr(start,
            comma == std::string::npos ? std::string::npos : comma - start);
        const std::string t = Trim(part);
        if (!t.empty()) {
            Expr e;
            std::string perr;
            if (!e.parse(t, perr)) {
                err = perr.empty() ? "列表元素无法解析: " + t : perr;
                return false;
            }
            if (e.containsX()) {
                err = "列表元素不能含 x: " + t;
                return false;
            }
            const double v = e.eval(0.0);
            if (!std::isfinite(v)) {
                err = "列表元素不是有限数: " + t;
                return false;
            }
            out.push_back(v);
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return true;
}

// 1.0.6.3 Flatten 字面量实参：递归展开嵌套 {…}（矩阵 {{a,b},…}、混合嵌套
// {1,{2,3}} 均可，GGB 语义）为扁平数值列表。顶层按花括号深度感知的逗号
// 分段，叶子段按数值表达式求值（规则同 ParseListValues）
static bool ParseFlattenLiteral(const std::string &s, std::vector<double> &out,
                                std::string &err)
{
    if (s.size() < 2 || s.front() != '{' || s.back() != '}') {
        err = "Flatten 实参须为列表: " + s;
        return false;
    }
    const std::string inner = s.substr(1, s.size() - 2);
    std::vector<std::string> parts;
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i <= inner.size(); ++i) {
        if (i == inner.size() || inner[i] == ',') {
            if (depth == 0) {
                parts.push_back(inner.substr(start, i - start));
                start = i + 1;
            }
        } else if (inner[i] == '{') {
            ++depth;
        } else if (inner[i] == '}') {
            --depth;
        }
    }
    if (depth != 0) { err = "列表字面量格式：{a,b,…}"; return false; }
    for (const std::string &p : parts) {
        const std::string t = Trim(p);
        if (t.empty()) continue;
        if (t.front() == '{') {
            if (!ParseFlattenLiteral(t, out, err)) return false;
            continue;
        }
        Expr e;
        std::string perr;
        if (!e.parse(t, perr)) {
            err = perr.empty() ? "列表元素无法解析: " + t : perr;
            return false;
        }
        if (e.containsX()) {
            err = "列表元素不能含 x: " + t;
            return false;
        }
        const double v = e.eval(0.0);
        if (!std::isfinite(v)) {
            err = "列表元素不是有限数: " + t;
            return false;
        }
        out.push_back(v);
    }
    if (out.empty()) { err = "列表不能为空"; return false; }
    return true;
}

// 1.0.6.3 动态数值实参：表达式树 + 引用名→数值对象表（Take 起止序号、
// Sequence 端点步长等）。evalNow() 按对象当前值求值——闭包捕获本结构即
// 获得依随重算；字面量实参树为常数、refs 为空
struct NumRefExpr {
    Expr tree;
    std::vector<std::pair<std::string, GeoNumeric *>> refs;
    double evalNow() const
    {
        return tree.evalResolved(0.0, [this](const std::string &nm) -> double {
            for (const auto &r : refs) {
                if (r.first == nm) return r.second->value;
            }
            return NAN;
        });
    }
};

// 识别形式："(u,v)" 点 / "A=(u,v)" / 命令 / "f(x)=表达式" / 纯表达式(按函数画)
bool Kernel::inputImpl(const std::string &srcRaw)
{
    autoCells_ = 0;   // 本次输入经 refs 分支自动物化的空单元格数
    // 键盘线性形式预解析（第七包）：$A$1→A1、循环小数、带分数（带分数
    // 规则依赖空白，必须先于去空白执行；引号串内不受影响）
    const std::string pre = PreprocessInput(srcRaw);
    std::string src;
    src.reserve(pre.size());
    {
        // 引号串内（Text 的文本内容）保留空白
        bool inStr = false;
        for (char c : pre) {
            if (c == '"') inStr = !inStr;
            else if (isspace((unsigned char)c) && !inStr) continue;
            src.push_back(c);
        }
    }
    if (src.empty()) return false;
    // 上游 ans 变量：独立标识符替换为上一答案字面量
    src = ReplaceAns(src);
    lastError.clear();

    // 上游 快捷建点 "X(u,v)"：大写开头标识 + 两常量参数 ⇒ 改写为 "X=(u,v)"
    // 走普通赋值路径（自动获得标签冲突检测与重定义语义）
    {
        size_t po = src.find('(');
        if (po != std::string::npos && po > 0 && src.back() == ')'
            && src.find('=') == std::string::npos) {
            std::string head = src.substr(0, po);
            bool headIsName = isupper((unsigned char)head[0]) != 0;
            for (char c : head) {
                if (!isalnum((unsigned char)c)) { headIsName = false; break; }
            }
            // 命令名不是点标签：Take(5,1) 这类命令误用不该被改写成建点
            if (headIsName && IsCommandCallName(head)) headIsName = false;
            if (headIsName) {
                std::string inner = src.substr(po + 1, src.size() - po - 2);
                size_t cm = inner.find(',');
                if (cm != std::string::npos) {
                    Expr ex, ey;
                    std::string e1, e2;
                    std::string sx = inner.substr(0, cm), sy = inner.substr(cm + 1);
                    if (ex.parse(sx, e1) && ey.parse(sy, e2)
                        && !ex.containsX() && !ey.containsX()) {
                        src = head + "=(" + sx + "," + sy + ")";
                    }
                }
            }
        }
    }

    // 拆 "name=..."（批次⑤：'=' 前是 < > ! = 的属于比较符，不参与赋值分割，
    // 否则 "y<=2x" 会被拆成 lhs="y<" 的伪赋值；1.0.6.4：'==' 的第一个等号
    // 也不拆——否则 "x==3" 拆出 rhs="=3"）
    std::string name, rhs;
    size_t eq = std::string::npos;
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] != '=') continue;
        if (i > 0) {
            const char p = src[i - 1];
            if (p == '<' || p == '>' || p == '!' || p == '=') continue;
        }
        if (i + 1 < src.size() && src[i + 1] == '=') continue;   // "==" 首等号
        eq = i;
        break;
    }
    bool hasAssign = false;
    if (eq != std::string::npos) {
        std::string lhs = src.substr(0, eq);
        bool lhsIsName = !lhs.empty();
        for (size_t ci = 0; ci < lhs.size();) {
            if (IsGreekAt(lhs, ci)) { ci += 2; continue; }   // 希腊标识符（第八包）
            const char c = lhs[ci];
            if (!isalnum((unsigned char)c) && c != '_' && c != '(' && c != ')') { lhsIsName = false; break; }
            ++ci;
        }
        if (lhsIsName) {
            hasAssign = true;
            name = lhs;
            rhs = src.substr(eq + 1);
        }
    }
    if (!hasAssign) rhs = src;

    // f(x)= 形式的左部先归一化掉 "(x)"（与函数分支同规则）——必须在下方占用
    // 检查之前：否则查 "f(x)" 落空，f 已存在时会新建重复标签对象而非重定义
    if (hasAssign && name.size() > 3 && name.compare(name.size() - 3, 3, "(x)") == 0) {
        name.resize(name.size() - 3);
    }
    // ans 是求值历史保留名（上游 同款），禁止占用为标签
    if (hasAssign && name == "ans") {
        lastError = "ans 是保留名（上一答案），请换一个标签";
        return false;
    }

    // 标签占用检查：无依赖者的自由对象允许重定义（删旧建新，上游 redefine 语义），
    // 有依赖者或依随对象则报错引导先删除
    if (hasAssign && !name.empty()) {
        GeoElement *old = lookup(name);
        if (old) {
            bool hasDeps = false;
            for (auto &g : geos_) {
                for (GeoElement *in : g->inputs) {
                    if (in == old) { hasDeps = true; break; }
                }
                if (hasDeps) break;
            }
            GeoPoint *op = dynamic_cast<GeoPoint *>(old);
            GeoNumeric *on = dynamic_cast<GeoNumeric *>(old);
            GeoList *ol = dynamic_cast<GeoList *>(old);
            bool freeObj = (op && op->isFree && !op->calc) || (on && on->isFree && !on->calc)
                || dynamic_cast<GeoFunction *>(old) != nullptr
                || (ol != nullptr && ol->cmdName.empty())
                // 轮 D：依随表达式数值（B1=A1+1）无依赖者时允许输入行重建
                //（上游 单元格重输语义；重定义入口本就放行）
                || (on && on->calc && on->cmdName.empty())
                // 1.0.6.1：矩阵依随产物（Transpose/Invert 等）同理放行——
                // 上游 M={{…}} 重输即重建，Transpose(M) 的依赖者一并级联
                || (ol != nullptr && ol->calc);
            if (!hasDeps && freeObj) {
                for (auto it = geos_.begin(); it != geos_.end(); ++it) {
                    if (it->get() == old) { geos_.erase(it); break; }
                }
            } else {
                lastError = "标签 " + name + " 已存在且被依赖，请先删除它或换个名字";
                return false;
            }
        }
    }

    std::unique_ptr<GeoElement> made;

    // 1) 命令分发（Name(args)，大小写不敏感）
    size_t pOpen = rhs.find('(');
    if (pOpen != std::string::npos && rhs.back() == ')') {
        std::string cmd = rhs.substr(0, pOpen);
        bool cmdIsName = !cmd.empty() && isalpha((unsigned char)cmd[0]);
        for (char c : cmd) {
            if (!isalnum((unsigned char)c) && c != '_') { cmdIsName = false; break; }
        }
        // f(x)= 这类左部已在 hasAssign 处理，rhs 不含 "(x)" 前缀；
        // "(x)" 检查避开引号区（Text("(x)") 的文本内容）
        size_t qx = rhs.find("(x)");
        size_t q1 = rhs.find('"');
        if (cmdIsName && (qx == std::string::npos || (q1 != std::string::npos && q1 < qx))) {
            std::string args = rhs.substr(pOpen + 1, rhs.size() - pOpen - 2);
            if (tryCommand(ToLower(cmd), args, name, made)) {
                if (!made) {
                    if (lastError.empty()) return true;   // 无产物命令（StartAnimation 等）
                    return false;   // 命令识别但执行失败（lastError 已填）
                }
            } else if (!lastError.empty()) {
                return false;
            }
        }
    }

    // 1.7) 不等式（N6 批次⑤）：y<expr / y<=expr / x>常数（含两边互换与
    // kx+m<c 线性解）→ 半平面/曲线下侧区域。引号内比较符不参与（Text 等）。
    // 1.0.7.1 链式：顶层比较段全收集——≥2 段按 a op1 b op2 c 官方语义建
    // AND 区域（各段条件取交），单段走既有路径
    if (!made) {
        const std::vector<TextCmpSeg> cmpSegs = ScanTextCmp(rhs);
        // 链式/组合切段用剥壳视图（'((1<x<3))'、'(x>1)∧(x<5)' 类外壳不参与
        // 切段；单段路径仍按原文坐标切）；src 恒存原文
        std::string core = rhs;
        StripOuterParens(core);
        const std::vector<TextCmpSeg> coreCmp = ScanTextCmp(core);
        const std::vector<TextConnTok> connToks = ScanTextConn(core);
        const bool chained = coreCmp.size() >= 2;
        const size_t cpos = cmpSegs.empty() ? std::string::npos : cmpSegs[0].pos;
        const char cop = cmpSegs.empty() ? 0 : cmpSegs[0].op;
        const bool cEq = cmpSegs.empty() ? false : cmpSegs[0].eq;
        const size_t opLen = cmpSegs.empty() ? 0 : cmpSegs[0].len;
        // 1.0.6.4：不等式分支只接管「顶层就是比较且含 x/y」的区域式
        //（y<x^2、x>3、x^2+y^2<9）。其余整体可解析的（布尔对象式
        // b=(2>1)&&(3>2)、If(...) 分段函数、无 x/y 的 b=a>2）放行给后方
        // 函数/数值路径——按首个比较符硬切会切碎 If 与复合布尔式
        bool isRegion = cpos != std::string::npos;
        if (isRegion) {
            Expr whole;
            std::string werr;
            if (whole.parse(rhs, werr, std::string(), true)
                && (!whole.isBoolLike() || (!whole.containsX() && !whole.containsY()))) {
                isRegion = false;
            }
        }
        if (isRegion && !connToks.empty()) {
            // 1.0.7.2 ∧/∨ 组合区域：文本级 DNF——句内 AND（链式同构）、句间
            // OR（并集）。单句退化写 ineqChain（下游零改动），多句写 ineqDNF
            //（渲染 g=min(句内 max)<0，列扫描机制不变）
            auto f2 = std::make_unique<GeoFunction>();
            f2->src = rhs;
            std::vector<std::vector<GeoFunction::IneqCond>> cls;
            std::string nerr;
            if (!NormIneqDnf(rhs, cls, 0, nerr)) {
                lastError = nerr;
                return false;
            }
            if (cls.size() == 1) f2->ineqChain = std::move(cls[0]);
            else f2->ineqDNF = std::move(cls);
            made = std::move(f2);
        } else if (isRegion && chained) {
            // 1.0.7.1 链式不等式 a op1 b op2 c：官方语义 = (a op1 b)∧(b op2 c)
            // 的 AND 区域。每段归一化为 s=L−R 符号条件（lt=true 区域在 s<0 侧），
            // 渲染 g=max(归一化值)<0 走两变量列扫描。操作数须为纯算术式；
            // refs 模式解析（滑条引用 a<x<b 随拖动联动，渲染端按对象现值求值）
            auto f2 = std::make_unique<GeoFunction>();
            f2->src = rhs;
            size_t start = 0;
            std::vector<std::string> operands;
            for (const TextCmpSeg &sg : coreCmp) {
                operands.push_back(core.substr(start, sg.pos - start));
                start = sg.pos + sg.len;
            }
            operands.push_back(core.substr(start));
            bool ok = operands.size() == cmpSegs.size() + 1;
            for (size_t k = 0; ok && k + 1 < operands.size(); ++k) {
                Expr el, er;
                std::string e1, e2;
                if (!el.parse(operands[k], e1, std::string(), true)
                    || !er.parse(operands[k + 1], e2, std::string(), true)
                    || el.isBoolLike() || er.isBoolLike()) {
                    ok = false;
                    break;
                }
                GeoFunction::IneqCond c;
                c.l = std::move(el);
                c.r = std::move(er);
                c.lt = cmpSegs[k].op == '<';
                c.strict = !cmpSegs[k].eq;
                f2->ineqChain.push_back(std::move(c));
            }
            if (!ok) {
                lastError = "暂不支持链式不等式（各段须为算术式，如 a<x<b）";
                return false;
            }
            made = std::move(f2);
        } else if (isRegion) {
            auto flip = [](int o) { return o == 1 ? 3 : o == 2 ? 4 : o == 3 ? 1 : 2; };
            const std::string lhsS = rhs.substr(0, cpos);
            const std::string rhsS = rhs.substr(cpos + opLen);
            int op = cop == '<' ? (cEq ? 2 : 1) : (cEq ? 4 : 3);
            std::string exprSrc;
            std::string perr;
            Expr body;
            bool vert = false;
            double vx = 0;
            // 批次⑥ 两变量区域构建：L op R（任一侧含 y；含 y 的式子不能走
            // 单变量路径——那里 y 会被静默按 0 求值画错）
            auto buildTwoVar = [&](const std::string &L, const std::string &R, int op2) -> bool {
                Expr el2, er2;
                std::string e3, e4;
                if (!el2.parse(L, e3) || !er2.parse(R, e4)) return false;
                if (!el2.containsY() && !er2.containsY()) return false;
                auto f2 = std::make_unique<GeoFunction>();
                f2->src = rhs;
                f2->ineqOp = op2;
                f2->ineqTwoVar = true;
                f2->ineqL = std::move(el2);
                f2->ineqR = std::move(er2);
                made = std::move(f2);
                return true;
            };
            if (lhsS == "y" && rhsS != "y" && !rhsS.empty()) {
                exprSrc = rhsS;
                if (!body.parse(exprSrc, perr)) { lastError = perr; return false; }
                if (body.containsY() && !buildTwoVar(lhsS, rhsS, op)) {
                    lastError = "无法解析不等式";
                    return false;
                }
            } else if (rhsS == "y" && lhsS != "y" && !lhsS.empty()) {
                exprSrc = lhsS;
                op = flip(op);   // expr op y ⇒ y flip(op) expr
                if (!body.parse(exprSrc, perr)) { lastError = perr; return false; }
                if (body.containsY() && !buildTwoVar(lhsS, rhsS, op)) {
                    lastError = "无法解析不等式";
                    return false;
                }
            } else {
                // x 线性式 vs 常数：x>3 / 2x<6 / 3<=x / -x>3
                Expr el, er;
                std::string e1, e2;
                const bool pl = el.parse(lhsS, e1), pr = er.parse(rhsS, e2);
                double k = 0, m = 0, cst = 0;
                bool got = false;
                if (pl && pr) {
                    auto constOf = [](const Expr &e, double &out) {
                        return !e.containsX() && !e.containsY() && !e.empty()
                            && std::isfinite(out = e.eval(0.0));
                    };
                    if (!el.containsX() && er.containsX() && er.linearCoef(k, m) && constOf(el, cst)) {
                        op = flip(op);   // 常数 op kx+m ⇒ x 方向翻转
                        got = true;
                    } else if (el.containsX() && !er.containsX() && el.linearCoef(k, m) && constOf(er, cst)) {
                        got = true;      // kx+m op 常数
                    }
                }
                if ((!got || fabs(k) < 1e-12)
                    && !buildTwoVar(lhsS, rhsS, op)) {
                    lastError = "不等式请写成 y<表达式、x>常数 或含 y 的两变量式（如 x^2+y^2<9）";
                    return false;
                }
                if (!made) {
                    vx = (cst - m) / k;
                    vert = true;
                    if (k < 0) op = flip(op);   // 负斜率再翻一次
                }
            }
            if (!made) {
                auto f = std::make_unique<GeoFunction>();
                f->src = rhs;
                f->ineqOp = op;
                f->ineqVertical = vert;
                f->ineqX = vx;
                f->body = std::move(body);
                made = std::move(f);
            }
        }
    }

    // 1.7) 关系符号 ∥ ⊥ ∈ ⊂ ⊆（1.0.6.6 键盘死区激活）：顶层出现即按语义
    // 建依随布尔对象，不进表达式层（线类/列表对象不是 double）。只认顶层
    //（括号/花括号/引号内维持原解析路径）；≅ ≇ → ⊗ 官方输入条亦无运算
    // 语义，维持插入后报错
    if (!made && !rhs.empty()) {
        size_t rsPos = std::string::npos;
        int rsKind = 0;   // 1=∥ 2=⊥ 3=∈ 4=⊂ 5=⊆
        {
            int depth = 0;
            bool inStr = false;
            for (size_t i = 0; i < rhs.size();) {
                const unsigned char c = (unsigned char)rhs[i];
                if (inStr) {
                    if (c == '"') inStr = false;
                    ++i;
                    continue;
                }
                if (c == '"') { inStr = true; ++i; continue; }
                if (c == '(' || c == '{') { ++depth; ++i; continue; }
                if (c == ')' || c == '}') {
                    if (depth > 0) --depth;
                    ++i;
                    continue;
                }
                if (c == 0xE2 && depth == 0 && i + 2 < rhs.size()) {
                    const unsigned char b1 = (unsigned char)rhs[i + 1];
                    const unsigned char b2 = (unsigned char)rhs[i + 2];
                    int k = 0;
                    if (b1 == 0x88 && b2 == 0xA5) k = 1;          // ∥ U+2225
                    else if (b1 == 0x8A && b2 == 0xA5) k = 2;     // ⊥ U+22A5
                    else if (b1 == 0x88 && b2 == 0x88) k = 3;     // ∈ U+2208
                    else if (b1 == 0x8A && b2 == 0x82) k = 4;     // ⊂ U+2282
                    else if (b1 == 0x8A && b2 == 0x86) k = 5;     // ⊆ U+2286
                    if (k) { rsPos = i; rsKind = k; break; }
                }
                ++i;
            }
        }
        if (rsKind) {
            const std::string lop = Trim(rhs.substr(0, rsPos));
            const std::string rop = Trim(rhs.substr(rsPos + 3));
            if (lop.empty() || rop.empty()) {
                lastError = "关系符号需要左右两个对象";
                return false;
            }
            auto n = std::make_unique<GeoNumeric>();
            n->boolLike = true;
            n->isFree = false;
            n->cmd = rhs;   // 定义列/保存重放原文（依随数值 cmd 为 rhs 惯例）
            // 列表实参：{…} 字面量拍快照（值拷贝进闭包）/标签活引用（挂 inputs）
            auto listOperand = [&](const std::string &s, std::vector<double> &lit,
                                   GeoList *&live) -> bool {
                live = nullptr;
                if (!s.empty() && s.front() == '{') {
                    if (s.back() != '}') { lastError = "列表字面量格式：{a,b,…}"; return false; }
                    std::string lerr;
                    if (!ParseListValues(s.substr(1, s.size() - 2), lit, lerr)) {
                        lastError = lerr.empty() ? "列表字面量无法解析: " + s : lerr;
                        return false;
                    }
                    return true;
                }
                GeoList *L = dynamic_cast<GeoList *>(lookup(s));
                if (!L || L->isMatrix) {
                    lastError = "需要数值列表（标签或 {…} 字面量）: " + s;
                    return false;
                }
                live = L;
                return true;
            };
            if (rsKind == 1 || rsKind == 2) {
                // 线类平行/垂直判定：夹角归一 |叉积|或|点积| / |d1||d2| < 1e-9
                GeoElement *ga = lookup(lop);
                GeoElement *gb = lookup(rop);
                double t1x, t1y, t2x, t2y;
                if (!ga || !gb || !RelationDir(ga, t1x, t1y)
                    || !RelationDir(gb, t2x, t2y)) {
                    lastError = std::string(rsKind == 1 ? "∥" : "⊥")
                        + " 需要两个线类对象（直线/线段/射线/向量/一次函数图像）";
                    return false;
                }
                const bool perp = rsKind == 2;
                n->calc = [ga, gb, perp]() -> double {
                    double d1x, d1y, d2x, d2y;
                    if (!RelationDir(ga, d1x, d1y) || !RelationDir(gb, d2x, d2y)) return 0.0;
                    const double m1 = sqrt(d1x * d1x + d1y * d1y);
                    const double m2 = sqrt(d2x * d2x + d2y * d2y);
                    if (m1 < 1e-12 || m2 < 1e-12) return 0.0;
                    const double q = perp ? fabs(d1x * d2x + d1y * d2y) / (m1 * m2)
                                          : fabs(d1x * d2y - d1y * d2x) / (m1 * m2);
                    return q < 1e-9 ? 1.0 : 0.0;
                };
                n->inputs = { ga, gb };
                n->update();
                made = std::move(n);
            } else if (rsKind == 3) {
                // x ∈ L：左侧数值表达式（可含数值对象引用，依随重算）；弧度
                // 会话角引用出弧度值（与赋值分支同规）
                Expr e;
                std::string err;
                if (!e.parse(lop, err, std::string(), true) || e.containsX()
                    || e.containsY()) {
                    lastError = err.empty() ? "∈ 左侧须为数值表达式: " + lop : err;
                    return false;
                }
                std::vector<std::pair<std::string, GeoNumeric *>> deps;
                for (const std::string &rn : e.refs()) {
                    GeoNumeric *dn = dynamic_cast<GeoNumeric *>(lookup(rn));
                    if (!dn) { lastError = "∈ 左侧引用须为数值对象: " + rn; return false; }
                    deps.emplace_back(rn, dn);
                }
                std::vector<double> lit;
                GeoList *L = nullptr;
                if (!listOperand(rop, lit, L)) return false;
                const double angScale = angleRadian ? M_PI / 180.0 : 1.0;
                n->calc = [e, deps, lit, L, angScale]() -> double {
                    const double v = e.evalResolved(0.0,
                        [deps, angScale](const std::string &nm) -> double {
                            for (const auto &d : deps) {
                                if (d.first == nm) {
                                    return d.second->value
                                        * (IsAngleValue(d.second) ? angScale : 1.0);
                                }
                            }
                            return NAN;
                        });
                    const std::vector<double> &vals = L ? L->values : lit;
                    for (double x : vals) {
                        if (x == v) return 1.0;   // 精确相等（NAN 恒不中）
                    }
                    return 0.0;
                };
                for (const auto &d : deps) n->inputs.push_back(d.second);
                if (L) n->inputs.push_back(L);
                n->update();
                made = std::move(n);
            } else {
                // L1 ⊂/⊆ L2：集合语义（去重后逐元素包含；⊂ 另要求真子集）
                std::vector<double> litA, litB;
                GeoList *LA = nullptr, *LB = nullptr;
                if (!listOperand(lop, litA, LA) || !listOperand(rop, litB, LB)) return false;
                const bool strict = rsKind == 4;
                n->calc = [litA, litB, LA, LB, strict]() -> double {
                    const std::vector<double> &A = LA ? LA->values : litA;
                    const std::vector<double> &B = LB ? LB->values : litB;
                    auto distinct = [](const std::vector<double> &v) {
                        std::vector<double> d;
                        for (double x : v) {
                            bool seen = false;
                            for (double y : d) {
                                if (y == x) { seen = true; break; }
                            }
                            if (!seen) d.push_back(x);
                        }
                        return d;
                    };
                    const std::vector<double> da = distinct(A);
                    const std::vector<double> db = distinct(B);
                    for (double x : da) {
                        bool in = false;
                        for (double y : db) {
                            if (y == x) { in = true; break; }
                        }
                        if (!in) return 0.0;
                    }
                    return (!strict || da.size() < db.size()) ? 1.0 : 0.0;
                };
                if (LA) n->inputs.push_back(LA);
                if (LB) n->inputs.push_back(LB);
                n->update();
                made = std::move(n);
            }
        }
    }

    // 1.5) 列表：{...} 或 L={...}（N6 批次③；元素为不含 x 的数值表达式）。
    // 1.0.6.1：{{...},{...}} 嵌套形式按矩阵解析（GGB 矩阵 = 列表的列表，
    // 行主序扁平进 values，isMatrix/mrows/mcols 落位；行错位即拒）
    if (!made && !rhs.empty() && rhs.front() == '{' && rhs.back() == '}'
        && (name.empty() || name.find('(') == std::string::npos)) {
        const std::string inner = rhs.substr(1, rhs.size() - 2);
        auto L = std::make_unique<GeoList>();
        L->src = "{" + inner + "}";
        // 嵌套判定：非引号区首个 '{' 在末尾之前（{{1,2},{3,4}} vs {1,{2}} 中
        // 混合形式也按矩阵处理——上游 混合即未定义，我们直接拒绝）
        {
            size_t firstBrace = std::string::npos;
            for (size_t bi = 0; bi < inner.size(); ++bi) {
                if (inner[bi] == '{') { firstBrace = bi; break; }
            }
            if (firstBrace != std::string::npos) {
                // 矩阵字面量：逐行拆 {..}，行内逐元素求值
                int rows = 0, cols = -1;
                bool ok = true;
                size_t pos = 0;
                while (pos < inner.size() && ok) {
                    const size_t rb = inner.find('{', pos);
                    if (rb == std::string::npos) break;
                    const size_t re = inner.find('}', rb);
                    if (re == std::string::npos) {
                        lastError = "矩阵行缺少右花括号";
                        ok = false;
                        break;
                    }
                    const std::string row = inner.substr(rb + 1, re - rb - 1);
                    int cnt = 0;
                    size_t rs = 0;
                    for (;;) {
                        const size_t rc = row.find(',', rs);
                        const std::string part = row.substr(rs,
                            rc == std::string::npos ? std::string::npos : rc - rs);
                        const std::string t = Trim(part);
                        if (!t.empty()) {
                            Expr e;
                            std::string err;
                            if (!e.parse(t, err)) {
                                lastError = err.empty() ? "矩阵元素无法解析: " + t : err;
                                ok = false;
                            } else if (e.containsX() || e.containsY()) {
                                lastError = "矩阵元素不能含变量: " + t;
                                ok = false;
                            } else {
                                const double v = e.eval(0.0);
                                if (!std::isfinite(v)) {
                                    lastError = "矩阵元素不是有限数: " + t;
                                    ok = false;
                                } else {
                                    L->values.push_back(v);
                                    ++cnt;
                                }
                            }
                        }
                        if (rc == std::string::npos || !ok) break;
                        rs = rc + 1;
                    }
                    if (ok) {
                        if (cols < 0) cols = cnt;
                        else if (cnt != cols) {
                            lastError = "矩阵各行元素个数不一致";
                            ok = false;
                        }
                    }
                    if (!ok) break;
                    ++rows;
                    pos = re + 1;
                }
                if (!ok) return false;
                // 行间残余文本（如 "{1,2}x{3,4}"）：只在花括号/逗号/空白之外
                // 扫描，数字与运算符是行内容的一部分，不在此判
                {
                    int depth = 0;
                    for (size_t ci = 0; ci < inner.size(); ++ci) {
                        const char c = inner[ci];
                        if (c == '{') ++depth;
                        else if (c == '}') --depth;
                        else if (depth == 0 && c != ',' && !isspace((unsigned char)c)) {
                            lastError = "矩阵字面量格式：{{a,b},…}";
                            return false;
                        }
                    }
                }
                if (rows <= 0 || cols <= 0) { lastError = "矩阵不能为空"; return false; }
                L->isMatrix = true;
                L->mrows = rows;
                L->mcols = cols;
                made = std::move(L);
            }
        }
    }

    // 1.5b) 平面列表：{...}（元素为不含 x 的数值表达式）
    if (!made && !rhs.empty() && rhs.front() == '{' && rhs.back() == '}'
        && (name.empty() || name.find('(') == std::string::npos)) {
        const std::string inner = rhs.substr(1, rhs.size() - 2);
        auto L = std::make_unique<GeoList>();
        L->src = "{" + inner + "}";
        std::string lerr;
        if (!ParseListValues(inner, L->values, lerr)) {
            lastError = lerr;
            return false;
        }
        if (L->values.empty()) { lastError = "列表不能为空"; return false; }
        made = std::move(L);
    }

    // 2) 点： "(u,v)" 或 "A=(u,v)"。无逗号/解析不完整时放行后续分支——
    // 1.0.6.4 布尔式可整体带括号（(2>1)&&(3>2)），不是点
    if (!made && !rhs.empty() && rhs.front() == '(' && rhs.back() == ')') {
        std::string inner = rhs.substr(1, rhs.size() - 2);
        size_t comma = inner.find(',');
        if (comma != std::string::npos) {
            std::string sx = inner.substr(0, comma), sy = inner.substr(comma + 1);
            std::string e1, e2;
            Expr ex, ey;
            if (ex.parse(sx, e1) && ey.parse(sy, e2) && sy.find(',') == std::string::npos) {
                if (ex.containsX() || ey.containsX()) { lastError = "点坐标不能含 x"; return false; }
                auto p = std::make_unique<GeoPoint>();
                p->x = ex.eval(0.0);
                p->y = ey.eval(0.0);
                if (!std::isfinite(p->x) || !std::isfinite(p->y)) { lastError = "坐标不是有限数"; return false; }
                made = std::move(p);
            }
        }
    }

    // 3) 函数：y=expr / f(x)=expr / 纯表达式（按函数画）
    if (!made) {
        std::string err;
        Expr body;
        std::string norm = rhs;
        std::string lhsName = name;
        if (hasAssign) {
            size_t p2 = lhsName.find("(x)");
            if (p2 != std::string::npos) {
                lhsName = lhsName.substr(0, p2);
            } else if (lhsName == "y") {
                lhsName.clear();   // y=expr 也按函数处理，不占用户名
            } else {
                // 纯数值赋值 a=5：建数值对象。轮 D：refs 模式解析（未知标识符
                // 记为对象引用）——引用全部为数值类（数值/滑动条/角）时建
                // "依随表达式"数值（B1=A1+1，inputs 注册依赖，拖点/滑杆/
                // 重定义级联重算）；引用不可解析（不存在/非数值/函数定义
                // h(x)=zz+1 混入）时置 made 为空回落旧流程，保持原错误文案
                Expr t;
                if (t.parse(norm, err, std::string(), true) && !t.containsX()
                    && !t.containsY()) {
                    const std::vector<std::string> &refs = t.refs();
                    std::vector<std::pair<std::string, GeoNumeric *>> deps;
                    std::vector<std::pair<std::string, GeoList *>> matDeps;
                    bool unresolvable = false;
                    for (const std::string &rn : refs) {
                        GeoElement *ge = lookup(rn);
                        GeoNumeric *dn = dynamic_cast<GeoNumeric *>(ge);
                        if (dn == nullptr) {
                            // 1.0.6.2：矩阵引用参与表达式算术（A+B、2A、AB、
                            // A/2、-A、A^k）——收为矩阵依赖，定型走 evalVal
                            GeoList *ml = dynamic_cast<GeoList *>(ge);
                            if (ml != nullptr && ml->isMatrix) {
                                matDeps.emplace_back(rn, ml);
                                continue;
                            }
                            // 表格空单元格引用（对象完全不存在时）：自动建
                            // 0 值数值占位（进构造列表与 undo 快照），用户填
                            // 值走重定义联动。已存在的非数值对象（如该地址
                            // 放了点）不物化，照旧回落报错；自引用亦然
                            if (ge == nullptr && IsCellLabel(rn) && rn != name) {
                                auto cell = std::make_unique<GeoNumeric>();
                                cell->value = 0;
                                GeoNumeric *raw = static_cast<GeoNumeric *>(
                                    add(std::move(cell), rn));
                                deps.emplace_back(rn, raw);
                                ++autoCells_;
                                continue;
                            }
                            unresolvable = true;
                            break;
                        }
                        deps.emplace_back(rn, dn);
                    }
                    if (unresolvable && autoCells_ > 0) {
                        // 回滚本输入已物化的占位格（均为最后压入的元素）
                        while (autoCells_ > 0) {
                            geos_.pop_back();
                            --autoCells_;
                        }
                    }
                    if (!unresolvable && !matDeps.empty()) {
                        // 矩阵算术（1.0.6.2）：一次性求值定型（结果须为矩阵；
                        // 不成立给运算规则文案）。src 记原式，getXml 按
                        // exp="C=A+B" 表达式重放往返；calc 闭包重走 evalVal
                        ExprVal v0;
                        auto res0 = [&](const std::string &nm, ExprVal &o) -> bool {
                            return FillExprVal(nm, o, deps, matDeps);
                        };
                        if (!t.evalVal(res0, v0) || !v0.isMat) {
                            while (autoCells_ > 0) {
                                geos_.pop_back();
                                --autoCells_;
                            }
                            lastError = "矩阵运算不支持该组合（同型矩阵可 + −；"
                                        "矩阵可乘矩阵、乘除标量、取非负整数次幂）";
                            return false;
                        }
                        auto out = std::make_unique<GeoList>();
                        out->isMatrix = true;
                        out->mrows = v0.rows;
                        out->mcols = v0.cols;
                        out->values = v0.vals;
                        out->src = norm;
                        out->calc = [t, deps, matDeps, rows0 = v0.rows, cols0 = v0.cols](
                                        GeoList *g) {
                            ExprVal v;
                            auto res = [&](const std::string &nm, ExprVal &o) -> bool {
                                return FillExprVal(nm, o, deps, matDeps);
                            };
                            if (!t.evalVal(res, v) || !v.isMat) {
                                // 依随重算失效（依赖改维等）：保持外形填 NaN
                                g->isMatrix = true;
                                g->mrows = rows0;
                                g->mcols = cols0;
                                g->values.assign((size_t)rows0 * cols0, NAN);
                                return;
                            }
                            g->isMatrix = true;
                            g->mrows = v.rows;
                            g->mcols = v.cols;
                            g->values = v.vals;
                        };
                        for (const auto &d : deps) {
                            out->inputs.push_back(d.second);
                        }
                        for (const auto &m : matDeps) {
                            out->inputs.push_back(m.second);
                        }
                        out->update();
                        made = std::move(out);
                    } else if (!unresolvable) {
                    auto n = std::make_unique<GeoNumeric>();
                    // 1.0.6.4 布尔对象：定义式顶层为比较/逻辑 → 值列 true/false
                    n->boolLike = t.isBoolLike();
                    if (deps.empty()) {
                        n->value = t.eval(0.0);
                    } else {
                        // calc 闭包捕获表达式树 + 依赖指针（inputs 已注册，
                        // 级联删除保证不悬垂；重定义保持指针身份）
                        // 1.0.6.6 弧度会话：表达式层的角对象引用出弧度值
                        //（×π/180）——radian 文件 sin(β)=1、β/2=π/4 与内核
                        // 裸数弧度三角一致；度制会话出内部度数不变。比例
                        // 创建时定型进闭包（官方 文件模式即会话模式）
                        const double angScale =
                            angleRadian ? M_PI / 180.0 : 1.0;
                        n->isFree = false;
                        n->calc = [t, deps, angScale]() -> double {
                            return t.evalResolved(0.0,
                                [deps, angScale](const std::string &nm) -> double {
                                    for (const auto &d : deps) {
                                        if (d.first == nm) {
                                            return d.second->value
                                                * (IsAngleValue(d.second) ? angScale : 1.0);
                                        }
                                    }
                                    return NAN;
                                });
                        };
                        for (const auto &d : deps) {
                            n->inputs.push_back(d.second);
                        }
                        n->update();
                    }
                    n->cmd = norm;
                    made = std::move(n);
                    }
                }
            }
        } else {
            size_t px = src.find("(x)=");
            if (px != std::string::npos && px > 0) {
                std::string lhs = src.substr(0, px);
                bool lhsIsName = !lhs.empty();
                for (size_t ci = 0; ci < lhs.size();) {
                    if (IsGreekAt(lhs, ci)) { ci += 2; continue; }
                    const char c = lhs[ci];
                    if (!isalnum((unsigned char)c) && c != '_') { lhsIsName = false; break; }
                    ++ci;
                }
                if (lhsIsName) {
                    lhsName = lhs;
                    norm = src.substr(px + 4);
                }
            }
        }
        if (!made) {
            if (!body.parse(norm, err)) {
                lastError = err;
                return false;
            }
            if (body.containsY()) {
                // y 已是第二变量（批次⑥）：单变量函数路径静默按 y=0 求值会画错，
                // 显式拒绝并引导到两变量不等式
                lastError = "函数暂不支持 y，两变量请用不等式（如 x^2+y^2<9）";
                return false;
            }
            if (!body.containsX()) {
                lastError = "表达式不含 x，暂不绘制";
                return false;
            }
            auto f = std::make_unique<GeoFunction>();
            f->src = norm;
            f->body = std::move(body);
            made = std::move(f);
            if (hasAssign && name.find('(') != std::string::npos) name = lhsName;
        }
    }

    if (!made) {
        if (lastError.empty()) lastError = "无法解析输入";
        return false;
    }
    GeoElement *added = add(std::move(made), name);
    // ans 回填（上游：代数输入的数值结果更新上一答案）
    if (GeoNumeric *gn = dynamic_cast<GeoNumeric *>(added)) {
        if (std::isfinite(gn->value)) SetAnsValue(gn->value);
    }
    flushExtras();
    return true;
}

// ---------- 命令分发 ----------

// Simpson 1/3 定积分（N6 Integral/函数分析共用；段数为偶数）
static double Simpson(const GeoFunction *f, double a, double b, int n)
{
    if (n % 2 != 0) ++n;
    double h = (b - a) / n;
    if (h == 0) return 0;
    double s = f->evalAt(a) + f->evalAt(b);
    for (int i = 1; i < n; ++i) {
        double y = f->evalAt(a + i * h);
        s += (i % 2 == 0 ? 2.0 : 4.0) * y;
    }
    return s * h / 3.0;
}

// ---------- 1.0.6.4 概率分布（GGB 5.4 语义） ----------
// Normal(μ,σ,x)=P(X≤x)（4 参带 true/false 切 PDF）、InverseNormal=分位数；
// BinomialDist/Poisson 带累积布尔；InverseBinomial/InversePoisson=最小 k。

// 标准正态 CDF Φ(z) = ½·erfc(−z/√2)（erfc 为 C++11 <cmath> 标准函数）
static double NormCdfStd(double z)
{
    return 0.5 * erfc(-z / 1.41421356237309504880);
}

static double NormPdf(double mu, double sd, double x)
{
    if (!(sd > 0)) return NAN;
    const double z = (x - mu) / sd;
    return exp(-0.5 * z * z) / (sd * 2.50662827463100050242);
}

static double NormCdf(double mu, double sd, double x)
{
    if (!(sd > 0)) return NAN;
    return NormCdfStd((x - mu) / sd);
}

// 正态分位数（Acklam 逼近 + 一次 Halley 精化，全 double 精度）
static double NormQuantileStd(double p)
{
    if (!(p > 0.0 && p < 1.0)) return NAN;
    static const double a[] = { -3.969683028665376e+01, 2.209460984245205e+02,
        -2.759285104469687e+02, 1.383577518672690e+02, -3.066479806614716e+01,
        2.506628277459239e+00 };
    static const double b[] = { -5.447609879822406e+01, 1.615858368580409e+02,
        -1.556989798598866e+02, 6.680131188771972e+01, -1.328068155288572e+01 };
    static const double c[] = { -7.784894002430293e-03, -3.223964580411365e-01,
        -2.400758277161838e+00, -2.549732539343734e+00, 4.374664141464968e+00,
        2.938163982698783e+00 };
    static const double d[] = { 7.784695709041462e-03, 3.224671290700398e-01,
        2.445134137142996e+00, 3.754408661907416e+00 };
    const double pl = 0.02425;
    double q, r;
    if (p < pl) {
        q = sqrt(-2.0 * log(p));
        return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
             / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    if (p > 1.0 - pl) {
        q = sqrt(-2.0 * log(1.0 - p));
        return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
                / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    q = p - 0.5;
    r = q * q;
    return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q
         / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
}

static double NormQuantile(double mu, double sd, double p)
{
    if (!(sd > 0) || !(p >= 0.0 && p <= 1.0)) return NAN;
    if (p == 0.0) return -HUGE_VAL;
    if (p == 1.0) return HUGE_VAL;
    double x = mu + sd * NormQuantileStd(p);
    // Halley 精化两步（erfc 高精度）
    for (int it = 0; it < 2; ++it) {
        const double z = (x - mu) / sd;
        const double e = NormCdfStd(z) - p;
        const double u = e * 2.50662827463100050242 * exp(0.5 * z * z);
        x -= sd * u / (1.0 + z * u / 2.0);
    }
    return x;
}

// ln C(n,k)（lgamma 路径，大 n 不溢出）
static double LChoose(double n, double k)
{
    return lgamma(n + 1.0) - lgamma(k + 1.0) - lgamma(n - k + 1.0);
}

static double BinomPoint(double n, double p, double k)
{
    if (k < 0 || k > n) return 0.0;
    if (p <= 0.0) return k == 0 ? 1.0 : 0.0;
    if (p >= 1.0) return k == n ? 1.0 : 0.0;
    return exp(LChoose(n, k) + k * log(p) + (n - k) * log1p(-p));
}

static double BinomCdf(double n, double p, double k)
{
    if (k < 0) return 0.0;
    if (k >= n) return 1.0;
    if (p <= 0.0) return 1.0;
    if (p >= 1.0) return 0.0;
    const long long kk = (long long)floor(k);
    // 数值稳定：k < n/2 从 0 累加，否则从顶端累减
    if (kk <= (long long)(n / 2)) {
        double acc = 0.0;
        for (long long i = 0; i <= kk; ++i) acc += BinomPoint(n, p, (double)i);
        return std::min(1.0, acc);
    }
    double acc = 0.0;
    for (long long i = kk + 1; i <= (long long)n; ++i) acc += BinomPoint(n, p, (double)i);
    return std::max(0.0, 1.0 - acc);
}

static double PoisPoint(double lam, double k)
{
    if (k < 0 || lam < 0) return 0.0;
    if (lam == 0.0) return k == 0 ? 1.0 : 0.0;
    return exp(-lam + k * log(lam) - lgamma(k + 1.0));
}

static double PoisCdf(double lam, double k)
{
    if (k < 0) return 0.0;
    if (lam == 0.0) return 1.0;
    const long long kk = (long long)floor(k);
    double acc = 0.0;
    for (long long i = 0; i <= kk; ++i) {
        acc += PoisPoint(lam, (double)i);
        if (acc >= 1.0) return 1.0;   // 大 λ 截断
    }
    return std::min(1.0, acc);
}

// 列表统计（上游 语义）：mean / median / min / max / sum / product /
// sd、variance（样本口径，n-1）。kind 用小写命令名。
static double ListStat(const std::vector<double> &v, const std::string &kind)
{
    if (v.empty()) return NAN;
    if (kind == "min" || kind == "max") {
        double best = v.front();
        for (double x : v) {
            if (kind == "min" ? x < best : x > best) best = x;
        }
        return best;
    }
    if (kind == "sum" || kind == "product") {
        double acc = kind == "sum" ? 0.0 : 1.0;
        for (double x : v) acc = kind == "sum" ? acc + x : acc * x;
        return acc;
    }
    double sum = 0;
    for (double x : v) sum += x;
    const double mean = sum / (double)v.size();
    if (kind == "mean") return mean;
    if (kind == "variance" || kind == "sd") {
        if (v.size() < 2) return 0.0;
        double ss = 0;
        for (double x : v) ss += (x - mean) * (x - mean);
        const double var = ss / (double)(v.size() - 1);
        return kind == "variance" ? var : sqrt(var);
    }
    // median
    std::vector<double> s(v);
    std::sort(s.begin(), s.end());
    const size_t m = s.size();
    return (m % 2 == 1) ? s[m / 2] : (s[m / 2 - 1] + s[m / 2]) / 2.0;
}

// 1.0.6.1 线性回归批：正规方程解法（MᵗM·p = Mᵗy，列主元高斯消元）。
// 上游用 QR 分解（commons-math）；N 数据点 ≤ 万级、阶数 ≤ 8 的场景两者
// 结果一致且高斯消元实现小、无外部依赖。奇异（数据点重复 x 且阶数≥去重后
// 点数）返回 false
static bool SolveNormalEquations(std::vector<std::vector<double>> M,
                                 std::vector<double> y, std::vector<double> &p)
{
    const int n = (int)M.size();
    const int w = n > 0 ? (int)M[0].size() : 0;
    if (n == 0 || w == 0) return false;
    // A = MᵗM（w×w 对称），b = Mᵗy
    std::vector<std::vector<double>> A(w, std::vector<double>(w, 0.0));
    std::vector<double> b(w, 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < w; ++j) {
            b[j] += M[i][j] * y[i];
            for (int k = 0; k < w; ++k) A[j][k] += M[i][j] * M[i][k];
        }
    }
    // 列主元高斯消元
    for (int col = 0; col < w; ++col) {
        int piv = col;
        for (int r = col + 1; r < w; ++r) {
            if (fabs(A[r][col]) > fabs(A[piv][col])) piv = r;
        }
        if (fabs(A[piv][col]) < 1e-12) return false;   // 奇异
        if (piv != col) {
            std::swap(A[piv], A[col]);
            std::swap(b[piv], b[col]);
        }
        for (int r = col + 1; r < w; ++r) {
            const double f = A[r][col] / A[col][col];
            if (f == 0.0) continue;
            for (int c2 = col; c2 < w; ++c2) A[r][c2] -= f * A[col][c2];
            b[r] -= f * b[col];
        }
    }
    p.assign(w, 0.0);
    for (int r = w - 1; r >= 0; --r) {
        double s = b[r];
        for (int c2 = r + 1; c2 < w; ++c2) s -= A[r][c2] * p[c2];
        p[r] = s / A[r][r];
        if (!std::isfinite(p[r])) return false;
    }
    return true;
}

// 多项式系数（p[0]=常数项 … p[k]=x^k 项）→ 内核函数源文（"c0 + c1*x + …"）。
// 系数用 FitNum 最短格式（1.0.6.6；%.17g 全精度会把解算噪声上屏，
// %.6g 会在大系数上出科学计数且损失精度）
// 1.0.6.6 拟合系数格式：在 1e-8 相对容差内取最短十进制表示——正规方程/
// 二项式展开的噪声（0.29999999999999999 这类）按 0.3 显示，大系数不出
// 科学计数（%.6g 会），重解析后曲线差异在 1e-8 相对量级，拟合语义不变
static std::string FitNum(double v)
{
    char b[40];
    for (int prec = 1; prec <= 10; ++prec) {
        snprintf(b, sizeof(b), "%.*g", prec, v);
        if (fabs(strtod(b, nullptr) - v) <= 1e-8 * (fabs(v) + 1e-300)) break;
    }
    return std::string(b);
}

static std::string PolyCoeffSrc(const std::vector<double> &p)
{
    std::string s;
    bool first = true;
    for (size_t i = 0; i < p.size(); ++i) {
        if (fabs(p[i]) < 1e-15) continue;
        // 七十三包：恒按绝对值格式化，符号只由前缀给——否则首项负系数
        // 产出 "--0.16…"（数值自带负号 + 前缀再补一个），代数区双负号
        if (!first) s += (p[i] > 0 ? " + " : " - ");
        else if (p[i] < 0) s += "-";
        s += FitNum(fabs(p[i]));
        if (i > 0) {
            s += "*x";
            if (i > 1) s += "^" + std::to_string(i);
        }
        first = false;
    }
    if (first) s = "0";
    return s;
}

// n×n 实矩阵求逆（列主元高斯-约当）。奇异返回 false
static bool InvertMatrix(std::vector<std::vector<double>> A,
                         std::vector<std::vector<double>> &inv)
{
    const int n = (int)A.size();
    if (n == 0) return false;
    for (int i = 0; i < n; ++i) {
        if ((int)A[i].size() != n) return false;
    }
    inv.assign(n, std::vector<double>(n, 0.0));
    for (int i = 0; i < n; ++i) inv[i][i] = 1.0;
    for (int col = 0; col < n; ++col) {
        int piv = col;
        for (int r = col + 1; r < n; ++r) {
            if (fabs(A[r][col]) > fabs(A[piv][col])) piv = r;
        }
        if (fabs(A[piv][col]) < 1e-12) return false;   // 奇异
        if (piv != col) {
            std::swap(A[piv], A[col]);
            std::swap(inv[piv], inv[col]);
        }
        const double d = A[col][col];
        for (int c2 = 0; c2 < n; ++c2) {
            A[col][c2] /= d;
            inv[col][c2] /= d;
        }
        for (int r = 0; r < n; ++r) {
            if (r == col) continue;
            const double f = A[r][col];
            if (f == 0.0) continue;
            for (int c2 = 0; c2 < n; ++c2) {
                A[r][c2] -= f * A[col][c2];
                inv[r][c2] -= f * inv[col][c2];
            }
        }
    }
    return true;
}

// 内核源文 → giac 求值串：mod(A,B) 翻译为 (A-B*floor(A/B))（欧几里得余，
// 与内核 mod 语义一致）。giac 内置 mod 是"设置模运算域"，直接送入不仅
// 输出不可绘制（x%3），还会给 CAS context 留下模域残留
static std::string GiacFromKernelSrc(const std::string &src)
{
    std::string out;
    out.reserve(src.size() + 32);
    size_t i = 0;
    while (i < src.size()) {
        if (src.compare(i, 4, "mod(") == 0 && (i == 0 || !isalpha(static_cast<unsigned char>(src[i - 1])))) {
            int depth = 0;
            size_t j = i, comma = std::string::npos;
            for (; j < src.size(); ++j) {
                char c = src[j];
                if (c == '(') ++depth;
                else if (c == ')') { --depth; if (depth == 0) break; }
                else if (c == ',' && depth == 1 && comma == std::string::npos) comma = j;
            }
            if (j == src.size() || comma == std::string::npos) {
                out += src[i++];
                continue;
            }
            const std::string A = src.substr(i + 4, comma - (i + 4));
            const std::string B = src.substr(comma + 1, j - (comma + 1));
            const std::string ga = GiacFromKernelSrc(A);
            const std::string gb = GiacFromKernelSrc(B);
            out += "((" + ga + ")-(" + gb + ")*floor((" + ga + ")/(" + gb + ")))";
            i = j + 1;
            continue;
        }
        out += src[i++];
    }
    return out;
}

// ---------- Factor 的原生兜底：整系数一元多项式因式分解 ----------
// Factor(f) 真机闪退定位前，整数多项式（用户最常见输入）不再进 giac：
// 整点采样 + 牛顿差分提取精确整系数，再按有理根定理做线性分解（精确整数
// 综合除法）。剩余部分次数 ≤3 且无有理根即不可约，分解完备。失败（非多项
// 式/系数非整/规模过大/剩余次数 ≥4 仍可能分解）返回 false，调用方回落 CAS。

// 整点采样提取整系数：f(0..12) 均为整数时差分表全整数，经斯特林展开成
// 单项式系数（放大 12! 保持整算），再用未采样点复核排除伪多项式（如 sin）
static bool PolyToIntCoeff(const Expr &e, std::vector<long long> &c)
{
    const int maxDeg = 12;
    long long f[maxDeg + 1];
    for (int j = 0; j <= maxDeg; ++j) {
        const double y = e.eval(j);
        const double r = llround(y);
        if (fabs(y - r) > 1e-6 * (1.0 + fabs(y)) || fabs(r) > 1e15) return false;
        f[j] = (long long)r;
    }
    // 原地差分：结束后 d[i] = Δ^i f(0)
    long long d[maxDeg + 1];
    for (int i = 0; i <= maxDeg; ++i) d[i] = f[i];
    for (int i = 1; i <= maxDeg; ++i)
        for (int j = maxDeg; j >= i; --j)
            d[j] -= d[j - 1];
    // 牛顿形式 f(x)=Σ d[i]/i!·(x)_i；放大到公分母 D=12! 后用第一类斯特林数
    // 展开下降阶乘 (x)_i，全程整数运算
    const long long D = 479001600LL;   // 12!
    static long long s[13][13];        // 第一类斯特林数（带符号）
    static bool sInit = false;
    if (!sInit) {
        memset(s, 0, sizeof(s));
        s[0][0] = 1;
        for (int n = 1; n <= 12; ++n)
            for (int k = 1; k <= n; ++k)
                s[n][k] = s[n - 1][k - 1] - (n - 1) * s[n - 1][k];
        sInit = true;
    }
    static const long long factInv[13] = { 479001600, 479001600, 239500800, 79833600,
                                           19958400, 3991680, 665280, 95040, 11880,
                                           1320, 132, 12, 1 };   // D / k!
    long long sum[maxDeg + 1];
    memset(sum, 0, sizeof(sum));
    for (int i = 0; i <= maxDeg; ++i) {
        if (d[i] == 0) continue;
        if (llabs(d[i]) > 1000000000000000LL) return false;
        const __int128 e1 = (__int128)d[i] * factInv[i];
        for (int m = 0; m <= i; ++m) {
            const __int128 t = e1 * s[i][m];
            if (t > (__int128)100000000000000000LL || t < -(__int128)100000000000000000LL)
                return false;
            sum[m] += (long long)t;
        }
    }
    c.clear();
    for (int m = 0; m <= maxDeg; ++m) {
        if (sum[m] % D != 0) return false;   // 非整系数多项式
        c.push_back(sum[m] / D);
    }
    while (c.size() > 1 && c.back() == 0) c.pop_back();
    // 复核：重构多项式在未采样点上的取值必须与原式一致
    const int pts[3] = { -1, -2, 7 };
    for (int q = 0; q < 3; ++q) {
        double y = 0;
        for (int m = (int)c.size() - 1; m >= 0; --m) y = y * pts[q] + (double)c[m];
        const double ref = e.eval(pts[q]);
        if (fabs(y - ref) > 1e-6 * (1.0 + fabs(ref))) return false;
    }
    return true;
}

static long long FactorGcdLL(long long a, long long b)
{
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) {
        const long long t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static void FactorDivisors(long long v, std::vector<long long> &out)
{
    v = v < 0 ? -v : v;
    out.clear();
    if (v == 0) return;
    for (long long i = 1; i * i <= v; ++i) {
        if (v % i == 0) {
            out.push_back(i);
            if (i != v / i) out.push_back(v / i);
        }
    }
}

// (d*x - m) 的打印：d=1 省略系数；m=0 退化为 dx
static std::string FactorLinStr(long long d, long long m)
{
    if (m == 0) return d == 1 ? "x" : std::to_string(d) + "x";
    std::string s = "(";
    s += d == 1 ? "x" : std::to_string(d) + "x";
    if (m > 0) s += "-" + std::to_string(m);
    else s += "+" + std::to_string(-m);
    s += ")";
    return s;
}

// 多项式串（c[k] 为 x^k 系数，最高次在前输出）
static std::string FactorPolyStr(const std::vector<long long> &c)
{
    std::string s;
    for (int k = (int)c.size() - 1; k >= 0; --k) {
        if (c[k] == 0) continue;
        const bool first = s.empty();
        if (!first) s += c[k] > 0 ? "+" : "-";
        else if (c[k] < 0) s += "-";
        const long long a = c[k] < 0 ? -c[k] : c[k];
        if (a != 1 || k == 0) s += std::to_string(a);
        if (k >= 1) {
            s += "x";
            if (k >= 2) s += "^" + std::to_string(k);
        }
    }
    return s.empty() ? "0" : s;
}

static bool NativeFactorPoly(const Expr &body, std::string &out)
{
    std::vector<long long> c;
    if (!PolyToIntCoeff(body, c)) return false;
    // 因子收集：相邻同因子并作乘幂（上游 风格 (x+1)^2）
    std::vector<std::pair<std::string, int>> facs;
    auto addFac = [&facs](const std::string &s) {
        if (!facs.empty() && facs.back().first == s) ++facs.back().second;
        else facs.push_back({ s, 1 });
    };
    // x=0 重根：提出 x 因子
    while (c.size() > 1 && c.front() == 0) {
        c.erase(c.begin());
        addFac("x");
    }
    if ((int)c.size() - 1 >= 1) {
        for (long long v : c)
            if (v > 1000000000LL || v < -1000000000LL) return false;
        std::vector<long long> nums, dens;
        FactorDivisors(c.front(), nums);   // 常数项因子 → 分子候选
        FactorDivisors(c.back(), dens);    // 首项因子 → 分母候选
        if ((long long)nums.size() * (long long)dens.size() > 20000) return false;
        for (long long den : dens) {
            if ((int)c.size() <= 1) break;
            for (long long num0 : nums) {
                if ((int)c.size() <= 1) break;
                for (int sign = 0; sign < 2 && (int)c.size() > 1; ++sign) {
                    const long long num = sign ? -num0 : num0;
                    if (num == 0 || FactorGcdLL(num, den) != 1) continue;
                    // 重根连除：除不动为止
                    for (;;) {
                        const int deg = (int)c.size() - 1;
                        if (deg < 1) break;
                        std::vector<long long> b(deg);
                        bool ok = true;
                        for (int k = deg - 1; k >= 0; --k) {
                            __int128 t = (__int128)c[k + 1];
                            if (k + 1 < deg) t += (__int128)num * b[k + 1];
                            if (t % (__int128)den != 0) {
                                ok = false;
                                break;
                            }
                            b[k] = (long long)(t / (__int128)den);
                        }
                        if (!ok) break;
                        if ((__int128)c[0] + (__int128)num * b[0] != 0) break;
                        c = b;
                        addFac(FactorLinStr(den, num));
                    }
                }
            }
        }
    }
    std::string acc;
    for (const auto &f : facs) {
        if (!acc.empty()) acc += "*";
        acc += f.first;
        if (f.second > 1) acc += "^" + std::to_string(f.second);
    }
    const int remDeg = (int)c.size() - 1;
    // 剩余次数 ≥4 仍可能分解成高次因子之积（如 (x²+1)(x²+2)），交给 CAS；
    // ≤3 次无有理根即不可约，本地分解已完备
    if (remDeg >= 4) return false;
    if (acc.empty()) {
        out = FactorPolyStr(c);   // 不可约（≤3 次无有理根），原式即完全分解
        return true;
    }
    if (remDeg <= 0) {
        const long long k = c.front();
        out = k == 1 ? acc : (k == -1 ? "-" + acc : std::to_string(k) + "*" + acc);
        return true;
    }
    // 剩余因子含加减号必须加括号（(x-1)*(x^2+1) 不能写成 *(x^2+1) 的裸串）
    std::string rem = FactorPolyStr(c);
    bool multi = false;
    for (size_t i = 1; i < rem.size(); ++i) {
        if (rem[i] == '+' || rem[i] == '-') { multi = true; break; }
    }
    if (multi) rem = "(" + rem + ")";
    out = acc + "*" + rem;
    return true;
}

// 命令表：N2 graphing 基础集 + N5 作图核心（求交/变换/多边形/角/切线/滑动条等）。
// 返回 true=命令名命中。args 为逗号分隔原文；name 为用户指定标签（可为空）。
// 命中命令统一回填 cmdName/cmdArgs（.ggb 保存重放 + 代数区定义列）。
bool Kernel::tryCommand(const std::string &cmdLower, const std::string &args,
                        const std::string &name, std::unique_ptr<GeoElement> &made)
{
    // 拆参数（保留原文供数值表达式解析）。深度感知：括号/花括号内的逗号
    // 不切分（Sequence(k^2, k, 1, mod(9,4)) 这类嵌套实参）；引号串内的
    // 逗号也不切（Text("a, b") 的文本内容）
    std::vector<std::string> a;
    if (!args.empty()) {
        size_t start = 0;
        int depth = 0;
        bool inStr = false;
        for (size_t i = 0; i < args.size(); ++i) {
            const char c = args[i];
            if (inStr) {
                if (c == '"') inStr = false;
                continue;
            }
            if (c == '"') {
                inStr = true;
                continue;
            }
            if (c == '(' || c == '{') ++depth;
            else if (c == ')' || c == '}') --depth;
            else if (c == ',' && depth == 0) {
                a.push_back(Trim(args.substr(start, i - start)));
                start = i + 1;
            }
        }
        a.push_back(Trim(args.substr(start)));
    }
    auto needPts = [&](size_t n) -> std::vector<GeoPoint *> {
        std::vector<GeoPoint *> pts;
        for (size_t i = 0; i < n && i < a.size(); ++i) {
            GeoPoint *p = lookupPoint(a[i]);
            if (!p) {
                lastError = "未找到点 " + a[i];
                return {};
            }
            pts.push_back(p);
        }
        return pts;
    };
    // 参数求值：数值对象引用或字面量表达式
    auto numArg = [&](const std::string &s, double &out) -> bool {
        if (GeoNumeric *n = dynamic_cast<GeoNumeric *>(lookup(s))) {
            out = n->value;
            return true;
        }
        Expr e;
        std::string err;
        if (!e.parse(s, err) || e.containsX()) return false;
        out = e.eval(0.0);
        return std::isfinite(out);
    };

    // ---------- 1.0.6.3 列表/图表命令共用实参解析 ----------
    // 字面量列表临时对象（命令实参允许 {1,2,3} 直接书写；不入构造列表，
    // 含字面量实参的产物为静态——闭包不得捕获这些指针）
    std::vector<std::unique_ptr<GeoList>> litLists;
    // 实参取列表：'{' 开头按字面量解析进 litLists，否则按标签查表（矩阵拒）
    auto listArg = [&](const std::string &s, bool &lit) -> GeoList * {
        lit = false;
        if (!s.empty() && s.front() == '{') {
            if (s.back() != '}') { lastError = "列表字面量格式：{a,b,…}"; return nullptr; }
            lit = true;
            auto t = std::make_unique<GeoList>();
            std::string err;
            if (!ParseListValues(s.substr(1, s.size() - 2), t->values, err)) {
                lastError = err.empty() ? "列表字面量无法解析: " + s : err;
                return nullptr;
            }
            litLists.push_back(std::move(t));
            return litLists.back().get();
        }
        GeoList *L = dynamic_cast<GeoList *>(lookup(s));
        if (!L) { lastError = "未找到列表 " + s; return nullptr; }
        if (L->isMatrix) { lastError = "需要数值列表（矩阵不可）"; return nullptr; }
        return L;
    };
    // 数值实参 → NumRefExpr（可含数值对象引用，进 deps 去重收集）。
    // 解析失败/含未知引用返回 false（lastError 不动，由调用方给文案）
    auto numRefArg = [&](const std::string &s, NumRefExpr &out,
                         std::vector<GeoNumeric *> &deps) -> bool {
        Expr e;
        std::string err;
        if (!e.parse(s, err, "", true)) return false;
        if (e.containsX() || e.containsY()) return false;
        for (const std::string &rn : e.refs()) {
            GeoNumeric *dn = dynamic_cast<GeoNumeric *>(lookup(rn));
            if (!dn) return false;
            bool seen = false;
            for (GeoNumeric *d : deps) {
                if (d == dn) { seen = true; break; }
            }
            if (!seen) deps.push_back(dn);
            out.refs.push_back({ rn, dn });
        }
        out.tree = e;
        return std::isfinite(out.evalNow());
    };

    if (cmdLower == "midpoint" && a.size() == 2) {
        auto pts = needPts(2);
        if (pts.empty()) { made = nullptr; return true; }
        auto mp = std::make_unique<GeoPoint>();
        mp->isFree = false;
        mp->inputs = { pts[0], pts[1] };
        mp->pointSize = 4;   // 上游 DEFAULT_POINT_SIZE_DEPENDENT
        mp->color = {0x64, 0x64, 0x64};
        mp->cmdName = "Midpoint";
        mp->cmdArgs = { a[0], a[1] };
        mp->update();
        made = std::move(mp);
        return true;
    }
    // 四十五包 Point(path[, param])：路径上取点。无参形式参数 0.5（.ggb
    // 回放后由 element <coords> 重投影恢复原位，见 xmlApplyStyle）；param
    // 语义随路径（线段 t / 圆 rad / 函数 x / 多边形 边+t / 线类有向距离）
    if (cmdLower == "point" && (a.size() == 1 || a.size() == 2)) {
        GeoElement *path = lookup(a[0]);
        if (!IsPathType(path)) {
            lastError = "Point 需要路径参数（线段/直线/射线/向量/圆/多边形/函数）";
            made = nullptr;
            return true;
        }
        double param = 0.5;
        if (a.size() == 2 && !numArg(a[1], param)) {
            lastError = "Point 第二参数须为数值";
            made = nullptr;
            return true;
        }
        auto pp = std::make_unique<GeoPoint>();
        pp->path = path;
        pp->pathParam = param;
        pp->isFree = false;
        pp->pointSize = 4;
        pp->inputs = { path };
        pp->calc = [](GeoPoint *q) {
            double x = 0, y = 0;
            if (PathPointAt(q->path, q->pathParam, x, y)) {
                q->x = x;
                q->y = y;
            }
        };
        pp->cmdName = "Point";
        pp->cmdArgs = { a[0] };
        pp->update();
        made = std::move(pp);
        return true;
    }
    if ((cmdLower == "line" || cmdLower == "segment") && a.size() == 2) {
        auto pts = needPts(2);
        if (!pts.empty()) {
            if (cmdLower == "segment") {
                auto s = std::make_unique<GeoSegment>();
                s->p1 = pts[0];
                s->p2 = pts[1];
                s->inputs = { pts[0], pts[1] };
                s->cmdName = "Segment";
                s->cmdArgs = { a[0], a[1] };
                made = std::move(s);
            } else {
                auto l = std::make_unique<GeoLine>();
                l->mode = GeoLine::Mode::TwoPoints;
                l->p1 = pts[0];
                l->p2 = pts[1];
                l->inputs = { pts[0], pts[1] };
                l->cmdName = "Line";
                l->cmdArgs = { a[0], a[1] };
                l->update();
                made = std::move(l);
            }
            return true;
        }
        // 未命中两点：上游 语义 Line(点, 直线/线段/一次函数) = 过点平行线
        if (cmdLower == "line") {
            GeoPoint *pt = lookupPoint(a[0]);
            GeoElement *base = lookup(a[1]);
            GeoFunction *fn = dynamic_cast<GeoFunction *>(base);
            bool ok = pt && base && base->type() != GeoType::Point
                && (dynamic_cast<GeoLine *>(base) || dynamic_cast<GeoSegment *>(base)
                    || (fn && fn->body.linearCoef()));
            if (!ok) {
                lastError = lastError.empty() ? "Line 需要（点, 点）或（点, 直线/线段/一次函数）" : lastError;
                made = nullptr;
                return true;
            }
            lastError.clear();
            auto l = std::make_unique<GeoLine>();
            l->mode = GeoLine::Mode::ParallelThrough;
            l->pt = pt;
            l->base = base;
            l->inputs = { pt, base };
            l->cmdName = "Line";
            l->cmdArgs = { a[0], a[1] };
            l->update();
            made = std::move(l);
            return true;
        }
        // 上游 语义 Segment(点, 长度) = 从点出发沿 x 正方向的定长线段（长可为
        // 字面量或数值对象——滑动条驱动，随其重算；走 image 缓存）
        if (cmdLower == "segment") {
            GeoPoint *pt = lookupPoint(a[0]);
            GeoNumeric *rn = dynamic_cast<GeoNumeric *>(lookup(a[1]));
            double lit = 0;
            bool litOk = false;
            if (!rn) {
                Expr re;
                std::string err;
                if (re.parse(a[1], err) && !re.containsX()) {
                    lit = re.eval(0.0);
                    litOk = std::isfinite(lit) && lit > 0;
                }
            }
            if (!pt || (!rn && !litOk)) {
                lastError = lastError.empty()
                    ? "Segment 需要（点, 点）或（点, 长度）" : lastError;
                made = nullptr;
                return true;
            }
            lastError.clear();
            auto s = std::make_unique<GeoSegment>();
            s->image = true;
            s->cmdName = "Segment";
            s->cmdArgs = { a[0], a[1] };
            if (rn) {
                GeoPoint *A = pt;
                GeoNumeric *R = rn;
                s->calc = [A, R](GeoSegment *g) {
                    g->ix1 = A->x;
                    g->iy1 = A->y;
                    g->ix2 = A->x + R->value;
                    g->iy2 = A->y;
                };
                s->inputs = { pt, rn };
            } else {
                GeoPoint *A = pt;
                double R = lit;
                s->calc = [A, R](GeoSegment *g) {
                    g->ix1 = A->x;
                    g->iy1 = A->y;
                    g->ix2 = A->x + R;
                    g->iy2 = A->y;
                };
                s->inputs = { pt };
            }
            s->update();
            made = std::move(s);
            return true;
        }
        made = nullptr;
        return true;
    }
    if (cmdLower == "circle" && a.size() == 2) {
        GeoPoint *center = lookupPoint(a[0]);
        if (!center) { lastError = "未找到点 " + a[0]; made = nullptr; return true; }
        auto c = std::make_unique<GeoCircle>();
        c->center = center;
        if (GeoPoint *through = lookupPoint(a[1])) {
            c->through = through;
            c->inputs = { center, through };
        } else if (GeoNumeric *rn = dynamic_cast<GeoNumeric *>(lookup(a[1]))) {
            // 半径引用数值对象（滑动条/动画驱动，随其重算）
            c->radiusNum = rn;
            c->inputs = { center, rn };
        } else {
            Expr re;
            std::string err;
            if (!re.parse(a[1], err) || re.containsX()) {
                lastError = "半径不是数: " + a[1];
                made = nullptr;
                return true;
            }
            c->radius = re.eval(0.0);
            if (!(c->radius > 0)) { lastError = "半径必须为正数"; made = nullptr; return true; }
            c->inputs = { center };
        }
        c->cmdName = "Circle";
        c->cmdArgs = { a[0], a[1] };
        c->update();
        made = std::move(c);
        return true;
    }
    if (cmdLower == "perpendicularline" && a.size() == 2) {
        // PerpendicularLine(点, 基准) — 上游 参数序：点在前，基准（直线/线段/
        // 一次函数图像）在后。先按点,基准解；若第一个参数不是点再试反序（容错）。
        size_t pi = 0, bi = 1;
        GeoPoint *pt = lookupPoint(a[0]);
        GeoElement *baseArg = lookup(a[1]);
        if (!pt) {
            pt = lookupPoint(a[1]);
            baseArg = lookup(a[0]);
            pi = 1;
            bi = 0;
        }
        GeoLine *baseLine = dynamic_cast<GeoLine *>(baseArg);
        GeoSegment *baseSeg = dynamic_cast<GeoSegment *>(baseArg);
        GeoFunction *baseFn = dynamic_cast<GeoFunction *>(baseArg);
        bool fnLinear = baseFn && baseFn->body.linearCoef();
        if (!pt || (!baseLine && !baseSeg && !fnLinear)) {
            if (baseFn && !fnLinear) {
                lastError = "PerpendicularLine 的函数基准须为一次式（如 f(x)=2x+1）";
            } else {
                lastError = "PerpendicularLine 需要（点, 直线/线段/一次函数）且对象须已存在";
            }
            made = nullptr;
            return true;
        }
        auto l = std::make_unique<GeoLine>();
        l->mode = GeoLine::Mode::PerpThrough;
        l->pt = pt;
        l->base = baseArg;
        l->inputs = { pt, baseArg };
        l->cmdName = "PerpendicularLine";
        l->cmdArgs = { a[pi], a[bi] };
        l->update();
        if (fabs(l->a) < 1e-15 && fabs(l->b) < 1e-15) {
            lastError = "基准对象无法确定方向";
            made = nullptr;
            return true;
        }
        made = std::move(l);
        return true;
    }
    if (cmdLower == "distance" && a.size() == 2) {
        auto pts = needPts(2);
        if (pts.empty()) {
            // 上游 语义：Distance(数, 数) = |a−b|（表格/统计场景对数值取距离）
            GeoNumeric *n1 = dynamic_cast<GeoNumeric *>(lookup(a[0]));
            GeoNumeric *n2 = dynamic_cast<GeoNumeric *>(lookup(a[1]));
            if (n1 != nullptr && n2 != nullptr) {
                lastError.clear();   // needPts 失败时留下的"未找到点"不再适用
                auto n = std::make_unique<GeoNumeric>();
                GeoNumeric *p1 = n1, *p2 = n2;
                n->calc = [p1, p2]() { return fabs(p2->value - p1->value); };
                n->cmd = "Distance(" + p1->label + ", " + p2->label + ")";
                n->cmdName = "Distance";
                n->cmdArgs = { a[0], a[1] };
                n->inputs = { n1, n2 };
                n->update();
                made = std::move(n);
            }
            // 上游 语义 Distance(点, 直线/线段/射线) = 点到直线距离
            //（两参数均不是数对时尝试；直线系数在 calc 内实时取，随基准重算）
            GeoPoint *dp = lookupPoint(a[0]);
            GeoElement *dl = dp ? lookup(a[1]) : nullptr;
            bool dlOk = dl && (dl->type() == GeoType::Line
                || dl->type() == GeoType::Segment || dl->type() == GeoType::Ray);
            if (!dlOk) {
                GeoPoint *dp2 = lookupPoint(a[1]);
                GeoElement *dl2 = dp2 ? lookup(a[0]) : nullptr;
                bool dlOk2 = dl2 && (dl2->type() == GeoType::Line
                    || dl2->type() == GeoType::Segment || dl2->type() == GeoType::Ray);
                if (dlOk2) {
                    dp = dp2;
                    dl = dl2;
                    dlOk = true;
                }
            }
            if (dlOk) {
                lastError.clear();   // needPts 的"未找到点"不再适用
                GeoPoint *P = dp;
                GeoElement *L = dl;
                auto n = std::make_unique<GeoNumeric>();
                n->calc = [P, L]() {
                    double la = 0, lb = 0, lc = 0;
                    if (GeoLine *l = dynamic_cast<GeoLine *>(L)) {
                        la = l->a; lb = l->b; lc = l->c;
                    } else if (GeoSegment *s = dynamic_cast<GeoSegment *>(L)) {
                        la = s->ey1() - s->ey2();
                        lb = s->ex2() - s->ex1();
                        lc = la * s->ex1() + lb * s->ey1();
                    } else if (GeoRay *r = dynamic_cast<GeoRay *>(L)) {
                        la = r->p1->y - r->p2->y;
                        lb = r->p2->x - r->p1->x;
                        lc = la * r->p1->x + lb * r->p1->y;
                    }
                    double h = sqrt(la * la + lb * lb);
                    return h > 1e-15 ? fabs(la * P->x + lb * P->y - lc) / h : 0.0;
                };
                n->cmd = "Distance(" + dp->label + ", " + dl->label + ")";
                n->cmdName = "Distance";
                n->cmdArgs = { dp->label, dl->label };
                n->inputs = { dp, dl };
                n->update();
                made = std::move(n);
            }
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        GeoPoint *p1 = pts[0], *p2 = pts[1];
        n->calc = [p1, p2]() {
            double dx = p2->x - p1->x, dy = p2->y - p1->y;
            return sqrt(dx * dx + dy * dy);
        };
        n->cmd = "Distance(" + p1->label + ", " + p2->label + ")";
        n->cmdName = "Distance";
        n->cmdArgs = { a[0], a[1] };
        n->inputs = { pts[0], pts[1] };
        n->update();
        made = std::move(n);
        return true;
    }

    // ---------- N5 作图核心 ----------

    if (cmdLower == "perpendicularbisector" && a.size() == 2) {
        auto pts = needPts(2);
        if (pts.empty()) { made = nullptr; return true; }
        auto l = std::make_unique<GeoLine>();
        l->mode = GeoLine::Mode::PerpBisector;
        l->p1 = pts[0];
        l->p2 = pts[1];
        l->inputs = { pts[0], pts[1] };
        l->cmdName = "PerpendicularBisector";
        l->cmdArgs = { a[0], a[1] };
        l->update();
        made = std::move(l);
        return true;
    }
    if (cmdLower == "anglebisector" && a.size() == 3) {
        auto pts = needPts(3);
        if (pts.empty()) { made = nullptr; return true; }
        auto l = std::make_unique<GeoLine>();
        l->mode = GeoLine::Mode::AngleBisector;
        l->pt = pts[1];
        l->inputs = { pts[0], pts[1], pts[2] };
        l->cmdName = "AngleBisector";
        l->cmdArgs = { a[0], a[1], a[2] };
        l->update();
        if (fabs(l->a) < 1e-15 && fabs(l->b) < 1e-15) {
            lastError = "角平分线方向退化";
            made = nullptr;
            return true;
        }
        made = std::move(l);
        return true;
    }
    if (cmdLower == "angle" && a.size() == 3) {
        // Angle(A,B,C)：从 BA 逆时针到 BC 的角度（度，0..360），画布画弧
        auto pts = needPts(3);
        if (pts.empty()) { made = nullptr; return true; }
        auto ang = std::make_unique<GeoAngle>();
        ang->va = pts[0];
        ang->vb = pts[1];
        ang->vc = pts[2];
        ang->inputs = { pts[0], pts[1], pts[2] };
        ang->cmdName = "Angle";
        ang->cmdArgs = { a[0], a[1], a[2] };
        GeoPoint *A = pts[0], *B = pts[1], *C = pts[2];
        ang->acalc = [A, B, C](GeoAngle *g) {
            double w0 = atan2(A->y - B->y, A->x - B->x);
            double w1 = atan2(C->y - B->y, C->x - B->x);
            double ccw = w1 - w0;
            while (ccw < 0) ccw += 2 * M_PI;
            g->value = ccw * 180.0 / M_PI;
        };
        ang->update();
        made = std::move(ang);
        return true;
    }
    if ((cmdLower == "ray" || cmdLower == "vector") && a.size() == 2) {
        auto pts = needPts(2);
        if (pts.empty()) {
            // 上游 语义 Vector(点, 向量) = 相等向量（自点出发的平移副本）。
            // 终点不是构造对象：走 image 缓存（同 GeoSegment 变换像）
            GeoVector *u = cmdLower == "vector"
                ? dynamic_cast<GeoVector *>(lookup(a[1])) : nullptr;
            GeoPoint *pt = u ? lookupPoint(a[0]) : nullptr;
            if (!pt) { made = nullptr; return true; }   // needPts 的错误保持
            lastError.clear();
            GeoPoint *A = pt;
            GeoVector *src = u;
            auto v = std::make_unique<GeoVector>();
            v->image = true;
            v->calc = [A, src](GeoVector *g) {
                g->ix1 = A->x;
                g->iy1 = A->y;
                g->ix2 = A->x + src->ex2() - src->ex1();
                g->iy2 = A->y + src->ey2() - src->ey1();
            };
            v->inputs = { pt, u };
            v->cmdName = "Vector";
            v->cmdArgs = { a[0], a[1] };
            v->update();
            made = std::move(v);
            return true;
        }
        if (cmdLower == "ray") {
            auto r = std::make_unique<GeoRay>();
            r->p1 = pts[0];
            r->p2 = pts[1];
            r->inputs = { pts[0], pts[1] };
            r->cmdName = "Ray";
            r->cmdArgs = { a[0], a[1] };
            made = std::move(r);
        } else {
            auto v = std::make_unique<GeoVector>();
            v->p1 = pts[0];
            v->p2 = pts[1];
            v->inputs = { pts[0], pts[1] };
            v->cmdName = "Vector";
            v->cmdArgs = { a[0], a[1] };
            made = std::move(v);
        }
        return true;
    }
    if (cmdLower == "polygon" && a.size() >= 3) {
        auto pts = needPts(a.size());
        if (pts.empty()) { made = nullptr; return true; }
        auto pg = std::make_unique<GeoPolygon>();
        pg->verts = pts;
        pg->inputs.assign(pts.begin(), pts.end());
        pg->cmdName = "Polygon";
        pg->cmdArgs = a;
        made = std::move(pg);
        return true;
    }
    if (cmdLower == "tangent" && (a.size() == 2 || a.size() == 3)) {
        // Tangent(点, 圆|函数)：圆外一点两条切线（仅无名时多输出，上游 语义）；
        // 命名/带序号（保存重放）只建第 n 条
        GeoPoint *P = lookupPoint(a[0]);
        GeoElement *t = lookup(a[1]);
        GeoCircle *circ = dynamic_cast<GeoCircle *>(t);
        GeoFunction *fn = dynamic_cast<GeoFunction *>(t);
        if (!P || (!circ && !fn)) {
            lastError = "Tangent 需要（点, 圆/函数）且对象须已存在";
            made = nullptr;
            return true;
        }
        int kOnly = -1;
        if (a.size() == 3) {
            double kv = 0;
            if (!numArg(a[2], kv)) { lastError = "切线序号不是数: " + a[2]; made = nullptr; return true; }
            kOnly = (int)kv - 1;
            if (kOnly < 0) { lastError = "切线序号从 1 开始"; made = nullptr; return true; }
        }
        if (!name.empty() && kOnly < 0) kOnly = 0;   // 命名单输出
        auto mkTangent = [&](int k) -> std::unique_ptr<GeoLine> {
            auto l = std::make_unique<GeoLine>();
            l->mode = GeoLine::Mode::Fixed;
            l->inputs = { P, t };
            l->cmdName = "Tangent";
            l->cmdArgs = { a[0], a[1] };
            l->cmdIndex = k;
            if (circ) {
                l->calc = [P, circ, k](GeoLine *L) {
                    double vx = P->x - circ->center->x, vy = P->y - circ->center->y;
                    double d2 = vx * vx + vy * vy;
                    double r = circ->radius;
                    if (d2 < 1e-24) { L->a = 0; L->b = 0; return; }
                    if (fabs(sqrt(d2) - r) <= 1e-9 * std::max(1.0, r)) {
                        // 点在圆上：切线 ⊥ 半径
                        L->a = vx;
                        L->b = vy;
                        L->c = L->a * P->x + L->b * P->y;
                        return;
                    }
                    if (d2 <= r * r) { L->a = 0; L->b = 0; return; }   // 圆内：无实切线
                    double d = sqrt(d2);
                    double ex = vx / d, ey = vy / d;
                    double foot = r * r / d;
                    double h = r * sqrt(d2 - r * r) / d;
                    double sgn = k == 0 ? 1.0 : -1.0;
                    double tx = circ->center->x + foot * ex - sgn * h * ey;
                    double ty = circ->center->y + foot * ey + sgn * h * ex;
                    L->a = ty - P->y;
                    L->b = P->x - tx;
                    L->c = L->a * P->x + L->b * P->y;
                };
            } else {
                l->calc = [P, fn](GeoLine *L) {
                    // 函数在 P.x 处的切线（数值导数）
                    double x0 = P->x;
                    double eps = 1e-6;
                    double y0 = fn->evalAt(x0);
                    double m = (fn->evalAt(x0 + eps) - fn->evalAt(x0 - eps)) / (2 * eps);
                    L->a = m;
                    L->b = -1.0;
                    L->c = m * x0 - y0;
                };
            }
            l->update();
            return l;
        };
        if (circ) {
            double dx = P->x - circ->center->x, dy = P->y - circ->center->y;
            double d = sqrt(dx * dx + dy * dy);
            if (kOnly >= 0) {
                if (d < circ->radius - 1e-9 || (kOnly > 0 && d <= circ->radius + 1e-9)) {
                    lastError = "未找到切线";
                    made = nullptr;
                    return true;
                }
                made = mkTangent(kOnly);
                return true;
            }
            if (d < circ->radius - 1e-9) {
                lastError = "点在圆内，不存在实切线";
                made = nullptr;
                return true;
            }
            made = mkTangent(0);
            if (d > circ->radius + 1e-9) extras_.push_back(mkTangent(1));   // 圆外：两条
        } else {
            if (kOnly > 0) { lastError = "未找到切线"; made = nullptr; return true; }
            made = mkTangent(0);
        }
        return true;
    }
    if (cmdLower == "intersect" && (a.size() == 2 || a.size() == 3)) {
        // Intersect(a, b)：全部交点（仅无名时多输出，上游 语义）；
        // 命名/带序号（保存重放）只建第 n 个
        int nth = -1;
        if (a.size() == 3) {
            double nval = 0;
            if (!numArg(a[2], nval)) { lastError = "交点序号不是数: " + a[2]; made = nullptr; return true; }
            nth = (int)nval - 1;
            if (nth < 0) { lastError = "交点序号从 1 开始"; made = nullptr; return true; }
        }
        GeoElement *o1 = lookup(a[0]), *o2 = lookup(a[1]);
        if (!o1 || !o2) { lastError = "未找到对象 " + (o1 ? a[1] : a[0]); made = nullptr; return true; }
        auto mkPt = [&](int idx) -> std::unique_ptr<GeoPoint> {
            auto p = std::make_unique<GeoPoint>();
            p->isFree = false;
            p->pointSize = 4;
            p->color = {0x64, 0x64, 0x64};
            p->inputs = { o1, o2 };
            p->cmdName = "Intersect";
            p->cmdArgs = { a[0], a[1] };
            p->cmdIndex = idx;
            GeoElement *e1 = o1, *e2 = o2;
            p->calc = [this, e1, e2, idx](GeoPoint *P) {
                double x, y;
                if (intersectAt(e1, e2, idx, x, y)) {
                    P->x = x;
                    P->y = y;
                }
            };
            double x, y;
            if (intersectAt(o1, o2, idx, x, y)) { p->x = x; p->y = y; }
            return p;
        };
        if (nth >= 0 || !name.empty()) {
            if (nth < 0) nth = 0;
            double x, y;
            if (!intersectAt(o1, o2, nth, x, y)) { lastError = "未找到交点"; made = nullptr; return true; }
            made = mkPt(nth);
            return true;
        }
        std::vector<std::pair<double, double>> pts;
        intersectAll(o1, o2, pts);
        if (pts.empty()) { lastError = "未找到交点"; made = nullptr; return true; }
        for (size_t k = 0; k < pts.size(); ++k) {
            auto p = mkPt((int)k);
            p->x = pts[k].first;
            p->y = pts[k].second;
            if (k == 0) made = std::move(p);
            else extras_.push_back(std::move(p));
        }
        return true;
    }

    // ---------- N5 变换命令族 ----------

    if (cmdLower == "translate" && a.size() == 2) {
        GeoElement *obj = lookup(a[0]);
        GeoVector *vec = dynamic_cast<GeoVector *>(lookup(a[1]));
        if (!obj || !vec) { lastError = "Translate 需要（对象, 向量）"; made = nullptr; return true; }
        GeoVector *vv = vec;
        made = makeImage(obj, "Translate", { a[0], a[1] }, [vv](double x, double y) {
            return std::make_pair(x + vv->p2->x - vv->p1->x, y + vv->p2->y - vv->p1->y);
        }, std::function<double(double)>(), vec);
        if (!made) lastError = "Translate 暂不支持该对象类型";
        return true;
    }
    if (cmdLower == "rotate" && a.size() == 3) {
        // Rotate(对象, 角度°, 中心点)；角度可为数值对象（slider 驱动旋转）
        GeoElement *obj = lookup(a[0]);
        GeoPoint *center = lookupPoint(a[2]);
        GeoNumeric *angObj = dynamic_cast<GeoNumeric *>(lookup(a[1]));
        double theta = 0;
        bool hasTheta = angObj != nullptr || numArg(a[1], theta);
        if (!obj || !center || !hasTheta) {
            lastError = "Rotate 需要（对象, 角度, 中心点）";
            made = nullptr;
            return true;
        }
        GeoPoint *cc = center;
        GeoNumeric *an = angObj;
        MapFn map = [cc, an, theta](double x, double y) {
            double th = (an ? an->value : theta) * M_PI / 180.0;
            double dx = x - cc->x, dy = y - cc->y;
            double cs = cos(th), sn = sin(th);
            return std::make_pair(cc->x + dx * cs - dy * sn, cc->y + dx * sn + dy * cs);
        };
        made = makeImage(obj, "Rotate", a, map, std::function<double(double)>(), center, angObj);
        if (!made) lastError = "Rotate 暂不支持该对象类型";
        return true;
    }
    if (cmdLower == "reflect" && a.size() == 2) {
        // Reflect(对象, 直线) / Reflect(对象, 点)
        GeoElement *obj = lookup(a[0]);
        GeoElement *mirror = lookup(a[1]);
        GeoPoint *mp = lookupPoint(a[1]);
        if (!obj || !mirror) { lastError = "Reflect 需要（对象, 直线/点）"; made = nullptr; return true; }
        if (mp) {
            GeoPoint *mm = mp;
            made = makeImage(obj, "Reflect", { a[0], a[1] }, [mm](double x, double y) {
                return std::make_pair(2 * mm->x - x, 2 * mm->y - y);
            }, std::function<double(double)>(), mp);
        } else {
            GeoElement *m = mirror;
            MapFn map = [m](double x, double y) {
                double la, lb, lc;
                if (!LineCoefsOf(m, la, lb, lc)) return std::make_pair(x, y);
                double den = la * la + lb * lb;
                double t = (la * x + lb * y - lc) / den;
                return std::make_pair(x - 2 * t * la, y - 2 * t * lb);
            };
            made = makeImage(obj, "Reflect", { a[0], a[1] }, map, std::function<double(double)>(), mirror);
        }
        if (!made) lastError = "Reflect 暂不支持该对象类型";
        return true;
    }
    if (cmdLower == "dilate" && a.size() == 3) {
        // Dilate(对象, 因子, 中心点)；因子可为数值对象
        GeoElement *obj = lookup(a[0]);
        GeoPoint *center = lookupPoint(a[2]);
        GeoNumeric *kObj = dynamic_cast<GeoNumeric *>(lookup(a[1]));
        double k = 0;
        bool hasK = kObj != nullptr || numArg(a[1], k);
        if (!obj || !center || !hasK) {
            lastError = "Dilate 需要（对象, 因子, 中心点）";
            made = nullptr;
            return true;
        }
        GeoPoint *cc = center;
        GeoNumeric *kn = kObj;
        MapFn map = [cc, kn, k](double x, double y) {
            double f = kn ? kn->value : k;
            return std::make_pair(cc->x + f * (x - cc->x), cc->y + f * (y - cc->y));
        };
        std::function<double(double)> rmul = [kn, k](double) { return fabs(kn ? kn->value : k); };
        made = makeImage(obj, "Dilate", a, map, rmul, center, kObj);
        if (!made) lastError = "Dilate 暂不支持该对象类型";
        return true;
    }

    // ---------- N5 滑动条与动画 ----------

    if (cmdLower == "slider" && a.size() >= 2) {
        double smin = 0, smax = 0, step = 0.1, speed = 1.0;
        if (!numArg(a[0], smin) || !numArg(a[1], smax)) {
            lastError = "Slider 需要数值范围，如 a=Slider(-5, 5)";
            made = nullptr;
            return true;
        }
        if (a.size() > 2 && !numArg(a[2], step)) { lastError = "步长不是数: " + a[2]; made = nullptr; return true; }
        if (a.size() > 3 && !numArg(a[3], speed)) { lastError = "速度不是数: " + a[3]; made = nullptr; return true; }
        if (!(smax > smin)) { lastError = "Slider 上限须大于下限"; made = nullptr; return true; }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = true;
        n->isSlider = true;
        n->visible = true;   // 滑动条要在画布绘制（数值类默认隐藏）
        n->smin = smin;
        n->smax = smax;
        n->sstep = step > 1e-12 ? step : 0.0;
        n->speed = speed > 0 ? speed : 1.0;
        n->value = smin;
        placeSlider(n.get());
        made = std::move(n);
        return true;
    }
    if (cmdLower == "startanimation") {
        if (a.size() == 1) {
            GeoNumeric *n = dynamic_cast<GeoNumeric *>(lookup(a[0]));
            if (n && n->isSlider) { n->animating = true; n->animDir = 1; }
        } else {
            for (auto &g : geos_) {
                GeoNumeric *n = dynamic_cast<GeoNumeric *>(g.get());
                if (n && n->isSlider) { n->animating = true; n->animDir = 1; }
            }
        }
        made = nullptr;   // 无产物命令（input 以 lastError 为空判定成功）
        return true;
    }
    if (cmdLower == "stopanimation") {
        for (auto &g : geos_) {
            GeoNumeric *n = dynamic_cast<GeoNumeric *>(g.get());
            if (n) n->animating = false;
        }
        made = nullptr;
        return true;
    }

    // ---------- N6 函数分析 ----------

    // Derivative(f) / Derivative(f, n)：数值导数（中心差分，n 阶迭代）。
    // 产物为 alt 闭包函数（源重定义后求值自动取最新定义），src = f'(x) 样式。
    if (cmdLower == "derivative" && (a.size() == 1 || a.size() == 2)) {
        GeoFunction *f = resolveFunctionArg(a[0]);
        if (!f) { lastError = "Derivative 需要（函数）或（函数, 阶数）"; made = nullptr; return true; }
        int n = 1;
        if (a.size() == 2) {
            double nv = 0;
            if (!numArg(a[1], nv) || nv < 1 || nv > 5) {
                lastError = "导数阶数须为 1..5 的数: " + a[1];
                made = nullptr;
                return true;
            }
            n = (int)nv;
        }
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        GeoFunction *src = f;
        int order = n;
        if (order == 1) {
            d->alt = [src](double x) {
                double eps = 1e-5 * std::max(1.0, fabs(x));
                return (src->evalAt(x + eps) - src->evalAt(x - eps)) / (2 * eps);
            };
        } else {
            d->alt = [src, order](double x) {
                double eps = 1e-4 * std::max(1.0, fabs(x));
                double sum = 0;
                // n 阶中心差分系数表（n<=5）
                static const double c2[3] = { 1, -2, 1 };
                static const double c3[5] = { -0.5, 1, 0, -1, 0.5 };
                static const double c4[5] = { 1, -4, 6, -4, 1 };
                static const double c5[7] = { -0.5, 2, -2.5, 0, 2.5, -2, 0.5 };
                const double *co = nullptr;
                int m = 0;
                if (order == 2) { co = c2; m = 1; }
                else if (order == 3) { co = c3; m = 2; }
                else if (order == 4) { co = c4; m = 2; }
                else { co = c5; m = 3; }
                double fact = 1;
                for (int i = 2; i <= order; ++i) fact *= i;
                for (int i = -m; i <= m; ++i) {
                    sum += co[i + m] * src->evalAt(x + i * eps);
                }
                return sum / (fact * pow(eps, order));
            };
        }
        d->src = "Derivative(" + a[0] + (a.size() == 2 ? ", " + a[1] : "") + ")";
        d->cmdName = "Derivative";
        d->cmdArgs = { a[0] };
        if (a.size() == 2) d->cmdArgs.push_back(a[1]);
        d->inputs = { f };
        d->update();
        made = std::move(d);
        return true;
    }
    // NthRoot(x, n)：n 次方根（上游 f(x) 页 ⁿ√□ 模板键）。负底奇数次取实根
    if (cmdLower == "nthroot" && a.size() == 2) {
        double xv = 0, nv = 0;
        if (!numArg(a[0], xv) || !numArg(a[1], nv)) {
            lastError = "NthRoot 需要（数, 次数）";
            made = nullptr;
            return true;
        }
        const long long ni = llround(nv);
        if (ni == 0 || std::fabs(nv - (double)ni) > 1e-9) {
            lastError = "根指数须为非零整数: " + a[1];
            made = nullptr;
            return true;
        }
        const double root = (xv < 0 && ni % 2 != 0)
            ? -pow(-xv, 1.0 / (double)ni)
            : pow(xv, 1.0 / (double)ni);
        if (!std::isfinite(root)) {
            lastError = "NthRoot 结果不是有限数";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->value = root;
        n->cmd = "NthRoot(" + a[0] + ", " + a[1] + ")";
        n->cmdName = "NthRoot";
        n->cmdArgs = { a[0], a[1] };
        made = std::move(n);
        return true;
    }
    // Floor(x) / Ceil(x)：上游 #&¬ 页 ⌊⌋ ⌈⌉ 模板键对应命令（取整 → 数值对象）
    if ((cmdLower == "floor" || cmdLower == "ceil") && a.size() == 1) {
        double xv = 0;
        if (!numArg(a[0], xv)) {
            lastError = std::string(cmdLower == "floor" ? "Floor" : "Ceil") + " 需要（数）";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->value = cmdLower == "floor" ? std::floor(xv) : std::ceil(xv);
        n->cmd = (cmdLower == "floor" ? "Floor(" : "Ceil(") + a[0] + ")";
        n->cmdName = cmdLower == "floor" ? "Floor" : "Ceil";
        n->cmdArgs = { a[0] };
        made = std::move(n);
        return true;
    }
    // Integral(f) / Integral(f, a, b)：定积分（Simpson 1/3，512 段）→ 数值对象
    if (cmdLower == "integral" && (a.size() == 1 || a.size() == 3)) {
        GeoFunction *f = resolveFunctionArg(a[0]);
        if (!f) { lastError = "Integral 需要（函数）或（函数, 下限, 上限）"; made = nullptr; return true; }
        double lo = 0, hi = 0;
        GeoNumeric *loN = nullptr, *hiN = nullptr;
        if (a.size() == 3) {
            loN = dynamic_cast<GeoNumeric *>(lookup(a[1]));
            hiN = dynamic_cast<GeoNumeric *>(lookup(a[2]));
            bool ok = (loN != nullptr || numArg(a[1], lo)) && (hiN != nullptr || numArg(a[2], hi));
            if (!ok) { lastError = "积分上下限须为数或数值对象"; made = nullptr; return true; }
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->visible = false;
        GeoFunction *src = f;
        GeoNumeric *A = loN, *B = hiN;
        double l0 = lo, h0 = hi;
        n->calc = [src, A, B, l0, h0]() {
            double x0 = A ? A->value : l0;
            double x1 = B ? B->value : h0;
            if (x1 < x0) return -Simpson(src, x1, x0, 512);
            return Simpson(src, x0, x1, 512);
        };
        n->cmd = "Integral(" + a[0] + (a.size() == 3 ? ", " + a[1] + ", " + a[2] : "") + ")";
        n->cmdName = "Integral";
        n->cmdArgs = { a[0] };
        if (a.size() == 3) { n->cmdArgs.push_back(a[1]); n->cmdArgs.push_back(a[2]); }
        n->inputs = { f };
        if (loN) n->inputs.push_back(loN);
        if (hiN) n->inputs.push_back(hiN);
        n->update();
        made = std::move(n);
        return true;
    }
    // Root(f, a, b)：区间内全部零点（无名多输出，上游 语义；保存重放的
    // Root(f, a, b, k) 第 n 形式只建第 k 个）。Root(f, 起点)：起点附近首个零点。
    if (cmdLower == "root" && (a.size() == 2 || a.size() == 3 || a.size() == 4)) {
        GeoFunction *f = resolveFunctionArg(a[0]);
        if (!f) { lastError = "Root 需要（函数, a, b）或（函数, 起点）"; made = nullptr; return true; }
        double x0 = 0, x1 = 0;
        if (!numArg(a[1], x0)) { lastError = "求根区间起点不是数: " + a[1]; made = nullptr; return true; }
        if (a.size() >= 3 && !numArg(a[2], x1)) {
            lastError = "求根区间终点不是数: " + a[2];
            made = nullptr;
            return true;
        }
        GeoFunction *src = f;
        // 区间内等距粗扫找变号 + 二分收敛
        auto findRoot = [src](double lo, double hi, double &out) -> bool {
            const int N = 200;
            double fa = src->evalAt(lo);
            for (int i = 1; i <= N; ++i) {
                double x = lo + (hi - lo) * i / N;
                double fx = src->evalAt(x);
                if (std::isfinite(fa) && std::isfinite(fx) && (fa <= 0) != (fx <= 0)) {
                    double a2 = lo + (hi - lo) * (i - 1) / N, b2 = x;
                    for (int it = 0; it < 80; ++it) {
                        double mid = 0.5 * (a2 + b2);
                        double fm = src->evalAt(mid);
                        if (!std::isfinite(fm)) break;
                        if ((fa <= 0) != (fm <= 0)) { b2 = mid; } else { a2 = mid; fa = fm; }
                    }
                    out = 0.5 * (a2 + b2);
                    return true;
                }
                if (std::isfinite(fx)) fa = fx;
            }
            return false;
        };
        auto rootsIn = [findRoot](double lo, double hi, std::vector<double> &out) {
            out.clear();
            const int N = 200;
            double span = (hi - lo) / N;
            for (int i = 0; i < N; ++i) {
                // 逐子区间扫描（相邻区间共享端点，不漏端点邻域）
                double r = 0;
                if (findRoot(lo + i * span, lo + (i + 1) * span, r)) {
                    bool dup = false;
                    for (double v : out) {
                        if (fabs(v - r) < 1e-6 * std::max(1.0, fabs(r))) { dup = true; break; }
                    }
                    if (!dup) out.push_back(r);
                }
            }
        };
        // 序号重放：保存的第 n 形式经 xmlCommand 还原为 Root(f, a, b, k) 四参
        int kOnly = -1;
        if (a.size() == 4) {
            double kv = 0;
            if (!numArg(a[3], kv)) { lastError = "求根序号不是数: " + a[3]; made = nullptr; return true; }
            kOnly = (int)kv - 1;
            if (kOnly < 0) { lastError = "求根序号从 1 开始"; made = nullptr; return true; }
        }
        std::vector<double> roots;
        if (a.size() >= 3) {
            rootsIn(x0, x1, roots);
        } else {
            // 起点形式：窗口倍增扩到首个变号零点
            for (double span = 1; span <= 1024; span *= 2) {
                rootsIn(x0 - span, x0 + span, roots);
                if (!roots.empty()) break;
            }
        }
        if (roots.empty()) {
            lastError = a.size() >= 3 ? "区间内未找到变号零点" : "起点附近未找到零点";
            made = nullptr;
            return true;
        }
        if (kOnly >= 0) {
            if (kOnly >= (int)roots.size()) { lastError = "未找到第 " + a[3] + " 个零点"; made = nullptr; return true; }
        }
        // 依随重算：第 idx 个零点（与创建时同一扫描逻辑）
        std::vector<double> anchored = roots;
        double ax0 = x0, ax1 = x1;
        bool bounded = a.size() >= 3;
        auto mkRootPt = [&](int idx) -> std::unique_ptr<GeoPoint> {
            auto p = std::make_unique<GeoPoint>();
            p->isFree = false;
            p->pointSize = 4;
            p->color = {0x64, 0x64, 0x64};
            p->cmdName = "Root";
            p->cmdArgs = { a[0], a[1] };
            if (a.size() >= 3) p->cmdArgs.push_back(a[2]);
            p->cmdIndex = idx;   // 第 n 个零点（保存重放第 n 形式）
            p->calc = [src, findRoot, rootsIn, expandOK = !bounded, ax0, ax1, idx](GeoPoint *g) {
                std::vector<double> rs;
                if (expandOK) {
                    for (double span = 1; span <= 1024; span *= 2) {
                        rootsIn(ax0 - span, ax0 + span, rs);
                        if (!rs.empty()) break;
                    }
                } else {
                    rootsIn(ax0, ax1, rs);
                }
                if (idx < (int)rs.size()) { g->x = rs[idx]; g->y = 0; }
            };
            p->x = anchored[idx];
            p->y = 0;
            p->update();
            return p;
        };
        // 命名（含保存重放 "B=Root(f,a,b)"）或带序号（第 n 形式）：只建 1 个；
        // 无名区间形式才是多输出（上游 语义）。多输出时多余的会与后续重放的
        // 命名对象撞标签——这正是"已跳过 N 个"假阳性的来源
        if (kOnly >= 0 || !name.empty()) {
            int idx = kOnly >= 0 ? kOnly : 0;
            if (idx >= (int)roots.size()) {
                lastError = kOnly >= 0 ? "未找到第 " + a[3] + " 个零点" : "未找到零点";
                made = nullptr;
                return true;
            }
            made = mkRootPt(idx);
            return true;
        }
        made = mkRootPt(0);
        for (size_t k = 1; k < roots.size(); ++k) {
            extras_.push_back(mkRootPt((int)k));
        }
        return true;
    }
    // Extremum(f, a, b)：区间内极值点（导数变号扫描 + 二分）→ 点列表；
    // 保存重放的 Extremum(f, a, b, k) 第 n 形式只建第 k 个
    if (cmdLower == "extremum" && (a.size() == 3 || a.size() == 4)) {
        GeoFunction *f = resolveFunctionArg(a[0]);
        if (!f) { lastError = "Extremum 需要（函数, a, b）"; made = nullptr; return true; }
        double x0 = 0, x1 = 0;
        if (!numArg(a[1], x0) || !numArg(a[2], x1)) {
            lastError = "极值区间须为数";
            made = nullptr;
            return true;
        }
        if (!(x1 > x0)) { lastError = "区间上限须大于下限"; made = nullptr; return true; }
        int kOnly = -1;
        if (a.size() == 4) {
            double kv = 0;
            if (!numArg(a[3], kv)) { lastError = "极值序号不是数: " + a[3]; made = nullptr; return true; }
            kOnly = (int)kv - 1;
            if (kOnly < 0) { lastError = "极值序号从 1 开始"; made = nullptr; return true; }
        }
        GeoFunction *src = f;
        // 命名（含保存重放 "D=Extremum(f,a,b)"）或带序号：只建 1 个（第 target
        // 个）；无名才是多输出
        const bool single = kOnly >= 0 || !name.empty();
        const int target = single ? (kOnly >= 0 ? kOnly : 0) : -1;
        const int N = 400;
        double eps = (x1 - x0) / N;
        int found = 0;   // 已确认极值点数（保存重放第 n 形式需要）
        for (int i = 0; i < N; ++i) {
            double xa = x0 + i * eps;
            double xm = xa + eps * 0.5;
            double xb = xa + eps;
            // 中心差分导数在小区间内变号 → 二分求导数零点
            double da = (src->evalAt(xa + 1e-6) - src->evalAt(xa - 1e-6)) / 2e-6;
            double db = (src->evalAt(xb + 1e-6) - src->evalAt(xb - 1e-6)) / 2e-6;
            if (!std::isfinite(da) || !std::isfinite(db) || (da <= 0) == (db <= 0)) continue;
            if (single && found != target) { ++found; continue; }
            double a2 = xa, b2 = xb;
            for (int it = 0; it < 60; ++it) {
                double mid = 0.5 * (a2 + b2);
                double dm = (src->evalAt(mid + 1e-6) - src->evalAt(mid - 1e-6)) / 2e-6;
                if (!std::isfinite(dm)) break;
                if ((da <= 0) == (dm <= 0)) { a2 = mid; da = dm; } else { b2 = mid; }
            }
            double xe = 0.5 * (a2 + b2);
            auto pt = std::make_unique<GeoPoint>();
            pt->isFree = false;
            pt->pointSize = 4;
            pt->color = {0x64, 0x64, 0x64};
            pt->x = xe;
            pt->y = src->evalAt(xe);
            pt->cmdName = "Extremum";
            pt->cmdArgs = { a[0], a[1], a[2] };
            pt->cmdIndex = found;   // 第 n 个极值点（保存重放第 n 形式）
            pt->calc = [src, xe](GeoPoint *g) {
                g->x = xe;
                g->y = src->evalAt(xe);
            };
            pt->update();
            if (single) {
                made = std::move(pt);
                return true;
            }
            extras_.push_back(std::move(pt));
            ++found;
        }
        if (single) {
            // 扫描完仍没凑到第 target 个
            lastError = kOnly >= 0 ? "未找到第 " + a[3] + " 个极值点" : "未找到极值点";
            made = nullptr;
            return true;
        }
        if (extras_.empty()) {
            // 无极值点也允许命令成功（上游 返回空列表）；建隐藏数值占位会污染，
            // 直接无产物 + 无错误
            made = nullptr;
            lastError.clear();
            return true;
        }
        flushExtras();
        made = nullptr;
        lastError.clear();
        return true;
    }

    // ---------- N6 批次④ 文本与积分 ----------

    // Text("内容"[, P|(x,y)]) → 画布文本对象（依附点时跟随移动；缺省落点 (1,1)）。
    // 批次⑤ 动态文本：Text("a="+b)、Text(b)——非字面段引用对象 valueText，
    // 重算时自动刷新（滑动条拖动即时变字）
    if (cmdLower == "text" && !a.empty()
        && ((a[0].size() >= 2 && a[0].front() == '"')
            || (a.size() == 1 && lookup(a[0]) != nullptr))) {
        if (a.size() > 2) { lastError = "Text 参数为 (\"文本\"[, 位置])"; made = nullptr; return true; }
        auto t = std::make_unique<GeoText>();
        std::vector<std::pair<bool, std::string>> segs;
        std::vector<GeoElement *> refs;
        if (a.size() == 1 && a[0].front() != '"') {
            // Text(b)：显示对象 b 的当前值
            segs.push_back({ false, a[0] });
            refs.push_back(lookup(a[0]));
        } else if (a[0].find('+') == std::string::npos) {
            segs.push_back({ true, a[0].substr(1, a[0].size() - 2) });
        } else {
            // 顶层 '+' 切分（引号内的 '+' 不切），逐段：引号字面量/数值/对象引用
            std::vector<std::string> toks;
            bool inQ = false;
            size_t start = 0;
            for (size_t i = 0; i < a[0].size(); ++i) {
                if (a[0][i] == '"') inQ = !inQ;
                else if (!inQ && a[0][i] == '+') {
                    toks.push_back(a[0].substr(start, i - start));
                    start = i + 1;
                }
            }
            toks.push_back(a[0].substr(start));
            bool ok = true;
            for (const std::string &tok : toks) {
                if (tok.empty()) { ok = false; break; }
                if (tok.front() == '"' && tok.size() >= 2 && tok.back() == '"') {
                    segs.push_back({ true, tok.substr(1, tok.size() - 2) });
                    continue;
                }
                Expr num;
                std::string ne;
                double nv;
                if (num.parse(tok, ne) && !num.containsX() && std::isfinite(nv = num.eval(0.0))) {
                    segs.push_back({ true, FormatNum(nv) });
                    continue;
                }
                GeoElement *r = lookup(tok);
                if (!r) { lastError = "文本引用了不存在的对象: " + tok; ok = false; break; }
                segs.push_back({ false, tok });
                refs.push_back(r);
            }
            if (!ok) {
                if (lastError.empty()) lastError = "动态文本格式：\"前缀\" + 对象";
                made = nullptr;
                return true;
            }
        }
        if (a.size() == 2) {
            if (GeoPoint *p = lookupPoint(a[1])) {
                t->anchor = p;
                t->inputs.push_back(p);
            } else if (a[1].front() == '(' && a[1].back() == ')') {
                const std::string inner = a[1].substr(1, a[1].size() - 2);
                const size_t comma = inner.find(',');
                double vx = 0, vy = 0;
                if (comma == std::string::npos
                    || !numArg(inner.substr(0, comma), vx)
                    || !numArg(inner.substr(comma + 1), vy)) {
                    lastError = "文本位置须为点或 (x,y): " + a[1];
                    made = nullptr;
                    return true;
                }
                t->x = vx;
                t->y = vy;
            } else {
                lastError = "未找到点 " + a[1];
                made = nullptr;
                return true;
            }
        }
        for (GeoElement *r : refs) t->inputs.push_back(r);
        t->segs = std::move(segs);
        t->refs = std::move(refs);
        t->cmdName = "Text";
        t->cmdArgs = a;
        t->update();
        made = std::move(t);
        return true;
    }

    // Integral(f, a, b) / Integral(f, b) → 定积分值 + 曲线下填充（批次④）。
    // 上限用 Integral(f, b) 形式时下限取 0；a/b 为数或滑动条（依随重算+重绘）
    if (cmdLower == "integral" && (a.size() == 2 || a.size() == 3)) {
        GeoFunction *f = resolveFunctionArg(a[0]);
        if (!f || !f->definable()) { lastError = "Integral 需要（函数）参数"; made = nullptr; return true; }
        GeoNumeric *na = dynamic_cast<GeoNumeric *>(lookup(a[1]));
        GeoNumeric *nb = a.size() == 3 ? dynamic_cast<GeoNumeric *>(lookup(a[2])) : nullptr;
        double va = 0, vb = 0;
        if (!numArg(a[1], va)) { lastError = "积分下限须为数或数值对象: " + a[1]; made = nullptr; return true; }
        if (a.size() == 3) {
            if (!numArg(a[2], vb)) { lastError = "积分上限须为数或数值对象: " + a[2]; made = nullptr; return true; }
        } else {
            vb = va;
            va = 0;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        auto fs = std::make_shared<GeoFillState>();
        fs->fn = f;
        fs->a = va;
        fs->b = vb;
        fs->on = true;
        n->fill = fs;
        GeoFunction *fn = f;
        n->calc = [fn, na, nb, va, vb, fs]() -> double {
            const double A = na ? na->value : va;
            const double B = nb ? nb->value : vb;
            fs->a = A;
            fs->b = B;
            const int m = 512;   // 复合 Simpson（偶数段）
            const double h = (B - A) / m;
            double s = fn->evalAt(A) + fn->evalAt(B);
            for (int i = 1; i < m; ++i)
                s += fn->evalAt(A + i * h) * (i % 2 ? 4 : 2);
            return s * h / 3.0;
        };
        n->cmdName = "Integral";
        n->cmdArgs = a;
        n->cmd = "Integral(" + a[0] + ", " + a[1] + (a.size() == 3 ? ", " + a[2] : "") + ")";
        n->inputs = { f };
        if (na) n->inputs.push_back(na);
        if (nb) n->inputs.push_back(nb);
        n->visible = true;   // 填充需要在画布显示
        n->update();
        made = std::move(n);
        return true;
    }

    // ---------- N6 批次③ 列表与统计 ----------

    // Sequence(expr, var, from, to[, step]) → 数值列表（expr 里的 var 为循环变量）
    if (cmdLower == "sequence" && (a.size() == 4 || a.size() == 5)) {
        Expr seq;
        std::string err;
        // 1.0.6.3 refs 模式：表达式可引用其他数值对象（滑动条等，GGB 语义
        // Sequence(k*a,k,1,5)），未知符号在下方校验给专用文案
        if (!seq.parse(a[0], err, a[1], true)) {
            lastError = err.empty() ? "序列表达式无法解析: " + a[0] : err;
            made = nullptr;
            return true;
        }
        if (seq.containsY()) {
            lastError = "序列表达式不能含 y";
            made = nullptr;
            return true;
        }
        // 1.0.6.3 动态化：起止/步长可为数值对象引用（含引用表达式如 n+1），
        // 端点变化经 calc 闭包重新烘焙；全字面量时保持原静态行为
        NumRefExpr bFrom, bTo, bStep;
        std::vector<GeoNumeric *> ndeps;
        const bool hasStep = a.size() == 5;
        if (!numRefArg(a[2], bFrom, ndeps)) {
            lastError = "序列起止须为数或数值对象";
            made = nullptr;
            return true;
        }
        if (!numRefArg(a[3], bTo, ndeps)) {
            lastError = "序列起止须为数或数值对象";
            made = nullptr;
            return true;
        }
        if (hasStep && !numRefArg(a[4], bStep, ndeps)) {
            lastError = "序列步长须为数或数值对象";
            made = nullptr;
            return true;
        }
        // 字面量步长为 0 维持既有报错；引用形式的 0 允许创建（置空列表，
        // 对象改值后重算恢复）
        if (hasStep && bStep.refs.empty() && bStep.evalNow() == 0.0) {
            lastError = "序列步长不能为 0";
            made = nullptr;
            return true;
        }
        // 表达式其余引用（循环变量以外）须为数值对象，进 deps 依随重算
        std::vector<std::pair<std::string, GeoNumeric *>> extra;
        for (const std::string &rn : seq.refs()) {
            GeoNumeric *dn = dynamic_cast<GeoNumeric *>(lookup(rn));
            if (!dn) {
                lastError = "序列表达式含未知对象 " + rn;
                made = nullptr;
                return true;
            }
            extra.push_back({ rn, dn });
            bool seen = false;
            for (GeoNumeric *d : ndeps) {
                if (d == dn) { seen = true; break; }
            }
            if (!seen) ndeps.push_back(dn);
        }
        auto bake = [seq, bFrom, bTo, bStep, hasStep, extra](GeoList *g) {
            const double from = bFrom.evalNow();
            const double to = bTo.evalNow();
            const double step = hasStep ? bStep.evalNow() : 1.0;
            g->values.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            // 步长失效（引用对象被改成 0/非有限）：置空列表而非死循环
            if (!std::isfinite(from) || !std::isfinite(to)
                || !std::isfinite(step) || step == 0) return;
            for (double v = from; step > 0 ? v <= to + 1e-9 : v >= to - 1e-9; v += step) {
                g->values.push_back(seq.evalResolved(v,
                    [&](const std::string &nm2) -> double {
                        for (const auto &pr : extra) {
                            if (pr.first == nm2) return pr.second->value;
                        }
                        return NAN;
                    }));
                if (g->values.size() >= 10000) break;   // 防失控
            }
        };
        auto L = std::make_unique<GeoList>();
        L->cmdName = "Sequence";
        L->cmdArgs = a;
        bake(L.get());
        if (!ndeps.empty()) {
            L->calc = bake;
            for (GeoNumeric *d : ndeps) L->inputs.push_back(d);
        }
        made = std::move(L);
        return true;
    }

    // Mean/Median/Min/Max/Sum/Product/SD/Variance(list) → 数值（依随列表；
    // 1.0.6.6 实参收 {…} 字面量——listArg 字面量产物为静态，不挂 inputs）
    if (cmdLower == "mean" || cmdLower == "median" || cmdLower == "min" || cmdLower == "max"
        || cmdLower == "sum" || cmdLower == "product" || cmdLower == "sd"
        || cmdLower == "variance") {
        if (a.size() != 1) {
            // Min/Max 两数形式（上游 支持 Max(3,7)）：回落表达式求值路径
            if ((cmdLower == "min" || cmdLower == "max") && a.size() == 2) {
                lastError.clear();
                return false;
            }
            lastError = "统计命令需要（列表）参数"; made = nullptr; return true;
        }
        bool lit = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { made = nullptr; return true; }
        const char *ggbName = cmdLower == "mean" ? "Mean"
                            : cmdLower == "median" ? "Median"
                            : cmdLower == "min" ? "Min"
                            : cmdLower == "max" ? "Max"
                            : cmdLower == "sum" ? "Sum"
                            : cmdLower == "product" ? "Product"
                            : cmdLower == "sd" ? "SD" : "Variance";
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        GeoList *src = L;
        const std::string kind = cmdLower;
        n->cmdName = ggbName;
        n->cmdArgs = { a[0] };
        n->cmd = ggbName + std::string("(") + a[0] + ")";
        if (lit) {
            n->value = ListStat(src->values, kind);   // 字面量静态：一次定型
        } else {
            n->calc = [src, kind]() { return ListStat(src->values, kind); };
            n->update();
            n->inputs = { L };
        }
        made = std::move(n);
        return true;
    }

    // Element(list, k) / First(list) / Last(list) → 数值（依随列表；k 为 1 起；
    // 1.0.6.6 列表实参收 {…} 字面量，静态定型）
    const bool listGet = (cmdLower == "element" && a.size() == 2)
        || (a.size() == 1 && (cmdLower == "first" || cmdLower == "last"));
    if (listGet) {
        bool lit = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { made = nullptr; return true; }
        double k = 1;
        const bool indexed = cmdLower == "element";
        if (indexed && !numArg(a[1], k)) {
            lastError = "元素序号须为数或数值对象: " + a[1];
            made = nullptr;
            return true;
        }
        const char *ggbName = indexed ? "Element" : cmdLower == "first" ? "First" : "Last";
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        GeoList *src = L;
        const double k0 = k;
        const bool isLast = cmdLower == "last";
        n->cmdName = ggbName;
        n->cmdArgs = a;
        n->cmd = std::string(ggbName) + "(" + a[0] + (indexed ? ", " + a[1] : "") + ")";
        if (lit) {
            if (src->values.empty()) {
                n->value = NAN;
            } else if (indexed) {
                const size_t i = (size_t)llround(k0);
                n->value = (i >= 1 && i <= src->values.size()) ? src->values[i - 1] : NAN;
            } else {
                n->value = isLast ? src->values.back() : src->values.front();
            }
        } else {
            n->calc = [src, k0, indexed, isLast]() -> double {
                if (src->values.empty()) return NAN;
                if (!indexed) return isLast ? src->values.back() : src->values.front();
                const size_t i = (size_t)llround(k0);
                if (i < 1 || i > src->values.size()) return NAN;
                return src->values[i - 1];
            };
            n->update();
            n->inputs = { L };
        }
        made = std::move(n);
        return true;
    }

    // Sort/Unique/Reverse(list) → 列表（依随源列表；1.0.6.6 收 {…} 字面量）
    if (cmdLower == "sort" || cmdLower == "unique" || cmdLower == "reverse") {
        if (a.size() != 1) { lastError = "命令需要（列表）参数"; made = nullptr; return true; }
        bool lit = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { made = nullptr; return true; }
        const char *ggbName = cmdLower == "sort" ? "Sort" : cmdLower == "unique" ? "Unique" : "Reverse";
        auto out = std::make_unique<GeoList>();
        GeoList *src = L;
        const std::string kind = cmdLower;
        auto bake = [src, kind](GeoList *g) {
            g->values = src->values;
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            if (kind == "sort") {
                std::sort(g->values.begin(), g->values.end());
            } else if (kind == "reverse") {
                std::reverse(g->values.begin(), g->values.end());
            } else {
                // unique：排序去重（上游 Unique 保序；实现按保序去重）
                std::vector<double> keep;
                for (double x : g->values) {
                    bool seen = false;
                    for (double y : keep) {
                        if (y == x) { seen = true; break; }
                    }
                    if (!seen) keep.push_back(x);
                }
                g->values = keep;
            }
        };
        out->cmdName = ggbName;
        out->cmdArgs = { a[0] };
        if (lit) {
            bake(out.get());   // 字面量静态：一次定型，不挂重算闭包
        } else {
            out->calc = bake;
            out->inputs = { L };
            out->update();
        }
        made = std::move(out);
        return true;
    }

    // Length(list) → 元素个数（两点/线段形式在上方分支处理；1.0.6.6 收字面量）
    if (cmdLower == "length" && a.size() == 1) {
        bool lit = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { lastError = "Length 需要（点, 点）/（线段/多边形）或（列表）"; made = nullptr; return true; }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        GeoList *src = L;
        n->cmdName = "Length";
        n->cmdArgs = { a[0] };
        n->cmd = "Length(" + a[0] + ")";
        if (lit) {
            n->value = (double)src->values.size();
        } else {
            n->calc = [src]() { return (double)src->values.size(); };
            n->update();
            n->inputs = { L };
        }
        made = std::move(n);
        return true;
    }

    // ---------- 1.0.6.3 列表指令补齐与图表指令 ----------
    // 实参列表支持标签或 {…} 字面量（字面量产物为静态，不挂重算闭包——
    // 临时字面量对象不入构造列表，闭包捕获会悬垂）；数值实参可含数值对象
    // 引用（进 deps 依随重算）。全标签形式挂 calc 闭包 + inputs
    auto finishList = [&](std::unique_ptr<GeoList> &out,
                          const std::function<void(GeoList *)> &bake, bool anyLit,
                          std::vector<GeoElement *> inputs) {
        bake(out.get());
        if (!anyLit && !inputs.empty()) {
            out->calc = bake;
            out->inputs = std::move(inputs);
        }
        made = std::move(out);
    };

    // Take(L, m[, n]) / Drop(L, m[, n])：1 起下标取段/去段（含端点）。
    // 两参形式：Take 取 m..末尾，Drop 去前 m 个。序号钳位进 [1, size]
    if ((cmdLower == "take" || cmdLower == "drop") && (a.size() == 2 || a.size() == 3)) {
        bool lit = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { made = nullptr; return true; }
        NumRefExpr bm, bn;
        std::vector<GeoNumeric *> ndeps;
        const char *nm = cmdLower == "take" ? "Take" : "Drop";
        if (!numRefArg(a[1], bm, ndeps)) {
            lastError = std::string(nm) + " 序号须为数或数值对象";
            made = nullptr; return true;
        }
        const bool hasEnd = a.size() == 3;
        if (hasEnd && !numRefArg(a[2], bn, ndeps)) {
            lastError = std::string(nm) + " 序号须为数或数值对象";
            made = nullptr; return true;
        }
        auto out = std::make_unique<GeoList>();
        out->cmdName = nm;
        out->cmdArgs = a;
        GeoList *src = L;
        const bool isTake = cmdLower == "take";
        auto bake = [src, bm, bn, hasEnd, isTake](GeoList *g) {
            g->values.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            const double mv = bm.evalNow();
            const double nv = hasEnd ? bn.evalNow() : 0.0;
            if (!std::isfinite(mv) || !std::isfinite(nv)) return;
            const long long sz = (long long)src->values.size();
            auto cl = [sz](double v) -> long long {
                long long i = (long long)llround(v);
                return i < 1 ? 1 : (i > sz ? sz : i);
            };
            if (isTake) {
                const long long lo = cl(mv);
                const long long hi = hasEnd ? cl(nv) : sz;
                for (long long i = lo; i <= hi; ++i) {
                    g->values.push_back(src->values[(size_t)(i - 1)]);
                }
            } else if (hasEnd) {
                const long long lo = cl(mv), hi = cl(nv);
                for (long long i = 1; i <= sz; ++i) {
                    if (i < lo || i > hi) g->values.push_back(src->values[(size_t)(i - 1)]);
                }
            } else {
                long long m = (long long)llround(mv);
                m = m < 0 ? 0 : (m > sz ? sz : m);
                for (long long i = m + 1; i <= sz; ++i) {
                    g->values.push_back(src->values[(size_t)(i - 1)]);
                }
            }
        };
        std::vector<GeoElement *> inputs;
        for (GeoNumeric *d : ndeps) inputs.push_back(d);
        inputs.push_back(L);
        finishList(out, bake, lit, std::move(inputs));
        return true;
    }

    // Insert(obj, L, pos)（GGB 官方参数序：Insert(x², {1,2,3,4,5}, 3)）：
    // obj 为数或列表（整体插入），pos 为 1 起插入位（钳位 [1, size+1]）。
    // obj 在前解析为数，解析不了按列表
    if (cmdLower == "insert" && a.size() == 3) {
        bool lit = false, litObj = false;
        GeoList *L = listArg(a[1], lit);
        if (!L) { made = nullptr; return true; }
        NumRefExpr bo, bp;
        std::vector<GeoNumeric *> ndeps;
        const bool objIsList = !numRefArg(a[0], bo, ndeps);
        GeoList *obj = nullptr;
        if (objIsList) {
            obj = listArg(a[0], litObj);
            if (!obj) { made = nullptr; return true; }
        }
        if (!numRefArg(a[2], bp, ndeps)) {
            lastError = "Insert 插入位置须为数或数值对象";
            made = nullptr; return true;
        }
        auto out = std::make_unique<GeoList>();
        out->cmdName = "Insert";
        out->cmdArgs = a;
        GeoList *src = L, *ins = obj;
        const bool asList = objIsList;
        auto bake = [src, ins, asList, bo, bp](GeoList *g) {
            g->values.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            const double posv = bp.evalNow();
            if (!std::isfinite(posv)) return;
            const long long sz = (long long)src->values.size();
            long long pos = (long long)llround(posv);
            if (pos < 1) pos = 1;
            if (pos > sz + 1) pos = sz + 1;
            for (long long i = 0; i < pos - 1; ++i) {
                g->values.push_back(src->values[(size_t)i]);
            }
            if (asList) {
                g->values.insert(g->values.end(), ins->values.begin(), ins->values.end());
            } else {
                g->values.push_back(bo.evalNow());
            }
            for (long long i = pos - 1; i < sz; ++i) {
                g->values.push_back(src->values[(size_t)i]);
            }
        };
        std::vector<GeoElement *> inputs;
        for (GeoNumeric *d : ndeps) inputs.push_back(d);
        inputs.push_back(L);
        if (asList) inputs.push_back(obj);
        finishList(out, bake, lit || litObj, std::move(inputs));
        return true;
    }

    // Remove(L1, L2)：从 L1 去掉 L2 中出现的元素（值相等）；L2 也可为单数
    if (cmdLower == "remove" && a.size() == 2) {
        bool lit = false, litObj = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { made = nullptr; return true; }
        NumRefExpr bv;
        std::vector<GeoNumeric *> ndeps;
        const bool valForm = numRefArg(a[1], bv, ndeps);
        GeoList *rm = nullptr;
        if (!valForm) {
            rm = listArg(a[1], litObj);
            if (!rm) { made = nullptr; return true; }
        }
        auto out = std::make_unique<GeoList>();
        out->cmdName = "Remove";
        out->cmdArgs = a;
        GeoList *src = L;
        auto bake = [src, rm, valForm, bv](GeoList *g) {
            g->values.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            if (valForm) {
                const double v = bv.evalNow();
                if (std::isfinite(v)) {
                    for (double x : src->values) {
                        if (x != v) g->values.push_back(x);
                    }
                } else {
                    g->values = src->values;
                }
            } else {
                for (double x : src->values) {
                    bool hit = false;
                    for (double y : rm->values) {
                        if (y == x) { hit = true; break; }
                    }
                    if (!hit) g->values.push_back(x);
                }
            }
        };
        std::vector<GeoElement *> inputs;
        for (GeoNumeric *d : ndeps) inputs.push_back(d);
        inputs.push_back(L);
        if (!valForm) inputs.push_back(rm);
        finishList(out, bake, lit || litObj, std::move(inputs));
        return true;
    }

    // Join(L1, L2, …)：顺次拼接（≥2 个列表）
    if (cmdLower == "join" && a.size() >= 2) {
        std::vector<GeoList *> srcs;
        bool anyLit = false;
        for (const std::string &s : a) {
            bool lit = false;
            GeoList *L = listArg(s, lit);
            if (!L) { made = nullptr; return true; }
            if (lit) anyLit = true;
            srcs.push_back(L);
        }
        auto out = std::make_unique<GeoList>();
        out->cmdName = "Join";
        out->cmdArgs = a;
        auto bake = [srcs](GeoList *g) {
            g->values.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            for (GeoList *s : srcs) {
                g->values.insert(g->values.end(), s->values.begin(), s->values.end());
            }
        };
        std::vector<GeoElement *> inputs(srcs.begin(), srcs.end());
        finishList(out, bake, anyLit, std::move(inputs));
        return true;
    }

    // Flatten(L)：矩阵/嵌套列表展开为行主序扁平列表。唯一允许矩阵/嵌套
    // 字面量实参的列表命令（listArg 拒矩阵，这里手解；字面量递归展开为
    // 静态产物，标签形式依随源列表）
    if (cmdLower == "flatten" && a.size() == 1) {
        auto out = std::make_unique<GeoList>();
        bool lit = false;
        GeoList *L = nullptr;
        if (!a[0].empty() && a[0].front() == '{') {
            std::string ferr;
            if (!ParseFlattenLiteral(a[0], out->values, ferr)) {
                lastError = ferr.empty() ? "Flatten 实参无法解析: " + a[0] : ferr;
                made = nullptr;
                return true;
            }
            lit = true;
        } else {
            L = dynamic_cast<GeoList *>(lookup(a[0]));
            if (!L) { lastError = "未找到列表 " + a[0]; made = nullptr; return true; }
        }
        out->cmdName = "Flatten";
        out->cmdArgs = a;
        GeoList *src = L;
        auto bake = [src](GeoList *g) {
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            if (src) g->values = src->values;   // 字面量分支：值已在解析时填好
        };
        std::vector<GeoElement *> fin;
        if (L) fin.push_back(L);
        finishList(out, bake, lit, std::move(fin));
        return true;
    }

    // Zip(表达式, 变量1, 列表1[, 变量2, 列表2…])：并行迭代（长度取最小），
    // 变量按名进表达式（refs 求值）；表达式其余引用须为数值对象
    if (cmdLower == "zip" && a.size() >= 3 && a.size() % 2 == 1) {
        Expr body;
        std::string err;
        if (!body.parse(a[0], err, "", true)) {
            lastError = err.empty() ? "Zip 表达式无法解析: " + a[0] : err;
            made = nullptr;
            return true;
        }
        if (body.containsX() || body.containsY()) {
            lastError = "Zip 表达式不能含 x/y";
            made = nullptr;
            return true;
        }
        const size_t np = (a.size() - 1) / 2;
        std::vector<std::string> vars;
        std::vector<GeoList *> srcs;
        bool anyLit = false;
        for (size_t i = 0; i < np; ++i) {
            vars.push_back(a[1 + 2 * i]);
            bool lit = false;
            GeoList *L = listArg(a[2 + 2 * i], lit);
            if (!L) { made = nullptr; return true; }
            if (lit) anyLit = true;
            srcs.push_back(L);
        }
        std::vector<std::pair<std::string, GeoNumeric *>> extra;
        for (const std::string &rn : body.refs()) {
            bool isVar = false;
            for (const std::string &v : vars) {
                if (v == rn) { isVar = true; break; }
            }
            if (isVar) continue;
            GeoNumeric *dn = dynamic_cast<GeoNumeric *>(lookup(rn));
            if (!dn) { lastError = "Zip 表达式含未知对象 " + rn; made = nullptr; return true; }
            extra.push_back({ rn, dn });
        }
        auto out = std::make_unique<GeoList>();
        out->cmdName = "Zip";
        out->cmdArgs = a;
        auto bake = [body, vars, srcs, extra](GeoList *g) {
            g->values.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            size_t n = srcs.empty() ? 0 : srcs[0]->values.size();
            for (GeoList *s : srcs) n = std::min(n, s->values.size());
            for (size_t i = 0; i < n; ++i) {
                g->values.push_back(body.evalResolved(0.0,
                    [&](const std::string &nm2) -> double {
                        for (size_t p = 0; p < vars.size(); ++p) {
                            if (vars[p] == nm2) return srcs[p]->values[i];
                        }
                        for (const auto &pr : extra) {
                            if (pr.first == nm2) return pr.second->value;
                        }
                        return NAN;
                    }));
            }
        };
        std::vector<GeoElement *> inputs(srcs.begin(), srcs.end());
        for (const auto &pr : extra) inputs.push_back(pr.second);
        finishList(out, bake, anyLit, std::move(inputs));
        return true;
    }

    // Frequency(L)：与 Unique(L) 同序（首现序）的各值出现次数
    if (cmdLower == "frequency" && a.size() == 1) {
        bool lit = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { made = nullptr; return true; }
        auto out = std::make_unique<GeoList>();
        out->cmdName = "Frequency";
        out->cmdArgs = a;
        GeoList *src = L;
        auto bake = [src](GeoList *g) {
            g->values.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            std::vector<double> uniq;
            for (double x : src->values) {
                bool seen = false;
                for (double y : uniq) {
                    if (y == x) { seen = true; break; }
                }
                if (!seen) uniq.push_back(x);
            }
            for (double u : uniq) {
                long long c = 0;
                for (double x : src->values) {
                    if (x == u) ++c;
                }
                g->values.push_back((double)c);
            }
        };
        finishList(out, bake, lit, { L });
        return true;
    }

    // 图表通用：可见（画布绘制）、默认蓝（同多边形）、依随源列表重算。
    // 收尾同 finishList（字面量实参 → 静态）
    auto finishChart = [&](std::unique_ptr<GeoList> &out,
                           const std::function<void(GeoList *)> &bake, bool anyLit,
                           std::vector<GeoElement *> inputs) {
        out->visible = true;
        out->color = {0x2C, 0x6B, 0xD4};
        finishList(out, bake, anyLit, std::move(inputs));
    };

    // BarChart(数据) / BarChart(横坐标, 高度[, 条宽])：单列表形式按 1..n
    // 定位；条宽缺省 0.8（chartB 存）
    if (cmdLower == "barchart" && (a.size() == 1 || a.size() == 2 || a.size() == 3)) {
        bool lit = false;
        GeoList *Lx = listArg(a[0], lit);
        if (!Lx) { made = nullptr; return true; }
        bool anyLit = lit;
        GeoList *Lh = nullptr;
        if (a.size() >= 2) {
            bool lit2 = false;
            Lh = listArg(a[1], lit2);
            if (!Lh) { made = nullptr; return true; }
            anyLit = anyLit || lit2;
        }
        NumRefExpr bw;
        std::vector<GeoNumeric *> ndeps;
        const bool hasW = a.size() == 3;
        if (hasW) {
            if (!numRefArg(a[2], bw, ndeps) || !(bw.evalNow() > 0)) {
                lastError = "BarChart 条宽须为正数";
                made = nullptr;
                return true;
            }
        }
        auto out = std::make_unique<GeoList>();
        out->chart = "bar";
        out->chartB = 0.8;
        out->cmdName = "BarChart";
        out->cmdArgs = a;
        GeoList *xs = Lx, *hs = Lh ? Lh : Lx;
        const bool twoLists = Lh != nullptr;
        auto bake = [xs, hs, twoLists, bw, hasW](GeoList *g) {
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            if (hasW) {
                const double w = bw.evalNow();
                g->chartB = (std::isfinite(w) && w > 0) ? w : 0.8;
            }
            if (twoLists) {
                const size_t n = std::min(xs->values.size(), hs->values.size());
                g->chartX.assign(xs->values.begin(), xs->values.begin() + (long)n);
                g->values.assign(hs->values.begin(), hs->values.begin() + (long)n);
            } else {
                g->chartX.clear();
                g->values = xs->values;
            }
        };
        std::vector<GeoElement *> inputs;
        for (GeoNumeric *d : ndeps) inputs.push_back(d);
        inputs.push_back(Lx);
        if (twoLists) inputs.push_back(Lh);
        finishChart(out, bake, anyLit, std::move(inputs));
        return true;
    }

    // Histogram(数据, 组数) / Histogram(组界, 数据)：前者等宽自动分组
    //（第二参可解析为数即此形式）；后者按组界 [b_i, b_{i+1}) 计数（末组
    // 含右端点），界外数据忽略
    if (cmdLower == "histogram" && a.size() == 2) {
        NumRefExpr bn;
        std::vector<GeoNumeric *> ndeps;
        const bool binsForm = numRefArg(a[1], bn, ndeps);
        bool lit = false, litD = false;
        GeoList *Lb = nullptr, *Ld = nullptr;
        if (binsForm) {
            const double nraw = bn.evalNow();
            const double nr = std::isfinite(nraw) ? llround(nraw) : 0;
            if (nr < 1 || nr > 1000) {
                lastError = "Histogram 组数须为 1..1000 的整数";
                made = nullptr;
                return true;
            }
            Ld = listArg(a[0], lit);
            if (!Ld) { made = nullptr; return true; }
        } else {
            Lb = listArg(a[0], lit);
            if (!Lb) { made = nullptr; return true; }
            Ld = listArg(a[1], litD);
            if (!Ld) { made = nullptr; return true; }
            for (size_t k = 0; k + 1 < Lb->values.size(); ++k) {
                if (Lb->values[k + 1] < Lb->values[k]) {
                    lastError = "Histogram 组界须升序";
                    made = nullptr;
                    return true;
                }
            }
            if (Lb->values.size() < 2) {
                lastError = "Histogram 组界至少两个";
                made = nullptr;
                return true;
            }
        }
        auto out = std::make_unique<GeoList>();
        out->chart = "hist";
        out->cmdName = "Histogram";
        out->cmdArgs = a;
        GeoList *sb = Lb, *sd = Ld;
        auto bake = [sb, sd, bn, binsForm](GeoList *g) {
            g->values.clear();
            g->chartX.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            const auto &d = sd->values;
            if (d.empty()) return;
            if (binsForm) {
                const double nraw = bn.evalNow();
                if (!std::isfinite(nraw)) return;
                long long n = (long long)llround(nraw);
                if (n < 1 || n > 1000) return;
                double mn = d[0], mx = d[0];
                for (double v : d) {
                    mn = std::min(mn, v);
                    mx = std::max(mx, v);
                }
                double w = (mx - mn) / (double)n;
                if (w < 1e-12) { mn -= 0.5; mx += 0.5; w = (mx - mn) / (double)n; }
                for (long long k = 0; k <= n; ++k) g->chartX.push_back(mn + w * (double)k);
                g->values.assign((size_t)n, 0.0);
                for (double v : d) {
                    long long b2 = (long long)floor((v - mn) / w);
                    if (b2 < 0) b2 = 0;
                    if (b2 >= n) b2 = n - 1;
                    g->values[(size_t)b2] += 1.0;
                }
            } else {
                const auto &b = sb->values;
                if (b.size() < 2) return;
                g->chartX = b;
                g->values.assign(b.size() - 1, 0.0);
                for (double v : d) {
                    for (size_t k = 0; k + 1 < b.size(); ++k) {
                        const bool last = k + 2 == b.size();
                        if (v >= b[k] && (last ? v <= b[k + 1] : v < b[k + 1])) {
                            g->values[k] += 1.0;
                            break;
                        }
                    }
                }
            }
        };
        std::vector<GeoElement *> inputs;
        for (GeoNumeric *d : ndeps) inputs.push_back(d);
        inputs.push_back(Ld);
        if (Lb) inputs.push_back(Lb);
        finishChart(out, bake, lit || litD, std::move(inputs));
        return true;
    }

    // LineGraph / ScatterPlot(x 列表, y 列表)：折线 / 散点（长度取最小）
    if ((cmdLower == "linegraph" || cmdLower == "scatterplot") && a.size() == 2) {
        bool lit1 = false, lit2 = false;
        GeoList *Lx = listArg(a[0], lit1);
        if (!Lx) { made = nullptr; return true; }
        GeoList *Ly = listArg(a[1], lit2);
        if (!Ly) { made = nullptr; return true; }
        auto out = std::make_unique<GeoList>();
        out->chart = cmdLower == "linegraph" ? "line" : "scatter";
        out->cmdName = cmdLower == "linegraph" ? "LineGraph" : "ScatterPlot";
        out->cmdArgs = a;
        auto bake = [Lx, Ly](GeoList *g) {
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            const size_t n = std::min(Lx->values.size(), Ly->values.size());
            g->chartX.assign(Lx->values.begin(), Lx->values.begin() + (long)n);
            g->values.assign(Ly->values.begin(), Ly->values.begin() + (long)n);
        };
        finishChart(out, bake, lit1 || lit2, { Lx, Ly });
        return true;
    }

    // BoxPlot(y 中心[, 箱高], 数据)：五数概括（线性插值分位数），values 存
    // {min, Q1, 中位数, Q3, max}，chartA/chartB 存中心 y 与箱高（缺省 1）
    if (cmdLower == "boxplot" && (a.size() == 2 || a.size() == 3)) {
        NumRefExpr bo, bs;
        std::vector<GeoNumeric *> ndeps;
        if (!numRefArg(a[0], bo, ndeps) || !std::isfinite(bo.evalNow())) {
            lastError = "BoxPlot 第一参数须为数（箱中心 y）";
            made = nullptr;
            return true;
        }
        const bool hasScale = a.size() == 3;
        if (hasScale && (!numRefArg(a[1], bs, ndeps) || !(bs.evalNow() > 0))) {
            lastError = "BoxPlot 箱高须为正数";
            made = nullptr;
            return true;
        }
        bool lit = false;
        GeoList *L = listArg(a[hasScale ? 2 : 1], lit);
        if (!L) { made = nullptr; return true; }
        auto out = std::make_unique<GeoList>();
        out->chart = "box";
        out->chartB = 1.0;
        out->cmdName = "BoxPlot";
        out->cmdArgs = a;
        GeoList *src = L;
        auto bake = [src, bo, bs, hasScale](GeoList *g) {
            g->values.clear();
            g->chartX.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            g->chartA = bo.evalNow();
            g->chartB = 1.0;
            if (hasScale) {
                const double s = bs.evalNow();
                g->chartB = (std::isfinite(s) && s > 0) ? s : 1.0;
            }
            if (src->values.empty()) return;
            std::vector<double> s2(src->values);
            std::sort(s2.begin(), s2.end());
            auto quant = [&s2](double p) -> double {
                const double pos = p * (double)(s2.size() - 1);
                const size_t lo = (size_t)floor(pos);
                const size_t hi = (size_t)ceil(pos);
                const double t = pos - (double)lo;
                return s2[lo] * (1.0 - t) + s2[hi] * t;
            };
            g->values = { s2.front(), quant(0.25), quant(0.5), quant(0.75), s2.back() };
        };
        std::vector<GeoElement *> inputs;
        for (GeoNumeric *d : ndeps) inputs.push_back(d);
        inputs.push_back(L);
        finishChart(out, bake, lit, std::move(inputs));
        return true;
    }

    // StemPlot(数据[, adjustment])：茎叶图（文本形态，画布左上固定区绘制）。
    // adjustment d 把数据 ×10^d 取整对齐（小数数据 d>0、过大数据 d<0），
    // values 存调整后的排序数据
    if (cmdLower == "stemplot" && (a.size() == 1 || a.size() == 2)) {
        NumRefExpr badj;
        std::vector<GeoNumeric *> ndeps;
        const bool hasAdj = a.size() == 2;
        if (hasAdj && !numRefArg(a[1], badj, ndeps)) {
            lastError = "StemPlot 调整位数须为数";
            made = nullptr;
            return true;
        }
        bool lit = false;
        GeoList *L = listArg(a[0], lit);
        if (!L) { made = nullptr; return true; }
        auto out = std::make_unique<GeoList>();
        out->chart = "stem";
        out->cmdName = "StemPlot";
        out->cmdArgs = a;
        GeoList *src = L;
        auto bake = [src, badj, hasAdj](GeoList *g) {
            g->chartX.clear();
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            double adj = 0;
            if (hasAdj) {
                const double a2 = badj.evalNow();
                adj = std::isfinite(a2) ? (double)llround(a2) : 0.0;
            }
            if (adj > 30) adj = 30;
            if (adj < -30) adj = -30;
            g->chartA = adj;
            const double mul = pow(10.0, adj);
            g->values.clear();
            for (double v : src->values) g->values.push_back(v * mul);
            std::sort(g->values.begin(), g->values.end());
        };
        std::vector<GeoElement *> inputs;
        for (GeoNumeric *d : ndeps) inputs.push_back(d);
        inputs.push_back(L);
        finishChart(out, bake, lit, std::move(inputs));
        return true;
    }

    // ---------- 1.0.6.1 回归批（Fit 族）与矩阵代数 ----------
    // 输入形态对齐上游：点序列 = 点列表（列表命令走 tryCommand 时实参只有
    // 列表标签）或两个数值列表（x/y 各一）。内核列表是纯数值（点列表在
    // 本内核无对应物），点序列按「两个等长数值列表」解释——FitLine({1,2},
    // {3,5}) 即回归 y on x。官方 .ggb 保存的点列表重放为 Expression
    // {A1,A2,…}（本内核点无法进列表），这类文件落到「未找到列表」的既有
    // 报错，不静默错拟

    // FitLine(x 列表, y 列表) → 直线（y on x 最小二乘）。依随两列表重算。
    // 直线 ax+by=c 与上游 AlgoFitLineY 同式（Sxy/Sxx 消除除法的系数形）
    if (cmdLower == "fitline" && a.size() == 2) {
        // 1.0.7.1 实参收 {…} 字面量（GGB 同款内嵌数据式）：字面量产物为
        // 静态（闭包捕获冻结副本），标签实参维持活引用
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        auto l = std::make_unique<GeoLine>();
        l->mode = GeoLine::Mode::Fixed;
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        l->calc = [xvals, yvals](GeoLine *g) {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            const size_t n = std::min(XV.size(), YV.size());
            double sumx = 0, sumy = 0, sumxx = 0, sumxy = 0;
            for (size_t i = 0; i < n; ++i) {
                const double x = XV[i], y = YV[i];
                sumx += x; sumy += y; sumxx += x * x; sumxy += x * y;
            }
            if (n < 2) { g->a = 0; g->b = 1; g->c = 0; return; }
            const double Sxy = sumxy - sumx * sumy / n;
            const double Sxx = sumxx - sumx * sumx / n;
            const double mux = sumx / n, muy = sumy / n;
            if (fabs(Sxx) < 1e-12) {
                // x 全同：最小二乘直线退化为竖直线 x = mux（Sxy 同为 0 时
                // 上游 AlgoFitLineY 判未定义；几何上竖直线是唯一解）
                g->a = 1;
                g->b = 0;
                g->c = mux;
                g->verticalConst = true;
                return;
            }
            // y - muy = (Sxy/Sxx)(x - mux) → Sxy·x − Sxx·y = Sxy·mux − Sxx·muy
            g->verticalConst = false;
            g->a = Sxy;
            g->b = -Sxx;
            g->c = Sxy * mux - Sxx * muy;
        };
        l->cmdName = "FitLine";
        l->cmdArgs = { a[0], a[1] };
        if (!litX && !litY) l->inputs = { Lx, Ly };
        l->update();
        made = std::move(l);
        return true;
    }

    // FitPoly(x 列表, y 列表, 阶数) → 多项式回归函数（1..8 阶；正规方程）。
    // 依随两列表与阶数对象重算；重算后 src/body 整体重建
    if (cmdLower == "fitpoly" && (a.size() == 2 || a.size() == 3)) {
        // 1.0.7.1 实参收 {…} 字面量（GGB 同款内嵌数据式）：字面量产物为
        // 静态（闭包捕获冻结副本），标签实参维持活引用
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        double degv = 1;
        GeoNumeric *degObj = nullptr;
        if (a.size() == 3) {
            degObj = dynamic_cast<GeoNumeric *>(lookup(a[2]));
            if (!degObj && !numArg(a[2], degv)) {
                lastError = "阶数须为数或数值对象: " + a[2];
                made = nullptr;
                return true;
            }
            if (degObj) degv = degObj->value;
        }
        const int deg = (int)llround(degv);
        if (deg < 1 || deg > 8) {
            lastError = "阶数须为 1..8 的整数: " + (a.size() == 3 ? a[2] : std::string("(缺省)"));
            made = nullptr;
            return true;
        }
        const int deg0 = deg;
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        GeoNumeric *dobj = degObj;
        // 范数化拟合（x 减均值除标准差，改善消元条件数）；dg=1..8
        auto fitNorm = [xvals, yvals](int dg, std::vector<double> &p) -> bool {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            const size_t n = std::min(XV.size(), YV.size());
            if (n < 2 || (int)n < dg + 1) return false;
            double mx = 0;
            for (size_t i = 0; i < n; ++i) mx += XV[i];
            mx /= n;
            double vx = 0;
            for (size_t i = 0; i < n; ++i) vx += (XV[i] - mx) * (XV[i] - mx);
            vx = sqrt(vx / n);
            if (vx < 1e-12) return false;   // x 全同（竖直线，非函数）
            std::vector<std::vector<double>> M(n, std::vector<double>(dg + 1));
            std::vector<double> y(n);
            for (size_t i = 0; i < n; ++i) {
                const double t = (XV[i] - mx) / vx;
                double b = 1;
                for (int j = 0; j <= dg; ++j) { M[i][j] = b; b *= t; }
                y[i] = YV[i];
            }
            return SolveNormalEquations(M, y, p);
        };
        // 范数化系数 → 原始 x 系数：(t = (x-mx)/vx) 按二项式展开回 x 的幂
        auto normToRaw = [xvals](int dg, const std::vector<double> &pn) {
            const std::vector<double> &XV = xvals();
            double mx = 0;
            const size_t n = XV.size();
            for (size_t i = 0; i < n; ++i) mx += XV[i];
            mx /= n;
            double vx = 0;
            for (size_t i = 0; i < n; ++i) vx += (XV[i] - mx) * (XV[i] - mx);
            vx = sqrt(vx / n);
            std::vector<double> raw(dg + 1, 0.0);
            for (int i = 0; i <= dg; ++i) {
                // t^i = Σ C(i,k) (-mx)^(i-k) x^k / vx^i（t=(x-mx)/vx 按二项式展开）
                double cm = 1;   // C(i,k)
                for (int k = 0; k <= i; ++k) {
                    raw[k] += pn[i] * cm * pow(-mx, i - k) / pow(vx, i);
                    cm = cm * (i - k) / (k + 1);
                }
            }
            return raw;
        };
        std::vector<double> p0;
        if (!fitNorm(deg0, p0)) {
            lastError = "FitPoly 求解失败（点数不足 / x 全同 / 奇异）";
            made = nullptr;
            return true;
        }
        const std::vector<double> raw0 = normToRaw(deg0, p0);
        d->src = PolyCoeffSrc(raw0);
        {
            std::string err;
            d->body.parse(d->src, err);
        }
        // 依随重算走 alt 闭包（Derivative 同款）：求值前比对两列表内容与
        // 阶数，变了就重解并记忆化。七十三包：重解后同步重建 src/body——
        // 二者原本只承载初始拟合，数据/阶数变化后代数区仍显示旧方程；
        // unique_ptr 转移所有权不搬移点对象，self 与 geos_ 中实体同址
        auto cache = std::make_shared<std::vector<double>>();   // raw 系数（空=无效）
        auto cacheKey = std::make_shared<std::vector<double>>();
        GeoFunction *self = d.get();
        d->alt = [xvals, yvals, dobj, deg0, fitNorm, normToRaw, cache, cacheKey, self](double x) -> double {
            const int dg = dobj ? std::max(1, std::min(8, (int)llround(dobj->value))) : deg0;
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            std::vector<double> nk;
            nk.reserve(XV.size() + YV.size() + 1);
            nk.insert(nk.end(), XV.begin(), XV.end());
            nk.insert(nk.end(), YV.begin(), YV.end());
            nk.push_back((double)dg);
            if (cache->empty() || *cacheKey != nk) {
                std::vector<double> p;
                if (!fitNorm(dg, p)) return NAN;
                *cache = normToRaw(dg, p);
                *cacheKey = nk;
                self->src = PolyCoeffSrc(*cache);
                std::string perr;
                self->body.parse(self->src, perr);
            }
            double r = 0;   // Horner（cache 内为常数项…x^dg 项）
            for (auto it = cache->rbegin(); it != cache->rend(); ++it) r = r * x + *it;
            return r;
        };
        d->cmdName = "FitPoly";
        d->cmdArgs = a;
        if (!litX && !litY) d->inputs = { Lx, Ly };
        if (degObj) d->inputs.push_back(degObj);
        d->update();
        made = std::move(d);
        return true;
    }

    // FitExp(x 列表, y 列表) → a·e^(b·x)（ln(y) 线性化最小二乘，上游 同式：
    // y≤0 的点按首个 y 的符号取绝对值，仍负则失败）
    if (cmdLower == "fitexp" && a.size() == 2) {
        // 1.0.7.1 实参收 {…} 字面量（GGB 同款内嵌数据式）：字面量产物为
        // 静态（闭包捕获冻结副本），标签实参维持活引用
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        // 先解一次拿 a/b 定 src（失败即拒）
        double A0 = 0, B0 = 0;
        {
            double sx = 0, sy = 0, sxx = 0, sxy = 0;
            size_t n = 0;
            const double sign = (!Ly->values.empty() && Ly->values[0] < 0) ? -1.0 : 1.0;
            for (size_t i = 0; i < Lx->values.size() && i < Ly->values.size(); ++i) {
                const double yv = Ly->values[i] * sign;
                if (yv <= 0) break;
                const double ly = log(yv);
                sx += Lx->values[i]; sy += ly; sxx += Lx->values[i] * Lx->values[i];
                sxy += Lx->values[i] * ly;
                ++n;
            }
            if (n < 2 || n != std::min(Lx->values.size(), Ly->values.size())) {
                lastError = "FitExp 需要至少 2 个 y>0（或同号）的数据点";
                made = nullptr;
                return true;
            }
            const double den = n * sxx - sx * sx;
            if (fabs(den) < 1e-12) { lastError = "FitExp 求解失败（x 全同）"; made = nullptr; return true; }
            B0 = (n * sxy - sx * sy) / den;
            A0 = (sy - B0 * sx) / n;
            A0 = exp(A0) * sign;
        }
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        // a/b 双值缓存（shared_ptr 保闭包悬垂安全）：alt 求值前先重解
        // （内容键比对去抖），两列表任一重算后拟合自动跟随
        auto ab = std::make_shared<std::pair<double, double>>(A0, B0);
        auto key = std::make_shared<std::vector<double>>();
        GeoFunction *self = d.get();
        d->alt = [xvals, yvals, ab, key, self](double x) -> double {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            std::vector<double> nk;
            nk.reserve(XV.size() + YV.size());
            nk.insert(nk.end(), XV.begin(), XV.end());
            nk.insert(nk.end(), YV.begin(), YV.end());
            if (*key != nk) {
                size_t n = 0;
                double s1 = 0, s2 = 0, s3 = 0, s4 = 0;
                const double sign = (!YV.empty() && YV[0] < 0) ? -1.0 : 1.0;
                for (size_t i = 0; i < XV.size() && i < YV.size(); ++i) {
                    const double yv = YV[i] * sign;
                    if (yv <= 0) return NAN;
                    const double ly = log(yv);
                    s1 += XV[i]; s2 += ly; s3 += XV[i] * XV[i];
                    s4 += XV[i] * ly;
                    ++n;
                }
                if (n >= 2) {
                    const double den = n * s3 - s1 * s1;
                    if (fabs(den) >= 1e-12) {
                        ab->second = (n * s4 - s1 * s2) / den;
                        ab->first = exp((s2 - ab->second * s1) / n) * sign;
                        // 七十三包：重解成功才同步代数区显示（失败沿用旧拟合，
                        // 与求值取旧 ab 一致）
                        self->src = PolyCoeffSrc({ab->first}) + "*e^("
                                  + PolyCoeffSrc({ab->second}) + "*x)";
                    }
                }
                *key = std::move(nk);
            }
            return ab->first * exp(ab->second * x);
        };
        d->src = PolyCoeffSrc({A0}) + "*e^(" + PolyCoeffSrc({B0}) + "*x)";
        d->cmdName = "FitExp";
        d->cmdArgs = { a[0], a[1] };
        if (!litX && !litY) d->inputs = { Lx, Ly };
        d->update();
        made = std::move(d);
        return true;
    }

    // FitGrowth(x 列表, y 列表) → a·b^x（对数线性：ln y = ln a + x·ln b，
    // 需同号 y；拟合口径与 FitExp 同款，内容键去抖重解）
    if (cmdLower == "fitgrowth" && a.size() == 2) {
        // 1.0.7.1 实参收 {…} 字面量（GGB 同款内嵌数据式）：字面量产物为
        // 静态（闭包捕获冻结副本），标签实参维持活引用
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        double A0 = 0, B0 = 0;
        {
            double s1 = 0, s2 = 0, s3 = 0, s4 = 0;
            size_t n = 0;
            const double sign = (!Ly->values.empty() && Ly->values[0] < 0) ? -1.0 : 1.0;
            for (size_t i = 0; i < Lx->values.size() && i < Ly->values.size(); ++i) {
                const double yv = Ly->values[i] * sign;
                if (yv <= 0) break;
                const double ly = log(yv);
                s1 += Lx->values[i]; s2 += ly; s3 += Lx->values[i] * Lx->values[i];
                s4 += Lx->values[i] * ly;
                ++n;
            }
            if (n < 2 || n != std::min(Lx->values.size(), Ly->values.size())) {
                lastError = "FitGrowth 需要至少 2 个 y>0（或同号）的数据点";
                made = nullptr;
                return true;
            }
            const double den = n * s3 - s1 * s1;
            if (fabs(den) < 1e-12) { lastError = "FitGrowth 求解失败（x 全同）"; made = nullptr; return true; }
            B0 = (n * s4 - s1 * s2) / den;
            A0 = exp((s2 - B0 * s1) / n) * sign;
        }
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        auto ab = std::make_shared<std::pair<double, double>>(A0, B0);
        auto key = std::make_shared<std::vector<double>>();
        GeoFunction *self = d.get();
        d->alt = [xvals, yvals, ab, key, self](double x) -> double {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            std::vector<double> nk;
            nk.reserve(XV.size() + YV.size());
            nk.insert(nk.end(), XV.begin(), XV.end());
            nk.insert(nk.end(), YV.begin(), YV.end());
            if (*key != nk) {
                size_t n = 0;
                double s1 = 0, s2 = 0, s3 = 0, s4 = 0;
                const double sign = (!YV.empty() && YV[0] < 0) ? -1.0 : 1.0;
                for (size_t i = 0; i < XV.size() && i < YV.size(); ++i) {
                    const double yv = YV[i] * sign;
                    if (yv <= 0) return NAN;
                    const double ly = log(yv);
                    s1 += XV[i]; s2 += ly; s3 += XV[i] * XV[i];
                    s4 += XV[i] * ly;
                    ++n;
                }
                if (n >= 2) {
                    const double den = n * s3 - s1 * s1;
                    if (fabs(den) >= 1e-12) {
                        ab->second = (n * s4 - s1 * s2) / den;
                        ab->first = exp((s2 - ab->second * s1) / n) * sign;
                        self->src = PolyCoeffSrc({ab->first}) + "*"
                                  + PolyCoeffSrc({exp(ab->second)}) + "^x";
                    }
                }
                *key = std::move(nk);
            }
            return ab->first * exp(ab->second * x);
        };
        d->src = PolyCoeffSrc({A0}) + "*" + PolyCoeffSrc({exp(B0)}) + "^x";
        d->cmdName = "FitGrowth";
        d->cmdArgs = { a[0], a[1] };
        if (!litX && !litY) d->inputs = { Lx, Ly };
        d->update();
        made = std::move(d);
        return true;
    }

    // FitLog(x 列表, y 列表) → a + b·ln(x)（ln x 线性化最小二乘；x>0）。
    // 1.0.7.1 回归分析面板配套补齐（GGB 对数回归同款）；字面量冻结/活引用同上
    if (cmdLower == "fitlog" && a.size() == 2) {
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        auto solve = [](const std::vector<double> &XV, const std::vector<double> &YV,
                        double &A, double &B) -> bool {
            size_t n = 0;
            double s1 = 0, s2 = 0, s3 = 0, s4 = 0;
            for (size_t i = 0; i < XV.size() && i < YV.size(); ++i) {
                if (XV[i] <= 0) return false;   // ln 定义域
                const double lx = log(XV[i]);
                s1 += lx; s2 += YV[i]; s3 += lx * lx; s4 += lx * YV[i];
                ++n;
            }
            if (n < 2 || n != std::min(XV.size(), YV.size())) return false;
            const double den = n * s3 - s1 * s1;
            if (fabs(den) < 1e-12) return false;   // ln x 全同
            B = (n * s4 - s1 * s2) / den;
            A = (s2 - B * s1) / n;
            return std::isfinite(A) && std::isfinite(B);
        };
        double A0 = 0, B0 = 0;
        if (!solve(Lx->values, Ly->values, A0, B0)) {
            lastError = "FitLog 需要 ≥2 个 x>0 的数据点（ln x 不全同）";
            made = nullptr;
            return true;
        }
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        auto ab = std::make_shared<std::pair<double, double>>(A0, B0);
        auto key = std::make_shared<std::vector<double>>();
        GeoFunction *self = d.get();
        d->alt = [xvals, yvals, ab, key, self, solve](double x) -> double {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            std::vector<double> nk;
            nk.reserve(XV.size() + YV.size());
            nk.insert(nk.end(), XV.begin(), XV.end());
            nk.insert(nk.end(), YV.begin(), YV.end());
            if (*key != nk) {
                double A, B;
                if (solve(XV, YV, A, B)) {
                    *ab = { A, B };
                    self->src = PolyCoeffSrc({A}) + "+" + PolyCoeffSrc({B}) + "*ln(x)";
                }
                *key = std::move(nk);
            }
            return x <= 0 ? NAN : ab->first + ab->second * log(x);
        };
        d->src = PolyCoeffSrc({A0}) + "+" + PolyCoeffSrc({B0}) + "*ln(x)";
        d->cmdName = "FitLog";
        d->cmdArgs = { a[0], a[1] };
        if (!litX && !litY) d->inputs = { Lx, Ly };
        d->update();
        made = std::move(d);
        return true;
    }

    // FitPow(x 列表, y 列表) → a·x^b（ln x/ln y 线性化最小二乘；x>0、y 同号，
    // 负号整体提出同 FitExp/FitGrowth 约定）
    if (cmdLower == "fitpow" && a.size() == 2) {
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        auto solve = [](const std::vector<double> &XV, const std::vector<double> &YV,
                        double &A, double &B) -> bool {
            size_t n = 0;
            double s1 = 0, s2 = 0, s3 = 0, s4 = 0;
            const double sign = (!YV.empty() && YV[0] < 0) ? -1.0 : 1.0;
            for (size_t i = 0; i < XV.size() && i < YV.size(); ++i) {
                if (XV[i] <= 0 || YV[i] * sign <= 0) return false;
                const double lx = log(XV[i]), ly = log(YV[i] * sign);
                s1 += lx; s2 += ly; s3 += lx * lx; s4 += lx * ly;
                ++n;
            }
            if (n < 2 || n != std::min(XV.size(), YV.size())) return false;
            const double den = n * s3 - s1 * s1;
            if (fabs(den) < 1e-12) return false;
            B = (n * s4 - s1 * s2) / den;
            A = exp((s2 - B * s1) / n) * sign;
            return std::isfinite(A) && std::isfinite(B);
        };
        double A0 = 0, B0 = 0;
        if (!solve(Lx->values, Ly->values, A0, B0)) {
            lastError = "FitPow 需要 ≥2 个 x>0 且同号 y 的数据点";
            made = nullptr;
            return true;
        }
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        auto ab = std::make_shared<std::pair<double, double>>(A0, B0);
        auto key = std::make_shared<std::vector<double>>();
        GeoFunction *self = d.get();
        d->alt = [xvals, yvals, ab, key, self, solve](double x) -> double {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            std::vector<double> nk;
            nk.reserve(XV.size() + YV.size());
            nk.insert(nk.end(), XV.begin(), XV.end());
            nk.insert(nk.end(), YV.begin(), YV.end());
            if (*key != nk) {
                double A, B;
                if (solve(XV, YV, A, B)) {
                    *ab = { A, B };
                    self->src = PolyCoeffSrc({A}) + "*x^" + PolyCoeffSrc({B});
                }
                *key = std::move(nk);
            }
            return x <= 0 ? NAN : ab->first * pow(x, ab->second);
        };
        d->src = PolyCoeffSrc({A0}) + "*x^" + PolyCoeffSrc({B0});
        d->cmdName = "FitPow";
        d->cmdArgs = { a[0], a[1] };
        if (!litX && !litY) d->inputs = { Lx, Ly };
        d->update();
        made = std::move(d);
        return true;
    }

    // FitSin(x 列表, y 列表) → a + b·sin(c·x + d)（均采样假设下 DFT 主频定 c，
    // 固定 c 线性最小二乘解 [a, b·cos d, b·sin d]；内容键去抖重解）
    if (cmdLower == "fitsin" && a.size() == 2) {
        // 1.0.7.1 实参收 {…} 字面量（GGB 同款内嵌数据式）：字面量产物为
        // 静态（闭包捕获冻结副本），标签实参维持活引用
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        if (std::min(Lx->values.size(), Ly->values.size()) < 4) {
            lastError = "FitSin 需要至少 4 个数据点";
            made = nullptr;
            return true;
        }
        auto fit = [](const std::vector<double> &XV, const std::vector<double> &YV,
                      double &A, double &B, double &C, double &D) -> bool {
            const size_t n = std::min(XV.size(), YV.size());
            if (n < 4 || n > 8192) return false;
            double x0 = XV[0], x1 = XV[0];
            for (size_t i = 0; i < n; ++i) {
                x0 = std::min(x0, XV[i]);
                x1 = std::max(x1, XV[i]);
            }
            const double dx = (x1 - x0) / (double)(n - 1);
            if (!(dx > 1e-12)) return false;   // x 全同/乱序采样
            double my = 0;
            for (size_t i = 0; i < n; ++i) my += YV[i];
            my /= (double)n;
            // DFT 主频（k=1..n/2）：幅值最大者 → c = 2πk/(n·Δx)
            const double kPi2 = 6.28318530717958647692;
            double bestAmp = -1.0;
            long long bestK = 1;
            for (long long k = 1; k <= (long long)(n / 2); ++k) {
                double re = 0, im = 0;
                for (size_t i = 0; i < n; ++i) {
                    const double ph = -kPi2 * (double)k * (double)i / (double)n;
                    const double v = YV[i] - my;
                    re += v * cos(ph);
                    im += v * sin(ph);
                }
                const double amp = re * re + im * im;
                if (amp > bestAmp) { bestAmp = amp; bestK = k; }
            }
            const double c = kPi2 * (double)bestK / ((double)n * dx);
            // 固定 c：y = a + B1·sin(cx) + B2·cos(cx) 线性最小二乘
            std::vector<std::vector<double>> M(n, std::vector<double>(3));
            std::vector<double> y(n);
            for (size_t i = 0; i < n; ++i) {
                M[i][0] = 1.0;
                M[i][1] = sin(c * XV[i]);
                M[i][2] = cos(c * XV[i]);
                y[i] = YV[i];
            }
            std::vector<double> p;
            if (!SolveNormalEquations(M, y, p)) return false;
            A = p[0];
            B = hypot(p[1], p[2]);
            D = atan2(p[2], p[1]);
            C = c;
            return true;
        };
        double A0 = 0, B0 = 0, C0 = 0, D0 = 0;
        if (!fit(Lx->values, Ly->values, A0, B0, C0, D0)) {
            lastError = "FitSin 求解失败（点数不足 / x 全同 / 点数超上限）";
            made = nullptr;
            return true;
        }
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        auto abcd = std::make_shared<std::array<double, 4>>(std::array<double, 4>{A0, B0, C0, D0});
        auto key = std::make_shared<std::vector<double>>();
        GeoFunction *self = d.get();
        d->alt = [xvals, yvals, abcd, key, self, fit](double x) -> double {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            std::vector<double> nk;
            nk.reserve(XV.size() + YV.size());
            nk.insert(nk.end(), XV.begin(), XV.end());
            nk.insert(nk.end(), YV.begin(), YV.end());
            if (*key != nk) {
                double A, B, C, D;
                if (fit(xvals(), yvals(), A, B, C, D)) {
                    (*abcd)[0] = A; (*abcd)[1] = B; (*abcd)[2] = C; (*abcd)[3] = D;
                    self->src = PolyCoeffSrc({A}) + "+" + PolyCoeffSrc({B}) + "*sin("
                              + PolyCoeffSrc({C}) + "*x+" + PolyCoeffSrc({D}) + ")";
                }
                *key = std::move(nk);
            }
            const auto &v = *abcd;
            return v[0] + v[1] * sin(v[2] * x + v[3]);
        };
        d->src = PolyCoeffSrc({A0}) + "+" + PolyCoeffSrc({B0}) + "*sin("
               + PolyCoeffSrc({C0}) + "*x+" + PolyCoeffSrc({D0}) + ")";
        d->cmdName = "FitSin";
        d->cmdArgs = { a[0], a[1] };
        if (!litX && !litY) d->inputs = { Lx, Ly };
        d->update();
        made = std::move(d);
        return true;
    }

    // FitLogistic(x 列表, y 列表) → a/(1 + b·e^(−k·x))（GGB 同形；y>0。
    // 固定 k 时 z=1/y 对 [1, e^(−kx)] 线性回归，k 在对数域黄金分割，
    // 以 y 空间 SSE 排序；内容键去抖重解）
    if (cmdLower == "fitlogistic" && a.size() == 2) {
        // 1.0.7.1 实参收 {…} 字面量（GGB 同款内嵌数据式）：字面量产物为
        // 静态（闭包捕获冻结副本），标签实参维持活引用
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        if (std::min(Lx->values.size(), Ly->values.size()) < 3) {
            lastError = "FitLogistic 需要至少 3 个 y>0 的数据点";
            made = nullptr;
            return true;
        }
        for (size_t i = 0; i < Ly->values.size(); ++i) {
            if (Ly->values[i] <= 0) {
                lastError = "FitLogistic 需要 y>0 的数据点";
                made = nullptr;
                return true;
            }
        }
        auto fit = [](const std::vector<double> &XV, const std::vector<double> &YV,
                      double &A, double &B, double &K) -> bool {
            const size_t n = std::min(XV.size(), YV.size());
            if (n < 3) return false;
            // SSE(k)：z=1/y 线性回归（α=1/a、β=b/a）后回 y 空间算残差
            auto sseAt = [&](double k, double &aOut, double &bOut) -> double {
                std::vector<std::vector<double>> M(n, std::vector<double>(2));
                std::vector<double> z(n);
                for (size_t i = 0; i < n; ++i) {
                    M[i][0] = 1.0;
                    M[i][1] = exp(-k * XV[i]);
                    z[i] = 1.0 / YV[i];
                }
                std::vector<double> p;
                if (!SolveNormalEquations(M, z, p)) return HUGE_VAL;
                const double a = 1.0 / p[0];
                const double b = p[1] / p[0];
                if (!std::isfinite(a) || !std::isfinite(b) || a == 0.0) return HUGE_VAL;
                double sse = 0;
                for (size_t i = 0; i < n; ++i) {
                    const double f = a / (1.0 + b * exp(-k * XV[i]));
                    const double dv = f - YV[i];
                    sse += dv * dv;
                }
                aOut = a;
                bOut = b;
                return sse;
            };
            // 黄金分割极小化 ln k ∈ [ln 1e-6, ln 1e4]
            double lo = log(1e-6), hi = log(1e4);
            const double gr = 0.6180339887498949;
            double c2 = hi - gr * (hi - lo), d2 = lo + gr * (hi - lo);
            double a1 = 0, b1 = 0, a2 = 0, b2 = 0;
            double fc = sseAt(exp(c2), a1, b1), fd = sseAt(exp(d2), a2, b2);
            for (int it = 0; it < 80 && hi - lo > 1e-9; ++it) {
                if (fc < fd) {
                    hi = d2; d2 = c2; fd = fc;
                    c2 = hi - gr * (hi - lo);
                    fc = sseAt(exp(c2), a1, b1);
                } else {
                    lo = c2; c2 = d2; fc = fd;
                    d2 = lo + gr * (hi - lo);
                    fd = sseAt(exp(d2), a2, b2);
                }
            }
            const double k = exp(0.5 * (lo + hi));
            double aOut, bOut;
            if (sseAt(k, aOut, bOut) >= HUGE_VAL) return false;
            A = aOut;
            B = bOut;
            K = k;
            return std::isfinite(A) && std::isfinite(B) && std::isfinite(K);
        };
        double A0 = 0, B0 = 0, K0 = 0;
        if (!fit(Lx->values, Ly->values, A0, B0, K0)) {
            lastError = "FitLogistic 求解失败（数据不适合 Logistic 形）";
            made = nullptr;
            return true;
        }
        auto d = std::make_unique<GeoFunction>();
        d->color = {0x8B, 0x45, 0x13};
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        auto abk = std::make_shared<std::array<double, 3>>(std::array<double, 3>{A0, B0, K0});
        auto key = std::make_shared<std::vector<double>>();
        GeoFunction *self = d.get();
        d->alt = [xvals, yvals, abk, key, self, fit](double x) -> double {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            std::vector<double> nk;
            nk.reserve(XV.size() + YV.size());
            nk.insert(nk.end(), XV.begin(), XV.end());
            nk.insert(nk.end(), YV.begin(), YV.end());
            if (*key != nk) {
                double A, B, K;
                if (fit(xvals(), yvals(), A, B, K)) {
                    (*abk)[0] = A; (*abk)[1] = B; (*abk)[2] = K;
                    self->src = PolyCoeffSrc({A}) + "/(1+" + PolyCoeffSrc({B})
                              + "*e^(-" + PolyCoeffSrc({K}) + "*x))";
                }
                *key = std::move(nk);
            }
            const auto &v = *abk;
            return v[0] / (1.0 + v[1] * exp(-v[2] * x));
        };
        d->src = PolyCoeffSrc({A0}) + "/(1+" + PolyCoeffSrc({B0})
               + "*e^(-" + PolyCoeffSrc({K0}) + "*x))";
        d->cmdName = "FitLogistic";
        d->cmdArgs = { a[0], a[1] };
        if (!litX && !litY) d->inputs = { Lx, Ly };
        d->update();
        made = std::move(d);
        return true;
    }

    // CorrelationCoefficient(x 列表, y 列表) → Pearson 积差相关系数 r
    if (cmdLower == "correlationcoefficient" && a.size() == 2) {
        // 1.0.7.1 实参收 {…} 字面量（GGB 同款内嵌数据式）：字面量产物为
        // 静态（闭包捕获冻结副本），标签实参维持活引用
        bool litX = false, litY = false;
        GeoList *Lx = listArg(a[0], litX);
        GeoList *Ly = Lx ? listArg(a[1], litY) : nullptr;
        if (!Lx || !Ly) { made = nullptr; return true; }   // listArg 已给文案
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        auto fx = litX ? std::make_shared<std::vector<double>>(Lx->values) : nullptr;
        auto fy = litY ? std::make_shared<std::vector<double>>(Ly->values) : nullptr;
        auto xvals = [Lx, fx]() -> const std::vector<double> & { return fx ? *fx : Lx->values; };
        auto yvals = [Ly, fy]() -> const std::vector<double> & { return fy ? *fy : Ly->values; };
        n->calc = [xvals, yvals]() -> double {
            const std::vector<double> &XV = xvals();
            const std::vector<double> &YV = yvals();
            const size_t cnt = std::min(XV.size(), YV.size());
            if (cnt < 2) return NAN;
            double sumx = 0, sumy = 0, sumxx = 0, sumyy = 0, sumxy = 0;
            for (size_t i = 0; i < cnt; ++i) {
                const double x = XV[i], y = YV[i];
                sumx += x; sumy += y; sumxx += x * x; sumyy += y * y; sumxy += x * y;
            }
            const double dxx = sumxx * cnt - sumx * sumx;
            const double dyy = sumyy * cnt - sumy * sumy;
            if (dxx <= 0 || dyy <= 0) return NAN;
            return (sumxy * cnt - sumx * sumy) / sqrt(dxx * dyy);
        };
        n->cmdName = "CorrelationCoefficient";
        n->cmdArgs = { a[0], a[1] };
        n->cmd = "CorrelationCoefficient(" + a[0] + ", " + a[1] + ")";
        if (!litX && !litY) n->inputs = { Lx, Ly };
        n->update();
        made = std::move(n);
        return true;
    }

    // ---------- 1.0.6.4 概率分布（GGB 5.4 语义；参数可依随数值对象） ----------
    // 布尔实参：字面 true/false（GGB 同款，大小写不敏感）或数值对象（≠0=真）

    // Normal(μ, σ, x) → P(X≤x)；Normal(μ, σ, x, true|false) → CDF/PDF；
    // Normal(μ, σ, u, v) → P(u≤X≤v)
    if (cmdLower == "normal" && (a.size() == 3 || a.size() == 4)) {
        NumRefExpr bm, bs, bx, bv;
        std::vector<GeoNumeric *> ndeps;
        auto badArg = [&]() {
            lastError = "Normal 参数须为数或数值对象";
            made = nullptr;
        };
        if (!numRefArg(a[0], bm, ndeps) || !numRefArg(a[1], bs, ndeps)) { badArg(); return true; }
        bool cum = true, interval = false;
        if (a.size() == 4) {
            std::string low;
            for (char ch : a[3]) low += (char)tolower((unsigned char)ch);
            if (low == "true" || low == "false") {
                cum = low == "true";
                if (!numRefArg(a[2], bx, ndeps)) { badArg(); return true; }
            } else {
                interval = true;
                if (!numRefArg(a[2], bx, ndeps) || !numRefArg(a[3], bv, ndeps)) { badArg(); return true; }
            }
        } else {
            if (!numRefArg(a[2], bx, ndeps)) { badArg(); return true; }
        }
        if (bs.refs.empty() && !(bs.evalNow() > 0)) {
            lastError = "Normal 标准差须为正数";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->cmdName = "Normal";
        n->cmdArgs = a;
        n->calc = [bm, bs, bx, bv, cum, interval]() -> double {
            const double mu = bm.evalNow(), sd = bs.evalNow();
            if (!std::isfinite(mu) || !(sd > 0)) return NAN;
            const double x = bx.evalNow();
            if (!std::isfinite(x)) return NAN;
            if (interval) {
                const double v = bv.evalNow();
                if (!std::isfinite(v)) return NAN;
                return NormCdf(mu, sd, std::max(x, v)) - NormCdf(mu, sd, std::min(x, v));
            }
            return cum ? NormCdf(mu, sd, x) : NormPdf(mu, sd, x);
        };
        for (GeoNumeric *d : ndeps) n->inputs.push_back(d);
        n->update();
        made = std::move(n);
        return true;
    }

    // InverseNormal(μ, σ, p) → 分位数 x：P(X≤x)=p
    if (cmdLower == "inversenormal" && a.size() == 3) {
        NumRefExpr bm, bs, bp;
        std::vector<GeoNumeric *> ndeps;
        if (!numRefArg(a[0], bm, ndeps) || !numRefArg(a[1], bs, ndeps)
            || !numRefArg(a[2], bp, ndeps)) {
            lastError = "InverseNormal 参数须为数或数值对象";
            made = nullptr;
            return true;
        }
        if (bs.refs.empty() && !(bs.evalNow() > 0)) {
            lastError = "InverseNormal 标准差须为正数";
            made = nullptr;
            return true;
        }
        if (bp.refs.empty() && !(bp.evalNow() >= 0.0 && bp.evalNow() <= 1.0)) {
            lastError = "InverseNormal 概率须在 0..1";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->cmdName = "InverseNormal";
        n->cmdArgs = a;
        n->calc = [bm, bs, bp]() -> double {
            const double mu = bm.evalNow(), sd = bs.evalNow(), p = bp.evalNow();
            if (!std::isfinite(mu) || !(sd > 0) || !(p >= 0.0 && p <= 1.0)) return NAN;
            return NormQuantile(mu, sd, p);
        };
        for (GeoNumeric *d : ndeps) n->inputs.push_back(d);
        n->update();
        made = std::move(n);
        return true;
    }

    // BinomialDist(n, p, k[, 累积 true|false]) → P(X=k) / P(X≤k)
    if (cmdLower == "binomialdist" && (a.size() == 3 || a.size() == 4)) {
        NumRefExpr bn, bp, bk;
        std::vector<GeoNumeric *> ndeps;
        if (!numRefArg(a[0], bn, ndeps) || !numRefArg(a[1], bp, ndeps)
            || !numRefArg(a[2], bk, ndeps)) {
            lastError = "BinomialDist 参数须为数或数值对象";
            made = nullptr;
            return true;
        }
        bool cum = false;
        GeoNumeric *cumObj = nullptr;
        if (a.size() == 4) {
            std::string low;
            for (char ch : a[3]) low += (char)tolower((unsigned char)ch);
            if (low == "true" || low == "false") {
                cum = low == "true";
            } else {
                cumObj = dynamic_cast<GeoNumeric *>(lookup(a[3]));
                if (!cumObj) {
                    lastError = "BinomialDist 第四参数须为 true/false 或布尔对象";
                    made = nullptr;
                    return true;
                }
                ndeps.push_back(cumObj);
            }
        }
        const double n0 = bn.evalNow(), p0 = bp.evalNow(), k0 = bk.evalNow();
        if (bn.refs.empty() && !(n0 >= 0 && n0 == floor(n0) && n0 <= 100000)) {
            lastError = "BinomialDist 试验次数须为 0..100000 的整数";
            made = nullptr;
            return true;
        }
        if (bp.refs.empty() && !(p0 >= 0.0 && p0 <= 1.0)) {
            lastError = "BinomialDist 成功概率须在 0..1";
            made = nullptr;
            return true;
        }
        if (bk.refs.empty() && !(k0 >= 0 && k0 == floor(k0))) {
            lastError = "BinomialDist 取值须为非负整数";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->cmdName = "BinomialDist";
        n->cmdArgs = a;
        n->calc = [bn, bp, bk, cum, cumObj]() -> double {
            const double nn = bn.evalNow(), pp = bp.evalNow(), kk = bk.evalNow();
            if (!(nn >= 0 && nn == floor(nn) && nn <= 100000)) return NAN;
            if (!(pp >= 0.0 && pp <= 1.0)) return NAN;
            if (!(kk >= 0 && kk == floor(kk))) return NAN;
            const bool cu = cumObj ? cumObj->value != 0.0 : cum;
            return cu ? BinomCdf(nn, pp, kk) : BinomPoint(nn, pp, kk);
        };
        for (GeoNumeric *d : ndeps) n->inputs.push_back(d);
        n->update();
        made = std::move(n);
        return true;
    }

    // InverseBinomial(n, p, P) → 最小 k 使 P(X≤k) ≥ P
    if (cmdLower == "inversebinomial" && a.size() == 3) {
        NumRefExpr bn, bp, bpp;
        std::vector<GeoNumeric *> ndeps;
        if (!numRefArg(a[0], bn, ndeps) || !numRefArg(a[1], bp, ndeps)
            || !numRefArg(a[2], bpp, ndeps)) {
            lastError = "InverseBinomial 参数须为数或数值对象";
            made = nullptr;
            return true;
        }
        const double n0 = bn.evalNow(), p0 = bp.evalNow(), q0 = bpp.evalNow();
        if (bn.refs.empty() && !(n0 >= 0 && n0 == floor(n0) && n0 <= 100000)) {
            lastError = "InverseBinomial 试验次数须为 0..100000 的整数";
            made = nullptr;
            return true;
        }
        if (bp.refs.empty() && !(p0 >= 0.0 && p0 <= 1.0)) {
            lastError = "InverseBinomial 成功概率须在 0..1";
            made = nullptr;
            return true;
        }
        if (bpp.refs.empty() && !(q0 >= 0.0 && q0 <= 1.0)) {
            lastError = "InverseBinomial 概率须在 0..1";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->cmdName = "InverseBinomial";
        n->cmdArgs = a;
        n->calc = [bn, bp, bpp]() -> double {
            const double nn = bn.evalNow(), pp = bp.evalNow(), q = bpp.evalNow();
            if (!(nn >= 0 && nn == floor(nn) && nn <= 100000)) return NAN;
            if (!(pp >= 0.0 && pp <= 1.0) || !(q >= 0.0 && q <= 1.0)) return NAN;
            if (q <= 0.0) return 0.0;
            double acc = 0.0;
            for (long long k = 0; k <= (long long)nn; ++k) {
                acc += BinomPoint(nn, pp, (double)k);
                if (acc >= q) return (double)k;
            }
            return NAN;   // P=1 且累积和因舍入未达：视为无解
        };
        for (GeoNumeric *d : ndeps) n->inputs.push_back(d);
        n->update();
        made = std::move(n);
        return true;
    }

    // Poisson(λ, k[, 累积 true|false]) → P(X=k) / P(X≤k)
    if (cmdLower == "poisson" && (a.size() == 2 || a.size() == 3)) {
        NumRefExpr bl, bk;
        std::vector<GeoNumeric *> ndeps;
        if (!numRefArg(a[0], bl, ndeps) || !numRefArg(a[1], bk, ndeps)) {
            lastError = "Poisson 参数须为数或数值对象";
            made = nullptr;
            return true;
        }
        bool cum = false;
        GeoNumeric *cumObj = nullptr;
        if (a.size() == 3) {
            std::string low;
            for (char ch : a[2]) low += (char)tolower((unsigned char)ch);
            if (low == "true" || low == "false") {
                cum = low == "true";
            } else {
                cumObj = dynamic_cast<GeoNumeric *>(lookup(a[2]));
                if (!cumObj) {
                    lastError = "Poisson 第三参数须为 true/false 或布尔对象";
                    made = nullptr;
                    return true;
                }
                ndeps.push_back(cumObj);
            }
        }
        const double l0 = bl.evalNow(), k0 = bk.evalNow();
        if (bl.refs.empty() && !(l0 >= 0 && l0 <= 1e8)) {
            lastError = "Poisson λ 须为 0..1e8 的非负数";
            made = nullptr;
            return true;
        }
        if (bk.refs.empty() && !(k0 >= 0 && k0 == floor(k0) && k0 <= 1e8)) {
            lastError = "Poisson 取值须为非负整数";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->cmdName = "Poisson";
        n->cmdArgs = a;
        n->calc = [bl, bk, cum, cumObj]() -> double {
            const double lam = bl.evalNow(), kk = bk.evalNow();
            if (!(lam >= 0 && lam <= 1e8)) return NAN;
            if (!(kk >= 0 && kk == floor(kk))) return NAN;
            const bool cu = cumObj ? cumObj->value != 0.0 : cum;
            return cu ? PoisCdf(lam, kk) : PoisPoint(lam, kk);
        };
        for (GeoNumeric *d : ndeps) n->inputs.push_back(d);
        n->update();
        made = std::move(n);
        return true;
    }

    // InversePoisson(λ, P) → 最小 k 使 P(X≤k) ≥ P
    if (cmdLower == "inversepoisson" && a.size() == 2) {
        NumRefExpr bl, bpp;
        std::vector<GeoNumeric *> ndeps;
        if (!numRefArg(a[0], bl, ndeps) || !numRefArg(a[1], bpp, ndeps)) {
            lastError = "InversePoisson 参数须为数或数值对象";
            made = nullptr;
            return true;
        }
        if (bl.refs.empty() && !(bl.evalNow() >= 0 && bl.evalNow() <= 1e8)) {
            lastError = "InversePoisson λ 须为 0..1e8 的非负数";
            made = nullptr;
            return true;
        }
        if (bpp.refs.empty() && !(bpp.evalNow() >= 0.0 && bpp.evalNow() <= 1.0)) {
            lastError = "InversePoisson 概率须在 0..1";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->cmdName = "InversePoisson";
        n->cmdArgs = a;
        n->calc = [bl, bpp]() -> double {
            const double lam = bl.evalNow(), q = bpp.evalNow();
            if (!(lam >= 0 && lam <= 1e8) || !(q >= 0.0 && q <= 1.0)) return NAN;
            if (q <= 0.0) return 0.0;
            double acc = 0.0;
            for (long long k = 0; k <= 10000000; ++k) {
                acc += PoisPoint(lam, (double)k);
                if (acc >= q) return (double)k;
            }
            return NAN;
        };
        for (GeoNumeric *d : ndeps) n->inputs.push_back(d);
        n->update();
        made = std::move(n);
        return true;
    }

    // ---------- 1.0.6.1 矩阵代数（矩阵 = 列表的列表 {{…},{…}}，GGB 同构） ----------

    // Transpose(M) → 矩阵（依随源重算）
    if (cmdLower == "transpose" && a.size() == 1) {
        GeoList *M = dynamic_cast<GeoList *>(lookup(a[0]));
        if (!M || !M->isMatrix) { lastError = "Transpose 需要（矩阵）"; made = nullptr; return true; }
        auto out = std::make_unique<GeoList>();
        GeoList *src = M;
        out->calc = [src](GeoList *g) {
            g->isMatrix = true;
            g->mrows = src->mcols;
            g->mcols = src->mrows;
            g->values.assign(src->values.size(), 0.0);
            for (int r = 0; r < src->mrows; ++r)
                for (int c = 0; c < src->mcols; ++c)
                    g->values[(size_t)c * src->mrows + r] = src->values[(size_t)r * src->mcols + c];
        };
        out->cmdName = "Transpose";
        out->cmdArgs = { a[0] };
        out->inputs = { M };
        out->update();
        made = std::move(out);
        return true;
    }

    // Determinant(M) → 数值（整数矩阵取整，上游 GgbMat.hasOnlyIntegers 语义）。
    // 1.0.6.2：det 为 GGB 同款短名别名（产物仍记 Determinant，保存重放同形）
    if ((cmdLower == "determinant" || cmdLower == "det") && a.size() == 1) {
        GeoList *M = dynamic_cast<GeoList *>(lookup(a[0]));
        if (!M || !M->isMatrix || M->mrows != M->mcols) {
            lastError = "Determinant 需要（方阵）";
            made = nullptr;
            return true;
        }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        GeoList *src = M;
        n->calc = [src]() -> double {
            const int n = src->mrows;
            std::vector<std::vector<double>> A(n, std::vector<double>(n));
            bool allInt = true;
            for (int r = 0; r < n; ++r) {
                for (int c = 0; c < n; ++c) {
                    const double v = src->values[(size_t)r * n + c];
                    A[r][c] = v;
                    if (v != floor(v)) allInt = false;
                }
            }
            double det = 1;
            for (int col = 0; col < n; ++col) {
                int piv = col;
                for (int r = col + 1; r < n; ++r) {
                    if (fabs(A[r][col]) > fabs(A[piv][col])) piv = r;
                }
                if (fabs(A[piv][col]) < 1e-300) return 0.0;   // 奇异
                if (piv != col) { std::swap(A[piv], A[col]); det = -det; }
                det *= A[col][col];
                for (int r = col + 1; r < n; ++r) {
                    const double f = A[r][col] / A[col][col];
                    if (f == 0.0) continue;
                    for (int c2 = col; c2 < n; ++c2) A[r][c2] -= f * A[col][c2];
                }
            }
            return allInt ? round(det) : det;
        };
        n->cmdName = "Determinant";
        n->cmdArgs = { a[0] };
        n->cmd = "Determinant(" + a[0] + ")";
        n->inputs = { M };
        n->update();
        made = std::move(n);
        return true;
    }

    // Invert(M) → 矩阵（方阵且非奇异；整数矩阵取整元素，上游 同语义）。
    // 创建即校验奇异（直接拒绝）；依随重算中源变奇异则产物置 NaN 阵
    if (cmdLower == "invert" && a.size() == 1) {
        GeoList *M = dynamic_cast<GeoList *>(lookup(a[0]));
        if (!M || !M->isMatrix || M->mrows != M->mcols) {
            lastError = "Invert 需要（方阵）";
            made = nullptr;
            return true;
        }
        {
            const int n = M->mrows;
            std::vector<std::vector<double>> A(n, std::vector<double>(n));
            for (int r = 0; r < n; ++r)
                for (int c = 0; c < n; ++c)
                    A[r][c] = M->values[(size_t)r * n + c];
            std::vector<std::vector<double>> inv;
            if (!InvertMatrix(A, inv)) {
                lastError = "矩阵奇异（行列式为 0），不可求逆";
                made = nullptr;
                return true;
            }
        }
        auto out = std::make_unique<GeoList>();
        GeoList *src = M;
        out->calc = [src](GeoList *g) {
            g->isMatrix = true;
            g->mrows = src->mrows;
            g->mcols = src->mcols;
            const int n = src->mrows;
            std::vector<std::vector<double>> A(n, std::vector<double>(n));
            for (int r = 0; r < n; ++r)
                for (int c = 0; c < n; ++c)
                    A[r][c] = src->values[(size_t)r * n + c];
            std::vector<std::vector<double>> inv;
            if (!InvertMatrix(A, inv)) {
                g->values.assign((size_t)n * n, NAN);   // 奇异：未定义矩阵
                return;
            }
            g->values.assign((size_t)n * n, 0.0);
            for (int r = 0; r < n; ++r)
                for (int c = 0; c < n; ++c)
                    g->values[(size_t)r * n + c] = inv[r][c];
        };
        out->cmdName = "Invert";
        out->cmdArgs = { a[0] };
        out->inputs = { M };
        out->update();
        made = std::move(out);
        return true;
    }

    // Identity(n) → n 阶单位矩阵
    if (cmdLower == "identity" && a.size() == 1) {
        double nv = 0;
        if (!numArg(a[0], nv) || nv < 1 || nv > 16 || floor(nv) != nv) {
            lastError = "Identity 需要 1..16 的整数阶数";
            made = nullptr;
            return true;
        }
        const int n = (int)nv;
        auto out = std::make_unique<GeoList>();
        out->isMatrix = true;
        out->mrows = n;
        out->mcols = n;
        out->values.assign((size_t)n * n, 0.0);
        for (int i = 0; i < n; ++i) out->values[(size_t)i * n + i] = 1.0;
        // 七十二包修复：命令产物必须回填 cmdName/cmdArgs，否则 getXml 落到
        // 自由列表分支写出 exp="M="（src 为空），保存重开即被跳过（宿主探针
        // 复现：Identity 往返后 skipped=5）。Determinant/Transpose/Invert/
        // Row/Column 均有回填，唯 Identity 漏。
        out->cmdName = "Identity";
        out->cmdArgs = { a[0] };
        made = std::move(out);
        return true;
    }

    // Row(M, r) / Column(M, c) → 列表（1 起）
    if ((cmdLower == "row" || cmdLower == "column") && a.size() == 2) {
        GeoList *M = dynamic_cast<GeoList *>(lookup(a[0]));
        if (!M || !M->isMatrix) { lastError = "Row/Column 需要（矩阵, 序号）"; made = nullptr; return true; }
        double kv = 0;
        if (!numArg(a[1], kv)) { lastError = "序号须为数或数值对象: " + a[1]; made = nullptr; return true; }
        const int k = (int)kv;
        const bool isRow = cmdLower == "row";
        const int limit = isRow ? M->mrows : M->mcols;
        if (k < 1 || k > limit) { lastError = "序号超出范围: " + a[1]; made = nullptr; return true; }
        auto out = std::make_unique<GeoList>();
        GeoList *src = M;
        const int k0 = k;
        const bool byRow = isRow;
        out->calc = [src, k0, byRow](GeoList *g) {
            g->isMatrix = false;
            g->mrows = g->mcols = 0;
            g->values.clear();
            if (byRow) {
                for (int c = 0; c < src->mcols; ++c)
                    g->values.push_back(src->values[(size_t)(k0 - 1) * src->mcols + c]);
            } else {
                for (int r = 0; r < src->mrows; ++r)
                    g->values.push_back(src->values[(size_t)r * src->mcols + k0 - 1]);
            }
        };
        out->cmdName = isRow ? "Row" : "Column";
        out->cmdArgs = { a[0], a[1] };
        out->inputs = { M };
        out->update();
        made = std::move(out);
        return true;
    }

    // ---------- N6 代数化简（giac CAS 后端） ----------

    // Simplify/Expand/Factor(f)：CAS 处理函数源文，结果须能被数值求值器
    // 接受（含 x 可绘制）才建函数对象；保存按命令重放（CAS 对同一源确定）
    if (cmdLower == "simplify" || cmdLower == "expand" || cmdLower == "factor") {
        if (a.size() != 1) {
            lastError = "命令需要（函数）参数";
            made = nullptr;
            return true;
        }
        GeoFunction *f = resolveFunctionArg(a[0]);
        if (!f) { lastError = "未找到函数 " + a[0]; made = nullptr; return true; }
        const std::string giacCmd = cmdLower;   // giac 内置即小写同名命令
        const std::string ggbCmd = cmdLower == "simplify" ? "Simplify"
                                 : cmdLower == "expand" ? "Expand" : "Factor";
        std::string res;
        if (cmdLower == "factor") {
            // 原生兜底优先：整系数多项式不再进 giac（真机 Factor 闪退未定位，
            // 常见输入全部本地分解；本地处理不了的再走 CAS）
            std::string nat;
            if (NativeFactorPoly(f->body, nat)) res = nat;
        }
        if (res.empty())
            res = CasCaseval(giacCmd + "(" + GiacFromKernelSrc(f->src) + ")");
        if (res.rfind("error:", 0) == 0 || res.rfind("cas-not-ready:", 0) == 0) {
            lastError = "CAS 求值失败: " + res;
            made = nullptr;
            return true;
        }
        Expr body;
        std::string err;
        if (!body.parse(res, err) || !body.containsX()) {
            lastError = "化简结果无法绘制: " + res;
            made = nullptr;
            return true;
        }
        auto d = std::make_unique<GeoFunction>();
        d->src = res;
        d->body = std::move(body);
        d->cmdName = ggbCmd;
        d->cmdArgs = { a[0] };
        d->inputs = { f };
        made = std::move(d);
        return true;
    }

    // ---------- N6 几何度量 ----------

    // Slope(直线/线段/射线/一次函数) → 数值（上游：水平线 0）
    if (cmdLower == "slope" && a.size() == 1) {
        GeoElement *o = lookup(a[0]);
        double k = 0;
        bool ok = false;
        if (GeoLine *l = dynamic_cast<GeoLine *>(o)) {
            if (fabs(l->b) > 1e-15) { k = -l->a / l->b; ok = true; }
        } else if (GeoSegment *s = dynamic_cast<GeoSegment *>(o)) {
            double dx = s->ex2() - s->ex1(), dy = s->ey2() - s->ey1();
            if (fabs(dx) > 1e-15) { k = dy / dx; ok = true; }
        } else if (GeoFunction *f = dynamic_cast<GeoFunction *>(o)) {
            double k2 = 0, m2 = 0;
            if (f->body.linearCoef(k2, m2)) { k = k2; ok = true; }
            else { lastError = "Slope 的函数须为一次式"; made = nullptr; return true; }
        }
        if (!ok) { lastError = "Slope 需要（直线/线段/一次函数）"; made = nullptr; return true; }
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        GeoElement *base = o;
        n->calc = [base]() -> double {
            if (GeoLine *l = dynamic_cast<GeoLine *>(base)) {
                return fabs(l->b) > 1e-15 ? -l->a / l->b : 0.0;
            }
            if (GeoSegment *s = dynamic_cast<GeoSegment *>(base)) {
                double dx = s->ex2() - s->ex1();
                return fabs(dx) > 1e-15 ? (s->ey2() - s->ey1()) / dx : 0.0;
            }
            GeoFunction *f2 = dynamic_cast<GeoFunction *>(base);
            double k2 = 0, m2 = 0;
            return f2 && f2->body.linearCoef(k2, m2) ? k2 : 0.0;
        };
        n->cmd = "Slope(" + a[0] + ")";
        n->cmdName = "Slope";
        n->cmdArgs = { a[0] };
        n->inputs = { o };
        n->update();
        made = std::move(n);
        return true;
    }
    // Length(线段/向量/多边形/两点) → 数值
    if (cmdLower == "length" && a.size() == 2) {
        auto pts = needPts(2);
        if (!pts.empty()) {
            GeoPoint *p1 = pts[0], *p2 = pts[1];
            auto n = std::make_unique<GeoNumeric>();
            n->isFree = false;
            n->calc = [p1, p2]() {
                double dx = p2->x - p1->x, dy = p2->y - p1->y;
                return sqrt(dx * dx + dy * dy);
            };
            n->cmd = "Length(" + p1->label + ", " + p2->label + ")";
            n->cmdName = "Length";
            n->cmdArgs = { a[0], a[1] };
            n->inputs = { p1, p2 };
            n->update();
            made = std::move(n);
            return true;
        }
        GeoElement *o = lookup(a[0]);
        if (GeoSegment *s = dynamic_cast<GeoSegment *>(o)) {
            GeoPoint *p1 = s->p1, *p2 = s->p2;
            auto n = std::make_unique<GeoNumeric>();
            n->isFree = false;
            n->calc = [p1, p2]() {
                double dx = p2->x - p1->x, dy = p2->y - p1->y;
                return sqrt(dx * dx + dy * dy);
            };
            n->cmd = "Length(" + s->label + ")";
            n->cmdName = "Length";
            n->cmdArgs = { a[0] };
            n->inputs = { s };
            n->update();
            made = std::move(n);
            return true;
        }
        if (GeoPolygon *pg = dynamic_cast<GeoPolygon *>(o)) {
            GeoPolygon *src = pg;
            auto n = std::make_unique<GeoNumeric>();
            n->isFree = false;
            n->calc = [src]() {
                double per = 0;
                if (src->image) {
                    size_t c = src->ivx.size();
                    for (size_t i = 0; i < c; ++i) {
                        size_t j = (i + 1) % c;
                        double dx = src->ivx[j] - src->ivx[i], dy = src->ivy[j] - src->ivy[i];
                        per += sqrt(dx * dx + dy * dy);
                    }
                } else {
                    for (size_t i = 0; i < src->verts.size(); ++i) {
                        GeoPoint *a2 = src->verts[i], *b2 = src->verts[(i + 1) % src->verts.size()];
                        double dx = b2->x - a2->x, dy = b2->y - a2->y;
                        per += sqrt(dx * dx + dy * dy);
                    }
                }
                return per;
            };
            n->cmd = "Length(" + pg->label + ")";
            n->cmdName = "Length";
            n->cmdArgs = { a[0] };
            n->inputs = { pg };
            n->update();
            made = std::move(n);
            return true;
        }
        lastError = "Length 需要（两点 / 线段 / 多边形）";
        made = nullptr;
        return true;
    }
    // Area(多边形/圆) → 数值
    if (cmdLower == "area" && a.size() == 1) {
        GeoElement *o = lookup(a[0]);
        if (GeoPolygon *pg = dynamic_cast<GeoPolygon *>(o)) {
            GeoPolygon *src = pg;
            auto n = std::make_unique<GeoNumeric>();
            n->isFree = false;
            n->calc = [src]() {
                double s2 = 0;   // 鞋带公式 2 倍有向面积
                if (src->image) {
                    size_t c = src->ivx.size();
                    for (size_t i = 0; i < c; ++i) {
                        size_t j = (i + 1) % c;
                        s2 += src->ivx[i] * src->ivy[j] - src->ivx[j] * src->ivy[i];
                    }
                } else {
                    for (size_t i = 0; i < src->verts.size(); ++i) {
                        size_t j = (i + 1) % src->verts.size();
                        s2 += src->verts[i]->x * src->verts[j]->y
                            - src->verts[j]->x * src->verts[i]->y;
                    }
                }
                return fabs(s2) / 2.0;
            };
            n->cmd = "Area(" + pg->label + ")";
            n->cmdName = "Area";
            n->cmdArgs = { a[0] };
            n->inputs = { pg };
            n->update();
            made = std::move(n);
            return true;
        }
        if (GeoCircle *c = dynamic_cast<GeoCircle *>(o)) {
            GeoCircle *src = c;
            auto n = std::make_unique<GeoNumeric>();
            n->isFree = false;
            n->calc = [src]() { return M_PI * src->radius * src->radius; };
            n->cmd = "Area(" + c->label + ")";
            n->cmdName = "Area";
            n->cmdArgs = { a[0] };
            n->inputs = { c };
            n->update();
            made = std::move(n);
            return true;
        }
        lastError = "Area 需要（多边形 / 圆）";
        made = nullptr;
        return true;
    }
    // Perimeter(多边形) / Circumference(圆) → 数值
    if (cmdLower == "perimeter" && a.size() == 1) {
        GeoPolygon *pg = dynamic_cast<GeoPolygon *>(lookup(a[0]));
        if (!pg) { lastError = "Perimeter 需要（多边形）"; made = nullptr; return true; }
        GeoPolygon *src = pg;
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->calc = [src]() {
            double per = 0;
            if (src->image) {
                size_t c = src->ivx.size();
                for (size_t i = 0; i < c; ++i) {
                    size_t j = (i + 1) % c;
                    double dx = src->ivx[j] - src->ivx[i], dy = src->ivy[j] - src->ivy[i];
                    per += sqrt(dx * dx + dy * dy);
                }
            } else {
                for (size_t i = 0; i < src->verts.size(); ++i) {
                    GeoPoint *a2 = src->verts[i], *b2 = src->verts[(i + 1) % src->verts.size()];
                    double dx = b2->x - a2->x, dy = b2->y - a2->y;
                    per += sqrt(dx * dx + dy * dy);
                }
            }
            return per;
        };
        n->cmd = "Perimeter(" + pg->label + ")";
        n->cmdName = "Perimeter";
        n->cmdArgs = { a[0] };
        n->inputs = { pg };
        n->update();
        made = std::move(n);
        return true;
    }
    if (cmdLower == "circumference" && a.size() == 1) {
        GeoCircle *c = dynamic_cast<GeoCircle *>(lookup(a[0]));
        if (!c) { lastError = "Circumference 需要（圆）"; made = nullptr; return true; }
        GeoCircle *src = c;
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->calc = [src]() { return 2 * M_PI * src->radius; };
        n->cmd = "Circumference(" + c->label + ")";
        n->cmdName = "Circumference";
        n->cmdArgs = { a[0] };
        n->inputs = { c };
        n->update();
        made = std::move(n);
        return true;
    }
    // Radius(圆) → 数值
    if (cmdLower == "radius" && a.size() == 1) {
        GeoCircle *c = dynamic_cast<GeoCircle *>(lookup(a[0]));
        if (!c) { lastError = "Radius 需要（圆）"; made = nullptr; return true; }
        GeoCircle *src = c;
        auto n = std::make_unique<GeoNumeric>();
        n->isFree = false;
        n->calc = [src]() { return src->radius; };
        n->cmd = "Radius(" + c->label + ")";
        n->cmdName = "Radius";
        n->cmdArgs = { a[0] };
        n->inputs = { c };
        n->update();
        made = std::move(n);
        return true;
    }
    return false;
}

// ---------- 构造列表管理 ----------

GeoElement *Kernel::add(std::unique_ptr<GeoElement> made, const std::string &name)
{
    made->label = name.empty() ? nextLabelFor(made->type()) : name;
    GeoElement *raw = made.get();
    geos_.push_back(std::move(made));
    recomputeAll();
    return raw;
}

GeoElement *Kernel::lookup(const std::string &label) const
{
    // 大小写敏感（上游 同款，真机反馈"还原大小写、不再混用"）：
    // 点 A 与直线 a 共存，大/小写两套标签池互不侵占
    for (auto &g : geos_) {
        if (g->label == label) return g.get();
    }
    return nullptr;
}

GeoPoint *Kernel::lookupPoint(const std::string &label) const
{
    GeoElement *e = lookup(label);
    return e && e->type() == GeoType::Point ? static_cast<GeoPoint *>(e) : nullptr;
}

// 六十八包④：命令函数实参解析（GGB 语义对齐）。函数命令（Derivative/
// Integral/Root/Extremum/Simplify…）的函数槽此前只认「已定义函数对象」，
// 直接填表达式 Integral(x^2, 1, 6) 报「需要（函数）」——GGB 支持表达式
// 直填。规则：先按对象名查（命中函数对象即还回）；查不到时按含 x 表达式
// 解析，成功则构造不进构造列表的匿名函数（仅作命令实参，无标签不显示；
// 依随语义：匿名无依随源，常量表达式即刻定型，与 GGB 直填行为一致）。
// 解析失败/不含 x 置 lastError 返回 nullptr
GeoFunction *Kernel::resolveFunctionArg(const std::string &arg)
{
    if (GeoFunction *f = dynamic_cast<GeoFunction *>(lookup(arg))) {
        if (!f->ineqOp && !f->ineqTwoVar && f->ineqChain.empty()
            && f->ineqDNF.empty() && f->definable()) {
            return f;
        }
        lastError = "函数实参须为可绘函数（不等式/区域不可）: " + arg;
        return nullptr;
    }
    Expr body;
    std::string err;
    if (!body.parse(arg, err) || !body.containsX()) {
        lastError = "未找到函数 " + arg;
        return nullptr;
    }
    auto anon = std::make_unique<GeoFunction>();
    anon->src = arg;
    anon->body = std::move(body);
    return anon.release();
}
void Kernel::deleteLast()
{
    if (geos_.empty()) return;
    const std::string pre = getXml();
    cascadeErase(geos_.back().get());
    recomputeAll();
    commitUndoSnapshot(pre);
}

bool Kernel::deleteByLabel(const std::string &label)
{
    GeoElement *e = lookup(label);
    if (!e) return false;
    const std::string pre = getXml();
    cascadeErase(e);
    recomputeAll();
    commitUndoSnapshot(pre);
    return true;
}

// 全部对象一起析构：inputs 里的 raw 指针同批消失，不存在悬垂访问
void Kernel::clearAll()
{
    if (geos_.empty() && extras_.empty()) {
        picked_ = nullptr;
        toolPts_.clear();
        toolObj_ = nullptr;
        lastError.clear();
        view.reset();
        return;
    }
    const std::string pre = getXml();
    geos_.clear();
    extras_.clear();
    picked_ = nullptr;
    toolPts_.clear();
    toolObj_ = nullptr;
    tablePtCols_.clear();   // 1.0.7.1 新建同时清函数值表叠加点
    lastError.clear();
    view.reset();
    commitUndoSnapshot(pre);
}

// 级联删除：移除 dead 及全部（直接或间接）引用它的对象。
// construction list 里只会引用列表内对象，unique_ptr 释放后其他对象
// 的 raw 指针必须同步清理，故循环扫描直到一轮无删除。
void Kernel::cascadeErase(GeoElement *dead)
{
    std::vector<GeoElement *> doomed{dead};
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto it = geos_.begin(); it != geos_.end();) {
            GeoElement *g = it->get();
            bool drop = false;
            for (GeoElement *d : doomed) {
                if (g == d) { drop = true; break; }
                for (GeoElement *in : g->inputs) {
                    if (in == d) { drop = true; break; }
                }
                if (drop) break;
            }
            if (drop) {
                if (g != dead) doomed.push_back(g);
                it = geos_.erase(it);   // unique_ptr 析构；g 此后不得再访问
                changed = true;
            } else {
                ++it;
            }
        }
    }
}

void Kernel::recomputeAll()
{
    // 构造序即拓扑序（创建时依赖必然已存在），单遍按序 update 即全量重算
    for (auto &g : geos_) g->update();
    // 1.0.7.1 滑条范围对象引用动态联动：重算后引用对象可能已变值，
    // 刷新范围并钳位；值变化再级联一遍（见 refreshSliderRefBounds）
    refreshSliderRefBounds();
}

void Kernel::refreshSliderRefBounds()
{
    for (int pass = 0; pass < 4; ++pass) {
        bool changed = false;
        for (const auto &gp : geos_) {
            GeoNumeric *num = dynamic_cast<GeoNumeric *>(gp.get());
            if (!num || (num->sminRef.empty() && num->smaxRef.empty())) continue;
            if (!num->sminRef.empty()) {
                GeoNumeric *r = dynamic_cast<GeoNumeric *>(lookup(num->sminRef));
                if (r) num->smin = r->value;
            }
            if (!num->smaxRef.empty()) {
                GeoNumeric *r = dynamic_cast<GeoNumeric *>(lookup(num->smaxRef));
                if (r) num->smax = r->value;
            }
            if (!(num->smax > num->smin)) num->smax = num->smin + 5.0;
            // 加载口径一致：只钳位不吸附（步长吸附仅交互路径 setSliderValue）
            const double clamped = std::max(num->smin, std::min(num->smax, num->value));
            if (clamped != num->value) {
                num->value = clamped;
                changed = true;
            }
        }
        if (!changed) break;
        for (auto &g : geos_) g->update();   // 钳位值的依赖者重算
    }
}

// 1.0.7.1 函数值表「显示点」叠加层：解析 "f:0,1,2;g:0,1"（列标签:x 序列）。
// 渲染时按对象当前定义求 y——标签失效/类型变化自动跳过，宿主无需跟随重推
void Kernel::setTablePoints(const std::string &spec)
{
    tablePtCols_.clear();
    if (spec.empty()) return;
    size_t pos = 0;
    while (pos < spec.size()) {
        const size_t semi = spec.find(';', pos);
        const std::string item = spec.substr(
            pos, semi == std::string::npos ? std::string::npos : semi - pos);
        pos = semi == std::string::npos ? spec.size() : semi + 1;
        const size_t colon = item.find(':');
        if (colon == std::string::npos || colon == 0) continue;
        std::vector<double> xs;
        size_t p2 = colon + 1;
        while (p2 <= item.size()) {
            const size_t comma = item.find(',', p2);
            const std::string tok = item.substr(
                p2, comma == std::string::npos ? std::string::npos : comma - p2);
            if (!tok.empty()) {
                try {
                    xs.push_back(std::stod(tok));
                } catch (...) {
                }
            }
            if (comma == std::string::npos) break;
            p2 = comma + 1;
        }
        if (!xs.empty()) tablePtCols_.push_back({ item.substr(0, colon), std::move(xs) });
    }
}

// 叠加点绘制（render 对象层之后调用=画在最上）：函数取 evalAt、非竖直
// 直线取 (c−ax)/b；样式随源对象配色，半径同依随点 4px×uiScale，不参与拾取
void Kernel::appendTablePoints(std::vector<DrawCmd> &out) const
{
    for (const auto &col : tablePtCols_) {
        GeoElement *e = lookup(col.first);
        if (!e || !e->visible) continue;
        const GeoFunction *fn = dynamic_cast<GeoFunction *>(e);
        const GeoLine *ln = dynamic_cast<GeoLine *>(e);
        if (fn && (fn->ineqOp || fn->ineqCombo())) continue;
        if (!fn && (!ln || ln->verticalConst)) continue;
        const std::string base = ObjectColor(e->color, darkTheme);
        for (double x : col.second) {
            const double y = fn ? fn->evalAt(x) : (ln->c - ln->a * x) / ln->b;
            if (!std::isfinite(y)) continue;
            const double cx = view.px(x), cy = view.py(y);
            if (cx < -20 || cx > view.width + 20 || cy < -20 || cy > view.height + 20) continue;
            DrawCmd d;
            d.op = DrawOp::FillCircle;
            d.color = base;
            d.pts = { (float)cx, (float)cy, (float)(4.0 * view.uiScale) };
            out.push_back(d);
        }
    }
}

std::string Kernel::nextLabelFor(GeoType t)
{
    // 上游 标签惯例：点大写 A..Z（用尽进位 A1..），函数 f,g,h..，其余（线/圆/
    // 多边形/数值）小写 a..z（跳过 f 留给函数），列表 L1,L2,...。撞已占用标签继续向后扫。
    const char *pool;
    size_t poolLen;
    if (t == GeoType::Point) {
        pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        poolLen = 26;
    } else if (t == GeoType::Function) {
        pool = "fghjklmnpqrs";
        poolLen = 12;
    } else if (t == GeoType::List) {
        for (int n = 1; n < 1000; ++n) {
            std::string cand = "L" + std::to_string(n);
            if (lookup(cand) == nullptr) return cand;
        }
        return "X";
    } else if (t == GeoType::Text) {
        for (int n = 1; n < 1000; ++n) {
            std::string cand = "text" + std::to_string(n);
            if (lookup(cand) == nullptr) return cand;
        }
        return "X";
    } else {
        pool = "abcdeghijklmnopqrstuvwxyz";
        poolLen = 25;
    }
    for (int round_ = 0; round_ < 1000; ++round_) {
        for (size_t i = 0; i < poolLen; ++i) {
            std::string cand(1, pool[i]);
            if (round_ > 0) cand += std::to_string(round_);
            if (lookup(cand) == nullptr) return cand;
        }
    }
    return "X";
}

// ---------- 代数区 ----------

void Kernel::algebraRows(std::vector<AlgebraRow> &out) const
{
    out.clear();
    for (auto &g : geos_) {
        AlgebraRow r;
        r.label = g->label;
        switch (g->type()) {
        case GeoType::Point: r.type = "point"; break;
        case GeoType::Segment: r.type = "segment"; break;
        case GeoType::Line: r.type = "line"; break;
        case GeoType::Ray: r.type = "ray"; break;
        case GeoType::Vector: r.type = "vector"; break;
        case GeoType::Circle: r.type = "circle"; break;
        case GeoType::Polygon: r.type = "polygon"; break;
        case GeoType::Function: r.type = "function"; break;
        case GeoType::List: r.type = "list"; break;
        case GeoType::Text: r.type = "text"; break;
        case GeoType::Angle: r.type = "angle"; break;
        case GeoType::Numeric: {
            GeoNumeric *n = static_cast<GeoNumeric *>(g.get());
            r.type = n->isSlider ? "slider" : "numeric";
            break;
        }
        }
        r.value = g->valueText();
        r.definition = g->definitionText();
        r.visible = g->visible;
        r.selected = g->selected;
        if (g->type() == GeoType::Function) {
            GeoFunction *fn = static_cast<GeoFunction *>(g.get());
            if (fn->ineqOp || fn->ineqCombo()) {
                r.fillAlphaPct = (int)std::lround(fn->fillAlpha * 100.0);
            }
        } else if (g->type() == GeoType::Polygon) {
            // 十四轮：多边形/角度区域填充透明度滑杆（协议第 7 列，同不等式）
            r.fillAlphaPct =
                (int)std::lround(static_cast<GeoPolygon *>(g.get())->fillAlpha * 100.0);
        } else if (g->type() == GeoType::Angle) {
            r.fillAlphaPct =
                (int)std::lround(static_cast<GeoAngle *>(g.get())->fillAlpha * 100.0);
        }
        out.push_back(std::move(r));
    }
}

// 行协议 v2 文本（NAPI 层 AppendGeoList 移入内核：序列化单点维护，host
// 回归测试可直达同一实现）
std::string Kernel::algebraText() const
{
    std::vector<AlgebraRow> rows;
    algebraRows(rows);
    std::string out;
    for (const auto &r : rows) {
        out += r.label;
        out.push_back('|');
        out += r.type;
        out.push_back('|');
        out += r.value;
        out.push_back('|');
        out += r.definition;
        // 第 5 列固定可见性（代数区实/空心按钮），第 6 列画布选中（行高亮）
        out += r.visible ? "|1" : "|0";
        out += r.selected ? "|1" : "|0";
        // 第 7 列起 key=value;key=value（v2）
        std::string tail;
        if (r.type == "slider") {
            GeoNumeric *n = dynamic_cast<GeoNumeric *>(lookup(r.label));
            tail += "anim=";
            tail += n && n->animating ? "1" : "0";
            tail += ";min=" + FormatNum(n ? n->smin : 0.0);
            tail += ";max=" + FormatNum(n ? n->smax : 0.0);
            tail += ";step=" + FormatNum(n ? n->sstep : 0.0);
        }
        if (r.fillAlphaPct >= 0) {
            if (!tail.empty()) tail.push_back(';');
            tail += "fill=" + std::to_string(r.fillAlphaPct);
        }
        if (!tail.empty()) {
            out.push_back('|');
            out += tail;
        }
        out.push_back('\n');
    }
    return out;
}

GeoPoint *Kernel::pickPoint(double px, double py, double *outD)
{
    GeoPoint *best = nullptr;
    double bestD = 1e9;
    const double grab = 18.0 * view.uiScale;   // 命中半径（像素），上游 SELECTION_RADIUS_MIN=12 再放宽到触控尺寸
    for (auto &g : geos_) {
        if (g->type() != GeoType::Point) continue;
        GeoPoint *p = static_cast<GeoPoint *>(g.get());
        double dx = view.px(p->x) - px;
        double dy = view.py(p->y) - py;
        double d = sqrt(dx * dx + dy * dy);
        if (d < grab && d < bestD) {
            bestD = d;
            best = p;
        }
    }
    if (outD) *outD = bestD;   // 无命中 = 1e9，调用方做就近比较用
    return best;
}

void Kernel::movePoint(GeoPoint *p, double wx, double wy)
{
    if (!p) return;
    if (p->path) {
        // 四十五包：路径依附点拖动 = 指针位置投影落轨（出界端点钳位、
        // 圆按角度连续 unwrap），参数变了重算依随对象
        double param = 0, nx = 0, ny = 0;
        if (!PathProject(p->path, wx, wy, p->pathParam, param, nx, ny)) return;
        p->pathParam = param;
        recomputeAll();
        return;
    }
    if (!p->isFree) return;
    p->x = wx;
    p->y = wy;
    recomputeAll();
}

// ---------- 四十五包 附着/脱离点 ----------

bool Kernel::attachPoint(GeoPoint *p, GeoElement *path, double wx, double wy)
{
    if (!p || !path || p->path == path) return false;
    if (!IsPathType(path)) {
        lastError = "该对象不能作为路径（可附着：线段/直线/射线/向量/圆/多边形/函数图像）";
        return false;
    }
    if (!toolListed(p) || !toolListed(path)) {
        lastError = "对象不存在";
        return false;
    }
    double param = 0, nx = 0, ny = 0;
    if (!PathProject(path, wx, wy, p->path ? p->pathParam : 1e18, param, nx, ny)) {
        lastError = "路径退化，无法附着";
        return false;
    }
    p->path = path;
    p->pathParam = param;
    p->x = nx;
    p->y = ny;
    p->isFree = false;
    p->inputs = { path };
    p->calc = [](GeoPoint *pp) {
        double x = 0, y = 0;
        if (PathPointAt(pp->path, pp->pathParam, x, y)) {
            pp->x = x;
            pp->y = y;
        }
    };
    p->cmdName = "Point";
    p->cmdArgs = { path->label };
    p->cmdIndex = 0;
    recomputeAll();
    return true;
}

bool Kernel::detachPoint(GeoPoint *p)
{
    if (!p || !p->path) return false;
    p->path = nullptr;
    p->pathParam = 0;
    p->calc = nullptr;
    p->inputs.clear();
    p->isFree = true;
    p->cmdName.clear();
    p->cmdArgs.clear();
    p->cmdIndex = 0;
    // 坐标保留当前位置（脱离不跳变）
    recomputeAll();
    return true;
}

// ---------- N5 构造列表扩展：多输出 / 工具 / 拾取 ----------

void Kernel::flushExtras()
{
    if (extras_.empty()) return;
    for (auto &e : extras_) {
        e->label = nextLabelFor(e->type());
        geos_.push_back(std::move(e));
    }
    extras_.clear();
    recomputeAll();
}

GeoElement *Kernel::makeFreePoint(double wx, double wy)
{
    const std::string pre = getXml();
    auto p = std::make_unique<GeoPoint>();
    p->x = wx;
    p->y = wy;
    GeoElement *made = add(std::move(p), "");
    if (made) commitUndoSnapshot(pre);   // 工具空白点按先建点：独立一步撤销
    return made;
}

// 两对象求交：直线类(直线/线段/射线)、圆、函数三类两两解析/数值求解。
// 线段/射线的有界性经 OnObject 过滤；含函数时在可视区间扫描 + 二分。
bool Kernel::intersectAll(GeoElement *o1, GeoElement *o2,
                          std::vector<std::pair<double, double>> &pts, int cap)
{
    pts.clear();
    auto isStraight = [](GeoElement *e) {
        GeoType t = e->type();
        return t == GeoType::Line || t == GeoType::Segment || t == GeoType::Ray;
    };
    GeoElement *ln1 = isStraight(o1) ? o1 : (isStraight(o2) ? o2 : nullptr);
    GeoElement *ln2 = nullptr;
    if (ln1 && isStraight(o1) && isStraight(o2)) ln2 = (ln1 == o1) ? o2 : o1;
    GeoCircle *c1 = dynamic_cast<GeoCircle *>(o1) ? dynamic_cast<GeoCircle *>(o1)
                                                  : dynamic_cast<GeoCircle *>(o2);
    GeoCircle *c2 = c1 ? (c1 == dynamic_cast<GeoCircle *>(o1) ? dynamic_cast<GeoCircle *>(o2)
                                                              : dynamic_cast<GeoCircle *>(o1))
                       : nullptr;
    GeoFunction *fa = dynamic_cast<GeoFunction *>(o1) ? dynamic_cast<GeoFunction *>(o1)
                                                      : dynamic_cast<GeoFunction *>(o2);
    GeoFunction *fb = fa ? (fa == dynamic_cast<GeoFunction *>(o1) ? dynamic_cast<GeoFunction *>(o2)
                                                                  : dynamic_cast<GeoFunction *>(o1))
                         : nullptr;

    auto pushBounded = [&](double x, double y) {
        if ((int)pts.size() >= cap) return;
        if (ln1 && !OnObject(ln1, x, y)) return;
        if (ln2 && !OnObject(ln2, x, y)) return;
        pts.push_back({ x, y });
    };

    // 直线 × 直线（解析）
    if (ln1 && ln2) {
        double a1, b1, cc1, a2, b2, cc2;
        if (!LineCoefsOf(ln1, a1, b1, cc1) || !LineCoefsOf(ln2, a2, b2, cc2)) return false;
        double det = a1 * b2 - a2 * b1;
        if (fabs(det) < 1e-15) return false;
        pushBounded((cc1 * b2 - cc2 * b1) / det, (a1 * cc2 - a2 * cc1) / det);
        return !pts.empty();
    }
    // 直线 × 圆（解析，参数化求根）
    if (ln1 && c1) {
        double la, lb, lc;
        if (!LineCoefsOf(ln1, la, lb, lc)) return false;
        double cx = c1->ccx(), cy = c1->ccy(), r = c1->radius;
        double x1, y1, x2, y2;
        if (fabs(lb) > 1e-15) {
            x1 = 0; y1 = lc / lb; x2 = 1; y2 = (lc - la) / lb;
        } else {
            x1 = x2 = lc / la; y1 = 0; y2 = 1;
        }
        double dx = x2 - x1, dy = y2 - y1;
        double A = dx * dx + dy * dy;
        double B = 2 * ((x1 - cx) * dx + (y1 - cy) * dy);
        double C = (x1 - cx) * (x1 - cx) + (y1 - cy) * (y1 - cy) - r * r;
        double disc = B * B - 4 * A * C;
        if (A < 1e-18 || disc < 0) return false;
        double sq = sqrt(disc);
        double t1 = (-B - sq) / (2 * A), t2 = (-B + sq) / (2 * A);
        pushBounded(x1 + t1 * dx, y1 + t1 * dy);
        if (sq > 1e-12) pushBounded(x1 + t2 * dx, y1 + t2 * dy);
        return !pts.empty();
    }
    // 圆 × 圆（解析）
    if (c1 && c2) {
        double dx = c2->ccx() - c1->ccx(), dy = c2->ccy() - c1->ccy();
        double d = sqrt(dx * dx + dy * dy);
        double r1 = c1->radius, r2 = c2->radius;
        if (d < 1e-15) return false;
        if (d > r1 + r2 + 1e-12 || d < fabs(r1 - r2) - 1e-12) return false;
        double aa = (d * d + r1 * r1 - r2 * r2) / (2 * d);
        double h2 = r1 * r1 - aa * aa;
        double hx = dx / d, hy = dy / d;
        double bx = c1->ccx() + aa * hx, by = c1->ccy() + aa * hy;
        if (h2 <= 1e-18) {
            pushBounded(bx, by);
        } else {
            double h = sqrt(h2);
            pushBounded(bx - h * hy, by + h * hx);
            pushBounded(bx + h * hy, by - h * hx);
        }
        return !pts.empty();
    }
    // 含函数：h(x) 扫描 + 二分（交点 y 取 fa(x)）
    if (!fa) return false;   // 多边形等其他组合暂不支持
    std::function<double(double)> h;
    if (fb) {
        h = [fa, fb](double x) { return fa->evalAt(x) - fb->evalAt(x); };
    } else if (ln1) {
        double la, lb, lc;
        if (!LineCoefsOf(ln1, la, lb, lc)) return false;
        if (fabs(lb) < 1e-15) {
            // 竖直线 x = lc/la 与函数：单点代入
            double kx = lc / la;
            double fy = fa->evalAt(kx);
            if (std::isfinite(fy)) pushBounded(kx, fy);
            return !pts.empty();
        }
        h = [fa, la, lb, lc](double x) { return fa->evalAt(x) - (lc - la * x) / lb; };
    } else if (c1) {
        double cx = c1->ccx(), cy = c1->ccy(), r = c1->radius;
        h = [fa, cx, cy, r](double x) {
            double fy = fa->evalAt(x);
            return (x - cx) * (x - cx) + (fy - cy) * (fy - cy) - r * r;
        };
    } else {
        return false;
    }
    const double x0 = view.xmin() - 1.0, x1 = view.xmax() + 1.0;
    const int N = 2048;
    double step = (x1 - x0) / N;
    double px_ = x0, pv = h(x0);
    for (int i = 1; i <= N && (int)pts.size() < cap; ++i) {
        double qx = x0 + i * step;
        double qv = h(qx);
        if (std::isfinite(pv) && std::isfinite(qv)) {
            if (pv == 0.0) {
                pushBounded(px_, fa->evalAt(px_));
            } else if (qv == 0.0) {
                pushBounded(qx, fa->evalAt(qx));
            } else if (pv * qv < 0) {
                double lo = px_, hi = qx, flo = pv;
                for (int it = 0; it < 60; ++it) {
                    double mid = 0.5 * (lo + hi);
                    double fm = h(mid);
                    if (!std::isfinite(fm)) break;
                    if (flo * fm <= 0) hi = mid;
                    else { lo = mid; flo = fm; }
                }
                double rx = 0.5 * (lo + hi);
                pushBounded(rx, fa->evalAt(rx));
            }
        }
        px_ = qx;
        pv = qv;
    }
    return !pts.empty();
}

bool Kernel::intersectAt(GeoElement *o1, GeoElement *o2, int nth, double &ox, double &oy)
{
    std::vector<std::pair<double, double>> pts;
    intersectAll(o1, o2, pts, nth + 1);
    if (nth < (int)pts.size()) {
        ox = pts[nth].first;
        oy = pts[nth].second;
        return true;
    }
    return false;
}

// 对象拾取（点以外）：线段/向量/射线/直线/圆/多边形/函数按像素距离取最近
GeoElement *Kernel::pickObject(double px, double py)
{
    GeoElement *best = nullptr;
    double bestD = 1e9;
    const double grab = 12.0 * view.uiScale;
    for (auto &g : geos_) {
        if (!g->visible) continue;
        GeoType t = g->type();
        double d = 1e18;
        if (t == GeoType::Segment) {
            GeoSegment *s = static_cast<GeoSegment *>(g.get());
            d = PtSegDist(px, py, view.px(s->ex1()), view.py(s->ey1()),
                          view.px(s->ex2()), view.py(s->ey2()));
        } else if (t == GeoType::Vector) {
            GeoVector *v = static_cast<GeoVector *>(g.get());
            d = PtSegDist(px, py, view.px(v->ex1()), view.py(v->ey1()),
                          view.px(v->ex2()), view.py(v->ey2()));
        } else if (t == GeoType::Ray) {
            GeoRay *r = static_cast<GeoRay *>(g.get());
            double x1 = view.px(r->p1->x), y1 = view.py(r->p1->y);
            double dx = view.px(r->p2->x) - x1, dy = view.py(r->p2->y) - y1;
            double len2 = dx * dx + dy * dy;
            double tt = len2 > 1e-18 ? ((px - x1) * dx + (py - y1) * dy) / len2 : 0.0;
            tt = std::max(0.0, tt);
            double cx = x1 + tt * dx, cy = y1 + tt * dy;
            d = sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
        } else if (t == GeoType::Line) {
            GeoLine *l = static_cast<GeoLine *>(g.get());
            if (fabs(l->a) > 1e-15 || fabs(l->b) > 1e-15) {
                double p1x, p1y, p2x, p2y;
                if (fabs(l->b) < 1e-15) {
                    p1x = p2x = view.px(l->c / l->a);
                    p1y = 0;
                    p2y = (double)view.height;
                } else {
                    p1x = view.px(view.xmin());
                    p1y = view.py((l->c - l->a * view.xmin()) / l->b);
                    p2x = view.px(view.xmax());
                    p2y = view.py((l->c - l->a * view.xmax()) / l->b);
                }
                double ddx = p2x - p1x, ddy = p2y - p1y;
                double len = sqrt(ddx * ddx + ddy * ddy);
                d = len > 1e-9 ? fabs((px - p1x) * ddy - (py - p1y) * ddx) / len : 1e18;
            }
        } else if (t == GeoType::Circle) {
            GeoCircle *c = static_cast<GeoCircle *>(g.get());
            double ccx = view.px(c->ccx()), ccy = view.py(c->ccy());
            double rr = c->radius * view.xscale;
            d = fabs(sqrt((px - ccx) * (px - ccx) + (py - ccy) * (py - ccy)) - rr);
        } else if (t == GeoType::Polygon) {
            GeoPolygon *pg = static_cast<GeoPolygon *>(g.get());
            size_t n = pg->image ? pg->ivx.size() : pg->verts.size();
            d = 1e18;
            for (size_t i = 0; i < n; ++i) {
                size_t j = (i + 1) % n;
                double ax = pg->image ? pg->ivx[i] : pg->verts[i]->x;
                double ay = pg->image ? pg->ivy[i] : pg->verts[i]->y;
                double bx = pg->image ? pg->ivx[j] : pg->verts[j]->x;
                double by = pg->image ? pg->ivy[j] : pg->verts[j]->y;
                double dd = PtSegDist(px, py, view.px(ax), view.py(ay), view.px(bx), view.py(by));
                if (dd < d) d = dd;
            }
        } else if (t == GeoType::Function) {
            GeoFunction *f = static_cast<GeoFunction *>(g.get());
            d = 1e18;
            if (f->ineqOp || f->ineqCombo()) {
                // 区域拾取：触点满足不等式即命中（权重 6px，边界上的其他对象优先）
                const double wxx = view.wx(px), wyy = view.wy(py);
                bool inside = false;
                if (f->ineqTwoVar) {
                    const double s = f->ineqL.evalXY(wxx, wyy) - f->ineqR.evalXY(wxx, wyy);
                    inside = std::isfinite(s)
                        && ((f->ineqOp == 1 || f->ineqOp == 2) ? s < 0.0 : s > 0.0);
                } else if (f->ineqCombo()) {
                    // 1.0.7.1 链式 + 1.0.7.2 ∧/∨ 组合拾取：与渲染同口径 g<0
                    auto resolver = [this](const std::string &nm) -> double {
                        GeoNumeric *n = dynamic_cast<GeoNumeric *>(lookup(nm));
                        return n ? n->value : NAN;
                    };
                    const double g = IneqComboG(*f, wxx, wyy, resolver);
                    inside = std::isfinite(g) && g < 0.0;
                } else if (f->ineqVertical) {
                    inside = (f->ineqOp == 3 || f->ineqOp == 4) ? wxx > f->ineqX : wxx < f->ineqX;
                } else {
                    const double yv = f->body.eval(wxx);
                    if (std::isfinite(yv)) {
                        inside = (f->ineqOp == 1 || f->ineqOp == 2) ? wyy < yv : wyy > yv;
                    }
                }
                if (inside) d = 6.0 * view.uiScale;
            } else {
                // 到函数折线的距离（上游 DrawFunction 命中 = 到绘制折线的距
                // 离）：采样点连段做点-段距，陡峭段/渐近线竖连线同样覆盖。
                // 旧实现取「整条水平线上最小 y 差」，任何被曲线纵跨的点击
                // 都误命中曲线——工具在其附近永远建不出自由点（二十四包⑤
                // 连带修正）
                double lx = 0, ly = 0;
                bool has = false;
                for (int sx = 0; sx <= view.width; sx += 3) {
                    double fy = f->evalAt(view.wx(sx));
                    if (!std::isfinite(fy)) {
                        has = false;   // 未定义区断线，重开段
                        continue;
                    }
                    double cy = view.py(fy);
                    if (has) {
                        double dd = PtSegDist(px, py, lx, ly, (double)sx, cy);
                        if (dd < d) d = dd;
                    }
                    lx = (double)sx;
                    ly = cy;
                    has = true;
                }
            }
        } else if (t == GeoType::Text) {
            // 文本拾取：锚点像素附近的方形区域（宽≈字号×字符数，考虑 fillText 基线）
            GeoText *tx = static_cast<GeoText *>(g.get());
            const double cx = view.px(tx->x), cy = view.py(tx->y);
            const double tw = 14.0 * view.uiScale * 0.6 * (double)tx->text.size();
            if (px >= cx - 4 && px <= cx + tw + 4
                && py >= cy - 16 * view.uiScale && py <= cy + 6 * view.uiScale)
                d = 0;
            else
                d = 1e18;
        } else {
            continue;   // 点/数值不参与对象拾取
        }
        if (d < grab && d < bestD) {
            bestD = d;
            best = g.get();
        }
    }
    return best;
}

// ---------- N5 滑动条与动画 ----------

void Kernel::placeSlider(GeoNumeric *n)
{
    // 画布左上区域按列堆叠（列满换列）；坐标为画布位图 px
    int count = 0;
    for (auto &g : geos_) {
        GeoNumeric *m = dynamic_cast<GeoNumeric *>(g.get());
        if (m && m->isSlider) ++count;
    }
    double us = view.uiScale > 0 ? view.uiScale : 1.0;
    int rows = std::max(1, (int)((view.height - 80 * us) / (56 * us)));
    int col = count / rows, row = count % rows;
    n->sx = (36 + 260 * col) * us;
    n->sy = (44 + 56 * row) * us;
}

GeoNumeric *Kernel::pickSliderHandle(double px, double py)
{
    const double us = view.uiScale;
    const double TL = 140.0 * us;
    for (auto &g : geos_) {
        GeoNumeric *n = dynamic_cast<GeoNumeric *>(g.get());
        if (!n || !n->isSlider) continue;
        double frac = (n->smax - n->smin) > 1e-15 ? (n->value - n->smin) / (n->smax - n->smin) : 0.0;
        frac = std::max(0.0, std::min(1.0, frac));
        double hx = n->sx + TL * frac;
        if (fabs(px - hx) <= 16 * us && fabs(py - n->sy) <= 16 * us) return n;
    }
    return nullptr;
}

void Kernel::dragSliderTo(GeoNumeric *n, double px)
{
    if (!n) return;
    const double us = view.uiScale;
    const double TL = 140.0 * us;
    double t = (px - n->sx) / TL;
    t = std::max(0.0, std::min(1.0, t));
    double v = n->smin + t * (n->smax - n->smin);
    if (n->sstep > 1e-12) {
        v = n->smin + round((v - n->smin) / n->sstep) * n->sstep;
    }
    n->value = std::max(n->smin, std::min(n->smax, v));
    recomputeAll();
}

bool Kernel::setVisible(const std::string &label, bool vis)
{
    GeoElement *e = lookup(label);
    if (!e) return false;
    if (e->visible == vis) return true;   // 无变化不留撤销步
    const std::string pre = getXml();
    e->visible = vis;
    commitUndoSnapshot(pre);
    return true;
}

void Kernel::setSelected(const std::string &label)
{
    for (auto &g : geos_) {
        g->selected = !label.empty() && g->label == label;
    }
}

// ---------- 通用属性 API（轮 A） ----------

std::string Kernel::propsOf(const std::string &label) const
{
    GeoElement *g = lookup(label);
    if (!g) return "";
    std::string out;
    auto add = [&out](const std::string &k, const std::string &v) {
        if (!out.empty()) out.push_back(';');
        out += k + "=" + v;
    };
    std::string ty;
    switch (g->type()) {
    case GeoType::Point: ty = "point"; break;
    case GeoType::Segment: ty = "segment"; break;
    case GeoType::Line: ty = "line"; break;
    case GeoType::Ray: ty = "ray"; break;
    case GeoType::Vector: ty = "vector"; break;
    case GeoType::Circle: ty = "circle"; break;
    case GeoType::Polygon: ty = "polygon"; break;
    case GeoType::Function: ty = "function"; break;
    case GeoType::List: ty = "list"; break;
    case GeoType::Text: ty = "text"; break;
    case GeoType::Angle: ty = "angle"; break;
    case GeoType::Numeric: ty = "numeric"; break;
    }
    add("type", ty);
    add("vis", g->visible ? "1" : "0");
    add("sel", g->selected ? "1" : "0");
    add("color", ColorHex(g->color));   // 原始对象色（不随深色主题反白）
    // 轮 C 样式条：点 = pointSize（1-9），其余可描边对象 = lineThickness（1-13）
    if (g->type() == GeoType::Point) {
        add("size", std::to_string(static_cast<const GeoPoint *>(g)->pointSize));
    } else if (g->type() != GeoType::Numeric && g->type() != GeoType::List
               && g->type() != GeoType::Text) {
        add("size", std::to_string(g->lineThickness));
    }
    if (g->type() == GeoType::Function) {
        GeoFunction *f = static_cast<GeoFunction *>(g);
        if (f->ineqOp || f->ineqCombo()) {
            add("fill", std::to_string((int)std::lround(f->fillAlpha * 100.0)));
        }
    } else if (g->type() == GeoType::Polygon) {
        add("fill", std::to_string(
            (int)std::lround(static_cast<GeoPolygon *>(g)->fillAlpha * 100.0)));
    } else if (g->type() == GeoType::Angle) {
        add("fill", std::to_string(
            (int)std::lround(static_cast<GeoAngle *>(g)->fillAlpha * 100.0)));
    }
    if (g->type() == GeoType::Numeric) {
        GeoNumeric *n = static_cast<GeoNumeric *>(g);
        if (n->isSlider) {
            add("anim", n->animating ? "1" : "0");
            add("min", FormatNum(n->smin));
            add("max", FormatNum(n->smax));
            add("step", FormatNum(n->sstep));
        }
    }
    return out;
}

// 二十三包 表格视图批量求值（上游 TableValuesView 同构）：labels/xs 逗号
// 分隔。列资格 = 上游 GeoElement.hasTableOfValues 子集——可写成一元求值的
// 函数（不等式区域除外）与非竖直直线（y=(c−ax)/b）；行协议：行 ';'/格 ','。
std::string Kernel::tableEval(const std::string &labelsCsv, const std::string &xsCsv)
{
    std::vector<std::string> labels;
    size_t pos = 0;
    while (pos <= labelsCsv.size()) {
        size_t c = labelsCsv.find(',', pos);
        if (c == std::string::npos) c = labelsCsv.size();
        std::string t = labelsCsv.substr(pos, c - pos);
        // 去空白（宿主 join 可能带空格）
        size_t b = t.find_first_not_of(" \t");
        size_t e = t.find_last_not_of(" \t");
        labels.push_back(b == std::string::npos ? "" : t.substr(b, e - b + 1));
        pos = c + 1;
    }
    std::vector<double> xs;
    pos = 0;
    while (pos <= xsCsv.size()) {
        size_t c = xsCsv.find(',', pos);
        if (c == std::string::npos) c = xsCsv.size();
        xs.push_back(strtod(xsCsv.substr(pos, c - pos).c_str(), nullptr));
        pos = c + 1;
    }
    std::string out;
    for (double x : xs) {
        for (const std::string &lb : labels) {
            double v = std::numeric_limits<double>::quiet_NaN();
            GeoElement *g = lookup(lb);
            GeoFunction *f = dynamic_cast<GeoFunction *>(g);
            if (f && !f->ineqOp && !f->ineqTwoVar && f->ineqChain.empty()
                && f->ineqDNF.empty() && f->definable()) {
                v = f->evalAt(x);
            } else if (GeoLine *l = dynamic_cast<GeoLine *>(g)) {
                if (fabs(l->b) > 1e-15) {
                    v = (l->c - l->a * x) / l->b;
                }
            }
            if (!out.empty() && out.back() != ';') out += ',';
            if (std::isfinite(v)) out += FormatNum(v);
        }
        out += ';';
    }
    if (!out.empty()) out.pop_back();   // 去末尾 ';'
    return out;
}

bool Kernel::setProp(const std::string &label, const std::string &key,
                     const std::string &val)
{
    GeoElement *g = lookup(label);
    if (!g) {
        lastError = "未找到对象 " + label;
        return false;
    }
    const bool on = val == "1" || val == "true";
    if (key == "vis") {
        g->visible = on;
        return true;
    }
    if (key == "fill") {
        double a = strtod(val.c_str(), nullptr);
        if (a < 0) a = 0;
        if (a > 1) a = 1;
        if (g->type() == GeoType::Function) {
            GeoFunction *f = static_cast<GeoFunction *>(g);
            if (!f->ineqOp && f->ineqChain.empty() && f->ineqDNF.empty()) {
                lastError = "该函数没有区域填充";
                return false;
            }
            f->fillAlpha = a;
            return true;
        }
        if (g->type() == GeoType::Polygon) {
            static_cast<GeoPolygon *>(g)->fillAlpha = a;
            return true;
        }
        if (g->type() == GeoType::Angle) {
            static_cast<GeoAngle *>(g)->fillAlpha = a;
            return true;
        }
        lastError = "该对象没有区域填充";
        return false;
    }
    if (key == "color") {
        // "#RRGGBB"（大小写均可）；其余格式拒绝
        auto hv = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
        };
        if (val.size() < 7 || val[0] != '#') {
            lastError = "颜色格式应为 #RRGGBB";
            return false;
        }
        int r = hv(val[1]) * 16 + hv(val[2]);
        int gg = hv(val[3]) * 16 + hv(val[4]);
        int b = hv(val[5]) * 16 + hv(val[6]);
        if (r < 0 || gg < 0 || b < 0) {
            lastError = "颜色格式应为 #RRGGBB";
            return false;
        }
        g->color = GeoColor{r, gg, b};
        return true;
    }
    if (key == "size") {
        // 点大小（1-9）/ 线宽（1-13，渲染宽度 = thickness/3）——上游 同范围
        int v = static_cast<int>(std::lround(strtod(val.c_str(), nullptr)));
        if (g->type() == GeoType::Point) {
            if (v < 1) v = 1;
            if (v > 9) v = 9;
            static_cast<GeoPoint *>(g)->pointSize = v;
            return true;
        }
        if (g->type() == GeoType::Numeric || g->type() == GeoType::List
            || g->type() == GeoType::Text) {
            lastError = "该对象没有大小属性";
            return false;
        }
        if (v < 1) v = 1;
        if (v > 13) v = 13;
        g->lineThickness = v;
        return true;
    }
    if (g->type() == GeoType::Numeric) {
        GeoNumeric *n = static_cast<GeoNumeric *>(g);
        if (key == "value") {
            if (!n->isFree) {
                lastError = "仅自由数值可直接赋值";
                return false;
            }
            if (n->isSlider) {
                setSliderValue(n, strtod(val.c_str(), nullptr));
            } else {
                n->value = strtod(val.c_str(), nullptr);
                recomputeAll();
            }
            return true;
        }
        if (n->isSlider) {
            if (key == "min") {
                n->smin = strtod(val.c_str(), nullptr);
                if (!(n->smax > n->smin)) n->smax = n->smin + 1.0;
                return true;
            }
            if (key == "max") {
                n->smax = strtod(val.c_str(), nullptr);
                if (!(n->smax > n->smin)) n->smin = n->smax - 1.0;
                return true;
            }
            if (key == "step") {
                double s = strtod(val.c_str(), nullptr);
                n->sstep = s > 1e-12 ? s : 0.0;
                return true;
            }
            if (key == "anim") {
                if (on != n->animating) animateToggle(label);
                return true;
            }
        }
    }
    lastError = "不支持的属性 " + key;
    return false;
}

void Kernel::eraseGeo(GeoElement *e)
{
    for (auto it = geos_.begin(); it != geos_.end(); ++it) {
        if (it->get() == e) {
            geos_.erase(it);
            return;
        }
    }
}

// ---------- 重命名 / 重定义 ----------

// 重命名对象并同步所有命令参数里的引用。返回空串=成功，否则错误信息。
// 词边界替换（rename 同步表达式定义文本用）：命中处前后不能是字母/数字，
// 防 "a" 误伤 "a1"
static void ReplaceWord(std::string &s, const std::string &w, const std::string &rep)
{
    if (w.empty() || s.empty()) return;
    std::string out;
    out.reserve(s.size() + rep.size());
    size_t i = 0;
    while (i < s.size()) {
        if (s.compare(i, w.size(), w) == 0
            && (i == 0 || !isalnum((unsigned char)s[i - 1]))
            && (i + w.size() >= s.size() || !isalnum((unsigned char)s[i + w.size()]))) {
            out += rep;
            i += w.size();
        } else {
            out += s[i];
            ++i;
        }
    }
    s = out;
}

std::string Kernel::rename(const std::string &oldLabel, const std::string &newLabel)
{
    GeoElement *e = lookup(oldLabel);
    if (!e) return "未找到对象 " + oldLabel;
    if (newLabel.empty()) return "新标签不能为空";
    if (newLabel == e->label) return "";
    if (newLabel.size() > 32) return "标签过长";
    // 键盘可创建 a_n / α 等标签（第八包），改名规则与之一致
    if (!isalpha((unsigned char)newLabel[0]) && !IsGreekAt(newLabel, 0)) return "标签需以字母开头";
    for (size_t ci = 0; ci < newLabel.size();) {
        if (IsGreekAt(newLabel, ci)) { ci += 2; continue; }
        const char c = newLabel[ci];
        if (!isalnum((unsigned char)c) && c != '_') return "标签只能含字母、数字和下划线";
        ++ci;
    }
    if (lookup(newLabel)) return "标签 " + newLabel + " 已存在";
    const std::string pre = getXml();
    e->label = newLabel;
    // 命令参数引用同步（.ggb 重放与代数区定义列依赖它）
    for (auto &g : geos_) {
        for (std::string &arg : g->cmdArgs) {
            if (arg == oldLabel) arg = newLabel;
        }
    }
    // 表达式定义文本同步（轮 D 依随数值 cmd="A1+1"；顺带修正命令数值的
    // cmd 展示文本）。undo 快照按改后 XML 走，缺这步重放会丢依赖
    for (auto &g : geos_) {
        if (GeoNumeric *n = dynamic_cast<GeoNumeric *>(g.get())) {
            ReplaceWord(n->cmd, oldLabel, newLabel);
        }
    }
    recomputeAll();
    commitUndoSnapshot(pre);
    return "";
}

// 就地把 src 对象的几何/依赖状态搬到 dst（保持指针身份，依赖方无感）。
// 样式（颜色/线宽）与命令记录一并更新；label/visible/selected 由调用方管理。
static void CopyGeoState(GeoElement *dst, GeoElement *src)
{
    dst->color = src->color;
    dst->lineThickness = src->lineThickness;
    dst->inputs = src->inputs;
    dst->cmdName = src->cmdName;
    dst->cmdArgs = src->cmdArgs;
    dst->cmdIndex = src->cmdIndex;
    if (dst->type() == src->type()) {
        switch (dst->type()) {
        case GeoType::Point: {
            GeoPoint *d = static_cast<GeoPoint *>(dst);
            GeoPoint *s = static_cast<GeoPoint *>(src);
            d->x = s->x;
            d->y = s->y;
            d->isFree = s->isFree;
            d->calc = s->calc;
            d->pointSize = s->pointSize;
            // 四十五包：路径依附状态随重定义迁移（重定义为坐标=脱离，
            // 重定义为 Point(path)=换路径/附着）
            d->path = s->path;
            d->pathParam = s->pathParam;
            break;
        }
        case GeoType::Segment: {
            GeoSegment *d = static_cast<GeoSegment *>(dst);
            GeoSegment *s = static_cast<GeoSegment *>(src);
            d->p1 = s->p1;
            d->p2 = s->p2;
            d->image = s->image;
            d->ix1 = s->ix1;
            d->iy1 = s->iy1;
            d->ix2 = s->ix2;
            d->iy2 = s->iy2;
            d->calc = s->calc;
            break;
        }
        case GeoType::Line: {
            GeoLine *d = static_cast<GeoLine *>(dst);
            GeoLine *s = static_cast<GeoLine *>(src);
            d->a = s->a;
            d->b = s->b;
            d->c = s->c;
            d->verticalConst = s->verticalConst;
            d->mode = s->mode;
            d->p1 = s->p1;
            d->p2 = s->p2;
            d->pt = s->pt;
            d->base = s->base;
            d->calc = s->calc;
            break;
        }
        case GeoType::Ray: {
            GeoRay *d = static_cast<GeoRay *>(dst);
            GeoRay *s = static_cast<GeoRay *>(src);
            d->p1 = s->p1;
            d->p2 = s->p2;
            break;
        }
        case GeoType::Vector: {
            // GeoVector 与 GeoRay 布局不同（多 image/ix 缓存 + calc），按
            // GeoRay 拷会写花对象——二十四包修正的潜伏 UB
            GeoVector *d = static_cast<GeoVector *>(dst);
            GeoVector *s = static_cast<GeoVector *>(src);
            d->p1 = s->p1;
            d->p2 = s->p2;
            d->image = s->image;
            d->ix1 = s->ix1;
            d->iy1 = s->iy1;
            d->ix2 = s->ix2;
            d->iy2 = s->iy2;
            d->calc = s->calc;
            break;
        }
        case GeoType::Circle: {
            GeoCircle *d = static_cast<GeoCircle *>(dst);
            GeoCircle *s = static_cast<GeoCircle *>(src);
            d->center = s->center;
            d->through = s->through;
            d->radiusNum = s->radiusNum;
            d->radius = s->radius;
            d->image = s->image;
            d->icx = s->icx;
            d->icy = s->icy;
            d->calc = s->calc;
            break;
        }
        case GeoType::Polygon: {
            GeoPolygon *d = static_cast<GeoPolygon *>(dst);
            GeoPolygon *s = static_cast<GeoPolygon *>(src);
            d->verts = s->verts;
            d->image = s->image;
            d->ivx = s->ivx;
            d->ivy = s->ivy;
            d->fillAlpha = s->fillAlpha;
            d->calc = s->calc;
            break;
        }
        case GeoType::Function: {
            GeoFunction *d = static_cast<GeoFunction *>(dst);
            GeoFunction *s = static_cast<GeoFunction *>(src);
            d->src = s->src;
            d->body = s->body;
            d->alt = s->alt;
            d->ineqOp = s->ineqOp;
            d->ineqVertical = s->ineqVertical;
            d->ineqX = s->ineqX;
            d->ineqTwoVar = s->ineqTwoVar;
            d->ineqL = s->ineqL;
            d->ineqR = s->ineqR;
            d->ineqChain = s->ineqChain;
            d->ineqDNF = s->ineqDNF;
            d->fillAlpha = s->fillAlpha;
            break;
        }
        case GeoType::Numeric: {
            GeoNumeric *d = static_cast<GeoNumeric *>(dst);
            GeoNumeric *s = static_cast<GeoNumeric *>(src);
            d->value = s->value;
            d->isFree = s->isFree;
            d->calc = s->calc;
            d->cmd = s->cmd;
            d->isSlider = s->isSlider;
            d->smin = s->smin;
            d->smax = s->smax;
            d->sstep = s->sstep;
            d->speed = s->speed;
            d->fill = s->fill;   // shared：重定义后 calc 与渲染仍读同一份
            d->animating = false;
            break;
        }
        case GeoType::Angle: {
            GeoAngle *d = static_cast<GeoAngle *>(dst);
            GeoAngle *s = static_cast<GeoAngle *>(src);
            d->value = s->value;
            d->isFree = s->isFree;
            d->calc = s->calc;
            d->cmd = s->cmd;
            d->va = s->va;
            d->vb = s->vb;
            d->vc = s->vc;
            d->acalc = s->acalc;
            d->fillAlpha = s->fillAlpha;
            break;
        }
        case GeoType::List: {
            GeoList *d = static_cast<GeoList *>(dst);
            GeoList *s = static_cast<GeoList *>(src);
            d->values = s->values;
            d->src = s->src;
            d->calc = s->calc;
            break;
        }
        case GeoType::Text: {
            GeoText *d = static_cast<GeoText *>(dst);
            GeoText *s = static_cast<GeoText *>(src);
            d->x = s->x;
            d->y = s->y;
            d->text = s->text;
            d->anchor = s->anchor;
            d->segs = s->segs;
            d->refs = s->refs;
            break;
        }
        }
    }
}

// 重定义：保持标签与指针身份，用新表达式替换对象定义（上游 redefine 语义）。
// 做法：旧对象临时改名腾出标签 → input() 常规机制新建 → 同类型则就地搬状态
// 并删除临时对象；类型改变时仅当旧对象无被依赖才允许（删旧留新）。
bool Kernel::redefine(const std::string &label, const std::string &exprRaw)
{
    GeoElement *old = lookup(label);
    if (!old) {
        lastError = "未找到对象 " + label;
        return false;
    }
    // 键盘线性形式预解析（$ / 循环小数 / 带分数）+ ans 替换，与 inputImpl 同规则
    const std::string preRaw = PreprocessInput(exprRaw);
    std::string src;
    src.reserve(preRaw.size());
    {
        bool inStr = false;   // 引号串内（Text 文本）保留空白
        for (char c : preRaw) {
            if (c == '"') inStr = !inStr;
            else if (isspace((unsigned char)c) && !inStr) continue;
            src.push_back(c);
        }
    }
    if (src.empty()) {
        lastError = "表达式为空";
        return false;
    }
    src = ReplaceAns(src);
    // 带赋值头时标签必须一致（改名走重命名入口）。
    // '=' 前是 < > ! = 的属于比较符，跳过（y<=x^2 这类不等式无赋值头）
    {
        size_t eq = std::string::npos;
        for (size_t i = 0; i < src.size(); ++i) {
            if (src[i] != '=') continue;
            if (i > 0) {
                const char p = src[i - 1];
                if (p == '<' || p == '>' || p == '!' || p == '=') continue;
            }
            eq = i;
            break;
        }
        if (eq != std::string::npos && eq > 0) {
            std::string lhs = src.substr(0, eq);
            size_t p = lhs.find("(x)");
            std::string lhsName = p != std::string::npos ? lhs.substr(0, p) : lhs;
            if (lhsName != "y" && lhsName != label) {
                lastError = "重定义不能更换标签（改名请点对象字母）";
                return false;
            }
        }
    }
    // 纯数值/角对象的定义列没有 "a=" 前缀（"5"、"Angle(A,B,C)"）→ 补成
    // "label=..." 才能走赋值创建（角带类型后同数值，十四轮）。
    // 轮 D：依随表达式（"A1+1"）普通解析不认对象标签，refs 模式判定
    if ((old->type() == GeoType::Numeric || old->type() == GeoType::Angle)
        && src.find('=') == std::string::npos) {
        Expr t;
        std::string terr;
        if (t.parse(src, terr, std::string(), true) && !t.containsX()) {
            src = label + "=" + src;
        }
    }
    GeoNumeric *oldNum = dynamic_cast<GeoNumeric *>(old);
    bool keepSliderPos = oldNum && oldNum->isSlider;
    double oldSx = keepSliderPos ? oldNum->sx : 0;
    double oldSy = keepSliderPos ? oldNum->sy : 0;

    const std::string pre = getXml();
    old->label = "#tmp#";
    size_t before = geos_.size();
    ++undoSuspend_;   // 内部 input 不打点：重定义整体算一步
    const bool inputOk = input(src);
    --undoSuspend_;
    if (!inputOk) {
        old->label = label;
        extras_.clear();
        return false;   // lastError 已填
    }
    if (geos_.size() != before + 1 + autoCells_) {
        while (geos_.size() > before) geos_.pop_back();
        autoCells_ = 0;
        extras_.clear();
        old->label = label;
        recomputeAll();
        lastError = "重定义表达式需恰好定义一个对象";
        return false;
    }
    GeoElement *made = geos_[before + autoCells_].get();
    if (made->type() != old->type()) {
        // 类型改变：仅当旧对象没有被依赖时允许（删旧留新）
        bool hasDeps = false;
        for (auto &g : geos_) {
            if (g.get() == made) continue;
            for (GeoElement *in : g->inputs) {
                if (in == old) { hasDeps = true; break; }
            }
            if (hasDeps) break;
        }
        if (hasDeps) {
            while (geos_.size() > before) geos_.pop_back();
            autoCells_ = 0;
            extras_.clear();
            old->label = label;
            recomputeAll();
            lastError = "新定义类型改变且被其他对象依赖，无法重定义";
            return false;
        }
        eraseGeo(old);
        made->label = label;
        recomputeAll();
        commitUndoSnapshot(pre);
        return true;
    }
    CopyGeoState(old, made);
    geos_.erase(geos_.begin() + before + autoCells_);
    autoCells_ = 0;
    old->label = label;
    oldNum = dynamic_cast<GeoNumeric *>(old);
    if (keepSliderPos && oldNum && oldNum->isSlider) {
        oldNum->sx = oldSx;
        oldNum->sy = oldSy;
    }
    old->update();
    recomputeAll();
    commitUndoSnapshot(pre);
    return true;
}

void Kernel::setSliderValue(GeoNumeric *n, double v)
{
    if (!n || !n->isSlider) return;
    if (n->sstep > 1e-12) {
        v = n->smin + std::round((v - n->smin) / n->sstep) * n->sstep;
    }
    n->value = std::max(n->smin, std::min(n->smax, v));
    recomputeAll();
}

// 二十八包⑤：四参数一次落位。sstep<=0 视为连续（与 Slider 命令口径一致）；
// 值走 setSliderValue（新步长吸附 + 新范围钳位 + 级联重算），一步到位
bool Kernel::setSliderParams(GeoNumeric *n, double smin, double smax,
    double sstep, double v)
{
    if (!n || !n->isSlider) {
        lastError = "对象不是滑动条";
        return false;
    }
    if (!(smax > smin)) {
        lastError = "Slider 上限须大于下限";
        return false;
    }
    n->smin = smin;
    n->smax = smax;
    n->sstep = sstep > 1e-12 ? sstep : 0.0;
    setSliderValue(n, v);
    return true;
}

bool Kernel::animateToggle(const std::string &label)
{
    GeoNumeric *n = dynamic_cast<GeoNumeric *>(lookup(label));
    if (!n || !n->isSlider) return false;
    n->animating = !n->animating;
    if (n->animating) {
        n->animDir = n->value >= n->smax ? -1 : 1;
    }
    return n->animating;
}

bool Kernel::animTick(double dt)
{
    bool changed = false;
    for (auto &g : geos_) {
        GeoNumeric *n = dynamic_cast<GeoNumeric *>(g.get());
        if (!n || !n->isSlider || !n->animating) continue;
        n->value += n->animDir * n->speed * dt;
        if (n->value >= n->smax) {
            n->value = n->smax;
            n->animDir = -1;
        } else if (n->value <= n->smin) {
            n->value = n->smin;
            n->animDir = 1;
        }
        changed = true;
    }
    if (changed) recomputeAll();
    return changed;
}

bool Kernel::anyAnimating() const
{
    for (auto &g : geos_) {
        GeoNumeric *n = dynamic_cast<GeoNumeric *>(g.get());
        if (n && n->isSlider && n->animating) return true;
    }
    return false;
}

// ---------- N5 变换像构造 ----------

std::unique_ptr<GeoElement> Kernel::makeImage(GeoElement *obj, const std::string &cmd,
                                              const std::vector<std::string> &args, const MapFn &map,
                                              const std::function<double(double)> &rmul,
                                              GeoElement *extraDep, GeoElement *extraDep2)
{
    std::unique_ptr<GeoElement> made;
    if (GeoPoint *p = dynamic_cast<GeoPoint *>(obj)) {
        auto ip = std::make_unique<GeoPoint>();
        ip->isFree = false;
        ip->pointSize = 4;
        ip->color = {0x64, 0x64, 0x64};
        ip->calc = [p, map](GeoPoint *q) {
            std::pair<double, double> r = map(p->x, p->y);
            q->x = r.first;
            q->y = r.second;
        };
        std::pair<double, double> r0 = map(p->x, p->y);
        ip->x = r0.first;
        ip->y = r0.second;
        made = std::move(ip);
    } else if (GeoSegment *s = dynamic_cast<GeoSegment *>(obj)) {
        auto is = std::make_unique<GeoSegment>();
        is->image = true;
        is->calc = [s, map](GeoSegment *q) {
            std::pair<double, double> r1 = map(s->ex1(), s->ey1());
            std::pair<double, double> r2 = map(s->ex2(), s->ey2());
            q->ix1 = r1.first;
            q->iy1 = r1.second;
            q->ix2 = r2.first;
            q->iy2 = r2.second;
        };
        std::pair<double, double> r1 = map(s->ex1(), s->ey1());
        std::pair<double, double> r2 = map(s->ex2(), s->ey2());
        is->ix1 = r1.first;
        is->iy1 = r1.second;
        is->ix2 = r2.first;
        is->iy2 = r2.second;
        made = std::move(is);
    } else if (GeoLine *l = dynamic_cast<GeoLine *>(obj)) {
        auto il = std::make_unique<GeoLine>();
        il->mode = GeoLine::Mode::Fixed;
        il->calc = [l, map](GeoLine *q) {
            // 取源直线上两点，映射后重定 a,b,c
            double x1, y1, x2, y2;
            if (fabs(l->b) > 1e-15) {
                x1 = 0;
                y1 = l->c / l->b;
                x2 = 1;
                y2 = (l->c - l->a) / l->b;
            } else {
                x1 = x2 = l->c / l->a;
                y1 = 0;
                y2 = 1;
            }
            std::pair<double, double> p1 = map(x1, y1);
            std::pair<double, double> p2 = map(x2, y2);
            q->a = p1.second - p2.second;
            q->b = p2.first - p1.first;
            q->c = q->a * p1.first + q->b * p1.second;
        };
        il->update();
        if (fabs(il->a) < 1e-15 && fabs(il->b) < 1e-15) return nullptr;   // 映射退化
        made = std::move(il);
    } else if (GeoCircle *c = dynamic_cast<GeoCircle *>(obj)) {
        auto ic = std::make_unique<GeoCircle>();
        ic->image = true;
        ic->calc = [c, map, rmul](GeoCircle *q) {
            std::pair<double, double> r0 = map(c->ccx(), c->ccy());
            q->icx = r0.first;
            q->icy = r0.second;
            q->radius = rmul ? rmul(c->radius) : c->radius;
        };
        std::pair<double, double> r0 = map(c->ccx(), c->ccy());
        ic->icx = r0.first;
        ic->icy = r0.second;
        ic->radius = rmul ? rmul(c->radius) : c->radius;
        made = std::move(ic);
    } else if (GeoPolygon *pg = dynamic_cast<GeoPolygon *>(obj)) {
        auto ipg = std::make_unique<GeoPolygon>();
        ipg->image = true;
        ipg->calc = [pg, map](GeoPolygon *q) {
            q->ivx.clear();
            q->ivy.clear();
            if (pg->image) {
                for (size_t i = 0; i < pg->ivx.size(); ++i) {
                    std::pair<double, double> r = map(pg->ivx[i], pg->ivy[i]);
                    q->ivx.push_back(r.first);
                    q->ivy.push_back(r.second);
                }
            } else {
                for (GeoPoint *v : pg->verts) {
                    std::pair<double, double> r = map(v->x, v->y);
                    q->ivx.push_back(r.first);
                    q->ivy.push_back(r.second);
                }
            }
        };
        ipg->update();
        made = std::move(ipg);
    } else {
        return nullptr;   // 函数/数值等暂不做像
    }
    made->inputs = { obj };
    if (extraDep) made->inputs.push_back(extraDep);
    if (extraDep2) made->inputs.push_back(extraDep2);
    made->cmdName = cmd;
    made->cmdArgs = args;
    return made;
}

// ---------- N5 点选式工具状态机 ----------
//
// 工具 id：point/segment/line/ray/vector/circle/midpoint/perpBisector/
// angleBisector/angle/polygon/intersect/tangent/attachDetach（四十五包）/
// translate/reflect/reflectPoint/perpLine/parLine/dist/area/slope/
// vecFromPoint（二十三包起）。
// 上游 行为：空白处点按先建自由点；工具完成一次作图后保持激活。
// 返回协议："done|标签,.." / "need|提示" / "err|原因"。
// 选中特效：中间拾取的对象即时点亮（selected），done/err 统一清除，
// need（还差拾取）保持点亮直至作图完成。
std::string Kernel::toolTap(double wx, double wy, double px, double py)
{
    std::string r = toolTapImpl(wx, wy, px, py);
    if (r.rfind("done|", 0) == 0 || r.rfind("err|", 0) == 0) {
        for (auto &g : geos_) g->selected = false;
    }
    return r;
}

std::string Kernel::toolTapImpl(double wx, double wy, double px, double py)
{
    if (tool_.empty()) return "err|未选择工具";
    if (!askKind_.empty()) {
        // 参数弹窗开着（宿主 scrim 挡画布，正常到不了这里）——防御：
        // 用 need| 而非 err|，避免 toolTap 包装层把待选对象的点亮清掉
        return "need|请先在弹窗输入参数，或取消后重画";
    }
    GeoPoint *hit = pickPoint(px, py);

    if (tool_ == "point") {
        GeoElement *p = hit ? hit : makeFreePoint(wx, wy);
        return "done|" + p->label;
    }

    // 二十七包 滑动条工具：点画布即建滑动条（上游 Slider 工具同位，默认范围
    // −5~5 步长 0.1），落点记入 sx/sy（placeSlider 位图 px 同坐标系）；画布
    // 暂不绘制滑动条，数值走代数区滑杆控件驱动参数联动
    if (tool_ == "slider") {
        const size_t before = geos_.size();
        if (!input("Slider(-5, 5, 0.1)")) {
            return "err|" + (lastError.empty() ? "无法创建滑动条" : lastError);
        }
        GeoNumeric *n = dynamic_cast<GeoNumeric *>(geos_[before].get());
        if (n != nullptr) {
            const double us = view.uiScale > 0 ? view.uiScale : 1.0;
            n->sx = px * us;
            n->sy = py * us;
        }
        return "done|" + geos_[before]->label;
    }

    // 两点工具
    const char *twoCmd = nullptr;
    if (tool_ == "segment") twoCmd = "Segment";
    else if (tool_ == "line") twoCmd = "Line";
    else if (tool_ == "ray") twoCmd = "Ray";
    else if (tool_ == "vector") twoCmd = "Vector";
    else if (tool_ == "circle") twoCmd = "Circle";
    else if (tool_ == "midpoint") twoCmd = "Midpoint";
    else if (tool_ == "perpBisector") twoCmd = "PerpendicularBisector";
    if (twoCmd) {
        if (!hit) hit = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        hit->selected = true;
        toolPts_.push_back(hit);
        if (toolPts_.size() < 2) {
            return "need|" + std::string(twoCmd) + "：还差 1 个点（点空白处会先建点）";
        }
        std::string expr = std::string(twoCmd) + "(" + toolPts_[0]->label + ", " + toolPts_[1]->label + ")";
        toolPts_.clear();
        size_t before = geos_.size();
        if (!input(expr)) {
            return "err|" + (lastError.empty() ? "无法创建" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    // 三点工具：角平分线 / 角
    if (tool_ == "angleBisector" || tool_ == "angle") {
        const char *cmd3 = tool_ == "angle" ? "Angle" : "AngleBisector";
        if (!hit) hit = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        hit->selected = true;
        toolPts_.push_back(hit);
        if (toolPts_.size() < 3) {
            return "need|" + std::string(cmd3) + "：还差 "
                + std::to_string(3 - (int)toolPts_.size()) + " 个点";
        }
        std::string expr = std::string(cmd3) + "(" + toolPts_[0]->label + ", "
            + toolPts_[1]->label + ", " + toolPts_[2]->label + ")";
        toolPts_.clear();
        size_t before = geos_.size();
        if (!input(expr)) {
            return "err|" + (lastError.empty() ? "无法创建" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    // 多边形：点第一个顶点闭合（上游 行为）
    if (tool_ == "polygon") {
        if (!hit) hit = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        if (!toolPts_.empty() && hit == toolPts_[0]) {
            if (toolPts_.size() < 3) {
                toolPts_.clear();
                return "err|多边形至少需要 3 个顶点";
            }
            std::string expr = "Polygon(";
            for (size_t i = 0; i < toolPts_.size(); ++i) {
                if (i > 0) expr += ", ";
                expr += toolPts_[i]->label;
            }
            expr += ")";
            toolPts_.clear();
            size_t before = geos_.size();
            if (!input(expr)) {
                return "err|" + (lastError.empty() ? "无法创建" : lastError);
            }
            return "done|" + geos_[before]->label;
        }
        if (toolPts_.size() >= 15) {
            toolPts_.clear();
            return "err|多边形顶点过多，请重新开始";
        }
        hit->selected = true;
        toolPts_.push_back(hit);
        return "need|多边形：已选 " + std::to_string(toolPts_.size())
            + " 个顶点，点第一个顶点闭合";
    }

    // 交点：依次点选两个对象
    if (tool_ == "intersect") {
        GeoElement *o = pickObject(px, py);
        if (!o) return "need|交点：请点选直线/线段/圆/函数等对象";
        if (!toolObj_) {
            toolObj_ = o;
            o->selected = true;
            return "need|交点：再点选第二个对象";
        }
        if (o == toolObj_) {
            return "need|交点：两对象相同，请重新点选第二个";
        }
        o->selected = true;   // 依次点亮，第一对象保持选中直至作图完成
        std::string expr = "Intersect(" + toolObj_->label + ", " + o->label + ")";
        toolObj_ = nullptr;
        size_t before = geos_.size();
        if (!input(expr)) {
            return "err|" + (lastError.empty() ? "未找到交点" : lastError);
        }
        std::string labels;
        for (size_t i = before; i < geos_.size(); ++i) {
            if (!labels.empty()) labels += ", ";
            labels += geos_[i]->label;
        }
        return "done|" + labels;
    }

    // 切线：一个点 + 圆/函数（先后不限）
    if (tool_ == "tangent") {
        GeoPoint *pt = pickPoint(px, py);
        GeoElement *o = pt ? nullptr : pickObject(px, py);
        bool oIsCircFn = o && (dynamic_cast<GeoCircle *>(o) || dynamic_cast<GeoFunction *>(o));
        if (pt) {
            if (toolPts_.empty()) {
                toolPts_.push_back(pt);
                pt->selected = true;
            } else if (toolPts_[0] != pt) {
                toolPts_[0]->selected = false;
                toolPts_[0] = pt;
                pt->selected = true;
            }
        } else if (oIsCircFn) {
            if (toolObj_ && toolObj_ != o) toolObj_->selected = false;
            toolObj_ = o;
            o->selected = true;
        } else {
            return "need|切线：请点选一个点和一个圆/函数";
        }
        if (toolPts_.empty() || !toolObj_) {
            return toolPts_.empty() ? "need|切线：再点选一个点"
                                    : "need|切线：再点选圆或函数";
        }
        GeoPoint *P = toolPts_[0];
        GeoElement *Q = toolObj_;
        toolPts_.clear();
        toolObj_ = nullptr;
        std::string expr = "Tangent(" + P->label + ", " + Q->label + ")";
        size_t before = geos_.size();
        if (!input(expr)) {
            return "err|" + (lastError.empty() ? "无切线" : lastError);
        }
        std::string labels;
        for (size_t i = before; i < geos_.size(); ++i) {
            if (!labels.empty()) labels += ", ";
            labels += geos_[i]->label;
        }
        return "done|" + labels;
    }

    // 附着/脱离点（四十五包，上游 同名工具）：点 → 路径=附着（参数取
    // 指针投影）；依附点 → 任意处=脱离（保留当前坐标）。中途点别的点=
    // 改选该点；附着步点空白=保持等待
    if (tool_ == "attachDetach") {
        if (!toolObj_) {
            if (!hit) {
                return "need|附着/脱离：先点选一个点（自由点将附着，依附点将脱离）";
            }
            toolObj_ = hit;
            hit->selected = true;
            if (hit->path) {
                return "need|" + hit->label + " 在 " + hit->path->label
                    + " 上：点画布任意处脱离";
            }
            return "need|附着：再点选路径（线段/直线/射线/向量/圆/多边形/函数）";
        }
        GeoPoint *pt = static_cast<GeoPoint *>(toolObj_);
        if (pt->path) {
            // 脱离步：点别的点改选，其余（空白/路径）一律脱离
            if (hit && hit != pt) {
                pt->selected = false;
                toolObj_ = hit;
                hit->selected = true;
                if (hit->path) {
                    return "need|" + hit->label + " 在 " + hit->path->label
                        + " 上：点画布任意处脱离";
                }
                return "need|附着：再点选路径（线段/直线/射线/向量/圆/多边形/函数）";
            }
            const std::string pre = getXml();
            toolObj_ = nullptr;
            if (!detachPoint(pt)) {
                return "err|脱离失败";
            }
            commitUndoSnapshot(pre);
            return "done|" + pt->label;
        }
        // 附着步：点别的点改选；点路径附着；点空白保持等待
        if (hit && hit != pt) {
            pt->selected = false;
            toolObj_ = hit;
            hit->selected = true;
            if (hit->path) {
                return "need|" + hit->label + " 在 " + hit->path->label
                    + " 上：点画布任意处脱离";
            }
            return "need|附着：再点选路径（线段/直线/射线/向量/圆/多边形/函数）";
        }
        GeoElement *o = pickObject(px, py);
        if (!o) {
            return "need|附着：再点选路径（线段/直线/射线/向量/圆/多边形/函数）";
        }
        const std::string pre = getXml();
        toolObj_ = nullptr;
        if (!attachPoint(pt, o, wx, wy)) {
            return "err|" + (lastError.empty() ? "无法附着" : lastError);
        }
        commitUndoSnapshot(pre);
        return "done|" + pt->label;
    }

    // 平移：对象 → 向量起点 → 向量终点（辅助向量隐藏，上游 工具同构）
    if (tool_ == "translate") {
        if (!toolObj_) {
            GeoElement *o = pickObject(px, py);
            if (!o) return "need|平移：请点选要平移的对象";
            toolObj_ = o;
            o->selected = true;
            return "need|平移：点选向量起点";
        }
        GeoElement *first = toolObj_;
        if (toolPts_.empty()) {
            if (!hit) hit = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
            hit->selected = true;
            toolPts_.push_back(hit);
            return "need|平移：点选向量终点";
        }
        GeoPoint *A = toolPts_[0];
        GeoPoint *B = hit ? hit : static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        B->selected = true;
        toolObj_ = nullptr;
        toolPts_.clear();
        size_t before = geos_.size();
        if (!input("Vector(" + A->label + ", " + B->label + ")")) {
            return "err|" + (lastError.empty() ? "无法创建向量" : lastError);
        }
        geos_[before]->visible = false;   // 辅助向量不显示（代数区仍可见）
        if (!input("Translate(" + first->label + ", " + geos_[before]->label + ")")) {
            return "err|" + (lastError.empty() ? "无法平移该对象" : lastError);
        }
        return "done|" + geos_[geos_.size() - 1]->label;
    }

    // 反射：对象 → 镜像线（点反射走命令 Reflect(对象, 点)）
    if (tool_ == "reflect") {
        if (!toolObj_) {
            GeoElement *o = pickObject(px, py);
            if (!o) return "need|反射：请点选要反射的对象";
            toolObj_ = o;
            o->selected = true;
            return "need|反射：点选镜像线（直线/线段/射线）";
        }
        GeoElement *first = toolObj_;
        GeoElement *mirror = pickObject(px, py);
        GeoType mt = mirror ? mirror->type() : GeoType::Point;
        if (mt != GeoType::Line && mt != GeoType::Segment && mt != GeoType::Ray) {
            return "err|反射：镜像须为直线/线段/射线（点反射请输入 Reflect(对象, 点)）";
        }
        mirror->selected = true;
        toolObj_ = nullptr;
        size_t before = geos_.size();
        if (!input("Reflect(" + first->label + ", " + mirror->label + ")")) {
            return "err|" + (lastError.empty() ? "无法反射该对象" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    // 二十三包 工具第一批（上游经典版工具盘补缺，全部映射到既有/扩展命令）

    // 垂线 / 平行线：直线类对象 + 点（先后不限）——上游 PerpendicularLine(P,l)
    // / Line(P,l)。直线类 = 直线/线段/射线/（线性）函数——上游 工具
    // EuclidianController.parallel/orthogonal 的 getSelectedFunctionList 同款：
    // 函数当方向用（GeoFunction 的 Lineable2D.getX = 一次式斜率、非一次式
    // 为 NaN 不成线），非线性函数由输入路径报「须为一次式」；空白处点按
    // 先建自由点（上游 工具同）
    if (tool_ == "perpLine" || tool_ == "parLine") {
        const char *name = tool_ == "perpLine" ? "垂线" : "平行线";
        GeoElement *o = pickObject(px, py);
        bool oIsBase = o && (o->type() == GeoType::Line || o->type() == GeoType::Segment
            || o->type() == GeoType::Ray || o->type() == GeoType::Function);
        if (oIsBase) {
            if (toolObj_ && toolObj_ != o) toolObj_->selected = false;
            toolObj_ = o;
            o->selected = true;
        } else if (hit) {
            if (!toolPts_.empty() && toolPts_[0] != hit) {
                toolPts_[0]->selected = false;
                toolPts_.clear();
            }
            hit->selected = true;
            toolPts_.push_back(hit);
        } else {
            if (!toolPts_.empty()) {
                toolPts_[0]->selected = false;
                toolPts_.clear();
            }
            GeoPoint *np = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
            np->selected = true;
            toolPts_.push_back(np);
        }
        if (toolPts_.empty() || !toolObj_) {
            return toolPts_.empty()
                ? "need|" + std::string(name) + "：再点选一个点"
                : "need|" + std::string(name) + "：再点选直线/线段/射线/函数";
        }
        GeoPoint *P = toolPts_[0];
        GeoElement *L = toolObj_;
        toolPts_.clear();
        toolObj_ = nullptr;
        const char *cmd = tool_ == "perpLine" ? "PerpendicularLine" : "Line";
        size_t before = geos_.size();
        if (!input(std::string(cmd) + "(" + P->label + ", " + L->label + ")")) {
            return "err|" + (lastError.empty() ? "无法创建" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    // 中心对称：对象 → 对称中心（点）——上游 Reflect(obj, P) 点镜像
    if (tool_ == "reflectPoint") {
        if (!toolObj_) {
            GeoElement *o = hit ? static_cast<GeoElement *>(hit) : pickObject(px, py);
            if (!o) return "need|中心对称：请点选要对称的对象";
            toolObj_ = o;
            o->selected = true;
            return "need|中心对称：点选对称中心";
        }
        GeoElement *first = toolObj_;
        GeoPoint *C = hit ? hit : static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        C->selected = true;
        toolObj_ = nullptr;
        size_t before = geos_.size();
        if (!input("Reflect(" + first->label + ", " + C->label + ")")) {
            return "err|" + (lastError.empty() ? "无法对称该对象" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    // 二十六包 工具第二批（上游 需参数对话框的工具）：拾取完成后进 "ask|"
    // 态，宿主弹参数框，确认后经 toolParam(v) 完成构造

    // 旋转 / 位似：对象 → 中心点 → 参数（上游 Rotate Around Point / Dilate
    // from Point 工具流；点也可作变换对象，与中心对称工具同口径）
    if (tool_ == "rotate" || tool_ == "dilate") {
        const char *name = tool_ == "rotate" ? "旋转" : "位似";
        if (!toolObj_) {
            GeoElement *o = hit ? static_cast<GeoElement *>(hit) : pickObject(px, py);
            if (!o) return "need|" + std::string(name) + "：请点选要变换的对象";
            toolObj_ = o;
            o->selected = true;
            return "need|" + std::string(name) + "：点选变换中心";
        }
        GeoElement *first = toolObj_;
        GeoPoint *C = hit ? hit : static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        C->selected = true;
        toolObj_ = nullptr;
        toolPts_.clear();
        askKind_ = tool_;
        askObj_ = first;
        askPts_ = { C };
        return tool_ == "rotate"
            ? "ask|旋转：输入角度（度，正=逆时针，可引用滑动条）"
            : "ask|位似：输入比例（可引用滑动条）";
    }

    // 圆（半径）：圆心 → 半径参数（上游 Circle with Center and Radius；
    // Circle(点, 数) 命令支持字面量/表达式/数值对象）
    if (tool_ == "circleRadius") {
        GeoPoint *C = hit ? hit : static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        C->selected = true;
        askKind_ = tool_;
        askObj_ = nullptr;
        askPts_ = { C };
        return "ask|圆（半径）：输入半径";
    }

    // 定长线段：起点 → 长度参数（上游 Segment with Given Length；
    // Segment(点, 数) 沿 x 轴正向，二十四包⑦ 已修渲染）
    if (tool_ == "segLength") {
        GeoPoint *A = hit ? hit : static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        A->selected = true;
        askKind_ = tool_;
        askObj_ = nullptr;
        askPts_ = { A };
        return "ask|定长线段：输入长度（沿 x 轴正向）";
    }

    // 定角：角边点 → 顶点 → 角度参数（上游 Angle with Given Size：
    // Angle(A, B, α) 顶点在第二点，自 BA 边逆时针量 α；toolParam 里以
    // Rotate 生成终边点后走三点 Angle，弧/值随点与参数数值联动重算）
    if (tool_ == "angleSize") {
        if (!hit) hit = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
        hit->selected = true;
        toolPts_.push_back(hit);
        if (toolPts_.size() < 2) {
            return "need|定角：还差 1 个点（第二个是顶点）";
        }
        askKind_ = tool_;
        askObj_ = nullptr;
        askPts_ = toolPts_;
        toolPts_.clear();
        return "ask|定角：输入角度（度，逆时针，可引用滑动条）";
    }

    // 距离/长度：两点，或一点 + 直线类（先后不限）——上游 Distance 工具；
    // 空白处点按先建自由点
    if (tool_ == "dist") {
        GeoElement *o = pickObject(px, py);
        bool oIsBase = o && (o->type() == GeoType::Line || o->type() == GeoType::Segment
            || o->type() == GeoType::Ray);
        if (hit) {
            // 距离工具累加两点（点不同才收，同一点重复点按只提示）
            if (toolPts_.empty() || toolPts_[0] != hit) {
                hit->selected = true;
                toolPts_.push_back(hit);
            }
        } else if (oIsBase) {
            if (toolObj_ && toolObj_ != o) toolObj_->selected = false;
            toolObj_ = o;
            o->selected = true;
        } else {
            if (!toolPts_.empty()) {
                toolPts_[0]->selected = false;
                toolPts_.clear();
            }
            GeoPoint *np = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
            np->selected = true;
            toolPts_.push_back(np);
        }
        size_t before = geos_.size();
        if (toolPts_.size() >= 2) {
            GeoPoint *A = toolPts_[0], *B = toolPts_[1];
            toolPts_.clear();
            toolObj_ = nullptr;
            if (!input("Distance(" + A->label + ", " + B->label + ")")) {
                return "err|" + (lastError.empty() ? "无法测量" : lastError);
            }
            return "done|" + geos_[before]->label;
        }
        if (toolPts_.size() == 1 && toolObj_) {
            GeoPoint *P = toolPts_[0];
            GeoElement *L = toolObj_;
            toolPts_.clear();
            toolObj_ = nullptr;
            if (!input("Distance(" + P->label + ", " + L->label + ")")) {
                return "err|" + (lastError.empty() ? "无法测量" : lastError);
            }
            return "done|" + geos_[before]->label;
        }
        if (toolPts_.empty() && !toolObj_) {
            return "need|距离：再点选一个点或直线/线段/射线";
        }
        return toolPts_.empty() ? "need|距离：再点选一个点"
                                : "need|距离：再点选第二个点或一条直线/线段/射线";
    }

    // 面积：点选多边形/圆（上游 Area 工具；结果为数值对象，进代数区）
    if (tool_ == "area") {
        GeoElement *o = pickObject(px, py);
        if (!o || (o->type() != GeoType::Polygon && o->type() != GeoType::Circle)) {
            return "need|面积：请点选多边形或圆";
        }
        size_t before = geos_.size();
        if (!input("Area(" + o->label + ")")) {
            return "err|" + (lastError.empty() ? "无法测量" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    // 斜率：点选直线/线段/一次函数（上游 Slope 工具；仅生成数值，不画斜率三角）
    if (tool_ == "slope") {
        GeoElement *o = pickObject(px, py);
        if (!o || (o->type() != GeoType::Line && o->type() != GeoType::Segment
            && o->type() != GeoType::Function)) {
            return "need|斜率：请点选直线/线段/一次函数";
        }
        size_t before = geos_.size();
        if (!input("Slope(" + o->label + ")")) {
            return "err|" + (lastError.empty() ? "无法测量" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    // 相等向量：点 + 向量（先后不限）——上游 Vector(P, u) 平移副本；
    // 空白处点按先建自由点。命中裁决（二十四包⑥）：点的 18vp 命中半径
    // 会把短向量中段可点区挤没，点箭身总被抢成端点——改为点只在「内圈」
    // （视觉点核 6vp，上游 DrawPoint 命中同级）内优先，其余箭身归向量；
    // 端点在向量上、按距离比较永远向量赢，故用内圈而不是就近
    if (tool_ == "vecFromPoint") {
        double ptD = 1e9;
        GeoPoint *pt = pickPoint(px, py, &ptD);
        GeoElement *o = pickObject(px, py);
        bool oIsVec = o && o->type() == GeoType::Vector;
        bool ptWins = pt && (!oIsVec || ptD <= 6.0 * view.uiScale);
        if (pt && ptWins) {
            if (!toolPts_.empty() && toolPts_[0] != pt) {
                toolPts_[0]->selected = false;
                toolPts_.clear();
            }
            pt->selected = true;
            toolPts_.push_back(pt);
        } else if (oIsVec) {
            if (toolObj_ && toolObj_ != o) toolObj_->selected = false;
            toolObj_ = o;
            o->selected = true;
        } else {
            if (!toolPts_.empty()) {
                toolPts_[0]->selected = false;
                toolPts_.clear();
            }
            GeoPoint *np = static_cast<GeoPoint *>(makeFreePoint(wx, wy));
            np->selected = true;
            toolPts_.push_back(np);
        }
        if (toolPts_.empty() || !toolObj_) {
            return toolPts_.empty() ? "need|相等向量：再点选一个点"
                                    : "need|相等向量：再点选向量";
        }
        GeoPoint *P = toolPts_[0];
        GeoElement *V = toolObj_;
        toolPts_.clear();
        toolObj_ = nullptr;
        size_t before = geos_.size();
        if (!input("Vector(" + P->label + ", " + V->label + ")")) {
            return "err|" + (lastError.empty() ? "无法创建向量" : lastError);
        }
        return "done|" + geos_[before]->label;
    }

    return "err|未知工具";
}

// 二十六包 工具第二批：参数确认（toolTap "ask|" 态的收尾）。参数文本原样
// 进命令表达式——纯数字是字面量，滑动条标签/表达式（如 2α）由 input() 的
// 常规解析兜住，上游 工具的「参数可动态」同款。失败保留 ask 态让弹窗改再
// 确认；成功清 ask 态 + 清点亮（与 toolTap "done|" 收尾一致）
std::string Kernel::toolParam(const std::string &v)
{
    if (askKind_.empty()) return "err|当前没有等待输入参数的工具";
    std::string t = v;
    while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.erase(t.begin());
    while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
    if (t.empty()) return "err|参数为空";
    // 弹窗期间对象可能被面板操作级联删除——悬空指针防御，作废重选
    if (askObj_ && !toolListed(askObj_)) {
        askKind_.clear();
        askObj_ = nullptr;
        askPts_.clear();
        return "err|对象已被删除，请重新操作";
    }
    for (auto *p : askPts_) {
        if (!toolListed(p)) {
            askKind_.clear();
            askObj_ = nullptr;
            askPts_.clear();
            return "err|点已被删除，请重新操作";
        }
    }
    const size_t before = geos_.size();
    const std::string kind = askKind_;
    std::string made;
    if (kind == "rotate") {
        if (!input("Rotate(" + askObj_->label + ", " + t + ", " + askPts_[0]->label + ")")) {
            return "err|" + (lastError.empty() ? "无法旋转该对象" : lastError);
        }
        made = geos_[before]->label;
    } else if (kind == "dilate") {
        if (!input("Dilate(" + askObj_->label + ", " + t + ", " + askPts_[0]->label + ")")) {
            return "err|" + (lastError.empty() ? "无法位似该对象" : lastError);
        }
        made = geos_[before]->label;
    } else if (kind == "circleRadius") {
        if (!input("Circle(" + askPts_[0]->label + ", " + t + ")")) {
            return "err|" + (lastError.empty() ? "无法创建圆" : lastError);
        }
        made = geos_[before]->label;
    } else if (kind == "segLength") {
        if (!input("Segment(" + askPts_[0]->label + ", " + t + ")")) {
            return "err|" + (lastError.empty() ? "无法创建线段" : lastError);
        }
        made = geos_[before]->label;
    } else if (kind == "angleSize") {
        // 上游 Angle with Given Size：先建可见的终边点（Rotate，上游 工具
        // 同款产物），角对象走三点 Angle——值/弧随角边点、顶点与参数数值
        // （滑动条引用时）联动重算
        if (!input("Rotate(" + askPts_[0]->label + ", " + t + ", " + askPts_[1]->label + ")")) {
            return "err|" + (lastError.empty() ? "无法旋转角边点" : lastError);
        }
        const std::string leg = geos_[before]->label;
        if (!input("Angle(" + askPts_[0]->label + ", " + askPts_[1]->label + ", " + leg + ")")) {
            return "err|" + (lastError.empty() ? "无法创建角" : lastError);
        }
        made = geos_[before + 1]->label + ", " + leg;
    } else {
        askKind_.clear();
        askObj_ = nullptr;
        askPts_.clear();
        return "err|未知的参数工具";
    }
    askKind_.clear();
    askObj_ = nullptr;
    askPts_.clear();
    for (auto &g : geos_) g->selected = false;
    return "done|" + made;
}

// ask 态防御：指针仍在构造列表（未被级联删除）
bool Kernel::toolListed(GeoElement *e) const
{
    for (const auto &g : geos_) {
        if (g.get() == e) return true;
    }
    return false;
}

// ---------- 绘图 ----------

Geometry Kernel::CircleGeom(const GeoCircle &c, const EuclidianView &v)
{
    Geometry g;
    g.x = v.px(c.ccx());
    g.y = v.py(c.ccy());
    g.r = c.radius * v.xscale;   // 圆用 xscale（上游 updateCircle 同）
    return g;
}

// 函数绘图：自适应采样（N4 升级，上游 CurvePlotter 思路）。
// 基准分段 ~64 段起步，逐段在中点检验"平直度"：中点到弦的像素距离 > 1.5px
// 或弦转角 > 15° 就减半步长细分，平坦则步长加倍——曲线平缓处点少、弯曲/
// 陡峭处点密（总量预算封顶，防振荡函数拖垮帧率）。NaN/Inf（未定义区）断线；
// 有限值间的渐近线跳变（如 tan）按 上游 行为画出竖直 Connector，由画布裁剪。
void Kernel::appendFunctionPlot(const GeoFunction &f, std::vector<DrawCmd> &out,
                                bool sel) const
{
    DrawCmd cmd;
    cmd.op = DrawOp::Polyline;
    cmd.color = ObjectColor(f.color, darkTheme);
    cmd.lineWidth = f.lineThickness / 3.0f;

    const double x0 = view.xmin() - 2.0 / view.xscale;
    const double x1 = view.xmax() + 2.0 / view.xscale;
    const double range = x1 - x0;
    const double baseStep = range / 64.0;
    const double minStep = range / 8192.0;
    int budget = 6000;   // 单条曲线点数/细分预算

    auto flush = [&]() {
        if (cmd.pts.size() >= 4) {
            if (sel) {
                // 选中：先垫一条加宽半透明亮色曲线
                DrawCmd h = cmd;
                h.color = darkTheme ? "rgba(143,182,255,0.40)" : "rgba(0,125,255,0.30)";
                h.lineWidth = cmd.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(cmd);
        }
        cmd.pts.clear();
    };
    auto emit = [&](double wx, double wy) {
        cmd.pts.push_back((float)view.px(wx));
        cmd.pts.push_back((float)view.py(wy));
    };

    double xa = x0;
    double ya = f.evalAt(xa);
    double h = baseStep;
    bool started = false;
    while (xa < x1 && budget > 0) {
        if (!std::isfinite(ya)) {
            // 未定义区：大步跳过，直到重新出现有限值
            if (started) flush();
            started = false;
            double xb = std::min(xa + baseStep, x1);
            ya = f.evalAt(xb);
            xa = xb;
            h = baseStep;
            continue;
        }
        if (!started) {
            emit(xa, ya);
            started = true;
        }
        double xb = std::min(xa + h, x1);
        if (xb <= xa + minStep * 0.5) break;
        double yb = f.evalAt(xb);
        --budget;
        if (!std::isfinite(yb)) {
            if (h > minStep) {
                h *= 0.5;   // 断点在前方：减半定位
                continue;
            }
            flush();        // 断点定位到底：在此断开
            started = false;
            xa = xb;
            ya = yb;
            h = baseStep;
            continue;
        }
        // 中点平直度检验
        bool flat = false;
        double xm = 0.5 * (xa + xb);
        double ym = f.evalAt(xm);
        --budget;
        if (std::isfinite(ym)) {
            double pax = view.px(xa), pay = view.py(ya);
            double pbx = view.px(xb), pby = view.py(yb);
            double pmx = view.px(xm), pmy = view.py(ym);
            double dx = pbx - pax, dy = pby - pay;
            double len = sqrt(dx * dx + dy * dy);
            double err = len < 1e-9
                ? sqrt((pmx - pax) * (pmx - pax) + (pmy - pay) * (pmy - pay))
                : fabs(dy * (pmx - pax) - dx * (pmy - pay)) / len;
            double a1 = atan2(pmy - pay, pmx - pax);
            double a2 = atan2(pby - pmy, pbx - pmx);
            double bend = fabs(a2 - a1);
            if (bend > M_PI) bend = 2 * M_PI - bend;
            flat = err <= 1.5 * view.uiScale && bend <= 0.26;
        }
        if (flat) {
            emit(xb, yb);
            xa = xb;
            ya = yb;
            h = std::min(h * 2.0, baseStep);   // 平坦则加速
        } else if (h > minStep) {
            h *= 0.5;
        } else {
            // 已到最小步长仍不平（陡峭/振荡）：接受该步防死循环
            emit(xb, yb);
            xa = xb;
            ya = yb;
            h = baseStep;
        }
    }
    flush();
}

// "#RRGGBB" + 不透明度 0-1 → "rgba(r,g,b,a)"。十四轮：ArkUI Canvas 的 fillStyle
// 把 8 位 hex 按 #AARRGGBB 误读（#RRGGBBAA 的 alpha 字节落进蓝色通道），症状即
// 滑杆调大只"变色"不变深、透明度 0 不消失；所有填充色一律走 rgba() 显式通道。
static std::string RgbaFromHex(const std::string &hex, double a)
{
    auto hv = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    int r = 0, g = 0, b = 0;
    if (hex.size() >= 7 && hex[0] == '#') {
        r = hv(hex[1]) * 16 + hv(hex[2]);
        g = hv(hex[3]) * 16 + hv(hex[4]);
        b = hv(hex[5]) * 16 + hv(hex[6]);
    }
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    char buf[48];
    snprintf(buf, sizeof(buf), "rgba(%d,%d,%d,%.3f)", r, g, b, a);
    return buf;
}

// 折线按弧长切 dash（MultiSeg 每段 moveTo+lineTo，on/off 像素交替）。
// dash 相位跨段累计（十三轮修复：原实现每段从 0 重启，列采样折线段长
// 2~10px 恒落在首段 on 相位内 → 整线呈实线，仅陡峭区截断出的长段
// 恰好切出虚线）
static void AppendDashSegs(const std::vector<float> &pts, double on, double off,
                           std::vector<float> &out)
{
    double phase = on;   // 当前 dash 元素剩余长度
    bool dashOn = true;
    for (size_t i = 0; i + 3 < pts.size(); i += 2) {
        const double ax = pts[i], ay = pts[i + 1], bx = pts[i + 2], by = pts[i + 3];
        const double len = sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
        if (len < 1e-9) continue;
        const double ux = (bx - ax) / len, uy = (by - ay) / len;
        double t = 0;
        while (t < len - 1e-9) {
            const double step = std::min(phase, len - t);
            if (dashOn) {
                out.push_back((float)(ax + ux * t));
                out.push_back((float)(ay + uy * t));
                out.push_back((float)(ax + ux * (t + step)));
                out.push_back((float)(ay + uy * (t + step)));
            }
            t += step;
            phase -= step;
            if (phase <= 1e-9) {
                dashOn = !dashOn;
                phase = dashOn ? on : off;
            }
        }
    }
}

// 1.0.6.3 图表列表绘制。bar/hist = 半透明填充 + 黑描边矩形；line = 折线；
// scatter = 实心点；box = 箱体填充 + 触须/端帽/中位线；stem = 茎叶文本
//（画布左上固定区，逐行 Text）。普通列表（chart 空）不绘制——走 render
// 的 GeoType::List 分支判空
void Kernel::appendChartPlot(const GeoList &L, std::vector<DrawCmd> &out,
                             const std::string &selHalo) const
{
    (void)selHalo;
    const std::string objC = ObjectColor(L.color, darkTheme);
    const std::string fillC = RgbaFromHex(objC, 0.45);
    const std::string lineC = ObjectColor(GeoColor{0, 0, 0}, darkTheme);

    if (L.chart == "bar" || L.chart == "hist") {
        for (size_t i = 0; i < L.values.size(); ++i) {
            double x0, x1;
            if (L.chart == "hist") {
                if (i + 1 >= L.chartX.size()) break;
                x0 = L.chartX[i];
                x1 = L.chartX[i + 1];
            } else {
                const double xc = i < L.chartX.size() ? L.chartX[i] : (double)(i + 1);
                const double w = L.chartB > 0 ? L.chartB : 0.8;
                x0 = xc - w / 2;
                x1 = xc + w / 2;
            }
            const double h = L.values[i];
            if (!std::isfinite(h)) continue;
            const double y0 = std::min(0.0, h), y1 = std::max(0.0, h);
            std::vector<float> poly = {
                (float)view.px(x0), (float)view.py(y0),
                (float)view.px(x1), (float)view.py(y0),
                (float)view.px(x1), (float)view.py(y1),
                (float)view.px(x0), (float)view.py(y1),
            };
            DrawCmd fill;
            fill.op = DrawOp::FillPoly;
            fill.color = fillC;
            fill.pts = poly;
            out.push_back(fill);
            DrawCmd st;
            st.op = DrawOp::Polyline;
            st.color = lineC;
            st.lineWidth = 1.6f;
            st.pts = poly;
            st.pts.push_back(poly[0]);
            st.pts.push_back(poly[1]);
            out.push_back(st);
        }
        return;
    }

    if (L.chart == "line") {
        const size_t n = std::min(L.values.size(), L.chartX.size());
        if (n >= 2) {
            DrawCmd d;
            d.op = DrawOp::Polyline;
            d.color = objC;
            d.lineWidth = L.lineThickness / 3.0f;
            for (size_t i = 0; i < n; ++i) {
                d.pts.push_back((float)view.px(L.chartX[i]));
                d.pts.push_back((float)view.py(L.values[i]));
            }
            out.push_back(d);
        } else if (n == 1) {
            DrawCmd d;
            d.op = DrawOp::FillCircle;
            d.color = objC;
            d.pts = { (float)view.px(L.chartX[0]), (float)view.py(L.values[0]),
                      (float)(3.0 * view.uiScale) };
            out.push_back(d);
        }
        return;
    }

    if (L.chart == "scatter") {
        const size_t n = std::min(L.values.size(), L.chartX.size());
        for (size_t i = 0; i < n; ++i) {
            if (!std::isfinite(L.chartX[i]) || !std::isfinite(L.values[i])) continue;
            DrawCmd d;
            d.op = DrawOp::FillCircle;
            d.color = objC;
            d.pts = { (float)view.px(L.chartX[i]), (float)view.py(L.values[i]),
                      (float)(3.0 * view.uiScale) };
            out.push_back(d);
        }
        return;
    }

    if (L.chart == "box") {
        if (L.values.size() < 5) return;
        const double mn = L.values[0], q1 = L.values[1], md = L.values[2];
        const double q3 = L.values[3], mx = L.values[4];
        const double yC = L.chartA;
        const double h = L.chartB > 0 ? L.chartB : 1.0;
        const double y0 = yC - h / 2, y1 = yC + h / 2;
        std::vector<float> poly = {
            (float)view.px(q1), (float)view.py(y0),
            (float)view.px(q3), (float)view.py(y0),
            (float)view.px(q3), (float)view.py(y1),
            (float)view.px(q1), (float)view.py(y1),
        };
        DrawCmd fill;
        fill.op = DrawOp::FillPoly;
        fill.color = fillC;
        fill.pts = poly;
        out.push_back(fill);
        DrawCmd st;
        st.op = DrawOp::Polyline;
        st.color = lineC;
        st.lineWidth = 1.6f;
        st.pts = poly;
        st.pts.push_back(poly[0]);
        st.pts.push_back(poly[1]);
        out.push_back(st);
        // 触须 + 端帽 + 中位线
        auto seg = [&](double ax, double ay, double bx, double by, float lw,
                       const std::string &c) {
            DrawCmd s;
            s.op = DrawOp::Segment;
            s.color = c;
            s.lineWidth = lw;
            s.pts = { (float)view.px(ax), (float)view.py(ay),
                      (float)view.px(bx), (float)view.py(by) };
            out.push_back(s);
        };
        seg(mn, yC, q1, yC, 1.6f, lineC);
        seg(q3, yC, mx, yC, 1.6f, lineC);
        seg(mn, y0, mn, y1, 1.6f, lineC);
        seg(mx, y0, mx, y1, 1.6f, lineC);
        seg(md, y0, md, y1, 2.4f, objC);
        return;
    }

    if (L.chart == "stem") {
        // 茎 = trunc(v/10)，叶 = v − 10·茎（负值叶带符号显示）；values 已排序，
        // 同茎自然聚组
        std::vector<std::pair<long long, std::vector<long long>>> groups;
        for (double v : L.values) {
            if (!std::isfinite(v)) continue;
            const long long iv = (long long)llround(v);
            const long long st2 = (long long)trunc((double)iv / 10.0);
            const long long lf = iv - st2 * 10;
            if (!groups.empty() && groups.back().first == st2) {
                groups.back().second.push_back(lf);
            } else {
                groups.push_back({ st2, { lf } });
            }
        }
        // 画布左上固定区绘制：y 从 115·us 起（菜单/撤销按钮组占据 ~0..100vp，
        // 50vp 起画会被 ☰ 按钮盖住——真机验证后下移）
        const float tx = 40.0f * view.uiScale, ty = 115.0f * view.uiScale;
        const float lh = 18.0f * view.uiScale;
        float y = ty;
        for (const auto &gp : groups) {
            // 茎叶分隔用全形竖线：帧协议按 '|' 分字段，ASCII 管道会切碎
            // Text 指令行（真机首测：画布不显示）
            std::string s = std::to_string(gp.first) + " │ ";
            for (size_t i = 0; i < gp.second.size(); ++i) {
                if (i) s += " ";
                s += std::to_string(gp.second[i]);
            }
            DrawCmd d;
            d.op = DrawOp::Text;
            d.color = objC;
            d.pts = { tx, y };
            d.text = s;
            out.push_back(d);
            y += lh;
        }
        return;
    }
}

// N6 批次⑤：不等式区域。填充 = 对象色低透明度（默认 0.10，选中行滑杆可调，
// fillAlpha 为不透明度：0 → 不画填充完全消失，1 → 对象色实底）；边界 strict(< >)
// 虚线、含等号实线；竖直半平面（x>3 类）整幅矩形 + 竖直边界。
// 列采样 2px 一格，非有限值（渐近线外）断开成多段各自闭合。
void Kernel::appendInequalityPlot(const GeoFunction &f, std::vector<DrawCmd> &out) const
{
    const std::string base = ObjectColor(f.color, darkTheme);
    const std::string fillCol = RgbaFromHex(base, f.fillAlpha);
    const std::string selHalo = darkTheme ? "rgba(143,182,255,0.40)" : "rgba(0,125,255,0.30)";
    const bool combo = !f.ineqChain.empty() || !f.ineqDNF.empty();
    const bool strict = combo
        ? std::all_of(f.ineqChain.begin(), f.ineqChain.end(),
                      [](const GeoFunction::IneqCond &c) { return c.strict; })
          && std::all_of(f.ineqDNF.begin(), f.ineqDNF.end(),
                      [](const std::vector<GeoFunction::IneqCond> &cl) {
                          return std::all_of(cl.begin(), cl.end(),
                              [](const GeoFunction::IneqCond &c) { return c.strict; });
                      })
        : (f.ineqOp == 1 || f.ineqOp == 3);
    const bool below = f.ineqOp == 1 || f.ineqOp == 2;   // y<expr：曲线下方
    const double pad = 4000.0;   // 填充多边形越界钳制（防 exp 类巨值坐标）

    if (f.ineqVertical) {
        const float bx = (float)view.px(f.ineqX);
        const bool right = f.ineqOp == 3 || f.ineqOp == 4;
        if (f.fillAlpha > 0.003) {
            DrawCmd fill;
            fill.op = DrawOp::FillPoly;
            fill.color = fillCol;
            if (right) {
                fill.pts = { bx, (float)-pad, (float)(view.width + pad), (float)-pad,
                             (float)(view.width + pad), (float)(view.height + pad),
                             bx, (float)(view.height + pad) };
            } else {
                fill.pts = { (float)-pad, (float)-pad, bx, (float)-pad,
                             bx, (float)(view.height + pad), (float)-pad, (float)(view.height + pad) };
            }
            out.push_back(fill);
        }
        if (f.selected) {
            DrawCmd h;
            h.op = DrawOp::Segment;
            h.color = selHalo;
            h.lineWidth = f.lineThickness / 3.0f + 8.0f * view.uiScale;
            h.pts = { bx, 0.0f, bx, (float)view.height };
            out.push_back(h);
        }
        if (strict) {
            DrawCmd seg;
            seg.op = DrawOp::MultiSeg;
            seg.color = base;
            seg.lineWidth = f.lineThickness / 3.0f;
            for (double yy = 0.0; yy < (double)view.height; yy += 11.0 * view.uiScale) {
                seg.pts.push_back(bx);
                seg.pts.push_back((float)yy);
                seg.pts.push_back(bx);
                seg.pts.push_back((float)std::min(yy + 6.0 * view.uiScale, (double)view.height));
            }
            out.push_back(seg);
        } else {
            DrawCmd seg;
            seg.op = DrawOp::Segment;
            seg.color = base;
            seg.lineWidth = f.lineThickness / 3.0f;
            seg.pts = { bx, 0.0f, bx, (float)view.height };
            out.push_back(seg);
        }
        return;
    }
    if (f.ineqTwoVar || f.ineqCombo()) {
        // 批次⑥：两变量区域（x^2+y^2<r^2 类）。逐列沿 y 扫 L−R 的符号，
        // 状态翻转处对 s 二分求边界交点 → 相邻交点成对构成竖直 quad 填充；
        // 偶/奇序交点分别连成两条边界折线（跳变 >24px 或列间断开），
        // strict 边界走虚线。步长随画布尺寸放大控制每帧求值次数。
        // 十三轮修复：①列扫描端点已在区域内时补"虚拟交点"参与配对并决定
        //   奇偶序（区域越出可见范围时交点数变奇 → 配对错位，被 UI 遮挡
        //   一侧出现竖直填充空缺、超过半遮挡整体不可见）；虚拟交点只参与
        //   排序配对，不画边界（避免沿画布边拉出水平线）。②边界折线在
        //   陡峭段（竖直切线，如圆与 x 轴的交点附近）列间二分细分后再发射，
        //   消除 >24px 跳变断行把小段整段丢弃造成的缺口。
        // 1.0.7.1 链式区域复用同一套机制：g = max(各段归一化值)（每段按
        // lt 归一到「<0 在区域内」）——max 连续，g 的符号翻转即 AND 区域
        // 边界；ref（滑条引用）按对象现值解析，随拖动联动
        const bool lt = f.ineqOp == 1 || f.ineqOp == 2;
        auto resolver = [this](const std::string &nm) -> double {
            GeoNumeric *n = dynamic_cast<GeoNumeric *>(lookup(nm));
            return n ? n->value : NAN;
        };
        auto evalS = [&](double wx, double wy) -> double {
            if (f.ineqTwoVar) {
                return f.ineqL.evalXY(wx, wy) - f.ineqR.evalXY(wx, wy);
            }
            return IneqComboG(f, wx, wy, resolver);
        };
        auto insideS = [&](double s) {
            if (f.ineqTwoVar) {
                return std::isfinite(s) && (lt ? s < 0.0 : s > 0.0);
            }
            return std::isfinite(s) && s < 0.0;
        };
        const int colW = std::max(2, (int)std::ceil(view.width / 320.0));
        const int rowPx = std::max(3, (int)std::ceil(view.height / 240.0));
        const int rows = std::max(4, view.height / rowPx);
        const double yTopW = view.wy(0), yBotW = view.wy(view.height);
        // 单列扫描：沿 y 找符号翻转并二分求交点（世界 y 升序），同时给出
        // 上下可见边处的"是否在区域内"状态
        auto scanCross = [&](double xc, std::vector<double> &cross, bool *inBot, bool *inTop) {
            cross.clear();
            const double wx = view.wx(xc);
            double prevWY = yBotW;
            bool prevIn = insideS(evalS(wx, prevWY));
            *inBot = prevIn;
            for (int r2 = 1; r2 <= rows; ++r2) {
                const double wy = yBotW + (yTopW - yBotW) * r2 / rows;
                const bool in = insideS(evalS(wx, wy));
                if (in != prevIn) {
                    double a = prevWY, b = wy;
                    double sa = evalS(wx, a);
                    for (int it = 0; it < 14; ++it) {
                        const double mid = 0.5 * (a + b);
                        const double sm = evalS(wx, mid);
                        if (!std::isfinite(sm)) break;
                        if ((sm < 0.0) == (sa < 0.0)) { a = mid; sa = sm; }
                        else b = mid;
                    }
                    cross.push_back(0.5 * (a + b));
                }
                prevIn = in;
                prevWY = wy;
            }
            *inTop = prevIn;
        };
        // 列节点：真实交点 + 虚拟边交点（isReal=false），按世界 y 排序。
        // 配对与奇偶序都以这份列表为准 → 区域越出可见边时奇偶依然对齐。
        struct ColNode {
            double x;
            std::vector<std::pair<double, bool>> pts;   // (世界 y, 是否真实交点)
        };
        auto buildNode = [&](double xc, ColNode &n, std::vector<double> &cross) {
            n.x = xc;
            n.pts.clear();
            bool inBot = false, inTop = false;
            scanCross(xc, cross, &inBot, &inTop);
            for (double y : cross) n.pts.push_back({ y, true });
            if (inBot) n.pts.push_back({ yBotW, false });
            if (inTop) n.pts.push_back({ yTopW, false });
            std::sort(n.pts.begin(), n.pts.end(),
                      [](const std::pair<double, bool> &a, const std::pair<double, bool> &b) {
                          return a.first < b.first;
                      });
        };
        std::vector<ColNode> nodes;
        nodes.reserve((size_t)(view.width / colW) + 2);
        std::vector<double> crossTmp;
        for (int px = 0; px < view.width; px += colW) {
            ColNode n;
            buildNode(px + colW * 0.5, n, crossTmp);
            nodes.push_back(std::move(n));
        }
        // 填充：成对交点区间 → 竖直 quad（虚拟交点补齐越出可见边的区间）
        if (f.fillAlpha > 0.003) {
            for (const ColNode &n : nodes) {
                const float xl = (float)(n.x - colW * 0.5);
                const float xr = xl + colW;
                for (size_t ci = 0; ci + 1 < n.pts.size(); ci += 2) {
                    const float pyA = (float)view.py(n.pts[ci].first);
                    const float pyB = (float)view.py(n.pts[ci + 1].first);
                    if (fabs(pyB - pyA) < 0.5f) continue;   // py 方向随 wy 反序，取幅值
                    DrawCmd fill;
                    fill.op = DrawOp::FillPoly;
                    fill.color = fillCol;
                    fill.pts = { xl, pyA, xr, pyA, xr, pyB, xl, pyB };
                    out.push_back(fill);
                }
            }
        }
        // 边界折线：奇偶序各一条；相邻列同序交点跳变 >24px（陡峭段）先二分
        // 细分再发射，细到 0.75px 仍跳变才真正断行（真不连续/另一叶）
        std::vector<float> runEven, runOdd;
        auto flushLine = [&](std::vector<float> &run) {
            if (run.size() < 4) { run.clear(); return; }
            if (f.selected) {
                DrawCmd h;
                h.op = DrawOp::Polyline;
                h.color = selHalo;
                h.lineWidth = f.lineThickness / 3.0f + 8.0f * view.uiScale;
                h.pts = run;
                out.push_back(h);
            }
            DrawCmd seg;
            if (strict) {
                seg.op = DrawOp::MultiSeg;
                seg.color = base;
                seg.lineWidth = f.lineThickness / 3.0f;
                AppendDashSegs(run, 6.0 * view.uiScale, 5.0 * view.uiScale, seg.pts);
                if (seg.pts.size() >= 4) out.push_back(seg);
            } else {
                seg.op = DrawOp::Polyline;
                seg.color = base;
                seg.lineWidth = f.lineThickness / 3.0f;
                seg.pts = run;
                out.push_back(seg);
            }
            run.clear();
        };
        auto appendPt = [&](std::vector<float> &run, double wy, double xc) {
            const float ppx = (float)xc;
            const float ppy = (float)view.py(wy);
            if (run.size() >= 2) {
                const float ly = run[run.size() - 1];
                const float lx = run[run.size() - 2];
                if (fabs(ppy - ly) > 24.0f || fabs(ppx - lx) > (float)colW + 0.5f) {
                    flushLine(run);
                }
            }
            run.push_back(ppx);
            run.push_back(ppy);
        };
        std::function<void(const ColNode &, const ColNode &, int)> emitRange =
            [&](const ColNode &a, const ColNode &b, int depth) {
            bool need = a.pts.size() != b.pts.size();
            for (size_t k = 0; !need && k < a.pts.size() && k < b.pts.size(); ++k) {
                if (fabs(view.py(b.pts[k].first) - view.py(a.pts[k].first)) > 24.0) need = true;
            }
            if (need && depth < 6 && b.x - a.x > 0.2) {
                ColNode m;
                buildNode(0.5 * (a.x + b.x), m, crossTmp);
                emitRange(a, m, depth + 1);
                emitRange(m, b, depth + 1);
                return;
            }
            for (size_t k = 0; k < b.pts.size(); ++k) {
                if (!b.pts[k].second) continue;   // 虚拟交点定奇偶序，不画边界
                appendPt(k % 2 == 0 ? runEven : runOdd, b.pts[k].first, b.x);
            }
        };
        if (!nodes.empty()) {
            for (size_t k = 0; k < nodes[0].pts.size(); ++k) {
                if (!nodes[0].pts[k].second) continue;
                appendPt(k % 2 == 0 ? runEven : runOdd, nodes[0].pts[k].first, nodes[0].x);
            }
            for (size_t i = 1; i < nodes.size(); ++i) emitRange(nodes[i - 1], nodes[i], 0);
        }
        flushLine(runEven);
        flushLine(runOdd);
        return;
    }
    if (f.body.empty()) return;

    const double x0 = view.xmin() - 1.0 / view.xscale;
    const double x1 = view.xmax() + 1.0 / view.xscale;
    const int steps = std::max(16, (int)(view.width / 2.0));
    const double edgeY = below ? (double)view.height + pad : -pad;
    std::vector<float> run;
    auto flushRun = [&]() {
        if (run.size() < 4) { run.clear(); return; }
        if (f.fillAlpha > 0.003) {
            DrawCmd fill;
            fill.op = DrawOp::FillPoly;
            fill.color = fillCol;
            fill.pts = run;
            fill.pts.push_back(run[run.size() - 2]);
            fill.pts.push_back((float)edgeY);
            fill.pts.push_back(run[0]);
            fill.pts.push_back((float)edgeY);
            out.push_back(fill);
        }
        if (f.selected) {
            DrawCmd h;
            h.op = DrawOp::Polyline;
            h.color = selHalo;
            h.lineWidth = f.lineThickness / 3.0f + 8.0f * view.uiScale;
            h.pts = run;
            out.push_back(h);
        }
        if (strict) {
            DrawCmd seg;
            seg.op = DrawOp::MultiSeg;
            seg.color = base;
            seg.lineWidth = f.lineThickness / 3.0f;
            AppendDashSegs(run, 6.0 * view.uiScale, 5.0 * view.uiScale, seg.pts);
            if (seg.pts.size() >= 4) out.push_back(seg);
        } else {
            DrawCmd seg;
            seg.op = DrawOp::Polyline;
            seg.color = base;
            seg.lineWidth = f.lineThickness / 3.0f;
            seg.pts = run;
            out.push_back(seg);
        }
        run.clear();
    };
    for (int i = 0; i <= steps; ++i) {
        const double wx = x0 + (x1 - x0) * i / steps;
        const double wy = f.body.eval(wx);
        if (!std::isfinite(wy)) { flushRun(); continue; }
        double py = view.py(wy);
        if (py < -pad) py = -pad;
        if (py > view.height + pad) py = view.height + pad;
        run.push_back((float)view.px(wx));
        run.push_back((float)py);
    }
    flushRun();
}

// 网格：上游 euclidianView1 网格样式 —— 从主刻度延伸的方格网。
// 主网格线 0.9px（视图默认灰 #999999），若存在更细的副网格（主格距 ≥ 6 个副格）
// 再画一层 0.4px 浅灰 #CCCCCC。跟随 uiScale。
void Kernel::appendGrid(std::vector<DrawCmd> &out) const
{
    double unit = 1.0;
    while (unit * view.xscale < 40 * view.uiScale) unit *= 10;
    while (unit * view.xscale > 110 * view.uiScale) unit /= 10;

    // 副网格：unit/5（上游 默认 5 分副格），仅在屏上副格距 ≥ 6px 时画
    double sub = unit / 5.0;
    if (sub * view.xscale >= 6 * view.uiScale) {
        DrawCmd gx;
        gx.op = DrawOp::MultiSeg;
        gx.color = darkTheme ? "#33363B" : "#CCCCCC";
        gx.lineWidth = 0.4f * (float)view.uiScale;
        gx.pts.reserve(128);
        for (double wx = ceil(view.xmin() / sub) * sub; wx <= view.xmax(); wx += sub) {
            if (fabs(wx / sub - round(wx / sub)) < 0.01 && fabs(wx / unit - round(wx / unit)) < 0.01) {
                continue;   // 主线位置留给主网格
            }
            float px = (float)view.px(wx);
            gx.pts.push_back(px); gx.pts.push_back(0.0f);
            gx.pts.push_back(px); gx.pts.push_back((float)view.height);
        }
        for (double wy = ceil(view.ymin() / sub) * sub; wy <= view.ymax(); wy += sub) {
            if (fabs(wy / sub - round(wy / sub)) < 0.01 && fabs(wy / unit - round(wy / unit)) < 0.01) {
                continue;
            }
            float py = (float)view.py(wy);
            gx.pts.push_back(0.0f); gx.pts.push_back(py);
            gx.pts.push_back((float)view.width); gx.pts.push_back(py);
        }
        if (!gx.pts.empty()) out.push_back(gx);
    }

    // 主网格：整屏竖线 + 横线（灰，从刻度处贯穿画布）
    DrawCmd gm;
    gm.op = DrawOp::MultiSeg;
    gm.color = darkTheme ? "#4A4D52" : "#999999";
    gm.lineWidth = 0.9f * (float)view.uiScale;
    gm.pts.reserve(64);
    for (double wx = ceil(view.xmin() / unit) * unit; wx <= view.xmax(); wx += unit) {
        float px = (float)view.px(wx);
        gm.pts.push_back(px); gm.pts.push_back(0.0f);
        gm.pts.push_back(px); gm.pts.push_back((float)view.height);
    }
    for (double wy = ceil(view.ymin() / unit) * unit; wy <= view.ymax(); wy += unit) {
        float py = (float)view.py(wy);
        gm.pts.push_back(0.0f); gm.pts.push_back(py);
        gm.pts.push_back((float)view.width); gm.pts.push_back(py);
    }
    if (!gm.pts.empty()) out.push_back(gm);
}

// 轴刻度：每个刻度是独立短划（MultiSeg，成对点），绝不能连成折线
void Kernel::appendTickMarks(std::vector<DrawCmd> &out) const
{
    double unit = 1.0;
    while (unit * view.xscale < 40 * view.uiScale) unit *= 10;
    while (unit * view.xscale > 110 * view.uiScale) unit /= 10;
    double tickH = 4.0 * view.uiScale;

    DrawCmd tks;
    tks.op = DrawOp::MultiSeg;
    tks.color = darkTheme ? "#8A9099" : "#666666";
    tks.lineWidth = 1.0f;
    tks.pts.reserve(64);
    for (double wx = ceil(view.xmin() / unit) * unit; wx <= view.xmax(); wx += unit) {
        if (fabs(wx) < unit * 0.5) continue;
        float px = (float)view.px(wx);
        tks.pts.push_back(px); tks.pts.push_back((float)(view.yZero - tickH));
        tks.pts.push_back(px); tks.pts.push_back((float)(view.yZero + tickH));
    }
    for (double wy = ceil(view.ymin() / unit) * unit; wy <= view.ymax(); wy += unit) {
        if (fabs(wy) < unit * 0.5) continue;
        float py = (float)view.py(wy);
        tks.pts.push_back((float)(view.xZero - tickH)); tks.pts.push_back(py);
        tks.pts.push_back((float)(view.xZero + tickH)); tks.pts.push_back(py);
    }
    if (!tks.pts.empty()) out.push_back(tks);

    // 轴数字（跟随 uiScale 缩放偏移）
    char buf[32];
    double ox = 2 * view.uiScale, oy = 14 * view.uiScale;
    for (double wx = ceil(view.xmin() / unit) * unit; wx <= view.xmax(); wx += unit) {
        if (fabs(wx) < unit * 0.5) continue;
        snprintf(buf, sizeof(buf), "%g", wx);
        DrawCmd t;
        t.op = DrawOp::Text;
        t.color = darkTheme ? "#C9CDD4" : "#333333";
        t.pts = { (float)(view.px(wx) + ox), (float)(view.yZero + oy) };
        t.text = buf;
        out.push_back(t);
    }
    for (double wy = ceil(view.ymin() / unit) * unit; wy <= view.ymax(); wy += unit) {
        if (fabs(wy) < unit * 0.5) continue;
        snprintf(buf, sizeof(buf), "%g", wy);
        DrawCmd t;
        t.op = DrawOp::Text;
        t.color = darkTheme ? "#C9CDD4" : "#333333";
        t.pts = { (float)(view.xZero + 4 * view.uiScale), (float)(view.py(wy) - 2 * view.uiScale) };
        t.text = buf;
        out.push_back(t);
    }
}

// ---------- 四十八包：线裁剪（Cohen-Sutherland，像素空间） ----------
// 线类对象（线段/直线/射线/向量）的端点先在像素域裁剪到画布矩形再下发：
// ①端点恒落在界内，超陡/超远对象的坐标不再以 ±1e9px 量级转 float 交给画
// 布；②整段在视口外的对象直接省略；③射线起点远离屏幕时也能延伸到视口
//（裁剪反求交点，旧版固定延伸长度够不着）。区域码位序：左1 右2 下4 上8
//（画布 y 向下，"上"=y 小于顶边）。
static const int CS_LEFT = 1, CS_RIGHT = 2, CS_BOTTOM = 4, CS_TOP = 8;

static int CsOutCode(double x, double y, double xmin, double ymin,
                     double xmax, double ymax)
{
    int c = 0;
    if (x < xmin) c |= CS_LEFT;
    else if (x > xmax) c |= CS_RIGHT;
    if (y < ymin) c |= CS_TOP;
    else if (y > ymax) c |= CS_BOTTOM;
    return c;
}

// 线段 [x1,y1]-[x2,y2] 就地裁剪到矩形；完全在外（或端点含非有限值，inf/
// NaN 无法求交）返回 false。选中端点在界内时坐标原样返回（恒等裁剪）。
static bool ClipSegRect(double &x1, double &y1, double &x2, double &y2,
                        double xmin, double ymin, double xmax, double ymax)
{
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2)
        || !std::isfinite(y2))
        return false;
    int c1 = CsOutCode(x1, y1, xmin, ymin, xmax, ymax);
    int c2 = CsOutCode(x2, y2, xmin, ymin, xmax, ymax);
    for (int iter = 0; c1 | c2; ++iter) {
        if (iter > 64) return false;   // 理论 4 轮封顶，防御性保险
        if (c1 & c2) return false;     // 两端同侧在外：整段不可见
        const int c = c1 ? c1 : c2;
        double x, y;
        if (c & CS_TOP) {
            x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
            y = ymin;
        } else if (c & CS_BOTTOM) {
            x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
            y = ymax;
        } else if (c & CS_RIGHT) {
            y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
            x = xmax;
        } else {
            y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
            x = xmin;
        }
        if (c == c1) {
            x1 = x; y1 = y;
            c1 = CsOutCode(x1, y1, xmin, ymin, xmax, ymax);
        } else {
            x2 = x; y2 = y;
            c2 = CsOutCode(x2, y2, xmin, ymin, xmax, ymax);
        }
    }
    return true;
}

// 渲染裁剪矩形：画布外扩 5px 出锋余量（上游 CLIP_DISTANCE 同量级）
static void ClipRectOf(const EuclidianView &v, double &xmin, double &ymin,
                       double &xmax, double &ymax)
{
    xmin = -5.0;
    ymin = -5.0;
    xmax = v.width + 5.0;
    ymax = v.height + 5.0;
}

void Kernel::render(std::vector<DrawCmd> &out)
{
    out.clear();
    DrawCmd clear;
    clear.op = DrawOp::Clear;
    out.push_back(clear);

    // 画布尺寸未就绪时只清屏
    if (view.width <= 1) return;

    // 坐标轴/网格/刻度（N3：设置面板可开关；刻度与轴数字随坐标轴一起开关）
    if (showAxes) {
        DrawCmd ax;
        ax.op = DrawOp::Segment;
        ax.color = darkTheme ? "#C9CDD4" : "#000000";
        ax.lineWidth = 1.5f;
        ax.pts = { 0.0f, (float)view.yZero, (float)view.width, (float)view.yZero };
        out.push_back(ax);
        DrawCmd ay = ax;
        ay.pts = { (float)view.xZero, 0.0f, (float)view.xZero, (float)view.height };
        out.push_back(ay);
    }

    if (showGrid) {
        appendGrid(out);
    }

    if (showAxes) {
        appendTickMarks(out);
    }

    // 选中特效配色（代数区点行 / 长按菜单 → kernelSelect）
    const std::string selHalo = darkTheme ? "rgba(143,182,255,0.40)" : "rgba(0,125,255,0.30)";
    const std::string selRing = darkTheme ? "#8FB6FF" : "#007DFF";

    // 对象
    for (auto &g : geos_) {
        if (!g->visible) continue;
        switch (g->type()) {
        case GeoType::Point: {
            GeoPoint *p = static_cast<GeoPoint *>(g.get());
            double cx = view.px(p->x), cy = view.py(p->y);
            if (cx < -20 || cx > view.width + 20 || cy < -20 || cy > view.height + 20) break;
            if (p->selected) {
                // 选中：点外一圈亮色圆环
                DrawCmd ring;
                ring.op = DrawOp::CirclePix;
                ring.color = selRing;
                ring.lineWidth = 2.5f * view.uiScale;
                ring.pts = { (float)cx, (float)cy, (float)((p->pointSize + 6) * view.uiScale) };
                out.push_back(ring);
            }
            DrawCmd d;
            d.op = DrawOp::FillCircle;
            d.color = ObjectColor(p->color, darkTheme);
            d.pts = { (float)cx, (float)cy, (float)(p->pointSize * view.uiScale) };
            out.push_back(d);
            // 标签：点右上方（上游 DrawPoint: xLabel=px+4, yLabel=py-2*pointSize）
            double us = view.uiScale;
            DrawCmd lb;
            lb.op = DrawOp::Text;
            lb.color = ObjectColor(p->color, darkTheme);
            lb.pts = { (float)(cx + 4 * us), (float)(cy - 2 * p->pointSize * us - 2 * us) };
            lb.text = p->label;
            out.push_back(lb);
            break;
        }
        case GeoType::Segment: {
            GeoSegment *s = static_cast<GeoSegment *>(g.get());
            DrawCmd d;
            d.op = DrawOp::Segment;
            d.color = ObjectColor(s->color, darkTheme);
            d.lineWidth = s->lineThickness / 3.0f;
            // ex/ey 访问器：像线段（Segment(P, 长度)）p1/p2 为空，裸读
            // p1->x 空指针崩溃——二十四包⑦ 真机闪退根因
            double axp = view.px(s->ex1()), ayp = view.py(s->ey1());
            double bxp = view.px(s->ex2()), byp = view.py(s->ey2());
            // 四十八包：整段在视口外直接省略，部分在外裁到界内
            double cxmin, cymin, cxmax, cymax;
            ClipRectOf(view, cxmin, cymin, cxmax, cymax);
            if (!ClipSegRect(axp, ayp, bxp, byp, cxmin, cymin, cxmax, cymax)) break;
            d.pts = { (float)axp, (float)ayp, (float)bxp, (float)byp };
            if (s->selected) {
                DrawCmd h = d;
                h.color = selHalo;
                h.lineWidth = d.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(d);
            // 标签：中点法向外 16px（上游 DrawSegment）
            double us = view.uiScale;
            double mx = 0.5 * (view.px(s->ex1()) + view.px(s->ex2()));
            double my = 0.5 * (view.py(s->ey1()) + view.py(s->ey2()));
            double nx = view.py(s->ey1()) - view.py(s->ey2());
            double ny = view.px(s->ex2()) - view.px(s->ex1());
            double len = sqrt(nx * nx + ny * ny);
            DrawCmd lb;
            lb.op = DrawOp::Text;
            lb.color = ObjectColor(s->color, darkTheme);
            if (len > 1e-9) {
                lb.pts = { (float)(mx + nx * 16 * us / len), (float)(my + ny * 16 * us / len) };
            } else {
                lb.pts = { (float)mx, (float)(my + 16 * us) };
            }
            lb.text = s->label;
            out.push_back(lb);
            break;
        }
        case GeoType::Line: {
            GeoLine *l = static_cast<GeoLine *>(g.get());
            if (fabs(l->a) < 1e-15 && fabs(l->b) < 1e-15) break;
            // 端点先取在视界外扩 5px 的矩形边界上，再统一 Cohen-Sutherland
            // 裁剪（四十八包）：主轴参数化——陡线（|a|>|b|）按 y 求端点、
            // 平线按 x 求端点；旧版一律按 x 取端点，近竖直线（|b| 极小）端
            // 点 y 以 ±1e9px 量级转 float 交给画布，渲染退化
            double cxmin, cymin, cxmax, cymax;
            ClipRectOf(view, cxmin, cymin, cxmax, cymax);
            double p1x, p1y, p2x, p2y;
            if (fabs(l->b) < 1e-15) {
                // 竖直线 x = c/a
                double wx = l->c / l->a;
                p1x = view.px(wx); p1y = cymin;
                p2x = view.px(wx); p2y = cymax;
            } else if (fabs(l->a) < 1e-15) {
                // 水平线 y = c/b
                double wy = l->c / l->b;
                p1x = cxmin; p1y = view.py(wy);
                p2x = cxmax; p2y = view.py(wy);
            } else if (fabs(l->a) > fabs(l->b)) {
                // 陡线：按 y 界求 x（x = (c - b y) / a，良态）
                p1x = view.px((l->c - l->b * view.wy(cymin)) / l->a);
                p1y = cymin;
                p2x = view.px((l->c - l->b * view.wy(cymax)) / l->a);
                p2y = cymax;
            } else {
                // 平线：按 x 界求 y（y = (c - a x) / b，良态）
                p1x = cxmin; p1y = view.py((l->c - l->a * view.wx(cxmin)) / l->b);
                p2x = cxmax; p2y = view.py((l->c - l->a * view.wx(cxmax)) / l->b);
            }
            if (!ClipSegRect(p1x, p1y, p2x, p2y, cxmin, cymin, cxmax, cymax)) break;
            DrawCmd d;
            d.op = DrawOp::Segment;
            d.color = ObjectColor(l->color, darkTheme);
            d.lineWidth = l->lineThickness / 3.0f;
            d.pts = { (float)p1x, (float)p1y, (float)p2x, (float)p2y };
            if (l->selected) {
                DrawCmd h = d;
                h.color = selHalo;
                h.lineWidth = d.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(d);
            break;
        }
        case GeoType::Circle: {
            GeoCircle *c = static_cast<GeoCircle *>(g.get());
            Geometry geo = CircleGeom(*c, view);
            if (!(geo.r > 0)) break;   // 滑动条半径可能 ≤0，负半径会让画布 arc 抛异常
            if (geo.r > 10 * (view.width + view.height)) break;   // 上游 BIG_RADIUS 保护
            const double CLIP = 10.0;
            if (geo.x + geo.r < -CLIP || geo.x - geo.r > view.width + CLIP ||
                geo.y + geo.r < -CLIP || geo.y - geo.r > view.height + CLIP) break;
            DrawCmd d;
            d.op = DrawOp::CirclePix;
            d.color = ObjectColor(c->color, darkTheme);
            d.lineWidth = c->lineThickness / 3.0f;
            d.pts = { (float)geo.x, (float)geo.y, (float)geo.r };
            if (c->selected) {
                DrawCmd h = d;
                h.color = selHalo;
                h.lineWidth = d.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(d);
            // 标签（上游 DrawCircle: 左上方向）
            DrawCmd lb;
            lb.op = DrawOp::Text;
            lb.color = ObjectColor(c->color, darkTheme);
            lb.pts = { (float)(geo.x - geo.r * 0.5), (float)(geo.y - geo.r * 0.85 + 14 * view.uiScale) };
            lb.text = c->label;
            out.push_back(lb);
            break;
        }
        case GeoType::Ray: {
            GeoRay *r = static_cast<GeoRay *>(g.get());
            double x1 = view.px(r->p1->x), y1 = view.py(r->p1->y);
            double dx = view.px(r->p2->x) - x1, dy = view.py(r->p2->y) - y1;
            double len = sqrt(dx * dx + dy * dy);
            if (len < 1e-9) break;
            // 延伸长度覆盖「起点到画布远角」：|p1| + 2×对角线（三角不等式
            // 保证与视口相交；旧版固定 2×(w+h)，起点远离屏幕时射线够不着
            // 视口，画面上整条消失）。延伸后 Cohen-Sutherland 裁剪反求可见
            // 段，端点收敛到界内（四十八包）
            double cxmin, cymin, cxmax, cymax;
            ClipRectOf(view, cxmin, cymin, cxmax, cymax);
            double diag = sqrt((double)view.width * view.width
                               + (double)view.height * view.height);
            double L = sqrt(x1 * x1 + y1 * y1) + 2.0 * diag + 20.0;
            double x2 = x1 + dx / len * L, y2 = y1 + dy / len * L;
            if (!ClipSegRect(x1, y1, x2, y2, cxmin, cymin, cxmax, cymax)) break;
            DrawCmd d;
            d.op = DrawOp::Segment;
            d.color = ObjectColor(r->color, darkTheme);
            d.lineWidth = r->lineThickness / 3.0f;
            d.pts = { (float)x1, (float)y1, (float)x2, (float)y2 };
            if (r->selected) {
                DrawCmd h = d;
                h.color = selHalo;
                h.lineWidth = d.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(d);
            break;
        }
        case GeoType::Vector: {
            GeoVector *v = static_cast<GeoVector *>(g.get());
            double x1 = view.px(v->ex1()), y1 = view.py(v->ey1());
            double x2 = view.px(v->ex2()), y2 = view.py(v->ey2());
            double dx = x2 - x1, dy = y2 - y1;
            if (sqrt(dx * dx + dy * dy) < 1e-9) break;
            // 轴杆裁剪（四十八包）；箭头/标签仍按原尖端与中点定位，尖端在
            // 界外时由画布自裁
            double cx1 = x1, cy1 = y1, cx2 = x2, cy2 = y2;
            double cxmin, cymin, cxmax, cymax;
            ClipRectOf(view, cxmin, cymin, cxmax, cymax);
            if (!ClipSegRect(cx1, cy1, cx2, cy2, cxmin, cymin, cxmax, cymax)) break;
            DrawCmd d;
            d.op = DrawOp::Segment;
            d.color = ObjectColor(v->color, darkTheme);
            d.lineWidth = v->lineThickness / 3.0f;
            d.pts = { (float)cx1, (float)cy1, (float)cx2, (float)cy2 };
            if (v->selected) {
                DrawCmd h = d;
                h.color = selHalo;
                h.lineWidth = d.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(d);
            // 箭头：自尖端向后的两条短划，张角约 ±25°（上游 DrawVector）
            double hl = 12.0 * view.uiScale;
            double base = atan2(dy, dx);
            double a1 = base + 2.70, a2 = base - 2.70;
            DrawCmd h;
            h.op = DrawOp::MultiSeg;
            h.color = d.color;
            h.lineWidth = d.lineWidth;
            h.pts = { (float)x2, (float)y2,
                      (float)(x2 + hl * cos(a1)), (float)(y2 + hl * sin(a1)),
                      (float)x2, (float)y2,
                      (float)(x2 + hl * cos(a2)), (float)(y2 + hl * sin(a2)) };
            out.push_back(h);
            DrawCmd lb;
            lb.op = DrawOp::Text;
            lb.color = d.color;
            lb.pts = { (float)(0.5 * (x1 + x2) + 6 * view.uiScale),
                       (float)(0.5 * (y1 + y2) - 6 * view.uiScale) };
            lb.text = v->label;
            out.push_back(lb);
            break;
        }
        case GeoType::Polygon: {
            GeoPolygon *pg = static_cast<GeoPolygon *>(g.get());
            size_t n = pg->image ? pg->ivx.size() : pg->verts.size();
            if (n < 3) break;
            std::vector<float> poly;
            poly.reserve(n * 2);
            double gx = 0, gy = 0;
            for (size_t i = 0; i < n; ++i) {
                double wxv = pg->image ? pg->ivx[i] : pg->verts[i]->x;
                double wyv = pg->image ? pg->ivy[i] : pg->verts[i]->y;
                double ppx = view.px(wxv), ppy = view.py(wyv);
                poly.push_back((float)ppx);
                poly.push_back((float)ppy);
                gx += ppx;
                gy += ppy;
            }
            if (pg->fillAlpha > 0.003) {
                DrawCmd fill;
                fill.op = DrawOp::FillPoly;
                fill.color = RgbaFromHex(ObjectColor(pg->color, darkTheme), pg->fillAlpha);
                fill.pts = poly;
                out.push_back(fill);
            }
            DrawCmd st;
            st.op = DrawOp::Polyline;
            st.color = ObjectColor(GeoColor{0, 0, 0}, darkTheme);
            st.lineWidth = pg->lineThickness / 3.0f;
            st.pts = poly;
            st.pts.push_back(poly[0]);   // 闭合
            st.pts.push_back(poly[1]);
            if (pg->selected) {
                DrawCmd h = st;
                h.color = selHalo;
                h.lineWidth = st.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(st);
            DrawCmd lb;
            lb.op = DrawOp::Text;
            lb.color = ObjectColor(GeoColor{0, 0, 0}, darkTheme);
            lb.pts = { (float)(gx / n), (float)(gy / n) };
            lb.text = pg->label;
            out.push_back(lb);
            break;
        }
        case GeoType::Function: {
            GeoFunction *f = static_cast<GeoFunction *>(g.get());
            if (f->ineqOp || f->ineqCombo()) appendInequalityPlot(*f, out);
            else appendFunctionPlot(*f, out, f->selected);
            break;
        }
        case GeoType::Angle: {
            GeoAngle *ga = static_cast<GeoAngle *>(g.get());
            if (!ga->vb || !ga->va || !ga->vc) break;
            double bx = view.px(ga->vb->x), by = view.py(ga->vb->y);
            double w0 = atan2(ga->va->y - ga->vb->y, ga->va->x - ga->vb->x);
            double w1 = atan2(ga->vc->y - ga->vb->y, ga->vc->x - ga->vb->x);
            double ccw = w1 - w0;
            while (ccw < 0) ccw += 2 * M_PI;
            if (ccw < 1e-9) break;
            double R = 26.0 * view.uiScale;
            // 角扇形填充（上游 角默认带浅填充；fillAlpha=0 时不画）
            if (ga->fillAlpha > 0.003) {
                DrawCmd fill;
                fill.op = DrawOp::FillPoly;
                fill.color = RgbaFromHex(darkTheme ? "#7FD1A8" : "#1C7E54", ga->fillAlpha);
                const int nseg = std::min(64, std::max(8, (int)std::ceil(ccw * 24.0 / M_PI)));
                fill.pts.push_back((float)bx);
                fill.pts.push_back((float)by);
                for (int i = 0; i <= nseg; ++i) {
                    const double t = (-w0 - ccw) + ccw * i / nseg;
                    fill.pts.push_back((float)(bx + cos(t) * R));
                    fill.pts.push_back((float)(by + sin(t) * R));
                }
                out.push_back(fill);
            }
            DrawCmd d;
            d.op = DrawOp::Arc;
            d.color = darkTheme ? "#7FD1A8" : "#1C7E54";
            d.lineWidth = 1.6f;
            // 世界逆时针角 = canvas 角取负；弧集 [w0, w0+ccw] ↦ [-w0-ccw, -w0]
            d.pts = { (float)bx, (float)by, (float)R, (float)(-w0 - ccw), (float)(-w0) };
            if (ga->selected) {
                DrawCmd h = d;
                h.color = selHalo;
                h.lineWidth = d.lineWidth + 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(d);
            double wm = w0 + ccw / 2;
            DrawCmd lb;
            lb.op = DrawOp::Text;
            lb.color = d.color;
            lb.pts = { (float)(bx + cos(wm) * (R + 14 * view.uiScale)),
                       (float)(by - sin(wm) * (R + 14 * view.uiScale)) };
            lb.text = FormatNum(ga->value) + "°";
            out.push_back(lb);
            break;
        }
        case GeoType::Numeric: {
            // 滑动条改在代数区行内渲染（真机反馈），画布不再绘制；
            // Integral 填充在此绘制（半透明填充 + 边界竖线，上游 同视觉）
            GeoNumeric *n = static_cast<GeoNumeric *>(g.get());
            if (n->fill && n->fill->on && n->fill->fn && n->fill->fn->definable()) {
                const double a = std::min(n->fill->a, n->fill->b);
                const double b = std::max(n->fill->a, n->fill->b);
                GeoFunction *ffn = n->fill->fn;
                if (b - a > 1e-12) {
                    const std::string fc = darkTheme ? "rgba(120,170,255,0.30)"
                                                     : "rgba(30,110,255,0.22)";
                    const double x0 = view.px(a), x1 = view.px(b);
                    const int steps = std::max(8, (int)((x1 - x0) / 2.0));
                    std::vector<float> poly;
                    poly.push_back((float)x0);
                    poly.push_back((float)view.py(0.0));
                    for (int i = 0; i <= steps; ++i) {
                        const double wxv = a + (b - a) * i / steps;
                        poly.push_back((float)view.px(wxv));
                        poly.push_back((float)view.py(ffn->evalAt(wxv)));
                    }
                    poly.push_back((float)x1);
                    poly.push_back((float)view.py(0.0));
                    if (n->selected) {
                        DrawCmd h;
                        h.op = DrawOp::FillPoly;
                        h.color = selHalo;
                        h.pts = poly;
                        out.push_back(h);
                    }
                    DrawCmd fill;
                    fill.op = DrawOp::FillPoly;
                    fill.color = fc;
                    fill.pts = poly;
                    out.push_back(fill);
                    DrawCmd e1;
                    e1.op = DrawOp::MultiSeg;
                    e1.color = ObjectColor(ffn->color, darkTheme);
                    e1.lineWidth = 1.2f;
                    e1.pts = { (float)x0, (float)view.py(0.0), (float)x0, (float)view.py(ffn->evalAt(a)),
                               (float)x1, (float)view.py(0.0), (float)x1, (float)view.py(ffn->evalAt(b)) };
                    out.push_back(e1);
                }
            }
            break;
        }
        case GeoType::List: {
            // 普通数值列表不绘制（上游 同语义）；1.0.6.3 图表列表走
            // appendChartPlot（bar/hist/line/scatter/box/stem）
            GeoList *cl = static_cast<GeoList *>(g.get());
            if (!cl->chart.empty()) appendChartPlot(*cl, out, selHalo);
            break;
        }
        case GeoType::Text: {
            GeoText *t = static_cast<GeoText *>(g.get());
            DrawCmd d;
            d.op = DrawOp::Text;
            d.color = ObjectColor(t->color, darkTheme);
            d.pts = { (float)view.px(t->x), (float)view.py(t->y) };
            d.text = t->text;
            if (t->selected) {
                // 选中特效：文本底一圈圆角底色（简单画个更大描边文本）
                DrawCmd h = d;
                h.color = selHalo;
                h.text = t->text;
                h.lineWidth = 8.0f * view.uiScale;
                out.push_back(h);
            }
            out.push_back(d);
            break;
        }
        }
    }

    // 1.0.7.1 函数值表「显示点」叠加层（对象层之上，不可拾取）
    appendTablePoints(out);
}

} // namespace dreamember

// ---------- N4 .ggb XML 序列化 / 重放 ----------

namespace dreamember {
namespace {

// 公共样式头：<element> 开标签 + show/objColor/layer/labelMode。
// objAlpha：填充不透明度（上游 objColor@alpha；不等式/多边形/角写入并读回）
void AppendElementHead(std::string &o, const GeoElement &g, const char *type,
                       double objAlpha = 0.0)
{
    o += std::string("<element type=\"") + type + "\" label=\"" + XmlEsc(g.label) + "\">\n";
    o += std::string("<show object=\"") + (g.visible ? "true" : "false") + "\" label=\"true\"/>\n";
    o += "<objColor r=\"" + std::to_string(g.color.r) + "\" g=\"" + std::to_string(g.color.g)
       + "\" b=\"" + std::to_string(g.color.b) + "\" alpha=\"" + NumA(objAlpha) + "\"/>\n";
    o += "<layer val=\"0\"/>\n<labelMode val=\"0\"/>\n";
}

void AppendLineStyle(std::string &o, int thickness)
{
    o += "<lineStyle thickness=\"" + std::to_string(thickness)
       + "\" type=\"0\" typeHidden=\"0\"/>\n</element>\n";
}

// 通用命令写出：<command name=..><input a0=../><output a0=本对象/></command>。
// 多输出命令的第 n 个对象（cmdIndex>0）按 上游 第 n 交点形式追加序号参数
void AppendCommandXml(std::string &o, const GeoElement &g)
{
    if (g.cmdName.empty()) return;
    o += "<command name=\"" + XmlEsc(g.cmdName) + "\">\n<input";
    for (size_t i = 0; i < g.cmdArgs.size(); ++i) {
        o += " a" + std::to_string(i) + "=\"" + XmlEsc(g.cmdArgs[i]) + "\"";
    }
    if (g.cmdIndex > 0) {
        o += " a" + std::to_string(g.cmdArgs.size()) + "=\"" + std::to_string(g.cmdIndex + 1) + "\"";
    }
    o += "/>\n<output a0=\"" + XmlEsc(g.label) + "\"/>\n</command>\n";
}

} // namespace

// 序列化为 GeoGebra 兼容 XML（格式 5.0 子集，可被本内核 setXml 往返载入）
std::string Kernel::getXml() const
{
    std::string o;
    o.reserve(1024 + geos_.size() * 160);
    o += "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    o += "<geogebra format=\"5.0\" app=\"dreamEmberGeoCalc\" version=\"1.0.0\">\n";
    o += "<euclidianView>\n<viewNumber viewNo=\"1\"/>\n";
    o += "<size width=\"" + std::to_string(view.width) + "\" height=\""
       + std::to_string(view.height) + "\"/>\n";
    o += "<coordSystem xZero=\"" + NumA(view.xZero) + "\" yZero=\"" + NumA(view.yZero)
       + "\" scale=\"" + NumA(view.xscale) + "\" yscale=\"" + NumA(view.yscale) + "\"/>\n";
    o += "</euclidianView>\n";
    // 1.0.6.6 弧度会话标记回写（官方 <kernel><angleUnit> 同位；度制=无段，
    // 读侧缺段即 degree）。表达式层角引用出弧度值的比例随会话，缺了这段
    // 自产文件重开会退回度制语义
    if (angleRadian) {
        o += "<kernel>\n\t<angleUnit val=\"radian\"/>\n</kernel>\n";
    }
    o += "<construction title=\"\" author=\"\" date=\"\">\n";

    for (auto &gp : geos_) {
        GeoElement *g = gp.get();
        switch (g->type()) {
        case GeoType::Point: {
            GeoPoint *p = static_cast<GeoPoint *>(g);
            AppendCommandXml(o, *p);   // Midpoint/Intersect/变换像等依随点
            AppendElementHead(o, *p, "point");
            o += "<coords x=\"" + NumA(p->x) + "\" y=\"" + NumA(p->y) + "\" z=\"1\"/>\n";
            o += "<pointSize val=\"" + std::to_string(p->pointSize) + "\"/>\n</element>\n";
            break;
        }
        case GeoType::Segment: {
            GeoSegment *s = static_cast<GeoSegment *>(g);
            AppendCommandXml(o, *s);
            AppendElementHead(o, *s, "segment");
            AppendLineStyle(o, s->lineThickness);
            break;
        }
        case GeoType::Line: {
            GeoLine *l = static_cast<GeoLine *>(g);
            AppendCommandXml(o, *l);   // Line/PerpendicularLine（含过点平行）
            AppendElementHead(o, *l, "line");
            AppendLineStyle(o, l->lineThickness);
            break;
        }
        case GeoType::Ray: {
            GeoRay *r = static_cast<GeoRay *>(g);
            AppendCommandXml(o, *r);
            AppendElementHead(o, *r, "ray");
            AppendLineStyle(o, r->lineThickness);
            break;
        }
        case GeoType::Vector: {
            GeoVector *v = static_cast<GeoVector *>(g);
            AppendCommandXml(o, *v);
            AppendElementHead(o, *v, "vector");
            AppendLineStyle(o, v->lineThickness);
            break;
        }
        case GeoType::Circle: {
            GeoCircle *c = static_cast<GeoCircle *>(g);
            AppendCommandXml(o, *c);
            AppendElementHead(o, *c, "circle");
            AppendLineStyle(o, c->lineThickness);
            break;
        }
        case GeoType::Polygon: {
            GeoPolygon *pg = static_cast<GeoPolygon *>(g);
            AppendCommandXml(o, *pg);
            AppendElementHead(o, *pg, "polygon", pg->fillAlpha);
            AppendLineStyle(o, pg->lineThickness);
            break;
        }
        case GeoType::Angle: {
            GeoAngle *ga = static_cast<GeoAngle *>(g);
            AppendCommandXml(o, *ga);
            AppendElementHead(o, *ga, "angle", ga->fillAlpha);
            // 官方 XML 角值恒存弧度（1.0.6.5；读侧 xmlElement 恒 ×180/π）
            o += "<value val=\"" + NumA(ga->value * M_PI / 180.0) + "\"/>\n</element>\n";
            break;
        }
        case GeoType::Function: {
            GeoFunction *f = static_cast<GeoFunction *>(g);
            if (f->ineqOp || f->ineqCombo()) {
                // 不等式（含 1.0.7.1 链式、1.0.7.2 ∧/∨ 组合）：exp="f=y<x^2"
                // 经赋值分割守卫重放回不等式分支
                o += "<expression label=\"" + XmlEsc(f->label) + "\" exp=\""
                   + XmlEsc(f->label + "=" + f->src) + "\"/>\n";
            } else if (!f->cmdName.empty()) {
                // 命令产物（Derivative 等）：按命令重放重建（src 对输入解析不可回放）
                AppendCommandXml(o, *f);
            } else {
                o += "<expression label=\"" + XmlEsc(f->label) + "\" exp=\""
                   + XmlEsc(f->label + "(x)=" + f->src) + "\"/>\n";
            }
            AppendElementHead(o, *f, "function",
                (f->ineqOp || f->ineqCombo()) ? f->fillAlpha : 0.0);
            AppendLineStyle(o, f->lineThickness);
            break;
        }
        case GeoType::Numeric: {
            GeoNumeric *n = static_cast<GeoNumeric *>(g);
            if (n->isAngleVal && !n->calc && n->cmdName.empty() && !n->isSlider) {
                // 文件重建的自由角（1.0.6.6）：按官方 angle 元素写回（值恒
                // 弧度，1.0.6.5 约定）——若走表达式会写成度数原文，重开丢
                // isAngleVal 且与弧度标记矛盾
                AppendElementHead(o, *n, "angle", 0.1);
                o += "<value val=\"" + NumA(n->value * M_PI / 180.0)
                   + "\"/>\n</element>\n";
                break;
            }
            if (n->isSlider) {
                // 滑动条经表达式重放重建（a=Slider(min, max, step)）
                o += "<expression label=\"" + XmlEsc(n->label) + "\" exp=\""
                   + XmlEsc(n->label + "=Slider(" + NumA(n->smin) + ", " + NumA(n->smax)
                            + ", " + NumA(n->sstep) + ")") + "\"/>\n";
            } else if (!n->cmdName.empty()) {
                AppendCommandXml(o, *n);   // Distance 等
            } else if (n->cmdName.empty() && !n->cmd.empty()) {
                // 依随表达式数值（B1=A1+1）/常量式/布尔对象（1.0.6.4）按
                // 原表达式重放——布尔值列是 true/false 文本，按值重放不可解析
                o += "<expression label=\"" + XmlEsc(n->label) + "\" exp=\""
                   + XmlEsc(n->label + "=" + n->cmd) + "\"/>\n";
            } else if (n->calc && n->inputs.size() == 2) {
                // 兜底：历史依随数值按 Distance 重放
                o += "<command name=\"Distance\">\n<input a0=\"" + XmlEsc(n->inputs[0]->label);
                o += "\" a1=\"" + XmlEsc(n->inputs[1]->label);
                o += "\"/>\n<output a0=\"" + XmlEsc(n->label) + "\"/>\n</command>\n";
            } else {
                o += "<expression label=\"" + XmlEsc(n->label) + "\" exp=\""
                   + XmlEsc(n->label + "=" + n->valueText()) + "\"/>\n";
            }
            AppendElementHead(o, *n, "numeric");
            if (n->isSlider) {
                // 1.0.6.6 范围对象引用原文回写（官方 <slider min="b"> 同形；
                // 空时写数值）——官方 App 打开我们的文件保留动态范围引用
                o += "<slider min=\""
                   + (n->sminRef.empty() ? NumA(n->smin) : XmlEsc(n->sminRef))
                   + "\" max=\""
                   + (n->smaxRef.empty() ? NumA(n->smax) : XmlEsc(n->smaxRef))
                   + "\" step=\"" + NumA(n->sstep) + "\" x=\"" + NumA(n->sx)
                   + "\" y=\"" + NumA(n->sy) + "\"/>\n";
                // 官方增量在 <animation step>（1.0.6.5 写出——官方 App 打开
                // 我们的文件滑条步长不丢）；自产读侧两处都收
                o += std::string("<animation step=\"") + NumA(n->sstep)
                   + "\" speed=\"" + NumA(n->speed)
                   + "\" playing=\"" + (n->animating ? "true" : "false")
                   + "\" type=\"1\"/>\n";
            }
            o += "<value val=\"" + NumA(n->value) + "\"/>\n</element>\n";
            break;
        }
        case GeoType::List: {
            GeoList *L = static_cast<GeoList *>(g);
            if (!L->cmdName.empty()) {
                AppendCommandXml(o, *L);   // Sequence/Sort 等按命令重放
            } else {
                // 自由列表按花括号表达式重放
                o += "<expression label=\"" + XmlEsc(L->label) + "\" exp=\""
                   + XmlEsc(L->label + "=" + L->src) + "\"/>\n";
            }
            AppendElementHead(o, *L, "list");
            o += "</element>\n";
            break;
        }
        case GeoType::Text: {
            GeoText *t = static_cast<GeoText *>(g);
            AppendCommandXml(o, *t);   // Text("...", P) 按命令重放
            AppendElementHead(o, *t, "text");
            o += "</element>\n";
            break;
        }
        }
    }
    o += "</construction>\n</geogebra>\n";
    return o;
}

// <command name="X"><input a0=".."/><output a0=".."/></command> → 重放为输入行。
// headTag 是 <command ...> 开标签的属性文本（body 从开标签之后开始，取不到自身）
void Kernel::xmlCommand(const std::string &headTag, const std::string &body, int *skipped)
{
    const std::string name = AttrVal(headTag, "name");
    const size_t in = body.find("<input");
    const size_t outT = body.find("<output");
    if (name.empty() || in == std::string::npos || outT == std::string::npos) { ++*skipped; return; }
    const std::string inTag = body.substr(in, body.find('>', in) - in);
    const std::string outTag = body.substr(outT, body.find('>', outT) - outT);

    // input 的 a0..aN 属性按序号收集（上游 真实文件即此结构）
    std::vector<std::pair<int, std::string>> args;
    size_t i = 0;
    for (;;) {
        i = inTag.find('a', i);
        if (i == std::string::npos) break;
        size_t d = i + 1;
        int idx = 0;
        bool has = false;
        while (d < inTag.size() && isdigit((unsigned char)inTag[d])) {
            idx = idx * 10 + (inTag[d] - '0');
            ++d;
            has = true;
        }
        if (has && d + 1 < inTag.size() && inTag[d] == '=' && inTag[d + 1] == '"') {
            size_t q2 = inTag.find('"', d + 2);
            if (q2 != std::string::npos) {
                // 属性值与 AttrVal 一致需反转义（&quot;/&lt; 等，含引号的 Text 参数）
                args.push_back({ idx, XmlUnesc(inTag.substr(d + 2, q2 - d - 2)) });
                i = q2 + 1;
                continue;
            }
        }
        i = d > i ? d : i + 1;
    }
    std::sort(args.begin(), args.end(),
              [](const std::pair<int, std::string> &x, const std::pair<int, std::string> &y) {
                  return x.first < y.first;
              });
    std::string argList;
    for (size_t k = 0; k < args.size(); ++k) {
        if (k > 0) argList += ", ";
        argList += args[k].second;
    }
    const std::string outLabel = AttrVal(outTag, "a0");
    const std::string expr = name + "(" + argList + ")";
    if (!input(outLabel.empty() ? expr : outLabel + "=" + expr)) {
        ++*skipped;   // 命令未支持或参数缺失（其依赖已被跳过）
    }
}

// 样式应用到既有对象（颜色/线宽/点径/显隐）
void Kernel::xmlApplyStyle(GeoElement *e, const std::string &inner)
{
    size_t oc = inner.find("<objColor");
    if (oc != std::string::npos) {
        const size_t end = inner.find('>', oc);
        const std::string t = inner.substr(oc, end == std::string::npos ? end : end - oc);
        e->color.r = (int)AttrNum(t, "r", (double)e->color.r);
        e->color.g = (int)AttrNum(t, "g", (double)e->color.g);
        e->color.b = (int)AttrNum(t, "b", (double)e->color.b);
        // 区域填充透明度：不等式 / 多边形 / 角（新版文件才有意义；旧版 alpha=0
        // 占位回默认，避免 0 当真把区域藏没）
        GeoFunction *fn = dynamic_cast<GeoFunction *>(e);
        if (fn && (fn->ineqOp || fn->ineqCombo())) {
            double a = AttrNum(t, "alpha", fn->fillAlpha);
            fn->fillAlpha = a > 0.0 ? (a > 1.0 ? 1.0 : a) : 0.10;
        }
        GeoPolygon *pg = dynamic_cast<GeoPolygon *>(e);
        if (pg) {
            double a = AttrNum(t, "alpha", pg->fillAlpha);
            pg->fillAlpha = a > 0.0 ? (a > 1.0 ? 1.0 : a) : 0.15;
        }
        GeoAngle *ga = dynamic_cast<GeoAngle *>(e);
        if (ga) {
            double a = AttrNum(t, "alpha", ga->fillAlpha);
            ga->fillAlpha = a > 0.0 ? (a > 1.0 ? 1.0 : a) : 0.12;
        }
    }
    size_t ls = inner.find("<lineStyle");
    if (ls != std::string::npos) {
        const size_t end = inner.find('>', ls);
        const std::string t = inner.substr(ls, end == std::string::npos ? end : end - ls);
        e->lineThickness = (int)AttrNum(t, "thickness", (double)e->lineThickness);
    }
    size_t sh = inner.find("<show");
    if (sh != std::string::npos) {
        const size_t end = inner.find('>', sh);
        const std::string t = inner.substr(sh, end == std::string::npos ? end : end - sh);
        const std::string obj = AttrVal(t, "object");
        if (obj == "false") e->visible = false;
        else if (obj == "true") e->visible = true;
    }
    GeoPoint *p = dynamic_cast<GeoPoint *>(e);
    if (p) {
        size_t ps = inner.find("<pointSize");
        if (ps != std::string::npos) {
            const size_t end = inner.find('>', ps);
            const std::string t = inner.substr(ps, end == std::string::npos ? end : end - ps);
            p->pointSize = (int)AttrNum(t, "val", (double)p->pointSize);
        }
        // 四十五包：路径依附点的 <coords> 重投影恢复路径参数（命令重放
        // Point(g) 只建出默认参数的点，真实落位由保存的坐标恢复；自由点
        // 不受影响——其坐标在创建分支已用）
        if (p->path) {
            size_t c = inner.find("<coords");
            if (c != std::string::npos) {
                const size_t end = inner.find('>', c);
                const std::string t = inner.substr(c, end - c);
                const double cx = AttrNum(t, "x", p->x);
                const double cy = AttrNum(t, "y", p->y);
                double param = 0, nx = 0, ny = 0;
                if (PathProject(p->path, cx, cy, p->pathParam, param, nx, ny)) {
                    p->pathParam = param;
                    p->update();
                }
            }
        }
    }
    GeoNumeric *num = dynamic_cast<GeoNumeric *>(e);
    if (num) {
        // <value> 先落位（官方文件里 value 在 slider/animation 之前），滑条
        // 范围/步长吸附要用它。两类跳过：布尔对象（<value val="true|false">
        // 经 strtod 得 0 会覆盖坏）；角（XML 角值恒存弧度——命令重建的角由
        // 几何重算，落这里会把弧度数当度数盖到重算值上；自由角的回填见
        // xmlElement 尾部）
        size_t vv = inner.find("<value");
        if (vv != std::string::npos && !dynamic_cast<GeoAngle *>(e)) {
            const size_t end = inner.find('>', vv);
            const std::string t = inner.substr(vv, end == std::string::npos ? end : end - vv);
            const std::string raw = AttrVal(t, "val");
            if (raw != "true" && raw != "false") {
                num->value = AttrNum(t, "val", num->value);   // 数值无单位
            }
        }
        size_t sl = inner.find("<slider");
        if (sl != std::string::npos) {
            const size_t end = inner.find('>', sl);
            const std::string t = inner.substr(sl, end == std::string::npos ? end : end - sl);
            num->isSlider = true;
            // 上游 min/max 属性存数值或对象 label 引用（evaluateToNumeric
            // 二次解析）。1.0.6.6：引用（非纯数）存原文，构造重放完成后
            // setXml 尾部统一解析（引用对象可定义在滑条之后）
            const std::string sminS = AttrVal(t, "min");
            const std::string smaxS = AttrVal(t, "max");
            auto sliderBound = [](const std::string &s, double &out,
                                  std::string &ref) {
                if (s.empty()) return;
                char *ep = nullptr;
                const double v = strtod(s.c_str(), &ep);
                if (!isspace((unsigned char)s[0]) && ep && *ep == '\0') out = v;
                else ref = s;
            };
            sliderBound(sminS, num->smin, num->sminRef);
            sliderBound(smaxS, num->smax, num->smaxRef);
            num->sstep = AttrNum(t, "step", num->sstep);
            num->sx = AttrNum(t, "x", num->sx);
            num->sy = AttrNum(t, "y", num->sy);
            if (!(num->smax > num->smin)) num->smax = num->smin + 5.0;
        }
        // 动画（上游 <animation step speed playing/>）：playing 即刻开跑。
        // 1.0.6.5 补读 step：官方增量存 <animation step>（自产在 <slider
        // step>）；只落参数不吸值（见下方 isSlider 块）
        size_t an = inner.find("<animation");
        if (an != std::string::npos && num->isSlider) {
            const size_t end = inner.find('>', an);
            const std::string t = inner.substr(an, end == std::string::npos ? end : end - an);
            num->sstep = AttrNum(t, "step", num->sstep);
            num->animating = AttrVal(t, "playing") == "true";
            num->speed = AttrNum(t, "speed", num->speed);
            if (num->speed <= 0) num->speed = 1.0;
            num->animDir = 1;
        }
        if (num->isSlider) {
            // 只做范围钳位；步长不吸附（官方行为：文件值原样保留，增量只在
            // 拖动/动画/键盘等交互路径生效——真机实测 2.3@步长0.5 吸成 2.5）。
            // 范围含对象引用（min="b"）时不在样式期钳位——smin 尚是表达式
            // 重放的占位值，终值要等 setXml 尾部引用解析后再钳，否则文件值
            // 被旧范围误钳（1.0.7.1 探针 C7 抓出）
            if (num->sminRef.empty() && num->smaxRef.empty()) {
                num->value = std::max(num->smin, std::min(num->smax, num->value));
            }
        }
    }
}

// <element> 重放：已有对象 → 只应用样式；自由点/数值/文本/布尔/角 → 创建；
// 其余（依赖对象的命令未被支持）计入跳过。
// 官方 .ggb 的滑动条是 <element type="numeric"><value/><slider min max .../>
// <animation .../>：数值按自由数值重建后由 xmlApplyStyle 落滑条参数与动画态。
// 单位约定（1.0.6.5 对齐官方）：XML 数值无单位，角值恒存弧度（与
// <kernel> angleUnit 无关）——角重建一律 ×180/π，数值不再按单位换算
void Kernel::xmlElement(const std::string &type, const std::string &label,
                        const std::string &inner, int *skipped)
{
    GeoElement *existing = lookup(label);
    if (existing) {
        xmlApplyStyle(existing, inner);
        return;
    }
    // 自由角的换算度值（对象落为隐藏数值，样式应用后回填——见尾部）
    double angleDeg = NAN;
    if (type == "point" && !label.empty()) {
        const size_t c = inner.find("<coords");
        if (c == std::string::npos) { ++*skipped; return; }
        const size_t end = inner.find('>', c);
        const std::string t = inner.substr(c, end - c);
        const double x = AttrNum(t, "x", 0.0);
        const double y = AttrNum(t, "y", 0.0);
        if (!input(label + "=(" + NumA(x) + "," + NumA(y) + ")")) { ++*skipped; return; }
    } else if (type == "numeric" && !label.empty()) {
        const size_t v = inner.find("<value");
        if (v == std::string::npos) { ++*skipped; return; }
        const size_t end = inner.find('>', v);
        const std::string t = inner.substr(v, end - v);
        const double val = AttrNum(t, "val", 0.0);   // 数值无单位，不换算
        if (!input(label + "=" + NumA(val))) { ++*skipped; return; }
    } else if (type == "angle" && !label.empty()) {
        // 角（上游 <element type="angle"><value val="弧度"/>，官方恒存弧度）：
        // 一律 ×180/π 转内部度制后建隐藏数值型角（无三点依附，仅代数区可见）
        const size_t v = inner.find("<value");
        if (v == std::string::npos) { ++*skipped; return; }
        const size_t end = inner.find('>', v);
        const std::string t = inner.substr(v, end - v);
        const double val = AttrNum(t, "val", 0.0) * 180.0 / M_PI;
        angleDeg = val;
        if (!input(label + "=" + NumA(val))) { ++*skipped; return; }
    } else if (type == "boolean" && !label.empty()) {
        // 布尔（上游 <value val="true|false"/>）：按布尔字面量落位（1.0.6.5
        // 内核布尔对象——值列 true/false、可参与 ∧∨¬；旧版按 1/0 数值落位）
        const size_t v = inner.find("<value");
        if (v == std::string::npos) { ++*skipped; return; }
        const size_t end = inner.find('>', v);
        const std::string t = inner.substr(v, end - v);
        const std::string s = AttrVal(t, "val");
        if (s != "true" && s != "false") { ++*skipped; return; }
        if (!input(label + "=" + s)) { ++*skipped; return; }
    } else if (type == "text" && !label.empty()) {
        // 自由文本（上游 <element type="text"><caption val=".."/> 或表达式
        // 字符串）→ Text("...") 命令重建，画布可显示
        std::string content;
        const size_t cp = inner.find("<caption");
        if (cp != std::string::npos) {
            const size_t ce = inner.find('>', cp);
            content = AttrVal(inner.substr(cp, ce == std::string::npos
                ? std::string::npos : ce - cp), "val");
        }
        if (content.empty()) {
            // 官方正文在 <value val="&quot;..&quot;">（字符串源式带引号壳，
            // AttrVal 已反转义）；caption 是自产兼容源
            const size_t vv = inner.find("<value");
            if (vv != std::string::npos) {
                const size_t ve = inner.find('>', vv);
                std::string v = AttrVal(inner.substr(vv, ve == std::string::npos
                    ? std::string::npos : ve - vv), "val");
                if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
                    v = v.substr(1, v.size() - 2);
                }
                content = v;
            }
        }
        if (content.empty()) {
            // 无 caption/value：依赖文本（startPoint exp 指向定义）暂不支持重建
            ++*skipped;
            return;
        }
        // 内容含引号时先翻转义（XML 已反转义，这里只防 Text 语法撞车）
        std::string lit;
        for (char ch : content) {
            if (ch == '"') lit += "'";
            else lit += ch;
        }
        if (!input(label + "=Text(\"" + lit + "\")")) { ++*skipped; return; }
        // 上游 <startPoint x= y=>（世界坐标落点）：Text 命令默认 (1,1)，
        // 重建后挪到保存位置
        const size_t sp = inner.find("<startPoint");
        if (sp != std::string::npos) {
            const size_t spe = inner.find('>', sp);
            const std::string t = inner.substr(sp, spe == std::string::npos
                ? std::string::npos : spe - sp);
            GeoText *tx = dynamic_cast<GeoText *>(lookup(label));
            if (tx && AttrVal(t, "exp").empty()) {
                tx->x = AttrNum(t, "x", tx->x);
                tx->y = AttrNum(t, "y", tx->y);
            }
        }
    } else {
        ++*skipped;
        return;
    }
    GeoElement *made = lookup(label);
    if (made) xmlApplyStyle(made, inner);
    if (!std::isnan(angleDeg) && made) {
        // 自由角落为普通数值对象：其 <value> 是弧度原文，xmlApplyStyle 只对
        // 真角对象跳过 value 落位，这里回填换算后的度值；isAngleVal 补回
        // 角语义（1.0.6.6 弧度会话表达式层缩放判定用）
        GeoNumeric *an = static_cast<GeoNumeric *>(made);
        an->value = angleDeg;
        an->isAngleVal = true;
    }
}

// 从 XML 重建（先清空；euclidianView 恢复坐标系，construction 按序重放）。
// 重放期间的 input/clearAll 不打撤销点（本函数即撤销/重做/载入的执行体）
bool Kernel::setXml(const std::string &xml, int *skippedOut)
{
    int skipped = 0;
    ++undoSuspend_;
    clearAll();

    // euclidianView：coordSystem（画布尺寸取本机当前值，仅恢复原点与比例）
    const size_t ev = xml.find("<euclidianView");
    if (ev != std::string::npos) {
        const size_t cs = xml.find("<coordSystem", ev);
        if (cs != std::string::npos) {
            const size_t csEnd = xml.find('>', cs);
            if (csEnd != std::string::npos) {
                const std::string tag = xml.substr(cs, csEnd - cs);
                view.xZero = AttrNum(tag, "xZero", view.xZero);
                view.yZero = AttrNum(tag, "yZero", view.yZero);
                view.xscale = AttrNum(tag, "scale", view.xscale);
                view.yscale = AttrNum(tag, "yscale", view.yscale);
            }
        }
    }

    // kernel 段：角度单位（degree/radian）。1.0.6.5 起数值不换算（XML 数值
    // 无单位）、角恒按弧度→度换算（官方角值与该标记无关）；标记仍读入备用。
    // 无该段 = degree
    angleRadian = false;
    {
        const size_t ks = xml.find("<kernel>");
        if (ks != std::string::npos) {
            const size_t ke = xml.find("</kernel>", ks);
            const std::string seg = xml.substr(ks, ke == std::string::npos
                ? std::string::npos : ke - ks);
            const size_t au = seg.find("<angleUnit");
            if (au != std::string::npos) {
                const size_t auEnd = seg.find('>', au);
                const std::string tag = seg.substr(au, auEnd == std::string::npos
                    ? std::string::npos : auEnd - au);
                angleRadian = AttrVal(tag, "val") == "radian";
            }
        }
    }

    const size_t cStart = xml.find("<construction");
    if (cStart == std::string::npos) {
        lastError = "XML 缺少 construction 段";
        --undoSuspend_;
        return false;
    }
    size_t cEnd = xml.find("</construction>");
    if (cEnd == std::string::npos) cEnd = xml.size();

    // 顺序扫描：command / expression / element 三类节点，其余跳过
    size_t pos = cStart;
    while (pos < cEnd) {
        const size_t lt = xml.find('<', pos);
        if (lt == std::string::npos || lt >= cEnd) break;
        const size_t nameStart = lt + 1;
        if (nameStart >= xml.size()) break;
        if (xml[nameStart] == '/' || xml[nameStart] == '?' || xml[nameStart] == '!') {
            const size_t gt = xml.find('>', nameStart);
            if (gt == std::string::npos) break;
            pos = gt + 1;
            continue;
        }
        size_t nameEnd = nameStart;
        while (nameEnd < xml.size() &&
               (isalnum((unsigned char)xml[nameEnd]) || xml[nameEnd] == '_')) ++nameEnd;
        const std::string tag = xml.substr(nameStart, nameEnd - nameStart);
        // 该标签的 '>'（引号感知，防 exp 里的 '>' 截断）
        size_t gt = nameEnd;
        bool inQ = false;
        while (gt < xml.size()) {
            const char c = xml[gt];
            if (c == '"') inQ = !inQ;
            else if (c == '>' && !inQ) break;
            ++gt;
        }
        if (gt >= xml.size()) break;
        const std::string tagText = xml.substr(lt + 1, gt - lt - 1);
        const bool selfClose = gt > 0 && xml[gt - 1] == '/';
        pos = gt + 1;

        if (tag == "command" && !selfClose) {
            size_t endC = xml.find("</command>", pos);
            if (endC == std::string::npos || endC > cEnd) endC = cEnd;
            xmlCommand(tagText, xml.substr(pos, endC - pos), &skipped);
            pos = endC + 10;
        } else if (tag == "expression" && selfClose) {
            const std::string exp = AttrVal(tagText, "exp");
            const std::string label = AttrVal(tagText, "label");
            if (!exp.empty()) {
                // 1.0.6.5：无 '=' 的自由对象（官方自由列表/文本串）优先带
                // label 落名——裸 input 会落成匿名对象，后续 <element> 对不上号
                const bool textLit = exp.size() >= 2
                    && exp.front() == '"' && exp.back() == '"';
                std::string src = exp;
                if (!label.empty() && exp.find('=') == std::string::npos) {
                    if (textLit) {
                        // 字符串字面量：表达式层无字符串，经 Text() 命令重建
                        std::string lit;
                        for (char ch : exp.substr(1, exp.size() - 2)) {
                            lit += ch == '"' ? '\'' : ch;
                        }
                        src = label + "=Text(\"" + lit + "\")";
                    } else {
                        src = label + "=" + exp;
                    }
                }
                if (!input(src) && !label.empty() && exp.find('=') == std::string::npos
                    && !textLit) {
                    input(exp);   // 带 label 仍失败才退回匿名
                }
            }
        } else if (tag == "element") {
            size_t endE = xml.find("</element>", pos);
            std::string inner;
            if (endE == std::string::npos || endE > cEnd) {
                inner = xml.substr(pos, cEnd - pos);
                pos = cEnd;
            } else {
                inner = xml.substr(pos, endE - pos);
                pos = endE + 10;
            }
            xmlElement(AttrVal(tagText, "type"), AttrVal(tagText, "label"), inner, &skipped);
        }
        // 其它标签：pos 已推进过该标签
    }

    // 1.0.6.6 滑条范围对象引用：构造重放完成后统一解析（引用对象可能定义
    // 在滑条之后）。解析为数值对象当前值（静态快照；拖动引用对象不联动，
    // 官方为动态求值——本版简化）；非数值类/未找到保留默认范围。解析后
    // 重做上界守卫与值钳位
    for (const auto &gp : geos_) {
        GeoNumeric *num = dynamic_cast<GeoNumeric *>(gp.get());
        if (!num || (num->sminRef.empty() && num->smaxRef.empty())) continue;
        auto resolveBound = [&](const std::string &ref, double def, double &out) {
            if (ref.empty()) return;
            GeoNumeric *r = dynamic_cast<GeoNumeric *>(lookup(ref));
            out = r ? r->value : def;
        };
        resolveBound(num->sminRef, num->smin, num->smin);
        resolveBound(num->smaxRef, num->smax, num->smax);
        if (!(num->smax > num->smin)) num->smax = num->smin + 5.0;
        num->value = std::max(num->smin, std::min(num->smax, num->value));
    }

    if (skippedOut) *skippedOut = skipped;
    lastError = skipped > 0 ? ("已跳过 " + std::to_string(skipped) + " 个暂不支持的对象") : "";
    --undoSuspend_;
    return true;
}

// ---------- 撤销/重做（轮 B：XML 快照栈） ----------

// 变更成功后压入"变更前"快照；容量 50 超出丢最旧；新步进清空重做栈
void Kernel::commitUndoSnapshot(const std::string &pre)
{
    if (undoSuspend_ > 0) return;
    undoStack_.push_back(pre);
    if (undoStack_.size() > 50) undoStack_.erase(undoStack_.begin());
    redoStack_.clear();
}

// 宿主在连续手势（拖点/滑杆拖动）首次变更前打点
void Kernel::pushUndo()
{
    commitUndoSnapshot(getXml());
}

bool Kernel::undoStep()
{
    if (undoStack_.empty()) return false;
    const std::string snap = undoStack_.back();
    undoStack_.pop_back();
    redoStack_.push_back(getXml());
    if (redoStack_.size() > 50) redoStack_.erase(redoStack_.begin());
    int skipped = 0;
    setXml(snap, &skipped);   // 自产 XML，必然可回载
    return true;
}

bool Kernel::redoStep()
{
    if (redoStack_.empty()) return false;
    const std::string snap = redoStack_.back();
    redoStack_.pop_back();
    undoStack_.push_back(getXml());
    if (undoStack_.size() > 50) undoStack_.erase(undoStack_.begin());
    int skipped = 0;
    setXml(snap, &skipped);
    return true;
}

// 打开 .ggb 后清空历史（上游 语义：打开文件即新撤销链）
void Kernel::clearUndoHistory()
{
    undoStack_.clear();
    redoStack_.clear();
}

} // namespace dreamember
