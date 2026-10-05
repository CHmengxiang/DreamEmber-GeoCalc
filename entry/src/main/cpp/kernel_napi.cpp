// kernel_napi.cpp — kernel 的 NAPI 桥：对象列表/绘图指令/手势 全部走轻量字符串协议
// 注意：与 napi_init.cpp 同属一个模块（"dreamember"），由 DreamemberRegisterKernelApis
// 把本文件的导出函数并入同一张属性表，避免一个 .so 注册两个模块名。
#include "napi/native_api.h"
#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include "kernel/app.h"
#include "kernel/ggbfile.h"

void DreamemberRegisterKernelApis(napi_env env, napi_value exports);

namespace {

dreamember::Kernel g_kernel;
dreamember::GeoPoint *g_dragPoint = nullptr;    // 当前被单指拖动的自由点（NAPI 侧持有，跨调用有效）
dreamember::GeoNumeric *g_dragSlider = nullptr; // N5 当前被拖动的滑动条

std::string NapiToString(napi_env env, napi_value v)
{
    size_t len = 0;
    napi_get_value_string_utf8(env, v, nullptr, 0, &len);
    std::string s(len + 1, '\0');
    napi_get_value_string_utf8(env, v, &s[0], len + 1, &len);
    s.resize(len);
    return s;
}

napi_value ToNapi(napi_env env, const std::string &s)
{
    napi_value out;
    napi_create_string_utf8(env, s.c_str(), s.size(), &out);
    return out;
}

// ---- 协议编码 ----
// 行格式（\n 分隔）：op|color|lineWidth|[text|]x1,y1,...
// op: c=clear s=segment m=成对短划 p=polyline o=circle f=fillCircle t=text
//     w=text 描边（选中底衬）
void AppendCmd(std::string &out, const dreamember::DrawCmd &c)
{
    char op = 'p';
    switch (c.op) {
    case dreamember::DrawOp::Clear: op = 'c'; break;
    case dreamember::DrawOp::Polyline: op = 'p'; break;
    case dreamember::DrawOp::Segment: op = 's'; break;
    case dreamember::DrawOp::MultiSeg: op = 'm'; break;
    case dreamember::DrawOp::CirclePix: op = 'o'; break;
    case dreamember::DrawOp::FillCircle: op = 'f'; break;
    case dreamember::DrawOp::Text: op = 't'; break;
    case dreamember::DrawOp::TextHalo: op = 'w'; break;
    case dreamember::DrawOp::Arc: op = 'a'; break;
    case dreamember::DrawOp::FillPoly: op = 'y'; break;
    }
    out.push_back(op);
    out.push_back('|');
    out += c.color;
    out.push_back('|');
    out += std::to_string(c.lineWidth);
    out.push_back('|');
    if (c.op == dreamember::DrawOp::Text || c.op == dreamember::DrawOp::TextHalo) {
        out += c.text;
        out.push_back('|');
    }
    for (size_t i = 0; i < c.pts.size(); ++i) {
        char b[32];
        snprintf(b, sizeof(b), "%g", (double)c.pts[i]);
        out += b;
        out.push_back(i + 1 < c.pts.size() ? ',' : '\n');
    }
    if (c.pts.empty()) out.push_back('\n');
}

// 对象列表（代数区）：label|type|value|definition|vis|sel[|k=v;k=v]。
// 序列化在内核 algebraText()（轮 A 协议 v2：第 7 列起 key=value 对，
// 不再按对象类型漂移位置列），绑定层只做转发
void AppendGeoList(std::string &out)
{
    out += g_kernel.algebraText();
}

std::string RenderFrame()
{
    std::vector<dreamember::DrawCmd> frame;
    g_kernel.render(frame);
    std::string out;
    out.reserve(4096);
    for (const auto &c : frame) AppendCmd(out, c);
    return out;
}

// ---- 导出的 NAPI 函数 ----

// kernelInput(expr: string): string — 错误时返回 "error: ..."，成功返回对象列表
napi_value KernelInput(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1 || argv[0] == nullptr) {
        napi_throw_error(env, nullptr, "kernelInput requires a string");
        return nullptr;
    }
    std::string expr = NapiToString(env, argv[0]);
    if (!g_kernel.input(expr)) {
        return ToNapi(env, "error: " + (g_kernel.lastError.empty() ? "无法解析输入" : g_kernel.lastError));
    }
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// kernelDeleteLast(): string — 返回对象列表
napi_value KernelDeleteLast(napi_env env, napi_callback_info)
{
    g_kernel.deleteLast();
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// kernelDeleteObj(label): string — 删除指定对象（级联删除依赖者），返回对象列表
napi_value KernelDeleteObj(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1 || argv[0] == nullptr) {
        napi_throw_error(env, nullptr, "kernelDeleteObj requires a string label");
        return nullptr;
    }
    g_kernel.deleteByLabel(NapiToString(env, argv[0]));
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// kernelAlgebra(): string — 代数区快照（label|type|value|definition），不动视图不重绘
napi_value KernelAlgebra(napi_env env, napi_callback_info)
{
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// kernelRender(w, h, uiScale, dark): string — 画布尺寸（位图px）+ vp/px 比例
// + 深色主题，输出一帧绘图指令
napi_value KernelRender(napi_env env, napi_callback_info info)
{
    size_t argc = 4;
    napi_value argv[4] = {nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelRender requires (width, height, uiScale?, dark?)");
        return nullptr;
    }
    double w = 0, h = 0, us = 1.0;
    napi_get_value_double(env, argv[0], &w);
    napi_get_value_double(env, argv[1], &h);
    if (argc >= 3 && argv[2] != nullptr) {
        napi_get_value_double(env, argv[2], &us);
    }
    if (argc >= 4 && argv[3] != nullptr) {
        bool dark = false;
        napi_get_value_bool(env, argv[3], &dark);
        g_kernel.darkTheme = dark;
    }
    g_kernel.view.setCanvasSize((int)w, (int)h, us);
    return ToNapi(env, RenderFrame());
}

// kernelPickPoint(px, py): string — 命中自由点则记住并返回 "+|label"，否则 "-|"
// （坐标为 Canvas 位图像素；ArkTS 侧负责 vp→px 换算）
napi_value KernelPickPoint(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelPickPoint requires (px, py)");
        return nullptr;
    }
    double px = 0, py = 0;
    napi_get_value_double(env, argv[0], &px);
    napi_get_value_double(env, argv[1], &py);
    g_dragPoint = g_kernel.pickPoint(px, py);
    if (g_dragPoint) return ToNapi(env, "+|" + g_dragPoint->label);
    return ToNapi(env, "-|");
}

// kernelDragPointTo(px, py): string — 把拾取的点拖到像素位置（内部转世界坐标），返回重绘帧
napi_value KernelDragPointTo(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelDragPointTo requires (px, py)");
        return nullptr;
    }
    double px = 0, py = 0;
    napi_get_value_double(env, argv[0], &px);
    napi_get_value_double(env, argv[1], &py);
    if (g_dragPoint) {
        g_kernel.movePoint(g_dragPoint, g_kernel.view.wx(px), g_kernel.view.wy(py));
        return ToNapi(env, RenderFrame());
    }
    return ToNapi(env, "-|");
}

// kernelEndDrag(): void — 单指抬起后清除拖动状态
napi_value KernelEndDrag(napi_env env, napi_callback_info)
{
    g_dragPoint = nullptr;
    g_dragSlider = nullptr;
    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
}

// kernelGestureZoom(px, py, factor): string — 以锚点缩放后返回重绘帧
napi_value KernelGestureZoom(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value argv[3] = {nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 3) {
        napi_throw_error(env, nullptr, "kernelGestureZoom requires (px, py, factor)");
        return nullptr;
    }
    double a[3] = {0, 0, 1};
    for (int i = 0; i < 3; ++i) napi_get_value_double(env, argv[i], &a[i]);
    g_kernel.view.zoomAt(a[0], a[1], a[2]);
    return ToNapi(env, RenderFrame());
}

// kernelPan(dx, dy): string — 像素平移后返回重绘帧
napi_value KernelPan(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelPan requires (dx, dy)");
        return nullptr;
    }
    double dx = 0, dy = 0;
    napi_get_value_double(env, argv[0], &dx);
    napi_get_value_double(env, argv[1], &dy);
    g_kernel.view.panBy(dx, dy);
    return ToNapi(env, RenderFrame());
}

// kernelSetOptions(grid, axes): string — 设置面板显示开关，返回重绘帧
napi_value KernelSetOptions(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelSetOptions requires (grid, axes)");
        return nullptr;
    }
    bool grid = true, axes = true;
    napi_get_value_bool(env, argv[0], &grid);
    napi_get_value_bool(env, argv[1], &axes);
    g_kernel.showGrid = grid;
    g_kernel.showAxes = axes;
    return ToNapi(env, RenderFrame());
}

// kernelResetView(): string — 视图回到标准状态（原点居中、SCALE_STANDARD），返回重绘帧
napi_value KernelResetView(napi_env env, napi_callback_info)
{
    g_kernel.resetView();
    return ToNapi(env, RenderFrame());
}

// kernelClearAll(): string — 新建：清空全部对象并重置视图，返回对象列表（空）
napi_value KernelClearAll(napi_env env, napi_callback_info)
{
    g_dragPoint = nullptr;
    g_dragSlider = nullptr;
    g_kernel.clearAll();
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// ---------- N5 工具 / 拾取 / 滑动条 / 动画 ----------

// kernelTool(name): void — 设置当前点选工具（"" = 选择/移动），复位待选状态
napi_value KernelTool(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    std::string tool;
    if (argc >= 1 && argv[0] != nullptr) tool = NapiToString(env, argv[0]);
    g_kernel.setTool(tool);
    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
}

// kernelToolTap(px, py): string — 工具模式下画布点按一次。
// 返回 "done|标签,.." / "need|提示" / "err|原因"
napi_value KernelToolTap(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelToolTap requires (px, py)");
        return nullptr;
    }
    double a[2] = {0, 0};
    for (int i = 0; i < 2; ++i) napi_get_value_double(env, argv[i], &a[i]);
    double wx = g_kernel.view.wx(a[0]);
    double wy = g_kernel.view.wy(a[1]);
    return ToNapi(env, g_kernel.toolTap(wx, wy, a[0], a[1]));
}

// kernelToolParam(v): string — 二十六包 工具第二批：toolTap 返回 "ask|" 后，
// 宿主参数弹窗确认时把输入文本送回内核完成构造。返回 "done|标签,.." / "err|原因"
// （失败保留内核 ask 态，弹窗可改再确认）
napi_value KernelToolParam(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "kernelToolParam requires (value)");
        return nullptr;
    }
    return ToNapi(env, g_kernel.toolParam(NapiToString(env, argv[0])));
}

// kernelPickObj(px, py): string — 长按菜单用：返回 "label|type|vis" 或 "-|"
napi_value KernelPickObj(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelPickObj requires (px, py)");
        return nullptr;
    }
    double a[2] = {0, 0};
    for (int i = 0; i < 2; ++i) napi_get_value_double(env, argv[i], &a[i]);
    dreamember::GeoElement *e = g_kernel.pickObject(a[0], a[1]);
    if (!e) return ToNapi(env, "-|");
    const char *tn = "numeric";
    switch (e->type()) {
    case dreamember::GeoType::Point: tn = "point"; break;
    case dreamember::GeoType::Segment: tn = "segment"; break;
    case dreamember::GeoType::Line: tn = "line"; break;
    case dreamember::GeoType::Ray: tn = "ray"; break;
    case dreamember::GeoType::Vector: tn = "vector"; break;
    case dreamember::GeoType::Circle: tn = "circle"; break;
    case dreamember::GeoType::Polygon: tn = "polygon"; break;
    case dreamember::GeoType::Function: tn = "function"; break;
    case dreamember::GeoType::List: tn = "list"; break;
    case dreamember::GeoType::Text: tn = "text"; break;
    case dreamember::GeoType::Angle: tn = "angle"; break;
    case dreamember::GeoType::Numeric: tn = "numeric"; break;
    }
    return ToNapi(env, e->label + "|" + tn + "|" + (e->visible ? "1" : "0"));
}

// kernelSetVisible(label, vis): string — 显隐对象，返回对象列表
napi_value KernelSetVisible(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelSetVisible requires (label, visible)");
        return nullptr;
    }
    std::string label = NapiToString(env, argv[0]);
    bool vis = false;
    napi_get_value_bool(env, argv[1], &vis);
    g_kernel.setVisible(label, vis);
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// kernelSelect(label): void — 设置画布选中对象（"" 清除），重绘由 ArkTS 侧 refresh 完成
napi_value KernelSelect(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    std::string label;
    if (argc >= 1 && argv[0] != nullptr) label = NapiToString(env, argv[0]);
    g_kernel.setSelected(label);
    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
}

// kernelRename(oldLabel, newLabel): string — 重命名并同步命令引用。
// 成功返回对象列表，失败返回 "error: ..."
napi_value KernelRename(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelRename requires (oldLabel, newLabel)");
        return nullptr;
    }
    std::string err = g_kernel.rename(NapiToString(env, argv[0]), NapiToString(env, argv[1]));
    if (!err.empty()) {
        return ToNapi(env, "error: " + err);
    }
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// kernelRedefine(label, expr): string — 重定义对象（保持标签与依赖指针）。
// 成功返回对象列表，失败返回 "error: ..."
napi_value KernelRedefine(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelRedefine requires (label, expr)");
        return nullptr;
    }
    if (!g_kernel.redefine(NapiToString(env, argv[0]), NapiToString(env, argv[1]))) {
        return ToNapi(env, "error: " +
            (g_kernel.lastError.empty() ? "无法重定义" : g_kernel.lastError));
    }
    std::string out;
    AppendGeoList(out);
    return ToNapi(env, out);
}

// kernelSetSlider(label, value): string — 设置滑动条值（吸附+钳位+级联重算）。
// 返回 "ok" 或 "-|"
napi_value KernelSetSlider(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelSetSlider requires (label, value)");
        return nullptr;
    }
    std::string label = NapiToString(env, argv[0]);
    double v = 0;
    napi_get_value_double(env, argv[1], &v);
    dreamember::GeoNumeric *n =
        dynamic_cast<dreamember::GeoNumeric *>(g_kernel.lookup(label));
    if (!n || !n->isSlider) return ToNapi(env, "-|");
    g_kernel.setSliderValue(n, v);
    return ToNapi(env, "ok");
}

// kernelSetSliderParams(label, min, max, step, value): string — 二十八包⑤
// 滑动条四参数原子应用（max>min 校验 → 范围/步长落位 → 值钳位吸附重算）。
// 返回 "ok" 或 "-|"
napi_value KernelSetSliderParams(napi_env env, napi_callback_info info)
{
    size_t argc = 5;
    napi_value argv[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 5) {
        napi_throw_error(env, nullptr,
            "kernelSetSliderParams requires (label, min, max, step, value)");
        return nullptr;
    }
    std::string label = NapiToString(env, argv[0]);
    double p[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) napi_get_value_double(env, argv[i + 1], &p[i]);
    dreamember::GeoNumeric *n =
        dynamic_cast<dreamember::GeoNumeric *>(g_kernel.lookup(label));
    if (!g_kernel.setSliderParams(n, p[0], p[1], p[2], p[3])) {
        return ToNapi(env, "-|");
    }
    return ToNapi(env, "ok");
}

// kernelSetFillAlpha(label, alpha 0-1): string — 设置区域填充不透明度（渲染态，
// 不触发重算）。不等式区域 / 多边形 / 角扇形；UI 层滑杆以透明度呈现（1−alpha）。
// 返回 "ok" 或 "-|"
napi_value KernelSetFillAlpha(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelSetFillAlpha requires (label, alpha)");
        return nullptr;
    }
    std::string label = NapiToString(env, argv[0]);
    double a = 0;
    napi_get_value_double(env, argv[1], &a);
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    dreamember::GeoElement *e = g_kernel.lookup(label);
    if (dreamember::GeoFunction *f = dynamic_cast<dreamember::GeoFunction *>(e)) {
        if (!f->ineqOp) return ToNapi(env, "-|");
        f->fillAlpha = a;
    } else if (dreamember::GeoPolygon *pg = dynamic_cast<dreamember::GeoPolygon *>(e)) {
        pg->fillAlpha = a;
    } else if (dreamember::GeoAngle *ga = dynamic_cast<dreamember::GeoAngle *>(e)) {
        ga->fillAlpha = a;
    } else {
        return ToNapi(env, "-|");
    }
    return ToNapi(env, "ok");
}

// kernelPickSlider(px, py): string — 命中滑动条手柄则记住并返回 "+|label"
napi_value KernelPickSlider(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelPickSlider requires (px, py)");
        return nullptr;
    }
    double a[2] = {0, 0};
    for (int i = 0; i < 2; ++i) napi_get_value_double(env, argv[i], &a[i]);
    g_dragSlider = g_kernel.pickSliderHandle(a[0], a[1]);
    if (g_dragSlider) return ToNapi(env, "+|" + g_dragSlider->label);
    return ToNapi(env, "-|");
}

// kernelDragSliderTo(px): string — 拖动滑动条手柄到像素 x，返回重绘帧
napi_value KernelDragSliderTo(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "kernelDragSliderTo requires px");
        return nullptr;
    }
    double px = 0;
    napi_get_value_double(env, argv[0], &px);
    if (g_dragSlider) {
        g_kernel.dragSliderTo(g_dragSlider, px);
        return ToNapi(env, RenderFrame());
    }
    return ToNapi(env, "-|");
}

// kernelAnimateToggle(label): string — 滑动条播放/暂停，返回 "+|1"/"+|0"
napi_value KernelAnimateToggle(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "kernelAnimateToggle requires label");
        return nullptr;
    }
    bool playing = g_kernel.animateToggle(NapiToString(env, argv[0]));
    return ToNapi(env, playing ? "+|1" : "+|0");
}

// kernelAnyAnimating(): string — "1" 有滑动条在动画
napi_value KernelAnyAnimating(napi_env env, napi_callback_info)
{
    return ToNapi(env, g_kernel.anyAnimating() ? "1" : "0");
}

// kernelGetProps(label): string — 对象属性快照 "k=v;k=v"（轮 A 通用属性 API，
// 供后续属性面板/样式条用；不含 value/definition，文本可能含 '=' 或 ';'）
// 找不到对象返回 "-|"
napi_value KernelGetProps(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "kernelGetProps requires label");
        return nullptr;
    }
    std::string props = g_kernel.propsOf(NapiToString(env, argv[0]));
    if (props.empty()) return ToNapi(env, "-|");
    return ToNapi(env, props);
}

// kernelSetProp(label, key, value): string — 通用属性写入口（vis/fill/value/
// min/max/step/anim/color，值统一字符串由内核解析）。返回 "ok" 或 "-|"
napi_value KernelSetProp(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value argv[3] = {nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 3) {
        napi_throw_error(env, nullptr, "kernelSetProp requires (label, key, value)");
        return nullptr;
    }
    bool ok = g_kernel.setProp(NapiToString(env, argv[0]),
                               NapiToString(env, argv[1]),
                               NapiToString(env, argv[2]));
    return ToNapi(env, ok ? "ok" : "-|");
}

// kernelTableEval(labels, xs): string — 表格视图批量求值（labels/xs 逗号
// 分隔；返回行 ';' 分隔、格 ',' 分隔，不可求值输出空格串）
napi_value KernelTableEval(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "kernelTableEval requires (labels, xs)");
        return nullptr;
    }
    return ToNapi(env, g_kernel.tableEval(NapiToString(env, argv[0]),
                                          NapiToString(env, argv[1])));
}

// kernelPushUndo(): string — 宿主在连续手势（拖点/滑杆拖动）首次变更前打点
napi_value KernelPushUndo(napi_env env, napi_callback_info)
{
    g_kernel.pushUndo();
    return ToNapi(env, "ok");
}

// kernelUndo(): string — 回退一步（"ok"/"no"）。宿主收到 "ok" 后自行刷新
// 画布与代数区（内核已整体重建，旧对象指针全部失效）
napi_value KernelUndo(napi_env env, napi_callback_info)
{
    return ToNapi(env, g_kernel.undoStep() ? "ok" : "no");
}

// kernelRedo(): string — 前进一步（"ok"/"no"）
napi_value KernelRedo(napi_env env, napi_callback_info)
{
    return ToNapi(env, g_kernel.redoStep() ? "ok" : "no");
}

// kernelUndoState(): string — "撤销深度,重做深度"（按钮置灰用）
napi_value KernelUndoState(napi_env env, napi_callback_info)
{
    return ToNapi(env, std::to_string(g_kernel.undoDepth()) + ","
        + std::to_string(g_kernel.redoDepth()));
}

// kernelAnimTick(dt): string — 动画帧推进（dt 秒）；有改动返回重绘帧，否则 "-|"
napi_value KernelAnimTick(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    double dt = 0.05;
    if (argc >= 1 && argv[0] != nullptr) napi_get_value_double(env, argv[0], &dt);
    if (g_kernel.anyAnimating() && g_kernel.animTick(dt)) {
        return ToNapi(env, RenderFrame());
    }
    return ToNapi(env, "-|");
}

// kernelGetGgb(): ArrayBuffer — 当前构造序列化打包为 .ggb（zip 容器，N4）
napi_value KernelGetGgb(napi_env env, napi_callback_info)
{
    std::string xml = g_kernel.getXml();
    std::string zip;
    if (!dreamember::MakeGgb(xml, zip)) {
        napi_throw_error(env, nullptr, "MakeGgb failed");
        return nullptr;
    }
    void *data = nullptr;
    napi_value ab = nullptr;
    if (napi_create_arraybuffer(env, zip.size(), &data, &ab) != napi_ok) {
        napi_throw_error(env, nullptr, "create arraybuffer failed");
        return nullptr;
    }
    if (!zip.empty()) memcpy(data, zip.data(), zip.size());
    return ab;
}

// kernelLoadGgb(data: ArrayBuffer): string — 打开 .ggb（zip+XML 重建构造列表）。
// 成功返回对象列表（warn: 前缀行携带跳过提示），失败返回 "error: ..."
napi_value KernelLoadGgb(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1 || argv[0] == nullptr) {
        napi_throw_error(env, nullptr, "kernelLoadGgb requires an ArrayBuffer");
        return nullptr;
    }
    void *data = nullptr;
    size_t len = 0;
    if (napi_get_arraybuffer_info(env, argv[0], &data, &len) != napi_ok) {
        return ToNapi(env, "error: 参数须为 ArrayBuffer");
    }
    std::string zip((const char *)data, len);
    std::string xml;
    if (!dreamember::ParseGgb(zip, xml)) {
        return ToNapi(env, "error: 无法读取 .ggb 文件（缺少 geogebra.xml 条目）");
    }
    int skipped = 0;
    g_dragPoint = nullptr;
    g_dragSlider = nullptr;
    if (!g_kernel.setXml(xml, &skipped)) {
        return ToNapi(env, "error: " + (g_kernel.lastError.empty()
            ? std::string("XML 解析失败") : g_kernel.lastError));
    }
    g_kernel.clearUndoHistory();   // 上游 语义：打开文件即新撤销链
    std::string out;
    if (skipped > 0) {
        out = "warn: " + g_kernel.lastError + "\n";
    }
    AppendGeoList(out);
    return ToNapi(env, out);
}

} // namespace

void DreamemberRegisterKernelApis(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "kernelInput", nullptr, KernelInput, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelDeleteLast", nullptr, KernelDeleteLast, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelDeleteObj", nullptr, KernelDeleteObj, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelAlgebra", nullptr, KernelAlgebra, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelRender", nullptr, KernelRender, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelPickPoint", nullptr, KernelPickPoint, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelDragPointTo", nullptr, KernelDragPointTo, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelEndDrag", nullptr, KernelEndDrag, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelGestureZoom", nullptr, KernelGestureZoom, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelPan", nullptr, KernelPan, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelSetOptions", nullptr, KernelSetOptions, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelResetView", nullptr, KernelResetView, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelClearAll", nullptr, KernelClearAll, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelGetGgb", nullptr, KernelGetGgb, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelLoadGgb", nullptr, KernelLoadGgb, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelTool", nullptr, KernelTool, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelToolTap", nullptr, KernelToolTap, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelToolParam", nullptr, KernelToolParam, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelPickObj", nullptr, KernelPickObj, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelSetVisible", nullptr, KernelSetVisible, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelSelect", nullptr, KernelSelect, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelRename", nullptr, KernelRename, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelRedefine", nullptr, KernelRedefine, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelSetSlider", nullptr, KernelSetSlider, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelSetSliderParams", nullptr, KernelSetSliderParams, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelSetFillAlpha", nullptr, KernelSetFillAlpha, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelPickSlider", nullptr, KernelPickSlider, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelDragSliderTo", nullptr, KernelDragSliderTo, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelAnimateToggle", nullptr, KernelAnimateToggle, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelAnyAnimating", nullptr, KernelAnyAnimating, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelAnimTick", nullptr, KernelAnimTick, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelGetProps", nullptr, KernelGetProps, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelSetProp", nullptr, KernelSetProp, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelTableEval", nullptr, KernelTableEval, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelPushUndo", nullptr, KernelPushUndo, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelUndo", nullptr, KernelUndo, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelRedo", nullptr, KernelRedo, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "kernelUndoState", nullptr, KernelUndoState, nullptr, nullptr, nullptr, napi_default, nullptr },
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
