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
  } else if (id === 'segment' || id === 'segment3D') {
    seg(10, 26, 32, 10, 2);
    dot(10, 26, 3.4);
    dot(32, 10, 3.4);
  } else if (id === 'line' || id === 'line3D') {
    seg(5, 29.5, 37, 6.5, 2);
    dot(14, 24, 3.4);
    dot(28, 12, 3.4);
  } else if (id === 'ray') {
    // 二十八包①：去箭头（原与向量同款箭头难区分）。上游 口径：射线=一个
    // 起点圆点 + 延伸到画布边的半线（线=两个内部点、线段=两个端点）
    seg(9, 27, 37, 6.5, 2);
    dot(9, 27, 3.4);
  } else if (id === 'vector' || id === 'vector3D') {
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
  } else if (id === 'midpoint' || id === 'midpoint3D') {
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
  } else if (id === 'intersect' || id === 'intersect3D') {
    seg(7, 25, 35, 11, 2);
    seg(14, 9, 28, 27, 2);
    dot(21, 18, 4.4);
  } else if (id === 'attachDetach') {
    // 四十五包 附着/脱离点：横线 + 线上落点 + 两支下指箭头（吸附到线）
    seg(6, 25, 37, 25, 2);
    dot(21, 25, 3.4);
    arrow(12.5, 8.5, 16.8, 16.5);
    arrow(29.5, 8.5, 25.2, 16.5);
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
  } else if (id === 'perpLine' || id === 'perpLine3D') {
    seg(6, 26, 36, 26, 2);
    seg(23, 5, 23, 31, 2);
    shape([18.5, 26, 18.5, 21.5, 23, 21.5], '', 1.4, false);
    dot(23, 26, 3.2);
    dot(9, 26, 2.6);
    dot(34, 26, 2.6);
  } else if (id === 'parLine' || id === 'parLine3D') {
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
  } else if (id === 'sphere3D') {
    // 球：圆 + 球心蓝点 + 球面标点（GGB 球心点工具同语汇）
    ctx.beginPath();
    ctx.arc(21, 18, 11, 0, Math.PI * 2);
    ctx.lineWidth = 2;
    ctx.strokeStyle = ink;
    ctx.stroke();
    dot(21, 18, 3);
    dot(29, 12, 3);
  } else if (id === 'point3D') {
    // 三维点：斜置平面（平行四边形，soft 填充）+ 蓝点
    shape([12, 25, 30, 29, 36, 14, 18, 10], soft, 1.2, true);
    dot(23, 19, 4.5);
  } else if (id === 'plane3D') {
    // 平面：斜置平行四边形（soft 填充 + ink 描边）+ 三点标
    shape([9, 26, 28, 30, 37, 12, 18, 8], soft, 1.2, true);
    dot(14, 24, 3);
    dot(30, 24, 3);
    dot(26, 12, 3);
  } else if (id === 'pyramid3D') {
    // 锥体：三角形底 + 锥顶三条棱（GGB 金字塔语汇）
    shape([10, 28, 32, 28, 21, 18], soft, 1.8, false);
    seg(21, 18, 21, 7, 2);
    seg(21, 7, 32, 28, 1.8);
    seg(21, 7, 10, 28, 1.8);
    dot(21, 7, 3);
  } else if (id === 'prism3D') {
    // 棱柱：底三角 + 平移顶三角 + 三条侧棱
    shape([8, 29, 30, 29, 19, 19], soft, 1.8, false);
    shape([17, 20, 39, 20, 28, 10], '', 1.4, false);
    seg(8, 29, 17, 20, 1.4);
    seg(30, 29, 39, 20, 1.4);
    seg(19, 19, 28, 10, 1.4);
  } else if (id === 'tetrahedron3D') {
    // 正四面体（1.1.3.9）：底面三角（soft）+ 顶点三棱实线，区别锥体虚感
    shape([9, 27, 33, 27, 21, 19], soft, 1.6, true);
    seg(21, 19, 21, 6, 2);
    seg(21, 6, 33, 27, 2);
    seg(21, 6, 9, 27, 2);
    dot(21, 6, 3);
    dot(9, 27, 2.6);
    dot(33, 27, 2.6);
  } else if (id === 'cube3D') {
    // 正方体（1.1.3.9）：前方面（soft）+ 顶/侧透视棱
    shape([10, 30, 28, 30, 28, 12, 10, 12], soft, 1.8, true);
    seg(10, 12, 17, 5, 1.4);
    seg(28, 12, 35, 5, 1.4);
    seg(28, 30, 35, 23, 1.4);
    seg(17, 5, 35, 5, 1.4);
    seg(35, 5, 35, 23, 1.4);
  } else if (id === 'translate3D') {
    // 平移（3D）：虚线基线 + 实线位移箭头 + 立体方块终点（2D translate 语汇
    // 加 3D 体块）
    dash(7, 27, 30, 11, 1.4);
    seg(10.5, 25, 22, 17.5, 2.2);
    arrow(10.5, 25, 25.5, 15.2);
    dot(7, 27, 3.2);
    shape([26, 10, 34, 10, 34, 18, 26, 18], soft, 1.5, true);
  } else if (id === 'reflect3D') {
    // 反射（3D）：斜置镜像平面 + 两翼对称点（soft 面表平面）
    shape([16, 6, 25, 30, 18, 32, 9, 8], soft, 1.4, true);
    dot(4.5, 20, 3);
    dot(37, 15, 3);
    seg(4.5, 20, 14, 19, 1.4);
    seg(27, 14.5, 37, 15, 1.4);
  } else if (id === 'reflectPoint3D') {
    // 中心对称（3D）：虚线对角 + 两端点 + 立体中心块
    dash(6, 29, 36, 8, 1.5);
    dot(6, 29, 3.2);
    dot(36, 8, 3.2);
    shape([17, 15, 25, 15, 25, 23, 17, 23], soft, 1.5, true);
  } else if (id === 'dist3D') {
    // 距离（3D）：斜线段 + 端尺寸线（2D dist 语汇加透视双斜标）
    seg(9, 26, 33, 10, 2);
    seg(6.8, 27.3, 11.2, 24.7, 1.5);
    seg(30.8, 11.3, 35.2, 8.7, 1.5);
    dot(9, 26, 3.2);
    dot(33, 10, 3.2);
    seg(19, 16, 24, 21, 1.4);
  } else if (id === 'angle3D') {
    // 角度（3D）：两线夹角 + 弧 + 平面软底（面面角语汇）
    shape([6, 30, 38, 30, 38, 22, 6, 22], soft, 1.2, true);
    seg(10, 26, 36, 26, 2);
    seg(10, 26, 28, 8, 2);
    arcStroke(10, 26, 10, -0.79, 0, 1.8);
    dot(10, 26, 3);
  } else if (id === 'octahedron3D') {
    // 正八面体：菱形双锥剪影 + 中带
    shape([21, 4, 33, 20, 21, 36, 9, 20], soft, 1.8, true);
    seg(9, 20, 33, 20, 1.4);
    dot(21, 4, 2.6);
    dot(21, 36, 2.6);
  } else if (id === 'icosahedron3D') {
    // 正二十面体：上三角 + 中带交叉棱 + 下三角
    shape([21, 4, 32, 12, 10, 12], soft, 1.5, true);
    shape([21, 32, 32, 24, 10, 24], soft, 1.5, true);
    seg(10, 12, 10, 24, 1.4);
    seg(32, 12, 32, 24, 1.4);
    seg(21, 4, 10, 24, 1.2);
    seg(21, 4, 32, 24, 1.2);
    seg(21, 32, 10, 12, 1.2);
    seg(21, 32, 32, 12, 1.2);
  } else if (id === 'dodecahedron3D') {
    // 正十二面体：五边形顶面 + 侧棱 + 底面五边形透视（1.1.3.15）
    shape([21, 5, 33, 13, 28, 24, 14, 24, 9, 13], soft, 1.5, true);
    seg(9, 13, 5, 26, 1.3);
    seg(14, 24, 12, 36, 1.3);
    seg(28, 24, 30, 36, 1.3);
    seg(33, 13, 37, 26, 1.3);
    seg(21, 5, 21, 29, 1.3);
    shape([5, 26, 12, 36, 30, 36, 37, 26, 21, 29], '', 1.3, true);
  } else if (id === 'rotate3D') {
    // 旋转（3D）：绕轴弧+箭头（2D rotate 同语汇+轴线）
    seg(10, 8, 10, 32, 1.4);
    dot(10, 8, 2.6);
    dot(10, 32, 2.6);
    shape([16, 14, 30, 14, 33, 22, 26, 29, 14, 27], '', 1.6, false);
    seg(33, 22, 37, 18, 1.4);
    seg(33, 22, 29, 17, 1.4);
  } else if (id === 'slider') {
    seg(8, 19, 34, 19, 2);
    seg(8, 15, 8, 23, 1.5);
    seg(34, 15, 34, 23, 1.5);
    dot(21, 19, 4.2);
  } else if (id === 'ellipse') {
    // 椭圆本像（缩放圆弧）+ 两焦点蓝点
    ctx.save();
    ctx.translate(21, 19);
    ctx.scale(1.55, 1);
    ctx.beginPath();
    ctx.arc(0, 0, 8.5, 0, Math.PI * 2);
    ctx.lineWidth = 2;
    ctx.strokeStyle = ink;
    ctx.stroke();
    ctx.restore();
    dot(14, 19, 2.6);
    dot(28, 19, 2.6);
  } else if (id === 'hyperbola') {
    // 双曲线两支（贝塞尔）+ 两焦点蓝点
    ctx.beginPath();
    ctx.moveTo(6, 8);
    ctx.bezierCurveTo(17, 12, 17, 26, 6, 30);
    ctx.moveTo(36, 8);
    ctx.bezierCurveTo(25, 12, 25, 26, 36, 30);
    ctx.lineWidth = 2;
    ctx.strokeStyle = ink;
    ctx.stroke();
    dot(16, 19, 2.6);
    dot(26, 19, 2.6);
  } else if (id === 'parabola') {
    // 准线虚线 + 焦点蓝点 + 开口向上抛物线
    dash(6, 30, 36, 30, 1.4);
    ctx.beginPath();
    ctx.moveTo(9, 8);
    ctx.quadraticCurveTo(21, 36, 33, 8);
    ctx.lineWidth = 2;
    ctx.strokeStyle = ink;
    ctx.stroke();
    dot(21, 10, 2.6);
  } else {
    dot(21, 18, 4.5);
  }
}
