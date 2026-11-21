// StepsPanel.ets — 构造步骤视图（144包/1.2.0.16）：构造序逐步回看（只读）。
// 行 = 序号. 标签 + 定义列（MathText 公式渲染，与代数区同管线）；点行画布
// 选中对象（GGB 构造步骤导航同语义）；◀ ▶ 步进选中并高亮当前步。
// 数据直接复用代数区行（构造序即 algebra 数组序，定义列回填可重放）。
import { AlgebraEntry } from '../model/GeoTypes';
import { Theme } from '../model/Theme';
import { MathText } from './MathField';

@Component
export struct StepsPanel {
  @Prop algebra: AlgebraEntry[] = [];
  @Prop dark: boolean = false;
  @State private cur: number = -1;   // 当前步下标（-1 = 未步进）
  // 151包 逐步隐藏：idx 供宿主按构造序裁剪画布可见性（GGB 导航口径）
  onSelect: (label: string, idx: number) => void = (_l: string, _i: number) => {};
  onShowAll: () => void = () => {};   // 「全部」退出逐步模式恢复全显
  onClose: () => void = () => {};

  // ◀ ▶ 步进：夹在 [0, n-1]，选中当前步对象（GGB 步进导航同语义；
  // 空构造不动）
  private step(d: number): void {
    const n = this.algebra.length;
    if (n === 0) {
      return;
    }
    let next = this.cur < 0 ? (d > 0 ? 0 : n - 1) : this.cur + d;
    if (next < 0) {
      next = n - 1;
    } else if (next >= n) {
      next = 0;
    }
    this.cur = next;
    this.onSelect(this.algebra[next].label, next);
  }

  private showAll(): void {
    this.cur = -1;
    this.onShowAll();
  }

  build() {
    Column() {
      Row() {
        Text('构造步骤')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.sub(this.dark))
          .layoutWeight(1)
        Text(`${this.algebra.length} 步`)
          .fontSize(11)
          .fontColor(Theme.faint(this.dark))
          .margin({ right: 10 })
        Text('◀')
          .fontSize(15)
          .fontColor(this.algebra.length > 0 ? Theme.accent(this.dark) : Theme.faint(this.dark))
          .width(34)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.step(-1))
        Text('▶')
          .fontSize(15)
          .fontColor(this.algebra.length > 0 ? Theme.accent(this.dark) : Theme.faint(this.dark))
          .width(34)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.step(1))
        Text('全部')
          .fontSize(12)
          .fontColor(Theme.accent(this.dark))
          .height(30)
          .padding({ left: 6, right: 6 })
          .textAlign(TextAlign.Center)
          .onClick(() => this.showAll())
        Text('✕')
          .fontSize(14)
          .fontColor(Theme.sub(this.dark))
          .width(34)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.onClose())
      }
      .width('100%')
      .height(38)
      .padding({ left: 14, right: 8 })

      List() {
        ForEach(this.algebra, (e: AlgebraEntry, idx: number) => {
          ListItem() {
            Row() {
            Text(`${idx + 1}.`)
              .fontSize(12)
              .fontColor(Theme.faint(this.dark))
              .width(30)
            Text(e.label)
              .fontSize(13)
              .fontWeight(FontWeight.Medium)
              .fontColor(Theme.typeColor(e.type, this.dark))
              .width(52)
            MathText({
              raw: e.definition.length > 0 ? e.definition : e.value,
              dark: this.dark,
              fontSize: 12,
              color: Theme.text(this.dark)
            })
              .layoutWeight(1)
          }
          .width('100%')
          .height(34)
          .padding({ left: 12, right: 10 })
          .borderRadius(6)
          .backgroundColor(this.cur === idx ? (this.dark ? '#2A2D33' : '#E8EEF9')
            : Color.Transparent)
          .border({ width: 1, color: this.cur === idx ? Theme.accent(this.dark)
            : Color.Transparent, radius: 6 })
          .onClick(() => {
            this.cur = idx;
            this.onSelect(e.label, idx);
          })
          }
        }, (e: AlgebraEntry): string => e.label)
      }
      .width('100%')
      .layoutWeight(1)
      .scrollBar(BarState.Off)
      .padding({ top: 2, bottom: 8 })
    }
    .width('100%')
    .height('100%')
  }
}
