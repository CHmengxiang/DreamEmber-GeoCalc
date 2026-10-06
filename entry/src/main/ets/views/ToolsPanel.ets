// ToolsPanel.ets — 底部"工具"Tab 的面板（防呆版）：工具单元格直接字面渲染，
// 分组头 + 行布局全部写死——不用 ForEach/条件渲染/@Builder。
// 背景：轮C 嵌套 ForEach 与轮D 首版（单层 ForEach + 预计算行单元）真机都
// 丢工具，而同结构的表格面板正常，无法真机日志定位，故去掉一切可疑机制，
// 只用 Row/Column 原语内联渲染（选中态直接读 this.tool，符合铁律）。
// 二十七包③：图标从 Unicode 字形 Text 换成 ToolIcon（Canvas 矢量绘制，
// 上游 风格原创，见 ToolIcons.ets）——字面单元格结构不变，只换 42×36 格内
// 的内容，图标选中/主题色由 ToolIcon 自绘。
// 增删工具：model/ToolDefs 的 TOOLS 与本文件的字面单元格需同步改。
import { Theme } from '../model/Theme';
import { toolUsage } from '../model/ToolDefs';
import { ToolIcon } from './ToolIcons';

@Component
export struct ToolsPanel {
  @Prop dark: boolean = false;
  @Prop tool: string = '';   // 当前工具 id（'' = 选择/移动）
  onSetTool: (id: string) => void = (_id: string) => {};
  onClose: () => void = () => {};

  build() {
    Column() {
      Row() {
        Text('工具')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.text(this.dark))
          .layoutWeight(1)
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

      Scroll() {
        Column() {
          // ---- 基础 ----
          Text('基础')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 6, top: 4 })
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: '', dark: this.dark, selected: this.tool === '' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === ''
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('选择')
                .fontSize(10)
                .fontColor(this.tool === '' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === '' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool(''))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'point', dark: this.dark, selected: this.tool === 'point' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'point'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('点')
                .fontSize(10)
                .fontColor(this.tool === 'point' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'point' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('point'))
          }
          .width('100%')

          // ---- 线 ----
          Text('线')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 6, top: 4 })
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'segment', dark: this.dark, selected: this.tool === 'segment' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'segment'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('线段')
                .fontSize(10)
                .fontColor(this.tool === 'segment' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'segment' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('segment'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'line', dark: this.dark, selected: this.tool === 'line' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'line'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('直线')
                .fontSize(10)
                .fontColor(this.tool === 'line' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'line' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('line'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'ray', dark: this.dark, selected: this.tool === 'ray' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'ray'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('射线')
                .fontSize(10)
                .fontColor(this.tool === 'ray' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'ray' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('ray'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'vector', dark: this.dark, selected: this.tool === 'vector' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'vector'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('向量')
                .fontSize(10)
                .fontColor(this.tool === 'vector' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'vector' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('vector'))
          }
          .width('100%')

          // 二十三包 工具第一批：垂线/平行线/相等向量
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'perpLine', dark: this.dark, selected: this.tool === 'perpLine' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'perpLine'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('垂线')
                .fontSize(10)
                .fontColor(this.tool === 'perpLine' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'perpLine' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('perpLine'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'parLine', dark: this.dark, selected: this.tool === 'parLine' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'parLine'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('平行线')
                .fontSize(10)
                .fontColor(this.tool === 'parLine' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'parLine' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('parLine'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'vecFromPoint', dark: this.dark,
                  selected: this.tool === 'vecFromPoint' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'vecFromPoint'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('相等向量')
                .fontSize(10)
                .fontColor(this.tool === 'vecFromPoint' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'vecFromPoint' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('vecFromPoint'))
            // 二十六包 工具第二批：定长线段（Segment(点, 长度)，长度弹窗输入）
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'segLength', dark: this.dark, selected: this.tool === 'segLength' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'segLength'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('定长线段')
                .fontSize(10)
                .fontColor(this.tool === 'segLength' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'segLength' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('segLength'))
          }
          .width('100%')

          // ---- 圆与多边形 ----
          Text('圆与多边形')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 6, top: 4 })
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'circle', dark: this.dark, selected: this.tool === 'circle' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'circle'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('圆')
                .fontSize(10)
                .fontColor(this.tool === 'circle' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'circle' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('circle'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'polygon', dark: this.dark, selected: this.tool === 'polygon' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'polygon'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('多边形')
                .fontSize(10)
                .fontColor(this.tool === 'polygon' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'polygon' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('polygon'))
            // 二十六包 工具第二批：圆（半径）（Circle(圆心, 数)，半径弹窗输入）
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'circleRadius', dark: this.dark,
                  selected: this.tool === 'circleRadius' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'circleRadius'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('圆（半径）')
                .fontSize(10)
                .fontColor(this.tool === 'circleRadius' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'circleRadius' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('circleRadius'))
          }
          .width('100%')

          // ---- 构造 ----
          Text('构造')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 6, top: 4 })
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'midpoint', dark: this.dark, selected: this.tool === 'midpoint' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'midpoint'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('中点')
                .fontSize(10)
                .fontColor(this.tool === 'midpoint' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'midpoint' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('midpoint'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'perpBisector', dark: this.dark,
                  selected: this.tool === 'perpBisector' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'perpBisector'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('中垂线')
                .fontSize(10)
                .fontColor(this.tool === 'perpBisector' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'perpBisector' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('perpBisector'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'angleBisector', dark: this.dark,
                  selected: this.tool === 'angleBisector' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'angleBisector'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('角平分线')
                .fontSize(10)
                .fontColor(this.tool === 'angleBisector' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'angleBisector' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('angleBisector'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'angle', dark: this.dark, selected: this.tool === 'angle' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'angle'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('角')
                .fontSize(10)
                .fontColor(this.tool === 'angle' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'angle' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('angle'))
          }
          .width('100%')
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'intersect', dark: this.dark, selected: this.tool === 'intersect' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'intersect'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('交点')
                .fontSize(10)
                .fontColor(this.tool === 'intersect' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'intersect' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('intersect'))
            // 二十六包 工具第二批：定角（Angle with Given Size，顶点在第二点、
            // 自角边逆时针量给定角度；角度弹窗输入）
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'angleSize', dark: this.dark, selected: this.tool === 'angleSize' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'angleSize'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('定角')
                .fontSize(10)
                .fontColor(this.tool === 'angleSize' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'angleSize' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('angleSize'))
            // 四十五包 附着/脱离点（上游 同名工具：自由点→路径附着，依附点→脱离）
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'attachDetach', dark: this.dark, selected: this.tool === 'attachDetach' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'attachDetach'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('附着/脱离点')
                .fontSize(10)
                .fontColor(this.tool === 'attachDetach' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'attachDetach' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('attachDetach'))
          }
          .width('100%')

          // ---- 变换 ----
          Text('变换')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 6, top: 4 })
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'translate', dark: this.dark, selected: this.tool === 'translate' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'translate'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('平移')
                .fontSize(10)
                .fontColor(this.tool === 'translate' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'translate' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('translate'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'reflect', dark: this.dark, selected: this.tool === 'reflect' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'reflect'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('反射')
                .fontSize(10)
                .fontColor(this.tool === 'reflect' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'reflect' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('reflect'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'reflectPoint', dark: this.dark,
                  selected: this.tool === 'reflectPoint' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'reflectPoint'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('中心对称')
                .fontSize(10)
                .fontColor(this.tool === 'reflectPoint' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'reflectPoint' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('reflectPoint'))
            // 二十六包 工具第二批：旋转（对象+中心+角度弹窗）
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'rotate', dark: this.dark, selected: this.tool === 'rotate' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'rotate'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('旋转')
                .fontSize(10)
                .fontColor(this.tool === 'rotate' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'rotate' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('rotate'))
          }
          .width('100%')
          // 二十六包 工具第二批：位似（对象+中心+比例弹窗）
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'dilate', dark: this.dark, selected: this.tool === 'dilate' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'dilate'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('位似')
                .fontSize(10)
                .fontColor(this.tool === 'dilate' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'dilate' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('dilate'))
          }
          .width('100%')

          // ---- 测量（二十三包 工具第一批：上游 工具箱 7 距离/面积/斜率）----
          Text('测量')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 6, top: 4 })
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'dist', dark: this.dark, selected: this.tool === 'dist' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'dist'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('距离')
                .fontSize(10)
                .fontColor(this.tool === 'dist' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'dist' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('dist'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'area', dark: this.dark, selected: this.tool === 'area' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'area'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('面积')
                .fontSize(10)
                .fontColor(this.tool === 'area' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'area' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('area'))
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'slope', dark: this.dark, selected: this.tool === 'slope' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'slope'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('斜率')
                .fontSize(10)
                .fontColor(this.tool === 'slope' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'slope' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('slope'))
          }
          .width('100%')

          // ---- 特殊 ----
          Text('特殊')
            .fontSize(11)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 6, top: 4 })
          Row() {
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'tangent', dark: this.dark, selected: this.tool === 'tangent' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'tangent'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('切线')
                .fontSize(10)
                .fontColor(this.tool === 'tangent' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'tangent' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('tangent'))
            // 二十七包 滑动条工具（点画布建 Slider，代数区滑杆驱动参数联动）
            Column({ space: 3 }) {
              Column() {
                ToolIcon({ tid: 'slider', dark: this.dark, selected: this.tool === 'slider' })
              }
              .width(42)
              .height(36)
              .borderRadius(8)
              .backgroundColor(this.tool === 'slider'
                ? Theme.accent(this.dark) : (this.dark ? '#2C2F34' : '#FFFFFF'))
              Text('滑动条')
                .fontSize(10)
                .fontColor(this.tool === 'slider' ? Theme.text(this.dark) : Theme.sub(this.dark))
                .maxLines(1)
            }
            .width('25%')
            .padding({ top: 6, bottom: 6, left: 3, right: 3 })
            .borderRadius(10)
            .backgroundColor(this.tool === 'slider' ? Theme.keyBg(this.dark) : Color.Transparent)
            .onClick(() => this.onSetTool('slider'))
          }
          .width('100%')
        }
        .width('100%')
        .padding({ left: 8, right: 8, bottom: 8 })
      }
      .scrollBar(BarState.Off)
      // 四十一包：AGC 上架自检（云测试 UX）指出可滑动控件滑到边界缺回弹
      // 动效——Scroll 组件默认 EdgeEffect.None；全应用 7 处 Scroll 统一补
      // Spring（List 默认已是 Spring，不动）。内容不满一屏时默认不生效，
      // 与华为「边界反馈」标准一致，不引入 alwaysEnabled 的过度弹性
      .edgeEffect(EdgeEffect.Spring)
      .layoutWeight(1)

      // 底部一行：当前工具的用法提示（与画布左上反馈条同源文案）
      Text(this.tool.length > 0
        ? `${toolUsage(this.tool) || '已选择工具：点画布作图'}`
        : '选择/移动：点选对象选中或拖动，空白处平移视图')
        .fontSize(11)
        .fontColor(Theme.sub(this.dark))
        .width('100%')
        .padding({ left: 12, right: 12, bottom: 8 })
        .maxLines(2)
    }
    .width('100%')
    .height('100%')
  }
}
