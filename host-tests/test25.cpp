// test25.cpp — 第二十八包 host 测试：滑动条四参数原子应用 setSliderParams
// （二十八包⑤，真机反馈：滑动条缺独立编辑逻辑）。覆盖：吸附+范围落位、
// 钳位、连续步长（0/负 = 连续）、max<=min 校验失败原值不动、非滑动条拒绝、
// 级联重算（依赖数值跟随）、单键 setProp 兼容仍在、algebraText 尾列同步、
// XML 往返参数持久。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test25.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test25.exe
#include "kernel/app.h"
#include <cstdio>
#include <cmath>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

static GeoNumeric *numOf(Kernel &k, const std::string &label)
{
    return dynamic_cast<GeoNumeric *>(k.lookup(label));
}

static void frame(Kernel &k)
{
    std::vector<DrawCmd> out;
    k.view.setCanvasSize(600, 400);
    k.render(out);
}

int main()
{
    // ---------- A. 原子应用：范围/步长落位 + 值吸附 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("b=Slider(-5,5,0.1)"));
        GeoNumeric *b = numOf(k, "b");
        CHECK(b != nullptr && b->isSlider);
        CHECK(k.setSliderParams(b, -10, 10, 0.5, 3.7));
        // 吸附：(3.7−(−10))/0.5 = 27.4 → 27 → −10+13.5 = 3.5
        CHECK(std::fabs(b->smin - (-10.0)) < 1e-12);
        CHECK(std::fabs(b->smax - 10.0) < 1e-12);
        CHECK(std::fabs(b->sstep - 0.5) < 1e-12);
        CHECK(std::fabs(b->value - 3.5) < 1e-12);
    }

    // ---------- B. 值钳位 + 连续步长（0 与负都视为连续） ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("b=Slider(0,5,0.01)"));
        GeoNumeric *b = numOf(k, "b");
        CHECK(k.setSliderParams(b, 0, 1, 0, 99));
        CHECK(std::fabs(b->value - 1.0) < 1e-12);   // 钳到新上限
        CHECK(k.setSliderParams(b, 0, 10, -1, 3.333));
        CHECK(b->sstep == 0.0);                      // 负步长 = 连续
        CHECK(std::fabs(b->value - 3.333) < 1e-12);  // 连续不吸附
    }

    // ---------- C. 校验失败：max<=min 拒绝 + 参数原值不动 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("b=Slider(0,5,0.1)"));
        GeoNumeric *b = numOf(k, "b");
        b->value = 2.5;
        CHECK(!k.setSliderParams(b, 5, 3, 1, 0));
        CHECK(k.lastError.find("上限") != std::string::npos);
        CHECK(std::fabs(b->smin - 0.0) < 1e-12 && std::fabs(b->smax - 5.0) < 1e-12);
        CHECK(std::fabs(b->value - 2.5) < 1e-12);   // 失败全不动
    }

    // ---------- D. 非滑动条 / 未知对象拒绝 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("c=7"));
        GeoNumeric *c = numOf(k, "c");
        CHECK(c != nullptr && !c->isSlider);
        CHECK(!k.setSliderParams(c, 0, 1, 0.1, 0.5));
        CHECK(k.setSliderParams(nullptr, 0, 1, 0.1, 0.5) == false);
    }

    // ---------- E. 级联重算：依赖数值跟随新值 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("b=Slider(0,5,0.1)"));
        CHECK(k.input("d=2*b"));
        GeoNumeric *b = numOf(k, "b");
        GeoNumeric *d = numOf(k, "d");
        CHECK(k.setSliderParams(b, 0, 10, 0.5, 3.7));
        CHECK(std::fabs(b->value - 3.5) < 1e-12);
        CHECK(d != nullptr && std::fabs(d->value - 7.0) < 1e-12);   // d=2b 跟随
    }

    // ---------- F. 单键 setProp 兼容仍在（原逐键路径不回归） ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("b=Slider(0,5,0.1)"));
        CHECK(k.setProp("b", "min", "-2"));
        CHECK(k.setProp("b", "max", "8"));
        CHECK(k.setProp("b", "step", "0.5"));
        CHECK(k.setProp("b", "value", "3.2"));
        GeoNumeric *b = numOf(k, "b");
        CHECK(std::fabs(b->value - 3.0) < 1e-12);   // 3.2 按 0.5 吸附 → 3.0
    }

    // ---------- G. algebraText 尾列同步 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("b=Slider(0,5,0.1)"));
        CHECK(k.setSliderParams(numOf(k, "b"), -3, 4, 0.25, 1.3));
        const std::string alg = k.algebraText();
        bool ok = false;
        size_t p = alg.find("b|slider|");
        if (p != std::string::npos) {
            const size_t e = alg.find('\n', p);
            const std::string line = alg.substr(p, e - p);
            ok = line.find("min=-3") != std::string::npos
                && line.find("max=4") != std::string::npos
                && line.find("step=0.25") != std::string::npos;
        }
        CHECK(ok);
    }

    // ---------- H. XML 往返：新参数持久 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("b=Slider(0,5,0.1)"));
        CHECK(k.setSliderParams(numOf(k, "b"), -3, 4, 0.25, 1.3));
        Kernel k2;
        int skipped = -1;
        CHECK(k2.setXml(k.getXml(), &skipped));
        CHECK(skipped == 0);
        GeoNumeric *b2 = numOf(k2, "b");
        CHECK(b2 != nullptr && b2->isSlider);
        CHECK(std::fabs(b2->smin - (-3.0)) < 1e-12 && std::fabs(b2->smax - 4.0) < 1e-12);
        CHECK(std::fabs(b2->sstep - 0.25) < 1e-12);
        CHECK(std::fabs(b2->value - 1.25) < 1e-12);   // 1.3 按 0.25 吸附 → 1.25
    }

    printf("test25: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
