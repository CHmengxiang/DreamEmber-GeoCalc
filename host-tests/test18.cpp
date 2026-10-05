// test18.cpp — 轮 D 表格视图 host 测试：A1 单元格模型 + 表达式对象标签解析
//（依随数值 B1=A1+1）、rename/undo/XML 往返对依赖链的正确性。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test18.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o test18.exe
#include "kernel/app.h"
#include <cstdio>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)

static GeoNumeric *numOf(Kernel &k, const std::string &label)
{
    return dynamic_cast<GeoNumeric *>(k.lookup(label));
}

int main()
{
    // ---------- A. 单元格自由数值 ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        GeoNumeric *a = numOf(k, "A1");
        CHECK(a != nullptr && a->value == 5.0 && a->isFree);
        CHECK(a->definitionText() == "A1 = 5");
        CHECK(k.input("B2=2.5"));
        CHECK(numOf(k, "B2") != nullptr && numOf(k, "B2")->value == 2.5);
        // 大写列 + 多位行号
        CHECK(k.input("H40=-3"));
        CHECK(numOf(k, "H40") != nullptr && numOf(k, "H40")->value == -3);
    }

    // ---------- B. 依随表达式（B1=A1+1）----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=A1+1"));
        GeoNumeric *b = numOf(k, "B1");
        CHECK(b != nullptr && b->value == 6.0);
        CHECK(!b->isFree && b->calc != nullptr);
        CHECK(b->definitionText() == "B1 = A1+1");
        CHECK(b->inputs.size() == 1 && b->inputs[0] == k.lookup("A1"));
        // 自由数值改值 → 级联重算
        CHECK(k.setProp("A1", "value", "10"));
        CHECK(b->value == 11.0);
        // 重定义依赖源
        CHECK(k.redefine("A1", "7"));
        CHECK(b->value == 8.0);
        // 复合表达式与去重
        CHECK(k.input("C1=A1*A1+A1"));
        CHECK(numOf(k, "C1")->value == 56.0);   // 7*7+7
        CHECK(numOf(k, "C1")->inputs.size() == 1);
    }

    // ---------- C. 链式依赖与重定义保指针身份 ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=A1+1"));
        CHECK(k.input("C1=B1+1"));
        CHECK(numOf(k, "C1")->value == 7.0);
        CHECK(k.redefine("B1", "A1*2"));
        CHECK(numOf(k, "B1")->value == 10.0);
        CHECK(numOf(k, "C1")->value == 11.0);   // C1 仍指向 B1 本体
        // 依随数值自身重定义走占用检查放行重建（无依赖者时）
        CHECK(k.input("D1=A1+A1"));   // 先建
        CHECK(k.input("D1=A1+100"));  // 直接重输（GGB 单元格语义）
        CHECK(numOf(k, "D1") != nullptr && numOf(k, "D1")->value == 105.0);
    }

    // ---------- D. 级联删除 ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=A1+1"));
        CHECK(k.input("C1=B1*2"));
        CHECK(k.deleteByLabel("A1"));
        CHECK(k.lookup("B1") == nullptr && k.lookup("C1") == nullptr);
    }

    // ---------- E. rename 同步表达式文本（undo/XML 依赖它） ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=A1+1"));
        CHECK(k.rename("A1", "C9") == "");
        GeoNumeric *b = numOf(k, "B1");
        CHECK(b->value == 6.0);
        CHECK(b->cmd == "C9+1");
        CHECK(b->definitionText() == "B1 = C9+1");
        // 词边界：a1 不被 "a" 误伤
        Kernel k2;
        CHECK(k2.input("a=2"));
        CHECK(k2.input("b1=a+1"));
        CHECK(k2.rename("a", "z9") == "");
        CHECK(numOf(k2, "b1")->cmd == "z9+1");
        CHECK(numOf(k2, "b1")->value == 3.0);
    }

    // ---------- F. XML 往返（保存/载入 + undo 快照重放） ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=A1+1"));
        CHECK(k.input("c1=Distance(A1,A1)"));   // 命令数值混排
        const std::string xml = k.getXml();
        CHECK(xml.find("exp=\"B1=A1+1\"") != std::string::npos);
        Kernel k2;
        CHECK(k2.setXml(xml, nullptr));
        CHECK(numOf(k2, "A1") != nullptr && numOf(k2, "A1")->value == 5.0);
        GeoNumeric *b = numOf(k2, "B1");
        CHECK(b != nullptr && b->value == 6.0 && !b->isFree && b->calc != nullptr);
        CHECK(k2.setProp("A1", "value", "8"));
        CHECK(b->value == 9.0);   // 重建后依赖链仍活
        CHECK(numOf(k2, "c1") != nullptr && numOf(k2, "c1")->value == 0.0);
        // undo：rename 的快照重放同样经 expression 重建
        Kernel k3;
        CHECK(k3.input("A1=5"));
        CHECK(k3.input("B1=A1+1"));
        CHECK(k3.rename("A1", "C9") == "");
        CHECK(k3.undoStep());
        CHECK(numOf(k3, "A1") != nullptr && numOf(k3, "A1")->value == 5.0);
        CHECK(numOf(k3, "B1") != nullptr && numOf(k3, "B1")->value == 6.0);
        CHECK(numOf(k3, "B1")->cmd == "A1+1");
    }

    // ---------- G. 错误路径 ----------
    {
        Kernel k;
        CHECK(!k.input("B1=zz+1"));   // 未定义引用
        CHECK(k.lastError.find("zz") != std::string::npos);
        CHECK(k.input("A1=(0,0)"));   // 单元格放点（合法）
        CHECK(!k.input("B1=A1+1"));   // 点不能进数值公式
        CHECK(k.lastError.find("A1") != std::string::npos);
        CHECK(!k.input("D1=A1+1"));   // A1 此时是点
        // x 仍走函数分支：单元格放函数
        CHECK(k.input("E1=x^2"));
        GeoFunction *f = dynamic_cast<GeoFunction *>(k.lookup("E1"));
        CHECK(f != nullptr && fabs(f->body.eval(3.0) - 9.0) < 1e-9);
    }

    // ---------- H. 单元格命令（Distance/Mean 等，主输出落在该格） ----------
    {
        Kernel k;
        CHECK(k.input("A1=(0,0)"));
        CHECK(k.input("B1=(3,4)"));
        CHECK(k.input("C1=Distance(A1,B1)"));
        GeoNumeric *c = numOf(k, "C1");
        CHECK(c != nullptr && fabs(c->value - 5.0) < 1e-9);
        CHECK(c->cmdName == "Distance");
        CHECK(k.input("L1={1,2,3,4}"));
        CHECK(k.input("D1=Mean(L1)"));
        CHECK(numOf(k, "D1") != nullptr && fabs(numOf(k, "D1")->value - 2.5) < 1e-9);
    }

    // ---------- I. 既有语义回归（plain parse 不受 refs 影响） ----------
    {
        Kernel k;
        CHECK(k.input("f(x)=x^2+1"));
        CHECK(k.input("g(x)=sin(x)"));
        CHECK(k.input("b=pi+1"));
        CHECK(numOf(k, "b") != nullptr && fabs(numOf(k, "b")->value - 4.14159) < 1e-3);
        CHECK(k.input("A=(1,2)"));
        CHECK(k.input("a=5"));
        CHECK(!k.input("q=zz*2"));
        CHECK(k.input("L1={1,2,3}"));
        CHECK(k.input("m=Mean(L1)"));
        CHECK(fabs(numOf(k, "m")->value - 2.0) < 1e-9);
        // 未知符号错误文案保持
        CHECK(!k.input("h(x)=zz+1"));
        CHECK(k.lastError.find("未知符号") != std::string::npos);
    }

    // ---------- J. 依随数值 + 撤销/重做 ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=A1+1"));
        CHECK(k.input("C1=B1*2"));
        CHECK(numOf(k, "C1")->value == 12.0);
        CHECK(k.undoDepth() == 3);
        CHECK(k.undoStep());
        CHECK(k.lookup("C1") == nullptr && numOf(k, "B1")->value == 6.0);
        CHECK(k.redoStep());
        CHECK(numOf(k, "C1") != nullptr && numOf(k, "C1")->value == 12.0);
        CHECK(!numOf(k, "C1")->isFree && numOf(k, "C1")->calc != nullptr);
    }

    // ---------- K. Expr refs 模式直测 ----------
    {
        Expr e;
        std::string err;
        CHECK(e.parse("2*A1+A1^2", err, std::string(), true));
        CHECK(e.refs().size() == 1 && e.refs()[0] == "A1");
        CHECK(!e.containsX());
        const double v = e.evalResolved(0.0, [](const std::string &nm) -> double {
            return nm == "A1" ? 3.0 : NAN;
        });
        CHECK(fabs(v - 15.0) < 1e-9);
        // 已知常量不进 refs
        Expr e2;
        CHECK(e2.parse("pi+1", err, std::string(), true));
        CHECK(e2.refs().empty());
        // param 与 refs 同用：k 绑定为参数节点，只有 A1 进 refs
        Expr e3;
        CHECK(e3.parse("k*A1", err, "k", true));
        CHECK(e3.refs().size() == 1 && e3.refs()[0] == "A1");
        const double v3 = e3.evalResolved(3.0, [](const std::string &nm) -> double {
            return nm == "A1" ? 2.0 : NAN;
        });
        CHECK(fabs(v3 - 6.0) < 1e-9);   // k=3, A1=2
    }

    // ---------- L. 表格空单元格自动物化（2026-09-30 反馈③，GGB 空格=0） ----------
    {
        // L1. input 路径：引用空格自动建 0 值占位，填值经重定义联动
        {
            Kernel k;
            CHECK(k.input("B2=A2+1"));
            GeoNumeric *a2 = numOf(k, "A2");
            CHECK(a2 != nullptr && a2->isFree && !a2->calc && a2->value == 0.0);
            GeoNumeric *b2 = numOf(k, "B2");
            CHECK(b2 != nullptr && !b2->isFree && b2->calc != nullptr
                  && b2->value == 1.0 && b2->cmd == "A2+1");
            CHECK(b2->inputs.size() == 1 && b2->inputs[0] == a2);
            CHECK(k.setProp("A2", "value", "5"));
            CHECK(b2->value == 6.0);
            // 一步 undo 同时撤掉 B2 与自动格 A2；redo 回到填值后状态
            CHECK(k.undoStep());
            CHECK(numOf(k, "B2") == nullptr && numOf(k, "A2") == nullptr);
            CHECK(k.redoStep());
            CHECK(numOf(k, "A2") != nullptr && numOf(k, "A2")->value == 5.0);
            CHECK(numOf(k, "B2") != nullptr && numOf(k, "B2")->value == 6.0);
        }
        // L2. redefine 路径：已有对象改写为引用空格的公式（多增 1 个对象仍成功）
        {
            Kernel k;
            CHECK(k.input("C4=7"));
            CHECK(k.redefine("C4", "A2*2"));
            GeoNumeric *a2 = numOf(k, "A2");
            GeoNumeric *c4 = numOf(k, "C4");
            CHECK(a2 != nullptr && a2->isFree && a2->value == 0.0);
            CHECK(c4 != nullptr && c4->value == 0.0);
            CHECK(k.setProp("A2", "value", "5"));
            CHECK(c4->value == 10.0);
        }
        // L3. 失败输入不留半成品：不可解析引用出现时回滚已物化的占位格
        {
            Kernel k;
            CHECK(!k.input("E5=F5+ZZ"));   // F5 物化后 ZZ 失败 → F5 一并回滚
            CHECK(numOf(k, "E5") == nullptr && numOf(k, "F5") == nullptr);
            CHECK(!k.input("D3=A2+QQ"));   // A2 物化后 QQ 失败 → 回滚 A2
            CHECK(numOf(k, "D3") == nullptr && numOf(k, "A2") == nullptr);
        }
        // L4. 自引用与列表命名空间不物化
        {
            Kernel k;
            CHECK(!k.input("G7=G7+1"));
            CHECK(numOf(k, "G7") == nullptr);
            CHECK(!k.input("h2=L1+1"));    // L 开头是列表命名空间
            CHECK(numOf(k, "h2") == nullptr && numOf(k, "L1") == nullptr);
        }
        // L5. XML 往返：A2 以 "A2=0"/"A2=5" 自由数值重放，依赖链重建后仍活
        {
            Kernel k;
            CHECK(k.input("B2=A2+1"));
            CHECK(k.setProp("A2", "value", "5"));
            Kernel k2;
            CHECK(k2.setXml(k.getXml(), nullptr));
            CHECK(numOf(k2, "A2") != nullptr && numOf(k2, "A2")->value == 5.0);
            CHECK(numOf(k2, "B2") != nullptr && numOf(k2, "B2")->value == 6.0);
            CHECK(k2.setProp("A2", "value", "9"));
            CHECK(numOf(k2, "B2")->value == 10.0);
        }
    }

    printf("test18: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
