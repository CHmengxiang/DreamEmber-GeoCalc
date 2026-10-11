// RatexView.ets — RaTeX LaTeX 排版渲染组件（五十二包）。
// 链路：线性数学串 → native ratexConvert（转 LaTeX，含未知构造返回 raw）
// → native ratexLayout（libratex_ffi.so 排版）→ DisplayList JSON →
// 本组件解析并用 Canvas 绘制。
//
// DisplayList 协议 v1（RaTeX docs/DISPLAYLIST_JSON_PROTOCOL.md）：
//   {version, width, height(ascent), depth(descent), items[]}，坐标全 em
//   （×fontSize=像素），**原点左上、y 向下、基线在 y=height**（五十四包
//   修正③：item.y 直接从盒顶量取绘制，不得再叠加基线偏移——此前把 item.y
//   加到基线上，整体下移一个 ascent，下半被画布裁掉）。item 四型：
//   GlyphPath{font 短名, char_code, x, y, scale}（KaTeX 字体字形，字体短名
//   ↔ rawfile/fonts/KaTeX_*.ttf 映射）、Line（分数线/上划线）、Rect、Path
//   （MoveTo/LineTo/CubicTo/QuadTo/Close，根号大钩形等）。
//
// 字体加载：@ohos.font.registerFont 以 $rawfile 路径注册 KaTeX 全套 TTF
//（family 名 = Katex<短名去连字符>）。字形绘制直接 fillText(char)
//（KaTeX 字体注册后按 font 名渲染，无需 outline 重建）。五十四包修正④：
// 真机上 fillText 曾回退系统字体——根号 √（Main-Regular，字面高 0.853em、
// 基线下探 -0.2em）是按 KaTeX 度量与横线对位的，系统字形矮一截够不到
// 横线，视觉上"钩与横线分离"；改走 UIContext Font 注册 + Resource 形式
// familySrc，并加延迟重绘兜底与字体回退探测日志。
//
// 异步时序：registerFont 提交后触发 repaint；首次布局在字体就绪后进行。
import { canvasDensity } from './canvasUtil';
import ratexNative from 'libdreamember.so';
import font from '@ohos.font';
import type { UIContext } from '@ohos.arkui.UIContext';

// KaTeX 字体短名 → rawfile 文件名映射（RaTeX FontId::as_str）
const KATEX_FONTS: Map<string, string> = new Map([
  ['Main-Regular', 'KaTeX_Main-Regular.ttf'],
  ['Main-Bold', 'KaTeX_Main-Bold.ttf'],
  ['Main-Italic', 'KaTeX_Main-Italic.ttf'],
  ['Main-BoldItalic', 'KaTeX_Main-BoldItalic.ttf'],
  ['Math-Italic', 'KaTeX_Math-Italic.ttf'],
  ['Math-BoldItalic', 'KaTeX_Math-BoldItalic.ttf'],
  ['AMS-Regular', 'KaTeX_AMS-Regular.ttf'],
  ['Caligraphic-Regular', 'KaTeX_Caligraphic-Regular.ttf'],
  ['Caligraphic-Bold', 'KaTeX_Caligraphic-Bold.ttf'],
  ['Fraktur-Regular', 'KaTeX_Fraktur-Regular.ttf'],
  ['Fraktur-Bold', 'KaTeX_Fraktur-Bold.ttf'],
  ['SansSerif-Regular', 'KaTeX_SansSerif-Regular.ttf'],
  ['SansSerif-Italic', 'KaTeX_SansSerif-Italic.ttf'],
  ['SansSerif-Bold', 'KaTeX_SansSerif-Bold.ttf'],
  ['Script-Regular', 'KaTeX_Script-Regular.ttf'],
  ['Typewriter-Regular', 'KaTeX_Typewriter-Regular.ttf'],
  ['Size1-Regular', 'KaTeX_Size1-Regular.ttf'],
  ['Size2-Regular', 'KaTeX_Size2-Regular.ttf'],
  ['Size3-Regular', 'KaTeX_Size3-Regular.ttf'],
  ['Size4-Regular', 'KaTeX_Size4-Regular.ttf'],
]);

// 已注册字体集合（进程级，重复注册会报错）
const registeredFonts: Set<string> = new Set();

interface DisplayListItem {
  type?: string;
  x?: number;
  y?: number;
  width?: number;
  height?: number;
  thickness?: number;
  scale?: number;
  font?: string;
  char_code?: number;
  commands?: DisplayPathCommand[];
  dashed?: boolean;
  color?: number[] | Record<string, number>;  // [r,g,b] 或 {r,g,b,a} 0-1
}

interface DisplayPathCommand {
  type?: string;
  x?: number;
  y?: number;
  x1?: number;
  y1?: number;
  x2?: number;
  y2?: number;
}

interface DisplayList {
  version?: number;
  width?: number;
  height?: number;
  depth?: number;
  items?: DisplayListItem[];
}

// 排版结果（em 几何原样持有，绘制时统一 ×fontSize）
export class RatexLayoutResult {
  json: string = '';
  widthEm: number = 0;
  heightEm: number = 0;
  depthEm: number = 0;
  items: DisplayListItem[] = [];
}

// 解析 DisplayList JSON（协议向前兼容：未知字段忽略、缺字段用默认值）；
// 结构异常抛错（调用方回退旧 painter）
export function parseRatexDisplayList(json: string): RatexLayoutResult {
  const raw = JSON.parse(json) as Record<string, object>;
  const out = new RatexLayoutResult();
  out.json = json;
  out.widthEm = raw['width'] !== undefined ? Number(raw['width']) : 0;
  out.heightEm = raw['height'] !== undefined ? Number(raw['height']) : 0;
  out.depthEm = raw['depth'] !== undefined ? Number(raw['depth']) : 0;
  const items = raw['items'];
  if (Array.isArray(items)) {
    out.items = items as DisplayListItem[];
  }
  if (out.items.length === 0 && out.widthEm === 0 && out.heightEm === 0) {
    throw new Error('ratex empty display list');
  }
  return out;
}

// 线性串 → LaTeX（native 转换；raw| 前缀=含未知构造，调用方回退）
export function ratexConvertLinear(linear: string): string | null {
  try {
    const r = ratexNative.ratexConvert(linear) as string;
    if (r.startsWith('ok|')) {
      return r.substring(3);
    }
    return null;
  } catch (e) {
    return null;
  }
}

// LaTeX → DisplayList（native 排版；异常/错误返回 null）。
// color 可选 '#RRGGBB'（五十四包：主题色透传 native，DisplayList 内 item
// 色即主题色；缺省黑）
export function ratexLayoutLatex(latex: string, color?: string): RatexLayoutResult | null {
  try {
    const r = ratexNative.ratexLayout(latex, 0, color) as string;
    if (!r.startsWith('ok|')) {
      return null;
    }
    return parseRatexDisplayList(r.substring(3));
  } catch (e) {
    return null;
  }
}

// 排版可用性探测（MathText 快速判定用，不保留结果）：转换+排版全通才 true
export function ratexLayoutOk(latex: string): boolean {
  return ratexLayoutLatex(latex) !== null;
}

// 注册单个 KaTeX 字体（五十四包④）：优先 UIContext Font 注册（全局
// font.registerFont 自 API18 起废弃，真机上疑似未生效致 fillText 回退系统
// 字体），失败再落全局接口；familySrc 用 $rawfile Resource 形式。
// 五十五包②：forceGlobal 跳过 UIContext 通道（探测发现回退后换通道重试用）
function registerOneFont(family: string, file: string, ui?: UIContext,
  forceGlobal: boolean = false): boolean {
  const opts: font.FontOptions = {
    familyName: family,
    familySrc: $rawfile(`fonts/${file}`)
  };
  if (ui !== undefined && !forceGlobal) {
    try {
      ui.getFont().registerFont(opts);
      return true;
    } catch (e) {
      // 落全局接口重试
    }
  }
  try {
    font.registerFont(opts);
    return true;
  } catch (e) {
    return false;
  }
}

// 注册全部 KaTeX 字体（幂等，按 family 记账；注册失败不记账、下个组件
// 实例创建时重试）。registerFont 同步返回，但部分版本底层字体装载异步
// 完成——宿主组件需延迟重绘兜底（见 RatexView.aboutToAppear）。
// 五十五包②：单字体失败打日志（registerFont 不抛错≠底层装载成功，
// 真机证据靠 [ratex] 日志链收集）
export function registerKatexFonts(done: () => void, ui?: UIContext): void {
  KATEX_FONTS.forEach((file: string, short: string) => {
    if (registeredFonts.has(short)) {
      return;
    }
    const family = 'Katex' + short.replace(/-/g, '');
    if (registerOneFont(family, file, ui)) {
      registeredFonts.add(short);
    } else {
      console.warn(`[ratex] register FAIL: ${family} (${file})`);
    }
  });
  done();
}

// 五十五包②：指定字体换全局通道重注册（探测到 fillText 仍回退系统字体
// 时调用——UIContext 注册成功但底层未装载的场景，换通道有机会救回）。
// 先清记账再注册，成功与否都记账（避免无限重试，交给外层重试上限）。
export function retryKatexFonts(shorts: string[], ui?: UIContext): void {
  shorts.forEach((short: string) => {
    const file = KATEX_FONTS.get(short);
    if (file === undefined) {
      return;
    }
    const family = 'Katex' + short.replace(/-/g, '');
    registeredFonts.delete(short);
    try {
      font.registerFont({
        familyName: family,
        familySrc: $rawfile(`fonts/${file}`)
      });
      console.warn(`[ratex] re-register via global: ${family}`);
      registeredFonts.add(short);
    } catch (e) {
      console.warn(`[ratex] re-register FAIL: ${family}`);
      registeredFonts.add(short);  // 记账防死循环，本轮放弃
    }
  });
}

@Component
export struct RatexView {
  @Prop @Watch('repaint') raw: string = '';
  @Prop @Watch('repaint') dark: boolean = false;
  @Prop @Watch('repaint') fontSize: number = 13;
  @Prop @Watch('repaint') color: string = '#333333';
  onResult: (ok: boolean) => void = (_ok: boolean) => {};
  @State private contentW: number = 40;
  @State private contentH: number = 22;
  private settings: RenderingContextSettings = new RenderingContextSettings(true);
  private ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.settings);
  private ready: boolean = false;
  private fontsReady: boolean = false;
  private lay: RatexLayoutResult | null = null;
  private den: number = canvasDensity();
  private probed: boolean = false;   // 字体回退探测只跑一次
  private retryCount: number = 0;    // 回退重注册轮数上限 2（五十五包②）

  aboutToAppear(): void {
    registerKatexFonts(() => {
      this.fontsReady = true;
      this.repaint();
    }, this.getUIContext());
    // 兜底重绘：部分版本底层字体装载异步完成，晚到的重绘避免首帧
    // （乃至常驻）回退系统字体
    setTimeout(() => this.repaint(), 150);
    setTimeout(() => this.repaint(), 600);
  }

  repaint(): void {
    if (this.ready) {
      this.paint();
    }
  }

  // 线性串 → LaTeX → 排版 → 几何就绪；任一步失败返回 false（宿主回退）
  private layout(): boolean {
    const latex = ratexConvertLinear(this.raw);
    if (latex === null) {
      return false;
    }
    const lay = ratexLayoutLatex(latex, this.color);
    if (lay === null) {
      return false;
    }
    this.lay = lay;
    return true;
  }

  // 字体回退探测（五十四包④引入、五十五包②扩展）：measureText 宽度与
  // 系统字体一致即判回退。探两只字体——Main-Regular 的 √（根号钩与横线
  // 对位依赖它）与 Math-Italic 的 x（斜体字母，真机曾出现"时有时无"）。
  // 探到回退：换全局通道重注册 + 延迟重绘再探，至多两轮（retryCount 上限）。
  private probeFontOnce(): void {
    if (this.probed || !this.fontsReady) {
      return;
    }
    this.probed = true;
    try {
      const probes: Array<[string, string]> = [
        ['Main-Regular', '\u221A'],
        ['Math-Italic', 'x'],
      ];
      const fallback: string[] = [];
      for (let i = 0; i < probes.length; i++) {
        const short = probes[i][0];
        const ch = probes[i][1];
        const family = 'Katex' + short.replace(/-/g, '');
        this.ctx.font = `40px ${family}`;
        const wk = this.ctx.measureText(ch).width;
        this.ctx.font = '40px sans-serif';
        const ws = this.ctx.measureText(ch).width;
        const same = Math.abs(wk - ws) < 0.5;
        console.info(`[ratex] font probe ${short} ${ch}: katex=${wk.toFixed(1)}`
          + ` sys=${ws.toFixed(1)} fallback=${same ? 'YES(字体未生效!)' : 'no'}`);
        if (same) {
          fallback.push(short);
        }
      }
      if (fallback.length > 0 && this.retryCount < 2) {
        this.retryCount++;
        retryKatexFonts(fallback, this.getUIContext());
        this.probed = false;  // 重注册后再探一轮
        setTimeout(() => this.repaint(), 250);
      }
    } catch (e) {
      // 探测失败不影响绘制
    }
  }

  private paint(): void {
    if (!this.layout()) {
      this.onResult(false);
      return;
    }
    const lay = this.lay as RatexLayoutResult;
    const fs = this.fontSize;
    // em → vp：坐标全 em × fontSize；Canvas 坐标 vp 口径（第十五包③终案）。
    // 五十四包③修正：item.y 为盒顶起算（基线在 y=height），直接绘制，
    // 只加 pad 顶边距——不再叠加基线偏移（旧码整体下移一个 ascent 被裁）
    const pad = 2;
    const w = Math.max(12, Math.ceil(lay.widthEm * fs) + pad * 2);
    const h = Math.max(fs + 6, Math.ceil((lay.heightEm + lay.depthEm) * fs) + pad * 2);
    if (Math.abs(w - this.contentW) > 0.5 || Math.abs(h - this.contentH) > 0.5) {
      this.contentW = w;
      this.contentH = h;
    }
    const color = this.color;
    this.ctx.clearRect(0, 0, this.contentW + 8, this.contentH + 8);
    this.ctx.fillStyle = color;
    this.ctx.strokeStyle = color;
    this.ctx.textBaseline = 'alphabetic';
    this.probeFontOnce();
    for (let i = 0; i < lay.items.length; i++) {
      this.drawItem(lay.items[i], fs, pad, color);
    }
    this.onResult(true);
  }

  private itemColor(item: DisplayListItem, fallback: string): string {
    const c = item.color;
    if (Array.isArray(c) && c.length >= 3) {
      const to255 = (v: number): number => Math.round(Math.max(0, Math.min(1, v)) * 255);
      const hex = (v: number): string => v.toString(16).padStart(2, '0');
      return '#' + hex(to255(c[0])) + hex(to255(c[1])) + hex(to255(c[2]));
    }
    if (c !== undefined && !Array.isArray(c)) {
      const r = c['r'];
      const g = c['g'];
      const b = c['b'];
      if (typeof r === 'number' && typeof g === 'number' && typeof b === 'number') {
        const to255 = (v: number): number => Math.round(Math.max(0, Math.min(1, v)) * 255);
        const hex = (v: number): string => v.toString(16).padStart(2, '0');
        return '#' + hex(to255(r)) + hex(to255(g)) + hex(to255(b));
      }
    }
    return fallback;
  }

  private drawItem(item: DisplayListItem, fs: number, topPad: number,
    fallbackColor: string): void {
    const color = this.itemColor(item, fallbackColor);
    const x = (item.x !== undefined ? item.x : 0) * fs;
    const y = topPad + (item.y !== undefined ? item.y : 0) * fs;
    const t = item.type;
    if (t === 'GlyphPath') {
      const short = item.font !== undefined ? item.font : 'Main-Regular';
      const file = KATEX_FONTS.get(short);
      if (file === undefined || item.char_code === undefined) {
        return;
      }
      const family = 'Katex' + short.replace(/-/g, '');
      const scale = item.scale !== undefined ? item.scale : 1;
      this.ctx.font = `${Math.round(fs * scale * this.den)}px ${family}`;
      this.ctx.fillStyle = color;
      this.ctx.fillText(String.fromCodePoint(item.char_code), x, y);
    } else if (t === 'Line') {
      // 分数线/上划线：y 为线中心，thickness 为线宽
      const th = Math.max(0.5, (item.thickness !== undefined ? item.thickness : 0.04) * fs);
      const w = (item.width !== undefined ? item.width : 0) * fs;
      this.ctx.fillStyle = color;
      this.ctx.fillRect(x, y - th / 2, w, th);
    } else if (t === 'Rect') {
      this.ctx.fillStyle = color;
      this.ctx.fillRect(x, y,
        (item.width !== undefined ? item.width : 0) * fs,
        (item.height !== undefined ? item.height : 0) * fs);
    } else if (t === 'Path') {
      this.drawPath(item, fs, topPad, color);
    }
  }

  private drawPath(item: DisplayListItem, fs: number, topPad: number,
    color: string): void {
    const cmds = item.commands;
    if (!Array.isArray(cmds) || cmds.length === 0) {
      return;
    }
    const ox = (item.x !== undefined ? item.x : 0) * fs;
    const oy = topPad + (item.y !== undefined ? item.y : 0) * fs;
    const ctx = this.ctx;
    ctx.beginPath();
    let open = false;
    for (let i = 0; i < cmds.length; i++) {
      const c = cmds[i];
      const t = c.type;
      if (t === 'MoveTo' && c.x !== undefined && c.y !== undefined) {
        ctx.moveTo(ox + c.x * fs, oy + c.y * fs);
        open = true;
      } else if (t === 'LineTo' && c.x !== undefined && c.y !== undefined) {
        ctx.lineTo(ox + c.x * fs, oy + c.y * fs);
      } else if (t === 'CubicTo' && c.x1 !== undefined && c.x2 !== undefined
        && c.x !== undefined) {
        ctx.bezierCurveTo(ox + c.x1 * fs, oy + (c.y1 as number) * fs,
          ox + c.x2 * fs, oy + (c.y2 as number) * fs,
          ox + c.x * fs, oy + (c.y as number) * fs);
      } else if (t === 'QuadTo' && c.x1 !== undefined && c.x !== undefined) {
        ctx.quadraticCurveTo(ox + c.x1 * fs, oy + (c.y1 as number) * fs,
          ox + c.x * fs, oy + (c.y as number) * fs);
      } else if (t === 'Close') {
        ctx.closePath();
      }
    }
    if (!open) {
      return;
    }
    if (item.dashed === true) {
      ctx.setLineDash([3, 2]);
    }
    ctx.fillStyle = color;
    ctx.fill();
    ctx.setLineDash([]);
  }

  build() {
    Canvas(this.ctx)
      .width(this.contentW)
      .height(this.contentH)
      .constraintSize({ maxWidth: '78%' })
      .clip(true)
      .onReady(() => {
        this.ready = true;
        this.paint();
      })
      .onAreaChange(() => this.repaint())
  }
}
