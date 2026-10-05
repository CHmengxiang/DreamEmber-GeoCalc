// test22.cpp — 第二十五包 host 测试：视图尺寸变化语义（真机反馈⑦——
// PC UI 宽屏启动画布先全宽居中，左侧视图栏+停靠面板随后加载把画布推窄，
// 原点偏到一侧、须手动重置视图）。
// setCanvasSize 新语义：宽度变化保持「视野水平中心的世界点」不动
//（xZero += Δw/2，启动逐帧累积恰好把原点送回新中心）；高度有意不补
//（窄屏抽屉把手拖高逐帧改高度，补了内容会跟着把手滑）。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test22.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test22.exe
#include "kernel/app.h"
#include <cstdio>
#include <cmath>
#include <vector>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);   // 崩溃前也要吐出 FAIL 行

    // ---------- A. 首次定尺寸：原点居中（原语义不变） ----------
    {
        EuclidianView v;
        v.setCanvasSize(600, 400);
        CHECK(v.width == 600 && v.height == 400);
        CHECK(std::fabs(v.xZero - 300.0) < 1e-9);
        CHECK(std::fabs(v.yZero - 200.0) < 1e-9);
        CHECK(std::fabs(v.px(0.0) - 300.0) < 1e-9);
        CHECK(std::fabs(v.py(0.0) - 200.0) < 1e-9);
    }

    // ---------- B. 宽度变化：视野水平中心世界点保持（原点回新中心） ----------
    {
        EuclidianView v;
        v.setCanvasSize(760, 400);   // 启动帧：全宽
        v.setCanvasSize(460, 400);   // 视图栏+停靠面板加载 → 画布推窄 300
        CHECK(std::fabs(v.xZero - 230.0) < 1e-9);    // (760-300)/2 = 新中心
        CHECK(std::fabs(v.yZero - 200.0) < 1e-9);    // 高度不动
        CHECK(std::fabs(v.px(0.0) - 230.0) < 1e-9);  // 原点 = 新画布中心
        // 尺度不被尺寸变化触碰
        CHECK(std::fabs(v.xscale - 50.0) < 1e-9 && std::fabs(v.yscale - 50.0) < 1e-9);
        // 反方向（面板收起）同样居中
        v.setCanvasSize(760, 400);
        CHECK(std::fabs(v.xZero - 380.0) < 1e-9);
        CHECK(std::fabs(v.px(0.0) - 380.0) < 1e-9);
    }

    // ---------- C. 高度变化不补（抽屉拖高逐帧改高度，内容不跟着滑） ----------
    {
        EuclidianView v;
        v.setCanvasSize(600, 400);
        v.setCanvasSize(600, 300);
        CHECK(std::fabs(v.xZero - 300.0) < 1e-9);
        CHECK(std::fabs(v.yZero - 200.0) < 1e-9);   // yZero 有意保持
        CHECK(v.height == 300);
    }

    // ---------- D. 平移/缩放后：宽度补偿保「当前视野中心」不跳 ----------
    {
        EuclidianView v;
        v.setCanvasSize(600, 400);
        v.panBy(-120.0, 0.0);            // 用户左移视野 → 中心世界点 x=+2.4
        CHECK(std::fabs(v.wx(300.0) - 2.4) < 1e-9);
        v.setCanvasSize(460, 400);       // 面板加载推窄
        CHECK(std::fabs(v.xZero - 110.0) < 1e-9);     // 180 + (460-600)/2
        // 旧视野中心的世界点 (2.4, 0) 仍在新画布中心像素处
        CHECK(std::fabs(v.px(2.4) - 230.0) < 1e-9);
        v.zoomAt(230.0, 200.0, 2.0);     // 以中心锚放大
        CHECK(std::fabs(v.xscale - 100.0) < 1e-9);
        v.setCanvasSize(600, 400);       // 再变宽：锚点世界点仍不动
        CHECK(std::fabs(v.px(2.4) - 300.0) < 1e-9);
    }

    // ---------- E. Kernel 渲染路径回归（尺寸变化后重渲染不崩、语义保持） ----------
    {
        Kernel k;
        k.view.setCanvasSize(600, 400);
        CHECK(k.input("A=(1, 2)"));
        std::vector<DrawCmd> out;
        k.render(out);
        k.view.setCanvasSize(460, 400);
        out.clear();
        k.render(out);
        // x 补偿（原点=新中心 230）、y 保持（yZero=200）
        CHECK(std::fabs(k.view.px(1.0) - (230.0 + 50.0)) < 1e-9);
        CHECK(std::fabs(k.view.py(2.0) - (200.0 - 100.0)) < 1e-9);
    }

    printf("test22: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
