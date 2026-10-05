// test21.cpp — 第二十四包 host 测试（真机反馈修复回归）：
// ⑦ Segment(点,长度) 像线段渲染空指针崩溃（render 裸读 p1/p2 → ex/ey 访问器）
//   + LineDirFromBase 同病（像线段作平行/垂直基准）+ CopyGeoState 向量按
//   GeoRay 拷的 UB（redefine 向量写花对象）；
// ⑤ 垂线/平行线工具接受（线性）函数基准（GGB EuclidianController 的
//   getSelectedFunctionList 同款；非线性函数由输入路径报「须为一次式」）；
// ⑥ 相等向量命中裁决（点内圈 6vp 优先、其余箭身归向量）。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test21.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test21.exe
#include "kernel/app.h"
#include <cstdio>
#include <cmath>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

static void frame(Kernel &k)
{
    std::vector<DrawCmd> out;
    k.view.setCanvasSize(600, 400);
    k.render(out);
}
static double PX(double wx) { return 300.0 + 50.0 * wx; }
static double PY(double wy) { return 200.0 - 50.0 * wy; }

static std::string doneLabel(const std::string &r)
{
    return r.rfind("done|", 0) == 0 ? r.substr(5) : "";
}

// 直线斜率（b≈0 返回 NAN）与过点判定（符号无关）
static double slopeOf(const GeoLine *l)
{
    return std::fabs(l->b) < 1e-12 ? NAN : -l->a / l->b;
}
static bool throughPt(const GeoLine *l, double x, double y)
{
    return std::fabs(l->a * x + l->b * y - l->c) < 1e-9;
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);   // 崩溃前也要吐出 FAIL 行

    // ---------- A. ⑦ 像线段渲染（真机闪退复现路径）----------
    {
        Kernel k;
        CHECK(k.input("A=(1, 2)"));
        CHECK(k.input("s=Segment(A, 3)"));
        GeoSegment *s = dynamic_cast<GeoSegment *>(k.lookup("s"));
        CHECK(s != nullptr && s->image);
        CHECK(s->valueText() == "3");
        // 沿 x 正方向：起点 A、终点 A+(3,0)
        CHECK(std::fabs(s->ex1() - 1) < 1e-9 && std::fabs(s->ey1() - 2) < 1e-9);
        CHECK(std::fabs(s->ex2() - 4) < 1e-9 && std::fabs(s->ey2() - 2) < 1e-9);
        // 渲染此前在此空指针崩溃，现必须出线段指令且端点正确
        std::vector<DrawCmd> fr;
        k.view.setCanvasSize(600, 400);
        k.render(fr);
        bool drew = false;
        for (const auto &c : fr) {
            if (c.op == DrawOp::Segment && c.pts.size() >= 4) {
                if (std::fabs(c.pts[0] - PX(1)) < 0.5 && std::fabs(c.pts[1] - PY(2)) < 0.5
                    && std::fabs(c.pts[2] - PX(4)) < 0.5 && std::fabs(c.pts[3] - PY(2)) < 0.5) {
                    drew = true;
                }
            }
        }
        CHECK(drew);
        // 像线段可拾取
        CHECK(k.pickObject(PX(2.5), PY(2)) == s);
        // 拖 A 级联：像随起点平移、长度不变；级联后渲染仍不崩
        k.movePoint(static_cast<GeoPoint *>(k.lookup("A")), 0, 0);
        CHECK(std::fabs(s->ex1()) < 1e-9 && std::fabs(s->ey1()) < 1e-9);
        CHECK(std::fabs(s->ex2() - 3) < 1e-9 && std::fabs(s->ey2()) < 1e-9);
        std::vector<DrawCmd> fr2;
        k.render(fr2);
    }

    // ---------- B. ⑦ 像线段作平行/垂直基准（LineDirFromBase ex/ey）----------
    {
        Kernel k;
        CHECK(k.input("A=(1, 2)"));
        CHECK(k.input("s=Segment(A, 3)"));
        CHECK(k.input("B=(0, 4)"));
        // Line(点, 像线段) = 过点平行线（水平线 y=4）
        CHECK(k.input("l=Line(B, s)"));
        GeoLine *l = dynamic_cast<GeoLine *>(k.lookup("l"));
        CHECK(l != nullptr);
        CHECK(std::fabs(slopeOf(l)) < 1e-12 && throughPt(l, 0, 4));
        // PerpendicularLine(点, 像线段) = 过点垂直线（竖直线 x=0）
        CHECK(k.input("m=PerpendicularLine(B, s)"));
        GeoLine *m = dynamic_cast<GeoLine *>(k.lookup("m"));
        CHECK(m != nullptr);
        CHECK(std::fabs(m->b) < 1e-12 && throughPt(m, 0, 4) && throughPt(m, 0, 9));
        frame(k);   // 像线段作基准后渲染不崩
    }

    // ---------- C. ⑦ redefine 向量（CopyGeoState 向量分支）----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(2, 1)"));
        CHECK(k.input("u=Vector(A, B)"));
        CHECK(k.input("P=(5, 5)"));
        CHECK(k.input("w=Vector(P, u)"));
        GeoVector *w = dynamic_cast<GeoVector *>(k.lookup("w"));
        CHECK(w != nullptr && w->image && w->valueText() == "(2, 1)");
        // 同型重定义：走 CopyGeoState（此前按 GeoRay 拷丢 image/ix/calc 写花对象）
        CHECK(k.redefine("w", "Vector(P, u)"));
        GeoVector *w2 = dynamic_cast<GeoVector *>(k.lookup("w"));
        CHECK(w2 != nullptr && w2->image);
        CHECK(w2->valueText() == "(2, 1)");
        // 重定义后仍级联、可拾取、可渲染
        k.movePoint(static_cast<GeoPoint *>(k.lookup("B")), 4, 1);
        CHECK(w2->valueText() == "(4, 1)");
        CHECK(k.pickObject(PX(6), PY(5.25)) == w2);
        frame(k);
    }

    // ---------- D. ⑤ 垂线工具点函数曲线（真机反馈场景）----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("f(x)=2x+1"));
        k.setTool("perpLine");
        // 先点曲线（x=1, y=3）再点空白建 P=(3, 0.5)
        std::string r1 = k.toolTap(1, 3, PX(1), PY(3));
        CHECK(r1.rfind("need|", 0) == 0);
        std::string r2 = k.toolTap(3, 0.5, PX(3), PY(0.5));
        std::string lb = doneLabel(r2);
        CHECK(!lb.empty());
        GeoLine *n = dynamic_cast<GeoLine *>(k.lookup(lb));
        CHECK(n != nullptr);
        // 法线：斜率 -1/2、过 P（GGB 语义 = 过点、以一次式斜率的负倒数为方向）
        CHECK(std::fabs(slopeOf(n) + 0.5) < 1e-9 && throughPt(n, 3, 0.5));
        // 反序：先空白建点，再点曲线
        Kernel k2;
        frame(k2);
        CHECK(k2.input("f(x)=2x+1"));
        k2.setTool("perpLine");
        std::string q1 = k2.toolTap(-2, 1, PX(-2), PY(1));
        CHECK(q1.rfind("need|", 0) == 0);
        std::string q2 = k2.toolTap(1, 3, PX(1), PY(3));
        std::string lb2 = doneLabel(q2);
        CHECK(!lb2.empty());
        GeoLine *n2 = dynamic_cast<GeoLine *>(k2.lookup(lb2));
        CHECK(n2 != nullptr);
        CHECK(std::fabs(slopeOf(n2) + 0.5) < 1e-9 && throughPt(n2, -2, 1));
    }

    // ---------- E. ⑤ 平行线工具点函数曲线 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("f(x)=2x+1"));
        k.setTool("parLine");
        std::string r1 = k.toolTap(1, 3, PX(1), PY(3));
        CHECK(r1.rfind("need|", 0) == 0);
        std::string r2 = k.toolTap(4, -2, PX(4), PY(-2));
        std::string lb = doneLabel(r2);
        CHECK(!lb.empty());
        GeoLine *l = dynamic_cast<GeoLine *>(k.lookup(lb));
        CHECK(l != nullptr);
        // 平行：斜率 2 过 (4,-2)
        CHECK(std::fabs(slopeOf(l) - 2) < 1e-9 && throughPt(l, 4, -2));
    }

    // ---------- F. ⑤ 非线性函数基准报错不创建 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("g(x)=x^2"));
        k.setTool("perpLine");
        std::string r1 = k.toolTap(1, 1, PX(1), PY(1));
        CHECK(r1.rfind("need|", 0) == 0);   // 曲线被选中（工具已收函数）
        std::string r2 = k.toolTap(3, 0.5, PX(3), PY(0.5));
        CHECK(r2.rfind("err|", 0) == 0);    // 非一次式 → 输入路径报错
        CHECK(k.geos().size() == 2);        // 只有 g 与新建点，无直线
        // 平行线工具同样报错（复用上面已建的点）
        k.setTool("parLine");
        std::string r3 = k.toolTap(3, 0.5, PX(3), PY(0.5));
        CHECK(r3.rfind("need|", 0) == 0);
        std::string r4 = k.toolTap(1, 1, PX(1), PY(1));
        CHECK(r4.rfind("err|", 0) == 0);
        CHECK(k.geos().size() == 2);
    }

    // ---------- G. ⑥ 相等向量命中裁决 ----------
    {
        // ① 箭身中段 → 向量；端点核内 → 点；点+向量齐 → 完成
        Kernel k;
        frame(k);
        CHECK(k.input("A=(1, 1)"));
        CHECK(k.input("B=(3, 1)"));
        CHECK(k.input("w=Vector(A, B)"));
        k.setTool("vecFromPoint");
        std::string r1 = k.toolTap(2, 1, PX(2), PY(1));
        CHECK(r1 == "need|相等向量：再点选一个点");
        std::string r2 = k.toolTap(3, 1, PX(3), PY(1));
        std::string lb1 = doneLabel(r2);
        CHECK(!lb1.empty());
        GeoVector *v1 = dynamic_cast<GeoVector *>(k.lookup(lb1));
        CHECK(v1 != nullptr && v1->image && v1->valueText() == "(2, 0)");

        // ② 新会话：先点端点核内（toolObj_ 空）→ 点入列，等向量
        Kernel k2;
        frame(k2);
        CHECK(k2.input("A=(1, 1)"));
        CHECK(k2.input("B=(3, 1)"));
        CHECK(k2.input("w=Vector(A, B)"));
        k2.setTool("vecFromPoint");
        std::string r3 = k2.toolTap(3, 1, PX(3), PY(1));
        CHECK(r3 == "need|相等向量：再点选向量");
        std::string r4 = k2.toolTap(2, 1, PX(2), PY(1));
        std::string lb2 = doneLabel(r4);
        CHECK(!lb2.empty());

        // ③ 修复点：端点旁箭身（B 外 7.5vp、核 6vp 之外）→ 向量
        //（修复前：18vp 点命中半径抢走，只能选到点）
        Kernel k3;
        frame(k3);
        CHECK(k3.input("A=(1, 1)"));
        CHECK(k3.input("B=(3, 1)"));
        CHECK(k3.input("w=Vector(A, B)"));
        k3.setTool("vecFromPoint");
        std::string r5 = k3.toolTap(2.85, 1, PX(2.85), PY(1));
        CHECK(r5 == "need|相等向量：再点选一个点");
        // 空白建点 + 已收向量 → 立即完成
        std::string r6 = k3.toolTap(0, -2, PX(0), PY(-2));
        std::string lb3 = doneLabel(r6);
        CHECK(!lb3.empty());
        GeoVector *v3 = dynamic_cast<GeoVector *>(k.lookup(lb3));
        CHECK(v3 != nullptr && v3->image && v3->valueText() == "(2, 0)");
        frame(k3);
    }

    // ---------- H. 回归：普通线段工具流与渲染不受影响 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(2, 0)"));
        CHECK(k.input("s=Segment(A, B)"));
        GeoSegment *s = dynamic_cast<GeoSegment *>(k.lookup("s"));
        CHECK(s != nullptr && !s->image);
        // 垂线工具：点线段（水平）→ 空白建点 (4,0) → 竖直线 x=4
        k.setTool("perpLine");
        std::string r1 = k.toolTap(1, 0, PX(1), PY(0));
        CHECK(r1.rfind("need|", 0) == 0);
        std::string r2 = k.toolTap(4, 0, PX(4), PY(0));
        std::string lb = doneLabel(r2);
        CHECK(!lb.empty());
        GeoLine *n = dynamic_cast<GeoLine *>(k.lookup(lb));
        CHECK(n != nullptr);
        CHECK(std::fabs(n->b) < 1e-12 && throughPt(n, 4, 0) && throughPt(n, 4, 3));
        std::vector<DrawCmd> fr;
        k.render(fr);
        CHECK(!fr.empty());
    }

    printf("test21: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
