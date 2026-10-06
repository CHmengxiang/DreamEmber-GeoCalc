// AlgebraPanel.ets — 代数区面板（自 CanvasView 拆出，轮 A 防乱重构）：
// 头部（计数/⌨/收起）+ 错误条 + 行列表 + 行内修改/新建行。
// 四十八包②：列表按对象类型分组呈现——组头（箭头折叠/展开 + 计数 + 显隐
// 眼批量开关组内对象）+ 成员行；分组只重排行序，行交互不变。
// 会话状态（待输入文本、编辑行、键盘开关）留在 CanvasView 统一编排，面板
// 只呈现并转发事件；宽屏左栏（showError=true：错误条在头部之下）与窄屏
// 抽屉（showError=false：错误条由画布下方渲染）共用本组件。
// 第七包：自带键盘模式下输入框上叠 MathField 公式化显示（上标/根号/绝对
// 值/占位框+光标），TextInput 保留在底层（视觉隐藏）承载焦点与编辑链路；
// ⌨ 切到系统键盘时恢复原生 TextInput 直显。
// 第十七包②：输入行 onAreaChange 经新回调 onInputArea 上报自身尺寸
// ——CanvasView 据此反解自带键盘垫高（运行时实测，静态几何两轮没对上）；
// 第二十二包④起输入行回面板顶部，避让只在输入行低于键盘上沿时才垫。
import { AlgebraEntry } from '../model/GeoTypes';
import { Theme } from '../model/Theme';
import { AlgebraRowView } from './AlgebraRowView';
import { MathField } from './MathField';

// 四十八包②：代数区分组——固定类型序（上游代数区按类型分组的常用次序），
// numeric 与 slider 同属「数字」组（上游滑动条即带滑杆的数值）；未知类型
// 归「其他」组兜底。分组只影响列表呈现（折叠/显隐头），行的交互与内核
// 顺序无关。
const GRP_DEFS: string[][] = [
  ['numeric', 'slider'],   // 数字（含滑动条）
  ['angle'],               // 角度
  ['point'],               // 点
  ['vector'],              // 向量
  ['segment'],             // 线段
  ['ray'],                 // 射线
  ['line'],                // 直线
  ['circle'],              // 圆
  ['function'],            // 函数
  ['polygon'],             // 多边形
  ['list'],                // 列表
  ['text']                 // 文本
];
const GRP_NAMES: string[] = ['数字', '角度', '点', '向量', '线段', '射线',
  '直线', '圆', '函数', '多边形', '列表', '文本'];

// 分组行模型：headType 非空 = 分组头行（entry 挂首成员仅占位，不渲染）；
// 否则为对象行。eyeColor/chevron 在行构建时算好，@Builder 内只做呈现。
class AlgebraGrpRow {
  headType: string = '';
  name: string = '';
  count: number = 0;
  allVis: boolean = true;
  eyeColor: string = '';
  chevron: string = '▾';
  entry: AlgebraEntry = new AlgebraEntry('', 'numeric', '', '');
  members: AlgebraEntry[] = [];
  key: string = '';
}

@Component
export struct AlgebraPanel {
  @Link algebra: AlgebraEntry[];
  @Link editingLabel: string;
  @Link creating: boolean;
  @Link pendingInput: string;
  @Link inputError: string;
  // 二十九包②：滑动条行内编辑（行下方展开四项，替代二十八包的弹窗）。
  // 状态真源在 CanvasView，@Link 双向绑定供字段直写
  @Link slideEditLabel: string;
  @Link slideEditMin: string;
  @Link slideEditMax: string;
  @Link slideEditStep: string;
  @Link slideEditVal: string;
  @Link slideEditErr: string;
  onSlideField: (i: number) => void = (_i: number) => {};
  onSlideBlur: (i: number) => void = (_i: number) => {};
  onSlideSel: (i: number, s: number, e: number) => void =
    (_i: number, _s: number, _e: number) => {};
  onSlideSubmit: () => void = () => {};
  onSlideCancel: () => void = () => {};
  onSlideSwitch: () => void = () => {};
  @Prop dark: boolean = false;
  @Prop useSysKeyboard: boolean = false;
  @Prop isWide: boolean = false;
  @Prop showError: boolean = false;
  // MathField 光标（自带键盘模式）与聚焦态
  @Prop mathCaret: number = 0;
  // 第十包②：光标框内/框外双态（框内小号上/下移，框外基线）
  @Prop mathCaretScript: boolean = false;
  @Prop fieldFocused: boolean = false;
  // 共享对象（父级创建并传入）：改名/编辑行的滚动避让与光标定位
  scroller: Scroller = new Scroller();
  createCtrl: TextInputController = new TextInputController();
  editCtrl: TextInputController = new TextInputController();
  onToggleVis: (l: string, s: boolean) => void = (_l: string, _s: boolean) => {};
  onSelect: (l: string) => void = (_l: string) => {};
  onRename: (l: string, n: string) => void = (_l: string, _n: string) => {};
  onRenameRequest: (l: string) => void = (_l: string) => {};
  onEdit: (l: string, d: string) => void = (_l: string, _d: string) => {};
  // 二十九包②：slider 行值文本点击 → 父级行内展开四项编辑（透传行组件）
  onSliderEdit: (l: string) => void = (_l: string) => {};
  onToggleAnim: (l: string) => void = (_l: string) => {};
  onDelete: (l: string) => void = (_l: string) => {};
  onSlider: (l: string, v: number, m: number) => void =
    (_l: string, _v: number, _m: number) => {};
  onAlpha: (l: string, v: number, m: number) => void =
    (_l: string, _v: number, _m: number) => {};
  onSubmit: () => void = () => {};
  onCancel: () => void = () => {};
  onCreate: () => void = () => {};
  onToggleKeyboard: () => void = () => {};
  onFocusInput: (focused: boolean) => void = (_f: boolean) => {};
  onSelChange: (s: number, e: number) => void = (_s: number, _e: number) => {};
  // 第十七包②：钉底输入行尺寸上报——CanvasView 据此反解键盘垫高（实测避让）
  onInputArea: (n: Area) => void = (_n: Area) => {};
  // 三十二包③：列表空白/非交互区点按通知——宿主收起滑动条行内编辑区
  //（块内点按已被块容器 stopPropagation 拦截，值文本开启处自行拦截）
  onListTap: () => void = () => {};
  // 三十二包③：块内任意点按通知——宿主设护栏时间窗，豁免冒泡到列表层
  // 的「点空白收起」（点块内空白/按钮不关编辑区，点块外才关）
  onBlockTap: () => void = () => {};
  onFieldActivate: () => void = () => {};
  // 四十六包：点触定位光标（MathField 点按反查停靠点 → 宿主设光标态并
  // 同步底层 TextInput 选区）
  onFieldTapCaret: (pos: number, script: boolean) => void =
    (_p: number, _s: boolean) => {};
  onClose: () => void = () => {};

  // 四十八包②：分组折叠态（键=组标识，如 'numeric'/'other'）；空=全展开。
  // 切换时整体重赋值（@State 数组须换引用才触发刷新）
  @State private grpCollapsed: string[] = [];

  // 组头行构建：眼睛颜色随组类型（其他组用中性色）、箭头随折叠态；
  // 键编码折叠/显隐态，状态翻转即换键 → ForEach 重建头行刷新眼睛
  private grpHeadRow(name: string, key: string, eyeType: string,
    idxs: number[]): AlgebraGrpRow {
    const r = new AlgebraGrpRow();
    r.headType = key;
    r.name = name;
    r.count = idxs.length;
    const ms: AlgebraEntry[] = [];
    let allVis = true;
    for (const i of idxs) {
      ms.push(this.algebra[i]);
      if (this.algebra[i].vis === '0') {
        allVis = false;
      }
    }
    r.members = ms;
    if (ms.length > 0) {
      r.entry = ms[0];
    }
    r.allVis = allVis;
    r.eyeColor = eyeType.length > 0
      ? Theme.typeColor(eyeType, this.dark) : Theme.sub(this.dark);
    r.chevron = this.grpCollapsed.indexOf(key) >= 0 ? '▸' : '▾';
    r.key = 'H:' + key + ':' + r.count + ':' + (allVis ? 1 : 0)
      + (r.chevron === '▸' ? 'c' : '');
    return r;
  }

  // 分组行序列：每组头行后跟成员行（折叠组不发成员行）；不入任何已知组
  // 的类型归「其他」兜底组
  private buildGrpRows(): AlgebraGrpRow[] {
    const rows: AlgebraGrpRow[] = [];
    const used: boolean[] = [];
    for (let i = 0; i < this.algebra.length; i++) {
      used.push(false);
    }
    for (let d = 0; d < GRP_DEFS.length; d++) {
      const idxs: number[] = [];
      for (let i = 0; i < this.algebra.length; i++) {
        if (!used[i] && GRP_DEFS[d].indexOf(this.algebra[i].type) >= 0) {
          idxs.push(i);
          used[i] = true;
        }
      }
      if (idxs.length === 0) {
        continue;
      }
      rows.push(this.grpHeadRow(GRP_NAMES[d], GRP_DEFS[d][0], GRP_DEFS[d][0], idxs));
      if (this.grpCollapsed.indexOf(GRP_DEFS[d][0]) >= 0) {
        continue;
      }
      for (const i of idxs) {
        const r = new AlgebraGrpRow();
        r.entry = this.algebra[i];
        r.key = r.entry.label + '#' + (this.dark ? 'D' : 'L');
        rows.push(r);
      }
    }
    const rest: number[] = [];
    for (let i = 0; i < this.algebra.length; i++) {
      if (!used[i]) {
        rest.push(i);
      }
    }
    if (rest.length > 0) {
      rows.push(this.grpHeadRow('其他', 'other', '', rest));
      if (this.grpCollapsed.indexOf('other') < 0) {
        for (const i of rest) {
          const r = new AlgebraGrpRow();
          r.entry = this.algebra[i];
          r.key = r.entry.label + '#' + (this.dark ? 'D' : 'L');
          rows.push(r);
        }
      }
    }
    return rows;
  }

  private toggleGrpCollapse(key: string): void {
    const had = this.grpCollapsed.indexOf(key) >= 0;
    const next: string[] = [];
    for (const k of this.grpCollapsed) {
      if (k !== key) {
        next.push(k);
      }
    }
    if (!had) {
      next.push(key);
    }
    this.grpCollapsed = next;
  }

  // 显隐眼：组内有隐藏 → 全显示；全显示 → 全隐藏。逐行下发宿主
  //（onToggleVis 同单行按钮同一通道）
  private toggleGrpVis(row: AlgebraGrpRow): void {
    const target = !row.allVis;
    for (const m of row.members) {
      this.onToggleVis(m.label, target);
    }
  }

  // 三十六包：浮层空壳——customKeyboard 挂载即抑制系统 IME，1px 透明壳
  // 的弹出/收起不可见；键盘本体在宿主页面顶层（CanvasView.mathKbOverlay，
  // mathKbVisible 驱动），换字段焦点转移不再「旧浮层收起+新浮层弹出」
  @Builder
  private emptyKb() {
    Row()
      .width('100%')
      .height(1)
      .opacity(0)
  }

  // 四项字段行（i：0=最小值 1=最大值 2=步长 3=当前值）。TextInput 挂
  // 空壳 customKeyboard（挂载即抑制系统 IME，键盘本体在宿主页面顶层
  // CanvasView.mathKbOverlay——三十六包换字段不重弹）；系统键盘模式解绑
  // 走系统 IME + 页面 OFFSET 避让
  @Builder
  private slideFieldRow(i: number, title: string, placeholder: string) {
    Row() {
      Text(title)
        .fontSize(12)
        .fontColor(Theme.sub(this.dark))
        .width(52)
      TextInput({ text: this.slideText(i), placeholder: placeholder })
        .id('slideEditF' + i)
        .fontSize(13)
        .height(32)
        .layoutWeight(1)
        .borderRadius(6)
        // 三十二包①：浅色下 inputBg 与块底同为白、无描边=「看不到框」——
        // 四框加浮层描边（深浅两态同取 Theme.border），与取消钮描边同源
        .border({ width: 1, color: Theme.border(this.dark) })
        .backgroundColor(Theme.inputBg(this.dark))
        .onChange((v: string) => {
          this.slideSetText(i, v);
        })
        .onFocus(() => this.onSlideField(i))
        .onBlur(() => this.onSlideBlur(i))
        .onTextSelectionChange((s: number, e: number): void =>
          this.onSlideSel(i, s, e))
        .onSubmit(() => this.onSlideSubmit())
        .customKeyboard(this.useSysKeyboard ? undefined : this.emptyKb())
    }
    .width('100%')
  }

  private slideText(i: number): string {
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

  private slideSetText(i: number, v: string): void {
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

  // 滑动条行下方展开的四项编辑块（点行值文本展开/收起，开关语义在
  // CanvasView.openSliderEdit）：最小值/最大值/步长/当前值依次排列、各自
  // 独立编辑，⌨ 切键盘，取消/确定整块应用（原子下内核）
  @Builder
  private slideEditFields() {
    Column({ space: 6 }) {
      this.slideFieldRow(0, '最小值', '')
      this.slideFieldRow(1, '最大值', '')
      this.slideFieldRow(2, '步长', '0 = 连续')
      this.slideFieldRow(3, '当前值', '')
      if (this.slideEditErr.length > 0) {
        Text(this.slideEditErr)
          .fontSize(11)
          .fontColor(Theme.error(this.dark))
          .width('100%')
      }
      Row() {
        Text('⌨')
          .fontSize(14)
          .fontColor(this.useSysKeyboard ? Theme.faint(this.dark) : Theme.accent(this.dark))
          .width(32)
          .height(28)
          .textAlign(TextAlign.Center)
          .onClick(() => this.onSlideSwitch())
        Blank()
        Text('取消')
          .fontSize(12)
          .fontColor(Theme.sub(this.dark))
          .borderRadius(6)
          .height(30)
          .padding({ left: 12, right: 12 })
          .border({ width: 0.5, color: Theme.border(this.dark) })
          .onClick(() => this.onSlideCancel())
        Text('确定')
          .fontSize(12)
          .fontColor('#FFFFFF')
          .backgroundColor(Theme.accent(this.dark))
          .borderRadius(6)
          .height(30)
          .padding({ left: 12, right: 12 })
          .margin({ left: 8 })
          .onClick(() => this.onSlideSubmit())
      }
      .width('100%')
    }
    .width('100%')
    .padding({ left: 38, right: 12, top: 6, bottom: 8 })
    .margin({ top: 2 })
    .borderRadius(6)
    .backgroundColor(Theme.rowBg(this.dark))
    // 三十一包①a：组件 id——垫高/滚动对齐的实测锚点都是「slideEditBlock
    // 整块」（含取消/确定行），CanvasView 按 getRectangleById 窗口矩形反
    // 解（Area 对被裁剪的 List 子项不可靠）。三十二包②：撤 onAreaChange
    // 重解触发——用户滚动列表时块面积连帧回调，避让反复反解与滚动打架
    //（真机②「内容不动、外框反向动」），锚定只在聚焦/键盘变化时进行
    .id('slideEditBlock')
    // 三十二包③：块内点按（含空白垫边）上报宿主设护栏，冒泡到列表层的
    //「点空白收起」在窗口期内不生效——点块内不关、点块外才关
    .onClick(() => {
      this.onBlockTap()
    })
  }

  // 输入行主体：TextInput（编辑与焦点锚点）+ MathField 公式化显示叠加。
  // 自带键盘模式 TextInput 视觉隐藏（0.01 透明度保焦点链路），MathField
  // 在上层展示公式与光标；系统键盘模式 TextInput 恢复直显
  @Builder
  private inputStack(inputId: string, placeholder: string) {
    Stack() {
      TextInput({ text: this.pendingInput, placeholder: placeholder,
        controller: inputId === 'algebraCreateInput' ? this.createCtrl : this.editCtrl })
        .id(inputId)
        .width('100%')
        .height('100%')
        .fontSize(14)
        .backgroundColor(Theme.inputBg(this.dark))
        .opacity(this.useSysKeyboard ? 1 : 0.01)
        .onChange((v: string) => {
          this.pendingInput = v;
        })
        .onTextSelectionChange((s: number, e: number) => {
          this.onSelChange(s, e);
        })
        .onFocus(() => {
          this.onFocusInput(true);
        })
        .onBlur(() => {
          this.onFocusInput(false);
        })
        .onSubmit(() => this.onSubmit())
        .customKeyboard(this.useSysKeyboard ? undefined : this.emptyKb())
      if (!this.useSysKeyboard) {
        MathField({
          raw: this.pendingInput,
          caret: this.mathCaret,
          caretScript: this.mathCaretScript,
          focused: this.fieldFocused,
          dark: this.dark,
          onActivate: () => this.onFieldActivate(),
          onTapCaret: (pos: number, script: boolean): void =>
            this.onFieldTapCaret(pos, script)
        })
      }
    }
    .alignContent(Alignment.Start)
    .layoutWeight(1)
    .height(32)
    .margin({ right: 6 })
  }

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

  @Builder
  private header() {
    Row() {
      Text('代数区')
        .fontSize(13)
        .fontWeight(FontWeight.Medium)
        .fontColor(Theme.sub(this.dark))
        .layoutWeight(1)
      Text(this.algebra.length > 0 ? `${this.algebra.length} 个对象` : '暂无对象')
        .fontSize(11)
        .fontColor(Theme.faint(this.dark))
        .margin({ right: 8 })
      // 第二十二包⑥：宽屏 ⌨ 呼出钮改 上游 ShowKeyboardButton 同位（应用左
      // 下角悬浮，CanvasView 渲染），头部不再重复放
      if (!this.isWide) {
        Text('⌨')
          .fontSize(15)
          .fontColor(this.useSysKeyboard ? Theme.faint(this.dark) : Theme.accent(this.dark))
          .width(32)
          .height(28)
          .textAlign(TextAlign.Center)
          .onClick(() => this.onToggleKeyboard())
      }
      Text(this.isWide ? '❱' : '▾')
        .fontSize(14)
        .fontColor(Theme.sub(this.dark))
        .width(32)
        .height(28)
        .textAlign(TextAlign.Center)
        .onClick(() => this.onClose())
    }
    .width('100%')
    .padding({ left: 12, right: 8, top: 6, bottom: 2 })
  }

  // 行内修改框（点行内数值/定义展开，与改名同款就地编辑）：对象名保留在左，
  // 输入框从名字处拉通到右侧 ✕/✓（回车或 ✓ 提交重定义，✕ 取消）
  @Builder
  private editRow(e: AlgebraEntry) {
    Row() {
      Text()
        .width(18)
        .height(18)
        .borderRadius(9)
        .borderWidth(2)
        .borderColor(Theme.typeColor(e.type, this.dark))
        .backgroundColor(e.vis === '0' ? Color.Transparent : Theme.typeColor(e.type, this.dark))
        .margin({ right: 8 })
      Text(e.label)
        .fontSize(14)
        .fontWeight(FontWeight.Medium)
        .fontColor(Theme.text(this.dark))
        .maxLines(1)
        .width(56)
        .height(32)
        .textAlign(TextAlign.Start)
      this.inputStack('algebraDefEdit', '')
      // 十四轮：Button 的 label 字符框过窄导致 ✕ 右缘被裁——改 Text 自绘
      // 点按区，字符框拉宽并居中
      Text('✕')
        .fontSize(15)
        .fontColor(Theme.sub(this.dark))
        .width(44)
        .height(32)
        .textAlign(TextAlign.Center)
        .borderRadius(8)
        .backgroundColor(Theme.keyBg(this.dark))
        .margin({ right: 6 })
        .onClick(() => this.onCancel())
      Text('✓')
        .fontSize(16)
        .fontColor(Theme.accent(this.dark))
        .width(44)
        .height(32)
        .textAlign(TextAlign.Center)
        .borderRadius(8)
        .backgroundColor(Theme.keyBg(this.dark))
        .onClick(() => this.onSubmit())
    }
    .width('100%')
    .height(44)
    .padding({ left: 12, right: 8 })
    .borderRadius(6)
    .borderWidth(1.5)
    .borderColor(Theme.accent(this.dark))
    .backgroundColor(Theme.rowBg(this.dark))
  }

  // "＋ 新建对象"输入行：列表末尾常驻按钮，点击展开行内输入框
  // （＋ 在左、输入框拉通、✕/✓ 紧贴右侧，与行内修改框同款布局）
  @Builder
  private createArea() {
    if (this.creating) {
      Row() {
        Text('＋')
          .fontSize(16)
          .fontColor(Theme.accent(this.dark))
          .width(26)
          .height(32)
          .textAlign(TextAlign.Center)
        this.inputStack('algebraCreateInput',
          '输入对象定义：y=x^2/4、(3,4)、Factor(f)…')
        // 同 editRow：Text 自绘 ✕/✓（字符框拉宽居中，防 Button label 裁字）
        Text('✕')
          .fontSize(15)
          .fontColor(Theme.sub(this.dark))
          .width(44)
          .height(32)
          .textAlign(TextAlign.Center)
          .borderRadius(8)
          .backgroundColor(Theme.keyBg(this.dark))
          .margin({ right: 6 })
          .onClick(() => this.onCancel())
        Text('✓')
          .fontSize(16)
          .fontColor(Theme.accent(this.dark))
          .width(44)
          .height(32)
          .textAlign(TextAlign.Center)
          .borderRadius(8)
          .backgroundColor(Theme.keyBg(this.dark))
          .onClick(() => this.onSubmit())
      }
      .width('100%')
      .height(44)
      .padding({ left: 12, right: 8 })
      .borderRadius(6)
      .borderWidth(1.5)
      .borderColor(Theme.accent(this.dark))
      .backgroundColor(Theme.rowBg(this.dark))
      .margin({ top: 2, bottom: 6 })
    } else {
      Row() {
        Text('＋ 新建对象')
          .fontSize(14)
          .fontColor(Theme.accent(this.dark))
          .height(40)
      }
      .width('100%')
      .justifyContent(FlexAlign.Center)
      .borderRadius(8)
      .borderWidth(1)
      .borderColor(Theme.border(this.dark))
      .backgroundColor(Theme.rowBg(this.dark))
      .margin({ top: 2, bottom: 6 })
      .onClick(() => this.onCreate())
    }
  }

  // 四十八包②：分组头行——箭头（折叠/展开）+ 组名 + 计数 + 显隐眼（实心=
  // 组内全显示，点按全隐藏；空心=有隐藏，点按全显示）。点行其余位置折叠
  // /展开
  @Builder
  private groupHeadRow(row: AlgebraGrpRow) {
    Row() {
      Text(row.chevron)
        .fontSize(11)
        .fontColor(Theme.faint(this.dark))
        .width(20)
        .height(24)
        .textAlign(TextAlign.Center)
      Text(row.name)
        .fontSize(12)
        .fontWeight(FontWeight.Medium)
        .fontColor(Theme.sub(this.dark))
      Text(`${row.count}`)
        .fontSize(11)
        .fontColor(Theme.faint(this.dark))
        .margin({ left: 6 })
      Blank()
      Text()
        .width(16)
        .height(16)
        .borderRadius(8)
        .borderWidth(2)
        .borderColor(row.eyeColor)
        .backgroundColor(row.allVis ? row.eyeColor : Color.Transparent)
        .onClick(() => {
          this.toggleGrpVis(row)
        })
    }
    .width('100%')
    .height(28)
    .padding({ left: 6, right: 12 })
    .margin({ top: 2 })
    .onClick(() => {
      this.toggleGrpCollapse(row.headType)
    })
  }

  // 对象行（编辑行/普通行/滑动条行内编辑块，四十八包②自 listArea 拆出）
  @Builder
  private entryItem(e: AlgebraEntry) {
    if (this.editingLabel === e.label) {
      // 修改模式：整行就地替换为编辑框（与改名同款）
      this.editRow(e)
    } else {
      Column() {
        AlgebraRowView({
          entry: e,
          dark: this.dark,
          onToggleVis: (l: string, s: boolean): void => this.onToggleVis(l, s),
          onSelect: (l: string): void => this.onSelect(l),
          onRename: (l: string, n: string): void => this.onRename(l, n),
          onRenameRequest: (l: string): void => this.onRenameRequest(l),
          onEdit: (l: string, d: string): void => this.onEdit(l, d),
          onSliderEdit: (l: string): void => this.onSliderEdit(l),
          onToggleAnim: (l: string): void => this.onToggleAnim(l),
          onDelete: (l: string): void => this.onDelete(l),
          onSlider: (l: string, v: number, m: number): void => this.onSlider(l, v, m),
          onAlpha: (l: string, v: number, m: number): void => this.onAlpha(l, v, m)
        })
        // 二十九包②：滑动条行内编辑块——行值文本点击后在本行下方
        // 展开（四项独立编辑 + ⌨ 切键盘），与行同一 ListItem 滚动，
        // 打开时该行已滚到列表顶部（CanvasView.openSliderEdit）
        if (e.type === 'slider' && this.slideEditLabel === e.label) {
          this.slideEditFields()
        }
      }
      .width('100%')
    }
  }

  @Builder
  private listArea() {
    List({ space: 2, scroller: this.scroller }) {
      ForEach(this.buildGrpRows(), (row: AlgebraGrpRow) => {
        ListItem() {
          if (row.headType.length > 0) {
            this.groupHeadRow(row)
          } else {
            this.entryItem(row.entry)
          }
        }
      }, (row: AlgebraGrpRow) => row.key)
    }
    .width('100%')
    .layoutWeight(1)
    .padding({ left: 8, right: 8 })
    .scrollBar(BarState.Auto)
    // 三十一包①a：列表 id——滚动对齐量按「块底 vs 列表视口底」窗口矩形
    // 实测（块在视口下方被裁剪的部分只能靠滚出来，垫高无解）
    .id('algebraList')
    // 三十二包③：点列表空白/非交互区 = 点别处，收起滑动条行内编辑区
    //（上游 编辑框失焦即收同语义；开块/换字段/切键盘的即时点按由宿主
    // 时间窗护栏豁免，防误收）
    .onClick(() => {
      this.onListTap()
    })
  }

  build() {
    Column() {
      this.header()
      if (this.showError) {
        this.errorText()
      }
      // 第二十二包④：「＋ 新建对象」输入行移回面板顶部（用户反馈「像一开
      // 始那样」；第十五包①钉底按 上游 输入行常驻底部的布局就此撤回）。
      // 键盘避让逻辑不变（applyKbSpacer 的 max(0,…) 只在输入行低于键盘上
      // 沿时垫高）——输入行在顶部时其底边天然高于键盘，避让自动不再触发
      //（垫高恒 0，键盘浮层盖住下方列表，上游 手机端同观感）。
      Column() {
        this.createArea()
      }
      .width('100%')
      .padding({ left: 8, right: 8, top: 2, bottom: 4 })
      .onAreaChange((_o: Area, n: Area) => {
        this.onInputArea(n);
      })
      this.listArea()
    }
    .width('100%')
    .layoutWeight(1)
  }
}
