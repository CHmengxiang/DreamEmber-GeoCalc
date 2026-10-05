// test20.cpp — 第二十三包 host 测试：表格批量求值（tableEval）+ 工具第一批
//（perpLine/parLine/reflectPoint/dist/area/slope/vecFromPoint 状态机流）+
// 命令扩展（Vector(P,u) 相等向量、Segment(点,长度)、Distance(点,直线)）。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test20.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test20.exe
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

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);   // 崩溃前也要吐出 FAIL 行
    // ---------- A. tableEval 批量求值 ----------
    {
        Kernel k;
        CHECK(k.input("f(x)=2x+1"));
        CHECK(k.input("g(x)=x^2"));
        CHECK(k.input("A=(1, 2)"));
        CHECK(k.input("s=Segment(A, 3)"));
        // 基本求值：f 在 0/1/2 → 1/3/5；g 在 2 → 4
        CHECK(k.tableEval("f,g", "0,1,2") == "1,0;3,1;5,4");
        // 含未知对象列：空格串占位
        CHECK(k.tableEval("f,nope", "0") == "1,");
        // 空输入
        CHECK(k.tableEval("", "") == "");
        // 负小数
        CHECK(k.tableEval("f", "-0.5") == "0");
        // 非水平直线列：y = 2x（两点式）
        CHECK(k.input("P=(0, 0)"));
        CHECK(k.input("Q=(1, 2)"));
        CHECK(k.input("l=Line(P, Q)"));
        CHECK(k.tableEval("l", "0,5") == "0;10");
        // 不等式区域排除（ineqOp，匿名对象从构造列表找标签）
        CHECK(k.input("y>x"));
        std::string ineqLabel;
        for (auto &g : k.geos()) {
            GeoFunction *gf = dynamic_cast<GeoFunction *>(g.get());
            if (gf && gf->ineqOp) { ineqLabel = gf->label; break; }
        }
        CHECK(k.tableEval(ineqLabel, "0") == "");
        // 竖直线排除（b≈0 无 y=f(x) 形式；经垂线构出 x=常数）
        CHECK(k.input("H1=(1, 1)"));
        CHECK(k.input("H2=(3, 1)"));
        CHECK(k.input("hline=Line(H1, H2)"));
        CHECK(k.input("VP=(2, 2)"));
        CHECK(k.input("vline=PerpendicularLine(VP, hline)"));
        CHECK(k.tableEval("vline", "0") == "");
    }

    // ---------- B. 工具：垂线 / 平行线（点先/线先两种顺序）----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(4, 0)"));
        CHECK(k.input("base=Line(A, B)"));
        // 垂线：先拾基准线（线上点 (2,0)），再点空白建点完成
        k.setTool("perpLine");
        CHECK(k.toolTap(2, 0, PX(2), PY(0)).rfind("need|") == 0);
        std::string r = k.toolTap(1, 2, PX(1), PY(2));
        std::string lp = doneLabel(r);
        CHECK(!lp.empty());
        GeoLine *perp = dynamic_cast<GeoLine *>(k.lookup(lp));
        CHECK(perp != nullptr && perp->mode == GeoLine::Mode::PerpThrough);
        // 过新点 (1,2) 且垂直于 y=0 → 竖直线 x=1；竖直线不在表列
        CHECK(perp != nullptr && k.tableEval(lp, "0") == "");
        // 平行线：先拾 base（(3,0) 处，避开上一步的竖直线 x=1），再点空白
        // 建点 (2,3) → 平行线 y = 3
        k.setTool("parLine");
        CHECK(k.toolTap(3, 0, PX(3), PY(0)).rfind("need|") == 0);
        std::string r2 = k.toolTap(2, 3, PX(2), PY(3));
        std::string lp2 = doneLabel(r2);
        CHECK(!lp2.empty());
        GeoLine *par = dynamic_cast<GeoLine *>(k.lookup(lp2));
        CHECK(par != nullptr && par->mode == GeoLine::Mode::ParallelThrough);
        CHECK(par != nullptr && par->valueText() == "y = 3");
    }

    // ---------- C. 工具：中心对称（Reflect(对象, 点)）----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(2, 0)"));
        CHECK(k.input("s=Segment(A, B)"));
        CHECK(k.input("C=(1, 1)"));
        k.setTool("reflectPoint");
        CHECK(k.toolTap(1, 0, PX(1), PY(0)).rfind("need|") == 0);   // 拾到 s
        std::string r = k.toolTap(1, 1, PX(1), PY(1));              // 拾到 C
        std::string ip = doneLabel(r);
        CHECK(!ip.empty());
        GeoSegment *img = dynamic_cast<GeoSegment *>(k.lookup(ip));
        CHECK(img != nullptr && img->image);
        CHECK(img != nullptr && std::fabs(img->ex1() - 2.0) < 1e-9
            && std::fabs(img->ey1() - 2.0) < 1e-9);
    }

    // ---------- D. 工具：距离（两点）+ Distance(点, 直线) 命令 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(3, 4)"));
        k.setTool("dist");
        CHECK(k.toolTap(0, 0, PX(0), PY(0)).rfind("need|") == 0);
        std::string r = k.toolTap(3, 4, PX(3), PY(4));
        std::string dp = doneLabel(r);
        GeoNumeric *d = numOf(k, dp);
        CHECK(d != nullptr && std::fabs(d->value - 5.0) < 1e-9);
        // 点到直线：Distance(P, l)，两种参数序，级联随点移动重算
        CHECK(k.input("p0=(0, 0)"));
        CHECK(k.input("q0=(4, 0)"));
        CHECK(k.input("hor=Line(p0, q0)"));
        CHECK(k.input("D=(1, 3)"));
        CHECK(k.input("dl=Distance(D, hor)"));
        CHECK(std::fabs(numOf(k, "dl")->value - 3.0) < 1e-9);
        CHECK(k.input("dl2=Distance(hor, D)"));
        CHECK(std::fabs(numOf(k, "dl2")->value - 3.0) < 1e-9);
        k.movePoint(static_cast<GeoPoint *>(k.lookup("D")), 1, 5);
        CHECK(std::fabs(numOf(k, "dl")->value - 5.0) < 1e-9);
    }

    // ---------- E. 工具：面积 / 斜率 ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(4, 0)"));
        CHECK(k.input("C=(4, 3)"));
        CHECK(k.input("t=Polygon(A, B, C)"));
        k.setTool("area");
        std::string r = k.toolTap(3.9, 0.1, PX(3.9), PY(0.1));   // 拾到边
        std::string ap = doneLabel(r);
        CHECK(numOf(k, ap) != nullptr && std::fabs(numOf(k, ap)->value - 6.0) < 1e-9);
        // 斜率：一次函数
        CHECK(k.input("fn(x)=2x+1"));
        k.setTool("slope");
        std::string r2 = k.toolTap(2, 5, PX(2), PY(5));   // fn(2)=5，正中曲线上
        std::string sp = doneLabel(r2);
        CHECK(numOf(k, sp) != nullptr && std::fabs(numOf(k, sp)->value - 2.0) < 1e-9);
        // 拒绝非一次函数（走命令直输；工具层对象拾取按纵向最近取线，二次
        // 函数与其他曲线同屏时命中序不稳定，命令层是同一守卫）
        CHECK(k.input("qq(x)=x^2"));
        CHECK(!k.input("sq=Slope(qq)"));
    }

    // ---------- F. 工具/命令：相等向量 Vector(P, u) ----------
    {
        Kernel k;
        frame(k);
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(2, 1)"));
        CHECK(k.input("u=Vector(A, B)"));
        CHECK(k.input("P=(5, 5)"));
        k.setTool("vecFromPoint");
        CHECK(k.toolTap(5, 5, PX(5), PY(5)).rfind("need|") == 0);       // 拾到 P
        std::string r = k.toolTap(1, 0.5, PX(1), PY(0.5));              // 拾到 u
        std::string vp = doneLabel(r);
        GeoVector *w = dynamic_cast<GeoVector *>(k.lookup(vp));
        CHECK(w != nullptr && w->image);
        CHECK(w->valueText() == "(2, 1)");
        CHECK(std::fabs(w->ex1() - 5.0) < 1e-9 && std::fabs(w->ey2() - 6.0) < 1e-9);
        // 命令直输
        CHECK(k.input("w1=Vector(P, u)"));
        CHECK(dynamic_cast<GeoVector *>(k.lookup("w1")) != nullptr);
        // 源向量随动：拖 B → 相等向量端点平移
        k.movePoint(static_cast<GeoPoint *>(k.lookup("B")), 4, 1);
        CHECK(w->valueText() == "(4, 1)");
        // image 向量可被对象拾取命中（pickObject 走 ex/ey 访问器）；w 自
        // P=(5,5) 起、方向 (4,1)，px=600 处线上 y=5.25
        CHECK(k.pickObject(PX(6), PY(5.25)) == w);
    }

    // ---------- G. Segment(点, 长度) 定长线段 ----------
    {
        Kernel k;
        CHECK(k.input("A=(1, 1)"));
        CHECK(k.input("len=3"));
        // 字面量长度
        CHECK(k.input("s1=Segment(A, 2)"));
        GeoSegment *s1 = dynamic_cast<GeoSegment *>(k.lookup("s1"));
        CHECK(s1 != nullptr && s1->image);
        CHECK(s1->valueText() == "2");
        CHECK(std::fabs(s1->ex1() - 1.0) < 1e-9 && std::fabs(s1->ey2() - 1.0) < 1e-9);
        // 数值对象长度：改 len → 线段随动
        CHECK(k.input("s2=Segment(A, len)"));
        GeoSegment *s2 = dynamic_cast<GeoSegment *>(k.lookup("s2"));
        CHECK(s2 != nullptr && s2->valueText() == "3");
        CHECK(k.setProp("len", "value", "5"));
        CHECK(s2->valueText() == "5");
        // 拖动 A → 端点跟随
        k.movePoint(static_cast<GeoPoint *>(k.lookup("A")), 2, 3);
        CHECK(std::fabs(s2->ex1() - 2.0) < 1e-9 && std::fabs(s2->iy2 - 3.0) < 1e-9);
        // 非法：长度非正或含 x
        CHECK(!k.input("s3=Segment(A, 0)"));
        CHECK(!k.input("s4=Segment(A, -1)"));
        CHECK(!k.input("s5=Segment(A, x)"));
        // 两点式不受影响
        CHECK(k.input("B=(4, 1)"));
        CHECK(k.input("s6=Segment(A, B)"));
        GeoSegment *s6 = dynamic_cast<GeoSegment *>(k.lookup("s6"));
        CHECK(s6 != nullptr && !s6->image);
    }

    // ---------- H. XML 往返：命令重放（Vector(P,u)/Segment(A,r)）----------
    {
        Kernel k;
        CHECK(k.input("A=(0, 0)"));
        CHECK(k.input("B=(2, 1)"));
        CHECK(k.input("u=Vector(A, B)"));
        CHECK(k.input("P=(5, 5)"));
        CHECK(k.input("w=Vector(P, u)"));
        CHECK(k.input("s=Segment(A, 2)"));
        Kernel k2;
        CHECK(k2.setXml(k.getXml(), nullptr));
        GeoVector *w2 = dynamic_cast<GeoVector *>(k2.lookup("w"));
        GeoSegment *s2 = dynamic_cast<GeoSegment *>(k2.lookup("s"));
        CHECK(w2 != nullptr && w2->image && w2->valueText() == "(2, 1)");
        CHECK(s2 != nullptr && s2->image && s2->valueText() == "2");
        // 重放后仍级联
        k2.movePoint(static_cast<GeoPoint *>(k2.lookup("B")), 3, 1);
        CHECK(w2->valueText() == "(3, 1)");
    }

    printf("test20: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
