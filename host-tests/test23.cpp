// test23.cpp — 第二十六包 host 测试：工具第二批（需参数对话框）：
// rotate / dilate / circleRadius / segLength / angleSize 的工具状态机流 +
// toolParam 参数确认协议（ask| → done|/err|；失败保留 ask 态、取消=setTool
// 复位）+ 动态参数（数值对象引用）+ 悬空/非法参数防御 + .ggb 重放。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test23.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test23.exe
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

// done| 协议取首个标签（无 done 返回空串）
static std::string doneLabel(const std::string &r)
{
    return r.rfind("done|", 0) == 0 ? r.substr(5) : "";
}
// done| 的逗号分隔第 i 段（段内去首尾空格——内核多标签用 ", " 连接）
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
    setvbuf(stdout, nullptr, _IONBF, 0);   // 崩溃前也要吐出 FAIL 行

    // ---------- A. 旋转：点对象 + 字面角度 + ask 态防御/空参保留 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(2, 0)"));
        CHECK(k.input("C=(0, 0)"));
        k.setTool("rotate");
        // 第一步：拾对象（点也可作变换对象）
        std::string r1 = k.toolTap(2, 0, PX(2), PY(0));
        CHECK(r1.rfind("need|") == 0 && r1.find("变换中心") != std::string::npos);
        // 第二步：拾中心 → ask|
        std::string r2 = k.toolTap(0, 0, PX(0), PY(0));
        CHECK(r2.rfind("ask|") == 0 && r2.find("角度") != std::string::npos);
        // ask 态下画布点按被守卫（need| 不清点亮）
        CHECK(k.toolTap(1, 1, PX(1), PY(1)).rfind("need|") == 0);
        // 空参数报错且 ask 态保留（弹窗改再确认）
        CHECK(k.toolParam("  ") == "err|参数为空");
        // 非法引用同样保留 ask 态
        CHECK(k.toolParam("nope").rfind("err|", 0) == 0);
        CHECK(!k.toolParam("nope").empty());
        // 正确参数完成构造：A=(2,0) 绕 C 逆时针 90° → (0,2)
        int depthBefore = k.undoDepth();
        std::string r3 = k.toolParam("90");
        std::string img = doneLabel(r3);
        CHECK(!img.empty());
        GeoPoint *p = k.lookupPoint(img);
        CHECK(p != nullptr && std::fabs(p->x) < 1e-9 && std::fabs(p->y - 2.0) < 1e-9);
        CHECK(k.undoDepth() == depthBefore + 1);   // 一次 input = 一步撤销
        // 完成后 ask 态已清：再调 toolParam 无待输入态
        CHECK(k.toolParam("45") == "err|当前没有等待输入参数的工具");
    }

    // ---------- B. 旋转：线段对象 + 数值对象引用（滑动条驱动联动） ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(2, 0)"));
        CHECK(k.input("B=(3, 0)"));
        CHECK(k.input("s=Segment(A, B)"));
        CHECK(k.input("C=(0, 0)"));
        // 参数可引用数值对象：真滑动条（setSliderValue 仅对 isSlider 生效），
        // 初始值 = 下限 0
        CHECK(k.input("t=Slider(0, 360)"));
        k.setTool("rotate");
        CHECK(k.toolTap(2.5, 0, PX(2.5), PY(0)).rfind("need|") == 0);   // 拾到线段
        CHECK(k.toolTap(0, 0, PX(0), PY(0)).rfind("ask|") == 0);
        std::string img = doneLabel(k.toolParam("t"));
        CHECK(!img.empty());
        GeoSegment *g = dynamic_cast<GeoSegment *>(k.lookup(img));
        CHECK(g != nullptr && g->image);
        // t=0：像与原线段重合
        CHECK(g != nullptr && std::fabs(g->ex1() - 2.0) < 1e-9
            && std::fabs(g->ey1()) < 1e-9 && std::fabs(g->ex2() - 3.0) < 1e-9);
        // 改 t → 像随动（t=90 → (0,2)/(0,3)）
        GeoNumeric *tn = numOf(k, "t");
        CHECK(tn != nullptr);
        if (tn) {
            k.setSliderValue(tn, 90);
            CHECK(std::fabs(g->ex1()) < 1e-9 && std::fabs(g->ey1() - 2.0) < 1e-9
                && std::fabs(g->ex2()) < 1e-9 && std::fabs(g->ey2() - 3.0) < 1e-9);
        }
    }

    // ---------- C. 位似：圆 + 正/负比例（半径倍率与反像） ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("C=(0, 0)"));
        CHECK(k.input("cc=Circle(C, 1)"));
        CHECK(k.input("P=(1, 0)"));
        k.setTool("dilate");
        CHECK(k.toolTap(0, 1, PX(0), PY(1)).rfind("need|") == 0);   // 圆上拾圆
        std::string r = k.toolTap(1, 0, PX(1), PY(0));              // 中心 P
        CHECK(r.rfind("ask|") == 0 && r.find("比例") != std::string::npos);
        std::string img = doneLabel(k.toolParam("3"));
        CHECK(!img.empty());
        GeoCircle *g = dynamic_cast<GeoCircle *>(k.lookup(img));
        // 像：中心 P+3(C−P)=(−2,0)，半径 1×3
        CHECK(g != nullptr && std::fabs(g->ccx() + 2.0) < 1e-9
            && std::fabs(g->ccy()) < 1e-9 && std::fabs(g->radius - 3.0) < 1e-9);
        // k=−1：线段反像（等价中心对称）
        CHECK(k.input("Q1=(2, 0)"));
        CHECK(k.input("Q2=(4, 0)"));
        CHECK(k.input("qs=Segment(Q1, Q2)"));
        k.setTool("dilate");
        CHECK(k.toolTap(3, 0, PX(3), PY(0)).rfind("need|") == 0);
        CHECK(k.toolTap(0, 0, PX(0), PY(0)).rfind("ask|") == 0);
        std::string img2 = doneLabel(k.toolParam("-1"));
        GeoSegment *sg = dynamic_cast<GeoSegment *>(k.lookup(img2));
        CHECK(sg != nullptr && std::fabs(sg->ex1() + 2.0) < 1e-9
            && std::fabs(sg->ex2() + 4.0) < 1e-9);
    }

    // ---------- D. 圆（半径）：空白建圆心 + 半径非法保留 ask ----------
    {
        Kernel k;
        frame(k);
        k.setTool("circleRadius");
        std::string r1 = k.toolTap(5, 5, PX(5), PY(5));   // 空白先建自由点
        CHECK(r1.rfind("ask|") == 0 && r1.find("半径") != std::string::npos);
        // 半径必须为正：报错且 ask 保留，改对后完成
        CHECK(k.toolParam("0").rfind("err|", 0) == 0);
        std::string img = doneLabel(k.toolParam("2.5"));
        CHECK(!img.empty());
        GeoCircle *c = dynamic_cast<GeoCircle *>(k.lookup(img));
        CHECK(c != nullptr && std::fabs(c->radius - 2.5) < 1e-9
            && std::fabs(c->ccx() - 5.0) < 1e-9 && std::fabs(c->ccy() - 5.0) < 1e-9);
        // 拖圆心 → 圆随动
        GeoPoint *center = c ? c->center : nullptr;
        CHECK(center != nullptr);
        if (center) {
            k.movePoint(center, 1, 1);
            CHECK(std::fabs(c->ccx() - 1.0) < 1e-9 && std::fabs(c->ccy() - 1.0) < 1e-9);
        }
    }

    // ---------- E. 定长线段：起点 + 长度（沿 x 正向）+ 拖动随动 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(1, 1)"));
        k.setTool("segLength");
        std::string r = k.toolTap(1, 1, PX(1), PY(1));
        CHECK(r.rfind("ask|") == 0 && r.find("长度") != std::string::npos);
        std::string img = doneLabel(k.toolParam("3"));
        CHECK(!img.empty());
        GeoSegment *s = dynamic_cast<GeoSegment *>(k.lookup(img));
        CHECK(s != nullptr && std::fabs(s->ex1() - 1.0) < 1e-9
            && std::fabs(s->ey1() - 1.0) < 1e-9 && std::fabs(s->ex2() - 4.0) < 1e-9
            && std::fabs(s->ey2() - 1.0) < 1e-9);
        GeoPoint *A = k.lookupPoint("A");
        CHECK(A != nullptr);
        if (A && s) {
            k.movePoint(A, 0, 0);
            CHECK(std::fabs(s->ex2() - 3.0) < 1e-9 && std::fabs(s->ey2()) < 1e-9);
        }
    }

    // ---------- F. 定角：边点+顶点+角度（值恒定、终边点随动）+ 代数区/XML ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(2, 0)"));
        CHECK(k.input("B=(0, 0)"));
        k.setTool("angleSize");
        std::string r0 = k.toolTap(2, 0, PX(2), PY(0));
        CHECK(r0.rfind("need|") == 0 && r0.find("顶点") != std::string::npos);
        std::string r = k.toolTap(0, 0, PX(0), PY(0));
        CHECK(r.rfind("ask|") == 0 && r.find("角度") != std::string::npos);
        int depthBefore = k.undoDepth();
        std::string made = doneLabel(k.toolParam("60"));
        // 两个产物：角 + 可见终边点（GGB Angle with Given Size 同款）
        CHECK(donePart(std::string("done|") + made, 0).size() > 0);
        std::string angLabel = donePart(std::string("done|") + made, 0);
        std::string legLabel = donePart(std::string("done|") + made, 1);
        CHECK(!angLabel.empty() && !legLabel.empty());
        GeoAngle *ang = dynamic_cast<GeoAngle *>(k.lookup(angLabel));
        GeoPoint *leg = k.lookupPoint(legLabel);
        CHECK(ang != nullptr && std::fabs(ang->value - 60.0) < 1e-6);
        // 终边点 = A 绕 B 逆时针 60°：(1, √3)
        CHECK(leg != nullptr && std::fabs(leg->x - 1.0) < 1e-9
            && std::fabs(leg->y - std::sqrt(3.0)) < 1e-9);
        CHECK(k.undoDepth() == depthBefore + 2);   // Rotate + Angle 两步
        // 拖角边点：角值不变（定角语义），终边点随动
        GeoPoint *A = k.lookupPoint("A");
        CHECK(A != nullptr);
        if (A && ang && leg) {
            k.movePoint(A, 0, 3);
            CHECK(std::fabs(ang->value - 60.0) < 1e-6);
            CHECK(std::fabs(leg->x + 2.598076211353316) < 1e-9
                && std::fabs(leg->y - 1.5) < 1e-9);
        }
        // 代数区行类型 = angle
        bool hasAngleRow = false;
        std::vector<AlgebraRow> rows;
        k.algebraRows(rows);
        for (const auto &row : rows) {
            if (row.label == angLabel) hasAngleRow = row.type == "angle";
        }
        CHECK(hasAngleRow);
        // .ggb 往返：命令以 <command name="..."> 序列化，重放无跳过
        std::string xml = k.getXml();
        CHECK(xml.find("name=\"Rotate\"") != std::string::npos
            && xml.find("name=\"Angle\"") != std::string::npos);
        Kernel k2;
        int skipped = -1;
        CHECK(k2.setXml(xml, &skipped));
        CHECK(skipped == 0);
        GeoAngle *ang2 = dynamic_cast<GeoAngle *>(k2.lookup(angLabel));
        CHECK(ang2 != nullptr && std::fabs(ang2->value - 60.0) < 1e-6);
    }

    // ---------- G. 取消语义：setTool 复位 ask 态，工具保持激活 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(2, 0)"));
        CHECK(k.input("C=(0, 0)"));
        k.setTool("rotate");
        CHECK(k.toolTap(2, 0, PX(2), PY(0)).rfind("need|") == 0);
        CHECK(k.toolTap(0, 0, PX(0), PY(0)).rfind("ask|") == 0);
        // 取消 = 重入同工具（宿主 cancelToolAsk 走 kernelTool(id)）
        k.setTool("rotate");
        CHECK(k.toolParam("45") == "err|当前没有等待输入参数的工具");
        size_t nGeo = k.geos().size();
        // 复位后可从头再来：拾取 → ask → 完成
        CHECK(k.toolTap(2, 0, PX(2), PY(0)).rfind("need|") == 0);
        CHECK(k.toolTap(0, 0, PX(0), PY(0)).rfind("ask|") == 0);
        CHECK(!doneLabel(k.toolParam("180")).empty());
        CHECK(k.geos().size() == nGeo + 1);
    }

    printf("test23: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
