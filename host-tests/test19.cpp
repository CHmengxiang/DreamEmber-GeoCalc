// test19.cpp — 第七/八包内核扩展 host 测试：GGB 键盘线性输入形式。
// 覆盖：带分数（1 3/4）、循环小数（0.(3) / 1.2(34)）、$ 绝对引用标记、
// log 对齐 GGB（log(x)=常用对数、log(b,x) 底数在前）、后缀 ! % °、
// ans 变量（代数输入与 CAS 回填）、≤ ≥ 闭边界不等式、NthRoot 命令；
// 第八包加：希腊字母标识符（α β1 κ3）、下划线标识符（a_n）、
// Floor/Ceil 命令、改名规则对齐键盘输入（下划线/希腊放行）。
// 构建（cpp 目录下）：
//   g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I .
//     test19.cpp kernel/app.cpp kernel/expr.cpp kernel/geos.cpp kernel/view.cpp
//     kernel/ggbfile.cpp cas/cas_wrapper.cpp -lz -o ../../host-tests/test19.exe
#include "kernel/app.h"
#include "kernel/expr.h"
#include "cas/cas_wrapper.h"
#include <cmath>
#include <cstdio>
#include <string>

using namespace dreamember;

static int g_n = 0, g_fail = 0;
#define CHECK(cond) do { ++g_n; if (!(cond)) { ++g_fail; printf("FAIL L%d: %s\n", __LINE__, #cond); } } while (0)
#define CHECK_NEAR(v, want, eps) do { ++g_n; if (!(std::fabs((v) - (want)) <= (eps))) { ++g_fail; printf("FAIL L%d: %s=%.9f want %.9f\n", __LINE__, #v, (double)(v), (double)(want)); } } while (0)

static GeoNumeric *numOf(Kernel &k, const std::string &label)
{
    return dynamic_cast<GeoNumeric *>(k.lookup(label));
}

static GeoFunction *fnOf(Kernel &k, const std::string &label)
{
    return dynamic_cast<GeoFunction *>(k.lookup(label));
}

int main()
{
    // ---------- A. 带分数 ----------
    {
        Kernel k;
        CHECK(k.input("a = 1 3/4"));
        CHECK(numOf(k, "a") != nullptr);
        CHECK_NEAR(numOf(k, "a")->value, 1.75, 1e-12);
        CHECK(k.input("b = 2 11/8"));
        CHECK_NEAR(numOf(k, "b")->value, 3.375, 1e-12);
        // 负带分数：-(1+1/2)
        CHECK(k.input("c = -1 1/2"));
        CHECK_NEAR(numOf(k, "c")->value, -1.5, 1e-12);
        // 不误伤：坐标/除法/隐式乘法
        CHECK(k.input("d = 8/2"));
        CHECK_NEAR(numOf(k, "d")->value, 4.0, 1e-12);
    }

    // ---------- B. 循环小数 ----------
    {
        Kernel k;
        CHECK(k.input("r1 = 0.(3)"));
        CHECK(numOf(k, "r1") != nullptr);
        CHECK_NEAR(numOf(k, "r1")->value, 1.0 / 3.0, 1e-12);
        CHECK(k.input("r2 = 1.2(34)"));
        // 1.234343434...
        CHECK_NEAR(numOf(k, "r2")->value, 1.0 + 232.0 / 990.0, 1e-12);
        CHECK(k.input("r3 = 0.(142857)"));
        CHECK_NEAR(numOf(k, "r3")->value, 1.0 / 7.0, 1e-12);
        // 函数调用括号不误伤
        CHECK(k.input("r4 = sqrt(4)"));
        CHECK_NEAR(numOf(k, "r4")->value, 2.0, 1e-12);
    }

    // ---------- C. $ 绝对引用标记 ----------
    {
        Kernel k;
        CHECK(k.input("A1=5"));
        CHECK(k.input("B1=$A$1+1"));
        CHECK(numOf(k, "B1") != nullptr && numOf(k, "B1")->value == 6.0);
    }

    // ---------- D. log 对齐 GGB ----------
    {
        Kernel k;
        CHECK(k.input("g1 = log(100)"));
        CHECK_NEAR(numOf(k, "g1")->value, 2.0, 1e-12);
        CHECK(k.input("g2 = log(2, 8)"));
        CHECK_NEAR(numOf(k, "g2")->value, 3.0, 1e-12);
        CHECK(k.input("g3 = ln(e)"));
        CHECK_NEAR(numOf(k, "g3")->value, 1.0, 1e-12);
    }

    // ---------- E. 后缀 ! % ° ----------
    {
        Kernel k;
        CHECK(k.input("h1 = 5!"));
        CHECK_NEAR(numOf(k, "h1")->value, 120.0, 1e-9);
        CHECK(k.input("h2 = 2^3!"));
        CHECK_NEAR(numOf(k, "h2")->value, 64.0, 1e-9);   // 2^(3!) = 2^6
        CHECK(k.input("h3 = 50%"));
        CHECK_NEAR(numOf(k, "h3")->value, 0.5, 1e-12);
        CHECK(k.input("h4 = sin(30°)"));
        CHECK_NEAR(numOf(k, "h4")->value, 0.5, 1e-12);
        CHECK(k.input("h5 = 90°"));
        CHECK_NEAR(numOf(k, "h5")->value, 1.570796317, 1e-6);
        // 非整数阶乘 → NaN 值（求值器语义，不建错误对象）
        CHECK(k.input("h6 = 2.5!"));
        CHECK(numOf(k, "h6") != nullptr && std::isnan(numOf(k, "h6")->value));
    }

    // ---------- F. ans 变量（代数输入回填） ----------
    {
        Kernel k;
        CHECK(k.input("a1 = 2+3"));
        CHECK(numOf(k, "a1") != nullptr && numOf(k, "a1")->value == 5.0);
        CHECK_NEAR(GetAnsValue(), 5.0, 1e-12);
        CHECK(k.input("b1 = ans+1"));
        CHECK(numOf(k, "b1") != nullptr && numOf(k, "b1")->value == 6.0);
        // ans 不可占用为标签
        CHECK(!k.input("ans = 3"));
        // CAS 数值结果回填（host 构建无 giac 时占位返回，跳过回填断言）
        const std::string r = CasCaseval("7*6");
        if (r.rfind("cas-not-ready", 0) != 0) {
            CHECK_NEAR(GetAnsValue(), 42.0, 1e-9);
        }
        SetAnsValue(42.0);
        CHECK(k.input("c1 = ans/2"));
        CHECK(numOf(k, "c1") != nullptr && numOf(k, "c1")->value == 21.0);
    }

    // ---------- G. ≤ ≥ 闭边界不等式 ----------
    {
        Kernel k;
        CHECK(k.input("y≥x"));
        GeoFunction *f1 = nullptr;
        for (auto &g : k.geos()) {
            GeoFunction *gf = dynamic_cast<GeoFunction *>(g.get());
            if (gf != nullptr && gf->ineqOp == 4) f1 = gf;
        }
        CHECK(f1 != nullptr && !f1->ineqVertical);   // y≥x：单变量函数式区域
        // 真两变量（两侧含 y）+ Unicode 闭边界
        CHECK(k.input("x^2+y^2≤9"));
        GeoFunction *f3 = nullptr;
        for (auto &g : k.geos()) {
            GeoFunction *gf = dynamic_cast<GeoFunction *>(g.get());
            if (gf != nullptr && gf->ineqTwoVar && gf->ineqOp == 2) f3 = gf;
        }
        CHECK(f3 != nullptr);
        CHECK(k.input("x≤3"));
        GeoFunction *f2 = nullptr;
        for (auto &g : k.geos()) {
            GeoFunction *gf = dynamic_cast<GeoFunction *>(g.get());
            if (gf != nullptr && gf->ineqOp == 2 && gf->ineqVertical) f2 = gf;
        }
        CHECK(f2 != nullptr);
        // ASCII <= 等价性保持
        CHECK(k.input("y<=2x"));
        bool gotLe = false;
        for (auto &g : k.geos()) {
            GeoFunction *gf = dynamic_cast<GeoFunction *>(g.get());
            if (gf != nullptr && gf->ineqOp == 2 && !gf->ineqVertical) gotLe = true;
        }
        CHECK(gotLe);
    }

    // ---------- H. NthRoot ----------
    {
        Kernel k;
        CHECK(k.input("n1 = NthRoot(8, 3)"));
        CHECK_NEAR(numOf(k, "n1")->value, 2.0, 1e-12);
        CHECK(k.input("n2 = NthRoot(-27, 3)"));
        CHECK_NEAR(numOf(k, "n2")->value, -3.0, 1e-12);
        CHECK(k.input("n3 = NthRoot(16, 4)"));
        CHECK_NEAR(numOf(k, "n3")->value, 2.0, 1e-12);
        CHECK(!k.input("n4 = NthRoot(4, 0)"));
    }

    // ---------- I. 预处理直测 ----------
    {
        CHECK(PreprocessInput("$A$1+1") == "A1+1");
        CHECK(PreprocessInput("0.(3)") == "3/9");
        CHECK(PreprocessInput("2*0.(3)") == "2*(3/9)");
        CHECK(PreprocessInput("1.2(34)") == "1+232/990");
        CHECK(PreprocessInput("5*1 3/4") == "5*(1+3/4)");
        CHECK(PreprocessInput("1 3/4") == "1+3/4");
        CHECK(PreprocessInput("sin(2)") == "sin(2)");
        CHECK(PreprocessInput("2(3)") == "2(3)");
        CHECK(PreprocessInput("Text(\"a b\")") == "Text(\"a b\")");
        CHECK(PreprocessInput("-1 1/2") == "-(1+1/2)");
        CHECK(PreprocessInput("3-1 1/2") == "3-(1+1/2)");
        SetAnsValue(5.0);
        CHECK(ReplaceAns("ans+1") == "(5)+1");
        CHECK(ReplaceAns("Text(\"ans\")") == "Text(\"ans\")");   // 引号内不动
    }

    // ---------- I2. 多参函数进表达式本体（逗号分词补齐后） ----------
    {
        Kernel k;
        CHECK(k.input("m9 = mod(9, 4)"));
        CHECK_NEAR(numOf(k, "m9")->value, 1.0, 1e-12);
        CHECK(k.input("mx = max(3, 7)"));
        CHECK_NEAR(numOf(k, "mx")->value, 7.0, 1e-12);
        // 列表形式的统计命令不受回落影响
        CHECK(k.input("L9={1,5,3}"));
        CHECK(k.input("mx2=Max(L9)"));
        CHECK_NEAR(numOf(k, "mx2")->value, 5.0, 1e-12);
    }

    // ---------- J. 回归：常规解析不受影响 ----------
    {
        Kernel k;
        CHECK(k.input("f(x)=x^2+1"));
        CHECK(fnOf(k, "f") != nullptr);
        CHECK_NEAR(fnOf(k, "f")->evalAt(2.0), 5.0, 1e-12);
        CHECK(k.input("P=(1,2)"));
        CHECK(k.input("q=1+2*3"));
        CHECK_NEAR(numOf(k, "q")->value, 7.0, 1e-12);
        CHECK(k.input("L={1,2,3}"));
    }

    // ---------- K. 第八包：希腊标识符 / Floor·Ceil / a_n / 改名规则 ----------
    {
        Kernel k;
        // 希腊字母作对象名与变量（GGB 希腊键盘）
        CHECK(k.input("α = 3"));
        CHECK(numOf(k, "α") != nullptr);
        CHECK_NEAR(numOf(k, "α")->value, 3.0, 1e-12);
        CHECK(k.input("β1 = α^2"));
        CHECK_NEAR(numOf(k, "β1")->value, 9.0, 1e-12);
        CHECK(k.input("γ2 = β1 + α"));
        CHECK_NEAR(numOf(k, "γ2")->value, 12.0, 1e-12);
        // π 仍是常量不是标识符
        CHECK(k.input("δ = π"));
        CHECK_NEAR(numOf(k, "δ")->value, M_PI, 1e-12);
        // 下划线标识符（f(x) 页 □ₙ 键官方插入 a_n）
        CHECK(k.input("a_n = 5"));
        CHECK_NEAR(numOf(k, "a_n")->value, 5.0, 1e-12);
        CHECK(k.input("s1 = a_n + 1"));
        CHECK_NEAR(numOf(k, "s1")->value, 6.0, 1e-12);
        // Floor / Ceil（#&¬ 页 ⌊⌋ ⌈⌉ 模板键命令）
        CHECK(k.input("f1 = Floor(-2.5)"));
        CHECK_NEAR(numOf(k, "f1")->value, -3.0, 1e-12);
        CHECK(k.input("f2 = Ceil(2.1)"));
        CHECK_NEAR(numOf(k, "f2")->value, 3.0, 1e-12);
        CHECK(k.input("f3 = floor(3.9)"));
        CHECK_NEAR(numOf(k, "f3")->value, 3.0, 1e-12);
        // 改名规则对齐键盘输入（下划线/希腊放行）
        CHECK(k.input("t1 = 7"));
        CHECK(k.rename("t1", "u_2") == "");
        CHECK(k.input("t2 = 8"));
        CHECK(k.rename("t2", "κ3") == "");
        // 希腊与下划线标签的混合引用
        CHECK(k.input("mix = u_2 + κ3"));
        CHECK_NEAR(numOf(k, "mix")->value, 15.0, 1e-12);
    }

    printf("test19: %d checks, %d failed\n", g_n, g_fail);
    return g_fail == 0 ? 0 : 1;
}
