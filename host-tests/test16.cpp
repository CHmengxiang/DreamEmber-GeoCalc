// test16.cpp — 轮B 回归（2026-09-30 重建；原文件在临时目录清理中丢失，
// 按原覆盖范围重写）：撤销/重做 XML 快照栈语义 + 代数区行协议 v2。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test16.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test16.exe
#include "kernel/app.h"
#include <algorithm>
#include <cstdio>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

static GeoNumeric *numOf(Kernel &k, const std::string &label)
{
    return dynamic_cast<GeoNumeric *>(k.lookup(label));
}

// 协议行查找（按 label 前缀匹配整行）
static std::string rowOf(const std::string &text, const std::string &label)
{
    size_t pos = 0;
    while (pos < text.size()) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) eol = text.size();
        const std::string line = text.substr(pos, eol - pos);
        if (line.compare(0, label.size(), label) == 0
            && line.size() > label.size() && line[label.size()] == '|') {
            return line;
        }
        pos = eol + 1;
    }
    return "";
}

int main()
{
    // ---------- A. 离散变更自动打点（input/delete/rename/redefine/vis/clear） ----------
    {
        Kernel k;
        CHECK(k.undoDepth() == 0 && k.redoDepth() == 0);
        CHECK(k.input("a=1"));
        CHECK(k.input("b=2"));
        CHECK(k.undoDepth() == 2);
        CHECK(k.undoStep());                       // 撤掉 b=2
        CHECK(numOf(k, "b") == nullptr && numOf(k, "a") != nullptr);
        CHECK(k.redoDepth() == 1);
        CHECK(k.redoStep());
        CHECK(numOf(k, "b") != nullptr && numOf(k, "b")->value == 2.0);
        // 新变更清空重做栈
        CHECK(k.input("c=3"));
        CHECK(k.redoDepth() == 0);
        // 删除打点
        const int beforeDel = k.undoDepth();
        CHECK(k.deleteByLabel("c"));
        CHECK(k.undoDepth() == beforeDel + 1);
        CHECK(numOf(k, "c") == nullptr);
        CHECK(k.undoStep());
        CHECK(numOf(k, "c") != nullptr);
        // 改名打点 + 依赖 cmd 同步
        CHECK(k.input("d=a+1"));
        CHECK(k.rename("a", "z9") == "");
        const int beforeRen = k.undoDepth();
        CHECK(k.rename("z9", "a") == "");
        CHECK(k.undoDepth() == beforeRen + 1);
        CHECK(k.undoStep());
        CHECK(k.lookup("z9") != nullptr && k.lookup("a") == nullptr);
        CHECK(numOf(k, "d")->cmd == "z9+1");
        CHECK(k.redoStep());
        CHECK(k.lookup("a") != nullptr && numOf(k, "d")->cmd == "a+1");
        // 重定义打点
        const int beforeRed = k.undoDepth();
        CHECK(k.redefine("b", "a*2"));
        CHECK(k.undoDepth() == beforeRed + 1);
        CHECK(numOf(k, "b")->value == 2.0);
        CHECK(k.undoStep());
        CHECK(numOf(k, "b")->value == 2.0 && numOf(k, "b")->cmd == "2");   // 自由数值 cmd=字面量
        // 显隐打点
        CHECK(k.setVisible("a", false));
        const int beforeVis = k.undoDepth();
        CHECK(k.setVisible("a", true));
        CHECK(k.undoDepth() == beforeVis + 1);
        CHECK(k.undoStep());
        CHECK(k.lookup("a")->visible == false);
        // 清空打点（一步回到空）
        k.clearAll();
        CHECK(k.geos().empty());
        CHECK(k.undoStep());
        CHECK(!k.geos().empty());
    }

    // ---------- B. 失败变更不留步 / 手动打点 / 容量 50 ----------
    {
        Kernel k;
        CHECK(k.input("a=1"));
        const int depth = k.undoDepth();
        CHECK(!k.input("b=未定义符号"));            // 失败输入不入栈
        CHECK(k.undoDepth() == depth);
        CHECK(!k.deleteByLabel("nope"));
        CHECK(k.undoDepth() == depth);
        k.pushUndo();                               // 宿主手势打点（拖点/滑杆前）
        CHECK(k.undoDepth() == depth + 1);
        k.pushUndo();
        CHECK(k.undoDepth() == depth + 2);
        CHECK(k.undoStep() && k.undoStep());
        CHECK(k.undoDepth() == depth);
        // 容量 50：连打 60 步只留最近 50
        Kernel k2;
        for (int i = 0; i < 60; ++i) {
            k2.input("n" + std::to_string(i) + "=" + std::to_string(i));
        }
        CHECK(k2.undoDepth() == 50);
        CHECK(k2.geos().size() == 60);
    }

    // ---------- C. 撤销语义细节：依赖链随快照整体回退、选中态联动清理 ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=A1+1"));
        CHECK(k.setProp("A1", "value", "8"));
        CHECK(numOf(k, "B1")->value == 9.0);
        k.setSelected("B1");
        CHECK(k.lookup("B1")->selected);
        CHECK(k.undoStep());                        // 回到 A1=5、B1 尚未存在的快照
        CHECK(numOf(k, "B1") == nullptr && numOf(k, "A1")->value == 5.0);
        CHECK(k.redoStep());
        CHECK(numOf(k, "B1") != nullptr);
        // 级联删除打点：删 A1 连带 B1，undo 一步双双恢复
        CHECK(k.deleteByLabel("A1"));
        CHECK(numOf(k, "A1") == nullptr && numOf(k, "B1") == nullptr);
        CHECK(k.undoStep());
        CHECK(numOf(k, "A1") != nullptr && numOf(k, "B1") != nullptr
              && numOf(k, "B1")->value == 9.0);   // redo 后 A1=8、B1=9
    }

    // ---------- D. 行协议 v2：固定列 + key=value 尾列 ----------
    {
        Kernel k;
        CHECK(k.input("A=(1,2)"));
        CHECK(k.input("s1=Slider(0,10,1)"));
        CHECK(k.input("B=(3,4)"));
        CHECK(k.input("seg1=Segment(A,B)"));
        CHECK(k.input("C=(0,1)"));
        CHECK(k.input("poly1=Polygon(A,B,C)"));
        const std::string text = k.algebraText();
        // 点行恒 6 列（无尾列）
        const std::string pa = rowOf(text, "A");
        CHECK(!pa.empty());
        CHECK(std::count(pa.begin(), pa.end(), '|') == 5);
        CHECK(pa.find("|1|0") != std::string::npos);   // vis=1 sel=0
        // slider 行尾列 anim=0;min=..;max=..;step=..
        const std::string ps = rowOf(text, "s1");
        CHECK(ps.find("|slider|") != std::string::npos);
        CHECK(ps.find("anim=0") != std::string::npos);
        CHECK(ps.find("min=0") != std::string::npos);
        CHECK(ps.find("max=10") != std::string::npos);
        CHECK(ps.find("step=1") != std::string::npos);
        // 多边形行尾列 fill=15（默认 15%）
        const std::string pp = rowOf(text, "poly1");
        CHECK(pp.find("|polygon|") != std::string::npos);
        CHECK(pp.find("fill=15") != std::string::npos);
        // 隐藏对象 vis=0
        CHECK(k.setVisible("A", false));
        const std::string pa2 = rowOf(k.algebraText(), "A");
        CHECK(pa2.find("|0|") != std::string::npos);
        // 画布选中 → sel=1
        k.setSelected("B");
        const std::string pb = rowOf(k.algebraText(), "B");
        CHECK(pb.find("|1") != std::string::npos && pb.find("|1|1") != std::string::npos);
        // 空内核输出空协议
        Kernel k2;
        CHECK(k2.algebraText().empty());
    }

    // ---------- E. 角/函数填充尾列 + 撤销后协议一致 ----------
    {
        Kernel k;
        CHECK(k.input("A=(0,0)"));
        CHECK(k.input("B=(1,0)"));
        CHECK(k.input("C=(0,1)"));
        CHECK(k.input("ang1=Angle(A,B,C)"));
        const std::string pang = rowOf(k.algebraText(), "ang1");
        CHECK(pang.find("|angle|") != std::string::npos);
        CHECK(pang.find("fill=12") != std::string::npos);
        CHECK(k.input("f(x)=x^2"));
        const std::string pf = rowOf(k.algebraText(), "f");
        CHECK(pf.find("|function|") != std::string::npos);
        CHECK(pf.find("fill=") == std::string::npos);   // 普通函数无填充尾列
        // 不等式区域 fill=10（匿名对象，按类型在 geos 里找不等式行）
        CHECK(k.input("y<x"));
        bool ineqFound = false;
        for (const auto &g : k.geos()) {
            GeoFunction *f = dynamic_cast<GeoFunction *>(g.get());
            if (f != nullptr && f->ineqOp != 0) {
                ineqFound = rowOf(k.algebraText(), g->label).find("fill=10")
                    != std::string::npos;
            }
        }
        CHECK(ineqFound);
        // undo 一步（撤不等式）后角行填充仍在
        CHECK(k.undoStep());
        CHECK(rowOf(k.algebraText(), "ang1").find("fill=12") != std::string::npos);
    }

    printf("test16: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
