// test24.cpp — 第二十七包 host 测试：滑动条工具（点画布建 Slider）+ 滑动条
// 驱动参数联动（rotate 弹窗参数引用滑动条 → setSliderValue/dragSliderTo 跟随）
// + 多滑动条自增标签 + 撤销 + XML 往返（<slider> 元素 + 表达式重放）。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test24.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test24.exe
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

// toolTap 前先建画布：setCanvasSize 把世界原点钉在画布中心（300,200），
// 50px/单位、y 向上 → 世界 (wx,wy) 的像素 = (300+50wx, 200−50wy)
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
static std::string donePart(const std::string &r, int i)
{
    if (r.rfind("done|", 0) != 0) return "";
    std::string body = r.substr(5);
    int cur = 0;
    size_t pos = 0;
    while (cur < i) {
        size_t c = body.find(',', pos);
        if (c == std::string::npos) return "";
        pos = c + 1;
        ++cur;
    }
    size_t c = body.find(',', pos);
    std::string part = c == std::string::npos ? body.substr(pos) : body.substr(pos, c - pos);
    size_t a = part.find_first_not_of(' ');
    if (a == std::string::npos) return "";
    size_t b = part.find_last_not_of(' ');
    return part.substr(a, b - a + 1);
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    // ---------- A. 滑动条工具：点画布即建，落点/范围/步长/值，工具保持激活 ----------
    {
        Kernel k;
        frame(k);
        k.setTool("slider");
        std::string r1 = k.toolTap(1, 2, PX(1), PY(2));
        std::string s1 = doneLabel(r1);
        CHECK(!s1.empty());
        GeoNumeric *n1 = numOf(k, s1);
        CHECK(n1 != nullptr && n1->isSlider);
        CHECK(std::fabs(n1->smin - (-5.0)) < 1e-12 && std::fabs(n1->smax - 5.0) < 1e-12);
        CHECK(std::fabs(n1->sstep - 0.1) < 1e-12);
        CHECK(std::fabs(n1->value - (-5.0)) < 1e-12);   // 初值 = 下限
        CHECK(std::fabs(n1->sx - PX(1)) < 1e-9 && std::fabs(n1->sy - PY(2)) < 1e-9);
        CHECK(n1->visible);   // 滑动条对象可见（普通数值默认不可见）
        // 工具保持激活：再点一颗（标签自增，不冲突）
        std::string r2 = k.toolTap(-2, -1, PX(-2), PY(-1));
        std::string s2 = doneLabel(r2);
        CHECK(!s2.empty() && s2 != s1);
        CHECK(numOf(k, s2) != nullptr && numOf(k, s2)->isSlider);
        // 撤销：一步撤掉第二颗
        int depth = k.undoDepth();
        CHECK(k.undoStep());
        CHECK(k.undoDepth() == depth - 1);
        CHECK(k.lookup(s2) == nullptr);
        CHECK(numOf(k, s1) != nullptr);   // 第一颗保留
    }

    // ---------- B. 滑动条 → 旋转弹窗参数联动（setSliderValue / dragSliderTo） ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(3, 0)"));
        CHECK(k.input("C=(0, 0)"));
        k.setTool("slider");
        std::string aLabel = doneLabel(k.toolTap(-2, 2, PX(-2), PY(2)));
        CHECK(!aLabel.empty());
        k.setTool("rotate");
        CHECK(k.toolTap(3, 0, PX(3), PY(0)).rfind("need|") == 0);
        CHECK(k.toolTap(0, 0, PX(0), PY(0)).rfind("ask|") == 0);
        // 参数填滑动条名（原样进命令表达式，GGB 动态参数同款）
        std::string r = k.toolParam(aLabel);
        CHECK(r.rfind("done|", 0) == 0);
        std::string img = donePart(r, 0);
        GeoPoint *imgP = dynamic_cast<GeoPoint *>(k.lookup(img));
        CHECK(imgP != nullptr);
        // 初值 −5° → 像 = (3cos(−5°), 3sin(−5°))
        GeoNumeric *sl = numOf(k, aLabel);
        CHECK(std::fabs(imgP->x - 3.0 * std::cos(-5.0 * M_PI / 180.0)) < 1e-9);
        CHECK(std::fabs(imgP->y - 3.0 * std::sin(-5.0 * M_PI / 180.0)) < 1e-9);
        // 设值 3°（范围内；90 越界会被钳到上限 5——滑动条语义如此）→ 像转 3°
        k.setSliderValue(sl, 3);
        CHECK(std::fabs(imgP->x - 3.0 * std::cos(3.0 * M_PI / 180.0)) < 1e-9);
        CHECK(std::fabs(imgP->y - 3.0 * std::sin(3.0 * M_PI / 180.0)) < 1e-9);
        // 拖拽手柄到轨道中点 → 0° → 像回到原位（dragSliderTo 吸附步长 0.1）
        k.dragSliderTo(sl, sl->sx + 70.0);
        CHECK(std::fabs(sl->value - 0.0) < 1e-9);
        CHECK(std::fabs(imgP->x - 3.0) < 1e-9 && std::fabs(imgP->y - 0.0) < 1e-9);
        // 代数区行类型 = slider（宿主滑杆控件挂这行）
        bool hasSliderRow = false;
        std::vector<AlgebraRow> rows;
        k.algebraRows(rows);
        for (const auto &row : rows) {
            if (row.label == aLabel) hasSliderRow = row.type == "slider";
        }
        CHECK(hasSliderRow);
    }

    // ---------- C. XML 往返：<slider> 属性 + 表达式重放无跳过 ----------
    {
        Kernel k;
        frame(k);
        k.setTool("slider");
        std::string s1 = doneLabel(k.toolTap(0, 1, PX(0), PY(1)));
        CHECK(!s1.empty());
        std::string xml = k.getXml();
        CHECK(xml.find("<slider") != std::string::npos);
        CHECK(xml.find("Slider(") != std::string::npos);   // 重放走表达式重建
        Kernel k2;
        int skipped = -1;
        CHECK(k2.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoNumeric *n2 = numOf(k2, s1);
        CHECK(n2 != nullptr && n2->isSlider);
        CHECK(std::fabs(n2->smin - (-5.0)) < 1e-12 && std::fabs(n2->smax - 5.0) < 1e-12);
        CHECK(std::fabs(n2->sstep - 0.1) < 1e-12);
    }

    printf("test24: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
