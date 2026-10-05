// MathKeyboard.ets — 代数输入的数学虚拟键盘（ArkUI 自绘键位）。
// 2026-09-30 第七包：布局逐键对齐 上游官方 keyboard-base 工厂源码
//（MathKeyboardFactory / FunctionKeyboardFactory / NumberKeyUtil /
// LetterKeyboardFactory.java），键帽图标的视觉方案同 上游官方 SVG 图标
//（icons/svg/web/keyboard/*.svg = 2×2 点阵虚线方框占位 + 实心笔画）——
// 自绘虚线边框小方框 + 文本笔画实现同视觉，不复制 上游 图标资产（合规）。
// 2026-09-30 第八包（真机反馈）：①x² 键帽改 虚线框^2（上游 POWA2 同构）；
// ②√□ / ⁿ√□ 键帽占位框上去掉横线；③□ₙ 键帽改 虚线框_虚线框；
// ④abc 页控制行最左加 αβγ 键（上游 SWITCH_TO_GREEK_CHARACTERS 同位），
// 切希腊字母副键盘（GreekKeyboardFactory 行序），⇧ 切大小写，切走自动回英文；
// ⑤页序对齐官方 123→f(x)→abc→#&¬ 并新增 #&¬ 页
//（SpecialSymbolsKeyboardFactory 无模板键分支；⌊⌋⌈⌉接内核 Floor/Ceil，
// 货币键用 $ 配合 $A$1 引用；逻辑/集合符号内核语义待后续）；
// ⑥删除 abc 页上方命令快捷条（工具 Tab 已覆盖其职责）。
// 2026-10-01 第十包（真机反馈）：①□_□ 键帽去掉 "_" 字形——第二个虚线框
// 直接作第一个框的右下角标（上游 A_N SVG 同构）；②光标升级框内/框外双态
//（上游 editor-base CursorController/InputController 语义线性移植，见
// CanvasView/MathField），←/→/⌫/键入在角标区内外的行为对齐 上游。
// 2026-10-01 第九包（真机反馈）：①□_□ 改插空下标模板 '_'（不预填 a/n，
// 第二个框为第一个框的角标；back=-1 哨兵 → CanvasView 智能落位：光标前
// 紧接标识符尾 → 光标落下标位，否则落 base 框先填底）；②←/→ 与 ⌫ 升级
// 为结构感知（槽位跳转进/出角标区、脚本内容级联删除、函数模板拆除），
// 实现在 MathField.ets 编辑模型导出 + CanvasView.ets。
//
// 123 页（MathKeyboardFactory 复刻）：
//   x y π e | 789×÷
//   □² ^□ √□ |□| | 456+−        （模板键键帽=虚线框占位符）
//   < > □̄ 带分数□□/□ | 123=⌫
//   ans , ( ) | 0.←→↵
// f(x) 页（FunctionKeyboardFactory 复刻）：sin cos tan | % ! $ ° /
//   sin⁻¹ cos⁻¹ tan⁻¹ | { } ≤ ≥ / ln log log□ | d□/d□ ∫□d□ i ⌫ /
//   e^□ 10^□ ⁿ√□ | □_□ ← → ↵
// abc 页（LetterKeyboardFactory 复刻）：qwerty 行 + ⇧ zxcvbnm ⌫ +
//   αβγ ,() 空格 ←→↵；希腊副键盘（GreekKeyboardFactory 复刻）：
//   ϕςερτυθιοπ / ασδφγηξκλ / ζχψωβνμ + ⇧行 + ABC ,() 空格 ←→↵。
// #&¬ 页（SpecialSymbolsKeyboardFactory 复刻）：∞ ≇ ≠ ∧ ∨ → ¬ ⊗ /
//   ∥ ⊥ ∈ ⊂ ⊆ ∠ ⌊⌋ ⌈⌉ / [ ] : & @ # $ ⌫ / ; ' " ′ ″ ←→↵。
//
// 插入语义（上游 同款）：函数/括号键自动带右括号、光标落在参数框内；
// ）键在已有右括号前跳过不重复插入；模板键插入线性 token 由内核预解析
// （√□→sqrt()、|□|→abs()、□̄→0.()、带分数→" /"、□²→^2、ⁿ√□→NthRoot()、
// □_□→_（空下标模板，第九包①）、⌊⌋→Floor()、⌈⌉→Ceil()）。
// onInsert(t, back, skip)：back=插入后光标从串尾回退字符数（-1=□_□ 智能
// 落位哨兵）；skip=光标处已是该串时仅右移光标。深色配色跟随系统，按压
// 反馈 stateStyles。

// 键盘总高（vp）：第八包 248→220（删 abc 命令条后四页统一）。第十七包②
// 起导出为常量——CanvasView 键盘避让目标（输入行贴键盘上沿）以此为基准，
// 不再各处硬编码
export const MATH_KB_HEIGHT = 220;

// 第二十一包②：宽屏（PC UI）键盘本体最大宽度（vp）——上游 桌面虚拟键盘
// 同款：浮层全宽但键盘居中、两侧大面积留白，键帽不随窗口拉宽
//（CanvasView.mathKbOverlay 消费；窄屏仍 100% 全宽）
export const KB_WIDE_MAX_WIDTH = 520;

// 三十六包：面板键盘处理函数束。浮层空壳化后键盘本体唯一化到宿主页面
// 顶层（CanvasView.mathKbOverlay）；表格/数据等键盘逻辑在面板私有上下文
//（范围字段/单元格状态），由面板在编辑会话期把本束注册到宿主，顶层键盘
// 按活跃上下文分发；滑动条/代数输入行的逻辑本就在宿主，直连。
export class MathKbHooks {
  insert: (t: string, back: number, skip: string) => void =
    (_t: string, _b: number, _s: string) => {};
  backspace: () => void = () => {};
  submit: () => void = () => {};
  hide: () => void = () => {};
  caretLeft: () => void = () => {};
  caretRight: () => void = () => {};
}

interface KeySpec {
  label: string;      // 键面文本（cap 非空时仅作无障碍备用）
  token: string;      // 兼容字段：未配 ins 时插入的文本
  w: number;          // 行内 layoutWeight 占比（上游 同款 0.2 间隔列）
  special?: string;   // 'bs' 'enter' 'shift' 'left' 'right' 'gap' 'greek'
  cap?: string;       // 富键帽样式 id（虚线框模板键）
  ins?: string;       // 插入串（可含自动补齐的右括号）
  back?: number;      // 插入后光标回退字符数（落在参数框内；-1=□_□ 智能落位）
  skip?: string;      // 光标处已存在该串时仅右移（）键跳过）
}

@Component
export struct MathKeyboard {
  onInsert: (t: string, back: number, skip: string) => void =
    (_t: string, _back: number, _skip: string) => {};
  onBackspace: () => void = () => {};
  onSubmit: () => void = () => {};
  onHide: () => void = () => {};
  onCaretLeft: () => void = () => {};
  onCaretRight: () => void = () => {};
  // 上游 同款：键盘上的"系统键盘"切换键（⌨），切到系统输入法
  onSwitchSystem: () => void = () => {};
  // 页序对齐 上游官方：123 → f(x) → abc → #&¬（第八包真机反馈⑤）
  @State private tab: number = 0;             // 0=123 1=f(x) 2=abc 3=#&¬
  @State private upperCase: boolean = false;  // abc/希腊页大小写（上游 惯例点大写需 shift）
  @State private greekMode: boolean = false;  // abc 页内希腊副键盘（上游 SWITCH_TO_GREEK）
  @StorageProp('ui.dark') private dark: boolean = false;
  private tabNames: string[] = ['123', 'f(x)', 'abc', '#&¬'];

  // ---- 深浅两套配色 ----
  private kbBg(): string {
    return this.dark ? '#202226' : '#E3E6EC';
  }

  private tabBg(active: boolean): string {
    if (this.dark) {
      return active ? '#2C4A77' : '#3A3E45';
    }
    return active ? '#CBDDFF' : '#DADDE5';
  }

  private tabFg(active: boolean): string {
    if (this.dark) {
      return active ? '#8FB6FF' : '#A8ADB5';
    }
    return active ? '#0050B8' : '#666666';
  }

  // 键帽笔画/虚线框描边色（上游 SVG 图标为单色黑，这里跟随深浅主题）
  private ink(): string {
    return this.dark ? '#C9CDD3' : '#4A4E55';
  }

  private capBg(k: KeySpec): string {
    if (k.special === 'gap') {
      return Color.Transparent.toString();
    }
    if (k.special === 'shift' && this.upperCase) {
      return this.dark ? '#3D5A8F' : '#BFD4FF';
    }
    if (k.special !== undefined) {
      return this.dark ? '#3E424A' : '#D0D4DE';
    }
    return this.dark ? '#33363B' : '#FFFFFF';
  }

  private capFg(): string {
    return this.dark ? '#E8EAED' : '#333333';
  }

  private press(k: KeySpec): void {
    if (k.special === 'bs') {
      this.onBackspace();
      return;
    }
    if (k.special === 'enter') {
      this.onSubmit();
      return;
    }
    if (k.special === 'shift') {
      this.upperCase = !this.upperCase;
      return;
    }
    if (k.special === 'greek') {
      // abc 页 αβγ ↔ ABC 副键盘切换（上游 SWITCH_TO_GREEK_CHARACTERS / SWITCH_TO_ABC）
      this.greekMode = !this.greekMode;
      return;
    }
    if (k.special === 'left') {
      this.onCaretLeft();
      return;
    }
    if (k.special === 'right') {
      this.onCaretRight();
      return;
    }
    if (k.special === 'gap') {
      return;
    }
    if (k.ins !== undefined) {
      this.onInsert(k.ins, k.back === undefined ? 0 : k.back,
        k.skip === undefined ? '' : k.skip);
      return;
    }
    this.onInsert(k.token.length > 0 ? k.token : k.label, 0, '');
  }

  private k(label: string, token: string = '', w: number = 1): KeySpec {
    return { label: label, token: token, w: w };
  }

  // 模板/函数键：ins 插入串 + back 光标回退（落在参数框内）
  private t(label: string, cap: string, ins: string, back: number, w: number = 1): KeySpec {
    return { label: label, token: '', w: w, cap: cap, ins: ins, back: back };
  }

  // 纯插入键（无富键帽）：括号自动补齐 / ）键跳过等
  private it(label: string, ins: string, back: number, skip: string = '', w: number = 1): KeySpec {
    return { label: label, token: '', w: w, ins: ins, back: back, skip: skip };
  }

  private sp(label: string, special: string, w: number = 1): KeySpec {
    return { label: label, token: '', w: w, special: special };
  }

  // 窄间隔列（上游 每行左簇与数字键盘之间 0.2 宽空位）
  private gap(w: number = 0.2): KeySpec {
    return { label: '', token: '', w: w, special: 'gap' };
  }

  // ---- 123 页：上游 MathKeyboardFactory 逐键复刻 ----
  // 行结构：4 键左簇 + 0.2 间隔 + 5 键数字键盘（789×÷/456+−/123=⌫/0.←→↵）
  private rows123(): KeySpec[][] {
    return [
      [this.k('x'), this.k('y'), this.k('π'), this.k('e'), this.gap(),
        this.k('7'), this.k('8'), this.k('9'), this.k('×', '*'), this.k('÷', '/')],
      [this.t('x²', 'x2', '^2', 0), this.t('^□', 'pow', '^', 0),
        this.t('√□', 'sqrt', 'sqrt()', 1), this.t('|□|', 'abs', 'abs()', 1),
        this.gap(),
        this.k('4'), this.k('5'), this.k('6'), this.k('+'), this.k('−', '-')],
      [this.k('<'), this.k('>'), this.t('□̄', 'rec', '0.()', 1),
        this.t('□□/□', 'mixed', ' /', 1),
        this.gap(),
        this.k('1'), this.k('2'), this.k('3'), this.k('='), this.sp('⌫', 'bs')],
      [this.t('ans', 'ans', 'ans', 0), this.k(','),
        this.it('(', '()', 1), this.it(')', ')', 0, ')'),
        this.gap(),
        this.k('0'), this.k('.'), this.sp('←', 'left'), this.sp('→', 'right'),
        this.sp('↵', 'enter')]
    ];
  }

  // ---- abc 页 / 希腊副键盘：上游 LetterKeyboardFactory / GreekKeyboardFactory ----
  // qwerty 三行（第二行两侧 0.5 补位）+ ⇧行（1.2 宽 ⇧/⌫、0.3 补位）+
  // 控制行 αβγ键(1.2) ,() 空格(3) ←→↵。αβγ 键在控制行最左（"，"左边，
  // 第八包真机反馈④ = 上游 ACTION_GREEK_LETTERS 同位），切到希腊字母；
  // 希腊行序抄 GreekKeyboardFactory 源码，⇧ 大小写同英文键盘。
  private rowsAbc(): KeySpec[][] {
    const up = this.upperCase;
    const row = (s: string): KeySpec[] => {
      const out: KeySpec[] = [];
      for (const c of s) {
        out.push(this.k(up ? c.toUpperCase() : c));
      }
      return out;
    };
    const greek = this.greekMode;
    // GreekKeyboardFactory 行序：ϕςερτυθιοπ / ασδφγηξκλ / ζχψωβνμ
    const top = greek ? 'ϕςερτυθιοπ' : 'qwertyuiop';
    const mid = greek ? 'ασδφγηξκλ' : 'asdfghjkl';
    const bot = greek ? 'ζχψωβνμ' : 'zxcvbnm';
    return [
      row(top),
      [this.gap(0.5)].concat(row(mid)).concat([this.gap(0.5)]),
      [this.sp('⇧', 'shift', 1.2), this.gap(0.3)].concat(row(bot))
        .concat([this.gap(0.3), this.sp('⌫', 'bs', 1.2)]),
      [this.sp(greek ? 'ABC' : 'αβγ', 'greek', 1.2), this.k(','),
        this.it('(', '()', 1), this.it(')', ')', 0, ')'),
        this.k(' ', ' ', 3), this.sp('←', 'left'), this.sp('→', 'right'),
        this.sp('↵', 'enter')]
    ];
  }

  // ---- f(x) 页：上游 FunctionKeyboardFactory 逐键复刻 ----
  // 左簇三键 5/3 宽 + 0.2 间隔 + 右簇；Derivative/Integral 内核命令已支持；
  // log□ 键插入 log(,)（上游 Log(底数, 值)，内核第七包对齐）
  private rowsFn(): KeySpec[][] {
    const inv = (name: string, fn: string): KeySpec => {
      return { label: name + '⁻¹', token: '', w: 1.7, cap: 'inv' + name,
        ins: fn + '()', back: 1 };
    };
    return [
      [this.t('sin', 'fn', 'sin()', 1, 1.7), this.t('cos', 'fn', 'cos()', 1, 1.7),
        this.t('tan', 'fn', 'tan()', 1, 1.7),
        this.gap(), this.k('%'), this.k('!'), this.k('$'), this.k('°')],
      [inv('sin', 'asin'), inv('cos', 'acos'), inv('tan', 'atan'),
        this.gap(), this.k('{'), this.k('}'), this.k('≤'), this.k('≥')],
      [this.t('ln', 'fn', 'ln()', 1, 1.7), this.t('log', 'fn', 'log()', 1, 1.7),
        this.t('log□', 'logb', 'log(,)', 3, 1.7),
        this.gap(), this.t('d□/d□', 'deriv', 'Derivative()', 1),
        this.t('∫□d□', 'integral', 'Integral()', 1), this.k('i'),
        this.sp('⌫', 'bs')],
      [this.t('e^□', 'epow', 'e^', 0, 1.7), this.t('10^□', 'pow10', '10^', 0, 1.7),
        this.t('ⁿ√□', 'nthroot', 'NthRoot()', 1, 1.7),
        this.gap(), this.t('□_□', 'an', '_', -1),
        this.sp('←', 'left'), this.sp('→', 'right'), this.sp('↵', 'enter')]
    ];
  }

  // ---- #&¬ 页：上游 SpecialSymbolsKeyboardFactory（无模板键分支）逐键复刻 ----
  // ⌊⌋ ⌈⌉ 接内核 Floor/Ceil 命令（第八包新增，光标落参数框内）；货币键用
  // $ 配合 $A$1 单元格引用；∞ ≇ ≠ ∧ ∨ → ¬ ⊗ ∥ ⊥ ∈ ⊂ ⊆ ∠ 等先照官方插入，
  // 逻辑/集合类符号的内核语义（布尔/集合对象体系）列入后续里程碑。
  private rowsSym(): KeySpec[][] {
    return [
      [this.k('∞'), this.k('≇'), this.k('≠'), this.k('∧'), this.k('∨'),
        this.k('→'), this.k('¬'), this.k('⊗')],
      [this.k('∥'), this.k('⊥'), this.k('∈'), this.k('⊂'), this.k('⊆'),
        this.k('∠'), this.t('⌊⌋', 'floor', 'Floor()', 1),
        this.t('⌈⌉', 'ceil', 'Ceil()', 1)],
      [this.k('['), this.k(']'), this.k(':'), this.k('&'), this.k('@'),
        this.k('#'), this.k('$'), this.sp('⌫', 'bs')],
      [this.k(';'), this.k("'"), this.k('"'), this.k('′'), this.k('″'),
        this.sp('←', 'left'), this.sp('→', 'right'), this.sp('↵', 'enter')]
    ];
  }

  private currentRows(): KeySpec[][] {
    if (this.tab === 1) {
      return this.rowsFn();
    }
    if (this.tab === 2) {
      return this.rowsAbc();
    }
    if (this.tab === 3) {
      return this.rowsSym();
    }
    return this.rows123();
  }

  // ---- 富键帽：上游 SVG 图标同视觉（点阵虚线框占位 + 实心笔画） ----

  @Builder
  private dottedBox(sz: number) {
    Column()
      .width(sz)
      .height(sz)
      .border({ width: 1, color: this.ink(), style: BorderStyle.Dashed, radius: 2 })
  }

  @Builder
  private stroke(w: number) {
    Column()
      .width(w)
      .height(1.5)
      .backgroundColor(this.ink())
  }

  // 通用键帽内容（cap 为空时退回文本）
  @Builder
  private keyFace(k: KeySpec) {
    if (k.cap === 'x2') {
      // 虚线框^2（上游 POWA2 键帽同构：占位框 + 右上标 2；第八包真机反馈①）
      Row({ space: 1 }) {
        this.dottedBox(9)
        Text('2').fontSize(10).fontColor(this.capFg()).offset({ y: -3 })
      }
      .alignItems(VerticalAlign.Top)
    } else if (k.cap === 'pow') {
      Row({ space: 1 }) {
        Column().width(10).height(10).border({ width: 1, color: this.ink(), style: BorderStyle.Dashed, radius: 2 }).offset({ y: 3 })
        Column().width(8).height(8).border({ width: 1, color: this.ink(), style: BorderStyle.Dashed, radius: 2 })
      }
      .alignItems(VerticalAlign.Top)
    } else if (k.cap === 'sqrt') {
      // √ + 虚线框（第八包真机反馈②：占位框上不加横线，√ 字形自带顶钩）
      Row({ space: 1 }) {
        Text('√').fontSize(15).fontColor(this.capFg())
        this.dottedBox(9)
      }
      .alignItems(VerticalAlign.Bottom)
    } else if (k.cap === 'abs') {
      Row({ space: 1 }) {
        Text('|').fontSize(13).fontColor(this.capFg())
        this.dottedBox(9)
        Text('|').fontSize(13).fontColor(this.capFg())
      }
      .alignItems(VerticalAlign.Center)
    } else if (k.cap === 'rec') {
      // 循环小数：实心顶横杠 + 虚线框（上游 recurring_decimal.svg 同构）
      Column({ space: 1 }) {
        this.stroke(13)
        this.dottedBox(9)
      }
      .alignItems(HorizontalAlign.Center)
    } else if (k.cap === 'mixed') {
      // 带分数：整数框 + (分子框/横杠/分母框)（上游 mixed_number.svg 同构）
      Row({ space: 2 }) {
        this.dottedBox(8)
        Column({ space: 1 }) {
          this.dottedBox(7)
          this.stroke(13)
          this.dottedBox(7)
        }
        .alignItems(HorizontalAlign.Center)
      }
      .alignItems(VerticalAlign.Center)
    } else if (k.cap === 'deriv') {
      // d□/d□ 分式模板（上游 derivative.svg 同构）
      Column({ space: 1 }) {
        Row({ space: 1 }) {
          Text('d').fontSize(9).fontColor(this.capFg())
          this.dottedBox(5)
        }
        .alignItems(VerticalAlign.Center)
        this.stroke(14)
        Row({ space: 1 }) {
          Text('d').fontSize(9).fontColor(this.capFg())
          this.dottedBox(5)
        }
        .alignItems(VerticalAlign.Center)
      }
      .alignItems(HorizontalAlign.Center)
    } else if (k.cap === 'integral') {
      Row({ space: 1 }) {
        Text('∫').fontSize(14).fontColor(this.capFg())
        this.dottedBox(7)
        Text('d').fontSize(9).fontColor(this.capFg())
        this.dottedBox(5)
      }
      .alignItems(VerticalAlign.Center)
    } else if (k.cap === 'nthroot') {
      // 指数小框 + √ + 虚线框（无横线，同 √ 键；第八包真机反馈②）
      Row({ space: 0 }) {
        Column().width(5).height(5).border({ width: 1, color: this.ink(), style: BorderStyle.Dashed, radius: 1 }).offset({ y: -8 })
        Text('√').fontSize(15).fontColor(this.capFg())
        this.dottedBox(9)
      }
      .alignItems(VerticalAlign.Bottom)
    } else if (k.cap === 'epow') {
      Row({ space: 0 }) {
        Text('e').fontSize(13).fontColor(this.capFg())
        Column().width(7).height(7).border({ width: 1, color: this.ink(), style: BorderStyle.Dashed, radius: 2 })
      }
      .alignItems(VerticalAlign.Top)
    } else if (k.cap === 'pow10') {
      Row({ space: 0 }) {
        Text('10').fontSize(12).fontColor(this.capFg())
        Column().width(7).height(7).border({ width: 1, color: this.ink(), style: BorderStyle.Dashed, radius: 2 })
      }
      .alignItems(VerticalAlign.Top)
    } else if (k.cap === 'an') {
      // □₍₎（第十包①：第二个虚线框直接作第一个框的右下角标，不再画 "_"，
      // 上游 A_N SVG 图标同构；上游 编辑器把键入的 a_n 解析为下标结构，
      // 本应用空模板插 '_'，光标由 CanvasView 智能落位）
      Row({ space: 1 }) {
        this.dottedBox(9)
        Column() {
          this.dottedBox(6)
        }
        .offset({ y: 2 })
      }
      .alignItems(VerticalAlign.Bottom)
    } else if (k.cap === 'floor' || k.cap === 'ceil') {
      // ⌊□⌋ / ⌈□⌉（#&¬ 页 Floor/Ceil 命令键，第八包新增）
      Row({ space: 1 }) {
        Text(k.cap === 'floor' ? '⌊' : '⌈').fontSize(13).fontColor(this.capFg())
        this.dottedBox(8)
        Text(k.cap === 'floor' ? '⌋' : '⌉').fontSize(13).fontColor(this.capFg())
      }
      .alignItems(VerticalAlign.Center)
    } else if (k.cap === 'logb') {
      Row({ space: 0 }) {
        Text('log').fontSize(12).fontColor(this.capFg())
        Row() {
          this.dottedBox(6)
        }
        .offset({ y: 3 })
      }
      .alignItems(VerticalAlign.Bottom)
    } else if (k.cap === 'invsin' || k.cap === 'invcos' || k.cap === 'invtan') {
      Row({ space: 0 }) {
        Text(k.cap === 'invsin' ? 'sin' : k.cap === 'invcos' ? 'cos' : 'tan')
          .fontSize(11).fontColor(this.capFg())
        Text('-1').fontSize(8).fontColor(this.capFg()).offset({ y: -4 })
      }
      .alignItems(VerticalAlign.Bottom)
    } else if (k.cap === 'fn') {
      Text(k.label).fontSize(11).fontColor(this.capFg())
    } else if (k.cap === 'ans') {
      Text('ans').fontSize(12).fontColor(this.capFg())
    } else {
      Text(k.label)
        .fontSize(k.label.length > 2 ? 12 : 17)
        .fontColor(this.capFg())
    }
  }

  build() {
    Column() {
      // 页签行（最左 ⌨ = 切换到系统键盘，上游 同款入口）
      Row({ space: 8 }) {
        Text('⌨')
          .fontSize(15)
          .fontColor(this.dark ? '#A8ADB5' : '#555555')
          .padding({ left: 10, right: 10, top: 4, bottom: 4 })
          .borderRadius(12)
          .backgroundColor(this.dark ? '#3E424A' : '#D0D4DE')
          .stateStyles({
            pressed: {
              .backgroundColor(this.dark ? '#4E5866' : '#C2C8D4')
            }
          })
          .onClick(() => {
            this.onSwitchSystem();
          })
        ForEach(this.tabNames, (name: string, idx: number) => {
          Text(name)
            .fontSize(13)
            .fontColor(this.tabFg(this.tab === idx))
            .fontWeight(this.tab === idx ? FontWeight.Medium : FontWeight.Normal)
            .padding({ left: 12, right: 12, top: 4, bottom: 4 })
            .borderRadius(12)
            .backgroundColor(this.tabBg(this.tab === idx))
            .stateStyles({
              pressed: {
                .backgroundColor(this.dark ? '#4E5866' : '#C2C8D4')
              }
            })
            .onClick(() => {
              // 切到别的页再切回 abc 默认英文键盘（上游 同款，真机反馈④）
              this.tab = idx;
              this.greekMode = false;
            })
        }, (name: string) => name)
        Blank()
        Text('完成')
          .fontSize(13)
          .fontColor(this.dark ? '#8FB6FF' : '#007DFF')
          .padding({ left: 12, right: 4, top: 4, bottom: 4 })
          .onClick(() => {
            this.onHide();
          })
      }
      .width('100%')
      .padding({ bottom: 4 })

      // 按键区：行距 3 与键间水平距一致；各页均 4 行
      Column({ space: 3 }) {
        ForEach(this.currentRows(), (row: KeySpec[], ri: number) => {
          Row({ space: 3 }) {
            ForEach(row, (k: KeySpec, ki: number) => {
              Column() {
                this.keyFace(k)
              }
              .justifyContent(FlexAlign.Center)
              .height(38)
              .layoutWeight(k.w)
              .backgroundColor(this.capBg(k))
              .stateStyles({
                pressed: {
                  .backgroundColor(this.dark ? '#4E5866' : '#C2C8D4')
                }
              })
              .borderRadius(6)
              .onClick(() => this.press(k))
            }, (k: KeySpec, ki: number) => ki.toString() + '|' + (k.cap ?? k.label)
              + '|' + k.w.toString())
          }
          .width('100%')
        }, (row: KeySpec[], ri: number) => ri.toString() + '|' + this.tab.toString()
          + (this.tab === 2 ? (this.upperCase ? 'U' : 'L') + (this.greekMode ? 'G' : '')
            : ''))
      }
      .width('100%')
      .alignItems(HorizontalAlign.Center)
    }
    .width('100%')
    .height(MATH_KB_HEIGHT)
    .padding({ left: 6, right: 6, top: 6, bottom: 14 })
    .backgroundColor(this.kbBg())
  }
}
