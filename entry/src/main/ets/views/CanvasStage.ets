// CanvasStage.ets — 画布交互层（自 CanvasView 拆出，轮 A 防乱重构）：
// Canvas 位图 + 触摸手势（拖点/平移/捏合/长按菜单/工具点按）+ ＋/－ 缩放按钮
// + 滚轮缩放 + 画布浮层（长按菜单/工具反馈条）。
// 职责边界：内核调用在本层直达（拖点/平移/缩放是逐帧路径，不能绕道），
// 但代数区同步、输入会话、选中联动、渲染编排统一经回调上达宿主 CanvasView
// （它是唯一的数据源头，避免双真相）。渲染（execFrame/kernelRender）留在
// 宿主：ctx 由宿主创建并传入，内核产出的帧串经 onFrame 回宿主落位图。
// 轮 C：顶部工具条删除（工具入口在底部"工具"Tab，防重复）。
// 反馈②（2026-09-30 轮）：☰ ↶ ↷ 按钮组浮于画布左上角（与右下角 ＋/－
// 缩放按钮同款圆形样式），页面顶栏整个删除；状态 canUndo/canRedo 由宿主
// CanvasView @Prop 直传（撤销栈真源在宿主），动作经回调回宿主。创建/命令
// 提示条（toolHint）让位至按钮组下方，消失时机由宿主控制（约 3 秒）。
// ☰ 为三横条自绘（字形在圆钮内不居中的真机反馈）。
import dreamemberKernel from 'libdreamember.so';
import { Theme } from '../model/Theme';

@Component
export struct CanvasStage {
  @Prop dark: boolean = false;
  @Prop tool: string = '';          // 当前工具 id（'' = 选择/移动）
  @Prop toolHint: string = '';      // 画布左上反馈条（创建/命令结果）
  @Link vpW: number;                // 画布 vp 尺寸（触摸坐标是 vp；缩放按钮定位）
  @Link vpH: number;
  // 渲染上下文由宿主创建并传入（宿主 execFrame 落帧）
  ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(
    new RenderingContextSettings(true));
  // ---- 宿主回调：内核编排统一在 CanvasView ----
  onCanvasReady: (w: number, h: number, uiScale: number) => void =
    (_w: number, _h: number, _s: number) => {};
  onFrame: (frame: string) => void = (_f: string) => {};
  // 画布请求选中变化（'' = 清除；宿主去重：''→'' 不重复同步）
  onSelectLabel: (label: string) => void = (_l: string) => {};
  // 拖点过程中的代数区节流同步 / 抬起补同步
  onDragMoveSync: () => void = () => {};
  onDragEndSync: () => void = () => {};
  // 工具点按结果（done|/need|/err|）交宿主处理（提示条+同步+动画）
  onToolResult: (r: string) => void = (_r: string) => {};
  onSetVisible: (l: string, show: boolean) => void = (_l: string, _s: boolean) => {};
  onDeleteObj: (l: string) => void = (_l: string) => {};
  onCommitPending: () => void = () => {};
  onDismissKeyboard: () => void = () => {};
  // 反馈②：左上角 ☰ ↶ ↷ 浮层（撤销栈状态由宿主直传，动作回调回宿主）
  @Prop canUndo: boolean = false;
  @Prop canRedo: boolean = false;
  onMenuAct: () => void = () => {};
  onUndoAct: () => void = () => {};
  onRedoAct: () => void = () => {};

  // 位图像素（ctx.width/height）与 vp/px 比例；onReady 就绪前渲染调用直接跳过
  private canvasW: number = 0;
  private canvasH: number = 0;
  private uiScale: number = 1.0;
  // 触摸状态
  private mode: number = 0;       // 0=无 1=拖点 2=平移/点按 3=捏合
  private pinchStartDist: number = 0;
  private pinchCx: number = 0;    // 位图px
  private pinchCy: number = 0;
  private lastPanX: number = 0;   // vp
  private lastPanY: number = 0;
  // 手势切换（捏合/拖点 → 平移）后置 true：首帧 Move 只重定位 lastPan 不平移，
  // 避免 Up 事件 touches[0] 残留已抬手指坐标导致整体跳一下
  private panRebase: boolean = false;
  // 鼠标滚轮缩放（onAxisEvent）：UPDATE 事件间的垂直轴值差
  private axisLastY: number = 0;
  // 长按菜单探测
  private lpTimer: number = -1;
  private lpMoved: boolean = false;
  private lpFired: boolean = false;
  private lpX: number = 0;                       // 长按起点（vp）
  private lpY: number = 0;
  private downX: number = 0;                     // 本次按下起点（vp，点按判定用）
  private downY: number = 0;
  private tapMoved: boolean = false;
  // 轮 B 撤销打点：拖点手势首次实际移动前压一次快照（按下未动不留步）
  private dragUndoMarked: boolean = false;
  @State private zoomPressed: string = '';       // 'in' | 'out' = 按压中的按钮
  // 第二十二包⑤：左上 ☰↶↷ 与缩放钮同款按压回弹（'menu'|'undo'|'redo'）
  @State private topPressed: string = '';
  // 长按对象菜单（上游 长按语义：显隐 / 删除）
  @State private objMenuShow: boolean = false;
  @State private objMenuX: number = 0;
  @State private objMenuY: number = 0;
  @State private objMenuLabel: string = '';
  @State private objMenuType: string = '';
  @State private objMenuVisible: boolean = true;

  // 触摸 vp 坐标 -> 位图像素
  private toPx(vpX: number, vpY: number): [number, number] {
    return [vpX * this.uiScale, vpY * this.uiScale];
  }

  // ---- PC 缩放：滚轮 + 右下角按钮 ----

  // 第二十二包⑤：左上 ☰↶↷ 按压回弹（与缩放钮 zoomAtCenter 同款 120ms）
  private pressTop(id: string): void {
    this.topPressed = id;
    setTimeout(() => {
      this.topPressed = '';
    }, 120);
  }

  // 缩放按钮：按压回弹（zoomPressed 状态 → scale 动画）+ 平滑视图缩放
  //（150ms 内 6 步小倍率逼近视图变换，锚点画布中心，十四轮）
  private zoomAtCenter(factor: number): void {
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
      this.onFrame(dreamemberKernel.kernelGestureZoom(this.canvasW / 2, this.canvasH / 2, stepF));
      done++;
      if (done < steps) {
        setTimeout(tick, 25);
      }
    };
    tick();
  }

  // 鼠标滚轮/触板（轴事件）：以光标为锚缩放，一格 10%。
  // 方向只能看轴值差：scrollStep 是无符号步数（滚上/滚下都为正，真机反馈
  // 两方向全在缩小）；轴值按滚动偏移语义随下滚增大 → 下滚缩小（上游 同向）。
  // 若某设备方向相反，把下面 dv 的符号对调即可。
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
        return;   // 无新滚动量（重复 UPDATE）不重复缩放
      }
      let steps = dv > 0 ? 1 : -1;
      if (ev.scrollStep !== undefined && ev.scrollStep > 0) {
        steps *= Math.min(4, Math.max(1, Math.round(ev.scrollStep)));
      }
      const f = Math.pow(1.1, -steps);
      const cx = (ev.x !== undefined ? ev.x : this.vpW / 2) * this.uiScale;
      const cy = (ev.y !== undefined ? ev.y : this.vpH / 2) * this.uiScale;
      this.onFrame(dreamemberKernel.kernelGestureZoom(cx, cy, f));
    } else if (ev.action === AxisAction.END || ev.action === AxisAction.CANCEL) {
      this.axisLastY = 0;
    }
  }

  // ---- 长按对象菜单 ----

  private cancelLongPress(): void {
    if (this.lpTimer >= 0) {
      clearTimeout(this.lpTimer);
      this.lpTimer = -1;
    }
  }

  private fireLongPress(): void {
    this.lpTimer = -1;
    if (this.lpMoved || this.mode === 3) {
      return;
    }
    const p = this.toPx(this.lpX, this.lpY);
    const objHit: string = dreamemberKernel.kernelPickObj(p[0], p[1]);
    if (objHit === '-|') {
      return;
    }
    const parts = objHit.split('|');
    if (parts.length < 3) {
      return;
    }
    if (this.mode === 1) {
      dreamemberKernel.kernelEndDrag();   // 取消拖点拾取
    }
    this.lpFired = true;
    this.mode = 0;
    this.objMenuLabel = parts[0];
    this.objMenuType = parts[1];
    this.objMenuVisible = parts[2] === '1';
    // 菜单弹出时对象加选中特效（上游 长按选中语义）
    this.onSelectLabel(parts[0]);
    this.objMenuX = Math.min(this.lpX + 8, Math.max(0, this.vpW - 150));
    this.objMenuY = Math.min(this.lpY + 8, Math.max(0, this.vpH - 130));
    this.objMenuShow = true;
  }

  private menuSetVisible(): void {
    this.onSetVisible(this.objMenuLabel, !this.objMenuVisible);
    this.objMenuVisible = !this.objMenuVisible;
    this.objMenuShow = false;
    this.onSelectLabel('');
  }

  private menuDelete(): void {
    this.objMenuShow = false;
    this.onSelectLabel('');
    this.onDeleteObj(this.objMenuLabel);
  }

  // 画布点按选中：先拾取点（pickObj 不含点），再拾取其他对象，命中即上报
  // 选中；未命中不做动作（Down 已清除旧选中）
  private tapSelectObj(): void {
    const p = this.toPx(this.downX, this.downY);
    let label = '';
    const ptHit: string = dreamemberKernel.kernelPickPoint(p[0], p[1]);
    if (ptHit.startsWith('+|')) {
      label = ptHit.substring(2);
    } else {
      const objHit: string = dreamemberKernel.kernelPickObj(p[0], p[1]);
      if (objHit !== '-|') {
        const parts = objHit.split('|');
        if (parts.length >= 3) {
          label = parts[0];
        }
      }
    }
    if (label.length === 0) {
      return;
    }
    this.onSelectLabel(label);
  }

  private handleTouch(ev: TouchEvent): void {
    switch (ev.type) {
      case TouchType.Down: {
        // 点画布 = 点别处：未提交的修改/新建行先按 ✓ 提交并收起（真机反馈），
        // 随后照常收回键盘
        this.onCommitPending();
        this.onDismissKeyboard();
        this.objMenuShow = false;   // 任意按下关闭长按菜单
        // 按下画布即取消选中（与长按/代数区点行互斥）；''→'' 由宿主去重
        this.onSelectLabel('');
        if (ev.touches.length === 1) {
          const p = this.toPx(ev.touches[0].x, ev.touches[0].y);
          this.downX = ev.touches[0].x;
          this.downY = ev.touches[0].y;
          this.tapMoved = false;
          this.lpMoved = false;
          this.lpFired = false;
          if (this.tool === '') {
            const hit: string = dreamemberKernel.kernelPickPoint(p[0], p[1]);
            if (hit.startsWith('+|')) {
              this.mode = 1;   // 拖点
              this.dragUndoMarked = false;
            } else {
              this.mode = 2;   // 待平移
              this.lastPanX = ev.touches[0].x;
              this.lastPanY = ev.touches[0].y;
              this.panRebase = false;
            }
          } else {
            this.mode = 2;   // 工具模式：移动即平移，抬起未移动算点按
            this.lastPanX = ev.touches[0].x;
            this.lastPanY = ev.touches[0].y;
            this.panRebase = false;
          }
          // 长按菜单探测（500ms 未移动即弹菜单）
          this.lpX = ev.touches[0].x;
          this.lpY = ev.touches[0].y;
          if (this.mode === 1 || this.mode === 2) {
            const objHit: string = dreamemberKernel.kernelPickObj(p[0], p[1]);
            if (objHit !== '-|') {
              this.lpTimer = setTimeout(() => this.fireLongPress(), 500);
            }
          }
        } else if (ev.touches.length >= 2) {
          this.cancelLongPress();
          this.tapMoved = true;   // 捏合后抬起不得误派发工具点按
          const p0 = this.toPx(ev.touches[0].x, ev.touches[0].y);
          const p1 = this.toPx(ev.touches[1].x, ev.touches[1].y);
          this.mode = 3;
          this.pinchStartDist = Math.sqrt((p0[0] - p1[0]) ** 2 + (p0[1] - p1[1]) ** 2);
          this.pinchCx = (p0[0] + p1[0]) / 2;
          this.pinchCy = (p0[1] + p1[1]) / 2;
        }
        break;
      }
      case TouchType.Move: {
        if (ev.touches.length === 1 && this.mode !== 3) {
          const mvx = ev.touches[0].x - this.downX;
          const mvy = ev.touches[0].y - this.downY;
          if (mvx * mvx + mvy * mvy > 64) {   // 位移 >8vp 算移动
            this.tapMoved = true;
            this.lpMoved = true;
            this.cancelLongPress();
          }
        }
        if (ev.touches.length >= 2 && this.mode === 3 && this.pinchStartDist > 10) {
          // 双指捏合：以起始中点（位图px）为锚缩放
          const p0 = this.toPx(ev.touches[0].x, ev.touches[0].y);
          const p1 = this.toPx(ev.touches[1].x, ev.touches[1].y);
          const dist = Math.sqrt((p0[0] - p1[0]) ** 2 + (p0[1] - p1[1]) ** 2);
          if (dist > 10) {
            const factor = dist / this.pinchStartDist;
            if (factor > 0.05 && factor < 20) {
              this.onFrame(dreamemberKernel.kernelGestureZoom(this.pinchCx, this.pinchCy, factor));
              this.pinchStartDist = dist;
              this.pinchCx = (p0[0] + p1[0]) / 2;
              this.pinchCy = (p0[1] + p1[1]) / 2;
            }
          }
        } else if (ev.touches.length === 1) {
          const t = ev.touches[0];
          if (this.mode === 1) {
            const p = this.toPx(t.x, t.y);
            if (!this.dragUndoMarked) {
              // 首次实际移动前压撤销快照（整段拖动一步；按下未动不留步）
              this.dragUndoMarked = true;
              dreamemberKernel.kernelPushUndo();
            }
            this.onFrame(dreamemberKernel.kernelDragPointTo(p[0], p[1]));
            // 拖点会级联更新依随对象：宿主节流同步代数区（保帧率）
            this.onDragMoveSync();
          } else if (this.mode === 2) {
            const dx = t.x - this.lastPanX;
            const dy = t.y - this.lastPanY;
            if (this.panRebase) {
              // 手势切换后的首个 Move：只对齐基准点，不平移（防跳变）
              this.panRebase = false;
            } else if (dx !== 0 || dy !== 0) {
              // 平移量用位图像素传给内核
              this.onFrame(dreamemberKernel.kernelPan(dx * this.uiScale, dy * this.uiScale));
            }
            this.lastPanX = t.x;
            this.lastPanY = t.y;
          }
        }
        break;
      }
      case TouchType.Up:
      case TouchType.Cancel: {
        this.cancelLongPress();
        if (this.mode === 3) {
          // 捏合中抬指：还有手指 → 转平移；全部抬起 → 复位。
          // 注意部分机型 Up 的 touches 仍携带已抬起点，不能只按 length===0 判定
          if (ev.touches.length >= 1) {
            this.mode = 2;
            this.lastPanX = ev.touches[0].x;
            this.lastPanY = ev.touches[0].y;
            this.panRebase = true;
          } else {
            this.mode = 0;
          }
          this.pinchStartDist = 0;
          break;
        }
        // 单指交互（拖点/平移/工具点按）一律按整手抬起处理，
        // 不依赖 touches.length（真机：Up 事件 touches 含已抬起的手指点，
        // 按 length===0 判定会漏掉全部工具点按）
        if (this.mode === 1) {
          dreamemberKernel.kernelEndDrag();
          // 抬起补一次代数区最终同步（拖动中节流可能落下最后几帧的值）
          this.onDragEndSync();
          if (!this.tapMoved && !this.lpFired) {
            // 点按点对象：选中特效并同步代数区
            this.tapSelectObj();
          }
        } else if (this.mode === 2 && !this.tapMoved && !this.lpFired && this.tool !== '') {
          // 工具模式点按：交给内核工具状态机
          const p = this.toPx(this.downX, this.downY);
          this.onToolResult(dreamemberKernel.kernelToolTap(p[0], p[1]));
        } else if (this.mode === 2 && !this.tapMoved && !this.lpFired) {
          // 移动工具点按对象：选中特效并同步代数区（空白点按 Down 已清除）
          this.tapSelectObj();
        }
        this.mode = 0;
        this.pinchStartDist = 0;
        break;
      }
      default:
        break;
    }
  }

  build() {
    // 画布 Stack 占满本组件（工具栏由宿主排在组件上方，见 CanvasView）
    Stack({ alignContent: Alignment.TopStart }) {
      Canvas(this.ctx)
        .width('100%')
        .height('100%')
        .backgroundColor(Theme.canvas(this.dark))
        .onAreaChange((_o: Area, n: Area) => {
          this.vpW = n.width as number;
          this.vpH = n.height as number;
        })
        .onReady(() => {
          // ctx.width/height 是位图像素；vp 尺寸此时已就绪。
          // 宿主持有同名尺寸字段（kernelRender 入参），在此上报
          this.canvasW = this.ctx.width;
          this.canvasH = this.ctx.height;
          this.uiScale = this.vpW > 0 ? this.canvasW / this.vpW : 1.0;
          this.onCanvasReady(this.canvasW, this.canvasH, this.uiScale);
        })
        .onTouch((ev: TouchEvent) => this.handleTouch(ev))
        .onAxisEvent((ev: AxisEvent) => this.onCanvasAxis(ev))
      // 创建/命令反馈条：让位给左上角按钮组，移到其下方（真机反馈①）；
      // 自动消失由宿主 CanvasView 的 showHint 定时器负责
      if (this.toolHint.length > 0) {
        Text(this.toolHint)
          .fontSize(11)
          .fontColor(Theme.text(this.dark))
          .maxLines(2)
          .padding({ left: 8, right: 8, top: 4, bottom: 4 })
          .backgroundColor(this.dark ? 'rgba(42,45,49,0.92)' : 'rgba(255,255,255,0.92)')
          .borderRadius(6)
          .constraintSize({ maxWidth: this.vpW > 16 ? this.vpW - 16 : 300 })
          .position({ x: 8, y: 58 })
      }
      // 反馈②：☰ ↶ ↷ 按钮组浮于画布左上角（真机反馈①：自右上角改到左上，
      // 上游 同位；缩放按钮同款圆形样式）。
      // ☰ 用三横条自绘（字形基线偏移导致圆钮内不居中的真机反馈）；无可
      // 回退/重做步时置灰不响应。状态/动作均为宿主直传回调（铁律：内联
      // 读取 @Prop，不经 @Builder）
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
          // 第二十二包⑤：↶↷ 与 ☰ 及缩放钮同款按压回弹（scale 0.86）
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
      if (this.objMenuShow) {
        // 长按对象菜单（上游 长按语义：显隐 / 删除）
        Column() {
          Text(this.objMenuLabel + ' · ' + Theme.typeName(this.objMenuType))
            .fontSize(12)
            .fontColor(Theme.sub(this.dark))
            .width('100%')
            .padding({ left: 12, right: 12, top: 8, bottom: 6 })
          Text(this.objMenuVisible ? '隐藏' : '显示')
            .fontSize(14)
            .fontColor(Theme.text(this.dark))
            .width('100%')
            .padding({ left: 12, right: 12, top: 8, bottom: 8 })
            .onClick(() => this.menuSetVisible())
          Text('删除')
            .fontSize(14)
            .fontColor(Theme.del(this.dark))
            .width('100%')
            .padding({ left: 12, right: 12, top: 8, bottom: 10 })
            .onClick(() => this.menuDelete())
        }
        .width(140)
        .borderRadius(10)
        .backgroundColor(this.dark ? 'rgba(42,45,49,0.97)' : 'rgba(255,255,255,0.97)')
        .shadow({ radius: 12, color: 'rgba(0,0,0,0.25)', offsetX: 0, offsetY: 2 })
        .position({ x: this.objMenuX, y: this.objMenuY })
      }
      // 缩放按钮（PC 无触屏；触屏设备同样可用）：右下角 ＋/－，锚点画布中心。
      // 按压回弹（zoomPressed → scale）+ 视图 150ms 平滑缩放（十四轮）
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
            .onClick(() => this.zoomAtCenter(1.25))
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
            .onClick(() => this.zoomAtCenter(0.8))
        }
        .position({ x: this.vpW - 60, y: this.vpH - 112 })
        .transition(TransitionEffect.asymmetric(
          TransitionEffect.opacity(0).combine(TransitionEffect.scale({ x: 0.6, y: 0.6 })),
          TransitionEffect.opacity(0).combine(TransitionEffect.scale({ x: 0.6, y: 0.6 }))
        ).animation({ duration: 180, curve: Curve.EaseOut }))
      }
    }
    .width('100%')
    .layoutWeight(1)
  }
}
