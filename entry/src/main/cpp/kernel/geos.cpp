// geos.cpp — GeoElement 实现
#include "kernel/geos.h"
#include <cmath>
#include <cstdio>

namespace dreamember {

std::string ColorHex(const GeoColor &c)
{
    char b[8];
    snprintf(b, sizeof(b), "#%02X%02X%02X", c.r & 0xFF, c.g & 0xFF, c.b & 0xFF);
    return std::string(b);
}

std::string FormatNum(double v)
{
    if (!std::isfinite(v)) return "?";
    if (v == 0.0) return "0";   // 归一 −0（度制会话浮点尾差出负零，官方显示 0）
    if (v == floor(v) && fabs(v) < 1e15) {
        char b[40];
        snprintf(b, sizeof(b), "%.0f", v);
        return std::string(b);
    }
    char b[48];
    snprintf(b, sizeof(b), "%.6g", v);
    return std::string(b);
}

std::string LineEqText(double a, double b, double c)
{
    if (fabs(b) > 1e-15) {
        std::string s = "y = " + FormatNum(c / b);
        if (fabs(a) > 1e-15) {
            // y = c/b − (a/b)x：斜率为负时写 " + m·x"（七十三包修双负号），
            // ±1 系数省略数值（GGB 同款）
            const double k = a / b;
            const bool neg = k < 0;
            const double m = neg ? -k : k;
            s += neg ? " + " : " - ";
            s += fabs(m - 1.0) < 1e-12 ? std::string() : FormatNum(m);
            s += "x";
        }
        return s;
    }
    if (fabs(a) > 1e-15) return "x = " + FormatNum(c / a);
    return "未定义";
}

// cmdName 非空时返回 "Cmd(a, b)"
std::string GeoElement::cmdDefText() const
{
    if (cmdName.empty()) return "";
    std::string s = cmdName + "(";
    for (size_t i = 0; i < cmdArgs.size(); ++i) {
        if (i > 0) s += ", ";
        s += cmdArgs[i];
    }
    return s + ")";
}

// ---- GeoPoint ----

void GeoPoint::update()
{
    if (calc) {
        calc(this);
        return;
    }
    // 旧语义兜底：inputs 恰为两个点时按 Midpoint 取中点（1.0.8.1：任一端
    // 为三维点时出三维中点，z 同取均值）
    if (inputs.size() == 2) {
        GeoPoint *p1 = dynamic_cast<GeoPoint *>(inputs[0]);
        GeoPoint *p2 = dynamic_cast<GeoPoint *>(inputs[1]);
        if (p1 && p2) {
            x = 0.5 * (p1->x + p2->x);
            y = 0.5 * (p1->y + p2->y);
            if (p1->is3D || p2->is3D) {
                z = 0.5 * (p1->z + p2->z);
                is3D = true;
            }
        }
    }
}

std::string GeoPoint::valueText() const
{
    if (is3D) return "(" + FormatNum(x) + ", " + FormatNum(y) + ", " + FormatNum(z) + ")";
    return "(" + FormatNum(x) + ", " + FormatNum(y) + ")";
}

std::string GeoPoint::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (inputs.size() == 2) {
        return "Midpoint(" + inputs[0]->label + ", " + inputs[1]->label + ")";
    }
    if (is3D) return "(" + FormatNum(x) + ", " + FormatNum(y) + ", " + FormatNum(z) + ")";
    return "(" + FormatNum(x) + ", " + FormatNum(y) + ")";
}

// ---- GeoSegment ----

std::string GeoSegment::valueText() const
{
    // 1.0.8.10 含 z：三维线段代数区显示真长（2D 段 dz=0 不变）
    double dx = ex2() - ex1(), dy = ey2() - ey1(), dz = ez2() - ez1();
    return FormatNum(sqrt(dx * dx + dy * dy + dz * dz));
}

std::string GeoSegment::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (p1 && p2) return std::string("Segment(") + p1->label + ", " + p2->label + ")";
    return "Segment";
}

// ---- GeoLine ----

// 从 inputs 推导 ax+by=c。base 可为直线、线段或一次函数图像（上游 允许
// PerpendicularLine(P, f)，f 为 y=kx+m 型函数；上游 里一次函数即直线）。
static bool LineDirFromBase(const GeoElement *base, double &a, double &b)
{
    if (!base) return false;
    if (base->type() == GeoType::Line) {
        const GeoLine *l = static_cast<const GeoLine *>(base);
        a = l->a;
        b = l->b;
        return fabs(a) > 1e-15 || fabs(b) > 1e-15;
    }
    if (base->type() == GeoType::Segment) {
        const GeoSegment *s = static_cast<const GeoSegment *>(base);
        // ex/ey 访问器：像线段（Segment(P, 长度)）p1/p2 为空，裸读崩溃
        a = s->ey1() - s->ey2();
        b = s->ex2() - s->ex1();
        return fabs(a) > 1e-15 || fabs(b) > 1e-15;
    }
    if (base->type() == GeoType::Function) {
        const GeoFunction *f = static_cast<const GeoFunction *>(base);
        double k = 0, m = 0;
        if (f->body.linearCoef(k, m)) {
            // y = k*x + m → -k*x + y = m（a=-k, b=1；k=0 即水平线 y=m 仍成立）
            a = -k;
            b = 1.0;
            return true;
        }
    }
    return false;
}

void GeoLine::update()
{
    if (calc) {
        calc(this);
        return;
    }
    switch (mode) {
    case Mode::TwoPoints:
        if (p1 && p2) {
            a = p1->y - p2->y;
            b = p2->x - p1->x;
            c = a * p1->x + b * p1->y;
            verticalConst = false;
        }
        break;
    case Mode::PerpThrough: {
        // 基准 ax+by=c → 垂直线 b*x - a*y = b*px - a*py
        double ba = 0, bb = 0;
        if (pt && LineDirFromBase(inputs.size() >= 2 ? inputs[1] : base, ba, bb)) {
            a = bb;
            b = -ba;
            c = a * pt->x + b * pt->y;
            verticalConst = false;
        }
        break;
    }
    case Mode::ParallelThrough: {
        // 基准 ax+by=c → 平行线 a*x + b*y = a*px + b*py
        double ba = 0, bb = 0;
        if (pt && LineDirFromBase(inputs.size() >= 2 ? inputs[1] : base, ba, bb)) {
            a = ba;
            b = bb;
            c = a * pt->x + b * pt->y;
            verticalConst = false;
        }
        break;
    }
    case Mode::PerpBisector: {
        // |X-A| = |X-B| → 2(B-A)·X = |B|² - |A|²
        if (p1 && p2) {
            a = p2->x - p1->x;
            b = p2->y - p1->y;
            c = 0.5 * (p2->x * p2->x + p2->y * p2->y - p1->x * p1->x - p1->y * p1->y);
            verticalConst = false;
        }
        break;
    }
    case Mode::AngleBisector: {
        // 内角平分线：单位向量(BA) + 单位向量(BC) 的方向，过顶点
        if (inputs.size() >= 3 && pt) {
            GeoPoint *A = dynamic_cast<GeoPoint *>(inputs[0]);
            GeoPoint *C = dynamic_cast<GeoPoint *>(inputs[2]);
            if (A && C) {
                double d1x = A->x - pt->x, d1y = A->y - pt->y;
                double d2x = C->x - pt->x, d2y = C->y - pt->y;
                double l1 = sqrt(d1x * d1x + d1y * d1y);
                double l2 = sqrt(d2x * d2x + d2y * d2y);
                if (l1 > 1e-15 && l2 > 1e-15) {
                    double ux = d1x / l1 + d2x / l2;
                    double uy = d1y / l1 + d2y / l2;
                    if (fabs(ux) < 1e-12 && fabs(uy) < 1e-12) {
                        ux = -d1y / l1;   // 平角：取垂直方向
                        uy = d1x / l1;
                    }
                    a = uy;
                    b = -ux;
                    c = a * pt->x + b * pt->y;
                    verticalConst = false;
                }
            }
        }
        break;
    }
    case Mode::Fixed:
        break;
    }
}

std::string GeoLine::valueText() const
{
    if (verticalConst) return "x = " + FormatNum(c);
    return LineEqText(a, b, c);
}

std::string GeoLine::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (!src.empty()) return src;   // 1.0.8.3 坐标常量线 x=3 / y=3 原文回填
    switch (mode) {
    case Mode::TwoPoints:
        if (p1 && p2) return std::string("Line(") + p1->label + ", " + p2->label + ")";
        break;
    case Mode::PerpThrough:
        if (pt && base) return std::string("PerpendicularLine(") + pt->label + ", " + base->label + ")";
        break;
    case Mode::ParallelThrough:
        if (pt && base) return std::string("Line(") + pt->label + ", " + base->label + ")";
        break;
    default:
        break;
    }
    return valueText();
}

// ---- GeoCircle ----

void GeoCircle::update()
{
    if (calc) {
        calc(this);
        return;
    }
    if (through) {
        double dx = through->x - center->x;
        double dy = through->y - center->y;
        radius = sqrt(dx * dx + dy * dy);
    } else if (radiusNum) {
        radius = radiusNum->value;
    }
}

std::string GeoCircle::valueText() const
{
    return FormatNum(radius);
}

std::string GeoCircle::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (center && through) return std::string("Circle(") + center->label + ", " + through->label + ")";
    if (center && radiusNum) return std::string("Circle(") + center->label + ", " + radiusNum->label + ")";
    if (center) return std::string("Circle(") + center->label + ", " + FormatNum(radius) + ")";
    return "Circle";
}

// ---- GeoFunction ----

std::string GeoFunction::valueText() const
{
    if (ineqOp || !ineqChain.empty() || !ineqDNF.empty()) return src;   // 不等式：值列/定义列即原文（可回填重定义）
    return label + "(x) = " + src;
}

std::string GeoFunction::definitionText() const
{
    if (ineqOp || !ineqChain.empty() || !ineqDNF.empty()) return src;
    return label + "(x) = " + src;
}

// ---- GeoRay ----

std::string GeoRay::valueText() const
{
    if (!image && (!p1 || !p2)) return "Ray";
    double a = ey1() - ey2(), b = ex2() - ex1();
    double c = a * ex1() + b * ey1();
    return LineEqText(a, b, c);
}

std::string GeoRay::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (!image && p1 && p2) return std::string("Ray(") + p1->label + ", " + p2->label + ")";
    return "Ray";
}

// ---- GeoVector ----

std::string GeoVector::valueText() const
{
    if (!image && (!p1 || !p2)) return "Vector";
    return "(" + FormatNum(ex2() - ex1()) + ", " + FormatNum(ey2() - ey1()) + ")";
}

std::string GeoVector::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (!image && p1 && p2) return std::string("Vector(") + p1->label + ", " + p2->label + ")";
    return "Vector";
}

// ---- GeoList ----

// 值列：{a, b, c}；超 10 个元素截断（值列只作预览，完整数据仍参与计算）
static std::string ListBraceText(const std::vector<double> &v, size_t maxShow)
{
    std::string s = "{";
    for (size_t i = 0; i < v.size() && i < maxShow; ++i) {
        if (i > 0) s += ", ";
        s += FormatNum(v[i]);
    }
    if (v.size() > maxShow) s += ", …";
    s += "}";
    return s;
}

std::string GeoList::valueText() const
{
    // 1.0.6.3 图表列表：值列给形态摘要（数据本体画在画布上，行列出条数）
    if (!chart.empty()) {
        const size_t n = chart == "box" ? 5 : values.size();
        const char *cn = chart == "bar"     ? "柱状图"
                       : chart == "hist"    ? "直方图"
                       : chart == "line"    ? "折线图"
                       : chart == "scatter" ? "散点图"
                       : chart == "box"     ? "箱线图"
                                            : "茎叶图";
        return std::string(cn) + "（" + std::to_string(n)
             + (chart == "hist" ? " 组）" : chart == "box" ? " 数概括）" : " 条）");
    }
    // 1.0.7.3 点列表：{(1, 2), (3, 4)}（最多 10 项，余略）
    if (!points.empty()) {
        std::string s = "{";
        for (size_t i = 0; i < points.size() && i < 10; ++i) {
            if (i > 0) s += ", ";
            s += "(" + FormatNum(points[i].first) + ", " + FormatNum(points[i].second) + ")";
        }
        if (points.size() > 10) s += ", …";
        return s + "}";
    }
    if (values.empty()) return "{}";
    if (isMatrix) {
        // 矩阵按行拆行显示（{{1,2},{3,4}} → 两行花括号串）；GGB 用 LaTeX
        // 矩阵排版，文本代数区用行串等价呈现
        std::string s = "{";
        for (int r = 0; r < mrows; ++r) {
            if (r > 0) s += ", ";
            s += "{";
            for (int c = 0; c < mcols; ++c) {
                if (c > 0) s += ", ";
                s += FormatNum(values[(size_t)r * mcols + c]);
            }
            s += "}";
        }
        return s + "}";
    }
    return ListBraceText(values, 10);
}

std::string GeoList::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    // 七十三包：依随表达式矩阵（C=A+B）定义列带标签，重定义输入框可直达
    //（GeoNumeric 依随式同款）；自由列表仍回原花括号文本
    if (calc && !src.empty()) return label + " = " + src;
    if (!src.empty()) return src;
    return valueText();
}

// ---- GeoPolygon ----

std::string GeoPolygon::valueText() const
{
    // 1.0.8.2 三维多边形：Newell 叉积和面积（|Σ p_i × p_{i+1}| / 2），
    // 顶点全 z=0 时与鞋带公式同值
    if (is3D) {
        double ax = 0, ay = 0, az = 0;
        for (size_t i = 0; i < verts.size(); ++i) {
            const GeoPoint *p = verts[i];
            const GeoPoint *q = verts[(i + 1) % verts.size()];
            ax += p->y * q->z - p->z * q->y;
            ay += p->z * q->x - p->x * q->z;
            az += p->x * q->y - p->y * q->x;
        }
        return FormatNum(sqrt(ax * ax + ay * ay + az * az) / 2.0);
    }
    // 鞋带公式面积（绝对值）
    double area = 0;
    if (image) {
        for (size_t i = 0; i < ivx.size(); ++i) {
            size_t j = (i + 1) % ivx.size();
            area += ivx[i] * ivy[j] - ivx[j] * ivy[i];
        }
    } else {
        for (size_t i = 0; i < verts.size(); ++i) {
            size_t j = (i + 1) % verts.size();
            area += verts[i]->x * verts[j]->y - verts[j]->x * verts[i]->y;
        }
    }
    return FormatNum(fabs(area) / 2.0);
}

std::string GeoPolygon::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (!verts.empty()) {
        std::string s = "Polygon(";
        for (size_t i = 0; i < verts.size(); ++i) {
            if (i > 0) s += ", ";
            s += verts[i]->label;
        }
        return s + ")";
    }
    return "Polygon";
}

// ---- 1.0.8.1 三维对象族 ----

std::string GeoSurface::valueText() const
{
    if (parametric) {
        // 参数曲面值列：分量原文（GGB 代数区同位显示分量式）
        return "(" + ex.text() + ", " + ey.text() + ", " + ez.text() + ")";
    }
    return "z = " + src;
}

std::string GeoSurface::definitionText() const
{
    if (!cmdName.empty()) return cmdDefText();
    if (parametric) return label + " = Surface(" + ex.text() + ", " + ey.text()
        + ", " + ez.text() + ", " + uName + ", " + vName + ")";
    return label + "(x, y) = " + src;
}

// ax+by+cz=d 展示（1.0.8.2；零系数跳过、±1 省略数值，全零写 "0 = d"）
std::string PlaneEqText(double a, double b, double c, double d)
{
    const char *names[3] = { "x", "y", "z" };
    const double co[3] = { a, b, c };
    std::string lhs;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(co[i]) < 1e-12) continue;
        const double ab = std::fabs(co[i]);
        const bool unit = ab - 1.0 < 1e-12;
        if (lhs.empty()) {
            lhs = (co[i] < 0 ? std::string("-") : std::string())
                + (unit ? std::string(names[i]) : FormatNum(ab) + names[i]);
        } else {
            lhs += co[i] > 0 ? " + " : " - ";
            lhs += unit ? std::string(names[i]) : FormatNum(ab) + names[i];
        }
    }
    if (lhs.empty()) lhs = "0";
    return lhs + " = " + FormatNum(d);
}

std::string GeoPlane::valueText() const
{
    if (std::fabs(a) < 1e-12 && std::fabs(b) < 1e-12 && std::fabs(c) < 1e-12) {
        return "未定义";
    }
    return PlaneEqText(a, b, c, d);
}

std::string GeoPlane::definitionText() const
{
    std::string dc = cmdDefText();
    if (!dc.empty()) return dc;
    return src.empty() ? "Plane" : src;
}

std::string GeoCircle3::valueText() const
{
    if (!defined()) return "未定义";
    return "圆心 (" + FormatNum(cx) + ", " + FormatNum(cy) + ", "
         + FormatNum(cz) + ")，半径 " + FormatNum(r);
}

std::string GeoCircle3::definitionText() const
{
    std::string d = cmdDefText();
    return d.empty() ? "Circle3" : d;
}

std::string GeoConic3::valueText() const
{
    // 1.1.0.1 圆=Circle3 口径；椭圆=中心+长短半轴（参数化奇异值，闭式
    // 长轴 R/|cos∠(法,轴)| 短轴 R）；平行直线=条数口径
    if (!defined()) return "未定义";
    if (kind == 2) return tangent ? "一条直线（切线）" : "两条平行直线";
    if (kind == 3) return "抛物线";
    if (kind == 4) return "双曲线";
    if (kind == 5) {
        // 1.1.0.7① 过顶点退化：方向零向量 → 仅顶点单点；否则按条数口径
        const double un = sqrt(ux * ux + uy * uy + uz * uz);
        const double vn = sqrt(vx * vx + vy * vy + vz * vz);
        if (!(un > 1e-12) && !(vn > 1e-12)) return "顶点（单点）";
        return tangent ? "一条直线（切线）" : "两条相交直线（过顶点）";
    }
    const double gu = ux * ux + uy * uy + uz * uz;
    const double gv = vx * vx + vy * vy + vz * vz;
    const double guv = ux * vx + uy * vy + uz * vz;
    const double tr = gu + gv;
    const double disc = sqrt(std::max(0.0, tr * tr / 4.0
                                        - (gu * gv - guv * guv)));
    const double sa = sqrt(tr / 2.0 + disc), sb = sqrt(tr / 2.0 - disc);
    if (kind == 0) {
        return "圆心 (" + FormatNum(cx) + ", " + FormatNum(cy) + ", "
             + FormatNum(cz) + ")，半径 " + FormatNum(sa);
    }
    return "中心 (" + FormatNum(cx) + ", " + FormatNum(cy) + ", "
         + FormatNum(cz) + ")，长半轴 " + FormatNum(sa) + "，短半轴 "
         + FormatNum(sb);
}

std::string GeoConic3::definitionText() const
{
    std::string d = cmdDefText();
    return d.empty() ? "Conic3" : d;
}

std::string GeoCone::valueText() const
{
    // 与圆柱同口径：r=底半径，h=顶点到底心现值
    double r = radiusNum ? radiusNum->value : radius;
    double h = sqrt(pow(bcx() - avx(), 2.0) + pow(bcy() - avy(), 2.0)
                    + pow(bcz() - avz(), 2.0));
    return "r=" + FormatNum(r) + "，h=" + FormatNum(h);
}

std::string GeoCone::definitionText() const
{
    std::string d = cmdDefText();
    return d.empty() ? "Cone" : d;
}

std::string GeoSphere::valueText() const
{
    double r = radius;
    if (radiusNum) r = radiusNum->value;
    else if (through) {
        const double dx = through->x - ccx();
        const double dy = through->y - ccy();
        const double dz = through->z - ccz();
        r = sqrt(dx * dx + dy * dy + dz * dz);
    }
    return "x² + y² + z² = " + FormatNum(r * r)
         + "（半径 " + FormatNum(r) + "）";
}

std::string GeoSphere::definitionText() const
{
    std::string d = cmdDefText();
    return d.empty() ? "Sphere" : d;
}

std::string GeoCylinder::valueText() const
{
    double r = radiusNum ? radiusNum->value : radius;
    // 1.1.0.2 h 随顶底心现值（字面量顶心形式的 height 字段不更新——
    // 1.0.8.6 起；访问器口径与渲染一致，各形式 tcz() 回退均自洽）
    double h = heightNum
                   ? heightNum->value
                   : sqrt(pow(tcx() - bcx(), 2.0) + pow(tcy() - bcy(), 2.0)
                          + pow(tcz() - bcz(), 2.0));
    return "r=" + FormatNum(r) + "，h=" + FormatNum(h);
}

std::string GeoCylinder::definitionText() const
{
    std::string d = cmdDefText();
    return d.empty() ? "Cylinder" : d;
}

std::string GeoLine3::valueText() const
{
    if (!image && (!p1 || !p2)) return "直线";
    if (image && (!std::isfinite(ix1) || !std::isfinite(ix2))) return "未定义";
    const double dx = ex2() - ex1(), dy = ey2() - ey1(), dz = ez2() - ez1();
    return "X = (" + FormatNum(ex1()) + ", " + FormatNum(ey1()) + ", "
         + FormatNum(ez1()) + ") + t·(" + FormatNum(dx) + ", "
         + FormatNum(dy) + ", " + FormatNum(dz) + ")";
}

std::string GeoLine3::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (!image && p1 && p2) return std::string("Line(") + p1->label + ", " + p2->label + ")";
    return "Line";
}

} // namespace dreamember
