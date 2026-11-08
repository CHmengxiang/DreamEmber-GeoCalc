// geos.h — GeoElement 基础类族（点/线段/直线/射线/向量/圆/多边形/函数/数值/角）
#ifndef DREAMEMBER_KERNEL_GEOS_H
#define DREAMEMBER_KERNEL_GEOS_H

#include <cmath>
#include <functional>
#include <string>
#include <vector>
#include "kernel/expr.h"
#include "kernel/view.h"

namespace dreamember {

enum class GeoType { Point, Segment, Line, Ray, Vector, Circle, Polygon, Function, Numeric, Angle, List, Text,
                     Surface, Sphere, Cylinder, Line3, Plane };

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
// ax+by+cz=d 的展示文本（1.0.8.2 平面：零系数跳过、±1 省略数值，全零写 "0 = d"）
std::string PlaneEqText(double a, double b, double c, double d);

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
// 1.0.8.1：z + is3D 扩展三维点（(x,y,z) 元组/3D 命令产物）；is3D=false 时
// z 恒 0，2D 渲染/命中不受影响，3D 视图按 z 绘制
class GeoPoint : public GeoElement {
public:
    GeoPoint() : GeoElement(GeoType::Point) { color = {77, 77, 255}; }
    double x = 0, y = 0;
    double z = 0;                   // 1.0.8.1 三维 z（is3D=false 恒 0）
    bool is3D = false;              // 三维点标记（(x,y,z) 解析/Sphere 等命令置位）
    int pointSize = 5;              // 上游 DEFAULT_POINT_SIZE
    bool isFree = true;
    // 依随点重算闭包（Intersect/变换像等；calc 为空时退回 Midpoint 语义）
    std::function<void(GeoPoint *)> calc;
    // 四十五包 路径依附点（附着/脱离点工具；Point(path[, param]) 命令）。
    // path 非空时 isFree=false、calc 由内核按路径参数化装配，位置 =
    // PathPointAt(path, pathParam)。参数语义随路径：线段/向量 t∈[0,1]，
    // 直线/射线=自锚点的有向距离（ℝ / [0,∞)），圆=弧度角 θ，函数=x，
    // 多边形=边序号+t（整数部分为边序号）。path 进 inputs（级联删除/重算）。
    GeoElement *path = nullptr;
    double pathParam = 0;

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
    // 1.0.7.1 链式不等式（a<x<b 官方语义 = 各段条件的 AND 区域）：每段
    // v_k op v_{k+1} 归一化为 s = L−R 的符号条件，lt=true 区域在 s<0 侧；
    // 渲染 g = max(各段归一化值) < 0（连续，复用两变量列扫描机制）。非空时
    // ineqOp/ineqVertical/ineqTwoVar 均不使用
    struct IneqCond {
        Expr l, r;
        bool lt = true;      // true: s<0 在区域内（op 为 < / ≤）
        bool strict = true;  // 严格不等（< / >）：边界虚线
    };
    std::vector<IneqCond> ineqChain;
    // 1.0.7.2 ∧/∨ 组合区域：DNF 句列表——句内条件 AND（与链式同构）、句间
    // OR（并集）。渲染 g=min(句内 max(归一化值))<0 复用两变量列扫描；非空时
    // ineqChain/ineqOp/ineqVertical/ineqTwoVar 均不使用
    std::vector<std::vector<IneqCond>> ineqDNF;
    // 链式/组合区域判据（ineqOp 之外）：1.0.7.1 链式 + 1.0.7.2 ∧/∨ 组合
    bool ineqCombo() const { return !ineqChain.empty() || !ineqDNF.empty(); }
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
    // 1.0.6.4 布尔对象：定义式顶层为比较/逻辑（a>2、b∧c）的依随数值，
    // 值列显示 true/false（内部仍 1/0 存储，参与运算同数值）
    bool boolLike = false;
    // 1.0.6.6 角度值标记：官方 radian 文件重放时表达式层的角值引用出
    // 弧度（×π/180）而非内部度数。自由角从文件重建为普通数值对象
    //（1.0.6.5），类型信息丢失，靠此标记补回角语义；GeoAngle 恒真
    bool isAngleVal = false;
    std::function<double()> calc;   // 依随数值的重算闭包（级联删除保证不悬垂）
    std::string cmd;                // 依随时记录的命令原文（"Distance(A, B)"）

    // N5 Slider：画布内滑动条（上游 同款：轨道 + 手柄 + 标签，绝对屏幕坐标 px）
    bool isSlider = false;
    double smin = 0, smax = 5, sstep = 0.1, speed = 1.0;
    double sx = 40, sy = 40;        // 轨道左端位置（画布位图 px）
    bool animating = false;
    int animDir = 1;                // 动画方向（到端点折返）
    // 1.0.6.6 滑条范围对象引用（官方 <slider min="b">）：读取期存原文，
    // setXml 构造重放完成后统一解析为数值对象当前值（静态快照）并重钳位；
    // 保存时原文回写。非空时对应数值侧不写死
    std::string sminRef, smaxRef;

    // N6 积分填充（Integral(f,a,b) 产物）：渲染时在 [a,b] 画曲线与 x 轴之间
    // 的填充多边形；calc 重算时同步刷新 a/b（滑动条边界）
    std::shared_ptr<GeoFillState> fill;

    void update() override
    {
        if (calc) value = calc();
    }
    std::string valueText() const override
    {
        // 1.0.6.6 ∞：IEEE 除零/溢出与 ∞ 常量按官方显示（NAN 维持 "?"）
        if (std::isinf(value)) return value > 0 ? "∞" : "-∞";
        return boolLike ? (value != 0.0 ? "true" : "false") : FormatNum(value);
    }
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
        isAngleVal = true;   // 角对象恒为角度值（1.0.6.6 弧度会话缩放判定）
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
    // 1.0.6.1 矩阵（GGB 的「矩阵 = 列表的列表」，{{1,2},{3,4}} 字面量）：
    // 行主序扁平存 values，isMatrix 标记 + 维度。Transpose/Invert/Row 等
    // 矩阵命令与元素寻址按此解释；普通统计/排序命令对矩阵拒绝
    bool isMatrix = false;
    int mrows = 0, mcols = 0;
    std::string src;   // 自由列表的原始花括号文本（定义列回填用）
    std::function<void(GeoList *)> calc;   // 依随列表（Sort/Unique/Transpose 等）重算

    // 1.0.6.3 图表列表（BarChart/Histogram/LineGraph/ScatterPlot/BoxPlot/
    // StemPlot 产物）：chart 标记绘制形态（空 = 普通列表，不绘制）。
    // chartX = 横坐标/组界（bar 两列表形式与 hist 的组界、line/scatter 的
    // x 坐标；空时 bar 按 1..n 定位）；chartA/chartB = BoxPlot 的中心 y 与
    // 箱高（StemPlot 复用 chartA 存 adjustment）。box 的 values 存五数概括
    // {min,Q1,中位数,Q3,max}，stem 的 values 存调整后的排序数据
    std::string chart;
    std::vector<double> chartX;
    double chartA = 0, chartB = 1;

    // 1.0.7.3 点列表第一期：元素为点。points/pointRefs 一一对应（pointRefs
    // 空串 = 字面坐标 (a,b)，静态；非空 = 引用点标签，calc 按点现值刷坐标，
    // inputs 挂引用点级联重算）。values 保持空；数值/统计命令对点列表拒绝
    std::vector<std::pair<double, double>> points;
    std::vector<std::string> pointRefs;
    bool isPointList() const { return !points.empty(); }

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

// ---------- 1.0.8.1 三维对象族（3D 视图一期） ----------
// 曲面 z = f(x,y)（f(x,y)=expr / z=expr 定义）。y 是表达式层原生第二变量
//（批次⑥ 已支持 evalXY），曲面求值直接走 evalXY。网格分辨率在渲染端。
class GeoSurface : public GeoElement {
public:
    GeoSurface() : GeoElement(GeoType::Surface) { color = {0x2C, 0x6B, 0xD4}; }
    std::string src;            // 规范化后的表达式原文（定义列回填用）
    Expr body;
    // 填充不透明度（曲面网格四边形填充；上游 3D 曲面默认较淡）
    double fillAlpha = 0.45;
    // 滑条/数值引用（refs 模式解析；inputs 同步注册保证级联删除与重算，
    // 渲染/求值按现值随动——1.0.7.1 滑条动态联动同机制）
    std::vector<std::pair<std::string, GeoNumeric *>> refDeps;
    // 1.0.8.2 参数曲面（Surface(X,Y,Z,u,u1,u2,v,v1,v2)）：分量表达式 refs
    // 模式解析，uName/vName 与滑条引用统一走 resolver 查值回调（复用
    // evalXYResolved，无 Expr API 改动；分量含原生 x/y 直接拒绝）
    bool parametric = false;
    std::string uName = "u", vName = "v";
    Expr ex, ey, ez;                    // x/y/z 分量（parametric 时有效）
    double u0 = 0, u1 = 1, v0 = 0, v1 = 1;   // 参数范围（仅数值字面量）
    double evalAt(double x, double y) const
    {
        if (refDeps.empty()) return body.evalXY(x, y);
        return body.evalXYResolved(x, y,
            [this](const std::string &nm) -> double {
                for (const auto &d : refDeps) {
                    if (d.first == nm) return d.second->value;
                }
                return NAN;
            });
    }
    // 参数曲面分量求值：u/v/滑条引用按名回填（滑条现值随动）
    void evalUV(double u, double v, double &ox, double &oy, double &oz) const
    {
        auto res = [this, u, v](const std::string &nm) -> double {
            if (nm == uName) return u;
            if (nm == vName) return v;
            for (const auto &d : refDeps) {
                if (d.first == nm) return d.second->value;
            }
            return NAN;
        };
        ox = ex.evalXYResolved(0.0, 0.0, res);
        oy = ey.evalXYResolved(0.0, 0.0, res);
        oz = ez.evalXYResolved(0.0, 0.0, res);
    }
    bool definable() const { return parametric || !body.empty(); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

// 球面（Sphere(M, r) / Sphere(M, A)）：M 为三维中心点（引用），半径取
// 数值/数值引用或过第二点距离。渲染 lat/long 线框。
class GeoSphere : public GeoElement {
public:
    GeoSphere() : GeoElement(GeoType::Sphere) { color = {0x2C, 0x6B, 0xD4}; }
    GeoPoint *center = nullptr;         // 三维中心（inputs 注册，级联删除保护）
    GeoPoint *through = nullptr;        // 过点形式（与 radius/radiusNum 三选一）
    GeoNumeric *radiusNum = nullptr;    // 半径引用数值对象（随其重算）
    double radius = 0;
    std::function<void(GeoSphere *)> calc;   // 依随重算（中心/过点/半径引用变动）
    void update() override { if (calc) calc(this); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

// 圆柱（Cylinder(M, r, h)：底面中心 M、半径 r、沿 z 高 h；Cylinder(A, B, r)：
// 轴线 A→B 半径 r）。渲染 = 底/顶圆线框 + 母线。top 缓存依随重算。
class GeoCylinder : public GeoElement {
public:
    GeoCylinder() : GeoElement(GeoType::Cylinder) { color = {0x2C, 0x6B, 0xD4}; }
    GeoPoint *base = nullptr;           // 底面中心（三维点）
    GeoPoint *topPt = nullptr;          // 轴线形式顶心（可空）
    GeoNumeric *radiusNum = nullptr;    // 半径引用（可空）
    GeoNumeric *heightNum = nullptr;    // 高引用（可空）
    double radius = 0, height = 0;      // 静态值（顶心形式下 height=|AB|）
    std::function<void(GeoCylinder *)> calc;
    void update() override { if (calc) calc(this); }
    std::string valueText() const override;
    std::string definitionText() const override;
};

// 空间直线（Line(A,B) 两端为三维点时产物）：参数式 P = p0 + t·d，渲染按
// 屏幕范围裁剪（t 端点由投影反向解出）。与 2D GeoLine 方程式并存互不影响。
class GeoLine3 : public GeoElement {
public:
    GeoLine3() : GeoElement(GeoType::Line3) { color = {0, 0, 0}; }
    GeoPoint *p1 = nullptr, *p2 = nullptr;
    std::string valueText() const override;
    std::string definitionText() const override;
};

// 平面一般式 a·x+b·y+c·z=d（1.0.8.2）。来源：Plane 三点式（pa/pb/pc 引用
// 随点重算）、Plane(点, 法向量)（calc 闭包重解）、隐式线性方程 x+y+z=d
//（裸输入或 Plane(lhs,rhs) 命令式，采样提取系数，滑条引用随动）、常值曲面
// f(x,y)=k / z=k（GGB 口径为平面，a=b=0, c=1）。渲染按视图立方体裁剪。
class GeoPlane : public GeoElement {
public:
    GeoPlane() : GeoElement(GeoType::Plane) { color = {0x2C, 0x6B, 0xD4}; }
    double a = 0, b = 0, c = 0, d = 0;   // 一般式系数（法向未归一化）
    double fillAlpha = 0.25;             // 半透明面填充（样式条透明度滑杆）
    std::string src;                     // 隐式/常数式定义原文（定义列回填用）
    GeoPoint *pa = nullptr, *pb = nullptr, *pc = nullptr;   // 三点式
    std::function<void(GeoPlane *)> calc;  // 依随重算（点/向量/滑条变动）
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
    // 1.0.8.2 三维多边形（Polygon 收到任一 is3D 顶点）：2D 视图跳绘，
    // render3D 按顶点现值投影填充（Newell 法向光照），面积走叉积和
    bool is3D = false;
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
