// expr.h — N1 阶段的双精度表达式求值器（变量仅 x）
#ifndef DREAMEMBER_KERNEL_EXPR_H
#define DREAMEMBER_KERNEL_EXPR_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dreamember {

// 支持 + - * / ^、一元正负、隐式乘法（2x、2sin(x)、(1+x)(2-x)）、
// 常用一元函数与常量 pi/π/e。NaN/Inf 由 double 运算自然产生，
// 供绘图断点（渐近线/间断）检测使用。
// 轮 D：对象标签引用（表格公式 B1=A1+1）——refs 模式下，非 x/y/常量/
// 函数的标识符收进 refs()（Ref 节点），求值经 evalResolved 的查值回调。
class Expr {
public:
    struct Node;   // 表达式树节点（实现见 expr.cpp）

    bool parse(const std::string &src, std::string &err);
    // 带"参数"的解析：src 里出现的标识符 param（如 Sequence 的循环变量 k）
    // 被绑定为参数节点，求值时由 eval 的第二实参提供
    bool parse(const std::string &src, std::string &err, const std::string &param);
    // refs 模式：未知标识符不再报错，记入 refs() 并解析为 Ref 节点
    //（调用方负责校验标签存在与类型）。param 与 allowRefs 可同用
    bool parse(const std::string &src, std::string &err, const std::string &param,
               bool allowRefs);
    double eval(double x) const;
    double eval(double x, double param) const;
    // 两变量求值（批次⑥：含 y 的不等式区域，如 x^2+y^2<r^2）
    double evalXY(double x, double y) const;
    // 带 Ref 求值：resolve(名字) 返回对象当前值；解析失败/无 resolver 时 Ref=NaN
    double evalResolved(double x,
                        const std::function<double(const std::string &)> &resolve) const;
    bool containsX() const { return hasX_; }
    bool containsY() const { return hasY_; }
    bool empty() const { return !root_; }
    const std::string &text() const { return text_; }
    // refs 模式解析出的对象标签（按首次出现序、去重；非 refs 模式为空）
    const std::vector<std::string> &refs() const { return refs_; }
    // 表达式是否为一次式 k*x+m（常数式 k=0 也算）；成功填 k/m
    bool linearCoef(double &k, double &m) const;
    // 同上，仅判定（不含常数式的歧义时也返回 true，k=0 表示水平线）
    bool linearCoef() const
    {
        double k = 0, m = 0;
        return linearCoef(k, m);
    }

private:
    std::shared_ptr<const Node> root_;
    bool hasX_ = false;
    bool hasY_ = false;
    std::string text_;
    std::vector<std::string> refs_;
};

// ---------- 键盘线性输入的预解析（第七包，上游 模板键的文本形式） ----------
// 引号串内（Text 文本）一律不动。三条规则：
//   $A$1 → A1          单元格绝对引用标记（本内核无复制语义，纯去标记）
//   0.(3) → (3/9)      循环小数 → 精确分数（1.2(34) → (1+232/990)）
//   1 3/4 → 1+3/4      带分数（整数 空白 分数 → 加法；须在去空白前做）
std::string PreprocessInput(const std::string &in);

// 把独立标识符 "ans" 替换为上一答案的字面量（上游 ans 变量；初值 0）。
// 由提交流程与 CAS 求值回填 SetAnsValue。
std::string ReplaceAns(const std::string &in);
double GetAnsValue();
void SetAnsValue(double v);

// s[p] 起是否希腊字母（U+0370–U+03FF）的 UTF-8 双字节序列。
// π（0xCF 0x80）除外——它是常量，由 tokenizer 单独处理。
// 希腊键盘（第八包）：α β γ 等作标识符（对象名/变量名）参与求值。
bool IsGreekAt(const std::string &s, size_t p);

} // namespace dreamember
#endif // DREAMEMBER_KERNEL_EXPR_H
