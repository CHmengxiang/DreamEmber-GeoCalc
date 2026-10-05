// ToolIcons.ets — 工具图标 Canvas 矢量绘制（二十七包③：上游 风格重绘）。
// 二十八包①②③按真机反馈修三枚：射线去箭头（与向量区分）、平移箭头
// 让位终点圆点、切线重绘为圆+切线本像。
// 背景：工具面板图标原为 Unicode 字形（ƒ✛▥▦…），字形风格杂、部分工具凑不
// 出贴切字形。本文件按 上游 工具图标的视觉语言**原创**绘制（细黑描边 +
// 蓝色点标记 + 虚线构造线 + 浅蓝填充面；上游 图形资产不可复制，仅风格对齐，
// 全部路径为本文件自绘）。画布坐标系 = vp（Canvas 坐标/measureText 为 vp
// 口径、仅 font-size 是物理 px——本绘制不含文字，无尺度坑）。
// 选中态：底色走工具格的 accent 背景，本绘制收 selected 用白墨重画。

// 每个图标独立 @Component 实例：@Prop 变化（主题翻转/选中切换）经 @Watch
// 触发重绘——Canvas onReady 只在挂载时回调一次，靠组件内自持 ctx 重画。
@Component
export struct ToolIcon {
  // 命名 tid：id 与 ArkUI 通用属性 id() 冲突（CustomComponent 基类口径）
  @Prop tid: string = '';
  @Prop @Watch('onRedraw') dark: boolean = false;
  @Prop @Watch('onRedraw') selected: boolean = false;
  private settings: RenderingContextSettings = new RenderingContextSettings(true);
  private ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.settings);

  private onRedraw(): void {
    drawToolIcon(this.ctx, this.tid, 42, 36, this.dark, this.selected);
  }

  build() {
    Canvas(this.ctx)
      .width(42)
      .height(36)
      .onReady(() => this.onRedraw())
  }
}

export function drawToolIcon(ctx: CanvasRenderingContext2D, id: string, w: number,
  h: number, dark: boolean, selected: boolean): void {
  const ink: string = selected ? '#FFFFFF' : (dark ? '#DCE1E6' : '#33383D');
  const blue: string = selected ? '#FFFFFF' : '#2F6BDF';
  const soft: string = selected ? 'rgba(255,255,255,0.40)' : 'rgba(47,107,223,0.26)';
  ctx.clearRect(0, 0, w, h);
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';

  const seg = (x1: number, y1: number, x2: number, y2: number, lw: number): void => {
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.lineWidth = lw;
    ctx.strokeStyle = ink;
    ctx.stroke();
  };
  const dash = (x1: number, y1: number, x2: number, y2: number, lw: number): void => {
    ctx.setLineDash([3, 3]);
    seg(x1, y1, x2, y2, lw);
    ctx.setLineDash([]);
  };
  const dot = (x: number, y: number, r: number): void => {
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.fillStyle = blue;
    ctx.fill();
    ctx.lineWidth = 1.3;
    ctx.strokeStyle = ink;
    ctx.stroke();
  };
  const arrow = (x1: number, y1: number, x2: number, y2: number): void => {
    const a = Math.atan2(y2 - y1, x2 - x1);
    const s = 6.5;
    ctx.beginPath();
    ctx.moveTo(x2, y2);
    ctx.lineTo(x2 - s * Math.cos(a - 0.44), y2 - s * Math.sin(a - 0.44));
    ctx.lineTo(x2 - s * Math.cos(a + 0.44), y2 - s * Math.sin(a + 0.44));
    ctx.closePath();
    ctx.fillStyle = ink;
    ctx.fill();
  };
  const shape = (pts: number[], fill: string, lw: number, closed: boolean): void => {
    ctx.beginPath();
    ctx.moveTo(pts[0], pts[1]);
    for (let i = 2; i < pts.length; i += 2) {
      ctx.lineTo(pts[i], pts[i + 1]);
    }
    if (closed) {
      ctx.closePath();
    }
    if (fill.length > 0) {
      ctx.fillStyle = fill;
      ctx.fill();
    }
    ctx.lineWidth = lw;
    ctx.strokeStyle = ink;
    ctx.stroke();
  };
  const arcStroke = (cx: number, cy: number, r: number, a0: number, a1: number,
    lw: number): void => {
    ctx.beginPath();
    ctx.arc(cx, cy, r, a0, a1);
    ctx.lineWidth = lw;
    ctx.strokeStyle = ink;
    ctx.stroke();
  };

  if (id === '') {
    // 选择/移动：指针箭头（自绘光标形）
    shape([15, 5, 15, 27, 20.5, 22, 23.5, 29, 26.5, 27.5, 23.5, 20.5, 29, 20],
      ink, 1.2, true);
  } else if (id === 'point') {
    dot(21, 18, 5);
  } else if (id === 'segment') {
    seg(10, 26, 32, 10, 2);
    dot(10, 26, 3.4);
    dot(32, 10, 3.4);
  } else if (id === 'line') {
    seg(5, 29.5, 37, 6.5, 2);
    dot(14, 24, 3.4);
    dot(28, 12, 3.4);
  } else if (id === 'ray') {
    // 二十八包①：去箭头（原与向量同款箭头难区分）。上游 口径：射线=一个
    // 起点圆点 + 延伸到画布边的半线（线=两个内部点、线段=两个端点）
    seg(9, 27, 37, 6.5, 2);
    dot(9, 27, 3.4);
  } else if (id === 'vector') {
    seg(9, 27, 30, 8.5, 2);
    arrow(9, 27, 33, 6);
    dot(9, 27, 3);
  } else if (id === 'circle') {
    ctx.beginPath();
    ctx.arc(20, 18, 11, 0, Math.PI * 2);
    ctx.lineWidth = 2;
    ctx.strokeStyle = ink;
    ctx.stroke();
    dot(20, 18, 3);
    dot(27.8, 10.2, 3);
  } else if (id === 'midpoint') {
    seg(7, 27, 35, 9, 2);
    dot(8.5, 26.2, 3);
    dot(33.5, 9.8, 3);
    dot(21, 18, 4.4);
  } else if (id === 'perpBisector') {
    seg(6, 25, 36, 25, 2);
    dot(8, 25, 3);
    dot(34, 25, 3);
    seg(21, 5, 21, 31, 2);
    shape([16.5, 25, 16.5, 20.5, 21, 20.5], '', 1.4, false);
    dot(21, 25, 3.2);
  } else if (id === 'angleBisector') {
    seg(12, 28, 37, 28, 2);
    seg(12, 28, 30, 6, 2);
    dash(12, 28, 34.5, 18, 1.6);
    arcStroke(12, 28, 7, -0.87, 0, 1.4);
    dot(35, 28, 3);
    dot(29.5, 7.5, 3);
  } else if (id === 'angle') {
    seg(12, 29, 37, 29, 2);
    seg(12, 29, 25, 6, 2);
    arcStroke(12, 29, 10, -0.99, 0, 1.8);
    dot(12, 29, 3);
    dot(35, 29, 3);
    dot(24.2, 7.6, 3);
  } else if (id === 'polygon') {
    shape([21, 5, 8, 29, 34, 29], soft, 2, true);
    dot(21, 5, 2.8);
    dot(8, 29, 2.8);
    dot(34, 29, 2.8);
  } else if (id === 'intersect') {
    seg(7, 25, 35, 11, 2);
    seg(14, 9, 28, 27, 2);
    dot(21, 18, 4.4);
  } else if (id === 'tangent') {
    // 二十八包③：重绘为「圆 + 切线」本像（原为抛物线弧+割线，不像切线）。
    // 圆下沿 y=24 与直线相切，切点圆点
    ctx.beginPath();
    ctx.arc(19, 15.5, 8.5, 0, Math.PI * 2);
    ctx.lineWidth = 2;
    ctx.strokeStyle = ink;
    ctx.stroke();
    seg(5, 24, 37, 24, 2);
    dot(19, 24, 3.2);
  } else if (id === 'translate') {
    // 二十八包②：位移箭头离终点圆点让位 3vp（原箭头尖被圆点压住不可见）
    dash(7, 28, 34, 9, 1.4);
    seg(10.5, 25.5, 24, 16, 2.4);
    arrow(10.5, 25.5, 28.3, 13);
    dot(7, 28, 3.2);
    dot(34, 9, 3.2);
  } else if (id === 'reflect') {
    shape([7, 9, 7, 25, 15, 25], soft, 1.8, true);
    shape([35, 9, 35, 25, 27, 25], soft, 1.8, true);
    dash(21, 4, 21, 32, 1.6);
  } else if (id === 'reflectPoint') {
    dash(9, 9, 33, 27, 1.6);
    dot(9, 9, 3.4);
    dot(33, 27, 3.4);
    dot(21, 18, 3);
  } else if (id === 'perpLine') {
    seg(6, 26, 36, 26, 2);
    seg(23, 5, 23, 31, 2);
    shape([18.5, 26, 18.5, 21.5, 23, 21.5], '', 1.4, false);
    dot(23, 26, 3.2);
    dot(9, 26, 2.6);
    dot(34, 26, 2.6);
  } else if (id === 'parLine') {
    seg(6, 24, 28, 6, 2);
    seg(14, 30, 36, 12, 2);
    dot(12, 19.1, 3);
    dot(27, 19.3, 3);
  } else if (id === 'vecFromPoint') {
    seg(8, 27, 21, 14, 2);
    arrow(8, 27, 24, 11.2);
    seg(18, 27, 31, 14, 2);
    arrow(18, 27, 34, 11.2);
    dot(8, 27, 2.6);
    dot(18, 27, 2.6);
  } else if (id === 'dist') {
    seg(9, 26, 33, 10, 2);
    seg(6.8, 27.3, 11.2, 24.7, 1.5);
    seg(30.8, 11.3, 35.2, 8.7, 1.5);
    dot(9, 26, 3.2);
    dot(33, 10, 3.2);
  } else if (id === 'area') {
    shape([8, 10, 34, 7, 36, 26, 10, 30], soft, 2, true);
    dot(8, 10, 2.7);
    dot(34, 7, 2.7);
    dot(36, 26, 2.7);
    dot(10, 30, 2.7);
  } else if (id === 'slope') {
    seg(6, 30, 36, 9, 2);
    shape([14, 24.4, 26, 24.4, 26, 16.7], soft, 1.5, true);
  } else if (id === 'segLength') {
    seg(9, 27, 33, 9, 2);
    dot(9, 27, 3.2);
    dot(33, 9, 3.2);
    // 中点双斜标（等长记号风格，与「距离」的端尺寸线区分）
    seg(18.6, 16.2, 23.4, 21.4, 1.5);
    seg(21.6, 13.6, 26.4, 18.8, 1.5);
  } else if (id === 'circleRadius') {
    ctx.beginPath();
    ctx.arc(19, 19, 10.5, 0, Math.PI * 2);
    ctx.lineWidth = 2;
    ctx.strokeStyle = ink;
    ctx.stroke();
    seg(19, 19, 26.4, 11.6, 1.8);
    dot(19, 19, 3);
    dot(26.4, 11.6, 2.6);
  } else if (id === 'angleSize') {
    seg(13, 29, 37, 29, 2);
    seg(13, 29, 26, 6, 2);
    ctx.beginPath();
    ctx.moveTo(13, 29);
    ctx.arc(13, 29, 11, -1.13, 0);
    ctx.closePath();
    ctx.fillStyle = soft;
    ctx.fill();
    arcStroke(13, 29, 11, -1.13, 0, 1.8);
    dot(13, 29, 3);
  } else if (id === 'rotate') {
    arcStroke(21, 19, 9.5, Math.PI * 0.75, Math.PI * 1.9, 2);
    const ex = 21 + 9.5 * Math.cos(Math.PI * 1.9);
    const ey = 19 + 9.5 * Math.sin(Math.PI * 1.9);
    arrow(ex - 5.6, ey - 4.4, ex, ey);
    dot(21, 19, 3);
  } else if (id === 'dilate') {
    shape([14, 22, 21, 22, 21, 29, 14, 29], '', 1.8, true);
    shape([26, 9, 37, 9, 37, 20, 26, 20], soft, 1.8, true);
    dash(7, 29, 25, 11, 1.4);
    dot(7, 29, 2.8);
  } else if (id === 'slider') {
    seg(8, 19, 34, 19, 2);
    seg(8, 15, 8, 23, 1.5);
    seg(34, 15, 34, 23, 1.5);
    dot(21, 19, 4.2);
  } else {
    dot(21, 18, 4.5);
  }
}
