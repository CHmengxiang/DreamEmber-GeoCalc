// StyleBar.ets — 轮 C 样式条（上游 选中对象样式条）：画布/代数区选中对象后
// 出现的一条横向快捷样式栏，全部走轮 A 通用属性 API（kernelGetProps 读、
// kernelSetProp 写；显隐走 kernelSetVisible 以复用内核自动撤销打点）。
// 能力按对象类型自适应：颜色色板（全部可描边对象）、大小 －/＋（点=点大小
// 1-9，其余=线宽 1-13）、透明度滑杆（不等式/多边形/角等有填充的对象）。
// 撤销语义：离散点按（色板/大小）由宿主在写入前 pushUndo 一步一变；透明度
// 滑杆拖动由宿主在 mode 0/3 打点，整段拖动一步（与行内滑杆一致）。
// 注意：本组件不用 @Builder 传参渲染任何依赖状态的 UI（按值传参不刷新，
// 轮 B 撤销按钮置灰问题的教训）。
import dreamemberKernel from 'libdreamember.so';
import { Theme } from '../model/Theme';

// 色板（上游 常用 8 色）
const SWATCHES: string[] = [
  '#000000', '#E53935', '#FB8C00', '#FDD835',
  '#43A047', '#00ACC1', '#1E88E5', '#8E44AD'
];

@Component
export struct StyleBar {
  @Prop dark: boolean = false;
  @Prop @Watch('refetch') label: string = '';   // 选中对象（'' 由宿主控制不挂载）
  @Prop @Watch('refetch') tick: number = 0;     // 宿主每次代数区同步后 +1 → 重取属性
  // 属性写入（markUndo=该次变更需宿主先打撤销快照；final=手势结束帧需强制
  // 同步，如滑杆 mode 2/3 与离散点按）与显隐切换
  onSetProp: (label: string, key: string, val: string, markUndo: boolean, final: boolean) => void =
    (_l: string, _k: string, _v: string, _m: boolean, _f: boolean) => {};
  onSetVisible: (label: string, show: boolean) => void = (_l: string, _s: boolean) => {};

  // kernelGetProps 快照解析结果（'' = 该属性不存在）
  @State private pType: string = '';
  @State private pVis: string = '1';
  @State private pColor: string = '';
  @State private pFill: string = '';   // 不透明度百分比（0-100）；'' = 无填充
  @State private pSize: string = '';   // 点大小/线宽；'' = 无大小

  aboutToAppear(): void {
    this.refetch();
  }

  // 重取属性快照（选中变化 / 任意代数区同步）
  private refetch(): void {
    this.pType = '';
    this.pVis = '1';
    this.pColor = '';
    this.pFill = '';
    this.pSize = '';
    if (this.label.length === 0) {
      return;
    }
    const s: string = dreamemberKernel.kernelGetProps(this.label);
    if (s === '-|' || s.length === 0) {
      return;
    }
    for (const kv of s.split(';')) {
      const eq = kv.indexOf('=');
      if (eq <= 0) {
        continue;
      }
      const k = kv.substring(0, eq);
      const v = kv.substring(eq + 1);
      if (k === 'type') {
        this.pType = v;
      } else if (k === 'vis') {
        this.pVis = v;
      } else if (k === 'color') {
        this.pColor = v;
      } else if (k === 'fill') {
        this.pFill = v;
      } else if (k === 'size') {
        this.pSize = v;
      }
    }
  }

  private isStylable(): boolean {
    return ['point', 'segment', 'line', 'ray', 'vector', 'circle',
      'polygon', 'angle', 'function', 'text'].indexOf(this.pType) >= 0;
  }

  private hasSize(): boolean {
    return this.pSize.length > 0;
  }

  private sizeHi(): number {
    return this.pType === 'point' ? 9 : 13;
  }

  private bumpSize(delta: number): void {
    const cur = parseInt(this.pSize);
    if (isNaN(cur)) {
      return;
    }
    const next = Math.min(this.sizeHi(), Math.max(1, cur + delta));
    if (next === cur) {
      return;   // 已在边界，避免产生无变化的撤销步
    }
    this.onSetProp(this.label, 'size', String(next), true, true);
  }

  private pickColor(c: string): void {
    if (c.toUpperCase() === this.pColor.toUpperCase()) {
      return;   // 同色不重复入撤销栈
    }
    this.onSetProp(this.label, 'color', c, true, true);
  }

  build() {
    Column() {
      if (this.isStylable()) {
        Row() {
          Text(this.label)
            .fontSize(13)
            .fontWeight(FontWeight.Medium)
            .fontColor(Theme.accent(this.dark))
            .constraintSize({ minWidth: 26, maxWidth: 64 })
            .textAlign(TextAlign.Center)
            .maxLines(1)
            .margin({ left: 10 })

          Scroll() {
            Row({ space: 10 }) {
              // 显/隐（走 kernelSetVisible：内核自动撤销打点 + 无变化不留步）
              Text(this.pVis === '1' ? '显' : '隐')
                .fontSize(12)
                .fontColor(this.pVis === '1' ? Theme.accent(this.dark) : Theme.del(this.dark))
                .width(28)
                .height(28)
                .textAlign(TextAlign.Center)
                .borderRadius(8)
                .borderWidth(1)
                .borderColor(Theme.border(this.dark))
                .onClick(() => this.onSetVisible(this.label, this.pVis !== '1'))

              ForEach(SWATCHES, (c: string) => {
                Column()
                  .width(20)
                  .height(20)
                  .borderRadius(10)
                  .backgroundColor(c)
                  .borderWidth(this.pColor.toUpperCase() === c ? 2 : 1)
                  .borderColor(this.pColor.toUpperCase() === c
                    ? Theme.text(this.dark)
                    : Theme.border(this.dark))
                  .onClick(() => this.pickColor(c))
              }, (c: string) => c)

              if (this.hasSize()) {
                // 大小 －/＋（点=点大小 1-9，线类=线宽 1-13）
                Row({ space: 2 }) {
                  Text('－')
                    .fontSize(14)
                    .fontColor(parseInt(this.pSize) > 1 ? Theme.text(this.dark) : Theme.faint(this.dark))
                    .width(26)
                    .height(26)
                    .textAlign(TextAlign.Center)
                    .onClick(() => this.bumpSize(-1))
                  Text(this.pSize)
                    .fontSize(11)
                    .fontColor(Theme.sub(this.dark))
                    .width(18)
                    .textAlign(TextAlign.Center)
                  Text('＋')
                    .fontSize(14)
                    .fontColor(parseInt(this.pSize) < this.sizeHi() ? Theme.text(this.dark) : Theme.faint(this.dark))
                    .width(26)
                    .height(26)
                    .textAlign(TextAlign.Center)
                    .onClick(() => this.bumpSize(1))
                }
                .borderRadius(8)
                .borderWidth(1)
                .borderColor(Theme.border(this.dark))
                .height(28)
              }

              if (this.pFill.length > 0) {
                // 透明度滑杆（0=不透明 100=消失；内核不透明度 = 1 − v/100）
                Text('透明')
                  .fontSize(11)
                  .fontColor(Theme.sub(this.dark))
                Slider({
                  value: 100 - (parseInt(this.pFill) || 0),
                  min: 0,
                  max: 100,
                  step: 1
                })
                  .selectedColor('#007DFF')
                  .trackColor(this.dark ? '#3A3D42' : '#C9CEDA')
                  .trackThickness(5)
                  .blockSize({ width: 16, height: 16 })
                  .height(26)
                  .width(108)
                  .onChange((v: number, mode: number) => {
                    this.onSetProp(this.label, 'fill', String((100 - v) / 100),
                      mode === 0 || mode === 3, mode === 2 || mode === 3);
                  })
              }
            }
            .padding({ left: 4, right: 10 })
          }
          .scrollable(ScrollDirection.Horizontal)
          .scrollBar(BarState.Off)
          .edgeEffect(EdgeEffect.Spring)
          .layoutWeight(1)
          .height('100%')
        }
        .width('100%')
        .height(44)
        .backgroundColor(Theme.panel(this.dark))
      }
    }
    .width('100%')
  }
}
