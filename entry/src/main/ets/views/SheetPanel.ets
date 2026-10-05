// SheetPanel.ets — 数据视图（底栏/宽屏左栏"数据"Tab，上游 Spreadsheet/数据区
// 同位）：A1 单元格模型（上游
// Spreadsheet 语义）。单元格地址 = 列字母 + 行号；点选单元格 → 选中态与
// 画布/代数区联动（有对象时），再次点选进入格内编辑（上游 桌面同款：编辑
// 器在单元格内，回车提交并下移一行），提交经宿主写内核（空内容删除单元
// 格对象，否则 A1=值/公式/命令）。单元格内容来自宿主的代数区列表（label
// 匹配单元格地址的对象即落在该格，数值/滑动条/角显示值，点显示坐标）。
// 行列数固定 A..H × 1..40，纵向滚动。
//
// 第二十一包③a（真机反馈"表格和 上游 差别很大，查源码还原"）：视觉/交互
// 逐项对齐 上游官方源码（上游 SpreadsheetStyling/SpreadsheetRenderer，EUPL 源码仅作行为参照、
// 不复制资产）：
// ① 配色 = 上游 常量原值——表头底 NEUTRAL_200(#F3F2F7)/字 NEUTRAL_700
//   (#6E6D73)，网格线 NEUTRAL_300(#E6E6EB)，选中格底 PURPLE_100(#F3F0FF)
//   + 边框 PURPLE_600(#6557D2)（深色取近似同系值）；
// ② 表头联动：选中格所在列字母/行号表头反色（NEUTRAL_700 底白字，上游
//   getSelectionHeaderColor 同位）；
// ③ 对齐 = 上游 DEFAULT_CELL_ALIGNMENT：数值右对齐、文本左对齐；
// ④ 编辑器入格：上游 SpreadsheetCellEditor 在单元格内编辑，回车提交并
//   下移一行（原底部输入条删除）；失焦同提交（焦点链路自动收口）。
// 注意：格子选中态在 ForEach itemGenerator 内联读取 this.selCell（@Builder
// 按值传参不参与状态刷新——轮 B 铁律，本组件不走 @Builder）；内层列不
// 用 ForEach、8 列字面渲染（真机丢项前科，第四包），改列需同步列头与
// 每行字面单元格两处（列字母 A..H 写死）。
import { AlgebraEntry } from '../model/GeoTypes';
import { Theme } from '../model/Theme';
import { MathKbHooks } from './MathKeyboard';

const N_ROWS: number = 40;

const ROW_NUMBERS: number[] = (() => {
  const out: number[] = [];
  for (let r = 1; r <= N_ROWS; ++r) {
    out.push(r);
  }
  return out;
})();

// ---- 上游 SpreadsheetStyling 配色（浅色取官方常量原值，深色同系近似）----
function hdrBg(dark: boolean): string {
  return dark ? '#26292D' : '#F3F2F7';       // NEUTRAL_200
}
function hdrFg(dark: boolean): string {
  return dark ? '#A8ADB5' : '#6E6D73';       // NEUTRAL_700
}
function gridCol(dark: boolean): string {
  return dark ? '#33363B' : '#E6E6EB';       // NEUTRAL_300
}
function selFill(dark: boolean): string {
  return dark ? 'rgba(101,87,210,0.28)' : '#F3F0FF';   // PURPLE_100
}
function selBorder(dark: boolean): string {
  return dark ? '#8B7FF0' : '#6557D2';       // PURPLE_600
}
function selHdrBg(dark: boolean): string {
  return dark ? '#3A3E45' : '#6E6D73';       // 选中表头反色（getSelectionHeaderColor）
}
function selHdrFg(dark: boolean): string {
  return dark ? '#E8EAED' : '#FFFFFF';
}

@Component
export struct SheetPanel {
  @Link algebra: AlgebraEntry[];    // 宿主代数区（单元格内容来源）
  @Prop dark: boolean = false;
  @Prop isWide: boolean = false;    // 宽屏左栏形态（布局参数预留，网格同构）
  // 二十四包②：键盘跟随设置（0=系统/1=自带，宿主 useSysKeyboard 下发），
  // 头部 ⌨ 切换；格内编辑器挂 customKeyboard（AlgebraPanel 同款）——此前
  // 格子编辑只有系统键盘一条路，自带键盘设置下弹系统键盘、面板位移，
  // 点已选格进编辑要快速按两次才稳（真机反馈②）
  @Prop useSysKeyboard: boolean = false;
  @State private selCell: string = '';    // 选中单元格（'' = 无）
  @State private editing: boolean = false;
  @State private input: string = '';
  @State private inputError: string = '';
  // 格内编辑器光标（自带键盘插入协议用；系统键盘模式由原生光标自理）
  @State private selA: number = 0;
  @State private selB: number = 0;
  private cellCtrl: TextInputController = new TextInputController();
  // 提交单元格（宿主 CanvasView.setCell：空=删除、其余写内核）。返回错误（空串=成功）
  onSetCell: (label: string, content: string) => string =
    (_l: string, _c: string) => '';
  // 单元格选中联动（有对象时画布选中特效；label 为地址，宿主自行查找）
  onSelectCell: (label: string) => void = (_l: string) => {};
  // 第二十三包②：单变量分析入口（上游 表格样式条 OneVar 分析工具同位）。
  // 参数 = 选区数值序列（逗号分隔）与来源描述（如 "B1:B8 · 6 个数值"）
  onShowAnalysis: (values: string, range: string) => void =
    (_v: string, _r: string) => {};
  // 二十四包②：头部 ⌨（切系统/自带键盘）与格内编辑态上报（宿主垫高）
  onToggleKeyboard: () => void = () => {};
  onEditFocus: (focused: boolean) => void = (_f: boolean) => {};
  // 三十六包：格内编辑会话期把面板键盘处理束注册到宿主（顶层键盘分发用；
  // null=会话结束注销）
  onKbHooks: (h: MathKbHooks | null) => void = (_h: MathKbHooks | null) => {};
  // 编辑格几何回报 → 宿主实测垫高（AlgebraPanel onInputArea 同机制复用）
  onEditArea: (n: Area) => void = (_n: Area) => {};
  onClose: () => void = () => {};

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
      h.submit = (): void => this.commitMoveDown();
      h.hide = (): void => {
        this.commit();
      };
      h.caretLeft = (): void => this.kbCaretTo(this.selA - 1);
      h.caretRight = (): void => this.kbCaretTo(this.selA + 1);
      this.hooksCache = h;
    }
    return this.hooksCache;
  }

  // 三十六包：浮层空壳——customKeyboard 挂载即抑制系统 IME，1px 透明壳
  // 的弹出/收起不可见；键盘本体在宿主页面顶层（CanvasView.mathKbOverlay），
  // 格间焦点转移不再「旧键盘收起+新键盘弹出」
  @Builder
  private emptyKb() {
    Row()
      .width('100%')
      .height(1)
      .opacity(0)
  }

  // 单元格对应的对象（label 完全匹配；不匹配返回 undefined）
  private entryOf(label: string): AlgebraEntry | undefined {
    return this.algebra.find((a: AlgebraEntry): boolean => a.label === label);
  }

  // 选中单元格的编辑预填：定义列去掉 "label = " 前缀（如 "B1 = A1+1" → "A1+1"）
  private prefillOf(label: string): string {
    const e = this.entryOf(label);
    if (!e) {
      return '';
    }
    const prefix = label + ' = ';
    return e.definition.startsWith(prefix) ? e.definition.substring(prefix.length)
      : e.definition;
  }

  // 数值判定（上游 DEFAULT_CELL_ALIGNMENT=ALIGN_RIGHT：数值右对齐、文本左）
  private isNum(v: string): boolean {
    if (v.length === 0) {
      return true;
    }
    return !isNaN(parseFloat(v));
  }

  // 格子边框：选中 = 上游 PURPLE_600 2px；普通 = 网格线 1px
  private cellBorder(label: string): string {
    return this.selCell === label ? selBorder(this.dark) : gridCol(this.dark);
  }

  private cellBg(label: string): ResourceColor {
    return this.selCell === label ? selFill(this.dark) : Color.Transparent;
  }

  // 选中格所在列/行的表头反色（上游 表头联动）
  private colSelected(letter: string): boolean {
    return this.selCell.length > 0 && this.selCell.startsWith(letter);
  }

  private rowSelected(r: number): boolean {
    if (this.selCell.length < 2) {
      return false;
    }
    return parseInt(this.selCell.substring(1)) === r;
  }

  private selectCell(label: string): void {
    this.selCell = label;
    this.editing = false;
    this.inputError = '';
    this.onSelectCell(this.entryOf(label) !== undefined ? label : '');
  }

  // 点单元格：已选中的再点 → 进入格内编辑；否则选中（编辑中先提交，失败留在原格）
  private tapCell(label: string): void {
    if (this.editing && !this.commit()) {
      return;   // 提交失败（错误已显示）不切换
    }
    if (this.selCell === label) {
      this.editing = true;
      this.input = this.prefillOf(label);
      this.selA = this.input.length;
      this.selB = this.selA;
      this.inputError = '';
      this.focusEdit(true);
      this.focusCellEditor();
    } else {
      this.selectCell(label);
    }
  }

  // 二十五包③：点已选格要连点三次才进编辑——编辑器 TextInput 是动态插入
  // 的，defaultFocus 只对首帧构建可靠，插入后拿不到焦点 → 无键盘无光标，
  // 第三次点按靠 focusOnTouch 才聚焦。修复：插入后 FocusController 显式
  // 抢焦（CanvasView 代数区同款模式），30ms 先试 + 150ms 重试兜底时序；
  // 系统键盘/自带键盘两模式同走焦点链路
  private focusCellEditor(): void {
    const grab = (): void => {
      try {
        this.getUIContext().getFocusController().requestFocus('sheetCellEditor');
      } catch (_e) {
        // 编辑器尚未挂载完成时忽略，由重试兜底
      }
    };
    setTimeout(grab, 30);
    setTimeout(grab, 150);
  }

  // ✓ 提交；成功收起编辑器。返回是否成功（失败错误留在 inputError）。
  // 编辑态守卫：回车提交后失焦会再触发一次 blur，二次进入直接放行，
  // 防止把已清空的 input 当"删除"写回内核
  private commit(): boolean {
    if (!this.editing) {
      return true;
    }
    if (this.selCell.length === 0) {
      this.editing = false;
      this.focusEdit(false);
      return true;
    }
    const err = this.onSetCell(this.selCell, this.input);
    if (err.length > 0) {
      this.inputError = err;
      return false;
    }
    this.editing = false;
    this.focusEdit(false);
    this.input = '';
    this.inputError = '';
    // 删除/重建后单元格内容可能变了，重新联动选中态
    this.onSelectCell(this.entryOf(this.selCell) !== undefined ? this.selCell : '');
    return true;
  }

  // 格内回车：提交后选中下移一行（上游 桌面 Enter 同款）
  private commitMoveDown(): void {
    const col = this.selCell.substring(0, 1);
    const row = parseInt(this.selCell.substring(1));
    if (!this.commit()) {
      return;
    }
    if (row < N_ROWS) {
      this.selectCell(col + (row + 1).toString());
    }
  }

  private cancelEdit(): void {
    this.editing = false;
    this.focusEdit(false);
    this.input = '';
    this.inputError = '';
  }

  // 第二十三包②：收集选中列的数值（上游 DataSource 快照选区同义；本表
  // 为单格选中，取该列全部非空数值格——上游 要求多格选区，此处按列下探
  // 是单格选择下的等价入口）。无数值时提示；有则回调宿主开分析面板
  private startAnalysis(): void {
    if (this.selCell.length === 0) {
      this.inputError = '先选中一个单元格，再点分析（收集该列数值）';
      return;
    }
    const col = this.selCell.substring(0, 1);
    const nums: number[] = [];
    let first = 0;
    let last = 0;
    for (let r = 1; r <= N_ROWS; ++r) {
      const e = this.entryOf(col + r.toString());
      const v = e ? parseFloat(e.value) : NaN;
      if (e !== undefined && e.value.length > 0 && !isNaN(v) && isFinite(v)) {
        if (nums.length === 0) {
          first = r;
        }
        last = r;
        nums.push(v);
      }
    }
    if (nums.length === 0) {
      this.inputError = `${col} 列没有数值，无法分析`;
      return;
    }
    this.inputError = '';
    const range = `${col}${first}:${col}${last} · ${nums.length} 个数值`;
    this.onShowAnalysis(nums.join(','), range);
  }

  // ---- 二十四包②：自带键盘（挂格内编辑器 customKeyboard，AlgebraPanel 同款）----

  // 光标就位（插入口径与 TextInput 原生光标同步）
  private kbCaretTo(i: number): void {
    const v = Math.max(0, Math.min(i, this.input.length));
    this.selA = v;
    this.selB = v;
    try {
      this.cellCtrl.caretPosition(v);
    } catch (_e) {
      // 焦点未就绪时忽略；下一次键入按本状态继续
    }
  }

  // 当前选区 [a, b]（越界/倒序收敛）
  private kbSelRange(): number[] {
    let a = this.selA;
    let b = this.selB;
    if (a < 0 || b < 0 || a > this.input.length || b > this.input.length) {
      a = this.input.length;
      b = a;
    }
    if (b < a) {
      const t = a;
      a = b;
      b = t;
    }
    return [a, b];
  }

  // 插入协议（简化版）：格子是纯文本、无角标结构——skip（模板跳出）保留、
  // back 负值哨兵（□_□ 智能下标）退化为普通退格
  private kbInsert(t: string, back: number, skip: string): void {
    const rg = this.kbSelRange();
    const a = rg[0];
    const b = rg[1];
    if (skip.length > 0 && a === b && a + skip.length <= this.input.length
      && this.input.substring(a, a + skip.length) === skip) {
      this.kbCaretTo(a + skip.length);
      return;
    }
    const del = Math.min(back < 0 ? 1 : back, a);
    this.input = this.input.substring(0, a - del) + t + this.input.substring(b);
    this.kbCaretTo(a - del + t.length);
  }

  private kbBackspace(): void {
    this.kbInsert('', 1, '');
  }

  // 列头字母（A..H 字面渲染——内层列 ForEach 有真机丢项前科，见文件头）
  @Builder
  private colHdr(letter: string) {
    Text(letter)
      .fontSize(11)
      .fontWeight(FontWeight.Medium)
      .fontColor(this.colSelected(letter) ? selHdrFg(this.dark) : hdrFg(this.dark))
      .textAlign(TextAlign.Center)
      .layoutWeight(1)
      .height(22)
      .backgroundColor(this.colSelected(letter) ? selHdrBg(this.dark) : hdrBg(this.dark))
  }

  // 数据格：内容 + 选中底色 + 格内编辑器（编辑该格时 TextInput 覆叠，
  // 上游 SpreadsheetCellEditor 同位）。八列共用（字面渲染调用）；内容在
  // Builder 内经 this.entryOf 自取——@Builder 按值传参不刷新（轮 B 铁律），
  // 地址串是常量可传，格值不能传
  @Builder
  private cell(addr: string) {
    Stack() {
      if (!(this.editing && this.selCell === addr)) {
        Text(this.entryOf(addr)?.value ?? '')
          .fontSize(12)
          .fontColor(this.dark ? '#E8EAED' : '#1C1C1F')   // 上游 默认字色（NEUTRAL_900 系）
          .maxLines(1)
          .width('100%')
          .height(28)
          .textAlign(this.isNum(this.entryOf(addr)?.value ?? '') ? TextAlign.End : TextAlign.Start)
          .padding({ left: 4, right: 4 })
      } else {
        TextInput({ text: this.input, controller: this.cellCtrl })
          .fontSize(11)
          .height(26)
          .width('100%')
          .padding({ left: 3, right: 3 })
          .borderRadius(2)
          .backgroundColor(Theme.inputBg(this.dark))
          .id('sheetCellEditor')   // 二十五包③：FocusController 显式抢焦锚点
          .onChange((v: string) => {
            this.input = v;
          })
          .onTextSelectionChange((s: number, e: number) => {
            this.selA = s;
            this.selB = e;
          })
          .onAreaChange((_o: Area, n: Area) => {
            this.onEditArea(n);   // 编辑格几何 → 宿主实测垫高（二十四包②）
          })
          .onSubmit(() => this.commitMoveDown())
          .onBlur(() => this.commit())
          // 自带键盘设置下挂数学键盘（否则解绑、聚焦弹系统键盘）——
          // customKeyboard 是浮层不参与避让，垫高由宿主按上报几何收敛
          .customKeyboard(this.useSysKeyboard ? undefined : this.emptyKb())
      }
    }
    .layoutWeight(1)
    .height(28)
    .alignContent(Alignment.Center)
    .border({ width: this.selCell === addr ? 2 : 1, color: this.cellBorder(addr) })
    .backgroundColor(this.cellBg(addr))
    .onClick(() => this.tapCell(addr))
  }

  build() {
    Column() {
      Row() {
        // 第二十二包①：标题随 Tab 归位——上游 语言包 Perspective.Spreadsheet
        // 译文即「数据」（本视图在导航栏的名字），面板标题同改
        Text('数据')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.text(this.dark))
          .layoutWeight(1)
        Text(this.algebra.length > 0 ? `${this.algebra.length} 个对象` : '点单元格输入数值或公式')
          .fontSize(11)
          .fontColor(Theme.faint(this.dark))
          .layoutWeight(1)
          .maxLines(1)
        // 二十四包②：⌨ 切系统/自带键盘（AlgebraPanel 头部同款，accent=自带生效）
        Text('⌨')
          .fontSize(16)
          .fontColor(this.useSysKeyboard ? Theme.faint(this.dark) : Theme.accent(this.dark))
          .width(30)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.onToggleKeyboard())
        // 第二十三包②：单变量分析入口（上游 表格样式条 OneVar 分析工具同位；
        // 无选中时半透明，点击给提示）
        Text('📊')
          .fontSize(14)
          .opacity(this.selCell.length > 0 ? 1 : 0.4)
          .width(30)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.startAnalysis())
        Text('✕')
          .fontSize(14)
          .fontColor(Theme.sub(this.dark))
          .width(30)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.onClose())
      }
      .width('100%')
      .padding({ left: 12, right: 8, top: 6, bottom: 2 })

      // 列头（固定，不随网格滚动；A..H 字面渲染，选中列反色联动）
      Row() {
        Text('')
          .width(28)
          .height(22)
          .backgroundColor(hdrBg(this.dark))
        this.colHdr('A')
        this.colHdr('B')
        this.colHdr('C')
        this.colHdr('D')
        this.colHdr('E')
        this.colHdr('F')
        this.colHdr('G')
        this.colHdr('H')
      }
      .width('100%')

      // 单元格网格：首列行号 + 8 列数据，纵向滚动。行 ForEach 保留（真机
      // 验证 40 行全渲染），行内 8 个单元格经 cell() @Builder 字面渲染
      //（传值参数为常量地址串，选中态读 @State 的地方都在 cell 内联，
      // 不踩"@Builder 按值传参不刷新"——刷新由 this.selCell 触发整行重建）
      Scroll() {
        Column() {
          ForEach(ROW_NUMBERS, (r: number) => {
            Row() {
              Text(r.toString())
                .fontSize(10)
                .fontWeight(FontWeight.Medium)
                .fontColor(this.rowSelected(r) ? selHdrFg(this.dark) : hdrFg(this.dark))
                .width(28)
                .height(28)
                .textAlign(TextAlign.Center)
                .backgroundColor(this.rowSelected(r) ? selHdrBg(this.dark) : hdrBg(this.dark))
              this.cell('A' + r)
              this.cell('B' + r)
              this.cell('C' + r)
              this.cell('D' + r)
              this.cell('E' + r)
              this.cell('F' + r)
              this.cell('G' + r)
              this.cell('H' + r)
            }
            .width('100%')
          }, (r: number) => 'row' + r + '#' + (this.dark ? 'D' : 'L'))
        }
        .width('100%')
      }
      .scrollBar(BarState.Auto)
      .layoutWeight(1)
      .align(Alignment.Top)

      // 错误条（提交失败时）
      if (this.inputError.length > 0) {
        Text(this.inputError)
          .fontSize(11)
          .fontColor(Theme.error(this.dark))
          .width('100%')
          .padding({ left: 12, right: 12, top: 2 })
          .maxLines(2)
      }

      if (!this.editing && this.selCell.length > 0) {
        // 未编辑态：提示再点一次进入格内编辑（上游 桌面为双击/直接键入）
        Text(`${this.selCell}：再点一次在格内编辑，回车提交并下移一行`)
          .fontSize(11)
          .fontColor(Theme.faint(this.dark))
          .width('100%')
          .padding({ left: 12, right: 12, top: 2, bottom: 6 })
      }
    }
    .width('100%')
    .height('100%')
  }
}
