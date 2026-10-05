/**
 * dreamember 原生层类型定义
 * N0：CAS；N1：画布 kernel；N2：代数区对象列表；N3：显示开关/重置视图/新建
 */
export const casCaseval: (expr: string) => string;
export const nativeVersion: () => string;

// N1 画布内核（行协议字符串）
export const kernelInput: (expr: string) => string;
export const kernelDeleteLast: () => string;
export const kernelDeleteObj: (label: string) => string;
export const kernelAlgebra: () => string;
export const kernelRender: (width: number, height: number, uiScale?: number, dark?: boolean) => string;
export const kernelPickPoint: (px: number, py: number) => string;
export const kernelDragPointTo: (px: number, py: number) => string;
export const kernelEndDrag: () => void;
export const kernelGestureZoom: (px: number, py: number, factor: number) => string;
export const kernelPan: (dx: number, dy: number) => string;

// N3 菜单/设置
export const kernelSetOptions: (grid: boolean, axes: boolean) => string;
export const kernelResetView: () => string;
export const kernelClearAll: () => string;

// N4 .ggb 文件读写（zip 容器 + GeoGebra 兼容 XML）
export const kernelGetGgb: () => ArrayBuffer;
export const kernelLoadGgb: (data: ArrayBuffer) => string;

// N5 点选式工具栏 / 对象拾取 / 滑动条与动画
export const kernelTool: (name: string) => void;
export const kernelToolTap: (px: number, py: number) => string;
// 二十六包 工具第二批：工具参数弹窗确认（toolTap 返回 ask| 后调用）
export const kernelToolParam: (value: string) => string;
export const kernelPickObj: (px: number, py: number) => string;
export const kernelSetVisible: (label: string, vis: boolean) => string;
export const kernelSelect: (label: string) => void;
export const kernelRename: (oldLabel: string, newLabel: string) => string;
export const kernelRedefine: (label: string, expr: string) => string;
export const kernelSetSlider: (label: string, value: number) => string;
// 二十八包⑤：滑动条四参数原子应用（max>min 校验，值钳位+吸附+级联重算）
export const kernelSetSliderParams: (label: string, min: number, max: number,
  step: number, value: number) => string;
export const kernelSetFillAlpha: (label: string, alpha: number) => string;
export const kernelAnimateToggle: (label: string) => string;
export const kernelAnyAnimating: () => string;
export const kernelAnimTick: (dt: number) => string;

// 轮 A 通用属性 API：新增对象属性不再各开一条导出；value/definition 文本
// 可能含 '=' 或 ';'，故不在属性快照内
export const kernelGetProps: (label: string) => string;
export const kernelSetProp: (label: string, key: string, value: string) => string;

// 二十三包 表格视图批量求值（GGB TableValuesView）：labels/xs 逗号分隔，
// 返回行 ';' 分隔、格 ',' 分隔；不可求值（不等式/竖直线/未知对象）与非
// 有限值输出空格串
export const kernelTableEval: (labels: string, xs: string) => string;

// 轮 B 撤销/重做：内核 XML 快照栈（容量 50）。离散变更（输入/重定义/删除/
// 改名/显隐/清空/工具作图）内核自动打点；连续手势（拖点/滑杆拖动/透明度
// 拖动）由宿主在首次变更前调 kernelPushUndo
export const kernelPushUndo: () => string;        // "ok"
export const kernelUndo: () => string;            // "ok" | "no"
export const kernelRedo: () => string;            // "ok" | "no"
export const kernelUndoState: () => string;       // "撤销深度,重做深度"
