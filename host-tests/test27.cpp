// test27.cpp — 第四十八包 host 测试：线裁剪（Cohen-Sutherland，像素空间）。
// 覆盖：①界内线段恒等裁剪（坐标原样）②部分在外的线段裁到视界 ③整段在外
// 省略（不再下发画布）④近竖直线端点不再爆量级（主轴参数化+裁剪）⑤起点远
// 离屏幕的射线仍能延伸到视口（旧版固定延伸长度够不着=整条消失）⑥向量轴杆
// 裁剪、箭头仍按原尖端。画布 600×400、原点 (300,200)、50px/单位：
//   PX(wx)=300+50wx ; PY(wy)=200−50wy ; 视界矩形裁剪域 [−5,605]×[−5,405]
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test27.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test27.exe
#include "kernel/app.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

static double PX(double wx) { return 300.0 + 50.0 * wx; }
static double PY(double wy) { return 200.0 - 50.0 * wy; }

static std::vector<DrawCmd> render(Kernel &k)
{
    std::vector<DrawCmd> out;
    k.view.setCanvasSize(600, 400);
    k.render(out);
    return out;
}

// 找与给定像素线段（±0.5px）匹配的 Segment 命令
static const DrawCmd *findSeg(const std::vector<DrawCmd> &fr,
                              double x1, double y1, double x2, double y2)
{
    for (const auto &c : fr) {
        if (c.op != DrawOp::Segment || c.pts.size() < 4) continue;
        if (std::fabs(c.pts[0] - x1) < 0.5 && std::fabs(c.pts[1] - y1) < 0.5
            && std::fabs(c.pts[2] - x2) < 0.5 && std::fabs(c.pts[3] - y2) < 0.5)
            return &c;
        // 方向无关（裁剪端点顺序可能对调）
        if (std::fabs(c.pts[0] - x2) < 0.5 && std::fabs(c.pts[1] - y2) < 0.5
            && std::fabs(c.pts[2] - x1) < 0.5 && std::fabs(c.pts[3] - y1) < 0.5)
            return &c;
    }
    return nullptr;
}

// 所有 Segment 命令的端点是否都落在裁剪域内（±0.6px 容差）
static bool allSegsInBounds(const std::vector<DrawCmd> &fr)
{
    for (const auto &c : fr) {
        if (c.op != DrawOp::Segment || c.pts.size() < 4) continue;
        for (int i = 0; i < 4; i += 2) {
            if (c.pts[i] < -5.6 || c.pts[i] > 605.6
                || c.pts[i + 1] < -5.6 || c.pts[i + 1] > 405.6)
                return false;
        }
    }
    return true;
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    // ---------- A. 界内线段恒等裁剪 ----------
    {
        Kernel k;
        CHECK(k.input("A=(1, 2)"));
        CHECK(k.input("B=(4, 2)"));
        CHECK(k.input("s=Segment(A, B)"));
        std::vector<DrawCmd> fr = render(k);
        CHECK(findSeg(fr, PX(1), PY(2), PX(4), PY(2)) != nullptr);
        CHECK(allSegsInBounds(fr));
    }

    // ---------- B. 部分在外：裁到视界边界 ----------
    {
        Kernel k;
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(20, 0)"));
        CHECK(k.input("s=Segment(A, B)"));
        std::vector<DrawCmd> fr = render(k);
        // 终点 px(20)=1300 裁到右界 605，y 不变
        CHECK(findSeg(fr, PX(0), PY(0), 605.0, PY(0)) != nullptr);
        CHECK(allSegsInBounds(fr));
    }

    // ---------- C. 整段在外：省略（不下发画布）----------
    {
        Kernel k;
        CHECK(k.input("A=(100, 0)"));
        CHECK(k.input("B=(101, 0)"));
        CHECK(k.input("s=Segment(A, B)"));
        std::vector<DrawCmd> fr = render(k);
        CHECK(findSeg(fr, PX(100), PY(0), PX(101), PY(0)) == nullptr);
        CHECK(allSegsInBounds(fr));
    }

    // ---------- D. 近竖直线：主轴参数化 + 裁剪，端点不爆量级 ----------
    {
        Kernel k;
        CHECK(k.input("P=(2, 0)"));
        CHECK(k.input("Q=(2.000001, 1)"));
        // 斜率 1e6 的直线（旧版按 x 取端点 → py ±1e9px 量级）
        CHECK(k.input("l=Line(P, Q)"));
        std::vector<DrawCmd> fr = render(k);
        CHECK(allSegsInBounds(fr));
        // 竖直线 x=2 恒等：端点恰在上下界
        CHECK(k.input("R=(2, 1)"));
        CHECK(k.input("v=Line(P, R)"));
        fr = render(k);
        CHECK(findSeg(fr, PX(2), -5.0, PX(2), 405.0) != nullptr);
        // 界外竖直线：省略
        CHECK(k.input("S=(100, 1)"));
        CHECK(k.input("o=Line(P, S)"));
        fr = render(k);
        CHECK(findSeg(fr, PX(100), -5.0, PX(100), 405.0) == nullptr);
        CHECK(allSegsInBounds(fr));
    }

    // ---------- E. 射线：起点远离屏幕仍达视口（旧版整条消失）----------
    {
        Kernel k;
        CHECK(k.input("A=(100, 0)"));
        CHECK(k.input("B=(99, 0)"));
        CHECK(k.input("r=Ray(A, B)"));
        std::vector<DrawCmd> fr = render(k);
        // 可见段 = 射线穿过视界的部分 [−5, 605]，y=200
        CHECK(findSeg(fr, -5.0, PY(0), 605.0, PY(0)) != nullptr);
        CHECK(allSegsInBounds(fr));
        // 普通射线（起点在界内）仍正确：起点像素原样、终点裁到边界
        CHECK(k.input("C=(1, 0)"));
        CHECK(k.input("D=(5, 0)"));
        CHECK(k.input("r2=Ray(C, D)"));
        fr = render(k);
        CHECK(findSeg(fr, PX(1), PY(0), 605.0, PY(0)) != nullptr);
    }

    // ---------- F. 向量：轴杆裁剪，箭头仍按原尖端 ----------
    {
        Kernel k;
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(20, 0)"));
        CHECK(k.input("v=Vector(A, B)"));
        std::vector<DrawCmd> fr = render(k);
        // 轴杆裁到右界
        CHECK(findSeg(fr, PX(0), PY(0), 605.0, PY(0)) != nullptr);
        // 箭头短划仍挂在原尖端 px(20)=1300（画布自裁）
        bool arrowAtTip = false;
        for (const auto &c : fr) {
            if (c.op == DrawOp::MultiSeg && c.pts.size() >= 8
                && std::fabs(c.pts[0] - PX(20)) < 0.5
                && std::fabs(c.pts[1] - PY(0)) < 0.5)
                arrowAtTip = true;
        }
        CHECK(arrowAtTip);
        CHECK(allSegsInBounds(fr));
    }

    // ---------- G. 界外对象裁剪后选中光环同步省略/裁剪（无越界命令）----------
    {
        Kernel k;
        CHECK(k.input("A=(100, 0)"));
        CHECK(k.input("B=(101, 0)"));
        CHECK(k.input("s=Segment(A, B)"));
        k.setSelected("s");
        std::vector<DrawCmd> fr = render(k);
        CHECK(allSegsInBounds(fr));
    }

    printf("test27: %d checks, %d failed\n", g_n, g_fail);
    return g_fail > 0 ? 1 : 0;
}
