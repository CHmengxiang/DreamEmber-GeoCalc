// test26.cpp — 第四十五包 host 测试：附着/脱离点（路径依附点）。
// 覆盖：线段附着参数化与拖动钳位、依附点随路径端点联动、脱离保留坐标、
// 圆（角度参数+unwrap）、函数图像（x 参数）、Point(path[, param]) 命令、
// XML 往返（coords 重投影恢复参数）、级联删除路径带走依附点、多边形边、
// 直线（有向距离参数）、重定义依附点为自由坐标=脱离、工具中途改选。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     ../../../host-tests/test26.cpp kernel/app.cpp kernel/expr.cpp
//     kernel/geos.cpp kernel/view.cpp kernel/ggbfile.cpp cas/cas_wrapper.cpp
//     -lz -o ../../../host-tests/test26.exe
#include "kernel/app.h"
#include <cstdio>
#include <cmath>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

static GeoPoint *ptOf(Kernel &k, const std::string &label)
{
    return k.lookupPoint(label);
}

static void frame(Kernel &k)
{
    std::vector<DrawCmd> out;
    k.view.setCanvasSize(600, 400);
    k.render(out);
}

// 工具点按：世界坐标 → 像素拾取坐标同算
static std::string tap(Kernel &k, double wx, double wy)
{
    return k.toolTap(wx, wy, k.view.px(wx), k.view.py(wy));
}

int main()
{
    // ---------- A. 线段附着：投影落轨 + 参数 + 拖动钳位 ----------
    std::string segLabel;
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0,0)"));
        CHECK(k.input("B=(4,0)"));
        CHECK(k.input("s=Segment(A,B)"));
        segLabel = "s";
        CHECK(k.input("P=(1,3)"));
        k.setTool("attachDetach");
        // 第一步点 P
        std::string r1 = tap(k, 1, 3);
        CHECK(r1.rfind("need|", 0) == 0);
        // 第二步点线段上 (2, 0.1)
        std::string r2 = tap(k, 2, 0.1);
        CHECK(r2 == "done|P");
        GeoPoint *P = ptOf(k, "P");
        CHECK(P != nullptr && !P->isFree);
        CHECK(P->path != nullptr && P->path->label == "s");
        CHECK(std::fabs(P->x - 2.0) < 1e-9);
        CHECK(std::fabs(P->y) < 1e-9);
        CHECK(std::fabs(P->pathParam - 0.5) < 1e-9);
        CHECK(P->cmdName == "Point" && P->cmdArgs.size() == 1 && P->cmdArgs[0] == "s");
        CHECK(P->inputs.size() == 1 && P->inputs[0] == P->path);
        CHECK(P->definitionText() == "Point(s)");
        // 拖出端点：钳位
        k.movePoint(P, 10, 5);
        CHECK(std::fabs(P->x - 4.0) < 1e-9 && std::fabs(P->y) < 1e-9);
        k.movePoint(P, -3, 1);
        CHECK(std::fabs(P->x) < 1e-9 && std::fabs(P->y) < 1e-9);
        k.movePoint(P, 1.2, 0.9);
        CHECK(std::fabs(P->x - 1.2) < 1e-9 && std::fabs(P->y) < 1e-9);
        // 此时参数已被拖到 t=0.3

        // ---------- B. 依附点随路径端点联动（参数不变） ----------
        k.movePoint(ptOf(k, "A"), 1, 1);   // A 端点拖到 (1,1)
        // P = A + 0.3*(B-A) = (1.9, 0.7)
        CHECK(std::fabs(P->x - 1.9) < 1e-9);
        CHECK(std::fabs(P->y - 0.7) < 1e-9);

        // ---------- C. 脱离：保留当前坐标 ----------
        std::string r3 = tap(k, 1.9, 0.7);   // 点 P 本体（此时在 (1.9,0.7)）
        CHECK(r3.rfind("need|", 0) == 0 && r3.find("脱离") != std::string::npos);
        std::string r4 = tap(k, 5, 5);       // 点空白 → 脱离
        CHECK(r4 == "done|P");
        CHECK(P->isFree && P->path == nullptr && P->inputs.empty());
        CHECK(P->cmdName.empty());
        CHECK(std::fabs(P->x - 1.9) < 1e-9 && std::fabs(P->y - 0.7) < 1e-9);
        CHECK(P->definitionText() == "(1.9, 0.7)");
        // 自由拖动恢复
        k.movePoint(P, 0, 0);
        CHECK(std::fabs(P->x) < 1e-9 && std::fabs(P->y) < 1e-9);
    }

    // ---------- D. 圆附着：角度参数 + unwrap ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("O=(0,0)"));
        CHECK(k.input("Q=(2,0)"));
        CHECK(k.input("c=Circle(O,Q)"));
        CHECK(k.input("P=(2,0)"));   // 与 Q 重合也先拾到 P（后建优先距离同零取先遍历，直接用工具点 P 处）
        k.setTool("attachDetach");
        // tap 落在 Q 与 P 重合处：pickPoint 取先遍历（O,Q 圆的关键点）——
        // 为避歧义先把 P 挪开一点再拾取
        k.movePoint(ptOf(k, "P"), 1.9, 0.3);
        std::string r1 = tap(k, 1.9, 0.3);
        CHECK(r1.rfind("need|", 0) == 0);
        std::string r2 = tap(k, 1.4142, 1.4142);   // 点圆 45° 弧附近（避开 Q/P）
        CHECK(r2 == "done|P");
        GeoPoint *P = ptOf(k, "P");
        CHECK(P != nullptr && P->path != nullptr && P->path->label == "c");
        CHECK(std::fabs(P->pathParam - M_PI / 4.0) < 1e-6);   // θ=π/4
        CHECK(std::fabs(sqrt(P->x * P->x + P->y * P->y) - 2.0) < 1e-9);
        // 拖到圆底部 (0,-2)：θ=-π/2（unwrap 取与 prev=π/4 同支最近等价角）
        k.movePoint(P, 0, -2);
        CHECK(std::fabs(P->pathParam + M_PI / 2.0) < 1e-6);
        CHECK(std::fabs(P->x) < 1e-9 && std::fabs(P->y + 2.0) < 1e-9);
        // 继续拖到 (-2, 0)：θ=π 与 prev=-π/2 同支等价角为 -π（跨 ±π 分界
        // 参数连续不跳 2π）
        k.movePoint(P, -2, 0);
        CHECK(std::fabs(P->pathParam + M_PI) < 1e-6);
        CHECK(std::fabs(P->x + 2.0) < 1e-9 && std::fabs(P->y) < 1e-9);
    }

    // ---------- E. 函数图像附着：x 参数 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("g(x)=x^2"));
        CHECK(k.input("P=(1.5, 8)"));
        k.setTool("attachDetach");
        CHECK(tap(k, 1.5, 8).rfind("need|", 0) == 0);
        CHECK(tap(k, 1.5, 2.3) == "done|P");
        GeoPoint *P = ptOf(k, "P");
        CHECK(P != nullptr && P->path != nullptr && P->path->label == "g");
        CHECK(std::fabs(P->x - 1.5) < 1e-9);
        CHECK(std::fabs(P->y - 2.25) < 1e-9);
        CHECK(std::fabs(P->pathParam - 1.5) < 1e-9);
        // 拖动 = x 跟随指针
        k.movePoint(P, -2, 100);
        CHECK(std::fabs(P->x + 2.0) < 1e-9 && std::fabs(P->y - 4.0) < 1e-9);
        // 不等式区域不能附着（R 离 g 曲线要远，防曲线抢先拾取）
        CHECK(k.input("h(x)=y<x"));
        CHECK(k.input("R=(3, 0.2)"));
        k.setTool("attachDetach");
        tap(k, 3, 0.2);
        std::string r = tap(k, 3, 0.2);   // 区域内点 → 不作为路径 → err
        CHECK(r.rfind("err|", 0) == 0);
    }

    // ---------- F. Point 命令（显式参数 / 默认）+ 级联删除 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0,0)"));
        CHECK(k.input("B=(4,0)"));
        CHECK(k.input("s=Segment(A,B)"));
        CHECK(k.input("R=Point(s, 0.75)"));
        GeoPoint *R = ptOf(k, "R");
        CHECK(R != nullptr && !R->isFree && R->path != nullptr);
        CHECK(std::fabs(R->x - 3.0) < 1e-9 && std::fabs(R->y) < 1e-9);
        CHECK(R->definitionText() == "Point(s)");
        CHECK(k.input("R2=Point(s)"));
        GeoPoint *R2 = ptOf(k, "R2");
        CHECK(R2 != nullptr && std::fabs(R2->x - 2.0) < 1e-9);
        // 非路径参数报错
        CHECK(!k.input("R3=Point(A)"));
        // 级联删除：删路径带走依附点
        CHECK(k.deleteByLabel("s"));
        CHECK(ptOf(k, "R") == nullptr && ptOf(k, "R2") == nullptr);
    }

    // ---------- G. XML 往返：coords 重投影恢复参数 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0,0)"));
        CHECK(k.input("B=(4,0)"));
        CHECK(k.input("s=Segment(A,B)"));
        CHECK(k.input("P=(1, 0.3)"));
        k.setTool("attachDetach");
        tap(k, 1, 0.3);
        tap(k, 1, 0.05);   // 附着到 t≈0.25
        GeoPoint *P = ptOf(k, "P");
        CHECK(P != nullptr && P->path != nullptr);
        const double savedParam = P->pathParam;
        const std::string xml = k.getXml();
        Kernel k2;
        frame(k2);
        int skipped = -1;
        CHECK(k2.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoPoint *P2 = k2.lookupPoint("P");
        CHECK(P2 != nullptr && P2->path != nullptr && P2->path->label == "s");
        CHECK(std::fabs(P2->pathParam - savedParam) < 1e-9);
        CHECK(std::fabs(P2->x - 1.0) < 1e-9 && std::fabs(P2->y) < 1e-9);
        CHECK(!P2->isFree && P2->definitionText() == "Point(s)");
        // 载入后仍可拖
        k2.movePoint(P2, 3.5, 2);
        CHECK(std::fabs(P2->x - 3.5) < 1e-9 && std::fabs(P2->y) < 1e-9);
    }

    // ---------- H. 多边形边附着（边序号+t） ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("T1=(0,0)"));
        CHECK(k.input("T2=(4,0)"));
        CHECK(k.input("T3=(0,4)"));
        CHECK(k.input("poly=Polygon(T1,T2,T3)"));
        CHECK(k.input("P=(2, 0.6)"));
        k.setTool("attachDetach");
        tap(k, 2, 0.6);
        CHECK(tap(k, 2, 0.1) == "done|P");
        GeoPoint *P = ptOf(k, "P");
        CHECK(P != nullptr && P->path != nullptr && P->path->label == "poly");
        CHECK(std::fabs(P->x - 2.0) < 1e-9 && std::fabs(P->y) < 1e-9);
        CHECK(std::fabs(P->pathParam - 0.5) < 1e-9);   // 边 0 中点
        // 拖到边 1（T2→T3）中点 (2,2)
        k.movePoint(P, 2, 2);
        CHECK(std::fabs(P->x - 2.0) < 1e-9 && std::fabs(P->y - 2.0) < 1e-9);
        CHECK(std::fabs(P->pathParam - 1.5) < 1e-9);
    }

    // ---------- I. 直线附着（有向距离参数）+ 线段重定义后仍成立 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0,0)"));
        CHECK(k.input("B=(4,0)"));
        CHECK(k.input("l=Line(A,B)"));
        CHECK(k.input("P=(3, 2)"));
        k.setTool("attachDetach");
        tap(k, 3, 2);
        CHECK(tap(k, 3, 0.1) == "done|P");
        GeoPoint *P = ptOf(k, "P");
        CHECK(P != nullptr && P->path != nullptr && P->path->label == "l");
        CHECK(std::fabs(P->x - 3.0) < 1e-9 && std::fabs(P->y) < 1e-9);
        // 参数=沿单位方向 d̂=(-b,a)/|·| 的有向距离：y=0 线 d̂=(-1,0)，
        // param = -3（与 PathPointAt 同一取向，互为逆映射）
        CHECK(std::fabs(P->pathParam + 3.0) < 1e-9);
        // 拖到直线另一侧远处仍在线上
        k.movePoint(P, -7.5, 3);
        CHECK(std::fabs(P->x + 7.5) < 1e-9 && std::fabs(P->y) < 1e-9);
        CHECK(std::fabs(P->pathParam - 7.5) < 1e-9);
        // 重定义依附点为坐标 → 脱离为自由点
        CHECK(k.redefine("P", "(9,9)"));
        CHECK(P->isFree && P->path == nullptr);
        CHECK(std::fabs(P->x - 9.0) < 1e-9);
    }

    // ---------- J. 工具中途改选 + 附着撤销一步 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0,0)"));
        CHECK(k.input("B=(4,0)"));
        CHECK(k.input("s=Segment(A,B)"));
        CHECK(k.input("P1=(1,3)"));
        CHECK(k.input("P2=(3,3)"));
        k.setTool("attachDetach");
        CHECK(tap(k, 1, 3).rfind("need|", 0) == 0);
        int d0 = k.undoDepth();
        // 中途点 P2 → 改选
        std::string r = tap(k, 3, 3);
        CHECK(r.rfind("need|", 0) == 0);
        CHECK(tap(k, 2, 0.1) == "done|P2");
        GeoPoint *P2 = ptOf(k, "P2");
        CHECK(P2 != nullptr && P2->path != nullptr);
        GeoPoint *P1 = ptOf(k, "P1");
        CHECK(P1 != nullptr && P1->isFree && P1->path == nullptr);
        CHECK(k.undoDepth() == d0 + 1);
        k.undoStep();
        // undo=整链 XML 重建，旧指针悬垂，重新 lookup
        GeoPoint *P2b = k.lookupPoint("P2");
        CHECK(P2b != nullptr && P2b->isFree && P2b->path == nullptr);
    }

    printf("test26: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
