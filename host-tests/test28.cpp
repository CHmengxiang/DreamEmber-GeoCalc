// test28.cpp — 1.0.6.1 host 测试：官方 .ggb 兼容（kernel 段 / 滑条 / 文本 /
// 布尔 / 自由角 element 重建）。
// 覆盖：①<kernel><angleUnit val="radian"> 弧度数值/角换算成内部度制
// ②官方滑条形态 <element type="numeric"><value/><slider min max step x y/>
// <animation playing speed>：滑条重建+范围/步长吸附+动画即刻开跑
// ③自由文本 <element type="text"><caption val=".."/><startPoint x y> 重建
// ④布尔 true/false → 1/0 ⑤依赖文本（无 caption）按跳过计数
// ⑥自产 XML 往返不受影响（回归护栏）
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     ../../../../host-tests/test28.cpp kernel/app.cpp kernel/expr.cpp
//     kernel/geos.cpp kernel/view.cpp kernel/ggbfile.cpp cas/cas_wrapper.cpp
//     -lz -o ../../../../host-tests/test28.exe
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

int main()
{
    // ① kernel 段：radian 文件 → 数值换算成度（π/2 rad = 90°）
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\">"
            "<euclidianView><coordSystem xZero=\"0\" yZero=\"0\" scale=\"50\"/></euclidianView>"
            "<kernel><decimals val=\"2\"/><angleUnit val=\"radian\"/></kernel>"
            "<construction>"
            "<element type=\"numeric\" label=\"w\"><value val=\"1.5707963267948966\"/></element>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoNumeric *w = dynamic_cast<GeoNumeric *>(find(k, "w"));
        CHECK(w != nullptr);
        if (w) CHECK(fabs(w->value - 90.0) < 1e-9);
        CHECK(k.angleRadian);
    }
    // ② degree 文件（无 kernel 段）：数值原样
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\"><construction>"
            "<element type=\"numeric\" label=\"q\"><value val=\"42.5\"/></element>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoNumeric *q = dynamic_cast<GeoNumeric *>(find(k, "q"));
        if (q) CHECK(fabs(q->value - 42.5) < 1e-12);
        CHECK(!k.angleRadian);
    }
    // ③ 官方滑条形态：numeric + slider(min/max/step/x/y) + animation(playing)
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\"><construction>"
            "<element type=\"numeric\" label=\"n\">"
            "<value val=\"70\"/>"
            "<slider min=\"1\" max=\"147\" absoluteScreenLocation=\"true\" width=\"200\" "
            "x=\"129\" y=\"81\" fixed=\"false\" horizontal=\"true\" showAlgebra=\"true\"/>"
            "<animation step=\"1\" speed=\"2\" type=\"0\" playing=\"true\"/>"
            "</element>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoNumeric *n = dynamic_cast<GeoNumeric *>(find(k, "n"));
        CHECK(n != nullptr);
        if (n) {
            CHECK(n->isSlider);
            CHECK(fabs(n->smin - 1.0) < 1e-12);
            CHECK(fabs(n->smax - 147.0) < 1e-12);
            CHECK(fabs(n->sx - 129.0) < 1e-12);
            CHECK(fabs(n->sy - 81.0) < 1e-12);
            CHECK(fabs(n->value - 70.0) < 1e-12);
            CHECK(n->animating);
            CHECK(fabs(n->speed - 2.0) < 1e-12);
        }
        // 步长吸附：value 落在 min + k*step 网格上
        CHECK(k.anyAnimating());
    }
    // ④ 滑条步长吸附：值 70.4、step 0.5 → 70.5
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\"><construction>"
            "<element type=\"numeric\" label=\"s\">"
            "<value val=\"70.4\"/>"
            "<slider min=\"0\" max=\"100\" step=\"0.5\" x=\"10\" y=\"20\"/>"
            "</element>"
            "</construction></geogebra>";
        CHECK(k.setXml(xml, nullptr));
        GeoNumeric *s = dynamic_cast<GeoNumeric *>(find(k, "s"));
        if (s) CHECK(fabs(s->value - 70.5) < 1e-9);
    }
    // ⑤ 自由文本：caption + startPoint 世界坐标
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\"><construction>"
            "<element type=\"text\" label=\"text1\">"
            "<show object=\"true\" label=\"false\"/>"
            "<caption val=\"Hello 世界\"/>"
            "<startPoint x=\"2.5\" y=\"3.0\"/>"
            "</element>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoText *t = dynamic_cast<GeoText *>(find(k, "text1"));
        CHECK(t != nullptr);
        if (t) {
            CHECK(t->text == "Hello 世界");
            CHECK(fabs(t->x - 2.5) < 1e-12);
            CHECK(fabs(t->y - 3.0) < 1e-12);
            CHECK(t->visible);
        }
    }
    // ⑥ 布尔：true/false → 1/0
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\"><construction>"
            "<element type=\"boolean\" label=\"flag\"><value val=\"true\"/></element>"
            "<element type=\"boolean\" label=\"off\"><value val=\"false\"/></element>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoNumeric *f = dynamic_cast<GeoNumeric *>(find(k, "flag"));
        GeoNumeric *o = dynamic_cast<GeoNumeric *>(find(k, "off"));
        if (f) CHECK(fabs(f->value - 1.0) < 1e-12);
        if (o) CHECK(fabs(o->value - 0.0) < 1e-12);
    }
    // ⑦ 依赖文本（无 caption）跳过计数
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\"><construction>"
            "<element type=\"text\" label=\"text2\">"
            "<startPoint exp=\"Midpoint[G, H]\"/>"
            "</element>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 1);
        CHECK(find(k, "text2") == nullptr);
    }
    // ⑧ 自由角 element（度制文件）：值原样落位
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\"><construction>"
            "<element type=\"angle\" label=\"α1\"><value val=\"45.0\"/></element>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoNumeric *a = dynamic_cast<GeoNumeric *>(find(k, "α1"));
        if (a) CHECK(fabs(a->value - 45.0) < 1e-12);
    }
    // ⑨ 弧度文件的自由角：π/4 → 45°
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\">"
            "<kernel><angleUnit val=\"radian\"/></kernel>"
            "<construction>"
            "<element type=\"angle\" label=\"beta\"><value val=\"0.7853981633974483\"/></element>"
            "</construction></geogebra>";
        CHECK(k.setXml(xml, nullptr));
        GeoNumeric *b = dynamic_cast<GeoNumeric *>(find(k, "beta"));
        if (b) CHECK(fabs(b->value - 45.0) < 1e-9);
    }
    // ⑩ 回归护栏：自产 XML（滑条+文本+点）往返载入不变
    {
        Kernel k;
        CHECK(k.input("a=Slider(-5, 5, 0.1)"));
        CHECK(k.input("t1=Text(\"测试\", (2, 3))"));
        CHECK(k.input("P=(1, 2)"));
        const std::string xml = k.getXml();
        Kernel k2;
        int skipped = -1;
        CHECK(k2.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoNumeric *a2 = dynamic_cast<GeoNumeric *>(find(k2, "a"));
        GeoText *t2 = dynamic_cast<GeoText *>(find(k2, "t1"));
        GeoPoint *p2 = dynamic_cast<GeoPoint *>(find(k2, "P"));
        CHECK(a2 != nullptr && a2->isSlider);
        if (a2) CHECK(fabs(a2->smin + 5.0) < 1e-12);
        CHECK(t2 != nullptr && t2->text == "测试");
        CHECK(p2 != nullptr && fabs(p2->x - 1.0) < 1e-12);
    }
    // ⑪ 官方真实样例：circles.ggb 的结构（point+Circle command+conic element）
    // conic 类型不重建 → 跳过；命令重放 Circle 需要 conic 支持——本内核
    // Circle(A,B) 产物为 GeoCircle（type=conic 的 element 落在已有对象上）
    {
        Kernel k;
        const char *xml =
            "<geogebra format=\"5.0\" version=\"5.0.478.0\" app=\"classic\">"
            "<euclidianView><coordSystem xZero=\"204.99\" yZero=\"343.0\" "
            "scale=\"50.0\" yscale=\"50.0\"/></euclidianView>"
            "<kernel><continuous val=\"false\"/><decimals val=\"2\"/>"
            "<angleUnit val=\"degree\"/></kernel>"
            "<construction title=\"\" author=\"\" date=\"\">"
            "<element type=\"point\" label=\"A\">"
            "<show object=\"true\" label=\"true\"/>"
            "<coords x=\"1.54\" y=\"1.78\" z=\"1.0\"/></element>"
            "<element type=\"point\" label=\"B\">"
            "<show object=\"true\" label=\"true\"/>"
            "<coords x=\"1.46\" y=\"4.06\" z=\"1.0\"/></element>"
            "<command name=\"Circle\"><input a0=\"A\" a1=\"B\"/><output a0=\"c\"/></command>"
            "</construction></geogebra>";
        int skipped = -1;
        CHECK(k.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoPoint *A = dynamic_cast<GeoPoint *>(find(k, "A"));
        GeoPoint *B = dynamic_cast<GeoPoint *>(find(k, "B"));
        CHECK(A != nullptr && B != nullptr);
        if (A) { CHECK(fabs(A->x - 1.54) < 1e-12); CHECK(fabs(A->y - 1.78) < 1e-12); }
        GeoCircle *c = dynamic_cast<GeoCircle *>(find(k, "c"));
        CHECK(c != nullptr);
        if (c && A && B) {
            // 半径 = |AB| 距离
            const double d = std::sqrt((A->x - B->x) * (A->x - B->x)
                + (A->y - B->y) * (A->y - B->y));
            CHECK(fabs(c->radius - d) < 1e-9);
        }
    }
    printf("test28: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
