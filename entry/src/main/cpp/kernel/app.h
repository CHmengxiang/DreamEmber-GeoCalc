// app.h — 原生内核控制器：构造列表 + 命令分发 + 视图状态 + 绘图指令输出
#ifndef DREAMEMBER_KERNEL_APP_H
#define DREAMEMBER_KERNEL_APP_H

#include <memory>
#include <string>
#include <vector>
#include "kernel/geos.h"
#include "kernel/view.h"

namespace dreamember {

// 一条绘图指令（C++ -> ArkTS Canvas 的轻量协议，避免每帧过 NAPI 传对象树）
enum class DrawOp {
    Clear = 0,
    Polyline,    // pts[](px,py)
    Segment,     // 两点
    MultiSeg,    // 成对短划：pts 每 4 个为一段（moveTo+lineTo），不连成折线
    CirclePix,   // 圆心+半径（像素）
    FillCircle,  // 实心圆
    Text,        // 文本
    Arc,         // 圆弧（角度工具）：cx,cy,r,start,end（canvas 弧度）
    FillPoly,    // 多边形填充：pts 成对顶点
    TextHalo,    // 文本选中底衬（strokeText 描边）
};

struct DrawCmd {
    DrawOp op;
    std::vector<float> pts;    // Polyline: 2n 个；Segment: 4 个；Circle/FillCircle: cx,cy,r；
                               // Arc: cx,cy,r,start,end；Text: x,y
    std::string text;          // Text 内容或颜色（Polyline/Segment/Circle 用 color 字段）
    std::string color;
    float lineWidth = 1.7f;    // 上游：lineThickness/3（真机反馈：线条略细）
};

struct Geometry {
    double x = 0, y = 0, r = 0;
};

// 代数区一行：label|type|value|definition
struct AlgebraRow {
    std::string label;
    std::string type;        // point/segment/line/ray/vector/circle/polygon/function/numeric/angle/slider
    std::string value;
    std::string definition;
    // 行级状态由 algebraRows 直填（不按 label 回查——历史重复标签时回查会
    // 错位到别的对象）：可见性 / 画布选中 / 不等式填充透明度百分比
    bool visible = true;
    bool selected = false;
    int fillAlphaPct = -1;   // -1 = 无填充（不显示透明度滑杆）
};

class Kernel {
public:
    Kernel() { geos_.reserve(64); }

    EuclidianView view;
    std::string lastError;
    // 显示开关（N3：设置面板控制；刻度/轴数字随坐标轴一起开关）
    bool showGrid = true;
    bool showAxes = true;
    // 深色画布（N3：跟随系统色深；近黑对象反白保证可读性）
    bool darkTheme = false;

    // 输入一条代数式（用户表达式或命令）。返回是否成功创建对象。
    bool input(const std::string &src);
    // 删除末尾对象（含其全部依赖者，级联）
    void deleteLast();
    // 按 label 删除（代数区操作）；级联删除依赖者。找不到返回 false
    bool deleteByLabel(const std::string &label);
    // 重置视图到标准状态（原点居中、SCALE_STANDARD）
    void resetView() { view.reset(); }
    // 新建：清空全部对象并重置视图（上游 File->New 语义）
    void clearAll();
    // 对象是否隐藏在代数区可见（valueText 永远可得，hidden 只影响画布）
    const std::vector<std::unique_ptr<GeoElement>> &geos() const { return geos_; }

    // 代数区数据
    void algebraRows(std::vector<AlgebraRow> &out) const;
    // 代数区行协议 v2 文本（序列化收进内核，host 测试与 NAPI 共用同一实现）：
    // label|type|value|def|vis|sel[|k=v;k=v]。前六列固定语义；第 7 列起为
    // key=value 对（分号分隔），属性追加不再按对象类型漂移位置列：
    // slider → anim/min/max/step；不等式区域/多边形/角扇形 → fill（百分比）
    std::string algebraText() const;

    // 生成一帧绘制指令（世界->像素转换在本层完成）
    void render(std::vector<DrawCmd> &out);

    // 命中测试：找像素 (px,py) 附近的可拖动自由点；返回 nullptr 表示无。
    // outD 非空时回写命中距离（无命中 1e9）——工具层点/对象就近比较用
    GeoPoint *pickPoint(double px, double py, double *outD = nullptr);
    // N5 对象拾取（长按菜单/工具用）：点以外的最近对象（线段/直线/射线/向量/
    // 圆/多边形/函数）；不在线上返回 nullptr
    GeoElement *pickObject(double px, double py);
    // N5 滑动条手柄拾取（拖动改值）
    GeoNumeric *pickSliderHandle(double px, double py);
    void dragSliderTo(GeoNumeric *n, double px);
    // 拾取的游离状态（kernelPickPoint / kernelDragPointTo 成对使用）
    GeoPoint *pickedPoint() const { return picked_; }
    void clearPicked() { picked_ = nullptr; }
    // 拖动点后重算依随对象（四十五包：路径依附点按投影落轨）
    void movePoint(GeoPoint *p, double wx, double wy);

    // 四十五包 附着/脱离点：p 附到 path 上（参数=指针位置投影）或脱离回
    // 自由点（保留当前坐标）。重算在内部完成，撤销快照由调用点打
    bool attachPoint(GeoPoint *p, GeoElement *path, double wx, double wy);
    bool detachPoint(GeoPoint *p);

    // N5 点选式工具（上游 工具栏模式）：工具 id 见 toolTap；"" = 选择/移动。
    // 换工具即复位待选状态并清除工具拾取的选中特效。二十六包：参数待输入
    // 态（askKind_/askObj_/askPts_）一并复位——弹窗取消=重入同工具复位
    void setTool(const std::string &tool)
    {
        tool_ = tool;
        toolPts_.clear();
        toolObj_ = nullptr;
        askKind_.clear();
        askObj_ = nullptr;
        askPts_.clear();
        for (auto &g : geos_) g->selected = false;
    }
    std::string tool() const { return tool_; }
    // 画布点按一次（已换算世界/像素坐标）。返回 "done|标签,.." / "need|提示" /
    // "err|原因" / "ask|提示"（二十六包：拾取齐了差一个参数——宿主弹参数
    // 框，确认后调 toolParam(v) 完成构造；工具保持激活（上游 行为），换工具
    // 时 setTool 复位。中间拾取的对象点亮 selected 特效，done/err/取消时
    // 统一清除，ask 态保持点亮
    std::string toolTap(double wx, double wy, double px, double py);
    // 二十六包 工具第二批：toolTap 返回 "ask|" 后由宿主把用户输入的参数
    // （数字或对象引用，原样进命令表达式）送回这里完成构造。返回协议同
    // toolTap 的 "done|" / "err|"；失败保留 ask 态（弹窗不关、可改再确认），
    // 成功清 ask 态并清点亮。无待输入态返回 "err|"
    std::string toolParam(const std::string &v);

    // 重命名：改标签并同步所有命令参数引用。返回空串=成功，否则错误信息
    std::string rename(const std::string &oldLabel, const std::string &newLabel);
    // 重定义：保持标签与指针身份，用新表达式替换对象定义（上游 redefine）
    bool redefine(const std::string &label, const std::string &expr);
    // 设置滑动条值（步长吸附+钳位，级联重算）
    void setSliderValue(GeoNumeric *n, double v);
    // 二十八包⑤：滑动条四参数原子应用（范围/步长落位后值钳位+吸附+重算）。
    // 逐键 setProp 有自动纠偏（min 抬 max 等），两项同改会互相矛盾；此处
    // 一次校验 max>min 后整体落位。失败返回 false（lastError 已填）
    bool setSliderParams(GeoNumeric *n, double smin, double smax, double sstep,
        double v);

    // N5 显隐 / 动画 / 选中
    bool setVisible(const std::string &label, bool vis);
    bool animateToggle(const std::string &label);   // 返回 toggle 后是否在动画
    bool animTick(double dt);                       // 有 slider 推进返回 true
    bool anyAnimating() const;
    // 画布选中特效（单选；label 空串清除）。渲染时对选中对象加亮色加宽描边
    void setSelected(const std::string &label);

    // 通用属性 API（轮 A 防乱重构：之后新增对象属性不再各开一条 NAPI 导出/
    // 协议列，UI 属性面板统一走这两个入口）
    // 对象属性快照 "k=v;k=v"（type/vis/sel/color + fill/min/max/step/anim 按
    // 对象提供）。不含 value/definition：文本可能含 '=' 或 ';'，会破坏
    // key=value 结构。找不到对象返回空串
    std::string propsOf(const std::string &label) const;
    // 写属性。key ∈ {vis, fill, value, min, max, step, anim, color}，val 统一
    // 字符串（数值/布尔由本层解析）。未知对象/键返回 false，lastError 已填
    bool setProp(const std::string &label, const std::string &key, const std::string &val);

    // 二十三包 表格视图批量求值（上游 TableValuesView：行=x 取值、列=函数/
    // 非水平直线 f(x)）。labels/xs 均逗号分隔；返回行协议：行间 ';'、格间
    // ','。不可求值列（不等式/竖直线/未知对象）与非有限值一律输出空格串
    std::string tableEval(const std::string &labelsCsv, const std::string &xsCsv);

    // label → 元素查找（找不到 nullptr）
    GeoElement *lookup(const std::string &label) const;
    GeoPoint *lookupPoint(const std::string &label) const;

    // N4 .ggb 读写：构造列表 ⇄ GeoGebra 兼容 XML（子集；N5 起含射线/向量/
    // 多边形/角/滑动条/交点/变换像，命令经 cmdName/cmdArgs 通用重放）
    std::string getXml() const;
    // 从 XML 重建（先清空）。支持的命令照常重放；不支持的命令及其依赖对象跳过，
    // 跳过数经 skippedOut 返回（可为 nullptr）。返回 false 时 lastError 已填
    bool setXml(const std::string &xml, int *skippedOut);

    // 轮 B 撤销/重做：XML 快照栈（容量 50，快照 = 变更前 getXml()）。
    // 离散变更（输入/重定义/删除/改名/显隐/清空/工具作图）由内核在变更点
    // 自动打点，且只有变更成功才入栈（失败输入不留步）；连续变更（拖点/
    // 滑杆拖动/透明度拖动）内核无法分辨手势边界，由宿主在首次变更前调
    // pushUndo() 打点。.ggb 打开经 clearUndoHistory() 清栈（上游 语义）
    void pushUndo();               // 宿主手动打点（清空重做栈）
    bool undoStep();               // 回退一步；空栈返回 false
    bool redoStep();               // 前进一步；空栈返回 false
    void clearUndoHistory();
    int undoDepth() const { return static_cast<int>(undoStack_.size()); }
    int redoDepth() const { return static_cast<int>(redoStack_.size()); }

private:
    std::vector<std::unique_ptr<GeoElement>> geos_;
    GeoPoint *picked_ = nullptr;

    // 撤销/重做快照栈（存变更前的完整 XML）
    std::vector<std::string> undoStack_;
    std::vector<std::string> redoStack_;
    int undoSuspend_ = 0;   // >0 = setXml 重放/redefine 内部 input 期间不自动打点
    // 轮D 反馈③：inputImpl refs 分支自动物化的空单元格数（redefine 的
    // "恰好一个对象"校验与 made 定位需计入；inputImpl 入口清零）
    int autoCells_ = 0;
    void commitUndoSnapshot(const std::string &pre);
    bool inputImpl(const std::string &srcRaw);   // input 的解析主体（打点在包装层）

    // N5 点选式工具状态
    std::string tool_;
    std::vector<GeoPoint *> toolPts_;
    GeoElement *toolObj_ = nullptr;

    // 二十六包 工具第二批参数待输入态：kind=工具名（rotate/dilate/
    // circleRadius/segLength/angleSize，空=无）；askObj_=变换对象（可空），
    // askPts_=已拾取点（旋转/位似=中心，定角=边点+顶点，圆半径/定长=锚点）。
    // 弹窗期间 canvas 点按被宿主 scrim 挡住，这里仍防御悬空（toolParam 前
    // 经 toolListed 校验）
    std::string askKind_;
    GeoElement *askObj_ = nullptr;
    std::vector<GeoPoint *> askPts_;

    // 多输出命令（Intersect/Tangent 无名创建）的额外对象，add 后统一入列
    std::vector<std::unique_ptr<GeoElement>> extras_;

    std::string nextLabelFor(GeoType t);
    void recomputeAll();
    // 级联删除：把引用 dead 的依赖者一并移除（循环直到无人引用）
    void cascadeErase(GeoElement *dead);
    // 添加对象并重算
    GeoElement *add(std::unique_ptr<GeoElement> made, const std::string &name);
    // extras_ 自动命名并入构造列表（多输出命令收尾）
    void flushExtras();
    // 工具点按时空白处先建自由点（上游 行为）
    GeoElement *makeFreePoint(double wx, double wy);
    std::string toolTapImpl(double wx, double wy, double px, double py);
    // ask 态防御：指针仍在构造列表（未被级联删除）
    bool toolListed(GeoElement *e) const;
    void eraseGeo(GeoElement *e);
    // 两对象求交：全部交点（≤cap 个）按序输出；成功返回 true
    bool intersectAll(GeoElement *o1, GeoElement *o2,
                      std::vector<std::pair<double, double>> &pts, int cap = 16);
    // 第 nth 个交点（0 起）
    bool intersectAt(GeoElement *o1, GeoElement *o2, int nth, double &ox, double &oy);
    // 变换像构造：obj 的像对象（点/线段/直线/圆/多边形），不支持时返回空。
    // map=点映射；rmul=半径倍率（Dilate 用，缺省恒 1）；extraDep/extraDep2=额外依赖
    // （中心点/向量/动态数值——进 inputs 保证级联删除与重算）
    using MapFn = std::function<std::pair<double, double>(double, double)>;
    std::unique_ptr<GeoElement> makeImage(GeoElement *obj, const std::string &cmd,
                                          const std::vector<std::string> &args, const MapFn &map,
                                          const std::function<double(double)> &rmul = std::function<double(double)>(),
                                          GeoElement *extraDep = nullptr,
                                          GeoElement *extraDep2 = nullptr);
    // 滑动条落位：按已有 slider 数堆叠（画布左上区域）
    void placeSlider(GeoNumeric *n);

    // 命令处理：cmd=命令名（保留大小写前的形式），args=逗号分隔参数
    // 命中返回 true（成功与否看 lastError 与返回对象）
    bool tryCommand(const std::string &cmdLower, const std::string &args,
                    const std::string &name, std::unique_ptr<GeoElement> &made);

    // N4 .ggb XML 重放辅助
    void xmlApplyStyle(GeoElement *e, const std::string &inner);
    // headTag = <command ...> 开标签属性文本；body = 开标签之后的内部内容
    void xmlCommand(const std::string &headTag, const std::string &body, int *skipped);
    void xmlElement(const std::string &type, const std::string &label,
                    const std::string &inner, int *skipped);

    static Geometry CircleGeom(const GeoCircle &c, const EuclidianView &v);
    void appendFunctionPlot(const GeoFunction &f, std::vector<DrawCmd> &out,
                            bool sel = false) const;
    // N6 批次⑤：不等式区域（填充 + 虚线/实线边界，含竖直半平面）
    void appendInequalityPlot(const GeoFunction &f, std::vector<DrawCmd> &out) const;
    void appendGrid(std::vector<DrawCmd> &out) const;
    void appendTickMarks(std::vector<DrawCmd> &out) const;
};

} // namespace dreamember
#endif // DREAMEMBER_KERNEL_APP_H
