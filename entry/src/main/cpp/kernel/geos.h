// geos.h — GeoElement 基础类族（点/线段/直线/射线/向量/圆/多边形/函数/数值/角）
#ifndef DREAMEMBER_KERNEL_GEOS_H
#define DREAMEMBER_KERNEL_GEOS_H

#include <functional>
#include <string>
#include <vector>
#include "kernel/expr.h"
#include "kernel/view.h"

namespace dreamember {

enum class GeoType { Point, Segment, Line, Ray, Vector, Circle, Polygon, Function, Numeric, Angle, List, Text };

struct GeoColor {
    int r = 0, g = 0, b = 0;
};

// Integral 填充状态（GeoNumeric 持有 shared_ptr：calc 重算与渲染读同一份，
// redefine 后新旧对象经拷贝共享，滑动条驱动的边界变化两边同步）
class GeoNumeric;
class GeoFunction;
struct GeoFillState {
    GeoFunction *fn = nullptr;
    double a = 0, b = 0;
    bool on = false;
};

std::string ColorHex(const GeoColor &c);
std::string FormatNum(double v);
// ax+by=c 的展示文本（y = ... 优先；b≈0 时 x = ...）
std::string LineEqText(double a, double b, double c);

// 基类：标签/可见性/颜色/线宽 + 依赖重算接口。
// inputs：依赖的前置对象（创建时填写；构造序即拓扑序，重算按构造序单遍 update，
// 级联删除也以它为依据——上游 construction list 的最小等价物）。
// 线宽：实际渲染宽度 = lineThickness / 3（真机反馈调细），默认 5。
// N5：cmdName/cmdArgs/cmdIndex 记录创建命令原文，供 .ggb 保存重放与代数区定义列。
class GeoElement {
public:
    explicit GeoElement(GeoType t) : type_(t) {}
    virtual ~GeoElement() = default;

    GeoType type() const { return type_; }
    std::string label;
    bool visible = true;
    bool selected = false;   // 画布选中特效（代数区点行/长按菜单联动；不入 .ggb）
    GeoColor color{0, 0, 0};
    int lineThickness = 5;
    std::vector<GeoElement *> inputs;

    std::string cmdName;               // 创建命令名（"Midpoint"/"Intersect"/...；自由对象为空）
    std::vector<std::string> cmdArgs;  // 命令参数（label 或字面量）
    int cmdIndex = 0;                  // 多输出命令中的序号（Intersect/Tangent 第 n 个，0 起）

    virtual void update() {}
    // 代数区"值"列：点坐标/线方程/长度/表达式等
    virtual std::string valueText() const = 0;
    // 代数区"定义"列：可直接回填输入框再创建的原文
    virtual std::string definitionText() const = 0;
    // cmdName 非空时返回 "Cmd(a, b)"，否则空串
    std::string cmdDefText() const;

private:
    GeoType type_;
};

// 自由点（或 Midpoint/Intersect/变换像等依随点）。默认色 = 上游 自由点 (77,77,255)。
class GeoPoint : public GeoElement {
public:
    GeoPoint() : GeoElement(GeoType::Point) { color = {77, 77, 255}; }
    double x = 0, y = 0;
    int pointSize = 5;              // 上游 DEFAULT_POINT_SIZE
    bool isFree = true;
    // 依随点重算闭包（Intersect/变换像等；calc 为空时退回 Midpoint 语义）
    std::function<void(GeoPoint *)> calc;

    void update() override;
    std::string valueText() const override;
    std::string definitionText() const override;
};

class GeoSegment : public GeoElement {
public:
    GeoSegment() : GeoElement(GeoType::Segment) {}
    GeoPoint *p1 = nullptr, *p2 = nullptr;
    // 变换像：端点缓存（image=true 时几何取缓存而非 p1/p2）
    bool image = false;
    double ix1 = 0, iy1 = 0, ix2 = 0, iy2 = 0;
    std::function<void(GeoSegment *)> calc;

    double ex1() const { return image ? ix1 : p1->x; }
    double ey1() const { return image ? iy1 : p1->y; }
    double ex2() const { return image ? ix2 : p2->x; }
    double ey2() const { return image ? iy2 : p2->y; }
    void update() override { if (calc) calc(this); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

// 直线 ax + by = c（世界坐标）。来源：两点式 / 过点垂直 / 过点平行 / 中垂线 /
// 角平分线 / 切线与变换像（calc）。
class GeoLine : public GeoElement {
public:
    enum class Mode { Fixed, TwoPoints, PerpThrough, ParallelThrough, PerpBisector, AngleBisector };
    GeoLine() : GeoElement(GeoType::Line) {}
    double a = 0, b = 0, c = 0;
    bool verticalConst = false;
    Mode mode = Mode::Fixed;
    GeoPoint *p1 = nullptr, *p2 = nullptr;   // TwoPoints / PerpBisector(A,B)
    GeoPoint *pt = nullptr;                  // Perp/ParallelThrough 的过点；AngleBisector 顶点
    GeoElement *base = nullptr;              // Perp/ParallelThrough 的基准（直线/线段/一次函数图像）
    std::function<void(GeoLine *)> calc;     // 切线/变换像等依随直线重算 a,b,c
    void update() override;
    std::string valueText() const override;
    std::string definitionText() const override;
};

class GeoCircle : public GeoElement {
public:
    GeoCircle() : GeoElement(GeoType::Circle) {}
    GeoPoint *center = nullptr;
    GeoPoint *through = nullptr;     // 与 radius / radiusNum 三选一
    GeoNumeric *radiusNum = nullptr; // 半径引用数值对象（随其重算）
    double radius = 0;
    // 变换像：圆心缓存（image=true 时圆心取 icx/icy）
    bool image = false;
    double icx = 0, icy = 0;
    std::function<void(GeoCircle *)> calc;

    double ccx() const { return image ? icx : center->x; }
    double ccy() const { return image ? icy : center->y; }
    void update() override;
    std::string valueText() const override;
    std::string definitionText() const override;
};

class GeoFunction : public GeoElement {
public:
    GeoFunction() : GeoElement(GeoType::Function) { color = {0x1C, 0x1C, 0x1F}; }
    std::string src;   // 规范化后的表达式原文
    Expr body;
    // N6：非表达式定义的函数（Derivative 数值导数等）——置 alt 后所有求值走
    // 闭包（捕获源函数指针，源重定义后求值仍取最新定义），body 保持空
    std::function<double(double)> alt;

    // N6 批次⑤ 不等式（ineqOp≠0 时本对象是区域而非曲线）：
    // 1='<' 2='<=' 3='>' 4='>='（y op 表达式，区域在 op 指向侧）；
    // ineqVertical 为 x 线性式解出的竖直半平面（x ? ineqX），此时 body 为空
    int ineqOp = 0;
    bool ineqVertical = false;
    double ineqX = 0;
    // 批次⑥ 两变量区域（x^2+y^2<r^2 类）：区域 = ineqL op ineqR，
    // L/R 为含 x、y 的表达式，body/ineqVertical 均不使用
    bool ineqTwoVar = false;
    Expr ineqL, ineqR;
    // 区域填充透明度（0-1，代数区选中行滑杆可调；真机反馈默认太实 0.18 → 0.10）。
    // 持久化复用 上游 <objColor alpha>（仅不等式元素写入；载入 alpha=0 视为旧版占位回默认）
    double fillAlpha = 0.10;

    double evalAt(double x) const { return alt ? alt(x) : body.eval(x); }
    bool definable() const { return alt != nullptr || !body.empty(); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

// 数值对象（a=5、Distance(A,B)、Slider）。不直接绘制（Slider 除外），仅进代数区。
class GeoNumeric : public GeoElement {
public:
    GeoNumeric() : GeoElement(GeoType::Numeric) { visible = false; }
    // 子类携带自身类型（GeoAngle 复用数值存储但需命中 Angle 渲染/协议分支）
    explicit GeoNumeric(GeoType t) : GeoElement(t) { visible = false; }
    double value = 0;
    bool isFree = true;
    std::function<double()> calc;   // 依随数值的重算闭包（级联删除保证不悬垂）
    std::string cmd;                // 依随时记录的命令原文（"Distance(A, B)"）

    // N5 Slider：画布内滑动条（上游 同款：轨道 + 手柄 + 标签，绝对屏幕坐标 px）
    bool isSlider = false;
    double smin = 0, smax = 5, sstep = 0.1, speed = 1.0;
    double sx = 40, sy = 40;        // 轨道左端位置（画布位图 px）
    bool animating = false;
    int animDir = 1;                // 动画方向（到端点折返）

    // N6 积分填充（Integral(f,a,b) 产物）：渲染时在 [a,b] 画曲线与 x 轴之间
    // 的填充多边形；calc 重算时同步刷新 a/b（滑动条边界）
    std::shared_ptr<GeoFillState> fill;

    void update() override
    {
        if (calc) value = calc();
    }
    std::string valueText() const override { return FormatNum(value); }
    std::string definitionText() const override
    {
        if (!cmdName.empty()) return cmdDefText();
        if (!calc && !cmd.empty()) return label + " = " + cmd;
        return cmd.empty() ? FormatNum(value) : label + " = " + cmd;
    }
};

// 角（度数 0..360，从 BA 逆时针到 BC；画布上画弧）。上游 Angle(A,B,C)。
class GeoAngle : public GeoNumeric {
public:
    GeoAngle() : GeoNumeric(GeoType::Angle)
    {
        isFree = false;
        visible = true;   // 需要在画布画弧（数值类默认隐藏）
    }
    GeoPoint *va = nullptr, *vb = nullptr, *vc = nullptr;   // 顶点 vb
    // 角扇形填充不透明度 0-1（0 = 不画扇形填充，上游 角默认带浅填充）
    double fillAlpha = 0.12;
    std::function<void(GeoAngle *)> acalc;
    void update() override { if (acalc) acalc(this); }
    std::string valueText() const override { return FormatNum(value) + "°"; }
    std::string definitionText() const override
    {
        std::string d = cmdDefText();
        return d.empty() ? GeoNumeric::definitionText() : d;
    }
};

// 射线（A 出发经 B）。绘制时沿方向延伸出画布，由画布裁剪。
class GeoRay : public GeoElement {
public:
    GeoRay() : GeoElement(GeoType::Ray) {}
    GeoPoint *p1 = nullptr, *p2 = nullptr;
    std::string valueText() const override;
    std::string definitionText() const override;
};

// N6 数值列表（{1,2,3} 字面量或 Sequence/Sort 等命令产物）。不绘制，进代数区，
// 是统计命令（Mean/Median/...）的输入。元素为双精度数。
class GeoList : public GeoElement {
public:
    GeoList() : GeoElement(GeoType::List) { visible = false; }
    std::vector<double> values;
    std::string src;   // 自由列表的原始花括号文本（定义列回填用）
    std::function<void(GeoList *)> calc;   // 依随列表（Sort/Unique/Reverse 等）重算

    void update() override { if (calc) calc(this); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

// N6 文本对象：Text("...", P) 在画布给定位置显示字符串。anchor 依附点存在
// 时跟随点移动，否则为静态文本（默认落点 (1,1)）。
// 批次⑤ 动态文本：Text("a="+a) / Text(b)——segs 为 (字面量?, 内容) 分段，
// refs 与非字面段一一对应；update() 时用被引对象当前 valueText 重拼 text，
// 滑动条/拖点驱动重算即自动刷新。
class GeoText : public GeoElement {
public:
    GeoText() : GeoElement(GeoType::Text) { color = {0x1C, 0x1C, 0x1F}; lineThickness = 3; }
    double x = 1, y = 1;             // 世界坐标
    std::string text;                 // 显示内容
    GeoPoint *anchor = nullptr;       // 依附点（可空）
    std::vector<std::pair<bool, std::string>> segs;
    std::vector<GeoElement *> refs;

    void update() override
    {
        if (anchor) {
            x = anchor->x;
            y = anchor->y;
        }
        if (!segs.empty()) {
            std::string s;
            size_t k = 0;
            for (const auto &seg : segs) {
                if (seg.first) s += seg.second;
                else if (k < refs.size()) s += refs[k++]->valueText();
            }
            text = s;
        }
    }
    std::string valueText() const override { return text; }
    std::string definitionText() const override
    {
        std::string d = cmdDefText();
        return d.empty() ? text : d;
    }
};

// 向量（A→B，画箭头）。Vector(点, 向量)（相等向量）走 image 缓存（同
// GeoSegment 变换像模式）：终点不是构造对象，几何取缓存。
class GeoVector : public GeoElement {
public:
    GeoVector() : GeoElement(GeoType::Vector) { color = {51, 51, 204}; }
    GeoPoint *p1 = nullptr, *p2 = nullptr;
    // 相等向量：起/终点缓存（image=true 时几何取缓存而非 p1/p2）
    bool image = false;
    double ix1 = 0, iy1 = 0, ix2 = 0, iy2 = 0;
    std::function<void(GeoVector *)> calc;

    double ex1() const { return image ? ix1 : p1->x; }
    double ey1() const { return image ? iy1 : p1->y; }
    double ex2() const { return image ? ix2 : p2->x; }
    double ey2() const { return image ? iy2 : p2->y; }
    void update() override { if (calc) calc(this); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

// 多边形（顶点序即边序；值 = 面积）。变换像走 ivx/ivy 缓存。
class GeoPolygon : public GeoElement {
public:
    GeoPolygon() : GeoElement(GeoType::Polygon) { color = {0x2C, 0x6B, 0xD4}; lineThickness = 3; }
    std::vector<GeoPoint *> verts;
    bool image = false;
    std::vector<double> ivx, ivy;
    // 区域填充不透明度 0-1（上游 多边形默认 ~15%）。代数区选中行滑杆以
    // "透明度 = 100×(1−fillAlpha)" 呈现：透明度 100 = 填充消失，0 = 最深
    double fillAlpha = 0.15;
    std::function<void(GeoPolygon *)> calc;
    void update() override { if (calc) calc(this); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

} // namespace dreamember
#endif // DREAMEMBER_KERNEL_GEOS_H
