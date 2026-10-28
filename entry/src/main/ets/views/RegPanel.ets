// RegPanel.ets — 回归分析面板（1.0.7.1，二十三包积压：GGB 表头列菜单回归
// 入口）。数据区列头菜单「回归分析…」以当列为因变量 Y 打开本面板，X 列在
// 面板内选；模型集与 GGB 回归分析对话框同集（线性/多项式/对数/幂/指数/
// 增长/正弦）。
// 「分析并创建」= kernelInput 固定标签 regFit 重定义：Fit 命令收 {…} 字面量
//（GGB 同款内嵌数据式），重跑同标签重定义不留垃圾对象；曲线对象进构造
//（代数区/画布可见，撤销一步可撤）。方程取代数区 value 列，R² 由数据与
// 曲线值（kernelTableEval）在宿主计算。散点+曲线小图画在本面板私有 Canvas
//（DataPanel 直方图同款：几何按 vp 口径、文字走原生 Text 叠加）。
// 偏差（v1）：X/Y 配对取两列行号相同的数值格（GGB 选区可任意框选）；数据
// 进表达式为字面量静态定型，改单元格不回灌（重跑一次分析即重建）。
import { AlgebraEntry } from '../model/GeoTypes';
import { Theme } from '../model/Theme';
import dreamemberKernel from 'libdreamember.so';

const N_ROWS: number = 40;
const FIT_LABEL: string = 'regFit';

// 模型定义（id / 显示名 / 内核命令 / 最少成对数；poly 带阶数）
interface RegModel {
  id: string;
  name: string;
  cmd: string;
  deg: number;      // 0 = 无阶数参
  minPts: number;
}

const MODELS: RegModel[] = [
  { id: 'linear', name: '线性', cmd: 'FitLine', deg: 0, minPts: 2 },
  { id: 'poly2', name: '多项式2', cmd: 'FitPoly', deg: 2, minPts: 3 },
  { id: 'poly3', name: '多项式3', cmd: 'FitPoly', deg: 3, minPts: 4 },
  { id: 'poly4', name: '多项式4', cmd: 'FitPoly', deg: 4, minPts: 5 },
  { id: 'log', name: '对数', cmd: 'FitLog', deg: 0, minPts: 2 },
  { id: 'pow', name: '幂', cmd: 'FitPow', deg: 0, minPts: 2 },
  { id: 'exp', name: '指数', cmd: 'FitExp', deg: 0, minPts: 2 },
  { id: 'growth', name: '增长', cmd: 'FitGrowth', deg: 0, minPts: 2 },
  { id: 'sin', name: '正弦', cmd: 'FitSin', deg: 0, minPts: 4 }
];

const COLS: string[] = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'];

// 绘图区几何 + 轴端文案（vp 口径；@State 驱动文字叠加层渲染）
interface PlotGeo {
  padL: number;
  padR: number;
  padT: number;
  plotW: number;
  plotH: number;
  x0: string;
  x1: string;
  y0: string;
  y1: string;
}

function fmt(v: number): string {
  if (!isFinite(v)) {
    return '—';
  }
  const r = Math.round(v * 100) / 100;
  return Number.isInteger(r) ? r.toString() : r.toFixed(2);
}

// R² 专用：两位小数会把 0.9996 舍成整数 1 丢信息（1.0.7.1 备查②）——
// 4 位小数去尾零；仅 R² 使用，坐标轴范围标签仍走 fmt
function fmtR2(v: number): string {
  if (!isFinite(v)) {
    return '—';
  }
  const s = v.toFixed(4);
  const t = s.replace(/0+$/, '').replace(/\.$/, '');
  return t.length > 0 && t !== '-' ? t : '0';
}

@Component
export struct RegPanel {
  @Link algebra: AlgebraEntry[];   // 宿主代数区（取 regFit 方程文本）
  @Prop dark: boolean = false;
  @Prop isWide: boolean = false;
  @Prop yCol: string = 'A';        // 因变量列（数据区列头菜单快照）
  // 提交拟合表达式（宿主 CanvasView.input 全管线：撤销打点+代数同步+重绘）。
  // 返回错误（空串=成功）
  onFit: (expr: string) => string = (_e: string) => '';
  onBack: () => void = () => {};   // 返回数据区
  onClose: () => void = () => {};

  @State private xCol: string = 'A';
  @State private modelId: string = 'linear';
  @State private eq: string = '';
  @State private r2Text: string = '';
  @State private hint: string = '';
  @State private nPts: number = 0;
  @State private pts: number[] = [];       // 数据点 (x,y) 平铺
  @State private curve: number[] = [];     // 曲线采样 (x,y) 平铺
  @State private bounds: number[] = [0, 1, 0, 1];   // xmin,xmax,ymin,ymax
  @State private geo: PlotGeo | undefined = undefined;
  private ctx: CanvasRenderingContext2D =
    new CanvasRenderingContext2D(new RenderingContextSettings(true));
  private panelVpW: number = 0;
  private panelVpH: number = 0;

  // 收集 X/Y 两列成对数值（行号对齐；真值走 kernelObjValue 绕开显示精度）
  private collectPairs(): number[] {
    const out: number[] = [];
    for (let r = 1; r <= N_ROWS; ++r) {
      const xv = dreamemberKernel.kernelObjValue(this.xCol + r.toString());
      const yv = dreamemberKernel.kernelObjValue(this.yCol + r.toString());
      if (isFinite(xv) && isFinite(yv)) {
        out.push(xv, yv);
      }
    }
    return out;
  }

  // 分析并创建：字面量实参拟合命令 → 固定标签 regFit 重定义
  private analyze(): void {
    const model = MODELS.find((m: RegModel): boolean => m.id === this.modelId);
    if (model === undefined) {
      return;
    }
    const pts = this.collectPairs();
    const n = pts.length / 2;
    if (n < model.minPts) {
      this.hint = `${model.name}回归需要至少 ${model.minPts} 对 ${this.xCol}/${this.yCol} 成对数值`;
      return;
    }
    const xs: string[] = [];
    const ys: string[] = [];
    for (let i = 0; i < pts.length; i += 2) {
      xs.push(pts[i].toString());
      ys.push(pts[i + 1].toString());
    }
    const deg = model.deg > 0 ? ',' + model.deg.toString() : '';
    const expr = `${FIT_LABEL}=${model.cmd}({${xs.join(',')}},{${ys.join(',')}}${deg})`;
    const err = this.onFit(expr);
    if (err.length > 0) {
      this.hint = err;
      return;
    }
    this.hint = '';
    this.pts = pts;
    this.nPts = n;
    // 方程：kernelAlgebra 行协议直取（label|type|value|def|…）——@Link 代数
    // 数组在本 tick 的同步读不保证已含新建对象（实测 eq 偶发为空）
    let eqText = '';
    const algText: string = dreamemberKernel.kernelAlgebra();
    const algLines: string[] = algText.split(String.fromCharCode(10));
    for (const line of algLines) {
      const cols = line.split('|');
      if (cols.length >= 3 && cols[0] === FIT_LABEL) {
        eqText = cols[2];
        break;
      }
    }
    const pfx = FIT_LABEL + '(x) = ';
    this.eq = eqText.startsWith(pfx) ? eqText.substring(pfx.length) : eqText;
    this.computeFitStats(pts);
  }

  // R²（1 − SSres/SStot）+ 曲线采样（数据 x 范围 60 点；kernelTableEval
  // 按 FormatNum 精度返回，绘图/R² 精度足够）
  private computeFitStats(pts: number[]): void {
    const n = pts.length / 2;
    const xsCsv: string[] = [];
    for (let i = 0; i < pts.length; i += 2) {
      xsCsv.push(pts[i].toString());
    }
    const fitRaw: string = dreamemberKernel.kernelTableEval(FIT_LABEL, xsCsv.join(','));
    const fit: string[] = fitRaw.length > 0 ? fitRaw.split(';') : [];
    let ssr = 0;
    let sst = 0;
    let mean = 0;
    for (let i = 0; i < n; ++i) {
      mean += pts[2 * i + 1];
    }
    mean /= n;
    let ok = true;
    for (let i = 0; i < n; ++i) {
      const fv = fit.length > i ? parseFloat(fit[i]) : NaN;
      if (!isFinite(fv)) {
        ok = false;
        break;
      }
      const dv = pts[2 * i + 1] - fv;
      ssr += dv * dv;
      const dt = pts[2 * i + 1] - mean;
      sst += dt * dt;
    }
    this.r2Text = ok && sst > 1e-15 ? fmtR2(1 - ssr / sst) : '—';
    // 曲线采样
    let x0 = pts[0];
    let x1 = pts[0];
    for (let i = 0; i < pts.length; i += 2) {
      x0 = Math.min(x0, pts[i]);
      x1 = Math.max(x1, pts[i]);
    }
    if (!(x1 > x0)) {
      x0 -= 1;
      x1 += 1;
    }
    const sweep: string[] = [];
    for (let k = 0; k < 60; ++k) {
      sweep.push((x0 + (x1 - x0) * k / 59).toString());
    }
    const curveRaw: string = dreamemberKernel.kernelTableEval(FIT_LABEL, sweep.join(','));
    const cv: string[] = curveRaw.length > 0 ? curveRaw.split(';') : [];
    const curve: number[] = [];
    for (let k = 0; k < 60 && k < cv.length; ++k) {
      const yv = parseFloat(cv[k]);
      if (isFinite(yv)) {
        curve.push(x0 + (x1 - x0) * k / 59, yv);
      }
    }
    this.curve = curve;
    this.computeBounds();
    this.drawPlot();
  }

  private computeBounds(): void {
    let x0 = Infinity;
    let x1 = -Infinity;
    let y0 = Infinity;
    let y1 = -Infinity;
    const consider = (arr: number[]): void => {
      for (let i = 0; i + 1 < arr.length; i += 2) {
        x0 = Math.min(x0, arr[i]);
        x1 = Math.max(x1, arr[i]);
        y0 = Math.min(y0, arr[i + 1]);
        y1 = Math.max(y1, arr[i + 1]);
      }
    };
    consider(this.pts);
    consider(this.curve);
    if (!isFinite(x0)) {
      x0 = 0;
      x1 = 1;
      y0 = 0;
      y1 = 1;
    }
    const dx = (x1 - x0) * 0.08 + 1e-9;
    const dy = (y1 - y0) * 0.08 + 1e-9;
    this.bounds = [x0 - dx, x1 + dx, y0 - dy, y1 + dy];
  }

  private axisCol(): string {
    return this.dark ? '#8A9099' : '#B4B3BA';
  }

  // 散点 + 拟合曲线（几何 vp 口径，坐标制画布按 u 缩放；文字走叠加层）
  private drawPlot(): void {
    const W = this.ctx.width;
    const H = this.ctx.height;
    if (W <= 1 || H <= 1 || this.panelVpW <= 1 || this.panelVpH <= 1) {
      return;
    }
    const u = W / this.panelVpW;
    const grid = this.dark ? '#33363B' : '#E6E6EB';
    const padL = 34;
    const padR = 10;
    const padT = 10;
    const padB = 22;
    const plotW = this.panelVpW - padL - padR;
    const plotH = this.panelVpH - padT - padB;
    if (plotW <= 10 || plotH <= 10) {
      return;
    }
    const bx = this.bounds;
    const px = (x: number): number =>
      (padL + (x - bx[0]) / (bx[1] - bx[0]) * plotW) * u;
    const py = (y: number): number =>
      (padT + (bx[3] - y) / (bx[3] - bx[2]) * plotH) * u;
    this.geo = {
      padL: padL, padR: padR, padT: padT, plotW: plotW, plotH: plotH,
      x0: fmt(bx[0]), x1: fmt(bx[1]), y0: fmt(bx[2]), y1: fmt(bx[3])
    };
    this.ctx.clearRect(0, 0, W, H);
    // 边框 + 中线网格
    this.ctx.strokeStyle = grid;
    this.ctx.lineWidth = Math.max(1, Math.round(u));
    this.ctx.strokeRect(padL * u, padT * u, plotW * u, plotH * u);
    // 曲线
    if (this.curve.length >= 4) {
      this.ctx.strokeStyle = this.dark ? '#8FB6FF' : '#007DFF';
      this.ctx.lineWidth = Math.max(1.5, 1.7 * u);
      this.ctx.beginPath();
      this.ctx.moveTo(px(this.curve[0]), py(this.curve[1]));
      for (let i = 2; i + 1 < this.curve.length; i += 2) {
        this.ctx.lineTo(px(this.curve[i]), py(this.curve[i + 1]));
      }
      this.ctx.stroke();
    }
    // 散点
    this.ctx.fillStyle = this.dark ? '#E8EAED' : '#1C1C1F';
    for (let i = 0; i + 1 < this.pts.length; i += 2) {
      this.ctx.beginPath();
      this.ctx.arc(px(this.pts[i]), py(this.pts[i + 1]), 2.5 * u, 0, 6.2832);
      this.ctx.fill();
    }
  }

  build() {
    Column() {
      Row() {
        Text('←')
          .fontSize(14)
          .fontColor(Theme.sub(this.dark))
          .width(30)
          .height(30)
          .textAlign(TextAlign.Center)
          .margin({ left: -8 })
          .onClick(() => this.onBack())
        Text(`回归分析 · Y = 列 ${this.yCol}`)
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.text(this.dark))
          .layoutWeight(1)
        Text(this.hint.length > 0 ? this.hint
          : (this.eq.length > 0 ? `n=${this.nPts} · R²=${this.r2Text}` : '选 X 列与模型'))
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

      Scroll() {
        Column() {
          // X 列选择（因变量 Y 已由入口定死）
          Row() {
            Text('X 列')
              .fontSize(12)
              .fontColor(this.dark ? '#A8ADB5' : '#6E6D73')
              .width(38)
            ForEach(COLS, (c: string) => {
              Text(c)
                .fontSize(12)
                .fontColor(this.xCol === c ? '#FFFFFF' : Theme.text(this.dark))
                .backgroundColor(this.xCol === c ? Theme.accent(this.dark)
                  : (this.dark ? '#33363B' : '#E9EBF2'))
                .borderRadius(12)
                .height(24)
                .padding({ left: 10, right: 10 })
                .margin({ right: 6 })
                .onClick(() => {
                  if (this.xCol !== c) {
                    this.xCol = c;
                  }
                })
            }, (c: string) => 'xcol' + c)
          }
          .width('100%')
          .margin({ top: 4 })

          // 模型选择（GGB 回归分析对话框同集）
          Flex({ wrap: FlexWrap.Wrap }) {
            ForEach(MODELS, (m: RegModel) => {
              Text(m.name)
                .fontSize(12)
                .fontColor(this.modelId === m.id ? '#FFFFFF' : Theme.text(this.dark))
                .backgroundColor(this.modelId === m.id ? Theme.accent(this.dark)
                  : (this.dark ? '#33363B' : '#E9EBF2'))
                .borderRadius(12)
                .height(24)
                .padding({ left: 10, right: 10 })
                .margin({ right: 6, top: 6 })
                .onClick(() => {
                  if (this.modelId !== m.id) {
                    this.modelId = m.id;
                  }
                })
            }, (m: RegModel) => 'm' + m.id)
          }
          .width('100%')
          .margin({ top: 6 })

          Row() {
            Text('分析并创建拟合曲线')
              .fontSize(13)
              .fontColor('#FFFFFF')
              .backgroundColor(Theme.accent(this.dark))
              .borderRadius(8)
              .height(32)
              .padding({ left: 14, right: 14 })
              .onClick(() => this.analyze())
            Blank()
            Text('曲线对象 regFit 进构造，撤销一步可撤')
              .fontSize(10)
              .fontColor(Theme.faint(this.dark))
          }
          .width('100%')
          .margin({ top: 10 })

          if (this.hint.length > 0) {
            Text(this.hint)
              .fontSize(11)
              .fontColor(Theme.error(this.dark))
              .width('100%')
              .margin({ top: 6 })
          }

          // 散点 + 拟合曲线图（DataPanel 直方图同款卡片）
          Stack({ alignContent: Alignment.TopStart }) {
            Canvas(this.ctx)
              .width('100%')
              .height('100%')
              .onAreaChange((_o: Area, n: Area) => {
                this.panelVpW = n.width as number;
                this.panelVpH = n.height as number;
                if (this.pts.length > 0) {
                  this.drawPlot();
                }
              })
              .onReady(() => {
                if (this.pts.length > 0) {
                  this.drawPlot();
                }
              })
            if (this.geo !== undefined) {
              Text(this.geo.y1)
                .fontSize(10)
                .fontColor(this.axisCol())
                .width(this.geo.padL - 4)
                .textAlign(TextAlign.End)
                .position({ x: 2, y: this.geo.padT - 6 })
              Text(this.geo.y0)
                .fontSize(10)
                .fontColor(this.axisCol())
                .width(this.geo.padL - 4)
                .textAlign(TextAlign.End)
                .position({ x: 2, y: this.geo.padT + this.geo.plotH - 12 })
              Text(this.geo.x0)
                .fontSize(10)
                .fontColor(this.axisCol())
                .width(60)
                .textAlign(TextAlign.Start)
                .position({ x: this.geo.padL, y: this.geo.padT + this.geo.plotH + 4 })
              Text(this.geo.x1)
                .fontSize(10)
                .fontColor(this.axisCol())
                .width(60)
                .textAlign(TextAlign.End)
                .position({ x: this.geo.padL + this.geo.plotW - 60,
                  y: this.geo.padT + this.geo.plotH + 4 })
            }
          }
          .width('100%')
          .height(190)
          .margin({ top: 10 })
          .border({ width: 1, color: this.dark ? '#33363B' : '#E6E6EB' })
          .borderRadius(6)
          .clip(true)
          .backgroundColor(this.dark ? '#202226' : '#FFFFFF')

          if (this.eq.length > 0) {
            Column() {
              Text('拟合曲线')
                .fontSize(11)
                .fontColor(this.dark ? '#A8ADB5' : '#6E6D73')
                .width('100%')
              Text(this.eq)
                .fontSize(12)
                .fontColor(Theme.text(this.dark))
                .width('100%')
                .margin({ top: 2 })
              Text(`R² = ${this.r2Text} · n = ${this.nPts}（X=列${this.xCol}，Y=列${this.yCol}）`)
                .fontSize(11)
                .fontColor(Theme.faint(this.dark))
                .width('100%')
                .margin({ top: 2 })
            }
            .width('100%')
            .margin({ top: 10, bottom: 8 })
            .padding(10)
            .borderRadius(6)
            .border({ width: 1, color: this.dark ? '#33363B' : '#E6E6EB' })
          }
        }
        .width('100%')
        .padding({ left: 12, right: 12, top: 2 })
      }
      .scrollBar(BarState.Auto)
      .edgeEffect(EdgeEffect.Spring)
      .width('100%')
      .layoutWeight(1)
      .align(Alignment.Top)
    }
    .width('100%')
    .height('100%')
  }
}
