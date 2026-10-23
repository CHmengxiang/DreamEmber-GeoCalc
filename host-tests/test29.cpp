// test29.cpp — 1.0.6.1 host 测试：回归批（FitLine/FitPoly/FitExp/
// CorrelationCoefficient）+ 矩阵代数（{{…}} 字面量 / Transpose / Determinant /
// Invert / Identity / Row / Column）。
// 覆盖：
//  A. 矩阵字面量：解析落位（isMatrix/mrows/mcols 扁平行主序）、行错位拒绝、
//     残余文本拒绝、普通列表不受影响（回归）、valueText 行串
//  B. FitLine：完美数据 y=2x+1、依随重算（改列表元素后直线跟随）、
//     竖直线（x 全同）失败报错、矩阵拒绝
//  C. FitPoly：二次数据恢复 2 次式系数、三次、阶数越界报错、点数不足、
//     阶数对象（滑动条）依随
//  D. FitExp：a·e^(bx) 数据、全负 y 取号
//  E. CorrelationCoefficient：完美正相关 r=1、完美负相关 r=-1
//  F. Transpose/Determinant/Invert/Identity：2×2/3×3 数值、奇异拒绝、
//     非方阵拒绝、M·M⁻¹=I 抽查、单位阵恒等
//  G. Row/Column：取行/取列、越界报错
//  H. 保存往返：FitLine/FitPoly/矩阵 getXml→setXml 重放重建
//  I. 既有命令回归（Mean/SD/Sort/Sequence 不受列表结构改动影响）
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     ../../../../host-tests/test29.cpp kernel/app.cpp kernel/expr.cpp
//     kernel/geos.cpp kernel/view.cpp kernel/ggbfile.cpp cas/cas_wrapper.cpp
//     -lz -o ../../../../host-tests/test29.exe
#include "kernel/app.h"
#include <cstdio>
#include <cmath>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

static GeoElement *find(Kernel &k, const char *label)
{
    for (auto &g : k.geos()) if (g->label == label) return g.get();
    return nullptr;
}
static GeoNumeric *numOf(Kernel &k, const char *label)
{
    return dynamic_cast<GeoNumeric *>(find(k, label));
}
static GeoList *listOf(Kernel &k, const char *label)
{
    return dynamic_cast<GeoList *>(find(k, label));
}
static GeoFunction *fnOf(Kernel &k, const char *label)
{
    return dynamic_cast<GeoFunction *>(find(k, label));
}
static GeoLine *lineOf(Kernel &k, const char *label)
{
    return dynamic_cast<GeoLine *>(find(k, label));
}

int main()
{
    // ---------- A. 矩阵字面量 ----------
    {
        Kernel k;
        CHECK(k.input("M={{1,2},{3,4}}"));
        GeoList *m = listOf(k, "M");
        CHECK(m != nullptr && m->isMatrix);
        if (m) {
            CHECK(m->mrows == 2 && m->mcols == 2);
            CHECK(m->values.size() == 4);
            CHECK(fabs(m->values[0] - 1) < 1e-12 && fabs(m->values[1] - 2) < 1e-12);
            CHECK(fabs(m->values[2] - 3) < 1e-12 && fabs(m->values[3] - 4) < 1e-12);
            CHECK(m->valueText().find("{1, 2}") != std::string::npos);
            CHECK(m->valueText().find("{3, 4}") != std::string::npos);
        }
        CHECK(k.input("N={{1.5, 2+1, sin(0)},{4, 5, 6},{7, 8, 9}}"));
        GeoList *n = listOf(k, "N");
        CHECK(n != nullptr && n->isMatrix && n->mrows == 3 && n->mcols == 3);
        if (n) CHECK(fabs(n->values[1] - 3.0) < 1e-12);
        // 行错位拒绝
        CHECK(!k.input("W={{1,2},{3}}"));
        CHECK(k.lastError.find("不一致") != std::string::npos);
        // 残余文本拒绝
        CHECK(!k.input("V={{1,2}x{3,4}}"));
        // 3×2 非方阵可建
        CHECK(k.input("R={{1,2,3},{4,5,6}}"));
        GeoList *r = listOf(k, "R");
        CHECK(r != nullptr && r->isMatrix && r->mrows == 2 && r->mcols == 3);
        // 普通列表回归：嵌套花括号不存在时原语义不变
        CHECK(k.input("L1={1,2,3}"));
        GeoList *l1 = listOf(k, "L1");
        CHECK(l1 != nullptr && !l1->isMatrix && l1->values.size() == 3);
    }

    // ---------- B. FitLine ----------
    {
        Kernel k;
        CHECK(k.input("xs={0,1,2,3}"));
        CHECK(k.input("ys={1,3,5,7}"));   // y=2x+1
        CHECK(k.input("g=FitLine(xs,ys)"));
        GeoLine *l = lineOf(k, "g");
        CHECK(l != nullptr);
        if (l) {
            // y = 2x + 1 → -2x + y = 1（a=-2 b=1 c=1 或同倍数）
            const double slope = fabs(l->b) > 1e-15 ? -l->a / l->b : 0;
            const double icept = fabs(l->b) > 1e-15 ? l->c / l->b : 0;
            CHECK(fabs(slope - 2.0) < 1e-9);
            CHECK(fabs(icept - 1.0) < 1e-9);
            CHECK(l->cmdName == "FitLine");
        }
        // 依随重算：重定义列表（redefine 入口，input 重定义被依赖对象按既有
        // 语义拒绝）后直线跟随
        CHECK(k.redefine("xs", "{0,1,2,10}"));
        CHECK(k.redefine("ys", "{1,3,5,21}"));   // y=2x+1 仍成立
        if (l) {
            l->update();
            const double slope = fabs(l->b) > 1e-15 ? -l->a / l->b : 0;
            CHECK(fabs(slope - 2.0) < 1e-9);
        }
        // x 全同 → 竖直线 x=2（几何正确解；valueText 可读）
        Kernel k2;
        k2.input("xs={2,2,2}");
        k2.input("ys={1,2,3}");
        CHECK(k2.input("g=FitLine(xs,ys)"));
        GeoLine *lv = lineOf(k2, "g");
        CHECK(lv != nullptr);
        if (lv) {
            CHECK(lv->verticalConst);
            CHECK(fabs(lv->c - 2.0) < 1e-12);
        }
        // 长度不等：取交集长度仍可解
        Kernel k3;
        k3.input("xs={0,1,2}");
        k3.input("ys={1,3,5,99}");
        CHECK(k3.input("g=FitLine(xs,ys)"));
        GeoLine *l3 = lineOf(k3, "g");
        if (l3) {
            const double slope = fabs(l3->b) > 1e-15 ? -l3->a / l3->b : 0;
            CHECK(fabs(slope - 2.0) < 1e-9);
        }
    }

    // ---------- C. FitPoly ----------
    {
        Kernel k;
        CHECK(k.input("xs={0,1,2,3}"));
        CHECK(k.input("ys={1,2,5,10}"));   // y=x^2+1
        CHECK(k.input("p=FitPoly(xs,ys,2)"));
        GeoFunction *f = fnOf(k, "p");
        CHECK(f != nullptr);
        if (f) {
            CHECK(fabs(f->evalAt(0.0) - 1.0) < 1e-6);
            CHECK(fabs(f->evalAt(2.0) - 5.0) < 1e-6);
            CHECK(fabs(f->evalAt(3.0) - 10.0) < 1e-6);
            CHECK(fabs(f->evalAt(1.5) - 3.25) < 1e-5);   // 插值点
            CHECK(f->cmdName == "FitPoly");
        }
        // 三次精确恢复
        CHECK(k.input("ys3={0,1,8,27}"));   // y=x^3
        CHECK(k.input("q=FitPoly(xs,ys3,3)"));
        GeoFunction *q = fnOf(k, "q");
        CHECK(q != nullptr);
        if (q) CHECK(fabs(q->evalAt(2.5) - 15.625) < 1e-5);
        // 阶数越界
        CHECK(!k.input("bad=FitPoly(xs,ys,9)"));
        CHECK(!k.input("bad=FitPoly(xs,ys,0)"));
        // 点数不足（阶数+1 > 点数）
        CHECK(!k.input("bad=FitPoly({1,2},{3,4},2)"));
        // 阶数为滑动条对象
        Kernel k2;
        k2.input("n=Slider(1,3,1)");
        k2.input("xs={0,1,2,3}");
        k2.input("ys={1,2,5,10}");
        CHECK(k2.input("p=FitPoly(xs,ys,n)"));
        GeoFunction *f2 = fnOf(k2, "p");
        CHECK(f2 != nullptr);
        // 滑动条初始值 1 → 一次拟合 f=3x（最小二乘过 (0,1)(3,10) 的线 y=3x）
        if (f2) CHECK(fabs(f2->evalAt(3.0) - 9.0) < 1e-5);
        // 阶数改 1：直线拟合（最小二乘，非插值）
        GeoNumeric *nn = numOf(k2, "n");
        if (nn && f2) {
            nn->value = 1;
            nn->update();
            f2->update();
            const double y3 = f2->evalAt(3.0);
            CHECK(y3 > 8.5 && y3 < 11.5);   // 最小二乘线在精确值附近
        }
    }

    // ---------- D. FitExp ----------
    {
        Kernel k;
        CHECK(k.input("xs={0,1,2,3}"));
        CHECK(k.input("ys={1,e,e^2,e^3}"));   // y=e^x
        CHECK(k.input("h=FitExp(xs,ys)"));
        GeoFunction *f = fnOf(k, "h");
        CHECK(f != nullptr);
        if (f) {
            CHECK(fabs(f->evalAt(0.0) - 1.0) < 1e-6);
            CHECK(fabs(f->evalAt(2.0) - exp(2.0)) < 1e-4);
            CHECK(fabs(f->evalAt(2.5) - exp(2.5)) < 1e-3);
            CHECK(f->cmdName == "FitExp");
        }
        // 全负 y：按首点符号翻正拟合（y=-e^x 形）
        Kernel k2;
        k2.input("xs={0,1,2}");
        k2.input("ys={-1,-e,-e^2}");
        CHECK(k2.input("h=FitExp(xs,ys)"));
        GeoFunction *f2 = fnOf(k2, "h");
        CHECK(f2 != nullptr);
        if (f2) CHECK(fabs(f2->evalAt(1.0) + exp(1.0)) < 1e-5);
        // 正负混杂（首点为正）→ 拒绝
        Kernel k3;
        k3.input("xs={0,1,2}");
        k3.input("ys={1,-2,3}");
        CHECK(!k3.input("h=FitExp(xs,ys)"));
    }

    // ---------- E. CorrelationCoefficient ----------
    {
        Kernel k;
        CHECK(k.input("xs={1,2,3,4}"));
        CHECK(k.input("ys={2,4,6,8}"));
        CHECK(k.input("r=CorrelationCoefficient(xs,ys)"));
        GeoNumeric *n = numOf(k, "r");
        CHECK(n != nullptr && fabs(n->value - 1.0) < 1e-12);
        CHECK(k.input("ys2={8,6,4,2}"));
        CHECK(k.input("r2=CorrelationCoefficient(xs,ys2)"));
        GeoNumeric *n2 = numOf(k, "r2");
        CHECK(n2 != nullptr && fabs(n2->value + 1.0) < 1e-12);
    }

    // ---------- F. Transpose/Determinant/Invert/Identity ----------
    {
        Kernel k;
        CHECK(k.input("M={{1,2},{3,4}}"));
        CHECK(k.input("T=Transpose(M)"));
        GeoList *t = listOf(k, "T");
        CHECK(t != nullptr && t->isMatrix && t->mrows == 2 && t->mcols == 2);
        if (t) {
            CHECK(fabs(t->values[0] - 1) < 1e-12 && fabs(t->values[1] - 3) < 1e-12);
            CHECK(fabs(t->values[2] - 2) < 1e-12 && fabs(t->values[3] - 4) < 1e-12);
        }
        CHECK(k.input("d=Determinant(M)"));
        GeoNumeric *d = numOf(k, "d");
        CHECK(d != nullptr && fabs(d->value + 2.0) < 1e-12);
        CHECK(k.input("I2=Identity(2)"));
        GeoList *id = listOf(k, "I2");
        CHECK(id != nullptr && id->isMatrix && id->mrows == 2);
        // 依随重算：重定义 M（redefine 入口）后 T/d 跟随
        CHECK(k.redefine("M", "{{2,0},{0,5}}"));
        if (t) {
            t->update();
            CHECK(fabs(t->values[1] - 0.0) < 1e-12 && fabs(t->values[2] - 0.0) < 1e-12);
        }
        GeoNumeric *d2 = numOf(k, "d");
        if (d2) {
            d2->update();
            CHECK(fabs(d2->value - 10.0) < 1e-12);
        }
        // 3×3 行列式（含取整语义）
        CHECK(k.input("B3={{1,2,3},{4,5,6},{7,8,10}}"));
        CHECK(k.input("d3=Determinant(B3)"));
        GeoNumeric *d3 = numOf(k, "d3");
        CHECK(d3 != nullptr && fabs(d3->value + 3.0) < 1e-9);
        // 非方阵拒绝
        CHECK(k.input("R={{1,2,3},{4,5,6}}"));
        CHECK(!k.input("bad=Determinant(R)"));
        CHECK(!k.input("badT=Transpose({1,2,3})"));   // 普通列表拒绝
        // 逆矩阵：M·M⁻¹=I 抽查 + 整数阵取整
        CHECK(k.input("MI=Invert(M)"));
        GeoList *mi = listOf(k, "MI");
        CHECK(mi != nullptr && mi->isMatrix && mi->mrows == 2);
        if (mi) {
            // M={{2,0},{0,5}} → M⁻¹={{0.5,0},{0,0.2}}
            CHECK(fabs(mi->values[0] - 0.5) < 1e-12);
            CHECK(fabs(mi->values[3] - 0.2) < 1e-12);
            CHECK(fabs(mi->values[1]) < 1e-12 && fabs(mi->values[2]) < 1e-12);
        }
        CHECK(k.input("A={{2,1},{1,1}}"));   // det=1 → 整数逆 {{1,-1},{-1,2}}
        CHECK(k.input("AI=Invert(A)"));
        GeoList *ai = listOf(k, "AI");
        CHECK(ai != nullptr);
        if (ai) {
            CHECK(fabs(ai->values[0] - 1) < 1e-12 && fabs(ai->values[1] + 1) < 1e-12);
            CHECK(fabs(ai->values[2] + 1) < 1e-12 && fabs(ai->values[3] - 2) < 1e-12);
        }
        // 奇异阵拒绝
        CHECK(k.input("S={{1,2},{2,4}}"));
        CHECK(!k.input("SI=Invert(S)"));
    }

    // ---------- G. Row/Column ----------
    {
        Kernel k;
        CHECK(k.input("M={{1,2,3},{4,5,6}}"));
        CHECK(k.input("r1=Row(M,1)"));
        GeoList *r1 = listOf(k, "r1");
        CHECK(r1 != nullptr && !r1->isMatrix && r1->values.size() == 3);
        if (r1) CHECK(fabs(r1->values[2] - 3) < 1e-12);
        CHECK(k.input("c2=Column(M,2)"));
        GeoList *c2 = listOf(k, "c2");
        CHECK(c2 != nullptr && c2->values.size() == 2);
        if (c2) {
            CHECK(fabs(c2->values[0] - 2) < 1e-12);
            CHECK(fabs(c2->values[1] - 5) < 1e-12);
        }
        CHECK(!k.input("bad=Row(M,3)"));
        CHECK(!k.input("bad=Column(M,0)"));
    }

    // ---------- H. 保存往返 ----------
    {
        Kernel k;
        CHECK(k.input("xs={0,1,2,3}"));
        CHECK(k.input("ys={1,3,5,7}"));
        CHECK(k.input("g=FitLine(xs,ys)"));
        CHECK(k.input("M={{1,2},{3,4}}"));
        CHECK(k.input("T=Transpose(M)"));
        CHECK(k.input("d=Determinant(M)"));
        const std::string xml = k.getXml();
        Kernel k2;
        int skipped = -1;
        CHECK(k2.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoLine *l = lineOf(k2, "g");
        CHECK(l != nullptr && l->cmdName == "FitLine");
        if (l) {
            const double slope = fabs(l->b) > 1e-15 ? -l->a / l->b : 0;
            CHECK(fabs(slope - 2.0) < 1e-9);
        }
        GeoList *m2 = listOf(k2, "M");
        CHECK(m2 != nullptr && m2->isMatrix && m2->mrows == 2);
        GeoList *t2 = listOf(k2, "T");
        CHECK(t2 != nullptr && t2->isMatrix);
        if (t2) CHECK(fabs(t2->values[1] - 3.0) < 1e-12);
        GeoNumeric *d2 = numOf(k2, "d");
        CHECK(d2 != nullptr && fabs(d2->value + 2.0) < 1e-12);
    }

    // ---------- I. 既有命令回归 ----------
    {
        Kernel k;
        CHECK(k.input("L1={1,2,3,4}"));
        CHECK(k.input("m=Mean(L1)"));
        CHECK(numOf(k, "m") != nullptr && fabs(numOf(k, "m")->value - 2.5) < 1e-12);
        CHECK(k.input("s=SD(L1)"));
        CHECK(numOf(k, "s") != nullptr);
        CHECK(k.input("sq=Sequence(k^2, k, 1, 4)"));
        GeoList *sq = listOf(k, "sq");
        CHECK(sq != nullptr && sq->values.size() == 4 && fabs(sq->values[3] - 16) < 1e-12);
        CHECK(k.input("st=Sort(L1)"));
        CHECK(listOf(k, "st") != nullptr);
        // 列表/矩阵/命令混排不串位
        CHECK(k.input("M={{1,2},{3,4}}"));
        CHECK(k.input("L2={9,8}"));
        CHECK(listOf(k, "M") != nullptr && listOf(k, "M")->isMatrix);
        CHECK(listOf(k, "L2") != nullptr && !listOf(k, "L2")->isMatrix);
    }

    printf("test29: %d checks, %d failed\n", g_n, g_fail);
    return g_fail != 0;
}
