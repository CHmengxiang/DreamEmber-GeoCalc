// test17.cpp — 轮C 回归（2026-09-30 重建；原文件在临时目录清理中丢失，
// 按原覆盖范围重写）：size 属性 + 样式条语义（propsOf/setProp、颜色、
// 填充透明度、滑动条属性、XML 往返保样式）。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test17.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test17.exe
#include "kernel/app.h"
#include <cstdio>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

// propsOf 输出里的 k=v 查找
static bool hasProp(const std::string &props, const std::string &kv)
{
    size_t pos = 0;
    while (pos <= props.size()) {
        size_t end = props.find(';', pos);
        if (end == std::string::npos) end = props.size();
        if (props.compare(pos, end - pos, kv) == 0) return true;
        if (end == props.size()) break;
        pos = end + 1;
    }
    return false;
}

int main()
{
    // ---------- A. propsOf 结构：type/vis/sel/color 恒有，size 按类型 ----------
    {
        Kernel k;
        CHECK(k.input("A=(1,2)"));
        CHECK(k.input("B=(3,4)"));
        CHECK(k.input("seg1=Segment(A,B)"));
        CHECK(k.input("n1=5"));
        const std::string pa = k.propsOf("A");
        CHECK(hasProp(pa, "type=point"));
        CHECK(hasProp(pa, "vis=1") && hasProp(pa, "sel=0"));
        CHECK(pa.find("color=") == 0 || pa.find(";color=") != std::string::npos);
        CHECK(hasProp(pa, "size=5"));              // 点默认 pointSize 5
        const std::string pseg = k.propsOf("seg1");
        CHECK(hasProp(pseg, "type=segment"));
        CHECK(hasProp(pseg, "size=5"));            // 线默认 lineThickness 5
        const std::string pn = k.propsOf("n1");
        CHECK(hasProp(pn, "type=numeric"));
        CHECK(pn.find("size=") == std::string::npos);   // 数值无大小属性
        CHECK(k.propsOf("nope").empty());          // 不存在 → 空串
    }

    // ---------- B. setProp size：点 1-9 / 线 1-13 钳制，数值拒绝 ----------
    {
        Kernel k;
        CHECK(k.input("A=(1,2)"));
        CHECK(k.input("B=(3,4)"));
        CHECK(k.input("seg1=Segment(A,B)"));
        CHECK(k.input("n1=5"));
        CHECK(k.setProp("A", "size", "3"));
        CHECK(hasProp(k.propsOf("A"), "size=3"));
        CHECK(k.setProp("A", "size", "99"));
        CHECK(hasProp(k.propsOf("A"), "size=9"));      // 上钳 9
        CHECK(k.setProp("A", "size", "0"));
        CHECK(hasProp(k.propsOf("A"), "size=1"));      // 下钳 1
        CHECK(k.setProp("seg1", "size", "13"));
        CHECK(hasProp(k.propsOf("seg1"), "size=13"));
        CHECK(k.setProp("seg1", "size", "20"));
        CHECK(hasProp(k.propsOf("seg1"), "size=13"));
        CHECK(!k.setProp("n1", "size", "3"));          // 数值无大小
        CHECK(k.setProp("A", "size", "abc"));          // 非数 → 0 → 钳 1（既定行为）
        CHECK(hasProp(k.propsOf("A"), "size=1"));
    }

    // ---------- C. setProp 颜色 / 显隐 ----------
    {
        Kernel k;
        CHECK(k.input("A=(1,2)"));
        CHECK(k.setProp("A", "color", "#FF8000"));
        CHECK(hasProp(k.propsOf("A"), "color=#FF8000"));
        CHECK(k.setProp("A", "color", "#ff8000"));     // 小写可解析
        CHECK(!k.setProp("A", "color", "FF8000"));     // 缺 # 拒绝
        CHECK(!k.setProp("A", "color", "#12 45 78"));
        CHECK(hasProp(k.propsOf("A"), "color=#FF8000"));   // 拒绝后保持原色
        CHECK(k.setProp("A", "vis", "0"));
        CHECK(hasProp(k.propsOf("A"), "vis=0"));
        CHECK(k.setProp("A", "vis", "1"));
        CHECK(hasProp(k.propsOf("A"), "vis=1"));
        CHECK(!k.setProp("nope", "vis", "1"));
    }

    // ---------- D. 填充透明度：多边形/角可调、函数仅限不等式、其余拒绝 ----------
    {
        Kernel k;
        CHECK(k.input("A=(0,0)"));
        CHECK(k.input("B=(2,0)"));
        CHECK(k.input("C=(0,2)"));
        CHECK(k.input("poly1=Polygon(A,B,C)"));
        CHECK(hasProp(k.propsOf("poly1"), "fill=15"));
        CHECK(k.setProp("poly1", "fill", "0.3"));
        CHECK(hasProp(k.propsOf("poly1"), "fill=30"));
        CHECK(k.setProp("poly1", "fill", "5"));        // >1 钳到 1
        CHECK(hasProp(k.propsOf("poly1"), "fill=100"));
        CHECK(k.input("ang1=Angle(A,B,C)"));
        CHECK(hasProp(k.propsOf("ang1"), "fill=12"));
        CHECK(k.setProp("ang1", "fill", "0"));
        CHECK(hasProp(k.propsOf("ang1"), "fill=0"));
        CHECK(k.input("y<x"));
        // 不等式是匿名对象（label 非 "y"）：按 ineqOp 在 geos 里找
        std::string ineqLabel;
        for (const auto &g : k.geos()) {
            GeoFunction *f = dynamic_cast<GeoFunction *>(g.get());
            if (f != nullptr && f->ineqOp != 0) ineqLabel = g->label;
        }
        CHECK(!ineqLabel.empty());
        CHECK(hasProp(k.propsOf(ineqLabel), "fill=10"));
        CHECK(k.setProp(ineqLabel, "fill", "0.25"));
        CHECK(hasProp(k.propsOf(ineqLabel), "fill=25"));
        CHECK(k.input("g(x)=x^2"));
        CHECK(!k.setProp("g", "fill", "0.5"));         // 普通函数无区域填充
        CHECK(!k.setProp("A", "fill", "0.5"));         // 点无填充
    }

    // ---------- E. 滑动条属性：min/max/step/anim ----------
    {
        Kernel k;
        CHECK(k.input("s1=Slider(0,10,1)"));
        const std::string p1 = k.propsOf("s1");
        // propsOf 报底层类型 numeric（algebraText 行类型才是 slider）
        CHECK(hasProp(p1, "type=numeric"));
        CHECK(hasProp(p1, "min=0") && hasProp(p1, "max=10") && hasProp(p1, "step=1"));
        CHECK(hasProp(p1, "anim=0"));
        CHECK(k.setProp("s1", "min", "-5"));
        CHECK(k.setProp("s1", "max", "20"));
        CHECK(k.setProp("s1", "step", "0.5"));
        CHECK(hasProp(k.propsOf("s1"), "min=-5"));
        CHECK(hasProp(k.propsOf("s1"), "max=20"));
        CHECK(hasProp(k.propsOf("s1"), "step=0.5"));
        CHECK(k.animateToggle("s1"));               // toggle 后返回新状态 true
        CHECK(hasProp(k.propsOf("s1"), "anim=1"));
        CHECK(!k.animateToggle("s1"));              // 再 toggle 返回 false
        CHECK(hasProp(k.propsOf("s1"), "anim=0"));
    }

    // ---------- F. XML 往返保样式（size/color/fill/滑杆范围） ----------
    {
        Kernel k;
        CHECK(k.input("A=(1,2)"));
        CHECK(k.setProp("A", "size", "7"));
        CHECK(k.setProp("A", "color", "#112233"));
        CHECK(k.input("B=(2,3)"));
        CHECK(k.input("seg1=Segment(A,B)"));
        CHECK(k.setProp("seg1", "size", "9"));
        CHECK(k.input("C=(0,2)"));
        CHECK(k.input("poly1=Polygon(A,B,C)"));
        CHECK(k.setProp("poly1", "fill", "0.4"));
        CHECK(k.input("s1=Slider(1,9,2)"));
        const std::string xml = k.getXml();
        Kernel k2;
        int skipped = -1;
        CHECK(k2.setXml(xml, &skipped));
        CHECK(skipped == 0);
        CHECK(hasProp(k2.propsOf("A"), "size=7"));
        CHECK(hasProp(k2.propsOf("A"), "color=#112233"));
        CHECK(hasProp(k2.propsOf("seg1"), "size=9"));
        CHECK(hasProp(k2.propsOf("poly1"), "fill=40"));
        CHECK(hasProp(k2.propsOf("s1"), "min=1"));
        CHECK(hasProp(k2.propsOf("s1"), "max=9"));
        CHECK(hasProp(k2.propsOf("s1"), "step=2"));
        // 真 GGB 语义：手动改样式是独立撤销步（轮C 样式条 pushUndo+setProp）
        k.pushUndo();
        k.setProp("A", "size", "1");
        CHECK(k.undoStep());
        CHECK(hasProp(k.propsOf("A"), "size=7"));
    }

    printf("test17: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
