// TablePanel.ets — 表格 Tab（第二十三包：上游 真·函数值表，TableValuesView
// 同位）。此前本 Tab 是数据分析占位；数据分析已改从数据区选区启动
//（panelTab 'analysis'），本面板接管 'data' Tab。
// 语义对齐 上游（上游表格视图，EUPL 源码仅作行为参照）：
// ① 行 = x 取值列，范围 −2/2/1（TableSettings DEFAULT_MIN/MAX/STEP），行
//   生成 = updateValuesFromRange 原循环（min 起步进累加至 max，尾部差
//   <1e-8 时补 max 行；min>max 或 step≤0 只留一行；(max−min)/step>200 报
//   上游 Error.TooManyRows 同义文案）；
// ② 列 = 构造里全部可求值函数 + 非竖直直线（GeoElement.hasTableOfValues
//   子集），列头 = 标签 + "(x)"（TableValuesFunctionColumn）；不等式区域
//   以值文本含 </> 过滤（内核 tableEval 二次兜底）；
// ③ 单元格 = 纯数值求值（非 CAS），经内核 tableEval 批量计算、FormatNum
//   格式化，空格串 = 未定义（上游 NaN 显示原始输入，此处显示空）。
// 偏差（v1）：x 列由范围驱动、不支持格内改 x/追加行（上游 的 x 是辅助
// GeoList）；列自动跟随构造，无 上游 的显式添加列/列头菜单。
// 渲染：外层行 ForEach + 行组件（TableRowView，@Prop 行数据）——真机嵌套
// ForEach 丢项前科（ToolsPanel 文件头），行组件化与 AlgebraRowView 同款。
import { AlgebraEntry } from '../model/GeoTypes';
import { Theme } from '../model/Theme';
import { MathKbHooks } from './MathKeyboard';
import dreamemberKernel from 'libdreamember.so';

const MAX_ROWS: number = 200;   // 上游 TableValuesView.MAX_ROWS

// 上游 updateValuesFromRange 原算法（ArkTS 版，累加值 1e-10 取整去浮点尾噪）
function genXs(min: number, max: number, step: number): number[] {
  if (min === 0 && max === 0 && step === 0) {
    return [];
  }
  if (min > max || step <= 0) {
    return [min];   // 上游：退化范围只留一行
  }
  const xs: number[] = [];
  let v = min;
  while (v <= max && xs.length <= MAX_ROWS) {
    xs.push(Math.round(v * 1e10) / 1e10);
    v += step;
  }
  if (xs.length <= MAX_ROWS && xs[xs.length - 1] < max - 1e-8) {
    xs.push(max);   // 上游：步进末行未达 max 时补一行
  }
  return xs;
}

// 行组件：一格一 Text（列数动态，无法字面渲染；单层 ForEach 安全）
@Component
struct TableRowView {
  @Prop cells: string[] = [];
  @Prop dark: boolean = false;

  build() {
    Row() {
      ForEach(this.cells, (c: string, i: number) => {
        Text(c)
          .fontSize(12)
          .fontColor(this.dark ? '#E8EAED' : '#1C1C1F')
          .textAlign(TextAlign.End)
          .maxLines(1)
          .height(30)
          .padding({ left: 6, right: 6 })
          .layoutWeight(i === 0 ? 1 : 2)
          .border({ width: 0.5, color: this.dark ? '#33363B' : '#E6E6EB' })
      }, (c: string, i: number) => `${i}#${c}#${this.dark ? 'D' : 'L'}`)
    }
    .width('100%')
  }
}

@Component
export struct TablePanel {
  // 构造列表（值/定义文本来源）；@Watch 变更即重算（函数重定义/增删跟随）
  @Prop @Watch('onAlgebraChg') algebra: AlgebraEntry[] = [];
  @Prop dark: boolean = false;
  @Prop isWide: boolean = false;   // 宽屏左栏形态（布局参数预留）
  onClose: () => void = () => {};
  // 二十四包①：范围三值由宿主持有——面板随 Tab 切换销毁，@State 会把
  // 起止/步长打回默认值（真机反馈①）；@Prop 下发 + applyRange 经回调回写
  @Prop @Watch('onAlgebraChg') xMin: number = -2;
  @Prop @Watch('onAlgebraChg') xMax: number = 2;
  @Prop @Watch('onAlgebraChg') xStep: number = 1;
  onRangeChange: (mn: number, mx: number, st: number) => void =
    (_mn: number, _mx: number, _st: number) => {};
  // 二十五包②：范围编辑合入自带键盘（0=系统/1=自带，宿主 useSysKeyboard
  // 下发）+ 头部 ⌨ 切换——此前三字段只有系统键盘一条路（真机反馈②），
  // 编辑行几何上报宿主实测垫高（键盘不盖编辑行，数据区同机制）
  @Prop useSysKeyboard: boolean = false;
  onToggleKeyboard: () => void = () => {};
  onEditFocus: (focused: boolean) => void = (_f: boolean) => {};
  // 三十六包：编辑会话期把面板键盘处理束注册到宿主（顶层键盘分发用；
  // null=会话结束注销）
  onKbHooks: (h: MathKbHooks | null) => void = (_h: MathKbHooks | null) => {};
  onEditArea: (n: Area) => void = (_n: Area) => {};
  @State private editingRange: boolean = false;
  @State private rangeErr: string = '';
  @State private minText: string = '-2';
  @State private maxText: string = '2';
  @State private stepText: string = '1';
  // 自带键盘插入协议状态：活动字段（0=起/1=止/2=步长）与光标；三字段各
  // 配 controller——caretPosition 只作用于绑定的那个输入框
  @State private rangeField: number = 0;
  @State private rangeSelA: number = 0;
  @State private rangeSelB: number = 0;
  private minCtrl: TextInputController = new TextInputController();
  private maxCtrl: TextInputController = new TextInputController();
  private stepCtrl: TextInputController = new TextInputController();
  @State private xs: number[] = genXs(-2, 2, 1);
  @State private cols: string[] = [];      // 函数/直线标签（列头 = 标签+(x)）
  @State private rows: string[][] = [];    // rows[r][0] = x 文本，其余 = f(x)

  aboutToAppear(): void {
    this.recompute();
  }

  private onAlgebraChg(): void {
    this.recompute();
  }

  // 列 = 构造里的函数（不等式经值文本过滤）+ 非竖直直线；行 = 范围生成 x
  // + 内核批量求值
  private recompute(): void {
    const nextCols: string[] = [];
    for (const e of this.algebra) {
      if (e.type !== 'function' && e.type !== 'line') {
        continue;
      }
      if (e.value.indexOf('<') >= 0 || e.value.indexOf('>') >= 0) {
        continue;   // 不等式区域（GeoFunction.ineqOp）无表列（上游 同）
      }
      nextCols.push(e.label);
    }
    const nextXs = genXs(this.xMin, this.xMax, this.xStep);
    const labels = nextCols.join(',');
    const xsText = nextXs.join(',');
    let grid: string[][] = [];
    if (nextCols.length > 0 && nextXs.length > 0) {
      const raw = dreamemberKernel.kernelTableEval(labels, xsText);
      grid = raw.length > 0
        ? raw.split(';').map((r: string): string[] => r.split(','))
        : [];
    }
    this.cols = nextCols;
    this.xs = nextXs;
    this.rows = nextXs.map((x: number, r: number): string[] => {
      const cells = (r < grid.length && grid[r].length >= nextCols.length)
        ? grid[r] : nextCols.map((): string => '');
      return [x.toString(), ...cells];
    });
  }

  // 二十五包②：收起范围编辑（头部范围文字再点/✕/确定共用）——顺带上报
  // 编辑态结束，宿主撤垫高；三十六包：同时注销面板键盘处理束
  private closeRangeEditor(): void {
    if (!this.editingRange) {
      return;
    }
    this.editingRange = false;
    this.onEditFocus(false);
    this.onKbHooks(null);
  }

  // 三十六包：编辑会话开合统一口——宿主垫高上报 + 键盘处理束注册同起同落
  private focusEdit(up: boolean): void {
    this.onEditFocus(up);
    this.onKbHooks(up ? this.hooks() : null);
  }

  private hooksCache: MathKbHooks | null = null;

  private hooks(): MathKbHooks {
    if (this.hooksCache === null) {
      const h = new MathKbHooks();
      h.insert = (t: string, back: number, skip: string): void =>
        this.kbInsert(t, back, skip);
      h.backspace = (): void => this.kbBackspace();
      h.submit = (): void => this.applyRange();
      h.hide = (): void => this.applyRange();
      h.caretLeft = (): void => this.kbCaretTo(this.rangeSelA - 1);
      h.caretRight = (): void => this.kbCaretTo(this.rangeSelA + 1);
      this.hooksCache = h;
    }
    return this.hooksCache;
  }

  // 三十六包：浮层空壳——customKeyboard 挂载即抑制系统 IME，1px 透明壳
  // 的弹出/收起不可见；键盘本体在宿主页面顶层（CanvasView.mathKbOverlay），
  // 三字段间焦点转移不再「旧键盘收起+新键盘弹出」
  @Builder
  private emptyKb() {
    Row()
      .width('100%')
      .height(1)
      .opacity(0)
  }

  // 应用范围（上游 InputDialogTableView：x起始值/终止值/步长；超 200 行报错）。
  // 值经 onRangeChange 回写宿主，重算由 xMin/xMax/xStep 的 @Watch 触发
  private applyRange(): void {
    const mn = parseFloat(this.minText);
    const mx = parseFloat(this.maxText);
    const st = parseFloat(this.stepText);
    if (isNaN(mn) || isNaN(mx) || isNaN(st)) {
      this.rangeErr = '请输入数字';
      return;
    }
    if (mn <= mx && st > 0 && (mx - mn) / st > MAX_ROWS) {
      this.rangeErr = '输入会创建过多的表格行，请更改范围值';
      return;
    }
    this.onRangeChange(mn, mx, st);
    this.rangeErr = '';
    this.closeRangeEditor();
  }

  // ---- 二十五包②：自带键盘（挂三个范围字段 customKeyboard，数据区同款）----

  private rangeText(): string {
    return this.rangeField === 0 ? this.minText
      : (this.rangeField === 1 ? this.maxText : this.stepText);
  }

  private setRangeText(v: string): void {
    if (this.rangeField === 0) {
      this.minText = v;
    } else if (this.rangeField === 1) {
      this.maxText = v;
    } else {
      this.stepText = v;
    }
  }

  private rangeCtrl(): TextInputController {
    return this.rangeField === 0 ? this.minCtrl
      : (this.rangeField === 1 ? this.maxCtrl : this.stepCtrl);
  }

  // 光标就位（与 TextInput 原生光标同步；未聚焦时静默失败，键入按本状态继续）
  private kbCaretTo(i: number): void {
    const v = Math.max(0, Math.min(i, this.rangeText().length));
    this.rangeSelA = v;
    this.rangeSelB = v;
    try {
      this.rangeCtrl().caretPosition(v);
    } catch (_e) {
    }
  }

  // 当前选区 [a, b]（越界/倒序收敛）
  private kbSelRange(): number[] {
    let a = this.rangeSelA;
    let b = this.rangeSelB;
    const len = this.rangeText().length;
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

  // 插入协议（数据区同款简化版：纯文本、skip 跳出保留、back<0 哨兵退化普通退格）
  private kbInsert(t: string, back: number, skip: string): void {
    const rg = this.kbSelRange();
    const a = rg[0];
    const b = rg[1];
    const cur = this.rangeText();
    if (skip.length > 0 && a === b && a + skip.length <= cur.length
      && cur.substring(a, a + skip.length) === skip) {
      this.kbCaretTo(a + skip.length);
      return;
    }
    const del = Math.min(back < 0 ? 1 : back, a);
    this.setRangeText(cur.substring(0, a - del) + t + cur.substring(b));
    this.kbCaretTo(a - del + t.length);
  }

  private kbBackspace(): void {
    this.kbInsert('', 1, '');
  }

  // 行 x 值文本（genXs 已做 1e-10 取整，直接 toString）
  private xText(x: number): string {
    return x.toString();
  }

  // 表头单元（x 列恒在——无函数时也显示 x 列 + 空表提示）
  private headerCells(): string[] {
    return ['x', ...this.cols];
  }

  build() {
    Column() {
      Row() {
        Text('表格')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.text(this.dark))
          .layoutWeight(1)
        Text(`${this.xMin} ~ ${this.xMax} · 步长 ${this.xStep}`)
          .fontSize(11)
          .fontColor(Theme.faint(this.dark))
          .margin({ right: 6 })
          .onClick(() => {
            if (this.editingRange) {
              this.closeRangeEditor();
              return;
            }
            this.minText = this.xMin.toString();
            this.maxText = this.xMax.toString();
            this.stepText = this.xStep.toString();
            this.rangeErr = '';
            this.editingRange = true;
          })
        // 二十五包②：⌨ 切系统/自带键盘（数据区/代数区头部同款，accent=自带生效）
        Text('⌨')
          .fontSize(16)
          .fontColor(this.useSysKeyboard ? Theme.faint(this.dark) : Theme.accent(this.dark))
          .width(30)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.onToggleKeyboard())
        Text('✕')
          .fontSize(14)
          .fontColor(Theme.sub(this.dark))
          .width(30)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => {
            this.closeRangeEditor();
            this.onClose();
          })
      }
      .width('100%')
      .padding({ left: 12, right: 8, top: 6, bottom: 2 })

      if (this.editingRange) {
        // 上游 InputDialogTableView 同款三字段（x起始值/x终止值/步长）。
        // 二十五包②：各字段挂自带键盘（按设置）+ 焦点切活动字段 + 编辑行
        // 几何上报宿主实测垫高（customKeyboard 浮层不参与避让）
        Row() {
          TextInput({ text: this.minText, controller: this.minCtrl })
            .fontSize(11).height(28).layoutWeight(1)
            .backgroundColor(Theme.inputBg(this.dark))
            .onChange((v: string) => { this.minText = v; })
            .onFocus(() => {
              this.rangeField = 0;
              this.rangeSelA = this.minText.length;
              this.rangeSelB = this.rangeSelA;
              this.focusEdit(true);
            })
            .onTextSelectionChange((s: number, e: number) => {
              if (this.rangeField === 0) {
                this.rangeSelA = s;
                this.rangeSelB = e;
              }
            })
            .customKeyboard(this.useSysKeyboard ? undefined : this.emptyKb())
          Text('起').fontSize(10).fontColor(Theme.sub(this.dark)).margin({ left: 3, right: 6 })
          TextInput({ text: this.maxText, controller: this.maxCtrl })
            .fontSize(11).height(28).layoutWeight(1)
            .backgroundColor(Theme.inputBg(this.dark))
            .onChange((v: string) => { this.maxText = v; })
            .onFocus(() => {
              this.rangeField = 1;
              this.rangeSelA = this.maxText.length;
              this.rangeSelB = this.rangeSelA;
              this.focusEdit(true);
            })
            .onTextSelectionChange((s: number, e: number) => {
              if (this.rangeField === 1) {
                this.rangeSelA = s;
                this.rangeSelB = e;
              }
            })
            .customKeyboard(this.useSysKeyboard ? undefined : this.emptyKb())
          Text('止').fontSize(10).fontColor(Theme.sub(this.dark)).margin({ left: 3, right: 6 })
          TextInput({ text: this.stepText, controller: this.stepCtrl })
            .fontSize(11).height(28).layoutWeight(1)
            .backgroundColor(Theme.inputBg(this.dark))
            .onChange((v: string) => { this.stepText = v; })
            .onFocus(() => {
              this.rangeField = 2;
              this.rangeSelA = this.stepText.length;
              this.rangeSelB = this.rangeSelA;
              this.focusEdit(true);
            })
            .onTextSelectionChange((s: number, e: number) => {
              if (this.rangeField === 2) {
                this.rangeSelA = s;
                this.rangeSelB = e;
              }
            })
            .customKeyboard(this.useSysKeyboard ? undefined : this.emptyKb())
          Text('步长').fontSize(10).fontColor(Theme.sub(this.dark)).margin({ left: 3, right: 6 })
          Text('确定')
            .fontSize(12).fontColor('#FFFFFF')
            .backgroundColor(Theme.accent(this.dark))
            .borderRadius(6).height(28)
            .padding({ left: 10, right: 10 })
            .onClick(() => this.applyRange())
        }
        .width('100%')
        .padding({ left: 12, right: 12, top: 2, bottom: 2 })
        .onAreaChange((_o: Area, n: Area) => {
          this.onEditArea(n);   // 编辑行几何 → 宿主实测垫高（二十五包②）
        })
      }

      if (this.rangeErr.length > 0) {
        Text(this.rangeErr)
          .fontSize(11)
          .fontColor(Theme.error(this.dark))
          .width('100%')
          .padding({ left: 12, right: 12, top: 2 })
      }

  // 表头（x + 各函数列；键含列名与主题，改名/换色即重建）
  Row() {
        ForEach(this.headerCells(), (c: string, i: number) => {
          Text(c)
            .fontSize(11)
            .fontWeight(FontWeight.Medium)
            .fontColor(this.dark ? '#A8ADB5' : '#6E6D73')
            .textAlign(TextAlign.End)
            .maxLines(1)
            .height(24)
            .padding({ left: 6, right: 6 })
            .layoutWeight(i === 0 ? 1 : 2)
            .backgroundColor(this.dark ? '#26292D' : '#F3F2F7')
        }, (c: string, i: number) => `${i}#${c}#${this.dark ? 'D' : 'L'}`)
      }
      .width('100%')

      if (this.cols.length === 0) {
        Text('在代数区输入函数（如 f(x)=2x+1）或直线后，这里显示对应数值表')
          .fontSize(12)
          .fontColor(Theme.faint(this.dark))
          .width('100%')
          .margin({ top: 18 })
          .textAlign(TextAlign.Center)
      }

      Scroll() {
        Column() {
          ForEach(this.rows, (row: string[]) => {
            TableRowView({ cells: row, dark: this.dark })
          }, (row: string[]) => row.join('#') + '#' + (this.dark ? 'D' : 'L'))
        }
        .width('100%')
      }
      .scrollBar(BarState.Auto)
      .layoutWeight(1)
      .align(Alignment.Top)
      .margin({ top: 4 })
    }
    .width('100%')
    .height('100%')
  }
}
