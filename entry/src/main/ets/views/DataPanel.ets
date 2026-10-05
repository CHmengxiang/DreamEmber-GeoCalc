// DataPanel.ets — 数据视图（底栏/宽屏左栏"数据"Tab）：单变量分析 + 直方图。
// 数据录入为文本框（逗号/空格/换行分隔），点"分析"在 ArkTS 侧完成解析与
// 统计，直方图画在本面板私有 Canvas（独立于几何画布的小图表；内核统计
// 命令保持原样，不往构造列表塞数据对象）。等分 10 桶，桶高 = 频数，坐标
// 轴 + 频数刻度。深浅色随宿主。
// 2026-09-30 第六包（标注字号最终方案）：ctx.font 在真机上不生效——画布只
// 画几何（网格/柱/轴），全部文字改用原生 Text 组件按 vp 几何叠加在 Stack
// 里；几何按 u = 画布单位/vp 缩放绘制。
//
// 第二十一包③b（真机反馈"数据和 上游 差别很大，查源码还原"）：对齐上游
// 官方单变量分析（DataAnalysisViewW/BasicStatTableW/StatTableModel，
// 仅作行为参照、不复制资产）：
// ① 布局 = DataAnalysisViewW：宽屏左数据源栏（addWest 300 同款比例）+
//   右绘图（上）与统计表（下）双栏；窄屏纵向堆叠同序（录入→图→表）；
// ② 统计表 = StatTableModel 单变量行序全量还原：N、均值、标准差（总体
//   SD）、样本标准差、Σx、Σx²、最小值、Q1、中位数、Q3、最大值，两对
//   名值一行、无表头（getColumnNames=null）、值列右对齐；
// ③ Q1/Q3 = AlgoQ1/AlgoQ3 原算法（按 n%4 分支的中位数半分法，n<2 留空）；
// ④ 绘图面板加边框白底卡片（上游 PlotPanel 同视觉），网格 NEUTRAL_300、
//   轴 NEUTRAL_500（深色近似同系）。
import { Theme } from '../model/Theme';

interface DataStats {
  n: number;
  mean: number;
  median: number;
  sdSample: number;   // 样本标准差（n-1）
  sdPop: number;      // 标准差（总体，n）——上游 表 SD 行
  sum: number;        // Σx
  sumSq: number;      // Σx²
  min: number;
  max: number;
  q1: number;         // NaN = 未定义（n<2，上游 AlgoQ1 留空）
  q3: number;
  bins: number[];     // 各桶频数
  binW: number;       // 桶宽
  lo: number;         // 直方图横轴下界（min）
  hi: number;         // 上界（max）
}

// 统计表一行 = 两对名值（上游 BasicStatTableW 两对/行）
interface StatRow {
  a: string;
  va: string;
  b: string;
  vb: string;
}

// 直方图绘图区几何 + 刻度文案（vp 口径；@State 驱动文字叠加层渲染）
interface HistGeo {
  padL: number;
  padR: number;
  padT: number;
  plotW: number;
  plotH: number;
  ticks: string[];   // 5 档频数刻度（自上而下）
  lo: string;
  mid: string;
  hi: string;
}

const N_BINS: number = 10;

function fmt(v: number): string {
  if (!isFinite(v)) {
    return '—';
  }
  const r = Math.round(v * 100) / 100;
  return Number.isInteger(r) ? r.toString() : r.toFixed(2);
}

// 上游 AlgoQ1 原算法（中位数
// 半分法按 n%4 分支；n<2 → NaN 留空）
function quartile1(sorted: number[]): number {
  const n = sorted.length;
  if (n < 2) {
    return NaN;
  }
  switch (n % 4) {
    case 0:
      return (sorted[n / 4 - 1] + sorted[(n + 4) / 4 - 1]) / 2;
    case 1:
      return (sorted[(n - 1) / 4 - 1] + sorted[(n + 3) / 4 - 1]) / 2;
    case 2:
      return sorted[(n + 2) / 4 - 1];
    default:
      return sorted[(n + 1) / 4 - 1];
  }
}

// 上游 AlgoQ3 原算法（与 Q1 同分支、索引自 3n/4 起）
function quartile3(sorted: number[]): number {
  const n = sorted.length;
  if (n < 2) {
    return NaN;
  }
  switch (n % 4) {
    case 0:
      return (sorted[3 * n / 4 - 1] + sorted[(3 * n + 4) / 4 - 1]) / 2;
    case 1:
      return (sorted[(3 * n + 1) / 4 - 1] + sorted[(3 * n + 5) / 4 - 1]) / 2;
    case 2:
      return sorted[(3 * n + 2) / 4 - 1];
    default:
      return sorted[(3 * n + 3) / 4 - 1];
  }
}

// 统计表配色（上游 网格 NEUTRAL_300 / 表头字 NEUTRAL_700 系，深色近似）
function tblGrid(dark: boolean): string {
  return dark ? '#33363B' : '#E6E6EB';
}
function tblNameCol(dark: boolean): string {
  return dark ? '#A8ADB5' : '#6E6D73';
}

@Component
export struct DataAnalysisPanel {
  @Prop dark: boolean = false;
  @Prop isWide: boolean = false;   // 宽屏左栏形态：数据源/图表左右分栏
  // 第二十三包②：数据分析改从数据区选区启动（上游 经典路径 DataSource
  // setDataListFromSelection：打开时快照选区）——prefill 为选区数值序列
  //（逗号分隔），sourceLabel 为来源描述（如 "B1:B8 · 6 个数值"）；打开即
  // 预填并分析，文本框仍可手改（上游 ImportData 同款可编辑数据源）
  @Prop prefill: string = '';
  @Prop sourceLabel: string = '';
  onBack: () => void = () => {};   // 返回数据区（上游 关停靠面板露出表格同义）
  onClose: () => void = () => {};
  @State private dataText: string = '';
  @State private stats: DataStats | undefined = undefined;
  @State private statRows: StatRow[] = [];   // 统计表行（analyze 重建）
  @State private hint: string = '';
  @State private histGeo: HistGeo | undefined = undefined;
  private ctx: CanvasRenderingContext2D =
    new CanvasRenderingContext2D(new RenderingContextSettings(true));
  private panelVpW: number = 0;   // 直方图画布 vp 尺寸（onAreaChange 上报）
  private panelVpH: number = 0;

  aboutToAppear(): void {
    if (this.prefill.length > 0) {
      this.dataText = this.prefill;
      this.analyze();
    }
  }

  // 解析输入（逗号/空格/换行/分号分隔）并统计
  private analyze(): void {
    const nums: number[] = [];
    for (const tok of this.dataText.split(/[,;\s]+/)) {
      if (tok.length === 0) {
        continue;
      }
      const v = parseFloat(tok);
      if (!isNaN(v) && isFinite(v)) {
        nums.push(v);
      }
    }
    if (nums.length === 0) {
      this.stats = undefined;
      this.statRows = [];
      this.histGeo = undefined;
      this.hint = '未解析到数值，请用逗号、空格或换行分隔';
      return;
    }
    const n = nums.length;
    const sum = nums.reduce((a: number, b: number): number => a + b, 0);
    const sumSq = nums.reduce((a: number, b: number): number => a + b * b, 0);
    const mean = sum / n;
    const sorted = [...nums].sort((a: number, b: number): number => a - b);
    const median = n % 2 === 1 ? sorted[(n - 1) / 2]
      : (sorted[n / 2 - 1] + sorted[n / 2]) / 2;
    let ss = 0;
    for (const v of nums) {
      ss += (v - mean) * (v - mean);
    }
    // 上游 表两行口径：SD = 总体（÷n）、Sample SD = 样本（÷(n-1)）；n=1 记 0
    const sdPop = Math.sqrt(ss / n);
    const sdSample = n > 1 ? Math.sqrt(ss / (n - 1)) : 0;
    const min = sorted[0];
    const max = sorted[n - 1];
    const span = max - min;
    const binW = span > 0 ? span / N_BINS : 1;
    const bins: number[] = new Array(N_BINS).fill(0);
    for (const v of nums) {
      let k = span > 0 ? Math.floor((v - min) / binW) : 0;
      if (k >= N_BINS) {
        k = N_BINS - 1;   // 上界入末桶
      }
      bins[k]++;
    }
    this.stats = {
      n: n, mean: mean, median: median,
      sdSample: sdSample, sdPop: sdPop, sum: sum, sumSq: sumSq,
      min: min, max: max, q1: quartile1(sorted), q3: quartile3(sorted),
      bins: bins, binW: binW, lo: min, hi: max
    };
    // 上游 StatTableModel 单变量行序：N Mean SD SampleSD Σx Σx² Min Q1
    // 中位数 Q3 Max（两对名值一行，末行单对留空）
    const s = this.stats;
    this.statRows = [
      { a: 'N', va: fmt(s.n), b: '均值', vb: fmt(s.mean) },
      { a: '标准差', va: fmt(s.sdPop), b: '样本标准差', vb: fmt(s.sdSample) },
      { a: 'Σx', va: fmt(s.sum), b: 'Σx²', vb: fmt(s.sumSq) },
      { a: '最小值', va: fmt(s.min), b: 'Q1', vb: isNaN(s.q1) ? '' : fmt(s.q1) },
      { a: '中位数', va: fmt(s.median), b: 'Q3', vb: isNaN(s.q3) ? '' : fmt(s.q3) },
      { a: '最大值', va: fmt(s.max), b: '', vb: '' }
    ];
    this.hint = '';
    this.drawHistogram(this.stats);
  }

  private histCol(): string {
    return this.dark ? '#9AA0A6' : '#888888';
  }

  // 画直方图：几何按 vp 口径计算，画布内只画网格/柱/轴（按 u 缩放适配
  // 坐标制），刻度与轴端文案进 histGeo 由 build 里的原生 Text 叠加渲染
  private drawHistogram(s: DataStats): void {
    const W = this.ctx.width;
    const H = this.ctx.height;
    if (W <= 1 || H <= 1 || this.panelVpW <= 1 || this.panelVpH <= 1) {
      return;   // Canvas 未就绪；重建时会经 onReady 再画
    }
    const u = this.panelVpW > 1 ? W / this.panelVpW
      : 1;   // 画布单位/vp（本机 vp 制 = 1；物理像素制设备自动 >1）
    const grid = this.dark ? '#33363B' : '#E6E6EB';   // NEUTRAL_300
    const axis = this.dark ? '#8A9099' : '#B4B3BA';   // NEUTRAL_500
    const padL = 20;
    const padR = 10;
    const padT = 12;
    const padB = 24;
    const plotW = this.panelVpW - padL - padR;
    const plotH = this.panelVpH - padT - padB;
    if (plotW <= 10 || plotH <= 10) {
      return;
    }
    const maxCount = Math.max(1, ...s.bins);
    const ticks: string[] = [];
    for (let i = 0; i <= 4; ++i) {
      ticks.push(Math.round(maxCount * (4 - i) / 4).toString());
    }
    this.histGeo = {
      padL: padL, padR: padR, padT: padT, plotW: plotW, plotH: plotH,
      ticks: ticks, lo: fmt(s.lo), mid: fmt((s.lo + s.hi) / 2), hi: fmt(s.hi)
    };
    this.ctx.clearRect(0, 0, W, H);
    const lw = Math.max(1, Math.round(u));
    // 网格线（4 档）
    this.ctx.strokeStyle = grid;
    this.ctx.lineWidth = lw;
    for (let i = 0; i <= 4; ++i) {
      const y = (padT + (plotH * i) / 4) * u;
      this.ctx.beginPath();
      this.ctx.moveTo(padL * u, y);
      this.ctx.lineTo((this.panelVpW - padR) * u, y);
      this.ctx.stroke();
    }
    // 柱
    const slot = plotW / N_BINS;
    const barW = Math.max(2, slot - 3);
    this.ctx.fillStyle = this.dark ? '#8FB6FF' : '#007DFF';
    for (let k = 0; k < N_BINS; ++k) {
      const c = s.bins[k];
      if (c <= 0) {
        continue;
      }
      const h = (plotH * c) / maxCount;
      this.ctx.fillRect((padL + k * slot + (slot - barW) / 2) * u,
        (padT + plotH - h) * u, barW * u, h * u);
    }
    // 轴
    this.ctx.strokeStyle = axis;
    this.ctx.lineWidth = lw;
    this.ctx.beginPath();
    this.ctx.moveTo(padL * u, padT * u);
    this.ctx.lineTo(padL * u, (padT + plotH) * u);
    this.ctx.lineTo((this.panelVpW - padR) * u, (padT + plotH) * u);
    this.ctx.stroke();
  }

  // 统计表（上游 BasicStatTable 同构：两对名值/行、无表头、值列右对齐）。
  // 外层 ForEach 行（真机验证过安全），行内 4 格字面渲染（内层 ForEach
  // 有丢项前科）；key 含值——@Builder/ForEach 按值传参不刷新（轮 B 铁律），
  // 键变即整行重建
  @Builder
  private statTable() {
    Column() {
      ForEach(this.statRows, (row: StatRow) => {
        Row() {
          Text(row.a)
            .fontSize(11)
            .maxLines(1)
            .fontColor(tblNameCol(this.dark))
            .textAlign(TextAlign.End)
            .layoutWeight(3)
            .height(26)
            .padding({ left: 4, right: 6 })
            .border({ width: 0.5, color: tblGrid(this.dark) })
          Text(row.va)
            .fontSize(11)
            .maxLines(1)
            .fontColor(Theme.text(this.dark))
            .textAlign(TextAlign.End)
            .layoutWeight(2)
            .height(26)
            .padding({ left: 4, right: 8 })
            .border({ width: 0.5, color: tblGrid(this.dark) })
          Text(row.b)
            .fontSize(11)
            .maxLines(1)
            .fontColor(tblNameCol(this.dark))
            .textAlign(TextAlign.End)
            .layoutWeight(3)
            .height(26)
            .padding({ left: 4, right: 6 })
            .border({ width: 0.5, color: tblGrid(this.dark) })
          Text(row.vb)
            .fontSize(11)
            .maxLines(1)
            .fontColor(Theme.text(this.dark))
            .textAlign(TextAlign.End)
            .layoutWeight(2)
            .height(26)
            .padding({ left: 4, right: 8 })
            .border({ width: 0.5, color: tblGrid(this.dark) })
        }
        .width('100%')
      }, (row: StatRow) => row.a + '=' + row.va + '#' + row.b + '=' + row.vb)
    }
    .width('100%')
    .margin({ left: 12, right: 12, bottom: 6 })
    .border({ width: 1, color: tblGrid(this.dark) })
    .borderRadius(6)
    .clip(true)
  }

  // 直方图卡片（上游 PlotPanel 白底 + 边框；几何绘制同第六包文案叠加方案）
  @Builder
  private plotCard() {
    Stack({ alignContent: Alignment.TopStart }) {
      Canvas(this.ctx)
        .width('100%')
        .height('100%')
        .onAreaChange((_o: Area, n: Area) => {
          this.panelVpW = n.width as number;
          this.panelVpH = n.height as number;
          if (this.stats !== undefined) {
            this.drawHistogram(this.stats);
          }
        })
        .onReady(() => {
          if (this.stats !== undefined) {
            this.drawHistogram(this.stats);
          }
        })
      if (this.histGeo !== undefined) {
        // 频数刻度 5 档（右对齐网格线左缘，垂直居中于线上）
        Text(this.histGeo.ticks[0])
          .fontSize(12)
          .fontColor(this.histCol())
          .width(this.histGeo.padL - 6)
          .textAlign(TextAlign.End)
          .position({ x: 2, y: this.histGeo.padT - 9 })
        Text(this.histGeo.ticks[1])
          .fontSize(12)
          .fontColor(this.histCol())
          .width(this.histGeo.padL - 6)
          .textAlign(TextAlign.End)
          .position({ x: 2, y: this.histGeo.padT + this.histGeo.plotH * 0.25 - 9 })
        Text(this.histGeo.ticks[2])
          .fontSize(12)
          .fontColor(this.histCol())
          .width(this.histGeo.padL - 6)
          .textAlign(TextAlign.End)
          .position({ x: 2, y: this.histGeo.padT + this.histGeo.plotH * 0.5 - 9 })
        Text(this.histGeo.ticks[3])
          .fontSize(12)
          .fontColor(this.histCol())
          .width(this.histGeo.padL - 6)
          .textAlign(TextAlign.End)
          .position({ x: 2, y: this.histGeo.padT + this.histGeo.plotH * 0.75 - 9 })
        Text(this.histGeo.ticks[4])
          .fontSize(12)
          .fontColor(this.histCol())
          .width(this.histGeo.padL - 6)
          .textAlign(TextAlign.End)
          .position({ x: 2, y: this.histGeo.padT + this.histGeo.plotH - 9 })
        // 轴端标注（下界 / 中值 / 上界）
        Text(this.histGeo.lo)
          .fontSize(12)
          .fontColor(this.histCol())
          .width(60)
          .textAlign(TextAlign.Start)
          .position({ x: this.histGeo.padL,
            y: this.histGeo.padT + this.histGeo.plotH + 5 })
        Text(this.histGeo.mid)
          .fontSize(12)
          .fontColor(this.histCol())
          .width(80)
          .textAlign(TextAlign.Center)
          .position({ x: this.panelVpW / 2 - 40,
            y: this.histGeo.padT + this.histGeo.plotH + 5 })
        Text(this.histGeo.hi)
          .fontSize(12)
          .fontColor(this.histCol())
          .width(60)
          .textAlign(TextAlign.End)
          .position({ x: this.panelVpW - this.histGeo.padR - 60,
            y: this.histGeo.padT + this.histGeo.plotH + 5 })
      }
    }
    .width('100%')
    .layoutWeight(1)
    .margin({ top: 4, left: 12, right: 12, bottom: 6 })
    .border({ width: 1, color: tblGrid(this.dark) })
    .borderRadius(6)
    .clip(true)
    .backgroundColor(this.dark ? '#202226' : '#FFFFFF')
  }

  // 数据源输入块（上游 左数据源栏同位：标题 + 多行 TextArea + 分析按钮；
  // 单根 Column——@Builder 多根在 Row 容器里会横排，故不拆）
  @Builder
  private sourceArea(buttonFull: boolean) {
    Column({ space: 4 }) {
      Text('数据')
        .fontSize(12)
        .fontColor(tblNameCol(this.dark))
        .width('100%')
      TextArea({ text: this.dataText,
        placeholder: '数值序列（逗号/空格/换行分隔），如 12, 15, 18, 22 …' })
        .width('100%')
        .height(72)
        .fontSize(12)
        .backgroundColor(Theme.inputBg(this.dark))
        .onChange((v: string) => {
          this.dataText = v;
        })
      Row() {
        Text('分析')
          .fontSize(13)
          .fontColor('#FFFFFF')
          .backgroundColor(Theme.accent(this.dark))
          .borderRadius(8)
          .height(32)
          .padding({ left: 14, right: 14 })
          .textAlign(TextAlign.Center)
          .onClick(() => this.analyze())
      }
      .width('100%')
      .justifyContent(buttonFull ? FlexAlign.Start : FlexAlign.End)
    }
    .width('100%')
    .alignItems(HorizontalAlign.Start)
  }

  build() {
    Column() {
      Row() {
        if (this.sourceLabel.length > 0) {
          // 选区启动（非手输）：返回数据区（上游 关面板露表格同义）
          Text('←')
            .fontSize(14)
            .fontColor(Theme.sub(this.dark))
            .width(30)
            .height(30)
            .textAlign(TextAlign.Center)
            .margin({ left: -8 })
            .onClick(() => this.onBack())
        }
        Text('数据分析 · 单变量')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.text(this.dark))
          .layoutWeight(1)
        Text(this.sourceLabel.length > 0 ? this.sourceLabel
          : (this.hint.length > 0 ? this.hint
            : (this.stats !== undefined ? `N=${this.stats.n}` : '输入数值后点"分析"')))
          .fontSize(11)
          .fontColor(Theme.faint(this.dark))
          .maxLines(1)
          .margin({ right: 8 })
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

      // 上游 DataAnalysisViewW：宽屏左数据源栏 + 右绘图（上），统计表跨栏
      // 沉底全宽（dock 栏 420vp 放不下 上游 300px 左栏 + 右栏统计表四列，
      // 沉底后统计表取满面板宽、名值列不再换行）；窄屏纵向堆叠同序
      //（录入→图→表）
      if (this.isWide) {
        Column() {
          Row() {
            Column() {
              this.sourceArea(true)
            }
            .width(240)
            .height('100%')
            .padding({ left: 12, right: 12, top: 2 })
            .alignItems(HorizontalAlign.Start)

            Column() {
              this.plotCard()
            }
            .layoutWeight(1)
            .height('100%')
          }
          .width('100%')
          .layoutWeight(1)

          if (this.statRows.length > 0) {
            this.statTable()
          }
        }
        .width('100%')
        .layoutWeight(1)
      } else {
        // 二十四包③：窄屏内容包进滚动——录入(约110vp)+直方图+统计表
        // (约180vp) 固定高合计超出抽屉档位，原先整块下溢、叠到底部 Tab 栏
        // 上（上游 手机端同场景即可滚动）
        Scroll() {
          Column() {
            Column() {
              this.sourceArea(false)
            }
            .width('100%')
            .padding({ left: 12, right: 12, top: 2 })

            // 直方图定高：Scroll 内 layoutWeight 不生效，固定 210vp
            Column() {
              this.plotCard()
            }
            .width('100%')
            .height(210)

            if (this.statRows.length > 0) {
              this.statTable()
            }
          }
          .width('100%')
        }
        .scrollBar(BarState.Auto)
        .width('100%')
        .layoutWeight(1)
        .align(Alignment.Top)
      }
    }
    .width('100%')
    .height('100%')
  }
}
