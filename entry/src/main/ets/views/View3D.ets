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
  onCanvasReady: (w: number, h: number, s: number) => void = (_w: number, _h: number, _s: number) => {};
  // 点按/拖动命中三维点后上报宿主（代数区选中联动，'' 清除）
  onSelectLabel: (l: string) => void = (_l: string) => {};
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
  // 鼠标滚轮（onAxisEvent）：UPDATE 事件间的垂直轴值差
  private axisLastY: number = 0;

  private onRefreshBump(): void {
    this.refresh();
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
          for (let i = 0; i + 3 < nums.length; i += 4) {
            this.ctx.beginPath();
            this.ctx.strokeStyle = color;
            this.ctx.lineWidth = lw;
            this.ctx.moveTo(nums[i], nums[i + 1]);
            this.ctx.lineTo(nums[i + 2], nums[i + 3]);
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
          this.mode = 2;   // 待旋转（Down 先探点：命中三维点则升级拖点）
          this.lastX = ev.touches[0].x;
          this.lastY = ev.touches[0].y;
          this.downX = ev.touches[0].x;
          this.downY = ev.touches[0].y;
          this.tapMoved = false;
          const p = this.toPx(ev.touches[0].x, ev.touches[0].y);
          const hit: string = dreamemberKernel.kernel3DPick(p[0], p[1]);
          if (hit.startsWith('+|')) {
            this.mode = 1;   // 拖三维点（kernel 内已记住命中点）
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
          // 1.0.8.3 双指手势：距离比 → 缩放，质心位移 → 平移（两次内核
          // 变更合并取末帧一次重绘）
          if (this.pinchDist > 1e-6 && dist > 1e-6) {
            dreamemberKernel.kernel3DZoom(dist / this.pinchDist);
          }
          const dpx = this.toPx(cx - this.pinchCx, cy - this.pinchCy);
          const frame = dreamemberKernel.kernel3DPan(dpx[0], dpx[1]);
          this.execFrame(frame);
          this.pinchDist = dist;
          this.pinchCx = cx;
          this.pinchCy = cy;
          this.tapMoved = true;
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
          const dpx = this.toPx(dx, dy);
          const frame = dreamemberKernel.kernel3DDragPoint(dpx[0], dpx[1]);
          if (frame !== '-|') {
            this.execFrame(frame);
          }
        } else if (this.mode === 2) {
          const dpx = this.toPx(dx, dy);
          this.execFrame(dreamemberKernel.kernel3DRotate(dpx[0], dpx[1]));
        }
        break;
      }
      case TouchType.Up:
      case TouchType.Cancel: {
        if (this.mode === 1 || this.mode === 2) {
          dreamemberKernel.kernelEndDrag();
        }
        // 点按（未移动未捏合）：拾取三维点选中，未命中清除选中
        if (ev.type === TouchType.Up && this.mode === 2 && !this.tapMoved) {
          const p = this.toPx(this.downX, this.downY);
          const hit: string = dreamemberKernel.kernel3DPick(p[0], p[1]);
          if (hit.startsWith('+|')) {
            this.onSelectLabel(hit.substring(2));
          } else {
            this.onSelectLabel('');
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
