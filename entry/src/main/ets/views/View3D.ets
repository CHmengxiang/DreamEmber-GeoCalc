// View3D.ets — 1.0.8.1 三维视图（3D 一期；上游 Ggb3D 轨道相机交互子集）。
// 画布本体：内核 render3D 出帧（底面网格/三轴/曲面网格/球柱线框/空间线/
// 三维点，投影后与 2D 帧同协议），本组件持有帧执行器（与 CanvasView.execFrame
// 同一套 op 语义）。
// 交互：单指拖 = 轨道旋转（按在三维点上则视平面拖点；1.1.0.6③ 拖动方向
// 与画面移动同向——内核方位取反）；双指 = 缩放+平移
//（1.0.8.3：质心拖动平移，距离比缩放）；
// 点按 = 选中三维点（代数区联动）；PC 滚轮 = 缩放；右下角 ＋/－ 缩放钮
//（1.1.0.6② 与 2D 同款）。左上 ☰↶↷ 改圆形钮风格与 2D 统一（1.1.0.6②）；
// 原右上「复位」按钮并入 ☰ 菜单"重置视图"（1.1.0.6⑤——菜单项按当前视图
// 给描述，3D 下复位轨道相机 yaw30/pitch20/scale50）。
// 3D 帧已无底面网状格（1.1.0.6④：有坐标轴即可）。
// 刷新：宿主在输入/删除/滑条动画等变更后 bump AppStorage view3dRefresh，
// 本组件 @StorageProp+@Watch 重取帧（代数区与本视图共享同一内核构造）。
import dreamemberKernel from 'libdreamember.so';
import { Theme } from '../model/Theme';

@Component
export struct View3D {
  @Prop dark: boolean = false;
  // 宿主变更信号（输入/删除/滑条动画）→ 重取 3D 帧
  @StorageProp('view3dRefresh') @Watch('onRefreshBump') refreshGen: number = 0;
  // 画布 vp 尺寸（左上按钮/右下缩放钮定位用）
  @State private vpW: number = 0;
  @State private vpH: number = 0;
  // 1.1.0.6②：左上 ☰↶↷ 与右下 ＋/－ 按压回弹（与 CanvasStage 同款 120ms）
  @State private topPressed: string = '';
  @State private zoomPressed: string = '';
  // 1.1.3.12 视图预设钮按压回弹（'std'|'top'|'front'|'side'）
  @State private presetPressed: string = '';
  onCanvasReady: (w: number, h: number, s: number) => void = (_w: number, _h: number, _s: number) => {};
  // 点按/拖动命中三维点后上报宿主（代数区选中联动，'' 清除）
  onSelectLabel: (l: string) => void = (_l: string) => {};
  // 1.1.3.2 三维工具：宿主当前工具 id（'' = 选择/移动）；非空时点按路由
  // kernelToolTap3D 状态机（拖动/旋转手势不受影响）
  @Prop tool: string = '';
  // 工具结果回报宿主（提示条 + 画布/代数区刷新，同 CanvasStage.onToolResult）
  onToolResult: (r: string) => void = (_r: string) => {};
  // 宿主统一提交待输入行/收键盘（点画布语义与 2D 一致）
  onCommitPending: () => void = () => {};
  onDismissKeyboard: () => void = () => {};
  // 1.0.8.1 左上浮层（☰/↶/↷：3D 视图顶替 CanvasStage 后主菜单/撤销入口
  // 不能丢——冒烟实测缺口；撤销栈状态由宿主直传）
  @Prop canUndo: boolean = false;
  @Prop canRedo: boolean = false;
  onMenuAct: () => void = () => {};
  onUndoAct: () => void = () => {};
  onRedoAct: () => void = () => {};
  // 1.1.3.12 拖动级联同步（代数区现值列节流刷新，2D CanvasStage 同款）
  onDragMoveSync: () => void = () => {};
  onDragEndSync: () => void = () => {};

  private ctx: CanvasRenderingContext2D =
    new CanvasRenderingContext2D(new RenderingContextSettings(true));
  // 位图像素（ctx.width/height）与 vp/px 比例；就绪前渲染调用直接跳过
  private canvasW: number = 0;
  private canvasH: number = 0;
  private uiScale: number = 1.0;
  // 触摸状态：0=无 1=拖三维点 2=旋转 3=双指（捏合缩放+拖动平移）
  private mode: number = 0;
  private lastX: number = 0;      // vp
  private lastY: number = 0;
  private pinchDist: number = 0;
  private pinchCx: number = 0;    // 双指质心（vp，1.0.8.3 平移基准）
  private pinchCy: number = 0;
  private tapMoved: boolean = false;
  private downX: number = 0;
  private downY: number = 0;
  // 1.1.3.8 拖动撤销打点（CanvasStage 同款）：首次实际移动前压快照，
  // 整段拖动一步（按下未动不留步）；点与非点对象拖动共用
  private dragUndoMarked: boolean = false;
  // 鼠标滚轮（onAxisEvent）：UPDATE 事件间的垂直轴值差
  private axisLastY: number = 0;
  // 1.1.3.21 渲染合帧节流：Move 高频事件只累积增量，≤16ms 一帧出渲染
  // （内核 rotateBy/dragDelta/panBy 均线性，合帧一次调用=逐次调用之和；
  // 重型场景全量 render3D 不再随输入频率跑满主线程——ANR 对症）。
  // Up 时先排空残余增量再收尾（拖点/旋转末帧不丢）
  private frameTimer: number = -1;
  private lastFrameAt: number = 0;
  private pendAccX: number = 0;     // 旋转/拖点累积位移（vp）
  private pendAccY: number = 0;
  private pendZoom: number = 1.0;   // 双指缩放因子连乘
  private pendPanX: number = 0;     // 双指平移累积（vp）
  private pendPanY: number = 0;

  private onRefreshBump(): void {
    this.refresh();
  }

  // 1.1.3.21 合帧：排队一次 ≤16ms 后的 flushFrame（已有排队帧继续累积）
  private scheduleFlush(): void {
    if (this.frameTimer >= 0) {
      return;
    }
    const delay = Math.max(0, 16 - (Date.now() - this.lastFrameAt));
    this.frameTimer = setTimeout(() => {
      this.frameTimer = -1;
      this.flushFrame();
    }, delay);
  }

  // 排空累积增量按当前手势一次出帧（旋转/拖点=位移和；双指=缩放因子
  // 连乘 + 平移和，内核变更合并取末帧一次重绘）
  private flushFrame(): void {
    const dx = this.pendAccX;
    const dy = this.pendAccY;
    const zf = this.pendZoom;
    const px = this.pendPanX;
    const py = this.pendPanY;
    this.pendAccX = 0;
    this.pendAccY = 0;
    this.pendZoom = 1.0;
    this.pendPanX = 0;
    this.pendPanY = 0;
    if (this.canvasW <= 1) {
      return;
    }
    if (this.mode === 3) {
      if (zf !== 1.0) {
        dreamemberKernel.kernel3DZoom(zf);
      }
      if (px !== 0 || py !== 0) {
        const dpx = this.toPx(px, py);
        this.execFrame(dreamemberKernel.kernel3DPan(dpx[0], dpx[1]));
      }
    } else if (this.mode === 1) {
      if (dx !== 0 || dy !== 0) {
        const dpx = this.toPx(dx, dy);
        const frame = dreamemberKernel.kernel3DDragPoint(dpx[0], dpx[1]);
        if (frame !== '-|') {
          this.execFrame(frame);
          this.onDragMoveSync();
        }
      }
    } else if (this.mode === 2) {
      if (dx !== 0 || dy !== 0) {
        const dpx = this.toPx(dx, dy);
        this.execFrame(dreamemberKernel.kernel3DRotate(dpx[0], dpx[1]));
      }
    }
    this.lastFrameAt = Date.now();
  }

  // Up/Cancel 收尾前排空排队帧（残余增量不丢）
  private drainPending(): void {
    if (this.frameTimer >= 0) {
      clearTimeout(this.frameTimer);
      this.frameTimer = -1;
    }
    this.flushFrame();
  }

  // 触摸 vp 坐标 -> 位图像素
  private toPx(vpX: number, vpY: number): number[] {
    return [vpX * this.uiScale, vpY * this.uiScale];
  }

  // 四十包 同款会聚重解：onReady（位图就绪）与 onAreaChange（vp 就绪）
  // 双侧重解、按值去重——无论两事件谁先到，后到者拿正确配对
  private syncCanvasMetrics(): void {
    const w = this.ctx.width;
    const h = this.ctx.height;
    if (w <= 0 || h <= 0) {
      return;
    }
    const s = this.vpW > 0 ? w / this.vpW : this.uiScale;
    if (w === this.canvasW && h === this.canvasH && s === this.uiScale) {
      return;
    }
    this.canvasW = w;
    this.canvasH = h;
    this.uiScale = s;
    this.onCanvasReady(w, h, s);
    this.refresh();
  }

  private refresh(): void {
    if (this.canvasW <= 1 || this.canvasH <= 1) {
      return;
    }
    this.execFrame(dreamemberKernel.kernelRender3D(this.canvasW, this.canvasH,
      this.uiScale, this.dark));
  }

  // ---- 帧执行器（op 语义与 CanvasView.execFrame 一致；3D 帧是同一协议） ----

  private execFrame(frame: string): void {
    if (frame.length === 0) {
      return;
    }
    this.ctx.fillStyle = Theme.canvas(this.dark);
    this.ctx.fillRect(0, 0, this.canvasW, this.canvasH);
    const rows = frame.split('\n');
    for (const row of rows) {
      if (row.length < 4) continue;
      const parts = row.split('|');
      if (parts.length < 4) continue;
      const op = parts[0];
      if (op === 'c') continue;
      const color = parts[1];
      const lw = parseFloat(parts[2]);
      let text = '';
      let coordsStr = parts[3];
      if (op === 't') {
        text = parts[3];
        coordsStr = parts.length > 4 ? parts[4] : '';
      }
      const nums: number[] = [];
      if (coordsStr.length > 0) {
        for (const s of coordsStr.split(',')) {
          const v = parseFloat(s);
          if (!isNaN(v)) nums.push(v);
        }
      }
      switch (op) {
        case 's':
          if (nums.length >= 4) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.moveTo(nums[0], nums[1]);
            this.ctx.lineTo(nums[2], nums[3]);
            this.ctx.stroke();
          }
          break;
        case 'm':
          // 1.1.3.21 批量描边：同命令同色同宽，一次 beginPath/stroke 出整行
          // 段（此前逐段 beginPath/stroke，主线程 canvas 调用数随段数倍增）
          if (nums.length >= 4) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            for (let i = 0; i + 3 < nums.length; i += 4) {
              this.ctx.moveTo(nums[i], nums[i + 1]);
              this.ctx.lineTo(nums[i + 2], nums[i + 3]);
            }
            this.ctx.stroke();
          }
          break;
        case 'p':
          if (nums.length >= 4) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.moveTo(nums[0], nums[1]);
            for (let i = 2; i + 1 < nums.length; i += 2) {
              this.ctx.lineTo(nums[i], nums[i + 1]);
            }
            this.ctx.stroke();
          }
          break;
        case 'o':
          if (nums.length >= 3) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.arc(nums[0], nums[1], nums[2], 0, Math.PI * 2);
            this.ctx.stroke();
          }
          break;
        case 'f':
          if (nums.length >= 3) {
            this.ctx.beginPath();
            this.ctx.fillStyle = color;
            this.ctx.arc(nums[0], nums[1], nums[2], 0, Math.PI * 2);
            this.ctx.fill();
          }
          break;
        case 't':
          if (text.length > 0 && nums.length >= 2) {
            this.ctx.fillStyle = color;
            this.ctx.font = `${Math.round(14 * this.uiScale)}px sans-serif`;
            this.ctx.fillText(text, nums[0], nums[1]);
          }
          break;
        case 'a':
          if (nums.length >= 6) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.arc(nums[0], nums[1], nums[2], nums[3], nums[4], false);
            this.ctx.stroke();
          }
          break;
        case 'y':
          // 曲面四边形填充（rgba() 串）
          if (nums.length >= 6) {
            this.ctx.beginPath();
            this.ctx.fillStyle = color;
            this.ctx.moveTo(nums[0], nums[1]);
            for (let i = 2; i + 1 < nums.length; i += 2) {
              this.ctx.lineTo(nums[i], nums[i + 1]);
            }
            this.ctx.closePath();
            this.ctx.fill();
          }
          break;
        default:
          break;
      }
    }
  }

  // ---- 交互 ----

  // 第二十二包⑤同款：左上按钮按压回弹（'menu'|'undo'|'redo'）
  private pressTop(id: string): void {
    this.topPressed = id;
    setTimeout(() => {
      this.topPressed = '';
    }, 120);
  }

  // 1.1.0.6② 右下角缩放钮：按压回弹 + 150ms 六步平滑缩放（kernel3DZoom
  // 缩 scale，锚点屏幕中心=世界原点投影，与 CanvasStage.zoomAtCenter 同口径）
  private zoomAtCenter3D(factor: number): void {
    if (this.canvasW <= 1 || this.canvasH <= 1) {
      return;
    }
    this.zoomPressed = factor > 1 ? 'in' : 'out';
    setTimeout(() => {
      this.zoomPressed = '';
    }, 120);
    const steps = 6;
    const stepF = Math.pow(factor, 1 / steps);
    let done = 0;
    const tick = (): void => {
      this.execFrame(dreamemberKernel.kernel3DZoom(stepF));
      done++;
      if (done < steps) {
        setTimeout(tick, 25);
      }
    };
    tick();
  }

  // PC 滚轮缩放（方向语义与 CanvasStage 同：轴值增大 = 下滚 = 缩小）
  private onCanvasAxis(ev: AxisEvent): void {
    if (ev.action === AxisAction.BEGIN) {
      this.axisLastY = ev.getVerticalAxisValue();
      return;
    }
    if (ev.action === AxisAction.UPDATE) {
      const cur = ev.getVerticalAxisValue();
      const dv = cur - this.axisLastY;
      this.axisLastY = cur;
      if (dv === 0 || this.canvasW <= 1) {
        return;
      }
      let steps = dv > 0 ? 1 : -1;
      if (ev.scrollStep !== undefined && ev.scrollStep > 0) {
        steps *= Math.min(4, Math.max(1, Math.round(ev.scrollStep)));
      }
      const f = Math.pow(1.1, -steps);
      this.execFrame(dreamemberKernel.kernel3DZoom(f));
    } else if (ev.action === AxisAction.END || ev.action === AxisAction.CANCEL) {
      this.axisLastY = 0;
    }
  }

  private handleTouch(ev: TouchEvent): void {
    switch (ev.type) {
      case TouchType.Down: {
        this.onCommitPending();
        this.onDismissKeyboard();
        if (ev.touches.length === 1) {
          this.mode = 2;   // 待旋转（Down 先探点：命中三维点/可拖非点对象则升级拖动）
          this.lastX = ev.touches[0].x;
          this.lastY = ev.touches[0].y;
          this.downX = ev.touches[0].x;
          this.downY = ev.touches[0].y;
          this.tapMoved = false;
          this.dragUndoMarked = false;
          const p = this.toPx(ev.touches[0].x, ev.touches[0].y);
          const hit: string = dreamemberKernel.kernel3DPick(p[0], p[1]);
          if (hit.startsWith('+|') && this.tool === '') {
            this.mode = 1;   // 拖动（点或球/面/实体，1.1.3.8）：kernel 内已
                             // 记住命中目标；工具态不升级——点按须交给
                             // kernelToolTap3D 拾取
          }
        } else if (ev.touches.length >= 2) {
          this.mode = 3;   // 双指：捏合缩放 + 质心拖动平移（GGB 3D 口径）
          dreamemberKernel.kernelEndDrag();   // 取消拖点拾取
          const dx = ev.touches[0].x - ev.touches[1].x;
          const dy = ev.touches[0].y - ev.touches[1].y;
          this.pinchDist = Math.sqrt(dx * dx + dy * dy);
          this.pinchCx = (ev.touches[0].x + ev.touches[1].x) / 2;
          this.pinchCy = (ev.touches[0].y + ev.touches[1].y) / 2;
        }
        break;
      }
      case TouchType.Move: {
        if (ev.touches.length >= 2 && this.mode === 3) {
          const dx = ev.touches[0].x - ev.touches[1].x;
          const dy = ev.touches[0].y - ev.touches[1].y;
          const dist = Math.sqrt(dx * dx + dy * dy);
          const cx = (ev.touches[0].x + ev.touches[1].x) / 2;
          const cy = (ev.touches[0].y + ev.touches[1].y) / 2;
          // 1.0.8.3 双指手势：距离比 → 缩放，质心位移 → 平移
          // 1.1.3.21 合帧：因子连乘/位移累积，flushFrame 一次出帧
          if (this.pinchDist > 1e-6 && dist > 1e-6) {
            this.pendZoom *= dist / this.pinchDist;
          }
          this.pendPanX += cx - this.pinchCx;
          this.pendPanY += cy - this.pinchCy;
          this.pinchDist = dist;
          this.pinchCx = cx;
          this.pinchCy = cy;
          this.tapMoved = true;
          this.scheduleFlush();
          break;
        }
        if (ev.touches.length === 0 || this.mode === 0) {
          break;
        }
        const dx = ev.touches[0].x - this.lastX;
        const dy = ev.touches[0].y - this.lastY;
        if (Math.abs(ev.touches[0].x - this.downX) > 6
          || Math.abs(ev.touches[0].y - this.downY) > 6) {
          this.tapMoved = true;
        }
        if (dx === 0 && dy === 0) {
          break;
        }
        this.lastX = ev.touches[0].x;
        this.lastY = ev.touches[0].y;
        if (this.mode === 1) {
          if (!this.dragUndoMarked) {
            // 首次实际移动前压撤销快照（整段拖动一步；按下未动不留步）
            this.dragUndoMarked = true;
            dreamemberKernel.kernelPushUndo();
          }
          // 1.1.3.21 合帧：位移累积，flushFrame 按手势一次出帧
          this.pendAccX += dx;
          this.pendAccY += dy;
          this.scheduleFlush();
        } else if (this.mode === 2) {
          this.pendAccX += dx;
          this.pendAccY += dy;
          this.scheduleFlush();
        }
        break;
      }
      case TouchType.Up:
      case TouchType.Cancel: {
        // 1.1.3.21 先排空残余增量（此时 drag 态仍在，末帧不丢）再收尾
        this.drainPending();
        if (this.mode === 1 || this.mode === 2) {
          dreamemberKernel.kernelEndDrag();
        }
        if (this.mode === 1) {
          this.onDragEndSync();
        }
        // 点按（未移动未捏合）：工具态走 kernelToolTap3D 状态机（1.1.3.2）；
        // 选择态拾取三维点选中，未命中清除选中
        if (ev.type === TouchType.Up && this.mode === 2 && !this.tapMoved) {
          const p = this.toPx(this.downX, this.downY);
          if (this.tool !== '') {
            this.onToolResult(dreamemberKernel.kernelToolTap3D(p[0], p[1]));
          } else {
            // 1.1.3.5 选中走 PickSel（点优先回退非点对象）；Down 的拖动升级
            // 仍用 kernel3DPick（点优先回退可拖非点对象，1.1.3.8）
            const hit: string = dreamemberKernel.kernel3DPickSel(p[0], p[1]);
            if (hit.startsWith('+|')) {
              this.onSelectLabel(hit.substring(2));
            } else {
              this.onSelectLabel('');
            }
          }
        }
        this.mode = 0;
        break;
      }
      default:
        break;
    }
  }

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      Canvas(this.ctx)
        .id('geo3dView')   // 1.2.0.12 导出 PNG 快照定位
        .width('100%')
        .height('100%')
        .backgroundColor(Theme.canvas(this.dark))
        .onAreaChange((_o: Area, n: Area) => {
          this.vpW = n.width as number;
          this.vpH = n.height as number;
          this.syncCanvasMetrics();
        })
        .onReady(() => {
          this.syncCanvasMetrics();
        })
        .onTouch((ev: TouchEvent) => this.handleTouch(ev))
        .onAxisEvent((ev: AxisEvent) => this.onCanvasAxis(ev))

      // 左上浮层 ☰ ↶ ↷（1.1.0.6②：与 2D CanvasStage 同款圆形钮风格——
      // 40×40 圆形、描边+阴影+按压回弹 scale 0.86，☰ 三横条自绘；无可回退/
      // 重做步置灰不响应。状态/动作均为宿主直传回调）
      if (this.vpW > 200) {
        Row({ space: 8 }) {
          Column({ space: 3 }) {
            Text('')
              .width(16)
              .height(1.6)
              .borderRadius(1)
              .backgroundColor(Theme.text(this.dark))
            Text('')
              .width(16)
              .height(1.6)
              .borderRadius(1)
              .backgroundColor(Theme.text(this.dark))
            Text('')
              .width(16)
              .height(1.6)
              .borderRadius(1)
              .backgroundColor(Theme.text(this.dark))
          }
          .justifyContent(FlexAlign.Center)
          .width(40)
          .height(40)
          .borderRadius(20)
          .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
          .borderWidth(1)
          .borderColor(Theme.border(this.dark))
          .shadow({ radius: 8, color: 'rgba(0,0,0,0.18)', offsetX: 0, offsetY: 1 })
          .scale({
            x: this.topPressed === 'menu' ? 0.86 : 1,
            y: this.topPressed === 'menu' ? 0.86 : 1
          })
          .animation({ duration: 120, curve: Curve.EaseOut })
          .onClick(() => {
            this.pressTop('menu');
            this.onMenuAct();
          })
          Text('↶')
            .fontSize(18)
            .fontColor(this.canUndo ? Theme.text(this.dark)
              : (this.dark ? '#6E747D' : '#BBBBBB'))
            .width(40)
            .height(40)
            .textAlign(TextAlign.Center)
            .borderRadius(20)
            .opacity(this.canUndo ? 1 : 0.55)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .shadow({ radius: 8, color: 'rgba(0,0,0,0.18)', offsetX: 0, offsetY: 1 })
            .scale({
              x: this.topPressed === 'undo' ? 0.86 : 1,
              y: this.topPressed === 'undo' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => {
              if (this.canUndo) {
                this.pressTop('undo');
                this.onUndoAct();
              }
            })
          Text('↷')
            .fontSize(18)
            .fontColor(this.canRedo ? Theme.text(this.dark)
              : (this.dark ? '#6E747D' : '#BBBBBB'))
            .width(40)
            .height(40)
            .textAlign(TextAlign.Center)
            .borderRadius(20)
            .opacity(this.canRedo ? 1 : 0.55)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .shadow({ radius: 8, color: 'rgba(0,0,0,0.18)', offsetX: 0, offsetY: 1 })
            .scale({
              x: this.topPressed === 'redo' ? 0.86 : 1,
              y: this.topPressed === 'redo' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => {
              if (this.canRedo) {
                this.pressTop('redo');
                this.onRedoAct();
              }
            })
        }
        .position({ x: 10, y: 10 })
      }

      // 左上提示条（交互说明；1.1.0.6② 移到按钮组下方，与 2D toolHint 同位）
      Text('单指拖=旋转 · 双指=缩放/平移 · 点选/拖动三维点')
        .fontSize(11)
        .fontColor(Theme.text(this.dark))
        .maxLines(2)
        .padding({ left: 8, right: 8, top: 4, bottom: 4 })
        .borderRadius(6)
        .backgroundColor(this.dark ? 'rgba(42,45,49,0.92)' : 'rgba(255,255,255,0.92)')
        .constraintSize({ maxWidth: this.vpW > 16 ? this.vpW - 16 : 300 })
        .position({ x: 10, y: 58 })

      // 1.1.3.12 视图预设钮（缩放钮左侧竖排四枚）：标准/俯视/正视/侧视
      if (this.vpW > 200 && this.vpH > 260) {
        Column({ space: 6 }) {
          Text('标')
            .fontSize(12)
            .fontColor(Theme.text(this.dark))
            .width(30)
            .height(30)
            .textAlign(TextAlign.Center)
            .borderRadius(15)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .scale({
              x: this.presetPressed === 'std' ? 0.86 : 1,
              y: this.presetPressed === 'std' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => {
              this.presetPressed = 'std';
              setTimeout(() => {
                this.presetPressed = '';
              }, 120);
              this.execFrame(dreamemberKernel.kernel3DPreset(0));
            })
          Text('俯')
            .fontSize(12)
            .fontColor(Theme.text(this.dark))
            .width(30)
            .height(30)
            .textAlign(TextAlign.Center)
            .borderRadius(15)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .scale({
              x: this.presetPressed === 'top' ? 0.86 : 1,
              y: this.presetPressed === 'top' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => {
              this.presetPressed = 'top';
              setTimeout(() => {
                this.presetPressed = '';
              }, 120);
              this.execFrame(dreamemberKernel.kernel3DPreset(1));
            })
          Text('正')
            .fontSize(12)
            .fontColor(Theme.text(this.dark))
            .width(30)
            .height(30)
            .textAlign(TextAlign.Center)
            .borderRadius(15)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .scale({
              x: this.presetPressed === 'front' ? 0.86 : 1,
              y: this.presetPressed === 'front' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => {
              this.presetPressed = 'front';
              setTimeout(() => {
                this.presetPressed = '';
              }, 120);
              this.execFrame(dreamemberKernel.kernel3DPreset(2));
            })
          Text('侧')
            .fontSize(12)
            .fontColor(Theme.text(this.dark))
            .width(30)
            .height(30)
            .textAlign(TextAlign.Center)
            .borderRadius(15)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .scale({
              x: this.presetPressed === 'side' ? 0.86 : 1,
              y: this.presetPressed === 'side' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => {
              this.presetPressed = 'side';
              setTimeout(() => {
                this.presetPressed = '';
              }, 120);
              this.execFrame(dreamemberKernel.kernel3DPreset(3));
            })
        }
        .position({ x: this.vpW - 48, y: this.vpH - 320 })
      }

      // 右下角 ＋/－ 缩放按钮（1.1.0.6②：与 2D CanvasStage 同款圆形样式；
      // kernel3DZoom 缩 scale，屏幕中心即世界原点投影=锚点中心）
      if (this.vpW > 120 && this.vpH > 200) {
        Column({ space: 8 }) {
          Text('＋')
            .fontSize(22)
            .fontColor(Theme.text(this.dark))
            .width(44)
            .height(44)
            .textAlign(TextAlign.Center)
            .borderRadius(22)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .shadow({ radius: 8, color: 'rgba(0,0,0,0.18)', offsetX: 0, offsetY: 1 })
            .scale({
              x: this.zoomPressed === 'in' ? 0.86 : 1,
              y: this.zoomPressed === 'in' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => this.zoomAtCenter3D(1.25))
          Text('－')
            .fontSize(22)
            .fontColor(Theme.text(this.dark))
            .width(44)
            .height(44)
            .textAlign(TextAlign.Center)
            .borderRadius(22)
            .backgroundColor(this.dark ? 'rgba(42,45,49,0.9)' : 'rgba(255,255,255,0.9)')
            .borderWidth(1)
            .borderColor(Theme.border(this.dark))
            .shadow({ radius: 8, color: 'rgba(0,0,0,0.18)', offsetX: 0, offsetY: 1 })
            .scale({
              x: this.zoomPressed === 'out' ? 0.86 : 1,
              y: this.zoomPressed === 'out' ? 0.86 : 1
            })
            .animation({ duration: 120, curve: Curve.EaseOut })
            .onClick(() => this.zoomAtCenter3D(0.8))
        }
        .position({ x: this.vpW - 60, y: this.vpH - 112 })
        .transition(TransitionEffect.asymmetric(
          TransitionEffect.opacity(0).combine(TransitionEffect.scale({ x: 0.6, y: 0.6 })),
          TransitionEffect.opacity(0).combine(TransitionEffect.scale({ x: 0.6, y: 0.6 }))
        ).animation({ duration: 180, curve: Curve.EaseOut }))
      }
    }
    .width('100%')
    .height('100%')
  }
}
