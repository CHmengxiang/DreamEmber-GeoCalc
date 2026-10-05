// BottomTabs.ets — 轮 B：上游经典版 式底部视图 Tab（窄屏底栏）。
// 代数区 / 工具 / 表格 / 数据；点击切换面板，再点同一 Tab 收起（active ''
// = 全部收起）。面板内容由宿主 CanvasView 按 active 渲染。
// 第二十二包①（上游 源码考据，NavigationRail.createCenter + menu_zh_CN）：
// 上游 导航栏顺序 Algebra(代数区)/Tools(工具)/Table(表格)/Spreadsheet(数据)
// ——中文语言包 Perspective.Spreadsheet 译文即「数据」（电子表格视图），
// 「表格」是另一个视图（函数值表）。此前我们把电子表格标成「表格」、
// 统计分析标成「数据」，正好弄反。现按 上游 归位：第 3 钮「表格」= 数据
// 分析面板（占位；上游 真·函数值表待后续补，届时数据分析改从数据区选区
// 启动）、第 4 钮「数据」= 字母+数字电子表格。
import { Theme } from '../model/Theme';

export interface TabDef {
  id: string;
  icon: string;
  name: string;
  // 二十五包①：字形垂直微调（vp）——四个图标是不同字形的字体字，✛ 十字
  // 字形天生浮在 em 框偏上（真机反馈：比另外三个略高一点），按字形下移。
  // 数值以 22vp（本栏字号）为基准；其他字号按比例折算（wideRail 用
  // dy×18/22——二十六包①模拟器实测：dy=2 后 fs18 栏墨迹中心仍偏高
  // 2.83px≈1.5vp，反推天生偏高 ≈3.5vp@18 → 比例 ≈4.3vp@22，故 4）
  dy: number;
}

// 第二十一包①：导出——宽屏左侧视图栏（CanvasView wideRail）复用同一组
// Tab 定义，保证宽窄两形态四钮同序同图
export const TABS: TabDef[] = [
  { id: 'algebra', icon: 'ƒ', name: '代数区', dy: 0 },
  { id: 'tools', icon: '✛', name: '工具', dy: 4 },
  { id: 'data', icon: '▥', name: '表格', dy: 0 },
  { id: 'sheet', icon: '▦', name: '数据', dy: 0 }
];

@Component
export struct BottomTabs {
  @Prop dark: boolean = false;
  @Prop active: string = '';   // 当前展开的 Tab id；'' = 全部收起
  onPick: (tab: string) => void = (_tab: string) => {};

  build() {
    Row() {
      ForEach(TABS, (t: TabDef) => {
        Column() {
          // 图标/文字各占固定高并居中（真机反馈③：默认行高随字形基线浮动，
          // 四个 Tab 的图标高低不一、图标与文字整体也不居中，钉死盒子统一）。
          // 二十四包③：图标加大（16→22，上游 移动端底栏大图标同观感）
          // 二十五包①：translate 不占布局，✛ 下移 dy 对齐其余字形
          Text(t.icon)
            .fontSize(22)
            .lineHeight(24)
            .height(28)
            .textAlign(TextAlign.Center)
            .translate({ x: 0, y: t.dy })
            .fontColor(this.active === t.id ? Theme.accent(this.dark) : Theme.sub(this.dark))
          Text(t.name)
            .fontSize(10)
            .lineHeight(12)
            .height(14)
            .textAlign(TextAlign.Center)
            .margin({ top: 1 })
            .fontColor(this.active === t.id ? Theme.accent(this.dark) : Theme.sub(this.dark))
        }
        .alignItems(HorizontalAlign.Center)
        .justifyContent(FlexAlign.Center)
        .layoutWeight(1)
        .height('100%')
        .onClick(() => this.onPick(t.id))
      }, (t: TabDef) => t.id)
    }
    .width('100%')
    .height(56)
    .padding({ top: 4, bottom: 4 })
    .backgroundColor(Theme.panel(this.dark))
  }
}
