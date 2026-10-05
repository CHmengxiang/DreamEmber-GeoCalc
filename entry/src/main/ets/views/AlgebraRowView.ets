// AlgebraRowView.ets — 代数区单行组件（自 CanvasView 拆出，轮 A 防乱重构）。
// @ObjectLink 直连行数据。交互：
// 实/空心按钮=显隐；字母点击=行内改名（expandHit 扩大触控区，onRenameRequest
// 通知父级滚动避让键盘）；值文本点击=定义回填修改模式（slider 行例外=父级
// 在本行下方行内展开四项参数编辑，二十九包②——原回填入口一动就把滑动条
// 改成固定值）；整行点击=画布选中；slider 行内嵌进度条（左右上下限 + 当前值）
// + ▶/⏸ 动画。
import { AlgebraEntry } from '../model/GeoTypes';
import { Theme } from '../model/Theme';
import { MathText } from './MathField';

@Component
export struct AlgebraRowView {
  @ObjectLink entry: AlgebraEntry;
  @Prop dark: boolean = false;
  onToggleVis: (label: string, show: boolean) => void = (_l: string, _s: boolean) => {};
  onSelect: (label: string) => void = (_l: string) => {};
  onRename: (label: string, next: string) => void = (_l: string, _n: string) => {};
  onRenameRequest: (label: string) => void = (_l: string) => {};
  onEdit: (label: string, definition: string) => void = (_l: string, _d: string) => {};
  // 二十九包②：slider 行值文本点击 → 父级行内展开四项参数编辑（不走定义回填）
  onSliderEdit: (label: string) => void = (_l: string) => {};
  onToggleAnim: (label: string) => void = (_l: string) => {};
  onDelete: (label: string) => void = (_l: string) => {};
  onSlider: (label: string, v: number, mode: number) => void =
    (_l: string, _v: number, _m: number) => {};
  onAlpha: (label: string, v: number, mode: number) => void =
    (_l: string, _v: number, _m: number) => {};
  @State private editing: boolean = false;   // 行内重命名中
  @State private editValue: string = '';
  @State private alphaEditing: boolean = false;   // 透明度百分比就地手改中
  @State private alphaText: string = '';

  // 滑杆/手改框用"透明度"语义（100=完全消失，0=对象色实底最深）：
  // 内核 fillAlpha 是不透明度，显示值 = 100×(1−fillAlpha)（十四轮翻转）
  private beginAlphaEdit(): void {
    this.alphaText = String(Math.round((1 - this.entry.fillAlpha) * 100));
    this.alphaEditing = true;
  }

  // 提交手改透明度：钳 0-100，mode 2（End）→ 父级立即下发内核并同步行
  private commitAlphaEdit(): void {
    if (!this.alphaEditing) return;   // onSubmit 后紧跟的 onBlur 不重复提交
    this.alphaEditing = false;
    const p = Math.max(0, Math.min(100, Math.round(parseFloat(this.alphaText) || 0)));
    this.onAlpha(this.entry.label, p, 2);
  }

  private fmt(n: number): string {
    return Number.isFinite(n) ? `${n}` : '';
  }

  private commitRename(): void {
    if (!this.editing) {
      return;
    }
    this.editing = false;
    const nv = this.editValue.trim();
    if (nv.length > 0 && nv !== this.entry.label) {
      this.onRename(this.entry.label, nv);
    }
  }

  private beginRename(): void {
    // 已在改名中则不重复进入（先报父级做避让滚动）
    this.editValue = this.entry.label;
    this.onRenameRequest(this.entry.label);
    this.editing = true;
    // defaultFocus 对动态插入的组件不生效（真机：首次点字母后还要再点一次
    // 输入框才能编辑），挂载完成后主动请求焦点弹起键盘；80ms 未中再补一次
    const inputId = 'renameInput_' + this.entry.label;
    setTimeout(() => {
      if (this.editing) {
        this.getUIContext().getFocusController().requestFocus(inputId);
      }
    }, 80);
    setTimeout(() => {
      if (this.editing) {
        this.getUIContext().getFocusController().requestFocus(inputId);
      }
    }, 320);
  }

  build() {
    Column() {
      Row() {
      // 可见性按钮：实心=显示，空心=隐藏
      Text()
        .width(18)
        .height(18)
        .borderRadius(9)
        .borderWidth(2)
        .borderColor(Theme.typeColor(this.entry.type, this.dark))
        .backgroundColor(this.entry.vis === '0'
          ? Color.Transparent : Theme.typeColor(this.entry.type, this.dark))
        .margin({ right: 8 })
        .onClick(() => this.onToggleVis(this.entry.label, this.entry.vis === '0'))
      if (this.editing) {
        // 行内改名输入框（提交/失焦生效；冲突失败父级弹警告不改）
        TextInput({ text: this.editValue })
          .width(96)
          .height(30)
          .fontSize(14)
          .id('renameInput_' + this.entry.label)
          .onChange((v: string) => {
            this.editValue = v;
          })
          .onSubmit(() => this.commitRename())
          .onBlur(() => this.commitRename())
          .defaultFocus(true)
      } else {
        // 字母触控热区扩大到 56×30（原 14pt 文本太难点、易误触右侧公式）
        Text(this.entry.label)
          .fontSize(14)
          .fontWeight(FontWeight.Medium)
          .fontColor(this.entry.vis === '0' ? Theme.faint(this.dark) : Theme.text(this.dark))
          .maxLines(1)
          .width(56)
          .height(30)
          .textAlign(TextAlign.Start)
          .hitTestBehavior(HitTestMode.Default)
          .responseRegion({ x: -8, y: -6, width: '120%', height: '140%' })
          .onClick(() => this.beginRename())
      }
      if (this.entry.type === 'slider') {
        // 行内滑动条：进度条 + 两端上下限 + 当前值（替代画布滑动条）
        Column({ space: 1 }) {
          Slider({
            value: parseFloat(this.entry.value) || 0,
            min: this.entry.min,
            max: this.entry.max > this.entry.min ? this.entry.max : this.entry.min + 1,
            step: this.entry.step > 0 ? this.entry.step : 0.01
          })
            .selectedColor('#007DFF')
            .trackColor(this.dark ? '#3A3D42' : '#D8DCE4')
            .onChange((v: number, mode: number) => this.onSlider(this.entry.label, v, mode))
          Row() {
            Text(this.fmt(this.entry.min))
              .fontSize(10)
              .fontColor(Theme.faint(this.dark))
            Text(this.entry.value)
              .fontSize(11)
              .fontColor(Theme.accent(this.dark))
              .layoutWeight(1)
              .textAlign(TextAlign.Center)
              .maxLines(1)
              .onClick(() => this.onSliderEdit(this.entry.label))
            Text(this.fmt(this.entry.max))
              .fontSize(10)
              .fontColor(Theme.faint(this.dark))
          }
          .width('100%')
        }
        .layoutWeight(1)
        .margin({ left: 4, right: 4 })
      } else {
        // 十三轮：修改框只由值文本本体触发（原 layoutWeight 拉通整行，
        // 点行其余位置也弹修改框）；空白区点按落回整行 = 仅选中。
        // 第八包：值/定义列公式渲染（x² 上标、√ 上划线、a_n 下标等，⑦）
        MathText({
          raw: this.entry.value,
          dark: this.dark,
          fontSize: 13,
          color: Theme.sub(this.dark)
        })
          .onClick(() => this.onEdit(this.entry.label, this.entry.definition))
        Blank()
          .layoutWeight(1)
      }
      if (this.entry.type === 'slider') {
        Text(this.entry.anim === '1' ? '⏸' : '▶')
          .fontSize(14)
          .fontColor(Theme.accent(this.dark))
          .padding({ left: 8, right: 4 })
          .onClick(() => this.onToggleAnim(this.entry.label))
      }
      Text('×')
        .fontSize(18)
        .fontColor(Theme.del(this.dark))
        .padding({ left: 10, right: 6 })
        .onClick(() => this.onDelete(this.entry.label))
      }
      .width('100%')
      .height(this.entry.type === 'slider' ? 64 : 36)
      .padding({ left: 12, right: 8 })

      // 选中态透明度滑杆（不等式区域 / 多边形 / 角扇形，真机反馈旧版行内
      // 28vp 太隐蔽看不见）：整行滑杆 + 百分比，置于对象名行之下；空点按
      // 吞掉不触发行选中。透明度 100 = 填充消失、0 = 对象色实底（十四轮翻转，
      // 此前 8 位 hex 被 ArkUI 按 #AARRGGBB 误读导致调大只变色不变深）。
      // 十三轮：接通 onChange（此前滑杆拖动不下发内核、画布不动）、步长
      // 5→1 细化、点百分比弹就地数字框手动改（提交/失焦生效，同其他编辑）
      if (this.entry.sel === '1' && this.entry.fillAlpha >= 0) {
        Row({ space: 6 }) {
          Text('透明度')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
          Slider({
            value: Math.round((1 - this.entry.fillAlpha) * 100),
            min: 0,
            max: 100,
            step: 1
          })
            .selectedColor('#007DFF')
            .trackColor(this.dark ? '#3A3D42' : '#C9CEDA')
            .trackThickness(5)
            .blockSize({ width: 16, height: 16 })
            .height(26)
            .layoutWeight(1)
            .onChange((v: number, mode: number) => this.onAlpha(this.entry.label, v, mode))
          if (this.alphaEditing) {
            TextInput({ text: this.alphaText })
              .fontSize(11)
              .fontColor(Theme.accent(this.dark))
              .width(44)
              .height(24)
              .padding({ left: 4, right: 4 })
              .textAlign(TextAlign.End)
              .backgroundColor(Theme.inputBg(this.dark))
              .onChange((v: string) => {
                this.alphaText = v;
              })
              .onSubmit(() => this.commitAlphaEdit())
              .onBlur(() => this.commitAlphaEdit())
          } else {
            Text(`${Math.round((1 - this.entry.fillAlpha) * 100)}%`)
              .fontSize(11)
              .fontColor(Theme.accent(this.dark))
              .width(34)
              .textAlign(TextAlign.End)
              .onClick(() => this.beginAlphaEdit())
          }
        }
        .width('100%')
        .height(32)
        .padding({ left: 38, right: 14 })
        .margin({ bottom: 4 })
        .onClick(() => {})
      }
    }
    .width('100%')
    .borderRadius(6)
    .borderWidth(1.5)
    .borderColor(this.entry.sel === '1' ? Theme.accent(this.dark) : Color.Transparent)
    .backgroundColor(Theme.rowBg(this.dark))
    .onClick(() => this.onSelect(this.entry.label))
  }
}
