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
        if (fabs(a) > 1e-15) s += " - " + FormatNum(a / b) + "x";
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
    // 旧语义兜底：inputs 恰为两个点时按 Midpoint 取中点
    if (inputs.size() == 2) {
        GeoPoint *p1 = dynamic_cast<GeoPoint *>(inputs[0]);
        GeoPoint *p2 = dynamic_cast<GeoPoint *>(inputs[1]);
        if (p1 && p2) {
            x = 0.5 * (p1->x + p2->x);
            y = 0.5 * (p1->y + p2->y);
        }
    }
}

std::string GeoPoint::valueText() const
{
    return "(" + FormatNum(x) + ", " + FormatNum(y) + ")";
}

std::string GeoPoint::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (inputs.size() == 2) {
        return "Midpoint(" + inputs[0]->label + ", " + inputs[1]->label + ")";
    }
    return "(" + FormatNum(x) + ", " + FormatNum(y) + ")";
}

// ---- GeoSegment ----

std::string GeoSegment::valueText() const
{
    double dx = ex2() - ex1(), dy = ey2() - ey1();
    return FormatNum(sqrt(dx * dx + dy * dy));
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
    if (ineqOp) return src;   // 不等式：值列/定义列即原文（可回填重定义）
    return label + "(x) = " + src;
}

std::string GeoFunction::definitionText() const
{
    if (ineqOp) return src;
    return label + "(x) = " + src;
}

// ---- GeoRay ----

std::string GeoRay::valueText() const
{
    if (!p1 || !p2) return "Ray";
    double a = p1->y - p2->y, b = p2->x - p1->x;
    double c = a * p1->x + b * p1->y;
    return LineEqText(a, b, c);
}

std::string GeoRay::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (p1 && p2) return std::string("Ray(") + p1->label + ", " + p2->label + ")";
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
    if (values.empty()) return "{}";
    return ListBraceText(values, 10);
}

std::string GeoList::definitionText() const
{
    std::string d = cmdDefText();
    if (!d.empty()) return d;
    if (!src.empty()) return src;
    return valueText();
}

// ---- GeoPolygon ----

std::string GeoPolygon::valueText() const
{
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

} // namespace dreamember
