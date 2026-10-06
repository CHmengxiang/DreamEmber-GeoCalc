// CanvasView.ets — N2 画布宿主（轮 A 防乱重构后改为"编排层"）：
// 持有全部会话状态与内核交互（输入/重命名/重定义/文件/动画/选中），负责
// 布局组装；UI 子件已拆出——代数区 AlgebraPanel、行组件 AlgebraRowView、
// 画布交互 CanvasStage（手势/缩放/长按菜单）、样式条 StyleBar（轮 C）、
// 工具面板 ToolsPanel、底部 Tab 栏 BottomTabs，配色统一走
// model/Theme，工具清单在 model/ToolDefs，行协议解析在 model/GeoTypes。
// 协议（C++ -> ArkTS，\n 分行）：label|type|value|def|vis|sel[|k=v;k=v]
//   绘图帧：op|color|lineWidth|[text|]x1,y1,... c=清屏 s=线段 m=成对短划
//   p=折线 o=圆 f=实心圆 t=文本 w=文本底衬 a=弧 y=多边形填充
// 坐标约定：C++ 侧全部使用 Canvas 位图像素；触摸坐标(vp) 在 CanvasStage
// 换算成位图像素再传给内核；canvas.onReady 里把 ctx.width/height（位图 px）
// 和 vp→px 比例上报宿主，kernelRender 由宿主统一调用。
// 手势：单指命中拖点 / 单指空白平移 / 双指捏合锚点缩放（上游 同款闭式解）。
// 代数区：宽屏（≥640vp 窗口实测断点，PC 虚拟机锁手机比例时按窄屏）左侧
// 固定栏，窄屏为画布下方抽屉。
// N3：数学虚拟键盘（customKeyboard 挂载，自绘优先，⌨ 双向切换）；键盘弹出
// 时同步 ui.mathKbVisible；垫高第十七包②起为运行时实测（见下）；深色模式
// 跟随系统。
// N4：.ggb 文件打开/保存（文件选择器 + fs，内核 kernelGetGgb/kernelLoadGgb）。
// N5：点选式工具栏（选工具 → 点画布作图，kernelTool/kernelToolTap 状态机）；
// 长按对象弹菜单（显示/隐藏、删除）；滑动条在代数区行内渲染拖动 +
// 动画定时器（kernelAnimTick）。
// PC 轮：画布鼠标滚轮缩放与右下角 ＋/－ 缩放按钮；行内编辑/新建对象。
// 十四轮：透明度语义翻转（UI 透明度 = 100×(1−fillAlpha)）、工具提示跟随
// 按钮/钳边、代数区与缩放按钮动画、顶栏去标题（Index 侧）。
// 协议 v2（轮 A）：第 7 列起 key=value 对（slider → anim/min/max/step，
// 填充对象 → fill 百分比），解析兼容旧位置列。
// 轮 B：上游经典版 外壳——窄屏底部四 Tab（代数区/工具/表格/数据，
// BottomTabs + bottomPanel 切换；表格/数据为占位）；撤销/重做走内核 XML
// 快照栈（离散变更内核自动打点，拖点/滑杆手势由宿主在首次变更前打点）。
// 轮 C：顶部工具条删除（工具入口在底部"工具"Tab；↶/↷ 迁至画布右上角浮层，
// 由 CanvasStage 渲染）；工具面板改分组网格；新增样式条 StyleBar（选中对象
// 的色板/大小/透明度/显隐，走 kernelGetProps/kernelSetProp，真机反馈的
// @Builder 按值传参不刷新问题已在迁移中一并规避）。
// 轮 D：表格视图 SheetPanel（A1 单元格模型：单元格地址即对象名，内核新增
// 表达式对象标签解析使 B1=A1+1 生效，选中态与画布/代数区联动）+ 数据视图
// DataPanel（单变量分析 + 直方图，面板私有 Canvas，不进几何画布）。
// 反馈②（2026-09-30 轮）：页面顶栏删除，☰ ↶ ↷ 移入 CanvasStage 画布左上
// 浮层——撤销状态经本组件 @State canUndo/canRedo 直传（快照栈真源在宿主），
// 动作与菜单请求经回调回传（Index 的 onMenuRequest 打开菜单抽屉）。
// 反馈④（同轮）：表格空格填数报"未找到对象"的根因是 setCell 把
// kernelGetProps 的不存在标记 "-|" 当作属性串，误走重定义；现按 "-|"
// 正确判定不存在走新建。创建提示条 3 秒自动消失（反馈①）。
// 第五包（2026-09-30）：CAS 视图迁入底部抽屉（panelTab='cas'，由 ☰ 菜单
// "视图"分组经 evt.openCas 打开，上游经典版 同位；Index 底部冒烟面板
// 删除，抽屉 cas 与代数区同档 40% 高）。
// 第九包（2026-10-01）：输入编辑结构感知——←/→ 按可视槽位进/出角标等
// 区域（mathCaretSlots）；紧贴 ^/_ 前键入追加进脚本内容（x|²+3→x^23）、
// ⌫ 从脚本内容级联外拆（内容→空组双括号→^/_ 本身）；紧贴函数模板 ( 前
// 退格整体拆除模板（sqrt(3)⌫→3）；□_□ 插空下标模板 '_'（back=-1 哨兵
// 智能落位，不预填 a/n）。
// 第十包（2026-10-01）：光标升级为「原文位置 + 框内/框外双态」（mathCaret
// + mathCaretScript），语义按上游 editor-base 结构化编辑器
//（CursorController/InputController/序列化器）
// 线性移植：←/→ 按 mathCaretStops 停靠序列进/出脚本框（框前↔框内是两个
// 独立停靠，base 尾可编辑）；框内键入落脚本内容（框首落内容首）；框外
// 紧贴脚本尾键入补 "*" 隔断（上游 序列化 ^(2) 的线性等价物）；⌫ 框首移除
// 整个结构、框内删内容字符、框外结构尾删内容末字符并进框（上游
// bkspCharacter/bkspLastFunctionArg）；^/_ 键紧邻同型结构只进框不嵌套
//（上游 newScript）；函数模板 ) 前 ⌫ 删参数末字符进框、空参整拆。
// 第十一包（2026-10-01）：①→ 切框内外光标样式不刷新（ForEach key 缺样
// 式，MathField 内修）；②□_□ 第一框：空 base 不产框首框外停靠（mathCaret
// Stops v2 按 run 走）+ 光标画进占位框（MathField caretIn）；③框外结构尾
// 键入去 "*"：按 上游 序列化器把裸脚本内容包成 ^(2) 再插兄弟字符（script
// RunEndAtCaret 探测任意 run 终结，串中段也生效）；④⌫ 对齐 上游 Remove
// Container（查证 ed_RemoveContainer.java：脚本节点空壳、base 留父序列）
// ——框首拆壳拼回主序列（x^2→x2、x^(23)→x23、空内容 x^→x；与前一裸 run
// 粘连先包裹前 run），框外空脚本框只进框不删（不再整删公式）；⑤同型 ^/_
// 在结构尾（框外）重入框尾（上游 newScript 向后游走）。
// 第十二包（2026-10-01）：①根号仍断开 → 渲染层整体改 Canvas 自绘（Math
// Field 内修，钩形+顶线一次 stroke 天然一体）；②用户询问 LaTeX/KaTeX——
// KaTeX 必须 WebView，与主功能禁用 Web 组件冲突，以 Canvas 排版替代；
// ③框_框 角标 ← 不到大框且键入进小框，两个根因都在本文件：(a) 框外结构
// 尾键入的 ^(…) 包裹插入点差一（rng[1]+1 是 ')' 的下标，字符插到 ')' 前
// = 落进脚本内容；上游 语义是兄弟节点，改 rng[1]+2），(b) 空脚本内容的框
// 内/框外两站同位、视觉重合，← 看似没动——空内容框内停靠标记 empty，
// ← 跳过一次到位大框/base 尾（→ 不跳，仍可进空框填底）。
// 第十四包（2026-10-02）：②键盘避让改系统键盘同款——窄屏垫高从页面级
//（Index 224 连底栏一起抬）移入本组件底栏上方 178（=键盘 220−底栏 46+
// 4 呼吸隙）：底栏钉在屏幕底被键盘浮层盖住、只把抽屉/样式条顶到键盘上
// 沿；宽屏仍走 Index 页面级垫高（ui.isWide 经 AppStorage 下发门控）。
// 第十五包（2026-10-02）：①输入行钉在代数区面板底部（AlgebraPanel：移出
// List 末项）——键盘弹出时垫高把抽屉顶到键盘上沿、输入行恰好贴键盘上方
//（旧 List 末项随内容滚动，短列表时被抬到离键盘很远 = "避让抬高过多"）；
// ②默认输入方式 0→1=自带键盘（CanvasView/Index/Ability 三处默认值，已
// 持久化用户设置不受影响）；③渲染不可见定案（MathField 内修：坐标制 vp
// 终案）。
// 第十六包（2026-10-02）：①自带键盘避让与系统键盘逐像素对齐——真机反馈
// 仍多出一个输入行高度：系统键盘避让是窗口整体压缩键盘高 220 后按压缩页
// 面取 40% 布局，本组件垫 178 但抽屉仍按未压缩全高取 40%，抽屉顶恰高出
// 0.4×220−底栏46。定案：垫高=整个键盘高 220（底栏照旧被盖），键盘弹出时
// 抽屉高度按 (H−220) 取比例（drawerHeight，pageH 由根 onAreaChange 维护）；
// ②角标框 ←/→ 均进不去：十二包③「← 跳过空框内停靠」把空框变成单向门
// （→ 能进、← 从结构尾直接跳过），删跳站逻辑、空框内停靠恢复普通站；
// 「框内 ← 出站下一站即大框」由站序天然保证；③默认自带键盘真机未生效：
// 旧包把当时的默认 0 写进了持久化，「已持久化不受影响」当成用户选择——
// Ability 恢复时按 optDefaultsVer 标记做一次性迁移（Ability 内修）。
// 第十七包（2026-10-02）：①PC 虚拟机锁手机比例（点最大化只顶满高度）时
// 布局仍是宽屏错位重叠——isPC 按设备类型强制宽屏作废，宽窄只看窗口实际
// 宽度（根 onAreaChange ≥640vp）；②自带键盘避让反多两行：静态几何两轮没
// 对上（15 包 178 少一行、16 包 220 多两行——垫高与输入行之间隔着样式条/
// 边距/分支，推不准），整体改**运行时实测**：钉底输入行 onAreaChange 上报
// 自身底边（globalPosition px→vp，AlgebraPanel 新回调 onInputArea），垫高
// kbSpacer 一步收敛到「输入行底边 = 键盘上沿 + 4」（底边随垫高 1:1 平移，
// s = 220+4−bottom+当前垫高）；抽屉恢复固定档位（40%/32%），宽屏垫高从
// Index 页面级挪进本组件（Row 下垫同值，Index 级垫高与输入行不同组件，
// 反解不收敛）；③④小框光标三处毛病一根因：自带键盘模式下 caretPosition()
// 程序化设位异步回声 onTextSelectionChange，onSelChange 无条件回写
// mathCaretScript=false 把框内态洗掉——③x^2 框内 2 前插 3 后光标跑小框外
// （按大框基线画到结构尾），④大小框间移动光标恒按大框渲染。门禁：自带键
// 盘模式忽略选区回声（框内态只由 applyInsert/caretMove 维护），系统键盘
// 模式回写照旧；MathField ④：小框 caretIn 不再加高（框随普通几何 −8/8、
// −3/8），框内光标条随框高收紧（大框 ±5 不变）。
// 第十八包（2026-10-02）：①输入行完全无避让、整个压在自带键盘下面——
// 两个独立缺陷叠加：(a) 键盘是浮层，弹出不触发任何布局变化，onAreaChange
// 在「可见翻转」后再无回调，而首版只在回调里算垫高 → kbSpacer 恒 0。修
// 复：输入行几何随上报随缓存（lastInputArea），可见翻转时刻立即用缓存反
// 解（syncKbSpacer → applyKbSpacer），另留 120ms 一次性重试兜底极端时序；
// (b) globalPosition 官方只标注 width/height 为 vp、坐标字段未标单位（社
// 区定论 px）——首版按 px÷k 解释，若真身 vp 则 bottom×k 越界同样垫高恒
// 0。修复：坐标改「距根顶差值」（消状态栏等页面外偏移）+ 双口径解释自校
// 准（vp 解释合法且更小则取之，否则取 px 解释——px 原值经 vp 解释必越出
// 页高，规则在两种世界都选中真值）；根 onAreaChange 同步缓存 rootY。
// 第十九包（2026-10-02）：①避让出现但恒多"一块"——目标原按页面底反解，
// 而页面底悬在导航条避让区上方、customKeyboard 浮层锚窗口底，三轮数据闭
// 环（15 包 228/16 包 270/18 包实测 224，差额恒 = 导航条高 + 常数）。目
// 标改按窗口底折算：MATH_KB_HEIGHT − 导航条高 + 4（导航条高由 Ability 查
// TYPE_NAVIGATION_INDICATOR 存 'ui.navBottomVp'，Ability 内修）；③大小框
// 间光标高度不变定案在 MathField（buildDisplayAtoms 邻居搜索，MathField
// 内修）。
import dreamemberKernel from 'libdreamember.so';
import { inputMethod } from '@kit.IMEKit';
import { fileIo as fs, picker } from '@kit.CoreFileKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { componentUtils } from '@kit.ArkUI';
import { AlgebraEntry, parseAlgebra } from '../model/GeoTypes';
import { Theme } from '../model/Theme';
import { AlgebraPanel } from './AlgebraPanel';
import { CanvasStage } from './CanvasStage';
import { ToolsPanel } from './ToolsPanel';
import { SheetPanel } from './SheetPanel';
import { DataAnalysisPanel } from './DataPanel';
import { TablePanel } from './TablePanel';
import { CasPanel } from './CasPanel';
import { BottomTabs, TABS, TabDef } from './BottomTabs';
import { MathKeyboard, MathKbHooks, MATH_KB_HEIGHT, KB_WIDE_MAX_WIDTH } from './MathKeyboard';
import { StyleBar } from './StyleBar';
// 第二十二包②：分隔条/抽屉把手鼠标悬停改变光标（↔/↕）——ArkUI 无组件级
// 光标属性，用 multimodalInput.pointer.setPointerStyleSync（公开 API）+
// onHover 进出设/还原；windowId 经 window.getLastWindow 解析
import pointer from '@ohos.multimodalInput.pointer';
import window from '@ohos.window';
// 二十九包①：KeyboardAvoidMode.RESIZE——弹窗打开期间把系统键盘避让从默认
// OFFSET（整个页面上移，页高不变）切成 RESIZE（页高确定性收缩）。真机根因：
// 默认 OFFSET 抬升页面 ≈ 一个键盘高，我方垫底再加一个键盘高 = 避让多出
// 一整个键盘高，且页高从不收缩、askSysPad 的「页高收缩闸门」永不触发
//（二十八包④真机两轮未解的根因；模拟器不真弹 IME 复现不了）。
// 四十四包 v4 推翻：真机帧证 RESIZE 页面回弹视觉是终帧瞬移（无动画），
// 弹窗卡片下降永远一次瞬跳——v4 起**不再切 RESIZE**，弹窗 OFFSET 端到端
//（垫底已退役，避让全靠系统抬升），下方 closeToolAsk 的 OFFSET 恢复保留
// 作防御性复位
import { KeyboardAvoidMode } from '@ohos.arkui.UIContext';
import { canvasDensity, functionOpenBefore, hasIdentBase, isTypewriterCh,
  matchParenBackward, MathCaretStop, mathCaretStops, parseMathAtoms,
  scriptContentRange, scriptRunEndAtCaret, scriptStructureEnd,
  templateUnwrapRange } from './MathField';

@Component
export struct CanvasView {
  @State private inputError: string = '';
  @State private pendingInput: string = '';
  @State private editingLabel: string = '';      // 非空 = 行内修改模式（重定义该对象）
  @State private creating: boolean = false;      // 列表末尾"新建对象"输入行
  @State private algebra: AlgebraEntry[] = [];   // 代数区对象列表（每次输入/删除后刷新）
  private algebraScroller: Scroller = new Scroller();   // 改名滚动避让（传入面板共享）
  @State private showPanel: boolean = false;     // 底部面板开关（窄屏抽屉/宽屏左栏）
  // 轮 B：当前 Tab（algebra/tools/data/sheet；cas 与第二十三包②的
  // analysis 为虚拟 Tab——不在底栏 TABS 内，分别由菜单与数据区选区打开）
  @State private panelTab: string = 'algebra';
  // 第二十三包②：数据分析选区快照（上游 DataSource.setDataListFromSelection
  // 同义：打开分析面板即快照，选区后续变化不回灌）
  @State private analysisSource: string = '';
  @State private analysisRange: string = '';
  // 二十四包①：函数值表范围三值（TablePanel 随 Tab 销毁，@State 提升到宿主
  // 才能在离开「表格」Tab 后保留起止/步长）
  @State private tableMin: number = -2;
  @State private tableMax: number = 2;
  @State private tableStep: number = 1;
  @State private isWide: boolean = false;        // 断点：≥640vp 宽屏双栏
  // 第二十一包①：宽屏左栏宽度（vp）——停靠面板右缘分隔条可拖宽（上游 桌面
  // dock 面板同款），拖拽起点快照供 onActionUpdate 增量换算
  @State private widePanelW: number = 420;
  private widePanelWStart: number = 420;
  // 第二十二包③：窄屏抽屉拖拽高度（vp；0=按默认档位 40%/32%；切换 Tab 归零）
  @State private drawerH: number = 0;
  private drawerHStart: number = 0;
  // 第二十二包②：鼠标光标（setPointerStyle 目标窗口 id，懒解析）
  private curWinId: number = -1;
  // 页面总宽（vp，根 onAreaChange 维护）：拖宽钳位上限用
  @State private pageW: number = 0;
  // 第十七包①：isPC 强制宽屏已删——PC 虚拟机/2in1 可锁手机比例（窗口宽
  // 度 <640vp、点最大化只顶满高度），按设备类型强制宽屏会把 PC 布局塞进
  // 窄窗导致错位重叠；宽窄只由根 onAreaChange 的窗口宽度决定
  // N3 显示开关：Index 设置面板写 AppStorage，@StorageLink 双向同步到这里，
  // 变化即下发内核并整帧重绘
  @StorageLink('opt.showGrid') @Watch('onDisplayOpt') private showGridOpt: boolean = true;
  @StorageLink('opt.showAxes') @Watch('onDisplayOpt') private showAxesOpt: boolean = true;
  // N3 菜单一次性事件（Index 侧 AppStorage.setOrCreate 打点触发）
  @StorageProp('evt.resetView') @Watch('onResetViewEvt') private resetViewTick: number = 0;
  @StorageProp('evt.clearAll') @Watch('onClearAllEvt') private clearAllTick: number = 0;
  @StorageProp('evt.dismissKb') @Watch('onDismissKbEvt') private dismissKbTick: number = 0;
  // N4 文件菜单事件（打开/保存 .ggb）
  @StorageProp('evt.openGgb') @Watch('onOpenGgbEvt') private openGgbTick: number = 0;
  @StorageProp('evt.saveGgb') @Watch('onSaveGgbEvt') private saveGgbTick: number = 0;
  // 第五包：CAS 视图（☰ 菜单"视图"分组发起，上游经典版 同位）
  @StorageProp('evt.openCas') @Watch('onOpenCasEvt') private openCasTick: number = 0;
  // 反馈②：左上浮层 ↶/↷ 状态（快照栈深度，syncUndo 刷新，@Prop 直传 CanvasStage）
  @State private canUndo: boolean = false;
  @State private canRedo: boolean = false;
  // 反馈②：☰ 菜单请求（CanvasStage 左上浮层发起，Index 打开菜单抽屉）
  onMenuRequest: () => void = () => {};
  // N3 深色模式：跟随系统（EntryAbility 同步 'ui.dark'）
  @StorageProp('ui.dark') @Watch('onDarkChanged') private dark: boolean = false;
  private settings: RenderingContextSettings = new RenderingContextSettings(true);
  private ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.settings);
  private canvasW: number = 0;    // 位图像素（CanvasStage onReady 上报）
  private canvasH: number = 0;
  // 画布 vp 尺寸。@State：右下角缩放按钮的显隐与定位依赖它们，普通字段
  // 变化不触发重渲染（十三轮：按钮首帧后永不出现）。CanvasStage @Link 写入
  @State private vpW: number = 0;
  @State private vpH: number = 0;
  private uiScale: number = 1.0;  // 位图px/vp
  // N3 数学虚拟键盘：光标跟踪（onTextSelectionChange）+ 光标处插入/退格。
  // 新建/修改两个输入行共用 pendingInput，各自持控制器（caret 定位指向焦点框）；
  // 控制器实例传入面板（TextInput controller 与光标定位共用同一对象）
  private createCtrl: TextInputController = new TextInputController();
  private editCtrl: TextInputController = new TextInputController();
  private selStart: number = -1;
  private selEnd: number = -1;
  // MathField 光标（第七包）：@State 供公式化显示渲染；编辑焦点态同步 @State
  //（MathField 的光标显隐与闪烁依赖它，普通字段不触发重渲染）
  @State private mathCaret: number = 0;
  // 第十包②：光标框内/框外双态（上游 currentField 的线性等价）。true=
  // 光标在 ^/_ 脚本框内（小号显示、键入写脚本内容）；false=框外（基线、
  // 键入写外层/base）。渲染源 MathField.caretScript
  @State private mathCaretScript: boolean = false;
  @State private inputFocused: boolean = false;
  // 二十四包②/二十五包②：数据区格内编辑（SheetPanel onEditFocus 上报）/
  // 表格范围编辑（TablePanel onEditFocus 上报）占用自带键盘——两者不同时
  // 出现，与代数区输入行共用实测垫高（mathKbVisible/kbSpacer）
  @State private sheetKbFocus: boolean = false;
  @State private tableKbFocus: boolean = false;
  // 二十九包②：滑动条行内编辑字段聚焦（自带键盘浮层占用）——垫高走
  // syncKbSpacer 的整垫分支（目标字段在列表顶部，垫满键盘高即保证可见）
  @State private slideKbFocus: boolean = false;
  // 自绘键盘/系统键盘切换（设置里可选默认，⌨ 双向入口仍可临时切换）。
  // customKeyboard 传 undefined（API 22+ 官方支持）即解绑自绘键盘、允许系统输入法。
  @State private useSysKeyboard: boolean = false;
  // 默认输入方式（Index 设置页写入并持久化）：0=系统键盘 1=自带键盘
  //（第十五包②：默认改 1=自带键盘；已持久化的用户设置不受影响）
  @StorageLink('opt.inputMethod')
  @Watch('onInputMethodOpt')
  private inputMethodOpt: number = 1;

  // ---- N5 工具 / 动画 ----
  @State private tool: string = '';              // 当前工具 id（'' = 选择/移动）
  @State private toolHint: string = '';          // 画布左上反馈条（创建/命令结果）
  private hintTimer: number = -1;                // 提示条自动消失定时器（约 3 秒）
  // 二十六包 工具第二批参数弹窗（toolTap "ask|" → 模态输入 → kernelToolParam）
  @State private toolAskKind: string = '';       // 非'' = 弹窗开启（值=工具 id）
  @State private toolAskHint: string = '';       // 内核 "ask|" 提示文案（弹窗标题）
  @State private toolAskText: string = '';       // 参数输入（数字或对象引用，原样进命令）
  @State private toolAskErr: string = '';        // 内核 err| 就地显示（弹窗不关可改）
  @State private toolAskCaretA: number = 0;      // 自带键盘插入协议光标
  @State private toolAskCaretB: number = 0;
  private toolAskCtrl: TextInputController = new TextInputController();
  // 二十九包②：滑动条四项行内编辑（最小值/最大值/步长/当前值）。行值文本
  // 点击入口——原入口回填定义修改，一动就把滑动条改成固定值（真机反馈）；
  // 二十八包曾做四项弹窗，二十九包按反馈撤弹窗、改代数区行内向下展开
  //（AlgebraPanel sliderEditFields，四项各自独立编辑）
  @State private slideEditLabel: string = '';    // 非'' = 行内编辑展开（目标滑动条）
  @State private slideEditMin: string = '';
  @State private slideEditMax: string = '';
  @State private slideEditStep: string = '';
  @State private slideEditVal: string = '';
  @State private slideEditErr: string = '';
  // 自带键盘活跃字段（0=最小 1=最大 2=步长 3=当前值，-1=无焦点）；光标
  // 跟踪只维护活跃字段的（其余字段聚焦时先落到串尾，随选择回调细化）
  @State private slideEditField: number = -1;
  private slideCaretA: number = 0;
  private slideCaretB: number = 0;
  // 二十七包②/二十九包①：系统键盘实测高度（vp；window 'keyboardHeightChange'
  // px÷密度，0=隐藏）。弹窗系统键盘模式垫卡片底——但默认 OFFSET 避让会把整
  // 页上移一个键盘高（页高不变），垫+移=真机多出一整个键盘高（二十八包④
  // 两轮未解的根因）；二十九包起弹窗期间切 RESIZE（页高确定性收缩）→ 收缩
  // 落地后本闸门置垫高为 0，收缩到达前的过渡态才短暂垫底
  // 四十三包：@Watch 兼作 ⌨ 切回自带的挂行时机——kbH 归零事件=IME 落定
  @StorageProp('ui.kbH') @Watch('onAskSysKb') private sysKbH: number = 0;
  private askOpenPageH: number = 0;              // 弹窗打开以来最大页高（键盘只缩不涨）
  @State private selectedLabel: string = '';     // 画布/代数区联动的选中对象
  @State private styleTick: number = 0;          // 轮 C：样式条属性重取节拍（每次代数区同步 +1）
  private animTimer: number = -1;                // 动画定时器（50ms 步进）
  // 拖点/滑杆时代数区同步节流
  private lastAlgebraMs: number = 0;

  aboutToAppear(): void {
    // 启动时按设置决定默认键盘（默认 0=系统键盘）
    this.useSysKeyboard = this.inputMethodOpt === 0;
    this.ensureWinId();   // 二十二包②：悬停光标目标窗口
    // 第十七包①：PC/2in1/模拟器强制宽屏的 isPC 判定删除——鸿蒙 PC 虚拟机
    // 可锁手机比例（宽度 <640vp），布局断点只看窗口宽度（build 尾
    // onAreaChange），与窗口形态解耦
  }

  // 二十七包②：系统键盘高度监听（px→vp 写 AppStorage 'ui.kbH'，本组件
  // @StorageProp 跟随；弹窗垫底用，与代数区/面板的自带键盘垫高管线无关）
  private watchKbHeight(w: window.Window): void {
    try {
      w.on('keyboardHeightChange', (px: number) => {
        AppStorage.setOrCreate('ui.kbH', px > 0 ? px / canvasDensity() : 0);
        // 四十四包 v5（用户真机反馈②）：过渡桥塌陷时机改 IME **收起**（px<=0）
        // ——本机 OFFSET 避让是「刚好抬到聚焦框露出」的部分抬升且聚焦后不
        // 重算：旧逻辑在 IME 弹起事件即塌桥，输入框随之下掉 173vp 沉入 IME
        // 后面（⌨ 切系统键「输入框下移被盖住」根因）。改为弹起期间保持
        // 键盘行/等高占位（输入框零位移），IME 收起才动画塌陷（随收键盘
        // 自然滑落）；三十三包原语义（RESIZE 世界页缩随动塌桥）作废
        if (px <= 0 && this.askBridgeKb) {
          this.getUIContext().animateTo({
            duration: 200,
            curve: Curve.EaseOut
          }, () => {
            this.askBridgeKb = false;
          });
        }
        // 三十一包①b：系统键盘态滑动条避让随实测键盘高复查（垫高+滚动对
        // 齐同闸复查；弹起过程连发，去抖以最后一次为准）。三十四包①：去抖
        // 120→300ms——部分 ROM 弹起动画未落定就先报实高，OFFSET 页面抬升
        // 随后到位，中途复查会先自垫、抬升落地后再压回（先上跳再回缩）；
        // 300ms 让弹起/抬升动画落定后一步复查到位
        this.scheduleSlideAvoid(300);
        // 三十四包②：行内编辑系统键盘态手势收起 IME——blur 只清字段号、
        // 复查链因 slideEditField<0 早退，垫高须在此显式滑落，防抽屉悬空
        //（⌨ 切回自带的落垫由 toggleSlideKeyboard 显式处理，同一目标无冲突）
        if (px <= 0 && this.useSysKeyboard && this.slideEditLabel.length > 0) {
          this.setSlideSpacer(0);
        }
      });
    } catch (_e) {
      // 监听不可用：弹窗系统键盘退回窗口自身避让
    }
  }

  // 弹窗系统键盘垫底——四十四包 v5 **删除**（历史：二十八包④引入、三十三
  // 包①收缩为旧 ROM 专属、v4 退役恒 0）：本机 OFFSET 避让自己会抬聚焦框
  //（部分抬升且聚焦后不重算），任何补垫都是二十八包④式「垫+移双份」；
  // v5 过渡期空间改由 askCard 内等高占位承担（askBridgeKb 分支）

  private onInputMethodOpt(): void {
    this.useSysKeyboard = this.inputMethodOpt === 0;
    if (this.inputFocused) {
      // 输入中切换：收起当前键盘，按新方式重新聚焦弹出
      this.activeCtrl().stopEditing();
      this.inputFocused = false;
      this.syncKbSpacer();
    }
  }

  // 焦点所在的输入行控制器（新建行 / 行内修改框）
  private activeCtrl(): TextInputController {
    return this.editingLabel.length > 0 ? this.editCtrl : this.createCtrl;
  }

  // 焦点所在输入行的 id（toggleKeyboard 重新聚焦用）
  private activeInputId(): string {
    return this.editingLabel.length > 0 ? 'algebraDefEdit' : 'algebraCreateInput';
  }

  // ---- 渲染（帧协议 → Canvas 位图） ----

  private execFrame(frame: string): void {
    if (frame.length === 0) {
      return;
    }
    // 整帧重绘：清屏（深色下画布底色同步）
    this.ctx.fillStyle = Theme.canvas(this.dark);
    this.ctx.fillRect(0, 0, this.canvasW, this.canvasH);
    const rows = frame.split('\n');
    for (const row of rows) {
      if (row.length < 4) continue;
      const parts = row.split('|');
      if (parts.length < 4) continue;
      const op = parts[0];
      if (op === 'c') continue;
      const color = parts[1];
      const lw = parseFloat(parts[2]);
      let text = '';
      let coordsStr = parts[3];
      if (op === 't') {
        text = parts[3];
        coordsStr = parts.length > 4 ? parts[4] : '';
      }
      const nums: number[] = [];
      if (coordsStr.length > 0) {
        for (const s of coordsStr.split(',')) {
          const v = parseFloat(s);
          if (!isNaN(v)) nums.push(v);
        }
      }
      switch (op) {
        case 's':
          if (nums.length >= 4) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.moveTo(nums[0], nums[1]);
            this.ctx.lineTo(nums[2], nums[3]);
            this.ctx.stroke();
          }
          break;
        case 'm':
          // 成对短划：每 4 个数一段（刻度用），不连成折线
          for (let i = 0; i + 3 < nums.length; i += 4) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.moveTo(nums[i], nums[i + 1]);
            this.ctx.lineTo(nums[i + 2], nums[i + 3]);
            this.ctx.stroke();
          }
          break;
        case 'p':
          if (nums.length >= 4) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.moveTo(nums[0], nums[1]);
            for (let i = 2; i + 1 < nums.length; i += 2) {
              this.ctx.lineTo(nums[i], nums[i + 1]);
            }
            this.ctx.stroke();
          }
          break;
        case 'o':
          if (nums.length >= 3) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.arc(nums[0], nums[1], nums[2], 0, Math.PI * 2);
            this.ctx.stroke();
          }
          break;
        case 'f':
          if (nums.length >= 3) {
            this.ctx.beginPath();
            this.ctx.fillStyle = color;
            this.ctx.arc(nums[0], nums[1], nums[2], 0, Math.PI * 2);
            this.ctx.fill();
          }
          break;
        case 't':
          if (text.length > 0 && nums.length >= 2) {
            this.ctx.fillStyle = color;
            this.ctx.font = `${Math.round(14 * this.uiScale)}px sans-serif`;
            this.ctx.fillText(text, nums[0], nums[1]);
          }
          break;
        case 'w':
          // 文本选中底衬：同位描边（线宽即底衬宽度）
          if (text.length > 0 && nums.length >= 2) {
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.lineJoin = 'round';
            this.ctx.font = `${Math.round(14 * this.uiScale)}px sans-serif`;
            this.ctx.strokeText(text, nums[0], nums[1]);
          }
          break;
        case 'a':
          // 圆弧（角度对象）：cx,cy,r,start,end
          if (nums.length >= 6) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.arc(nums[0], nums[1], nums[2], nums[3], nums[4], false);
            this.ctx.stroke();
          }
          break;
        case 'y':
          // 多边形填充（颜色为 rgba() 串——8 位 hex 会被 ArkUI 按 #AARRGGBB 误读）
          if (nums.length >= 6) {
            this.ctx.beginPath();
            this.ctx.fillStyle = color;
            this.ctx.moveTo(nums[0], nums[1]);
            for (let i = 2; i + 1 < nums.length; i += 2) {
              this.ctx.lineTo(nums[i], nums[i + 1]);
            }
            this.ctx.closePath();
            this.ctx.fill();
          }
          break;
        default:
          break;
      }
    }
  }

  private refresh(): void {
    if (this.canvasW <= 1 || this.canvasH <= 1) return;
    this.execFrame(dreamemberKernel.kernelRender(this.canvasW, this.canvasH, this.uiScale, this.dark));
  }

  private syncAlgebra(raw: string): void {
    const parsed = parseAlgebra(raw);
    // 就地合并：同 label 复用实例并改字段（@Observed 直更行组件），不整表重建
    const next: AlgebraEntry[] = [];
    for (const p of parsed) {
      const old = this.algebra.find((a: AlgebraEntry): boolean => a.label === p.label);
      if (old) {
        old.type = p.type;
        old.value = p.value;
        old.definition = p.definition;
        old.vis = p.vis;
        old.sel = p.sel;
        old.anim = p.anim;
        old.min = p.min;
        old.max = p.max;
        old.step = p.step;
        old.fillAlpha = p.fillAlpha;
        next.push(old);
      } else {
        next.push(p);
      }
    }
    this.algebra = next;
    this.syncUndo();
    this.styleTick++;   // 轮 C：样式条重取属性快照（选中对象的色/大小/透明度可能变了）
  }

  // 撤销/重做按钮态（快照栈深度）：所有变更路径最终都汇到 syncAlgebra，
  // 在此顺带刷新即可覆盖输入/删除/工具/拖动/滑杆全部场景。
  // 反馈②：按钮在画布右上浮层（CanvasStage），状态本组件持有直传
  private syncUndo(): void {
    const s: string = dreamemberKernel.kernelUndoState();
    let canUndo = false;
    let canRedo = false;
    const comma = s.indexOf(',');
    if (comma > 0) {
      canUndo = parseInt(s.substring(0, comma)) > 0;
      canRedo = parseInt(s.substring(comma + 1)) > 0;
    }
    this.canUndo = canUndo;
    this.canRedo = canRedo;
  }

  // 撤销/重做（轮 B）：内核整体重建，旧对象指针全部失效——清选中/编辑
  // 会话后照载入流程刷新。撤销前的隐式 ✓ 提交本身也是一步（连按 ↶ 可
  // 继续撤掉该次提交，语义与 上游 一致）
  private doUndo(): void {
    this.commitPending();
    if (dreamemberKernel.kernelUndo() !== 'ok') {
      return;
    }
    this.afterHistoryJump();
  }

  private doRedo(): void {
    this.commitPending();
    if (dreamemberKernel.kernelRedo() !== 'ok') {
      return;
    }
    this.afterHistoryJump();
  }

  private afterHistoryJump(): void {
    this.stopAnimTicker();
    this.selectedLabel = '';
    dreamemberKernel.kernelSelect('');
    this.editingLabel = '';
    this.creating = false;
    this.pendingInput = '';
    this.mathCaret = 0;
    this.inputError = '';
    this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    this.refresh();
    if (dreamemberKernel.kernelAnyAnimating() === '1') {
      this.startAnimTicker();
    }
  }

  // 输入一条代数式，刷新画布。返回错误信息（空串=成功）。
  public input(expr: string): string {
    const r: string = dreamemberKernel.kernelInput(expr);
    if (r.startsWith('error:')) {
      return r.substring(6);
    }
    this.syncAlgebra(r);
    this.refresh();
    if (dreamemberKernel.kernelAnyAnimating() === '1') {
      this.startAnimTicker();
    }
    return '';
  }

  public deleteObj(label: string): void {
    this.commitPending();
    if (this.selectedLabel === label) {
      this.selectedLabel = '';
      dreamemberKernel.kernelSelect('');
    }
    if (this.editingLabel === label) {
      this.editingLabel = '';
      this.pendingInput = '';
      this.mathCaret = 0;
    }
    const r: string = dreamemberKernel.kernelDeleteObj(label);
    this.syncAlgebra(r);
    this.refresh();
  }

  // ---- N5 工具栏 / 动画 ----

  private setTool(id: string): void {
    // 工具栏按钮不自动夺焦，改名框/键盘不会自然失焦——显式清除
    // （与点画布收键盘同语义：选工具即结束输入会话）
    try {
      this.getUIContext().getFocusController().clearFocus();
    } catch (e) {
      // 焦点清除失败不影响选工具
    }
    this.tool = id;
    dreamemberKernel.kernelTool(id);
  }

  // ---- 轮 C 样式条 ----

  // 样式条属性写入（色板/大小/透明度）。markUndo=离散变更（点按或滑杆起段）
  // 需先打撤销快照；final=手势结束帧（滑杆 mode 2/3 与离散点按）强制同步。
  // 写完统一同步代数区（styleTick 驱动样式条重取）+ 重绘；失败（如给无填充
  // 对象设 fill）由内核拒绝且不打步。
  private onStyleSetProp(label: string, key: string, val: string, markUndo: boolean,
                         final: boolean): void {
    this.commitPending();
    if (markUndo) {
      dreamemberKernel.kernelPushUndo();
    }
    dreamemberKernel.kernelSetProp(label, key, val);
    const now = Date.now();
    if (final || now - this.lastAlgebraMs > 80) {
      this.lastAlgebraMs = now;
      this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    }
    this.refresh();
  }

  // 样式条显隐切换（走 kernelSetVisible：内核自动打点 + 无变化不留步）
  private onStyleVisible(label: string, show: boolean): void {
    this.toggleRowVisible(label, show);
  }

  // 提示条统一入口：写入并安排约 3 秒后自动消失（真机反馈①：提示常驻
  // 会遮挡画布，上游 同类反馈也是短暂显示）。新提示会重置计时器
  private showHint(msg: string): void {
    this.toolHint = msg;
    if (this.hintTimer >= 0) {
      clearTimeout(this.hintTimer);
    }
    this.hintTimer = setTimeout(() => {
      this.toolHint = '';
      this.hintTimer = -1;
    }, 3000);
  }

  // kernelToolTap 返回（done|/need|/err|/ask|）→ 提示条 + 画布/代数区刷新；
  // ask| = 工具参数弹窗（二十六包）
  private applyToolResult(r: string): void {
    if (r.startsWith('done|')) {
      this.showHint('已创建 ' + r.substring(5));
      this.syncAlgebra(dreamemberKernel.kernelAlgebra());
      this.refresh();
      if (dreamemberKernel.kernelAnyAnimating() === '1') {
        this.startAnimTicker();
      }
    } else if (r.startsWith('ask|')) {
      this.openToolAsk(r.substring(5));
      this.refresh();   // 保持待选对象点亮特效
    } else if (r.startsWith('need|')) {
      this.showHint(r.substring(5));
      this.refresh();   // 刷新以显示工具拾取对象的点亮特效
    } else if (r.startsWith('err|')) {
      this.showHint(r.substring(4));
    }
  }

  // ---- 二十六包 工具参数弹窗（上游 InputDialog 同位：标题+数值输入+确定/取消）----

  // 弹窗开启：预填 上游 同款默认值（旋转 45°/位似 2/定角 60°/半径与定长 3），
  // 焦点抢入输入框（sheetCellEditor 同款动态插入组件显式抢焦）
  private openToolAsk(hint: string): void {
    // 二十九包①曾切 RESIZE（页高确定性收缩）——四十四包真机帧证推翻：本机
    // ROM 的 RESIZE 页面回弹视觉是**终帧瞬移**（布局 26ms 内到终值、视觉
    // 要到 IME 滑走后才一帧跳回，连拍抓不到中间帧=无动画），卡片下降永远
    // 是一次瞬跳（v1-v3 三版挂行闸怎么调都有 residual）。v4 改 OFFSET 端
    // 到端：卡片下降=系统页面抬升的收回（与 IME 滑落同编排，全应用其余
    // 系统键盘位均走此通道真机无瞬移投诉），挂行仍由 v3 轮询闸兜时序；
    // askSysPad 已随 v5 删除（OFFSET 避让自抬聚焦框，补垫=双份）
    this.askResizeOk = false;
    this.askBridgeKb = false;
    this.cancelAskKbSwitch();   // 四十三包：防御性复位（弹窗全新开启）
    this.toolAskHint = hint;
    this.toolAskKind = this.tool;
    this.toolAskErr = '';
    this.toolAskText = this.toolAskDefault(this.tool);
    this.toolAskCaretA = this.toolAskText.length;
    this.toolAskCaretB = this.toolAskCaretA;
    // 二十八包④：键盘避让垫底基准（键盘必未弹出，页高为无键盘全高）
    this.askOpenPageH = this.pageH;
    const grab = (): void => {
      try {
        this.getUIContext().getFocusController().requestFocus('toolAskEditor');
      } catch (_e) {
        // 编辑框尚未挂载完成时忽略，由重试兜底
      }
    };
    if (this.useSysKeyboard) {
      // 系统键盘模式：抢焦（30/150ms 双抓）+ showTextInput 弹 IME。
      // 自带键盘模式内联渲染（askCard），无需焦点
      setTimeout(grab, 30);
      setTimeout(grab, 150);
      setTimeout(() => {
        try {
          inputMethod.getController().showTextInput().catch(() => {
            // 未聚焦等输入法异常：用户点一下输入框唤起
          });
        } catch (_e) {
        }
      }, 350);
    }
  }

  private toolAskDefault(kind: string): string {
    if (kind === 'rotate') {
      return '45';
    }
    if (kind === 'dilate') {
      return '2';
    }
    if (kind === 'angleSize') {
      return '60';
    }
    return '3';   // circleRadius / segLength
  }

  // 确定：参数送回内核（失败保留弹窗就地报错，内核 ask 态不撤、可改再确认）
  private confirmToolAsk(): void {
    const r = dreamemberKernel.kernelToolParam(this.toolAskText);
    if (r.startsWith('done|')) {
      this.closeToolAsk();
      this.applyToolResult(r);
    } else {
      this.toolAskErr = r.startsWith('err|') ? r.substring(4) : '参数无效';
    }
  }

  // 取消：关弹窗并复位内核 ask/待选态（重入同工具 = setTool 复位；工具保持
  // 激活，上游 取消语义）
  private cancelToolAsk(): void {
    this.closeToolAsk();
    this.setTool(this.tool);
    this.refresh();
  }

  private closeToolAsk(): void {
    this.toolAskKind = '';
    this.toolAskErr = '';
    this.askBridgeKb = false;   // 三十三包①：过渡桥随弹窗收摊
    // 四十三包：切回自带的等待期关弹窗（确定/取消/scrim 外无入口）→ 撤
    // 等待定时器；键盘形态保持 system（useSysKeyboard/opt 同对未动），
    // IME 已随 clearFocus 落下，与弹窗关闭一致
    this.cancelAskKbSwitch();
    // 二十九包①曾在此恢复 OFFSET；四十四包 v4 起弹窗全程 OFFSET（不再切
    // RESIZE），本调用保留作防御性复位（弹窗外其余输入位——代数区/表格/
    // 数据区系统键盘——依赖 OFFSET，勿删）
    try {
      this.getUIContext().setKeyboardAvoidMode(KeyboardAvoidMode.OFFSET);
    } catch (_e) {
    }
    // 收键盘：系统键盘随焦点落下；自带键盘是 customKeyboard 随输入框解绑
    try {
      this.getUIContext().getFocusController().clearFocus();
    } catch (_e) {
    }
    this.syncKbSpacer();
  }

  // ---- 二十九包② 滑动条参数编辑（代数区行内展开，上游 滑动条设置同位）----

  // 行值文本点击入口：按代数行协议预填四项（仅 slider 行有效）。先按改名
  // 同款收起新建/修改行（半截输入按隐式 ✓ 提交），再把该行滚到列表顶部
  //（行内展开的四项 + 键盘都在其下方，滚顶即不被键盘遮挡）
  private openSliderEdit(label: string): void {
    const e = this.algebra.find(x => x.label === label);
    if (!e || e.type !== 'slider') {
      return;
    }
    // 点本行值文本 = 收起（开关语义）
    if (this.slideEditLabel === label) {
      this.closeSlideEdit();
      return;
    }
    this.commitPending();
    this.editingLabel = '';
    this.creating = false;
    // 三十三包②：换行编辑显式清键盘态与垫高（syncKbSpacer 行内编辑期间
    // 不清垫），旧垫高动画滑落、新行从静止位展开
    this.slideKbFocus = false;
    this.mathKbVisible = false;
    AppStorage.setOrCreate('ui.mathKbVisible', false);
    this.setSlideSpacer(0);
    this.slideEditLabel = label;
    this.slideEditMin = `${e.min}`;
    this.slideEditMax = `${e.max}`;
    this.slideEditStep = `${e.step}`;
    this.slideEditVal = e.value;
    this.slideEditErr = '';
    this.slideEditField = -1;
    this.slideTapGuardUntil = Date.now() + 400;   // 三十二包③：豁免列表层点按收起
    const idx = this.algebra.findIndex((a: AlgebraEntry): boolean => a.label === label);
    if (idx >= 0) {
      this.algebraScroller.scrollToIndex(idx, true, ScrollAlign.START);
    }
  }

  // 收起行内编辑。restore=true（取消/确定/再点值文本）：滚动对齐可能把编
  // 辑行顶离列表顶（块底贴视口底），收起后恢复到列表顶（与打开时同位）；
  // false（点画布/列表空白收起，三十二包③）：用户滚到哪算哪，不回跳
  private closeSlideEdit(restore: boolean = true): void {
    const idx = restore && this.slideEditLabel.length > 0
      ? this.algebra.findIndex((a: AlgebraEntry): boolean =>
        a.label === this.slideEditLabel)
      : -1;
    this.slideEditLabel = '';
    this.slideEditErr = '';
    this.slideEditField = -1;
    this.slideKbFocus = false;
    this.syncKbSpacer();
    if (idx >= 0) {
      this.algebraScroller.scrollToIndex(idx, true, ScrollAlign.START);
    }
  }

  // 确定：四项一次下内核（setSliderParams 原子应用：校验 max>min → 范围/
  // 步长落位 → 值钳位+吸附+级联重算）。失败就地报错、展开不收可改
  private confirmSlideEdit(): void {
    if (this.slideEditLabel.length === 0) {
      return;
    }
    const minV = parseFloat(this.slideEditMin);
    const maxV = parseFloat(this.slideEditMax);
    const stepV = parseFloat(this.slideEditStep);
    const valV = parseFloat(this.slideEditVal);
    if (!Number.isFinite(minV) || !Number.isFinite(maxV)
      || !Number.isFinite(stepV) || !Number.isFinite(valV)) {
      this.slideEditErr = '四项均须为数字';
      this.reanchorSlideBlock();   // 三十二包②：报错行增高，复查垫高
      return;
    }
    if (!(maxV > minV)) {
      this.slideEditErr = '上限须大于下限';
      this.reanchorSlideBlock();
      return;
    }
    if (dreamemberKernel.kernelSetSliderParams(this.slideEditLabel,
      minV, maxV, stepV, valV) !== 'ok') {
      this.slideEditErr = '滑动条参数无效';
      this.reanchorSlideBlock();
      return;
    }
    const label = this.slideEditLabel;
    this.closeSlideEdit();
    this.showHint('已修改 ' + label);
    this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    this.refresh();
  }

  // ---- 二十九包② 自带键盘四字段协议（ask 单字段协议的多字段版）----
  // 字段 0=最小值 1=最大值 2=步长 3=当前值；光标只跟踪活跃字段，其余字段
  // 聚焦时先落串尾、随选择回调细化

  private slideFieldText(i: number): string {
    if (i === 0) {
      return this.slideEditMin;
    }
    if (i === 1) {
      return this.slideEditMax;
    }
    if (i === 2) {
      return this.slideEditStep;
    }
    return this.slideEditVal;
  }

  private slideFieldSet(i: number, v: string): void {
    if (i === 0) {
      this.slideEditMin = v;
    } else if (i === 1) {
      this.slideEditMax = v;
    } else if (i === 2) {
      this.slideEditStep = v;
    } else {
      this.slideEditVal = v;
    }
  }

  // 字段聚焦：切活跃字段 + 光标落串尾；自带键盘模式下垫高（抽屉/面板随
  // spacer 上移，字段保持在浮层键盘之上）
  private onSlideField(i: number): void {
    this.slideEditField = i;
    this.slideTapGuardUntil = Date.now() + 400;   // 三十二包③：字段点按豁免列表层收起
    const len = this.slideFieldText(i).length;
    this.slideCaretA = len;
    this.slideCaretB = len;
    this.slideKbFocus = !this.useSysKeyboard;
    // 三十一包①b：系统键盘未弹时快照根底未抬位置（OFFSET 抬升参考基准
    // ——手动点按路径 IME 随后弹出、OFFSET 抬升，基准必须在弹起前落定）
    if (this.useSysKeyboard
      && (AppStorage.get<number>('ui.kbH') ?? 0) <= 0) {
      this.snapshotSlideRoot();
    }
    // 三十三包②：垫高只走 syncKbSpacer 的动画管线——换字段几何不变、
    // 收敛复查在动画 onFinish，原地 applySlideAvoid+排程是多余抖动源
    this.syncKbSpacer();
  }

  // 失焦：活跃字段收尾（切到另一字段时其 onFocus 随后覆盖）。三十三包②：
  // 只清字段号，不动垫高/键盘态——换字段时 blur 先于下一字段 focus，这里
  // 清垫正是「先往下闪再往上闪」的根因
  private onSlideBlur(i: number): void {
    if (this.slideEditField !== i) {
      return;
    }
    this.slideEditField = -1;
  }

  private onSlideSel(i: number, s: number, e: number): void {
    if (this.slideEditField !== i) {
      return;
    }
    this.slideCaretA = s;
    this.slideCaretB = e;
  }

  private slideCaretTo(pos: number): void {
    const len = this.slideEditField >= 0
      ? this.slideFieldText(this.slideEditField).length : 0;
    this.slideCaretA = Math.max(0, Math.min(pos, len));
    this.slideCaretB = this.slideCaretA;
  }

  private slideSelRange(): number[] {
    let a = this.slideCaretA;
    let b = this.slideCaretB;
    const len = this.slideEditField >= 0
      ? this.slideFieldText(this.slideEditField).length : 0;
    if (a < 0 || b < 0 || a > len || b > len) {
      a = len;
      b = a;
    }
    if (b < a) {
      const t = a;
      a = b;
      b = t;
    }
    return [a, b];
  }

  private slideInsert(t: string, back: number, skip: string): void {
    if (this.slideEditField < 0) {
      return;
    }
    const i = this.slideEditField;
    const rg = this.slideSelRange();
    const a = rg[0];
    const b = rg[1];
    const cur = this.slideFieldText(i);
    if (skip.length > 0 && a === b && a + skip.length <= cur.length
      && cur.substring(a, a + skip.length) === skip) {
      this.slideCaretTo(a + skip.length);
      return;
    }
    const del = Math.min(back < 0 ? 1 : back, a);
    this.slideFieldSet(i, cur.substring(0, a - del) + t + cur.substring(b));
    this.slideCaretTo(a - del + t.length);
  }

  private slideBackspace(): void {
    this.slideInsert('', 1, '');
  }

  // 键盘收起键：只落键盘，行内编辑保持展开（区别于 ask 卡的收起=确认）。
  // 三十三包②：真收键盘的显式路径——键盘态与垫高在此一步清（动画滑落），
  // 不再借 syncKbSpacer（行内编辑期间它已不清垫）
  private slideKbHide(): void {
    this.slideKbFocus = false;
    this.mathKbVisible = false;
    AppStorage.setOrCreate('ui.mathKbVisible', false);
    this.setSlideSpacer(0);
    try {
      this.getUIContext().getFocusController().clearFocus();
    } catch (_e) {
    }
  }

  // 行内编辑 ⌨ 切键盘（toggleToolAskKeyboard 同构）：切系统抢焦当前字段
  // + showTextInput；切回自带清焦让系统 IME 落下（浮层键盘随字段重新
  // 触摸聚焦弹出，同代数区行为）。三十三包②：切换全程不动垫高——切系统
  // 垫高原地保持、IME 实高事件一步换挡（动画滑变）；切回自带随 IME 落下
  // 动画滑落（浮层键盘要等重新点按才弹，先落垫无跳变）
  private toggleSlideKeyboard(): void {
    this.useSysKeyboard = !this.useSysKeyboard;
    AppStorage.setOrCreate('opt.inputMethod', this.useSysKeyboard ? 0 : 1);
    this.slideTapGuardUntil = Date.now() + 400;   // 三十二包③：⌨ 点按豁免列表层收起
    this.slideKbFocus = false;
    this.mathKbVisible = false;
    AppStorage.setOrCreate('ui.mathKbVisible', false);
    if (this.useSysKeyboard) {
      if (this.slideEditField < 0) {
        return;   // 无焦点字段：垫高原地保持，等 IME 实高事件换挡
      }
      const id = 'slideEditF' + this.slideEditField;
      const grab = (): void => {
        try {
          this.getUIContext().getFocusController().requestFocus(id);
        } catch (_e) {
          // 字段重排中抢焦失败：showTextInput 前还有一跳
        }
      };
      setTimeout(grab, 60);
      setTimeout(() => {
        try {
          inputMethod.getController().showTextInput().catch(() => {
            // 未聚焦等输入法异常：用户点一下输入框唤起
          });
        } catch (_e) {
        }
      }, 350);
    } else {
      this.setSlideSpacer(0);   // 随 IME 收起动画滑落；浮层键盘随重新点按弹出（customKeyboard detach 后程序化抢焦不弹，勿改）
      try {
        this.getUIContext().getFocusController().clearFocus();
      } catch (_e) {
      }
    }
  }

  // 二十七包②：⌨ 切键盘（回写 opt.inputMethod，设置页/面板同项同步）。
  // 自带键盘内联在卡片里（askCard），切回即随状态出现；切系统键盘需抢焦
  // + showTextInput（IME 不随 customKeyboard 解绑自动弹出）；切回时清焦
  // 让已弹的系统 IME 落下。三十三包①：切系统先保挂自带键盘行作过渡桥
  //（卡片原位不动），IME 实高事件到达再动画收合——单段过渡。四十三包：
  // 切回自带补齐对称的另一半——旧序先翻转挂行再清焦，行挂在仍缩着的页
  // 底（系统键盘上沿），IME 落下页高回弹整卡再跳下来；改先清焦落 IME，
  // v3 轮询闸（实测页高回弹 + 350ms 兜底）再动画挂行（completeAskKbSwitch）
  private toggleToolAskKeyboard(): void {
    if (this.askKbSwitching) {
      // 等待挂行期间再按 ⌨ = 反悔回系统键盘：此刻键盘行未挂载、无行可桥，
      // 撤等待后直接抢焦重弹 IME，不设过渡桥
      this.cancelAskKbSwitch();
      this.useSysKeyboard = true;
      AppStorage.setOrCreate('opt.inputMethod', 0);
      this.grabAskSystemKb();
      return;
    }
    this.useSysKeyboard = !this.useSysKeyboard;
    AppStorage.setOrCreate('opt.inputMethod', this.useSysKeyboard ? 0 : 1);
    if (this.useSysKeyboard) {
      // 四十四包 v4：过渡桥不再限于 RESIZE 世界——桥的时序锚=IME 实高事件
      // （watchKbHeight 200ms 收合），与避让模式无关，OFFSET 世界同样适用
      this.askBridgeKb = true;
      this.grabAskSystemKb();
    } else if (this.sysKbH > 0) {
      // IME 正在弹：先落 IME（清焦），轮询等「kbH 归零 + 实测页高回弹 + 350ms
      // 兜底」后再动画挂行——等待期键盘行保持卸载，杜绝「行渲染在屏幕中间」。
      // 事件在本机 ROM 提前报终值不可信（v1/v2 真机两连败），33ms 轮询同步
      // 实测为准（pollAskKbSwitch），1200/2400ms 强制窗口防卡死
      this.askKbSwitching = true;
      this.askKbSwitchStart = Date.now();
      this.askKbZeroAt = 0;
      this.askKbRetried = false;
      try {
        this.getUIContext().getFocusController().clearFocus();
      } catch (_e) {
      }
      this.kbdbg(`switch start kbH=${Math.round(this.sysKbH)} pageH=${Math.round(this.pageH)}`
        + ` measured=${Math.round(this.measureAskPageH())} base=${Math.round(this.askOpenPageH)}`
        + ` resizeOk=${this.askResizeOk}`);
      if (this.askKbPollTimer >= 0) {
        clearInterval(this.askKbPollTimer);
      }
      this.askKbPollTimer = setInterval(() => this.pollAskKbSwitch(), 33);
    }
    // sysKbH<=0：系统键盘本未弹，原地翻转即挂行，无过渡问题
  }

  // 抢焦 + showTextInput（切系统分支/反悔路径共用；openToolAsk 首开另有
  // 30/150ms 双抓序列，不动）
  private grabAskSystemKb(): void {
    const grab = (): void => {
      try {
        this.getUIContext().getFocusController().requestFocus('toolAskEditor');
      } catch (_e) {
        // 卡片重排中抢焦失败：showTextInput 前还有一跳
      }
    };
    setTimeout(grab, 60);
    setTimeout(() => {
      try {
        inputMethod.getController().showTextInput().catch(() => {
          // 未聚焦等输入法异常：用户点一下输入框唤起
        });
      } catch (_e) {
      }
    }, 350);
  }

  // 挂行落点（轮询闸通过或强制窗口到达）：动画挂行——行在最终位置展开，
  // 与反向桥收合（watchKbHeight 200ms EaseOut）对称
  private completeAskKbSwitch(): void {
    if (this.askKbPollTimer >= 0) {
      clearInterval(this.askKbPollTimer);
      this.askKbPollTimer = -1;
    }
    if (!this.askKbSwitching) {
      return;
    }
    this.askKbSwitching = false;
    this.kbdbg(`mount row elapsed=${Date.now() - this.askKbSwitchStart}`);
    this.getUIContext().animateTo({
      duration: 200,
      curve: Curve.EaseOut
    }, () => {
      this.useSysKeyboard = false;
      // 四十四包 v5：桥随翻转复位——过渡期等高占位与键盘行同高，换回时
      // 输入框零位移（桥已无塌陷位移可言，v3 挂行闸只剩时序兜底价值）
      this.askBridgeKb = false;
      // opt 回写同步触发 onInputMethodOpt 置同值，同一动画闭包内捕获，
      // 不产生第二段跳变
      AppStorage.setOrCreate('opt.inputMethod', 1);
    });
  }

  // v3 挂行闸：33ms 轮询同步实测，不吃事件时序——本机 ROM 在隐藏动画开始
  // 即报终值，v1 的 kbH 闸、v2 的 kbH+pageH 态双闸都被提前击穿（真机两连
  // 败）。通过条件 = kbH 已归零 且 实测页高回到弹窗打开基准（askOpenPageH=
  // 打开以来最大页高）且 kbH 归零后已过 350ms（时序兜底：OFFSET 世界实测
  // 不出位移、纯时序保底）。1200ms 强制窗：IME 仍挂补一次清焦、窗口推到
  // 2400ms，仍不动才强挂（防个别 ROM 事件/布局全缺卡死）
  private pollAskKbSwitch(): void {
    if (!this.askKbSwitching) {
      return;
    }
    const now = Date.now();
    const elapsed = now - this.askKbSwitchStart;
    if (this.sysKbH > 0) {
      if (elapsed < 1200) {
        return;
      }
      if (!this.askKbRetried) {
        this.askKbRetried = true;
        this.askKbSwitchStart = now - 1200;   // 强制窗口推到 2400ms
        this.kbdbg(`force retry clearFocus elapsed=${elapsed} kbH=${Math.round(this.sysKbH)}`);
        try {
          this.getUIContext().getFocusController().clearFocus();
        } catch (_e) {
        }
        return;
      }
      this.kbdbg(`force complete (ime stuck) elapsed=${elapsed} kbH=${Math.round(this.sysKbH)}`);
      this.completeAskKbSwitch();
      return;
    }
    if (this.askKbZeroAt === 0) {
      this.askKbZeroAt = now;
      this.kbdbg(`kbZero observed elapsed=${elapsed} pageH=${Math.round(this.pageH)}`
        + ` measured=${Math.round(this.measureAskPageH())}`);
    }
    const kbAge = now - this.askKbZeroAt;
    const measured = this.measureAskPageH();
    const heightOk = this.askOpenPageH <= 0 || measured <= 0
      || measured + 4 >= this.askOpenPageH;
    if (heightOk && kbAge >= 350) {
      this.kbdbg(`gate pass kbAge=${kbAge} elapsed=${elapsed} pageH=${Math.round(this.pageH)}`
        + ` measured=${Math.round(measured)} base=${Math.round(this.askOpenPageH)}`);
      this.completeAskKbSwitch();
      return;
    }
    if (elapsed >= 2400) {
      this.kbdbg(`force complete (never recovered) elapsed=${elapsed}`
        + ` measured=${Math.round(measured)} base=${Math.round(this.askOpenPageH)}`);
      this.completeAskKbSwitch();
    }
  }

  private cancelAskKbSwitch(): void {
    if (this.askKbPollTimer >= 0) {
      clearInterval(this.askKbPollTimer);
      this.askKbPollTimer = -1;
    }
    if (this.askKbSwitching) {
      this.kbdbg(`switch cancelled at ${Date.now() - this.askKbSwitchStart}ms`);
    }
    this.askKbSwitching = false;
  }

  // kbH 事件（真机调试期兼 KBDBG 埋点）：挂行判定已移交 pollAskKbSwitch
  private onAskSysKb(): void {
    this.kbdbg(`kbH event=${Math.round(this.sysKbH)}`);
  }

  // 页高事件（根 onAreaChange；埋点观察本机 ROM 事件时序）
  private onAskPageRecovered(): void {
    this.kbdbg(`pageH event=${Math.round(this.pageH)}`);
  }

  // v3 实测：geoRoot 当前页高（vp）。componentUtils 读真实布局，不受
  // @State/onAreaChange 时序影响；失败返回 -1（调用方退时序兜底）
  private measureAskPageH(): number {
    try {
      const r = componentUtils.getRectangleById('geoRoot');
      return this.getUIContext().px2vp(r.size.height as number);
    } catch (_e) {
      return -1;
    }
  }

  // KBDBG：⌨ 切键盘调试埋点（hilog tag=KBDBG，真机 hilog 过滤即得全时间线）
  private kbdbg(msg: string): void {
    hilog.info(0x0000, 'KBDBG', `${msg}`);
  }

  // ---- 自带键盘插入协议（表格范围字段同款简化版，单字段）----

  private askCaretTo(i: number): void {
    const v = Math.max(0, Math.min(i, this.toolAskText.length));
    this.toolAskCaretA = v;
    this.toolAskCaretB = v;
    try {
      this.toolAskCtrl.caretPosition(v);
    } catch (_e) {
    }
  }

  private askSelRange(): number[] {
    let a = this.toolAskCaretA;
    let b = this.toolAskCaretB;
    const len = this.toolAskText.length;
    if (a < 0 || b < 0 || a > len || b > len) {
      a = len;
      b = a;
    }
    if (b < a) {
      const t = a;
      a = b;
      b = t;
    }
    return [a, b];
  }

  private askInsert(t: string, back: number, skip: string): void {
    const rg = this.askSelRange();
    const a = rg[0];
    const b = rg[1];
    const cur = this.toolAskText;
    if (skip.length > 0 && a === b && a + skip.length <= cur.length
      && cur.substring(a, a + skip.length) === skip) {
      this.askCaretTo(a + skip.length);
      return;
    }
    const del = Math.min(back < 0 ? 1 : back, a);
    this.toolAskText = cur.substring(0, a - del) + t + cur.substring(b);
    this.askCaretTo(a - del + t.length);
  }

  private askBackspace(): void {
    this.askInsert('', 1, '');
  }

  // 弹窗自带键盘面板（表格 kbArea 同构；提交键/收起 = 确认参数）
  @Builder
  private askKbArea() {
    Row() {
      Column() {
        MathKeyboard({
          onInsert: (t: string, back: number, skip: string): void => {
            this.askInsert(t, back, skip);
          },
          onBackspace: (): void => this.askBackspace(),
          onSubmit: (): void => {
            this.confirmToolAsk();
          },
          onHide: (): void => {
            this.confirmToolAsk();
          },
          onSwitchSystem: (): void => this.toggleToolAskKeyboard(),
          onCaretLeft: (): void => this.askCaretTo(this.toolAskCaretA - 1),
          onCaretRight: (): void => this.askCaretTo(this.toolAskCaretA + 1)
        })
      }
      .width(this.isWide ? KB_WIDE_MAX_WIDTH : '100%')
      .height('100%')
      .borderRadius(this.isWide ? 12 : 0)
      .clip(true)
    }
    .width('100%')
    .height(MATH_KB_HEIGHT)
    .justifyContent(FlexAlign.Center)
  }

  // 弹窗参数输入框（askEditorGen 换支重建的公共体；customKeyboard 随
  // useSysKeyboard 解绑/重挂）
  @Builder
  private askEditorField() {
    TextInput({ text: this.toolAskText, controller: this.toolAskCtrl })
      .id('toolAskEditor')
      .fontSize(14)
      .height(34)
      .layoutWeight(1)
      .backgroundColor(Theme.inputBg(this.dark))
      .onChange((v: string) => {
        this.toolAskText = v;
      })
      .onFocus(() => {
        this.toolAskCaretA = this.toolAskText.length;
        this.toolAskCaretB = this.toolAskCaretA;
      })
      .onTextSelectionChange((s: number, e: number) => {
        this.toolAskCaretA = s;
        this.toolAskCaretB = e;
      })
      .onSubmit(() => this.confirmToolAsk())
      // 四十四包 v5（用户真机反馈①）：自带键盘态挂 1px 空壳（三十六包
      // 同款，挂载即抑制系统 IME）——点输入框只聚焦出光标，不再弹系统键
      // 盘盖住自带键盘行；系统键盘态传 undefined 解绑（API 22+ 官方支持，
      // AlgebraPanel 同款用法），IME 照常可弹
      .customKeyboard(this.useSysKeyboard ? undefined : this.askEmptyKb())
  }

  // 弹窗输入框的空壳键盘（挂载即抑制系统 IME；AlgebraPanel.emptyKb 同款）
  @Builder
  private askEmptyKb() {
    Row()
      .width('100%')
      .height(1)
      .opacity(0)
  }

  // 弹窗本体：页面根 Stack 的底部模态。头部行 = 提示文案 + 右上 ⌨ 切键盘
  // （二十七包②）。scrim 挡掉弹窗外全部点击；自带键盘内联在卡片内
  // （askCard），系统键盘避让交给窗口 OFFSET（部分抬升聚焦框）
  @Builder
  private toolAskDialog() {
    Stack({ alignContent: Alignment.Bottom }) {
      Column()
        .width('100%')
        .height('100%')
        .backgroundColor('rgba(0,0,0,0.18)')
        .onClick(() => {
          // 点空白不取消（防误触丢拾取），取消走按钮
        })
      // 弹窗卡片（⌨ 切键盘 = 整窗收起重开，见 toggleToolAskKeyboard）
      this.askCard()
    }
    .width('100%')
    .height('100%')
  }

  // 弹窗卡片（标题行 + 参数行 + 错误行 + 自带键盘行）。二十七包②定案：
  // 自带键盘**内联**渲染（曾用 TextInput.customKeyboard 浮层——弹出不随
  // 状态走，重挂后任何程序化抢焦都不再弹出、触摸聚焦才可以，模拟器实测
  // 三种挂载方式均复现）；内联后可见性纯状态驱动，⌨ 切换永不失效。四十四
  // 包 v5：切系统键过渡期行换**等高透明占位**（askBridgeKb，输入框零位
  // 移），IME 收起才塌；输入框挂空壳 customKeyboard 抑制自带键盘态误弹
  // 系统 IME（askEditorField）
  @Builder
  private askCard() {
    Column({ space: 10 }) {
      Row() {
        Text(this.toolAskHint)
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.text(this.dark))
          .layoutWeight(1)
        Text('⌨')
          .fontSize(15)
          .fontColor(Theme.sub(this.dark))
          .width(30)
          .height(28)
          .textAlign(TextAlign.Center)
          .onClick(() => this.toggleToolAskKeyboard())
      }
      .width('100%')
      Row() {
        this.askEditorField()
        Text('确定')
          .fontSize(13)
          .fontColor('#FFFFFF')
          .backgroundColor(Theme.accent(this.dark))
          .borderRadius(6)
          .height(34)
          .padding({ left: 14, right: 14 })
          .margin({ left: 8 })
          .onClick(() => this.confirmToolAsk())
        Text('取消')
          .fontSize(13)
          .fontColor(Theme.sub(this.dark))
          .borderRadius(6)
          .height(34)
          .padding({ left: 14, right: 14 })
          .margin({ left: 6 })
          .border({ width: 0.5, color: Theme.border(this.dark) })
          .onClick(() => this.cancelToolAsk())
      }
      .width('100%')
      if (this.toolAskErr.length > 0) {
        Text(this.toolAskErr)
          .fontSize(11)
          .fontColor(Theme.error(this.dark))
          .width('100%')
      }
      if (!this.useSysKeyboard) {
        this.askKbArea()
      } else if (this.askBridgeKb) {
        // 四十四包 v5（用户真机反馈②）：切系统键过渡期键盘行换**等高透明
        // 占位**——输入框零位移（本机 OFFSET 避让聚焦后不重算，行塌掉=
        // 输入框沉入 IME 被盖的根因）；占位区与升起的 IME 重叠不可见，
        // IME 收起时塌占位（watchKbHeight px<=0 分支）输入框自然滑落
        Column()
          .width('100%')
          .height(MATH_KB_HEIGHT)
      }
    }
    .width('100%')
    .padding({
      left: 14, right: 14, top: 12,
      bottom: 12
    })
    .backgroundColor(Theme.drawer(this.dark))
    .borderRadius({ topLeft: 14, topRight: 14 })
  }

  // 动画定时器：推进滑动条 → 重绘 + 代数区节流同步；无动画自动停止
  private startAnimTicker(): void {
    if (this.animTimer >= 0) {
      return;
    }
    this.animTimer = setInterval(() => {
      const frame = dreamemberKernel.kernelAnimTick(0.05);
      if (frame === '-|') {
        this.stopAnimTicker();
        if (this.showPanel) {
          this.syncAlgebra(dreamemberKernel.kernelAlgebra());
        }
        return;
      }
      this.execFrame(frame);
      const now = Date.now();
      if (this.showPanel && now - this.lastAlgebraMs > 200) {
        this.lastAlgebraMs = now;
        this.syncAlgebra(dreamemberKernel.kernelAlgebra());
      }
    }, 50);
  }

  private stopAnimTicker(): void {
    if (this.animTimer >= 0) {
      clearInterval(this.animTimer);
      this.animTimer = -1;
    }
  }

  private toggleAnim(label: string): void {
    this.commitPending();
    const r = dreamemberKernel.kernelAnimateToggle(label);
    if (r.startsWith('+|')) {
      if (r.charAt(2) === '1') {
        this.startAnimTicker();
      }
      this.syncAlgebra(dreamemberKernel.kernelAlgebra());
      this.refresh();
    }
  }

  // ---- 画布 ↔ 代数区选中联动（CanvasStage 经回调上达） ----

  // 画布请求选中 label（'' = 清除）。''→'' 去重：单指按下高频触发，避免
  // 每次按下都拉一次对象列表
  private canvasSelect(label: string): void {
    if (label === '' && this.selectedLabel === '') {
      return;
    }
    this.selectedLabel = label;
    dreamemberKernel.kernelSelect(label);
    this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    this.refresh();
  }

  // 拖点过程中的代数区同步（final=true = 抬起补一次最终同步）
  private onDragSync(final: boolean): void {
    if (!this.showPanel) {
      return;
    }
    const now = Date.now();
    if (!final && now - this.lastAlgebraMs <= 50) {
      return;
    }
    this.lastAlgebraMs = now;
    this.syncAlgebra(dreamemberKernel.kernelAlgebra());
  }

  // ---- N3 菜单/设置事件 ----

  // 显示开关变化：下发内核并整帧重绘（画布未就绪时内核标志也已生效，onReady 后生效）
  private onDisplayOpt(): void {
    const frame = dreamemberKernel.kernelSetOptions(this.showGridOpt, this.showAxesOpt);
    if (this.canvasW > 1 && this.canvasH > 1) {
      this.execFrame(frame);
    }
  }

  private onResetViewEvt(): void {
    const frame = dreamemberKernel.kernelResetView();
    if (this.canvasW > 1 && this.canvasH > 1) {
      this.execFrame(frame);
    }
  }

  private onClearAllEvt(): void {
    this.stopAnimTicker();
    this.setTool('');
    this.syncAlgebra(dreamemberKernel.kernelClearAll());
    this.selectedLabel = '';
    dreamemberKernel.kernelSelect('');
    this.inputError = '';
    this.pendingInput = '';
    this.mathCaret = 0;
    this.editingLabel = '';
    this.creating = false;
    this.refresh();
  }

  private onDarkChanged(): void {
    this.refresh();
  }

  private onDismissKbEvt(): void {
    this.dismissKeyboard();
  }

  // 第五包：☰ 菜单"视图"→ CAS 视图（上游经典版 同位）：非代数视图
  // 打开时收键盘；抽屉开合动画与 openTab 一致。CAS 不在底部四 Tab 内
  //（上游 中它也不是 Tab，是独立视图），打开时四 Tab 无高亮属预期
  private onOpenCasEvt(): void {
    this.commitPending();
    this.dismissKeyboard();
    this.getUIContext().animateTo({ duration: 260, curve: Curve.EaseInOut }, () => {
      this.showPanel = true;
      this.panelTab = 'cas';
    });
  }

  // ---- N4 .ggb 文件（打开/保存） ----

  // 保存默认名：软件名 + 日期 + 时间（真机反馈）
  private defaultGgbName(): string {
    const d = new Date();
    const p = (n: number): string => (n < 10 ? '0' + n : '' + n);
    return `梦烬几何_${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}_` +
      `${p(d.getHours())}${p(d.getMinutes())}${p(d.getSeconds())}.ggb`;
  }

  // 载入 .ggb 字节（内核 zip+XML 重建）。返回提示/错误（空串=成功）
  public loadGgb(data: ArrayBuffer): string {
    const r: string = dreamemberKernel.kernelLoadGgb(data);
    if (r.startsWith('error:')) {
      return r.substring(6);
    }
    this.stopAnimTicker();
    this.setTool('');
    this.selectedLabel = '';
    dreamemberKernel.kernelSelect('');
    this.editingLabel = '';
    this.creating = false;
    let rows = r;
    let warn = '';
    if (r.startsWith('warn:')) {
      const nl = r.indexOf('\n');
      warn = r.substring(5, nl < 0 ? r.length : nl);
      rows = nl < 0 ? '' : r.substring(nl + 1);
    }
    this.syncAlgebra(rows);
    this.inputError = warn;
    this.panelTab = 'algebra';
    this.setPanelVisible(true);
    this.refresh();
    if (dreamemberKernel.kernelAnyAnimating() === '1') {
      this.startAnimTicker();
    }
    return '';
  }

  private alertBox(title: string, msg: string): void {
    AlertDialog.show({
      title: title,
      message: msg,
      autoCancel: true,
      alignment: DialogAlignment.Center,
      primaryButton: {
        value: '确定',
        action: () => {}
      }
    });
  }

  private onOpenGgbEvt(): void {
    const docPicker = new picker.DocumentViewPicker();
    const opts = new picker.DocumentSelectOptions();
    opts.maxSelectNumber = 1;
    docPicker.select(opts).then((uris: Array<string>) => {
      if (uris.length === 0) {
        return;   // 用户取消
      }
      const uri = uris[0];
      const file = fs.openSync(uri, fs.OpenMode.READ_ONLY);
      let err = '';
      try {
        const size = fs.statSync(file.fd).size;
        const buf = new ArrayBuffer(size);
        fs.readSync(file.fd, buf);
        err = this.loadGgb(buf);
      } finally {
        fs.closeSync(file);
      }
      if (err.length > 0) {
        this.alertBox('打开失败', err);
      }
    }).catch(() => {
      // 取消或无权限：静默
    });
  }

  private onSaveGgbEvt(): void {
    const docPicker = new picker.DocumentViewPicker();
    const opts = new picker.DocumentSaveOptions();
    opts.newFileNames = [this.defaultGgbName()];
    docPicker.save(opts).then((uris: Array<string>) => {
      if (uris.length === 0) {
        return;   // 用户取消
      }
      const uri = uris[0];
      const file = fs.openSync(uri, fs.OpenMode.READ_WRITE | fs.OpenMode.TRUNC);
      try {
        fs.writeSync(file.fd, dreamemberKernel.kernelGetGgb());
      } finally {
        fs.closeSync(file);
      }
      this.alertBox('保存成功', uri.substring(uri.lastIndexOf('/') + 1));
    }).catch(() => {
      // 取消：静默
    });
  }

  // 收回键盘（上游 行为：点画布/打开菜单即收起）：结束编辑会话 + 清焦点，
  // 保证下次点输入框能重新弹出。三十二包③：点画布 = 点别处，滑动条行内
  // 编辑区一并收起（不回滚列表滚动位置）
  private dismissKeyboard(): void {
    this.activeCtrl().stopEditing();
    try {
      this.getUIContext().getFocusController().clearFocus();
    } catch (e) {
      // 焦点清除失败时 stopEditing 已收起键盘
    }
    this.inputFocused = false;
    this.slideKbFocus = false;   // 二十九包②：滑动条字段键盘一并收起
    this.syncKbSpacer();
    if (this.slideEditLabel.length > 0) {
      this.closeSlideEdit(false);
    }
  }

  // ---- N3 数学虚拟键盘 ----

  // 自绘键盘 ⇄ 系统键盘（上游 处理方式：自绘优先，键盘上留 ⌨ 切换键）
  private toggleKeyboard(): void {
    this.useSysKeyboard = !this.useSysKeyboard;
    // 二十四包②：⌨ 切换回写设置（设置页/数据区 ⌨ 同步同一状态）
    AppStorage.setOrCreate('opt.inputMethod', this.useSysKeyboard ? 0 : 1);
    const toSystem = this.useSysKeyboard;
    this.activeCtrl().stopEditing();   // 先收起当前键盘
    this.syncKbSpacer();
    if (toSystem) {
      // 切到系统键盘：重新聚焦输入框后向输入法框架请求弹出
      setTimeout(() => {
        try {
          this.getUIContext().getFocusController().requestFocus(this.activeInputId());
          setTimeout(() => {
            inputMethod.getController().showTextInput().catch(() => {
              // 未聚焦等输入法异常时退化为：用户点一下输入框唤起
            });
          }, 150);
        } catch (e) {
          // 重新聚焦失败则由用户点一下输入框唤起
        }
      }, 100);
    } else {
      // 切回自绘键盘：重新聚焦即随焦点弹出 customKeyboard；
      // MathField 光标取最近选区锚点（系统键盘模式下的选区回传仍有效）
      this.mathCaret = this.selEnd >= 0 && this.selEnd <= this.pendingInput.length
        ? this.selEnd : this.pendingInput.length;
      this.mathCaretScript = false;
      setTimeout(() => {
        try {
          this.getUIContext().getFocusController().requestFocus(this.activeInputId());
        } catch (e) {
          // 同上，退化为用户点击
        }
      }, 100);
    }
  }

  // 二十四包②：数据区头部 ⌨ 切换键盘；二十五包②表格范围编辑同款复用
  // （改名 togglePanelKeyboard）——同一设置项（回写 opt.inputMethod，设置
  // 页同步）；面板编辑没有代数输入会话，不做输入行重新聚焦流程
  private togglePanelKeyboard(): void {
    this.useSysKeyboard = !this.useSysKeyboard;
    AppStorage.setOrCreate('opt.inputMethod', this.useSysKeyboard ? 0 : 1);
    this.syncKbSpacer();
  }

  // 数学键盘可见状态（customKeyboard 是浮层，不参与避让，不垫高会盖住输
  // 入框；系统键盘模式由系统自身避让，不垫）。@State 驱动本组件宽/窄屏垫
  // 高（第十七包②起页面级垫高已删，AppStorage 副本仅留作状态广播）
  @State private mathKbVisible: boolean = false;
  // 三十六包：顶层键盘面板下探导航条避让区用（浮层旧锚窗口底，自绘面板
  // 锚页面底，需平移 navGap 才与浮层像素重合；避让目标公式不变）
  @StorageProp('ui.navBottomVp') navBottomVp: number = 0;
  // 三十六包：数据区/表格编辑会话期注册的面板键盘处理束（onKbHooks 回写；
  // null=无会话）。滑动条/代数输入行逻辑在宿主，分发时直连不经此处
  private sheetKbHooks: MathKbHooks | null = null;
  private tableKbHooks: MathKbHooks | null = null;
  // 页面总高（vp，根 onAreaChange 维护）：键盘避让实测基准。四十三包：
  // @Watch 兼作 ⌨ 切回自带的第二挂行条件——kbH 归零可能是 IME 隐藏动画
  // 开始即报 0（页未回弹），页高回到打开基准才算落定
  @State @Watch('onAskPageRecovered') private pageH: number = 0;
  // 根在窗口坐标系的 y（原值，root onAreaChange 维护）——输入行
  // globalPosition 与它做差归一成「距页面顶」，消掉状态栏等页面外偏移
  private rootY: number = 0;
  // 第十七包②：键盘垫改**运行时实测**——目标 = 可见输入行底边贴键盘上沿
  // +4（用户口径"系统键盘的避让刚好让输入框在键盘上面"）。静态值两轮都
  // 对不准（十五包 178 差一行、十六包 220 差两行），实测一次性消除。
  // 第十八包①修通：键盘弹出是浮层、不触发任何布局变化 → onAreaChange
  // 在「可见翻转」之后再无回调，首版只在上报事件里算垫高 → 垫高恒 0、
  // 输入行整个压在键盘下面（真机）。修复：输入行几何随上报随缓存，可见
  // 翻转时刻用缓存几何立即反解（另留一次性延迟重试兜底极端时序）。
  // 第十九包①定案"多避让一块"：目标原按**页面底**反解，但页面底悬在导航
  // 条避让区上方，而 customKeyboard 浮层锚**窗口底**——三轮数据闭环（十五
  // 包 228 多一行、十六包 270 多两行、十八包实测 224 仍多一块，差额恒 =
  // 导航条高 + 常数）。修复：目标改按窗口底折算（键盘高 − 导航条高 + 4，
  // 导航条高由 Ability 查 TYPE_NAVIGATION_INDICATOR 存 AppStorage）
  @State private kbSpacer: number = 0;
  private lastInputArea: Area | null = null;
  // 三十一包①a/①b：滑动条避让复查定时器（去抖）
  private slideAvoidTimer: number = -1;
  // 三十五包①：静止闸门——块底矩形连续两拍一致才实测解算（定时链/上拍
  // 签名/重试计数，动画在飞不计时）
  private slideStableTimer: number = -1;
  private slideStableSig: number = -1;
  private slideStableTry: number = 0;
  // 三十一包①a：滚动对齐卡住检测——上轮 scrollBy 量（差值≤1 判滚动未
  // 生效，停止排程；只向下滚故无振荡，无需符号阻尼）
  private slideAlignLastDy: number = 0;
  // 三十一包①b：OFFSET 抬升参考——根底未抬时的窗口 y（px）。键盘未弹
  // （系统态）/自带键盘态页面必然未被系统抬升，此刻随测随新；系统键盘
  // 弹起期间冻结（保留弹起前基准）
  private slideRootBottomPx: number = 0;
  // 三十二包③：列表层「点空白收起」的豁免时间窗（ms 时点）——开块/换字
  // 段/切键盘的点按若按冒泡模型连发到列表 onClick，会立即收掉刚开的编辑
  // 区；这些入口先把护栏推到 400ms 后，列表层点按在此前不生效
  private slideTapGuardUntil: number = 0;
  // 三十三包②：垫高动画在飞——期间挂起实测复查（animateTo 闭包里状态已
  // 终值而画面在途中，此刻实测会拿到中间值把已到位的垫高改飞），落定后
  // onFinish 复查一次收敛
  private slidePadAnimating: boolean = false;
  // 三十三包①曾记弹窗期 RESIZE 试设结果；四十四包 v4 起弹窗不再切
  // RESIZE（OFFSET 端到端）故恒 false，仅留作 KBDBG 诊断字段
  private askResizeOk: boolean = false;
  // 三十三包①：⌨ 切系统键的过渡桥——自带键盘行保挂到 IME 实高事件到达
  //（页缩随动）再动画收合，卡片单段过渡，不再「掉下去再弹上来」
  @State private askBridgeKb: boolean = false;
  // 四十三包：⌨ 切回自带的等待态——系统→自带与反向不同，行不能先挂
  //（旧序先挂行再清焦，行挂在仍缩着的页底=系统键盘上沿=屏幕中间，IME 落
  // 下页高回弹整卡再跳下来，真机割裂，旋转/定长线段弹窗同报）。v1（kbH
  // 闸）/v2（kbH+pageH 态双闸）真机两连败根因：本机 ROM 的相关事件在隐
  // 藏动画**开始**即报终值，事件闸全数提前击穿。v3 改 33ms 轮询 + component
  // Utils 同步实测 geoRoot 页高（布局真值，不吃事件时序）+ kbH 归零后
  // 350ms 时序兜底 + 1200/2400ms 强制窗口，KBDBG 埋点全程落 hilog
  @State private askKbSwitching: boolean = false;
  private askKbPollTimer: number = -1;      // v3 轮询（33ms）
  private askKbSwitchStart: number = 0;     // 切换起点（ms）
  private askKbZeroAt: number = 0;          // 轮询中首次观察到 kbH=0（ms）
  private askKbRetried: boolean = false;    // 强制窗口补清焦标记
  private syncKbSpacer(): void {
    // 二十九包②：滑动条行内编辑字段聚焦（自带键盘浮层）→ 按键盘实高整垫。
    // 钉底输入行的实测反解不适用——目标字段在列表顶部，抽屉整体随垫高上移
    // 后字段必在浮层之上；画布相应压缩
    if (this.slideKbFocus && !this.useSysKeyboard && !this.inputFocused
      && !this.sheetKbFocus && !this.tableKbFocus) {
      // 三十五包①：换字段零动作（三十三包②语义回归）——键盘在挂、垫高
      // 已收敛时字段切换不写任何状态。三十四包在此无条件同步实测，测值撞
      // 上布局/滚动中间态就重解垫高 = 编辑区先上跳再回缩（真机①「中途点
      // 其他编辑框有时复现」；系统键盘态本就不进此分支故无此病——与用户
      // 观察一致）
      if (this.mathKbVisible && this.kbSpacer > 0) {
        return;
      }
      this.mathKbVisible = true;
      AppStorage.setOrCreate('ui.mathKbVisible', true);
      // 三十五包①：首聚焦改静止闸门解算——块刚展开/列表滚动对齐在途时
      // 窗口矩形是中间值，同步实测必得偏大垫高、落定后复查回缩 =「先向上
      // 一大块再缩回来」（真机三十四包①首聚焦，自带/系统两键盘同病；系
      // 统态走 keyboardHeightChange 复查同闸门）。等块底矩形两拍一致再一
      // 步解算；整垫兜底移入闸门出口（gatedSlideAvoid）
      this.scheduleSlideAvoid(0);
      return;
    }
    // 三十三包②：行内编辑期间垫高归滑动条管线——blur/⌨ 切换等中间态翻
    // 转一律不清垫，「清了再重垫」正是换字段/切键盘先下后上闪的根因；真
    // 正收键盘都走显式路径（slideKbHide/closeSlideEdit/toggleSlideKeyboard）
    if (this.slideEditLabel.length > 0) {
      return;
    }
    const visible = (this.inputFocused || this.sheetKbFocus || this.tableKbFocus)
      && !this.useSysKeyboard;
    this.mathKbVisible = visible;
    AppStorage.setOrCreate('ui.mathKbVisible', visible);
    if (!visible) {
      this.kbSpacer = 0;
      AppStorage.setOrCreate('ui.kbSpacer', 0);
      return;
    }
    this.applyKbSpacer();
    if (this.lastInputArea === null) {
      setTimeout(() => this.applyKbSpacer(), 120);
    }
  }

  // ---- 三十六包：顶层键盘分发（mathKbOverlay 的 MathKeyboard 接线）----
  // 按活跃上下文路由：输入行 → 数据区 → 表格 → 滑动条。顺序即互斥序：
  // 同拍多标志时聚焦态优先（如滑动条会话开着点输入行，inputFocused 先翻
  // 真，键入必须进输入行而不是被会话吞掉）
  private kbKeyInsert(t: string, back: number, skip: string): void {
    if (this.inputFocused) {
      this.insertAtCaret(t, back, skip);
    } else if (this.sheetKbFocus && this.sheetKbHooks !== null) {
      this.sheetKbHooks.insert(t, back, skip);
    } else if (this.tableKbFocus && this.tableKbHooks !== null) {
      this.tableKbHooks.insert(t, back, skip);
    } else {
      this.slideInsert(t, back, skip);
    }
  }

  private kbKeyBackspace(): void {
    if (this.inputFocused) {
      this.backspaceAtCaret();
    } else if (this.sheetKbFocus && this.sheetKbHooks !== null) {
      this.sheetKbHooks.backspace();
    } else if (this.tableKbFocus && this.tableKbHooks !== null) {
      this.tableKbHooks.backspace();
    } else {
      this.slideBackspace();
    }
  }

  private kbKeySubmit(): void {
    if (this.inputFocused) {
      this.submitActive();
    } else if (this.sheetKbFocus && this.sheetKbHooks !== null) {
      this.sheetKbHooks.submit();
    } else if (this.tableKbFocus && this.tableKbHooks !== null) {
      this.tableKbHooks.submit();
    } else {
      this.confirmSlideEdit();
    }
  }

  private kbKeyHide(): void {
    if (this.inputFocused) {
      this.activeCtrl().stopEditing();
    } else if (this.sheetKbFocus && this.sheetKbHooks !== null) {
      this.sheetKbHooks.hide();
    } else if (this.tableKbFocus && this.tableKbHooks !== null) {
      this.tableKbHooks.hide();
    } else {
      this.slideKbHide();
    }
  }

  private kbKeyCaret(dir: number): void {
    if (this.inputFocused) {
      this.caretMove(dir);
    } else if (this.sheetKbFocus && this.sheetKbHooks !== null) {
      if (dir < 0) {
        this.sheetKbHooks.caretLeft();
      } else {
        this.sheetKbHooks.caretRight();
      }
    } else if (this.tableKbFocus && this.tableKbHooks !== null) {
      if (dir < 0) {
        this.tableKbHooks.caretLeft();
      } else {
        this.tableKbHooks.caretRight();
      }
    } else {
      this.slideCaretTo(this.slideCaretA + dir);
    }
  }

  private kbKeySwitch(): void {
    if (this.inputFocused) {
      this.toggleKeyboard();
    } else if (this.sheetKbFocus || this.tableKbFocus) {
      this.togglePanelKeyboard();
    } else {
      this.toggleSlideKeyboard();
    }
  }

  // 三十六包：页面顶层自带键盘面板（根 Stack 最后一个常规子节点 = 浮层
  // 同位）。浮层空壳化（各输入框挂 1px 透明 customKeyboard 仅抑制系统
  // IME）后键盘本体唯一化到这里：换字段/换格的焦点转移让旧浮层收起+新
  // 浮层弹出＝键盘重弹（真机①：原生 per-field 生命周期，无 keep-alive）；
  // 面板改由 mathKbVisible 驱动常驻顶层，换字段时该状态本就不动（三十五
  // 包①零动作）＝视觉零变化。几何与浮层像素重合：高 220、页底对齐后平
  // 移 navGap 下探导航条避让区（浮层锚窗口底），避让目标公式不受影响；
  // 可见性加活跃上下文条件防会话残留（弹窗开时 drawers 卸载、blur 链可
  // 能断）。显隐过渡 220ms 滑变对齐浮层动画体感
  @Builder
  private mathKbOverlay() {
    Row() {
      Column() {
        MathKeyboard({
          onInsert: (t: string, back: number, skip: string): void =>
            this.kbKeyInsert(t, back, skip),
          onBackspace: (): void => this.kbKeyBackspace(),
          onSubmit: (): void => this.kbKeySubmit(),
          onHide: (): void => this.kbKeyHide(),
          onSwitchSystem: (): void => this.kbKeySwitch(),
          onCaretLeft: (): void => this.kbKeyCaret(-1),
          onCaretRight: (): void => this.kbKeyCaret(1)
        })
      }
      .width(this.isWide ? KB_WIDE_MAX_WIDTH : '100%')
      .height('100%')
      .borderRadius(this.isWide ? 12 : 0)
      .clip(true)
    }
    .width('100%')
    .height(MATH_KB_HEIGHT)
    .justifyContent(FlexAlign.Center)
    .translate({ y: this.navBottomVp })
    .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.BOTTOM])
    .transition(TransitionEffect.asymmetric(
      TransitionEffect.translate({ y: MATH_KB_HEIGHT }),
      TransitionEffect.translate({ y: MATH_KB_HEIGHT })
    ).animation({ duration: 220, curve: Curve.EaseOut }))
  }

  private mathKbOverlayUp(): boolean {
    return this.mathKbVisible && !this.useSysKeyboard
      && this.toolAskKind === ''
      && (this.inputFocused || this.sheetKbFocus || this.tableKbFocus
        || this.slideKbFocus);
  }

  // 输入行容器几何回报（AlgebraPanel 钉底输入行，宽窄两分支同一容器）：
  // 随上报随缓存，可见时立即反解
  private onInputArea(n: Area): void {
    this.lastInputArea = n;
    this.applyKbSpacer();
  }

  // 元素底边距页面底（vp）：globalPosition 坐标字段官方未标单位（社区定论
  // px、相对窗口坐标系）——用「距根顶的差值 + 双口径解释」自校准：raw 为
  // px 时 vp 口径解释必越出页高（×k），raw 为 vp 时两种解释都合法且 vp 解
  // 释恒更小，规则在两种世界都选中真值（十七~十九包口径，输入行与滑动条
  // 块底反解共用）
  private areaBottomVp(a: Area): number {
    const k = canvasDensity();
    const dRaw = (a.globalPosition.y as number) - this.rootY;
    const hVp = a.height as number;
    const bVp = this.pageH - dRaw - hVp;
    const bPx = this.pageH - dRaw / k - hVp;
    const inPage = bVp >= 0 && bVp <= this.pageH;
    return inPage && bVp <= bPx ? bVp : Math.max(0, bPx);
  }

  // bottom = 输入行底边距页面底（vp，含当前垫高）；因底边随垫高同步平移，
  // s = target − bottom + 当前垫高 与当前状态无关、一步收敛。键盘上沿在
  // 窗口口径 = 键盘高 220；折算到页面口径（页面底 = 窗口底 − 导航条高）
  // 即 220 − 导航条高，目标再加 4 呼吸隙。
  private applyKbSpacer(): void {
    // 三十包①+三十四包②：滑动条行内编辑**会话期**整段让位（两键盘形态
    // 一致），垫高归滑动条管线独占。只让位「聚焦态」不够：⌨ 切换后
    // slideKbFocus=false 而此处的反解照跑，垫高动画每动一拍、面板顶部输
    // 入行 onAreaChange 就回连这里——钉底输入行在面板顶部、距页底恒大于
    // 键盘目标线，反解恒出 0 且**无动画瞬写**，与滑动条管线的动画赋值互
    // 相改写 = 切键盘「从下往上弹又瞬间回去」无限循环（真机②根因）
    if (this.slideEditLabel.length > 0) {
      return;
    }
    const a = this.lastInputArea;
    if (!this.mathKbVisible || a === null || this.pageH <= 0) {
      return;
    }
    const navGap = AppStorage.get<number>('ui.navBottomVp') ?? 0;
    const s = Math.max(0,
      Math.round(MATH_KB_HEIGHT - navGap + 4 - this.areaBottomVp(a) + this.kbSpacer));
    if (Math.abs(s - this.kbSpacer) > 1) {
      this.kbSpacer = s;
      AppStorage.setOrCreate('ui.kbSpacer', s);
    }
  }

  // 三十二包②：行内编辑块的显式重锚定入口——滚动期间的面积回调已撤（反
  // 解与用户滚动打架），块自身增高（就地报错行）等已知变化由此显式复查
  private reanchorSlideBlock(): void {
    if (this.slideEditLabel.length > 0) {
      this.scheduleSlideAvoid(150);
    }
  }

  // 三十一包①b：OFFSET 抬升参考基准——键盘未弹（系统态）/自带键盘态时
  // 页面必然未被系统抬升，此刻根底窗口 y 即未抬位置；系统键盘弹起期间
  // 不刷新（保留弹起前基准）
  private snapshotSlideRoot(): void {
    if (this.slideEditField < 0) {
      return;
    }
    try {
      const r = this.getUIContext().getComponentUtils()
        .getRectangleById('geoRoot');
      this.slideRootBottomPx = r.windowOffset.y + r.size.height;
    } catch (_e) {
    }
  }

  // 三十三包②：滑动条垫高统一动画赋值——首聚焦/换字段收敛/切键盘换挡/
  // 收起的起落从「瞬跳」改「滑变」；|Δ|≤1 视为收敛不动。动画期间挂起实
  // 测复查（slidePadAnimating），落定后 onFinish 复查一次收敛
  private setSlideSpacer(v: number): void {
    if (Math.abs(v - this.kbSpacer) <= 1) {
      return;
    }
    this.slidePadAnimating = true;
    this.getUIContext().animateTo({
      duration: 200,
      curve: Curve.EaseOut,
      onFinish: (): void => {
        this.slidePadAnimating = false;
        this.scheduleSlideAvoid(30);
      }
    }, () => {
      this.kbSpacer = v;
    });
    AppStorage.setOrCreate('ui.kbSpacer', v);
  }

  // 三十一包①a/①b：滑动条行内编辑统一避让（自带/系统键盘共用）——块底
  //（取消/确定行为块内最底元素）贴键盘上沿+4。窗口矩形差值对整页平移不
  // 敏感，但键盘锚窗口底、OFFSET 整页抬升两者都看不见 → 抬升量单独从根
  // 的绝对位置反解（未抬基准 − 当前根底）：手动点按路径 OFFSET 已抬够，
  // 差额 ≤ 0 不垫零回归；⌨ 切换的程序化 showTextInput 不触发 OFFSET（真
  // 机字段被盖根因），抬升 0，实测自垫补位。垫高 Row 在抽屉/面板之下，
  // 垫高只平移整页内容、块相对抽屉不动 → 与滚动对齐各定一个自由度，互
  // 不干扰；收敛式含当前垫高（块底随垫同步平移），一步收敛。
  // 三十四包①：返回实测是否落定——true=垫高已归本管线（含判定无需垫/
  // 动画在飞 onFinish 复查），false=实测不可用（静止闸门出口
  // gatedSlideAvoid 据此走整垫兜底）
  private applySlideAvoid(): boolean {
    if (this.slideEditField < 0 || this.inputFocused
      || this.sheetKbFocus || this.tableKbFocus) {
      return false;
    }
    if (this.slidePadAnimating) {
      return true;   // 三十三包②：垫高动画在飞，实测是中间值，落定后 onFinish 复查
    }
    const imeVp = AppStorage.get<number>('ui.kbH') ?? 0;
    const kbTop = this.useSysKeyboard ? imeVp : MATH_KB_HEIGHT;
    if (kbTop <= 0) {
      // 系统键盘已落：未抬基准随落随新；清系统态自垫（垫高 Row 条件含
      // kbSpacer>0，不清会残留空垫）
      this.snapshotSlideRoot();
      this.setSlideSpacer(0);
      return true;
    }
    if (!this.useSysKeyboard && !this.mathKbVisible) {
      return false;   // 自带键盘浮层未挂载：syncKbSpacer 翻转后再来
    }
    try {
      const k = canvasDensity();
      const r = this.getUIContext().getComponentUtils()
        .getRectangleById('geoRoot');
      const rootBottomPx = r.windowOffset.y + r.size.height;
      if (!this.useSysKeyboard) {
        this.slideRootBottomPx = rootBottomPx;   // 自带态无系统抬升，随测随新
      }
      const liftVp = this.slideRootBottomPx > 0
        ? Math.max(0, Math.round((this.slideRootBottomPx - rootBottomPx) / k))
        : 0;
      const b = this.getUIContext().getComponentUtils()
        .getRectangleById('slideEditBlock');
      const blockBottomVp = (rootBottomPx - b.windowOffset.y - b.size.height) / k
        + (AppStorage.get<number>('ui.navBottomVp') ?? 0);
      // s = 目标(键盘上沿+4) − 块底(含当前垫高) + 当前垫高，一步收敛
      const s = Math.max(0,
        Math.round(kbTop + 4 - liftVp - blockBottomVp + this.kbSpacer));
      this.setSlideSpacer(s);   // 三十三包②：动画滑变；落定后 onFinish 复查收敛
      return true;
    } catch (_e) {
      return false;   // 块重排中取矩形失败：onAreaChange/定时复查兜底
    }
  }

  // 三十一包①a：列表滚动对齐——块在视口下方的部分被 List 裁剪，垫高永
  // 远不可见（模拟器实测字段全压键盘下）。滚到「块底=视口底−4」即四字段
  // +按钮全露出；向下限幅保证聚焦字段不滚出视口顶。列表余量（块为末项
  // 时的下方空白）滚不掉 → 由垫高收敛补（垫至块底贴键盘上沿，抽屉空尾
  // 滑入键盘底下），两自由度合流后间隙恒 4vp。
  // 三十五包①：返回是否发生了滚动（true=滚动落地后闸门链复查；false=
  // 无需滚/滚不动，调用方当场解算垫高残差——先滚后垫次序的另一半）
  private alignSlideBlock(): boolean {
    if (this.slideEditField < 0 || this.inputFocused
      || this.sheetKbFocus || this.tableKbFocus) {
      return false;
    }
    if (this.slidePadAnimating) {
      return true;   // 三十三包②：垫高动画在飞，列表矩形在途中，落定后复查
    }
    const kbUp = this.useSysKeyboard
      ? (AppStorage.get<number>('ui.kbH') ?? 0) > 0 : this.mathKbVisible;
    if (!kbUp) {
      return false;
    }
    try {
      const k = canvasDensity();
      const b = this.getUIContext().getComponentUtils()
        .getRectangleById('slideEditBlock');
      const l = this.getUIContext().getComponentUtils()
        .getRectangleById('algebraList');
      // 滚动量 = 块底低于视口底的差额 + 4（贴视口底留呼吸隙）。只向下
      // 滚：块底高于视口底（列表余量/过冲）滚不掉也无需滚，由垫高收敛
      // 补（垫至块底贴键盘上沿，抽屉空尾滑入键盘底下）
      let dy = Math.round((b.windowOffset.y + b.size.height
        - l.windowOffset.y - l.size.height) / k) + 4;
      if (dy <= 3 || dy > 600) {
        this.slideAlignLastDy = 0;
        return false;
      }
      // 向下滚限幅：聚焦字段顶不越出视口顶（滚过头字段反而不可见）
      const f = this.getUIContext().getComponentUtils()
        .getRectangleById('slideEditF' + this.slideEditField);
      const maxDy = Math.round((f.windowOffset.y - l.windowOffset.y) / k);
      if (maxDy < dy) {
        dy = Math.max(0, maxDy);
      }
      if (dy <= 0 || Math.abs(dy - this.slideAlignLastDy) <= 1) {
        this.slideAlignLastDy = 0;
        return false;   // 无可滚/收敛/卡住（滚动未生效）：停止排程
      }
      this.slideAlignLastDy = dy;
      this.algebraScroller.scrollBy(0, dy);
      this.scheduleSlideAvoid(200);
      return true;
    } catch (_e) {
      // 块重排中取矩形失败：onAreaChange/定时复查兜底
      return false;
    }
  }

  // 三十一包①a/①b：避让+滚动对齐复查去抖——键盘弹起/垫高落地/OFFSET
  // 随动都连发，去抖以最后一次为准。三十五包①：回调改走静止闸门
  private scheduleSlideAvoid(delay: number): void {
    if (this.slideAvoidTimer >= 0) {
      clearTimeout(this.slideAvoidTimer);
    }
    this.slideAvoidTimer = setTimeout(() => {
      this.slideAvoidTimer = -1;
      this.gatedSlideAvoid();
    }, delay);
  }

  // 三十五包①：静止闸门解算——块底窗口矩形连续两拍（40ms）一致才实测
  // 解算+滚动对齐。块刚展开/列表滚动对齐/OFFSET 抬升在途时矩形是中间值，
  // 直接实测必得偏大垫高（块底偏低、抬升未满），落定后复查再回缩 =「先
  // 向上一大块再缩回来」两段跳（真机三十四包①，首聚焦与系统弹起同病）。
  // 垫高动画在飞直接退出（onFinish 复查，不空转闸门）；长滚动 ~0.8s 不静
  // 止按现值兜底解算
  private gatedSlideAvoid(): void {
    if (this.slideStableTimer >= 0) {
      clearTimeout(this.slideStableTimer);
      this.slideStableTimer = -1;
    }
    if (this.slidePadAnimating) {
      this.slideStableSig = -1;
      this.slideStableTry = 0;
      return;
    }
    let sig = -1;
    try {
      const b = this.getUIContext().getComponentUtils()
        .getRectangleById('slideEditBlock');
      sig = b.windowOffset.y + b.size.height;
    } catch (_e) {
      // 块重排/卸载中：按不可测处理，走重试→兜底
    }
    if (sig > 0 && sig === this.slideStableSig) {
      this.slideStableSig = -1;
      this.slideStableTry = 0;
      // 三十五包①真根因：先滚后垫。旧序先解垫高——getRectangleById 取
      // 块的「布局矩形」（未裁剪），首聚焦时块底常在视口折叠线以下，反解
      // 按「整块露出键盘」算出大幅过冲（模拟器实测 235vp vs 终值 172），
      // 滚动对齐落地后复查再大幅回缩 =「先向上避让一大块再缩回来」（真
      // 机①，两键盘同病、确定性复现；三十四包只验了动画次数与终态没验
      // 幅度，误判为时序竞态）。改序：滚动自由度先行把块完整露出视口，
      // 垫高只补残差，一步到位不再回缩
      if (!this.alignSlideBlock()) {
        const ok = this.applySlideAvoid();
        if (!ok && this.kbSpacer <= 0 && this.slideKbFocus
          && !this.useSysKeyboard && !this.inputFocused
          && !this.sheetKbFocus && !this.tableKbFocus) {
          // 实测不可用兜底（首聚焦场景）：按浮层键盘高整垫，宁多勿遮
          const navGap0 = AppStorage.get<number>('ui.navBottomVp') ?? 0;
          this.setSlideSpacer(Math.max(0,
            Math.round(MATH_KB_HEIGHT - navGap0 + 4)));
        }
      }
      return;
    }
    this.slideStableSig = sig;
    this.slideStableTry++;
    if (this.slideStableTry > 20) {
      this.slideStableSig = -1;
      this.slideStableTry = 0;
      if (!this.alignSlideBlock()) {
        this.applySlideAvoid();
      }
      return;
    }
    this.slideStableTimer = setTimeout(() => this.gatedSlideAvoid(), 40);
  }

  // 统一插入落位（第十包②）：写串（替换 [at,end) 区间）+ 四同步
  //（selStart/selEnd/mathCaret/mathCaretScript）。forceScript=true 时框内
  // 态显式给定（模板键），否则按新串/新光标重推（脚本边界歧义位置维持
  // 原态）
  private applyInsert(s: string, at: number, end: number, text: string,
    caret: number, forceScript: boolean): void {
    this.pendingInput = s.slice(0, at) + text + s.slice(end);
    this.selStart = caret;
    this.selEnd = caret;
    this.mathCaret = caret;
    this.mathCaretScript = forceScript
      || this.deriveCaretScript(this.pendingInput, caret, this.mathCaretScript);
    this.activeCtrl().caretPosition(caret);
  }

  // 插入/删除后重推光标内外态（上游 currentField 的线性等价）：严格处于
  // 脚本内容内 / 紧跟 ^/_ / 在未闭合 ')' 结构内 → 框内；恰在脚本边界
  // （框首/框尾）→ 维持原态；其余 → 框外
  private deriveCaretScript(s: string, caret: number, prev: boolean): boolean {
    if (caret > 0 && (s.charAt(caret - 1) === '^' || s.charAt(caret - 1) === '_')) {
      return true;
    }
    if (caret < s.length && s.charAt(caret) === ')') {
      return true;
    }
    const atoms = parseMathAtoms(s);
    let hostIdx = -1;
    for (let j = 0; j < atoms.length; j++) {
      if (atoms[j].end > caret) {
        hostIdx = j;
        break;
      }
    }
    if (hostIdx >= 0) {
      const h = atoms[hostIdx];
      if (h.sup || h.sub) {
        let rs = h.start;
        for (let j = hostIdx - 1; j >= 0 && (atoms[j].sup || atoms[j].sub); j--) {
          rs = atoms[j].start;
        }
        return caret > rs ? true : prev;
      }
      if (hostIdx > 0 && (atoms[hostIdx - 1].sup || atoms[hostIdx - 1].sub)
        && atoms[hostIdx - 1].end === caret) {
        return prev;
      }
      return false;
    }
    if (atoms.length > 0) {
      const last = atoms[atoms.length - 1];
      if (last.end === caret && (last.sup || last.sub)) {
        return prev;
      }
    }
    return false;
  }

  // 在光标处插入文本（有选区则替换选区）。back：插入后光标从串尾回退的
  // 字符数（上游 模板键：sqrt() 光标落参数框内）；back=-1 哨兵：□_□ 智能
  // 落位（第九包①）——光标前紧接标识符尾 → 0（光标落下标位 x_|，框内），
  // 否则 1（空 base 模板，光标落 base 框 _ 前一位，框外）；skip：光标处
  // 已存在该串时仅右移光标（）键在自动补齐的右括号前跳过，上游 同款）
  private insertAtCaret(t: string, back: number = 0, skip: string = ''): void {
    const s = this.pendingInput;
    let a = this.selStart;
    let b = this.selEnd;
    if (a < 0 || b < 0 || a > s.length || b > s.length) {
      a = s.length;
      b = s.length;
    }
    if (skip.length > 0 && a === b && a + skip.length <= s.length
      && s.substring(a, a + skip.length) === skip) {
      this.selStart = a + skip.length;
      this.selEnd = a + skip.length;
      this.mathCaret = this.selStart;
      this.mathCaretScript = this.deriveCaretScript(s, this.selStart,
        this.mathCaretScript);
      this.activeCtrl().caretPosition(this.selStart);
      return;
    }
    // 上游 newScript 同款（第十包②）：光标紧邻同型 ^/_ 结构时只进框、
    // 不再嵌套——框前 → 进框首，框尾 → 进框尾；框外紧贴同型结构尾 →
    // 进框尾（上游 newScript 向后游走，第十一包⑤）
    if ((t === '^' || t === '_') && a === b) {
      if (a < s.length && s.charAt(a) === t) {
        this.applyInsert(s, a, a, '', scriptContentRange(s, a)[0], true);
        return;
      }
      if (a > 0 && s.charAt(a - 1) === t) {
        this.applyInsert(s, a, a, '', scriptStructureEnd(s, a - 1), true);
        return;
      }
      if (!this.mathCaretScript) {
        const reOp = scriptRunEndAtCaret(s, a);
        if (reOp >= 0 && s.charAt(reOp) === t) {
          this.applyInsert(s, a, a, '', scriptContentRange(s, reOp)[1], true);
          return;
        }
      }
    }
    let effBack = back;
    if (back === -1 && t.length === 1) {
      effBack = hasIdentBase(s, a) ? 0 : 1;
    }
    if (a === b) {
      // 第十包②：框内/框外双态键入（上游 框内 offset ↔ 主序列 offset）
      const atOp = a < s.length && (s.charAt(a) === '^' || s.charAt(a) === '_');
      let insAt = a;
      let insText = t;
      if (atOp && this.mathCaretScript) {
        // 框内框首：键入落脚本内容首（上游 进框后 offset 0）
        insAt = s.charAt(a + 1) === '(' ? scriptContentRange(s, a)[0] : a + 1;
      } else if (!atOp && !this.mathCaretScript && t.length > 0
        && isTypewriterCh(t.charAt(0))) {
        // 首字符为字母/数字/下划线/希腊（含 sqrt()、10^ 等多字符模板键）
        // 紧贴裸脚本 run 尾插入会并进脚本内容 → 包裹；运算符/括号不粘连
        // 走普通插入
        const runOp = scriptRunEndAtCaret(s, a);
        if (runOp >= 0) {
          const force2 = t.length > 0
            && (t.charAt(0) === '^' || t.charAt(0) === '_') && effBack !== 1;
          if (s.charAt(runOp + 1) === '(') {
            // 已是 ^(…) 包裹形式：结构尾即兄弟位，直接插入（上游 主序列
            // 插兄弟节点，显示 x²c 无隔断——第十一包④去 "*"）
            this.applyInsert(s, a, a, t,
              Math.max(a, a + t.length - effBack), force2);
            return;
          }
          // 裸 ^2 形式：按 上游 序列化器把内容包成 ^(2) 再插兄弟字符
          //（线性串里裸数字/字母会并进脚本内容；包裹后兄弟字符插在 ')'
          // 之后、显示 x²c，与 上游 一致，第十一包④去 "*"。第十二包③修
          // 正插入点：包裹串里内容整体右移一位、')' 在 rng[1]+1，兄弟位
          // = rng[1]+2——旧值 rng[1]+1 恰是 ')' 的下标，字符插到 ')' 之
          // 前落进了脚本内容（框_框 框外键入进小框的根因））
          const rng = scriptContentRange(s, runOp);
          const wrapped = s.slice(0, rng[0]) + '(' + s.slice(rng[0], rng[1])
            + ')' + s.slice(rng[1]);
          const insAt2 = rng[1] + 2;
          this.applyInsert(wrapped, insAt2, insAt2, t,
            Math.max(a, insAt2 + t.length - effBack), force2);
          return;
        }
      }
      const force = t.length > 0 && (t.charAt(0) === '^' || t.charAt(0) === '_')
        && effBack !== 1;
      const caret = Math.max(a, insAt + insText.length - effBack);
      this.applyInsert(s, insAt, insAt, insText, caret, force);
      return;
    }
    const caret = Math.max(a, a + t.length - effBack);
    this.applyInsert(s, a, b, t, caret, false);
  }

  // 退格（第十一包⑤，上游 bkspCharacter + RemoveContainer 语义对齐）：
  // - 框内态：内容首（紧贴 ^/_ 或其 '(' 组）→ 拆结构壳、内容原位拼回
  //   主序列（x^2→x2、空内容 x^→x；上游 脚本节点空壳、base 留父序列）；
  //   紧跟函数模板 ( 前 → 整拆模板（第九包语义）；其余删框内前一字符
  // - 框外态：紧跟脚本结构尾 → 有内容删内容末字符并进框、空框仅进框
  //  （上游 bkspLastFunctionArg，第十一包⑤空框不再整删公式）；紧跟函数
  //   模板 ')' → 有参删参数末字符进框、空参整拆；其余删前一字符
  private backspaceAtCaret(): void {
    const s = this.pendingInput;
    let a = this.selStart;
    let b = this.selEnd;
    if (a < 0 || b < 0 || a > s.length || b > s.length) {
      a = s.length;
      b = s.length;
    }
    if (a !== b) {
      this.pendingInput = s.slice(0, a) + s.slice(b);
      this.selStart = a;
      this.selEnd = a;
      this.mathCaret = a;
      this.mathCaretScript = this.deriveCaretScript(this.pendingInput, a,
        this.mathCaretScript);
      this.activeCtrl().caretPosition(a);
      return;
    }
    if (a === 0) {
      return;
    }
    if (this.mathCaretScript) {
      // 框内 ⌫（第十一包⑤，上游 RemoveContainer.withBackspace →
      // deleteContainer 语义，脚本节点为空壳、base 留父序列）：内容首
      //（紧贴 ^/_ 或其 '(' 组首）→ 拆结构壳、内容原位拼回主序列
      //（x^2→x2、x^(23)→x23、空内容 x^→x；拼回内容与前一裸脚本 run 粘连
      // 时先包裹前 run，x^2_3 拆 _3 → x^(2)3）；其余删框内前一字符
      let op = -1;
      if (a < s.length && (s.charAt(a) === '^' || s.charAt(a) === '_')) {
        op = a;
      } else if (s.charAt(a - 1) === '^' || s.charAt(a - 1) === '_') {
        op = a - 1;
      } else if (a >= 2 && s.charAt(a - 1) === '('
        && (s.charAt(a - 2) === '^' || s.charAt(a - 2) === '_')) {
        op = a - 2;
      }
      if (op >= 0) {
        const rng = scriptContentRange(s, op);
        const content = s.substring(rng[0], rng[1]);
        let head = s.slice(0, op);
        const prevRun = content.length > 0 ? scriptRunEndAtCaret(s, op) : -1;
        if (prevRun >= 0 && s.charAt(prevRun + 1) !== '(') {
          const prng = scriptContentRange(s, prevRun);
          head = s.slice(0, prng[0]) + '(' + s.slice(prng[0], prng[1]) + ')'
            + s.slice(prng[1], op);
        }
        const end = scriptStructureEnd(s, op);
        this.pendingInput = head + content + s.slice(end);
        // 光标落 head 尾（上游：容器旧主序列下标 = 拼回内容之前；包裹
        // 前 run 时 head 变长，须用 head.length 而非旧 op）
        this.selStart = head.length;
        this.selEnd = head.length;
        this.mathCaret = head.length;
        this.mathCaretScript = false;
        this.activeCtrl().caretPosition(head.length);
        return;
      }
    }
    if (!this.mathCaretScript) {
      const runOp = scriptRunEndAtCaret(s, a);
      if (runOp >= 0) {
        const rng = scriptContentRange(s, runOp);
        if (rng[1] > rng[0]) {
          // 框外紧跟脚本结构尾：删内容末字符、光标进框（上游
          // bkspLastFunctionArg）
          const delAt = rng[1] - 1;
          this.pendingInput = s.slice(0, delAt) + s.slice(delAt + 1);
          this.selStart = delAt;
          this.selEnd = delAt;
          this.mathCaret = delAt;
          this.mathCaretScript = this.deriveCaretScript(this.pendingInput,
            delAt, true);
          this.activeCtrl().caretPosition(delAt);
        } else {
          // 空脚本框：仅进框不删（上游 空参 bkspLastFunctionArg 同款——
          // 第十一包⑤：空模板 ⌫ 不再整删公式）
          this.selStart = rng[0];
          this.selEnd = rng[0];
          this.mathCaret = rng[0];
          this.mathCaretScript = true;
          this.activeCtrl().caretPosition(rng[0]);
        }
        return;
      }
    }
    if (!this.mathCaretScript && s.charAt(a - 1) === ')') {
      // 函数模板 ')' 前：有参删参数末字符进框；空参整拆模板
      const open = matchParenBackward(s, a - 1);
      const nameStart = open >= 0 ? functionOpenBefore(s, open) : -1;
      if (nameStart >= 0) {
        if (open < a - 1) {
          this.pendingInput = s.slice(0, a - 2) + s.slice(a - 1);
          this.selStart = a - 2;
          this.selEnd = a - 2;
          this.mathCaret = a - 2;
          this.mathCaretScript = this.deriveCaretScript(this.pendingInput,
            a - 2, true);
        } else {
          this.pendingInput = s.slice(0, nameStart) + s.slice(a);
          this.selStart = nameStart;
          this.selEnd = nameStart;
          this.mathCaret = nameStart;
          this.mathCaretScript = false;
        }
        this.activeCtrl().caretPosition(this.mathCaret);
        return;
      }
    }
    // 其余：紧贴函数模板 ( 前整拆（第九包），或普通删前一字符
    const uw = templateUnwrapRange(s, a);
    if (uw.length === 2) {
      this.pendingInput = s.slice(0, uw[0]) + s.slice(uw[1]);
      a = uw[0];
      this.mathCaretScript = false;
    } else {
      this.pendingInput = s.slice(0, a - 1) + s.slice(a);
      a -= 1;
      this.mathCaretScript = this.deriveCaretScript(this.pendingInput, a,
        this.mathCaretScript);
    }
    this.selStart = a;
    this.selEnd = a;
    this.mathCaret = a;
    this.activeCtrl().caretPosition(a);
  }

  // 光标移动（数学键盘 ←/→，第十包② 上游 CursorController 线性映射）：
  // 沿 mathCaretStops 停靠序列移动——^/_ 结构的框前/框内是两个停靠点
  //（→ 进框首、框尾出框；← 镜像回退），base 尾与框内都可编辑；被吞语法
  // 字符区间内不再停靠。到达端点原地不动（上游 next/prev 返回 false 同款）
  private caretMove(dir: number): void {
    const s = this.pendingInput;
    const a = this.selStart < 0 ? s.length : this.selStart;
    const b = this.selEnd < 0 ? s.length : this.selEnd;
    const from = Math.min(a, b);
    const stops = mathCaretStops(s);
    let idx = -1;
    for (let i = 0; i < stops.length; i++) {
      if (stops[i].pos === from && stops[i].script === this.mathCaretScript) {
        idx = i;
        break;
      }
    }
    let stop: MathCaretStop | undefined = undefined;
    if (idx >= 0) {
      const ni = idx + dir;
      // 第十六包②：空框内停靠恢复为普通站——第十二包③的「← 跳过空框」
      // 实测把空框变成进不去的单向门（→ 能进、← 从结构尾直接跳过），真机
      // 反馈角标框 ←/→ 均进不去。上游 空框两个方向都可进；框内 ← 出站的
      // 下一站即框外框首/大框（空 base 无框首站时直接落大框内，第十二包
      // ③「一次到位大框」语义由站序天然保证，无需跳站）
      if (ni >= 0 && ni < stops.length) {
        stop = stops[ni];
      }
    } else if (dir > 0) {
      for (let i = 0; i < stops.length; i++) {
        if (stops[i].pos > from) {
          stop = stops[i];
          break;
        }
      }
    } else {
      for (let i = stops.length - 1; i >= 0; i--) {
        if (stops[i].pos < from) {
          stop = stops[i];
          break;
        }
      }
    }
    if (stop === undefined) {
      return;
    }
    this.selStart = stop.pos;
    this.selEnd = stop.pos;
    this.mathCaret = stop.pos;
    this.mathCaretScript = stop.script;
    this.activeCtrl().caretPosition(stop.pos);
  }

  // ---- 代数区行交互（AlgebraPanel 经回调上达） ----

  // 行内显隐：切换显隐（隐藏对象重新显示的入口）
  private toggleRowVisible(label: string, show: boolean): void {
    this.commitPending();
    const r = dreamemberKernel.kernelSetVisible(label, show);
    this.syncAlgebra(r);
    this.refresh();
  }

  // 点代数区整行 ⇄ 画布选中特效（再点一次取消）；行高亮由内核 sel 列回传。
  // 三十二包③：点其他行 = 点别处，滑动条行内编辑区一并收起（ArkUI 点击
  // 子组件优先消费，行根 onClick 吃掉事件、列表层点按收不到，此处兜底）
  private toggleRowSelect(label: string): void {
    this.commitPending();
    if (this.slideEditLabel.length > 0) {
      this.closeSlideEdit(false);
    }
    const next = this.selectedLabel === label ? '' : label;
    this.canvasSelect(next);
  }

  // 行内改名提交：冲突/非法名弹警告且不修改
  private applyRename(label: string, next: string): void {
    const r = dreamemberKernel.kernelRename(label, next);
    if (r.startsWith('error:')) {
      this.alertBox('重命名失败', r.substring(6));
      return;
    }
    if (this.selectedLabel === label) {
      this.selectedLabel = next;
    }
    if (this.editingLabel === label) {
      this.editingLabel = next;
    }
    this.syncAlgebra(r);
    this.refresh();
  }

  // 行内改名请求：退出修改模式并把该行滚动到列表顶部（配合键盘弹出时
  // 代数区被压缩，行在最上方即不被系统键盘遮挡）。三十二包③：点其他行
  // 改名 = 点别处，滑动条行内编辑区一并收起（子组件消费点击，列表层
  // 收不到，此处兜底）
  private renameRequest(label: string): void {
    this.commitPending();
    if (this.slideEditLabel.length > 0) {
      this.closeSlideEdit(false);
    }
    this.editingLabel = '';
    this.creating = false;
    this.inputError = '';
    const idx = this.algebra.findIndex((a: AlgebraEntry): boolean => a.label === label);
    if (idx >= 0) {
      this.algebraScroller.scrollToIndex(idx, true, ScrollAlign.START);
    }
  }

  // 点行内数值/定义：该行就地变为编辑框（重定义，批次内联编辑）。
  // 已有别的编辑行先按隐式 ✓ 提交；同行的定义取提交后的最新值。
  // 三十二包③：点其他行编辑 = 点别处，滑动条行内编辑区一并收起
  private editDefinition(label: string, definition: string): void {
    this.commitPending();
    if (this.slideEditLabel.length > 0) {
      this.closeSlideEdit(false);
    }
    const cur = this.algebra.find((a: AlgebraEntry): boolean => a.label === label);
    const def = cur ? cur.definition : definition;
    this.creating = false;
    this.editingLabel = label;
    this.pendingInput = def;
    this.mathCaret = def.length;
    this.inputError = '';
    const idx = this.algebra.findIndex((a: AlgebraEntry): boolean => a.label === label);
    if (idx >= 0) {
      this.algebraScroller.scrollToIndex(idx, true, ScrollAlign.START);
    }
    this.requestFocusLater('algebraDefEdit');
  }

  // "＋ 新建对象"：列表末尾展开大输入行（原底部输入条移除）。三十二包③：
  // 点新建 = 点别处，滑动条行内编辑区一并收起（按钮在列表外，列表层
  // 点按收不到，此处兜底）
  private startCreate(): void {
    this.commitPending();
    if (this.slideEditLabel.length > 0) {
      this.closeSlideEdit(false);
    }
    this.editingLabel = '';
    this.creating = true;
    this.pendingInput = '';
    this.mathCaret = 0;
    this.inputError = '';
    this.panelTab = 'algebra';
    this.setPanelVisible(true);
    this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    this.algebraScroller.scrollEdge(Edge.Bottom);
    this.requestFocusLater('algebraCreateInput');
  }

  // 动态挂载的输入框请求焦点（defaultFocus 对动态插入组件不生效，真机已验）：
  // 80ms 未中再补一次
  private requestFocusLater(id: string): void {
    setTimeout(() => {
      try {
        this.getUIContext().getFocusController().requestFocus(id);
      } catch (e) {
        // 组件未挂载完成时忽略（320ms 补一次）
      }
    }, 80);
    setTimeout(() => {
      try {
        this.getUIContext().getFocusController().requestFocus(id);
      } catch (e) {
        // 两次都未中则由用户点一下输入框
      }
    }, 320);
  }

  // 提交当前输入行（回车 / ✓ / 自绘键盘确定）：修改模式走重定义，新建走创建。
  // 失败保留输入行与内容并显示错误（就地改到对为止）
  private submitActive(): void {
    const expr = this.pendingInput.trim();
    if (this.editingLabel.length > 0) {
      if (expr.length === 0) {
        // 空输入视为取消修改
        this.editingLabel = '';
        this.pendingInput = '';
        this.mathCaret = 0;
        this.inputError = '';
        return;
      }
      const r = dreamemberKernel.kernelRedefine(this.editingLabel, expr);
      if (r.startsWith('error:')) {
        this.inputError = r.substring(6);
        return;
      }
      this.editingLabel = '';
      this.pendingInput = '';
      this.mathCaret = 0;
      this.inputError = '';
      this.syncAlgebra(r);
      this.refresh();
      if (dreamemberKernel.kernelAnyAnimating() === '1') {
        this.startAnimTicker();
      }
      return;
    }
    if (!this.creating || expr.length === 0) {
      return;
    }
    const err = this.input(expr);
    if (err.length === 0) {
      this.creating = false;
      this.pendingInput = '';
      this.mathCaret = 0;
      this.inputError = '';
    } else {
      this.inputError = err;   // 输入行保留，直接改
    }
  }

  // 取消当前输入行（✕）
  private cancelActive(): void {
    this.creating = false;
    this.editingLabel = '';
    this.pendingInput = '';
    this.mathCaret = 0;
    this.inputError = '';
  }

  // 点别处 = 隐式 ✓（真机反馈：点其它位置时修改框应提交并收回）。
  // 画布按下与代数区其它行交互前调用；失败时输入行与错误保留（同 ✓ 语义）。
  // ⌨ 键盘切换、自绘键盘"完成"不经过这里（只换键盘不提交）。
  private commitPending(): void {
    if (this.editingLabel.length > 0) {
      this.submitActive();   // 空输入由 submitActive 按取消处理
    } else if (this.creating) {
      if (this.pendingInput.trim().length === 0) {
        this.cancelActive();   // 空的新建行直接收起
      } else {
        this.submitActive();
      }
    }
  }

  // 代数区展开/收回统一入口：animateTo 驱动布局过渡，配合宽屏面板/窄条、
  // 窄屏抽屉容器上的 transition 形成滑入滑出（十四轮）
  private setPanelVisible(next: boolean): void {
    this.getUIContext().animateTo({ duration: 260, curve: Curve.EaseInOut }, () => {
      this.showPanel = next;
    });
  }

  // ---- 第二十二包③⑥② ----

  // 窄屏抽屉高度：拖拽过用拖拽值，否则按内容档位（代数/CAS 40%、其余 32%）
  private drawerHeight(): Length {
    if (this.drawerH > 0) {
      return this.drawerH;
    }
    return this.panelTab === 'algebra' || this.panelTab === 'cas' ? '40%' : '32%';
  }

  // 当前档位的 vp 值（拖拽起手快照；pageH 未测得时退 400）
  private drawerDefaultH(): number {
    const frac = this.panelTab === 'algebra' || this.panelTab === 'cas' ? 0.40 : 0.32;
    return this.pageH > 0 ? this.pageH * frac : 400 * frac;
  }

  // 鼠标悬停光标（②）：进入分隔条/把手 ↔（横拖）/↕（竖拖），离开还原。
  // setPointerStyle 作用于整窗，进出成对即互不干扰；API 异常（部分平台/
  // 窗口态）静默退回系统箭头，不影响交互
  private setResizeCursor(hover: boolean, vertical: boolean): void {
    try {
      const style = !hover ? pointer.PointerStyle.DEFAULT
        : (vertical ? pointer.PointerStyle.RESIZE_UP_DOWN
          : pointer.PointerStyle.RESIZE_LEFT_RIGHT);
      pointer.setPointerStyleSync(this.curWinId, style);
    } catch (_e) {
      // 光标 API 不可用：保持默认箭头（功能不受影响）
    }
  }

  private ensureWinId(): void {
    if (this.curWinId >= 0) {
      return;
    }
    const ctx = this.getUIContext().getHostContext();
    if (ctx === undefined) {
      return;
    }
    window.getLastWindow(ctx).then((w: window.Window) => {
      this.curWinId = w.getWindowProperties().id;
      this.watchKbHeight(w);   // 二十七包②：系统键盘高度监听
    }).catch(() => {
      // 解析失败保持 -1（setPointerStyleSync 对 -1 抛错被上层吞掉）
    });
  }

  // 底部 Tab 切换（轮 B）：同一 Tab 再点收起；切换前按隐式 ✓ 提交输入行，
  // 非代数 Tab 同时收起键盘（工具/表格/数据不涉及输入会话）
  private openTab(tab: string): void {
    this.commitPending();
    this.closeSlideEdit();   // 二十九包②：换 Tab 收起滑动条行内编辑
    this.drawerH = 0;   // 二十二包③：换 Tab 回默认档位
    const next = !this.showPanel || this.panelTab !== tab;
    if (next && tab !== 'algebra') {
      this.dismissKeyboard();
    }
    this.getUIContext().animateTo({ duration: 260, curve: Curve.EaseInOut }, () => {
      this.showPanel = next;
      this.panelTab = tab;
    });
    if (next && tab === 'algebra') {
      this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    }
  }

  // 行内滑动条拖动：内核吸附/钳位 + 级联重算，画布刷新，行值节流同步。
  // 撤销打点：mode 0=拖动开始 3=点按滑轨（值未写入内核），整段拖动一步
  private onSliderChange(label: string, v: number, mode: number): void {
    if (mode === 0 || mode === 3) {
      dreamemberKernel.kernelPushUndo();
    }
    dreamemberKernel.kernelSetSlider(label, v);
    this.refresh();
    const now = Date.now();
    if (mode === 2 || mode === 3 || now - this.lastAlgebraMs > 80) {
      this.lastAlgebraMs = now;
      this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    }
  }

  // 行内透明度滑杆（不等式/多边形/角扇形，仅渲染态）：滑杆是"透明度"
  // 0-100（100=消失）→ 内核不透明度 = 1 − v/100，节流行同步。
  // 轮 A：改走通用属性 API kernelSetProp（'fill'，0-1 不透明度）。
  // 撤销打点同滑动条：mode 0/3 在值写入内核前压快照
  private onAlphaChange(label: string, v: number, mode: number): void {
    if (mode === 0 || mode === 3) {
      dreamemberKernel.kernelPushUndo();
    }
    dreamemberKernel.kernelSetProp(label, 'fill', String((100 - v) / 100));
    this.refresh();
    const now = Date.now();
    if (mode === 2 || mode === 3 || now - this.lastAlgebraMs > 80) {
      this.lastAlgebraMs = now;
      this.syncAlgebra(dreamemberKernel.kernelAlgebra());
    }
  }

  // ---- 轮 D 表格视图（A1 单元格模型，上游 Spreadsheet 语义） ----

  // 写单元格：content 空 = 删除该格对象；其余按 "<地址>=<内容>" 写内核
  //（内容已带同地址前缀或 '=' 时先剥掉）。已存在的对象走重定义（保标签）。
  // 返回错误信息（空串=成功）
  public setCell(label: string, content: string): string {
    this.commitPending();
    let expr = content.trim();
    if (expr.startsWith('=')) {
      expr = expr.substring(1).trim();
    } else if (expr.startsWith(label + '=')) {
      expr = expr.substring(label.length + 1).trim();
    }
    // 存在性判定：kernelGetProps 对不存在的对象返回 "-|"（NAPI 约定），
    // 按非空判定会把空格子误判为已存在 → 走重定义报"未找到对象 A1"
    //（真机反馈④：空格填数必须走新建而非重定义）
    const props = dreamemberKernel.kernelGetProps(label);
    const exists = props.length > 0 && props !== '-|';
    if (expr.length === 0) {
      if (exists) {
        this.deleteObj(label);
      }
      return '';
    }
    if (exists) {
      const r = dreamemberKernel.kernelRedefine(label, expr);
      if (r.startsWith('error:')) {
        return r.substring(6);
      }
      this.syncAlgebra(r);
      this.refresh();
      if (dreamemberKernel.kernelAnyAnimating() === '1') {
        this.startAnimTicker();
      }
      return '';
    }
    return this.input(label + '=' + expr);
  }

  // 表格单元格选中联动：有对象则画布/代数区选中特效，无对象清除选中
  private sheetSelect(label: string): void {
    if (label.length === 0) {
      if (this.selectedLabel.length > 0) {
        this.canvasSelect('');
      }
      return;
    }
    this.canvasSelect(label);
  }

  // ---- 布局片段 ----

  // 窄屏布局的错误条（宽屏由 AlgebraPanel showError 在面板内渲染）
  @Builder
  private errorText() {
    if (this.inputError.length > 0) {
      Text(this.inputError)
        .fontSize(12)
        .fontColor(Theme.error(this.dark))
        .width('100%')
        .padding({ left: 10, right: 10, bottom: 2 })
        .backgroundColor(Theme.panel(this.dark))
    }
  }

  // 代数面板 + 全部会话回调（宽屏左栏 / 窄屏抽屉共用；showError 决定
  // 错误条渲染位置）
  @Builder
  private algebraPanel(showError: boolean) {
    AlgebraPanel({
      algebra: $algebra,
      editingLabel: $editingLabel,
      creating: $creating,
      pendingInput: $pendingInput,
      inputError: $inputError,
      // 二十九包②：滑动条行内编辑四项状态 + 活跃字段（面板内展开渲染）
      slideEditLabel: $slideEditLabel,
      slideEditMin: $slideEditMin,
      slideEditMax: $slideEditMax,
      slideEditStep: $slideEditStep,
      slideEditVal: $slideEditVal,
      slideEditErr: $slideEditErr,
      onSlideField: (i: number): void => this.onSlideField(i),
      onSlideBlur: (i: number): void => this.onSlideBlur(i),
      // 三十二包③：列表空白点按收起行内编辑——时间窗护栏豁免开块/换字
      // 段/切键盘的即时点按（块内点按另有空 onClick 接管，见 AlgebraPanel）
      onListTap: (): void => {
        if (this.slideEditLabel.length > 0
          && Date.now() >= this.slideTapGuardUntil) {
          this.closeSlideEdit(false);
        }
      },
      // 三十二包③：块内任意点按刷新护栏——冒泡到列表层不收起
      onBlockTap: (): void => {
        this.slideTapGuardUntil = Date.now() + 400;
      },
      onSlideSel: (i: number, s: number, e: number): void => this.onSlideSel(i, s, e),
      onSlideSubmit: (): void => this.confirmSlideEdit(),
      onSlideCancel: (): void => this.closeSlideEdit(),
      onSlideSwitch: (): void => this.toggleSlideKeyboard(),
      dark: this.dark,
      useSysKeyboard: this.useSysKeyboard,
      isWide: this.isWide,
      showError: showError,
      scroller: this.algebraScroller,
      createCtrl: this.createCtrl,
      editCtrl: this.editCtrl,
      onToggleVis: (l: string, s: boolean): void => this.toggleRowVisible(l, s),
      onSelect: (l: string): void => this.toggleRowSelect(l),
      onRename: (l: string, n: string): void => this.applyRename(l, n),
      onRenameRequest: (l: string): void => this.renameRequest(l),
      onEdit: (l: string, d: string): void => this.editDefinition(l, d),
      onSliderEdit: (l: string): void => this.openSliderEdit(l),
      onToggleAnim: (l: string): void => this.toggleAnim(l),
      onDelete: (l: string): void => this.deleteObj(l),
      onSlider: (l: string, v: number, m: number): void => this.onSliderChange(l, v, m),
      onAlpha: (l: string, v: number, m: number): void => this.onAlphaChange(l, v, m),
      onSubmit: () => this.submitActive(),
      onCancel: () => this.cancelActive(),
      onCreate: () => this.startCreate(),
      onToggleKeyboard: () => this.toggleKeyboard(),
      onFocusInput: (f: boolean): void => {
        this.inputFocused = f;
        this.syncKbSpacer();
      },
      onSelChange: (s: number, e: number): void => {
        // 第十七包③④：自带键盘模式下 caretPosition() 的程序化设位会异步
        // 回声 onTextSelectionChange，此前无条件回写 mathCaretScript=false，
        // 把刚进小框的框内态洗掉——③光标跑小框外、④光标恒按大框渲染的
        // 共同根因。框内态只由 applyInsert/caretMove 内部维护，回声一概
        // 忽略；系统键盘模式无公式光标，回写仍照旧。
        if (!this.useSysKeyboard) {
          return;
        }
        this.selStart = s;
        this.selEnd = e;
        this.mathCaret = s;
        this.mathCaretScript = false;
      },
      onInputArea: (n: Area): void => this.onInputArea(n),
      mathCaret: this.mathCaret,
      mathCaretScript: this.mathCaretScript,
      fieldFocused: this.inputFocused,
      onFieldActivate: (): void => this.requestFocusLater(this.activeInputId()),
      onClose: () => {
        this.commitPending();
        this.closeSlideEdit();
        this.setPanelVisible(false);
      }
    })
  }

  // 画布交互层（宽/窄屏共用）。轮 C：顶部工具条删除；宽屏样式条显示在
  // 画布上方（上游 桌面样式条位置），窄屏显示在底栏上方（见 build 窄屏分支）；
  // 撤销/重做/菜单按钮在画布左上浮层（CanvasStage 渲染，2026-09-30 轮）
  @Builder
  private canvasArea() {
    Column() {
      if (this.isWide && this.selectedLabel.length > 0) {
        this.styleBarRow()
      }
      CanvasStage({
        dark: this.dark,
        tool: this.tool,
        toolHint: this.toolHint,
        vpW: $vpW,
        vpH: $vpH,
        ctx: this.ctx,
        onCanvasReady: (w: number, h: number, s: number): void => {
          this.canvasW = w;
          this.canvasH = h;
          this.uiScale = s;
          this.refresh();
        },
        onFrame: (f: string): void => this.execFrame(f),
        onSelectLabel: (l: string): void => this.canvasSelect(l),
        onDragMoveSync: (): void => this.onDragSync(false),
        onDragEndSync: (): void => this.onDragSync(true),
        onToolResult: (r: string): void => this.applyToolResult(r),
        onSetVisible: (l: string, s: boolean): void => this.toggleRowVisible(l, s),
        onDeleteObj: (l: string): void => this.deleteObj(l),
        onCommitPending: (): void => this.commitPending(),
        onDismissKeyboard: (): void => this.dismissKeyboard(),
        canUndo: this.canUndo,
        canRedo: this.canRedo,
        onMenuAct: (): void => this.onMenuRequest(),
        onUndoAct: (): void => this.doUndo(),
        onRedoAct: (): void => this.doRedo()
      })
    }
    // 画布区占满剩余高度（窄/宽屏通用）：内层 CanvasStage 的 layoutWeight
    // 依赖本 Column 有确定高度，缺了它画布会被压成 0 高（真机反馈）
    .width('100%')
    .layoutWeight(1)
  }

  // 轮 C 样式条：选中对象时出现（是否可样式化与能力自适应在 StyleBar 内部）。
  // label/tick 变化由 StyleBar @Watch 重取属性快照
  @Builder
  private styleBarRow() {
    StyleBar({
      dark: this.dark,
      label: this.selectedLabel,
      tick: this.styleTick,
      onSetProp: (l: string, k: string, v: string, m: boolean, f: boolean): void =>
        this.onStyleSetProp(l, k, v, m, f),
      onSetVisible: (l: string, s: boolean): void => this.onStyleVisible(l, s)
    })
  }

  // 底部面板（窄屏抽屉）：内容随 panelTab 切换，高度按内容档位。
  // 第十七包②：撤回第十六包①的 drawerHeight（键盘弹出改按压缩后页面取
  // 比例）——静态几何两轮都没对上（178 少一行、220 多两行），垫高整体改
  // 运行时实测（见 onInputArea），抽屉恢复固定档位，随垫高整块上移。
  // 第二十一包①：增加 wide 参数——宽屏左栏与窄屏抽屉共用同一份内容
  //（代数区 showError、面板 isWide 随形态切换），抽屉/停靠一处维护

  // 底部面板（窄屏抽屉/宽屏左栏）：内容随 panelTab 切换，高度按内容档位。
  // 轮 D：表格/数据为实面板（A1 单元格模型 / 单变量分析+直方图）
  @Builder
  private bottomPanel(wide: boolean) {
    if (this.panelTab === 'algebra') {
      this.algebraPanel(wide)
    } else if (this.panelTab === 'tools') {
      ToolsPanel({
        dark: this.dark,
        tool: this.tool,
        onSetTool: (id: string): void => this.setTool(id),
        onClose: (): void => this.setPanelVisible(false)
      })
    } else if (this.panelTab === 'sheet') {
      SheetPanel({
        algebra: $algebra,
        dark: this.dark,
        isWide: wide,
        useSysKeyboard: this.useSysKeyboard,
        onSetCell: (l: string, c: string): string => this.setCell(l, c),
        onSelectCell: (l: string): void => this.sheetSelect(l),
        onShowAnalysis: (v: string, r: string): void => {
          // 第二十三包②：单变量分析从数据区选区启动（快照选区 → analysis）
          this.analysisSource = v;
          this.analysisRange = r;
          this.openTab('analysis');
        },
        onToggleKeyboard: (): void => this.togglePanelKeyboard(),
        // 三十六包：格内编辑会话期注册面板键盘处理束（顶层键盘分发用）
        onKbHooks: (h: MathKbHooks | null): void => {
          this.sheetKbHooks = h;
        },
        onEditFocus: (f: boolean): void => {
          // 二十四包②：格内编辑占用自带键盘 → 实测垫高（代数区同源）
          this.sheetKbFocus = f;
          this.syncKbSpacer();
        },
        onEditArea: (n: Area): void => this.onInputArea(n),
        onClose: (): void => this.setPanelVisible(false)
      })
    } else if (this.panelTab === 'cas') {
      // 第五包：CAS 视图（☰ 菜单"视图"打开，不在底部四 Tab 内）
      CasPanel({
        dark: this.dark,
        onClose: (): void => this.setPanelVisible(false)
      })
    } else if (this.panelTab === 'analysis') {
      // 第二十三包②：数据分析虚拟 Tab（不在底栏 TABS 内，数据区 📊 打开；
      // 上游 经典路径：选区 → 单变量分析工具 → VIEW_DATA_ANALYSIS 停靠面板，
      // 关闭回到表格）
      DataAnalysisPanel({
        dark: this.dark,
        isWide: wide,
        prefill: this.analysisSource,
        sourceLabel: this.analysisRange,
        onBack: (): void => this.openTab('sheet'),
        onClose: (): void => this.setPanelVisible(false)
      })
    } else {
      // 'data'：表格（第二十三包①起为 上游 真·函数值表；此前为数据分析占位）
      TablePanel({
        algebra: this.algebra,
        dark: this.dark,
        isWide: wide,
        xMin: this.tableMin,
        xMax: this.tableMax,
        xStep: this.tableStep,
        onRangeChange: (mn: number, mx: number, st: number): void => {
          // 二十四包①：范围回写宿主——切 Tab 后仍保留
          this.tableMin = mn;
          this.tableMax = mx;
          this.tableStep = st;
        },
        // 二十五包②：范围编辑合入自带键盘 + ⌨ 切换 + 实测垫高（数据区同机制）
        useSysKeyboard: this.useSysKeyboard,
        onToggleKeyboard: (): void => this.togglePanelKeyboard(),
        // 三十六包：范围编辑会话期注册面板键盘处理束（顶层键盘分发用）
        onKbHooks: (h: MathKbHooks | null): void => {
          this.tableKbHooks = h;
        },
        onEditFocus: (f: boolean): void => {
          this.tableKbFocus = f;
          this.syncKbSpacer();
        },
        onEditArea: (n: Area): void => this.onInputArea(n),
        onClose: (): void => this.setPanelVisible(false)
      })
    }
  }

  // 底部 Tab 栏（轮 B，上游经典版 外壳）：代数区/工具/表格/数据
  @Builder
  private bottomBar() {
    BottomTabs({
      dark: this.dark,
      active: this.showPanel ? this.panelTab : '',
      onPick: (tab: string): void => this.openTab(tab)
    })
  }

  // 第二十一包①：宽屏左侧视图栏——上游 PC UI 同位（真机反馈：手机 UI 底栏
  // 四钮在 PC UI 里于左侧从上到下排布；此前宽屏只有代数区面板、四 Tab 无
  // 处安放）。图标定义复用 BottomTabs.TABS（宽窄同序同图）；点击语义与窄
  // 屏底栏一致：切换面板 / 再点同一 Tab 收起（openTab）
  @Builder
  private wideRail() {
    Column() {
      ForEach(TABS, (t: TabDef) => {
        Column() {
          Text(t.icon)
            .fontSize(18)
            .lineHeight(22)
            .height(24)
            .textAlign(TextAlign.Center)
            .translate({ x: 0, y: t.dy * 18.0 / 22.0 })
            // 二十六包①：字形垂直微调按字号比例——dy 以底栏 22vp 为基准，
            // 本栏 18vp 折算（模拟器实测：固定值在窄底栏/宽左栏不同字号下
            // 覆盖不住同一字形偏高）
            .fontColor(this.showPanel && this.panelTab === t.id
              ? Theme.accent(this.dark) : Theme.sub(this.dark))
          Text(t.name)
            .fontSize(10)
            .lineHeight(12)
            .height(14)
            .margin({ top: 3 })
            .textAlign(TextAlign.Center)
            .fontColor(this.showPanel && this.panelTab === t.id
              ? Theme.accent(this.dark) : Theme.sub(this.dark))
        }
        .width(56)
        .height(52)
        .alignItems(HorizontalAlign.Center)
        .justifyContent(FlexAlign.Center)
        .borderRadius(8)
        .backgroundColor(this.showPanel && this.panelTab === t.id
          ? (this.dark ? 'rgba(143,182,255,0.12)' : 'rgba(0,125,255,0.08)')
          : Color.Transparent)
        .onClick(() => this.openTab(t.id))
      }, (t: TabDef) => t.id)
    }
    .width(56)
    .height('100%')
    .padding({ top: 8 })
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(Theme.panel(this.dark))
    .border({ width: this.dark ? 0.5 : 1, color: Theme.border(this.dark) })
  }

  build() {
    // 二十六包：根节点换 Stack——工具参数弹窗作为整页模态盖在最上层
    //（scrim 挡底栏/面板/画布全部点击；卡片钉页面底部，自带键盘沿其下）
    Stack({ alignContent: Alignment.Bottom }) {
      Column() {
      if (this.isWide) {
        // 宽屏 / PC（上游 桌面样式）：左侧视图栏（四视图钮竖排）+ 停靠面板
        //（代数区/工具/表格/数据，随 Tab 切换）+ 右侧画布。面板右缘分隔条
        // 可拖宽（上游 桌面 dock 同款）。第二十一包①：原「收回 26vp 窄条」
        // 删除——视图栏常驻，收起/展开全由栏上按钮驱动。
        // 第十七包②：键盘垫高从 Index 页面级挪进本组件（Row 下垫 kbSpacer）
        // ——实测避让按「输入行底边距页面底」反解垫高，垫高与输入行必须同
        // 组件才随动 1:1；Index 级垫高在组件外，反解不收敛
        Column() {
          Row() {
            this.wideRail()
            // 三十二包④：工具弹窗期间停靠面板整块卸载（与窄屏抽屉同语义
            // ——opacity 保位留下纯色占位区挡画布），画布扩展到弹窗背后
            if (this.showPanel && this.toolAskKind === '') {
              // 第二十二包⑥：面板外包一层 Stack，左下角悬浮 ⌨ 呼出钮——
              // 上游 ShowKeyboardButton 同位（应用左下角、编辑器聚焦时出现、
              // 虚拟键盘弹出后由键盘自身收起，本按钮只负责系统键盘→自带键
              // 盘的召唤）。窄屏头部 ⌨ 保留不变（用户口径）
              Stack({ alignContent: Alignment.BottomStart }) {
                Column() {
                  this.bottomPanel(true)
                }
                .width('100%')
                .height('100%')
                .borderRadius({ topRight: 12, bottomRight: 12 })
                .backgroundColor(Theme.drawer(this.dark))
                .transition(TransitionEffect.asymmetric(
                  TransitionEffect.opacity(0).combine(TransitionEffect.translate({ x: -40 })),
                  TransitionEffect.opacity(0).combine(TransitionEffect.translate({ x: -40 }))
                ).animation({ duration: 260, curve: Curve.EaseInOut }))

                if (this.inputFocused && this.useSysKeyboard) {
                  Text('⌨')
                    .fontSize(20)
                    .fontColor(Theme.sub(this.dark))
                    .opacity(0.6)
                    .width(36)
                    .height(36)
                    .textAlign(TextAlign.Center)
                    .borderRadius(18)
                    .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
                    .borderWidth(1)
                    .borderColor(Theme.border(this.dark))
                    .shadow({ radius: 8, color: 'rgba(0,0,0,0.18)', offsetX: 0, offsetY: 1 })
                    .margin({ left: 10, bottom: 10 })
                    .onClick(() => this.toggleKeyboard())
                }
              }
              .width(this.widePanelW)
              .height('100%')

              // 右缘拖宽分隔条（上游 dock 拖拽同款；可见竖线把手 + 12vp 触区；
              // 钳位 280 ~ 页宽-视栏-画布最小 300。二十二包②：悬停 ↔ 光标）
              Row() {
                Column()
                  .width(3)
                  .height('100%')
                  .backgroundColor(Theme.border(this.dark))
              }
              .width(12)
              .height('100%')
              .hitTestBehavior(HitTestMode.Default)
              .onHover((h: boolean) => this.setResizeCursor(h, false))
              .gesture(PanGesture({ fingers: 1, direction: PanDirection.Horizontal })
                .onActionStart(() => {
                  this.widePanelWStart = this.widePanelW;
                })
                .onActionUpdate((e: GestureEvent) => {
                  const maxW = Math.max(300, this.pageW - 56 - 300);
                  this.widePanelW = Math.min(Math.max(280,
                    this.widePanelWStart + e.offsetX), maxW);
                }))
            }

            Column() {
              this.canvasArea()
            }
            .layoutWeight(1)
            .height('100%')
          }
          .width('100%')
          .layoutWeight(1)

          if ((this.mathKbVisible || this.kbSpacer > 0) && this.isWide) {
            // 三十一包①b：kbSpacer 也覆盖系统键盘态滑动条避让（宽屏+系统
            // 键盘时 mathKbVisible=false，条件需含 kbSpacer>0，与窄屏对称）
            Row()
              .width('100%')
              .height(this.kbSpacer)
          }
        }
        .width('100%')
        .height('100%')
      } else {
        // 窄屏（手机竖屏）：画布 + 底部面板抽屉（代数区/工具/表格/数据）+ 四 Tab 栏
        Column() {
          this.canvasArea()

          this.errorText()

          // 面板抽屉：展开时占相应档位高度（二十二包③起顶部把手可竖向拖拽
          // 调高，钳位 160 ~ 页高-260，切 Tab 归回档位；上游 手机端面板可拖
          // 高同语义），画布相应压缩（cas 与代数区同档 40%——输入行 + 历史
          // 列表需要更多高度）。三十二包④：工具弹窗期间整块卸载——三十
          // 一包②的 opacity 保位只隐了内容，抽屉背景色仍占位成「纯色无内
          // 容区域」挡画布；卸载后画布扩展到弹窗背后，关闭时随过渡动画回位
          if (this.showPanel && this.toolAskKind === '') {
            Column() {
              // 顶部拖拽把手：36×4 圆角胶囊 + 18vp 触区；上拖增高/下拖减高
              //（二十二包③）。PC 鼠标悬停 ↕ 光标（二十二包②）
              Row() {
                Column()
                  .width(36)
                  .height(4)
                  .borderRadius(2)
                  .backgroundColor(this.dark ? 'rgba(255,255,255,0.30)' : 'rgba(0,0,0,0.25)')
              }
              .width('100%')
              .height(18)
              .justifyContent(FlexAlign.Center)
              .hitTestBehavior(HitTestMode.Default)
              .onHover((h: boolean) => this.setResizeCursor(h, true))
              .gesture(PanGesture({ fingers: 1, direction: PanDirection.Vertical })
                .onActionStart(() => {
                  this.drawerHStart = this.drawerH > 0
                    ? this.drawerH : this.drawerDefaultH();
                })
                .onActionUpdate((e: GestureEvent) => {
                  const maxH = Math.max(200, this.pageH - 260);
                  this.drawerH = Math.min(Math.max(160,
                    this.drawerHStart - e.offsetY), maxH);
                }))

              // 面板容器：layoutWeight(1) 占抽屉扣除把手后的净高——此前
              // bottomPanel 各面板根节点 height('100%') 是抽屉总高的 100%，
              // 加上 18vp 把手恒溢出 18vp 到底栏下方（二十四包③④：分析
              // 统计表/工具提示条被 Tab 栏遮住的根因）
              Column() {
                this.bottomPanel(false)
              }
              .width('100%')
              .layoutWeight(1)
            }
            .width('100%')
            .height(this.drawerHeight())
            .backgroundColor(Theme.drawer(this.dark))
            .borderRadius({ topLeft: 12, topRight: 12 })
            .transition(TransitionEffect.asymmetric(
              TransitionEffect.opacity(0).combine(TransitionEffect.translate({ y: 120 })),
              TransitionEffect.opacity(0).combine(TransitionEffect.translate({ y: 120 }))
            ).animation({ duration: 260, curve: Curve.EaseInOut }))
          }

          // 轮 C 样式条（选中对象时出现在底栏上方，上游 移动端样式条位置）
          if (this.selectedLabel.length > 0) {
            this.styleBarRow()
          }

          // 键盘避让（第十七包② = 运行时实测）：静态几何两轮没对上
          // （第十五包 178 少一行、第十六包 220 反多两行——中间隔着样式条/
          // 边距/分支，推不准）。现改为实测：钉底输入行 onAreaChange 上报
          // 自身底边（globalPosition，双口径自校准），垫高一步收敛到「输入行底边
          // = 键盘上沿 + 4」。系统键盘仍是窗口压缩、无需垫高；底栏照旧钉
          // 在屏幕底被自带键盘盖住（第十四包②）；宽屏垫高同源（Row 下方）
          // 三十一包①b：kbSpacer 也会被系统键盘态滑动条字段避让垫高（此时
          // mathKbVisible=false），垫高条件放宽
          if ((this.mathKbVisible || this.kbSpacer > 0) && !this.isWide) {
            Row()
              .width('100%')
              .height(this.kbSpacer)
          }

          // 三十包③：工具弹窗期间隐藏底栏 Tab——系统键盘态底栏被避让顶到
          // 键盘上沿露出来（自带键盘态卡片更高正好盖住，故只在系统键盘下
          // 可见）。隐藏对齐两种键盘形态；外包 Column 保位（布局高度不变，
          // 关弹窗无跳动），点击由 scrim 拦截
          Column() {
            this.bottomBar()
          }
          .width('100%')
          .opacity(this.toolAskKind === '' ? 1 : 0)
        }
        .width('100%')
        .height('100%')
      }
      }
      .width('100%')
      .height('100%')
      .id('geoRoot')
      .onAreaChange((_o: Area, n: Area) => {
      this.pageH = n.height as number;
      this.pageW = n.width as number;
      // 二十八包④：弹窗开着时页高回涨（上一次键盘收起）→ 基准跟上最大值，
      // 防打开瞬间拍到键盘余波中的缩高、随后闸门误判「未 resize」
      if (this.toolAskKind !== '' && this.pageH > this.askOpenPageH) {
        this.askOpenPageH = this.pageH;
      }
      this.rootY = n.globalPosition.y as number;   // 垫高实测基准（差值消页面外偏移）
      // 第十七包①：宽窄只看窗口实际宽度（PC 虚拟机锁手机比例时按窄屏
      // 布局，不再按设备类型强制宽屏）
      const wide = (n.width as number) >= 640;
      if (wide !== this.isWide) {
        this.isWide = wide;
        // Index 页面级键盘垫高只用于宽屏（窄屏垫高已移入本组件底栏上方，
        // 第十三包②）
        AppStorage.setOrCreate('ui.isWide', wide);
        if (wide && !this.showPanel) {
          // 上游 宽屏惯例：代数区常显
          this.panelTab = 'algebra';
          this.setPanelVisible(true);
          this.syncAlgebra(dreamemberKernel.kernelAlgebra());
        }
      }
      })

      // 三十六包：顶层自带键盘面板（在弹窗层之下——弹窗自带内联键盘，
      // 且 mathKbOverlayUp 含 toolAskKind 门，两者不会同现）
      if (this.mathKbOverlayUp()) {
        this.mathKbOverlay()
      }

      // 二十六包：工具参数弹窗（Stack 最后一个子节点 = 最上层）
      if (this.toolAskKind !== '') {
        this.toolAskDialog()
      }
    }
    .width('100%')
    .height('100%')
  }
}
