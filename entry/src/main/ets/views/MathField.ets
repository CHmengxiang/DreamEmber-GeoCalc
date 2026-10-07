// MathField.ets — 公式化输入显示（第七包，上游 输入框同体验）。
// 把线性原始串渲染成"类排版"公式：^2 的 2 渲染为右上角上标、sqrt(...) 渲染
// 为 √ 加参数上划线（vinculum）、abs(...) 渲染为 |·|、空参数 () 渲染为灰色
// 占位框 □、0.(3) 的循环位带上划线、a_n 的 n 渲染为下标。光标为内联闪烁
// 竖线，按原文插入位置渲染（随 ←/→/退格/插入同步）。
//
// 原子化解析（parseMathAtoms）：每个原子覆盖原文的一个区间 [start,end)，
// 被吞掉的语法字符（^ ( ) sqrt( 等）并入相邻原子的区间，保证光标索引到
// 显示位置的映射连续。堆栈分式（五十包）：applyFractions 后处理把 "/"
// 与两侧因子重组为分式原子（fracNum/fracDen 子原子组），纯显示层变换、
// 原文串与内核解析不变；划界规则与内核 parseTerm 左结合语义一致，显示
// 不与内核求值矛盾（详见 applyFractions 注释）。
//
// 2026-10-01 第九包（真机反馈）：①□_□ 改插空下标模板 '_'（不预填 a/n），
// 无 base 时渲染 base 占位框 + 零宽下标占位框（键帽框_框）；②新增编辑
// 模型导出（hasIdentBase / scriptContentRange / templateUnwrapRange /
// isTypewriterCh，CanvasView 插入/退格的角标区重定向与函数模板拆除）。
// 2026-10-01 第十包（真机反馈）：①键帽去 "_" 字形（第二框直接作角标，
// 在 MathKeyboard）；②光标升级「原文位置 + 框内/框外双态」，上游
// editor-base CursorController/InputController 语义线性移植——导出
// MathCaretStop / mathCaretStops（←/→ 停靠序列）、scriptStructureEnd /
// scriptEndsAtCaret / matchParenBackward / functionOpenBefore（⌫ 语义），
// buildDisplayAtoms 增加 caretScript 参数（光标小号上/下移 vs 基线）。
// 2026-10-01 第十一包（真机反馈）：①键帽 OK；②→ 切换框内外时光标样式
// 不刷新（ForEach key 未含样式，同 key 不重建、旧字号/偏移滞留）→ key
// 补样式签名；③□_□ 第一框光标进不去 → 空 base 不产框首框外停靠 +
// 光标画进占位框（caretIn，占位框 □ 字形改虚线画框）；④框外结构尾键入
// 去 "*" 隔断 → 上游 序列化器同款 ^(2) 括号包裹（解析支持 ^()/_() 空括
// 号组），scriptEndsAtCaret 升级为 scriptRunEndAtCaret（任意 run 终结探
// 测）；⑤⌫ 对齐 上游 RemoveContainer（查证 ed_RemoveContainer.java：脚本
// 节点为空壳、base 留在父序列）——框首拆壳拼回（x^2→x2、x^(23)→x23、
// 空内容 x^→x），框外空框只进框不删（不再整删公式）；⑥根号一体渲染
// （MathRadicalView：Path 连续钩形+顶线，参数内容作 kids 子原子）。
// 2026-10-01 第十二包（真机反馈）：①根号仍断开且更严重 + ②用户询问能否
// 用 LaTeX/KaTeX——KaTeX 输出 HTML/CSS 必须跑在 WebView 里，与"主功能禁
// 用 Web 组件"冲突 → 渲染层整体改 Canvas 自绘排版：钩形+顶线一条连续折
// 线一次 stroke（根号天然一体），上/下标、占位虚线框、光标、循环位上划
// 线按基线精确绘制；MathField 与 MathText 共用同一 painter（编辑/静态两
// 态），ForEach/key 渲染机制整体退役（第十一包②同 key 不重建类问题根
// 除，MathRadicalView 删除）；③框_框 角标 ← 不到大框且键入进小框——
// (a) 框外结构尾键入的 ^(…) 包裹插入点差一（CanvasView 修，rng[1]+2），
// (b) 空脚本内容的框内停靠标记 empty，← 跳过一次到位（mathCaretStops）。
//
// 本组件为纯显示层：编辑仍由其覆盖的（视觉隐藏的）TextInput 承担——焦点、
// customKeyboard 弹出、插入/退格/光标逻辑全部复用 CanvasView 既有链路。
// 2026-10-01 第十三包（真机反馈）：①根号一体 ✅ 结案；②Canvas 排版文字
// 很小（直方图改好之前同款）——Canvas font-size 单位只支持 px（官方文档），
// 真机按物理像素渲染，与 fp≈vp 口径的 Text 组件差一个屏幕密度倍（3x 设备
// 正好小 3 倍；第六包"font 不生效"定论实为此尺度误诊：11→14 物理像素肉眼
// 难辨）。修复：字号按 display.densityPixels 换算（fontPx = fp*k），排版几何
// 与 measureText 宽度（/k）保持 vp 口径。
// 2026-10-02 第十四包（真机反馈）：①字号正常但光标落 x 中间偏左（上上
// 个版本无此问题）——定案 Canvas **坐标与 font-size 同为物理 px**（排版
// advance /k 后落后字形实际 px 宽约 2/3 → 光标落字中；第五包"坐标制 vp"
// 定论据此修正）。painter 整体改 px 坐标系：measureText 去 /k、几何常量
// 全 ×k、组件宽 = px 宽 /k 折回。
// 2026-10-02 第十五包（真机反馈）：③输入全部不可见（盲打有效）——十四包
// "坐标=px"被反证：×k 后全部笔画落到可视区外 → **坐标与 measureText 均
// 为 vp、唯独 font-size 是物理 px**（四轮证据链闭环：十二包几何对/字小、
// 十三包字对/光标 33% 处、十四包全 ×k 画出界）。painter 回 vp 坐标（撤
// ×k），保留 font ×k 与 measure 原值直用。
// 2026-10-02 第十六包（真机反馈）：②光标经常不可见——blink 定时器只翻
// @State 无人重绘，光标仅在属性变化触发的重绘里出现、一半概率撞上
// blink=false 被画没。修复：翻转相后主动重绘 + 键入/移动/聚焦后重置相位
// 先常亮（restartBlink，上游 光标操作后必见同款）；MathField 的 @Watch
// 全部改道 onDispChange→restartBlink（onAreaChange 仍走 repaint 不重置
// 相位）。角标框 ←/→ 进不去的跳站删除在 CanvasView（十六包②）。
// 2026-10-02 第十七包（真机反馈）：④大框接小框（^/_）光标恒按大框渲染
// ——CanvasView 的 onSelChange 选区回写门禁（本包③④根因，CanvasView 修）
// 之后，本文件收尾：小框 caretIn 不再加高（sup −8/8、sub −3/8 普通几何，
// 仅 base 大框保留 −15/18），框内光标条随框高收紧（大框 ±5 不变，小框
// 缩进框内）；canvasDensity 导出（CanvasView 键盘避让 px→vp 复用）。
// 2026-10-02 第十九包（真机反馈）：③小框有文字时大小框间光标高度不变
// ——回声门禁只治了"框内态被洗掉"，框内首站仍恒大框：buildDisplayAtoms
// 前向邻居匹配 start >= cp 失配于吞语法字符的脚本原子（x^2 的 '2' 原子
// start=1 是 ^ 的下标，cp=内容起点 2 时匹配不上），后向也无人 end <= cp
// → csup=false → 基线大光标，恰在大小框交界处高度不变。前向条件改
// end > cp（首个未越过的脚本原子），框内各站（首/中/尾）均按 run 样式
// 渲染小号光标；空框 caretIn 路径不受影响。
// 2026-10-07 第五十包（堆栈分式）：①parseMathAtoms 后新增 applyFractions
// 显示层后处理——顶层 "/" 与两侧因子重组为分式原子（fracNum/fracDen 子
// 原子组 + slash 分界下标），分子回溯到 +−=<>,; 空格、未堆栈 "/"、既有
// 分式（嵌套堆栈留待行高支持）边界，分母取单个最小因子加紧随角标/后缀，
// 任一侧为空不堆栈；②分式排版绘制（mathLayout/mathDraw kind 4，子布局
// 根号高压缩防溢出）；③光标全链路：buildDisplayAtoms 分式下钻（cp≤slash
// 落分子尾前、否则分母），mathCaretStops 重构为递归 walk（分式与根号内
// 容全遍历，根号内角标首获框内停靠），点触反查 scriptStopSpan /
// baseStopXRec / stopIsSup 分式下钻；④MathText 静态行分式动态加高；编
// 辑行仍 30（单层分式恰容；嵌套分式与分母降部字裁切为已知限制）。
// 2026-10-07 第五十一包（真机反馈）：分子根号渲染错位（√(x)/2 的 x 偏左
// 与钩形重叠）——分式分支只平移了子布局顶层片，根号 kids 是子布局绝对
// 坐标没跟着动 → shiftPiecesX 递归平移（kind 2 kids / kind 4 子表一并，
// 点触反查等一切读片坐标处随之一致；NthRoot 根次片同获修正）。
import { Theme } from '../model/Theme';
import display from '@ohos.display';

// 屏幕密度（px/vp，如 3x 设备 = 3.0）：Canvas 坐标与 measureText 为 vp
// 口径、唯独 font-size 为物理 px（第十五包③四轮证据链终案）——字号
// ×k 换算、几何与宽度 vp 原值直用。第十七包②起 CanvasView 键盘避让的
// Area.globalPosition（px）换算也复用本函数
export function canvasDensity(): number {
  try {
    return display.getDefaultDisplaySync().densityPixels;
  } catch (e) {
    return 3;
  }
}

export interface MathAtom {
  ch: string;      // 显示文本（dim 占位为 '□'）
  sup: boolean;    // 上标（小号上移）
  sub: boolean;    // 下标（小号下移）
  over: boolean;   // 上划线（√ 参数 / 循环位）
  dim: boolean;    // 空参数占位框（灰色 □）
  caret: boolean;  // 光标伪原子
  start: number;   // 覆盖原文区间 [start, end)
  end: number;
  caretIn?: boolean;   // 光标画进占位框内（第十一包③）
  kids?: MathAtom[];   // 根号组内容子原子（第十一包⑥一体渲染）
  idx?: MathAtom[];    // 根次子原子（四十七包④ NthRoot：下标小字置于钩形缺口）
  fracNum?: MathAtom[]; // 堆栈分式分子子原子组（五十包；原子 ch='' 自身不渲染）
  fracDen?: MathAtom[]; // 堆栈分式分母子原子组
  slash?: number;      // "/" 的原文下标（分子/分母分界：cp≤slash → 分子）
}

const DIGITS: string = '0123456789';

function isDigitCh(c: string): boolean {
  return c.length === 1 && DIGITS.indexOf(c) >= 0;
}

function isAlnumCh(c: string): boolean {
  if (c.length !== 1) {
    return false;
  }
  const u = c.charCodeAt(0);
  return (u >= 48 && u <= 57) || (u >= 65 && u <= 90) || (u >= 97 && u <= 122);
}

function isAlphaCh(c: string): boolean {
  if (c.length !== 1) {
    return false;
  }
  const u = c.charCodeAt(0);
  return (u >= 65 && u <= 90) || (u >= 97 && u <= 122);
}

// 希腊字母（U+0370–U+03FF；ArkTS 字符串内为单个 UTF-16 码元，含 π）
function isGreekCh(c: string): boolean {
  if (c.length !== 1) {
    return false;
  }
  const u = c.charCodeAt(0);
  return u >= 0x0370 && u <= 0x03FF;
}

// ^/_ 脚本内容的可续写字符（四十七包③：字母数字 + 希腊——π/α 等键入后
// 必须并进脚本 run 渲染成角标，旧版只认 ASCII 致 e^π 渲染成 e□π）
function isScriptCh(c: string): boolean {
  return isAlnumCh(c) || isGreekCh(c);
}

// 括号匹配：open 为 '(' 的下标，返回配对 ')' 下标；无配对返回 -1
function matchParen(raw: string, open: number): number {
  let depth = 0;
  for (let j = open; j < raw.length; j++) {
    const c = raw.charAt(j);
    if (c === '(') {
      depth++;
    } else if (c === ')') {
      depth--;
      if (depth === 0) {
        return j;
      }
    }
  }
  return -1;
}

// 把 [from, to) 的字符按指定样式推入原子表；首原子并入 [preStart, from)
// 的吞掉字符，末原子并入 [to, postEnd) 的吞掉字符
function pushRun(atoms: MathAtom[], raw: string, from: number, to: number,
  sup: boolean, sub: boolean, over: boolean, dim: boolean,
  preStart: number, postEnd: number): void {
  for (let j = from; j < to; j++) {
    const a: MathAtom = {
      ch: raw.charAt(j), sup: sup, sub: sub, over: over, dim: dim,
      caret: false,
      start: (j === from ? Math.min(preStart, j) : j),
      end: j + 1
    };
    atoms.push(a);
  }
  if (to > from && atoms.length > 0) {
    const last = atoms[atoms.length - 1];
    last.end = Math.max(last.end, postEnd);
  }
}

// 原子化解析：raw → 原子表（每个原文下标恰好被一个原子覆盖）
export function parseMathAtoms(raw: string): MathAtom[] {
  const atoms: MathAtom[] = [];
  let i = 0;
  while (i < raw.length) {
    const c = raw.charAt(i);

    // ---------- ^ 上标 ----------
    if (c === '^') {
      if (i + 1 < raw.length && raw.charAt(i + 1) === '(') {
        const m = matchParen(raw, i + 1);
        if (m > 0) {
          if (m === i + 2) {
            // ^() 空指数括号组（第十一包④：框外键入把空框包成 ^(…) 后
            // 兄弟字符可插在组后）：占位框吞整个 ^()
            const ae: MathAtom = { ch: '□', sup: true, sub: false, over: false, dim: true, caret: false, start: i, end: m + 1 };
            atoms.push(ae);
          } else {
            // ^(...) 整组渲染为上标，^ 与括号吞进首末原子
            pushRun(atoms, raw, i + 2, m, true, false, false, false, i, m + 1);
          }
          i = m + 1;
          continue;
        }
      }
      let j = i + 1;
      while (j < raw.length && isScriptCh(raw.charAt(j))) {
        j++;
      }
      if (j > i + 1) {
        pushRun(atoms, raw, i + 1, j, true, false, false, false, i, j);
        i = j;
        continue;
      }
      // 悬空 ^：空指数占位框
      const a: MathAtom = { ch: '□', sup: true, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 };
      atoms.push(a);
      i++;
      continue;
    }

    // ---------- _ 下标 ----------
    if (c === '_') {
      const hasBase = hasIdentBase(raw, i);
      if (i + 1 < raw.length && raw.charAt(i + 1) === '(') {
        // _(...) 括号下标（第十一包④：与 ^ 同款，括号吞进首末原子；
        // 空内容 → 占位框吞整个 _()）
        const m = matchParen(raw, i + 1);
        if (m > 0) {
          if (m === i + 2) {
            if (hasBase) {
              const be: MathAtom = { ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i, end: m + 1 };
              atoms.push(be);
            } else {
              const bb: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 };
              const sb: MathAtom = { ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i + 1, end: m + 1 };
              atoms.push(bb);
              atoms.push(sb);
            }
          } else if (!hasBase) {
            const b0: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 };
            atoms.push(b0);
            pushRun(atoms, raw, i + 2, m, false, true, false, false, i + 1, m + 1);
          } else {
            pushRun(atoms, raw, i + 2, m, false, true, false, false, i, m + 1);
          }
          i = m + 1;
          continue;
        }
      }
      let j = i + 1;
      while (j < raw.length && isScriptCh(raw.charAt(j))) {
        j++;
      }
      if (j > i + 1) {
        // 有下标内容；base 缺失（"_1"）时 '_' 让位给 base 占位框
        if (!hasBase) {
          const b0: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 };
          atoms.push(b0);
          pushRun(atoms, raw, i + 1, j, false, true, false, false, i + 1, j);
        } else {
          pushRun(atoms, raw, i + 1, j, false, true, false, false, i, j);
        }
        i = j;
        continue;
      }
      // 悬空 _：有 base → 单个下标占位框（吞 '_'）；无 base → base 框 +
      // 零宽下标框（键帽"框_框"同构，第九包①）；零宽原子供光标落位
      if (hasBase) {
        const a2: MathAtom = { ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i, end: i + 1 };
        atoms.push(a2);
      } else {
        const b1: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 };
        const s1: MathAtom = { ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i + 1, end: i + 1 };
        atoms.push(b1);
        atoms.push(s1);
      }
      i++;
      continue;
    }

    // ---------- sqrt( → 根号组（第十一包⑥一体渲染） ----------
    // √ 钩形 + 顶线由渲染层画成一条连续折线；参数内容作 kids 子原子
    //（光标可落内容内），收尾零宽原子供光标停靠内容尾；')' 不再吞入
    // 末原子（原 end 吞 ')' 会使内容尾光标渲染提前一位）
    if (raw.startsWith('sqrt(', i)) {
      const m = matchParen(raw, i + 4);
      if (m > 0) {
        const kids: MathAtom[] = [];
        if (m === i + 5) {
          const k0: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i + 5, end: m + 1 };
          kids.push(k0);
        } else {
          pushRun(kids, raw, i + 5, m, false, false, false, false, i + 5, m);
        }
        const kc: MathAtom = { ch: '', sup: false, sub: false, over: false, dim: false, caret: false, start: m, end: m };
        kids.push(kc);
        const rad: MathAtom = { ch: '√', sup: false, sub: false, over: false, dim: false, caret: false, start: i, end: m + 1, kids: kids };
        atoms.push(rad);
        i = m + 1;
        continue;
      }
    }

    // ---------- NthRoot( → 带根次根号（四十七包④） ----------
    // NthRoot(根次, 被开方数)：根次作下标小字渲染进钩形缺口（idx 子原子
    // 组），被开方数同 sqrt 作 kids；函数名/左括号吞进根次首原子，右括号
    // 由根号原子吞（区间 [i, m+1)）。无顶层逗号（单参数 NthRoot(x)）不视
    // 为模板、走普通字符解析——键盘 ⁿ√□ 键插 NthRoot(,) 保证双参形态
    if (raw.startsWith('NthRoot(', i)
      && (i === 0 || !isAlnumCh(raw.charAt(i - 1)))) {
      const m = matchParen(raw, i + 7);
      if (m > 0) {
        let comma = -1;
        let depth = 0;
        for (let q = i + 8; q < m; q++) {
          const cq = raw.charAt(q);
          if (cq === '(') {
            depth++;
          } else if (cq === ')') {
            depth--;
          } else if (cq === ',' && depth === 0) {
            comma = q;
            break;
          }
        }
        if (comma > 0) {
          const idxAtoms: MathAtom[] = [];
          if (comma === i + 8) {
            const ix: MathAtom = { ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i, end: comma };
            idxAtoms.push(ix);
          } else {
            pushRun(idxAtoms, raw, i + 8, comma, false, true, false, false, i, comma);
          }
          const kids: MathAtom[] = [];
          if (m === comma + 1) {
            const k0: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: comma + 1, end: m + 1 };
            kids.push(k0);
          } else {
            pushRun(kids, raw, comma + 1, m, false, false, false, false, comma + 1, m);
          }
          const kc: MathAtom = { ch: '', sup: false, sub: false, over: false, dim: false, caret: false, start: m, end: m };
          kids.push(kc);
          const rad: MathAtom = { ch: '√', sup: false, sub: false, over: false, dim: false, caret: false, start: i, end: m + 1, kids: kids, idx: idxAtoms };
          atoms.push(rad);
          i = m + 1;
          continue;
        }
      }
    }

    // ---------- abs( → |·| ----------
    if (raw.startsWith('abs(', i)) {
      const m = matchParen(raw, i + 3);
      if (m > 0) {
        const a5: MathAtom = { ch: '|', sup: false, sub: false, over: false, dim: false, caret: false, start: i, end: i + 4 };
        atoms.push(a5);
        if (m === i + 4) {
          const a6: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i + 4, end: m + 1 };
          atoms.push(a6);
        } else {
          pushRun(atoms, raw, i + 4, m, false, false, false, false, i + 4, m);
        }
        const a7: MathAtom = { ch: '|', sup: false, sub: false, over: false, dim: false, caret: false, start: m, end: m + 1 };
        atoms.push(a7);
        i = m + 1;
        continue;
      }
    }

  // ---------- log( → 底数形式（四十七包⑤） ----------
  // log(底数, 参数)：渲染 "log" 字样 + 下标小字底数 + 参数（非空参数带
  // 括号——左括号为零宽区间装饰原子、右括号吞实字符；空参数渲染单个占位
  // 框吞 "(" ")"）。逗号作零宽隐字符原子（ch='' 不渲染，仅保区间连续）。
  // 单参数 log(x) 无顶层逗号 → 不视为底数形式、走普通字符解析（键盘
  // log□ 键插 log(,) 即底数形态）
  if (raw.startsWith('log(', i)
    && (i === 0 || !isAlphaCh(raw.charAt(i - 1)))) {
    const m = matchParen(raw, i + 3);
    if (m > 0) {
      let comma = -1;
      let depth = 0;
      for (let q = i + 4; q < m; q++) {
        const cq = raw.charAt(q);
        if (cq === '(') {
          depth++;
        } else if (cq === ')') {
          depth--;
        } else if (cq === ',' && depth === 0) {
          comma = q;
          break;
        }
      }
      if (comma > 0) {
        pushRun(atoms, raw, i, i + 3, false, false, false, false, i, i + 3);
        if (comma === i + 4) {
          const lb: MathAtom = { ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i + 3, end: comma };
          atoms.push(lb);
        } else {
          pushRun(atoms, raw, i + 4, comma, false, true, false, false, i + 3, comma);
        }
        const lc: MathAtom = { ch: '', sup: false, sub: false, over: false, dim: false, caret: false, start: comma, end: comma + 1 };
        atoms.push(lc);
        if (m === comma + 1) {
          const la: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: comma + 1, end: m + 1 };
          atoms.push(la);
        } else {
          const lp: MathAtom = { ch: '(', sup: false, sub: false, over: false, dim: false, caret: false, start: comma + 1, end: comma + 1 };
          atoms.push(lp);
          pushRun(atoms, raw, comma + 1, m, false, false, false, false, comma + 1, m);
          const lq: MathAtom = { ch: ')', sup: false, sub: false, over: false, dim: false, caret: false, start: m, end: m + 1 };
          atoms.push(lq);
        }
        i = m + 1;
        continue;
      }
    }
  }

    // ---------- 空参数 () → 灰色占位框 ----------
    if (c === '(' && i + 1 < raw.length && raw.charAt(i + 1) === ')') {
      const a8: MathAtom = { ch: '(', sup: false, sub: false, over: false, dim: false, caret: false, start: i, end: i + 1 };
      atoms.push(a8);
      const a9: MathAtom = { ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i + 1, end: i + 2 };
      atoms.push(a9);
      const a10: MathAtom = { ch: ')', sup: false, sub: false, over: false, dim: false, caret: false, start: i + 2, end: i + 3 };
      atoms.push(a10);
      i += 3;
      continue;
    }

    // ---------- 循环小数：数字.数字(数字) → 循环位上划线 ----------
    if (c === '(' && i > 0 && (isDigitCh(raw.charAt(i - 1)) || raw.charAt(i - 1) === '.')) {
      // 回溯「数字段 . 数字段」结构（至少小数点前有一位数字）
      let k1 = i;
      while (k1 > 0 && isDigitCh(raw.charAt(k1 - 1))) {
        k1--;
      }
      let dotPos = -1;
      let intStart = -1;
      if (k1 === i && i > 0 && raw.charAt(i - 1) === '.') {
        // 0.() 形态：空非循环位
        let k2 = i - 1;
        while (k2 > 0 && isDigitCh(raw.charAt(k2 - 1))) {
          k2--;
        }
        if (k2 < i - 1) {
          dotPos = i - 1;
          intStart = k2;
        }
      } else if (k1 > 0 && k1 < i && raw.charAt(k1 - 1) === '.') {
        let k2 = k1 - 1;
        while (k2 > 0 && isDigitCh(raw.charAt(k2 - 1))) {
          k2--;
        }
        if (k2 < k1 - 1) {
          dotPos = k1 - 1;
          intStart = k2;
        }
      }
      if (dotPos >= 0) {
        const m = matchParen(raw, i);
        if (m > 0) {
          if (m === i + 1) {
            const a11: MathAtom = { ch: '□', sup: false, sub: false, over: true, dim: true, caret: false, start: i, end: m + 1 };
            atoms.push(a11);
          } else {
            pushRun(atoms, raw, i + 1, m, false, false, true, false, i, m + 1);
          }
          i = m + 1;
          continue;
        }
      }
    }

    // ---------- 普通字符 ----------
    const a12: MathAtom = { ch: c, sup: false, sub: false, over: false, dim: false, caret: false, start: i, end: i + 1 };
    atoms.push(a12);
    i++;
  }
  return atoms;
}

// ---- 五十包：堆栈分式（纯显示层后处理，原文串与内核解析不变） ----
// 划界与内核 expr.cpp 语义对齐，显示不与求值矛盾：
//  · parseTerm 中 * / 与隐式乘法同优先级、严格左结合（1/2x=(1/2)·x、
//    1/2/3=(1/2)/3）→ 分子 = "/" 之前回溯到边界的整个左结合积（乘号/
//    并列/括号组/角标/根号/后缀 ! % ° 全归分子），分母 = 单个最小因子
//    （与 ^ 同级的 primary）加紧随角标与 ! % ° 后缀；
//  · ExpandMixedNumbers 先于去空白执行（"5 1/2"→"5+1/2"）→ 空格是分子
//    边界（带分数渲染为 整数 + 堆栈分式，恰同内核值）；
//  · 既有分式/未堆栈 "/" 作分子边界：前者 v1 不嵌套（编辑行 30 高不
//    够双层），后者防 "1/+2/3" 误读（内核按 1/6 求）；
//  · 任一侧为空（"1/"、"1/+2"、句首 "/"）不堆栈保持线性。
function isLetterAtomCh(c: string): boolean {
  return isAlphaCh(c) || isGreekCh(c);
}

function isDigitAtomCh(c: string): boolean {
  return isDigitCh(c) || c === '.';
}

// 原子表中向后配对 ch===')' 的原子下标（组首），无配对返回 -1
function matchParenAtomBackward(atoms: MathAtom[], close: number): number {
  let depth = 0;
  for (let q = close; q >= 0; q--) {
    const c = atoms[q].ch;
    if (c === ')') {
      depth++;
    } else if (c === '(') {
      depth--;
      if (depth === 0) {
        return q;
      }
    }
  }
  return -1;
}

// 原子表中向前配对 ch==='(' 的原子下标（组尾），无配对返回 -1
function matchParenAtomForward(atoms: MathAtom[], open: number): number {
  let depth = 0;
  for (let q = open; q < atoms.length; q++) {
    const c = atoms[q].ch;
    if (c === '(') {
      depth++;
    } else if (c === ')') {
      depth--;
      if (depth === 0) {
        return q;
      }
    }
  }
  return -1;
}

// "/" 原子两侧重组为分式原子；子表（括号组内、根号内容）递归应用。
// 幂等：已无普通 "/" 的表原样返回
export function applyFractions(atoms: MathAtom[]): MathAtom[] {
  const out: MathAtom[] = atoms.slice();
  for (let j = 0; j < out.length; j++) {
    const aj = out[j];
    if (aj.kids !== undefined && aj.kids.length > 0) {
      aj.kids = applyFractions(aj.kids);
    }
    if (aj.idx !== undefined && aj.idx.length > 0) {
      aj.idx = applyFractions(aj.idx);
    }
  }
  let i = 0;
  while (i < out.length) {
    const a = out[i];
    if (a.ch !== '/' || a.sup || a.sub || a.dim
      || (a.kids !== undefined && a.kids.length > 0)) {
      i++;
      continue;
    }
    // ---- 分子：从 i-1 回溯，ns = 分子首原子下标 ----
    let ns = i;
    for (let j = i - 1; j >= 0; j--) {
      const p = out[j];
      if (p.fracNum !== undefined || p.ch === '/' || p.ch === '(') {
        ns = j + 1;
        break;
      }
      if (p.ch === ')') {
        const open = matchParenAtomBackward(out, j);
        if (open < 0) {
          ns = j + 1;
          break;
        }
        ns = open;
        j = open; // for 的 j-- 使下一轮落在组首之前（组已整段归入）
        continue;
      }
      if (p.sup || p.sub || p.dim || p.ch.length === 0
        || isLetterAtomCh(p.ch) || isDigitCh(p.ch) || p.ch === '*'
        || p.ch === '.' || p.ch === '!' || p.ch === '%' || p.ch === '°'
        || p.ch === '$' || p.ch === '|'
        || (p.kids !== undefined && p.kids.length > 0)) {
        ns = j;
        continue;
      }
      ns = j + 1; // + - = < > , ; 空格 及其余符号 = 边界
      break;
    }
    // ---- 分母：从 i+1 取单个最小因子，de = 分母末原子下标 ----
    let de = -1;
    if (i + 1 < out.length) {
      const d0 = out[i + 1];
      let q = i + 1;
      let found = false;
      if (d0.ch === '(' && !d0.sup && !d0.sub) {
        const close = matchParenAtomForward(out, i + 1);
        if (close > 0) {
          q = close + 1;
          found = true;
        }
      } else if (isLetterAtomCh(d0.ch) && !d0.sup && !d0.sub) {
        while (q < out.length && isLetterAtomCh(out[q].ch) && !out[q].sup
          && !out[q].sub) {
          q++;
        }
        if (q < out.length && out[q].ch === '(') {
          // 字母串后跟 "(" = 函数调用，参数括号组归分母（sin(x)/2）
          const close = matchParenAtomForward(out, q);
          if (close > 0) {
            q = close + 1;
          }
        }
        found = true;
      } else if (isDigitAtomCh(d0.ch) && !d0.sup && !d0.sub) {
        while (q < out.length && isDigitAtomCh(out[q].ch) && !out[q].sup
          && !out[q].sub) {
          q++;
        }
        found = true;
      } else if ((d0.kids !== undefined && d0.kids.length > 0) || d0.dim) {
        q = i + 2; // 根号原子 / 占位框
        found = true;
      }
      if (found) {
        // 吸收紧随角标与 ! % ° 后缀（内核 ^ 与后缀绑得比 / 紧：1/2^3=1/(2^3)）
        for (;;) {
          if (q < out.length && (out[q].sup || out[q].sub)) {
            q++;
            continue;
          }
          if (q < out.length && (out[q].ch === '!' || out[q].ch === '%'
            || out[q].ch === '°')) {
            q++;
            continue;
          }
          break;
        }
        de = q - 1;
      }
    }
    // ---- 组装：替换 [ns, de] 为分式原子 ----
    if (ns < i && de > i) {
      const numAtoms = out.slice(ns, i);
      const denAtoms = out.slice(i + 1, de + 1);
      const frac: MathAtom = {
        ch: '', sup: false, sub: false, over: false, dim: false,
        caret: false,
        start: numAtoms[0].start,
        end: denAtoms[denAtoms.length - 1].end,
        fracNum: applyFractions(numAtoms),
        fracDen: applyFractions(denAtoms),
        slash: a.start
      };
      out.splice(ns, de - ns + 1, frac);
      i = ns + 1;
    } else {
      i++;
    }
  }
  return out;
}

// 原子表（含子表）中是否存在分式原子
function atomsHaveFrac(atoms: MathAtom[]): boolean {
  for (let j = 0; j < atoms.length; j++) {
    const a = atoms[j];
    if (a.fracNum !== undefined) {
      return true;
    }
    if (a.kids !== undefined && a.kids.length > 0) {
      if (atomsHaveFrac(a.kids)) {
        return true;
      }
    }
    if (a.idx !== undefined && a.idx.length > 0 && atomsHaveFrac(a.idx)) {
      return true;
    }
  }
  return false;
}

// 显示用原子表 = 线性解析 + 堆栈分式后处理：编辑/静态渲染、光标停靠与
// 点触反查统一入口。CanvasView.deriveCaretScript 维持用线性 parseMath
// Atoms——角标态判定与分式无关
function parseDisplayAtoms(raw: string): MathAtom[] {
  return applyFractions(parseMathAtoms(raw));
}

// 在原子表中按光标索引插入光标伪原子：落在某个原子区间内 → 该原子前；
// 恰在零宽下标框处 → 光标落框内（第九包①）；落在末尾 → 追加；落在根号
// 组区间内 → 下钻内容子原子（第十一包⑥）。
// caretScript（第十包②）= 光标内外双态：true=脚本区内（小号上/下移，样式
// 取相邻脚本原子），false=框外（基线正常大小）——同一原文位置的框内/框外
// 两态由此区分（上游 currentField(currentNode, offset) 的线性等价）。
// 第十一包③：紧贴占位框的独立光标移入框内渲染（caretIn）——base 框框外
// 态也入框，脚本框仅框内态入框（框外态保持基线、框前可见，上游 同款）
export function buildDisplayAtoms(raw: string, caret: number,
  caretScript: boolean = false): MathAtom[] {
  const atoms = parseDisplayAtoms(raw);
  const cp = Math.max(0, Math.min(caret, raw.length));
  let csup = false;
  let csub = false;
  if (caretScript) {
    let neighbor: MathAtom | undefined = undefined;
    for (let j = atoms.length - 1; j >= 0; j--) {
      if (atoms[j].end <= cp && (atoms[j].sup || atoms[j].sub)) {
        neighbor = atoms[j];
        break;
      }
    }
    if (neighbor === undefined) {
      // 第十九包③：前向匹配原条件 start >= cp 在「框内首站」必失配——
      // 脚本原子吞语法字符，start 是 ^/_ 的下标而非内容起点（x^2 的 '2'
      // 原子 start=1），cp=内容起点时 start(1) >= cp(2) 不成立、后向也无
      // 原子 end <= cp → 邻居找不到 → 光标按大框基线渲染 = "小框有文字
      // 时大小框间光标高度没变化"（恰在大小框交界处）。改按 end > cp 匹配
      // （首个尚未越过的脚本原子 = 内容首原子，其 sup/sub 即 run 样式）
      for (let j = 0; j < atoms.length; j++) {
        if (atoms[j].end > cp && (atoms[j].sup || atoms[j].sub)) {
          neighbor = atoms[j];
          break;
        }
      }
    }
    if (neighbor !== undefined) {
      csup = neighbor.sup;
      csub = neighbor.sub;
    }
  }
  const caretAtom: MathAtom = {
    ch: '|', sup: csup, sub: csub,
    over: false, dim: false, caret: true,
    start: cp, end: cp
  };
  let host: MathAtom | undefined = undefined;
  for (let j = 0; j < atoms.length; j++) {
    if (atoms[j].end > cp
      || (atoms[j].start === cp && atoms[j].end === atoms[j].start)) {
      host = atoms[j];
      break;
    }
  }
  if (host !== undefined) {
    if (host.fracNum !== undefined && host.fracDen !== undefined
      && cp > host.start) {
      // 分式内（五十包）：cp≤slash → 分子（cp=slash 落分子尾=键入续写
      // 分子，与原文插入语义一致）；否则分母。frac 尾（cp==end）不进：
      // host 探测 end>cp 已让位给下一原子=框外
      const inNum = host.slash === undefined || cp <= host.slash;
      const list = inNum ? host.fracNum : host.fracDen;
      if (list !== undefined) {
        let ih = -1;
        for (let k = 0; k < list.length; k++) {
          if (list[k].end > cp
            || (list[k].start === cp && list[k].end === list[k].start)) {
            ih = k;
            break;
          }
        }
        if (ih >= 0) {
          list.splice(ih, 0, caretAtom);
        } else {
          list.push(caretAtom);
        }
      }
    } else if (host.kids !== undefined && host.kids.length > 0
      && cp > host.start) {
      // 根号组内：光标先查根次组（NthRoot 根次，四十七包④），再落内容
      // 子原子（第十一包⑥）。根次组尾=空占位框时直接 caretIn 画进框
      let placed = false;
      if (host.idx !== undefined && host.idx.length > 0
        && cp <= host.idx[host.idx.length - 1].end) {
        let ihost = -1;
        for (let k = 0; k < host.idx.length; k++) {
          if (host.idx[k].end > cp
            || (host.idx[k].start === cp
              && host.idx[k].end === host.idx[k].start)) {
            ihost = k;
            break;
          }
        }
        const lastIdx = host.idx[host.idx.length - 1];
        if (ihost >= 0) {
          host.idx.splice(ihost, 0, caretAtom);
        } else if (lastIdx.dim && (lastIdx.sup || lastIdx.sub) && caretScript
          && cp === lastIdx.end && cp > lastIdx.start) {
          lastIdx.caretIn = true;
        } else {
          host.idx.push(caretAtom);
        }
        placed = true;
      }
      if (!placed) {
        let khost = -1;
        for (let k = 0; k < host.kids.length; k++) {
          if (host.kids[k].end > cp
            || (host.kids[k].start === cp
              && host.kids[k].end === host.kids[k].start)) {
            khost = k;
            break;
          }
        }
        if (khost >= 0) {
          host.kids.splice(khost, 0, caretAtom);
        } else {
          host.kids.push(caretAtom);
        }
      }
    } else {
      atoms.splice(atoms.indexOf(host), 0, caretAtom);
    }
  } else {
    atoms.push(caretAtom);
  }
  // 光标入占位框（上游 框内光标画进占位框，第十一包③）：光标前/后紧贴
  // 占位框时移入框内渲染，独立光标原子移除
  if (caret >= 0) {
    const ci = atoms.indexOf(caretAtom);
    if (ci >= 0) {
      const nextA = ci + 1 < atoms.length ? atoms[ci + 1] : undefined;
      const prevA = ci > 0 ? atoms[ci - 1] : undefined;
      if (nextA !== undefined && nextA.dim && !nextA.caret
        && (nextA.kids === undefined || nextA.kids.length === 0)) {
        const sBox = nextA.sup || nextA.sub;
        if ((cp === nextA.start && (caretScript || !sBox))
          || (caretScript && cp > nextA.start && cp <= nextA.end)) {
          nextA.caretIn = true;
          atoms.splice(ci, 1);
        }
      } else if (prevA !== undefined && prevA.dim && !prevA.caret
        && (prevA.sup || prevA.sub) && caretScript
        && cp === prevA.end && cp > prevA.start) {
        prevA.caretIn = true;
        atoms.splice(ci, 1);
      }
    }
  }
  return atoms;
}

// ---- 编辑模型（第九包②，CanvasView 的插入/退格/光标移动共用） ----

// raw[pos] 之前是否紧接一个合法标识符尾（字母/希腊开头，字母数字下划线
// 继续）——决定下标模板的 base 归属与 □_□ 智能落位
export function hasIdentBase(raw: string, pos: number): boolean {
  let j = pos;
  while (j > 0) {
    const c = raw.charAt(j - 1);
    if (isAlnumCh(c) || c === '_' || isGreekCh(c)) {
      j--;
      continue;
    }
    break;
  }
  if (j >= pos) {
    return false;
  }
  const c0 = raw.charAt(j);
  return isAlphaCh(c0) || isGreekCh(c0);
}

// ^/_ 的脚本内容区间 [cs, ce)：'(' 组返回括号内区间（无配对 → 空），
// 否则为紧随的字母数字串；无内容时 ce === cs。opPos 为 ^/_ 的下标
export function scriptContentRange(raw: string, opPos: number): number[] {
  if (opPos + 1 < raw.length && raw.charAt(opPos + 1) === '(') {
    const m = matchParen(raw, opPos + 1);
    return m > 0 ? [opPos + 2, m] : [opPos + 2, opPos + 2];
  }
  let j = opPos + 1;
  while (j < raw.length && isScriptCh(raw.charAt(j))) {
    j++;
  }
  return [opPos + 1, j];
}

// 光标紧贴函数模板 '(' 前（sqrt( / abs( / sin( …）的退格拆除区间
// [from, to)：从函数名首到配对 ')'（含），参数内容保留线性化。
// '(' 前无函数名（普通分组括号）或无配对闭括号 → 空数组（走普通退格）
export function templateUnwrapRange(raw: string, caret: number): number[] {
  if (caret <= 0 || raw.charAt(caret - 1) !== '(') {
    return [];
  }
  let j = caret - 1;
  while (j > 0 && isAlphaCh(raw.charAt(j - 1))) {
    j--;
  }
  if (j >= caret - 1) {
    return [];
  }
  const m = matchParen(raw, caret - 1);
  if (m < caret) {
    return [];
  }
  return [j, m + 1];
}

// "打字机"单字符（角标区内可直接续写的字符）：字母数字下划线 + 希腊
export function isTypewriterCh(c: string): boolean {
  return isAlnumCh(c) || c === '_' || isGreekCh(c);
}

// ---- 第十包②：上游 editor-base（CursorController / InputController）----
// ---- 语义的线性移植：光标 = 原文位置 + 框内/框外双态 ----

// 一个光标停靠点：pos=原文位置；script=true=脚本结构框内（小号、键入写
// 脚本内容），false=框外（基线、键入写外层/base）——对应 上游 光标的
// (currentField, offset)：框前=主序列 offset，框内=脚本序列 offset
export interface MathCaretStop {
  pos: number;
  script: boolean;
  empty?: boolean;   // 框内停靠且脚本内容为空（信息性标记；第十六包②起
                     // 不再据此跳站——空框 ←/→ 均可进出，跳站曾致单向门）
}

// ←/→ 的停靠序列（按视觉顺序，第十一包② v2 改按脚本 run 走）：普通原子
// 边界产框外停靠；每个 ^/_ run 先探测 op（首原子吞 op → start 即 op，
// 无 base 占位框让位 → start-1）——有合法 base（hasIdentBase）产「框外
// (base 尾)」一个停靠（③：空 base 无此停靠，← 直接进 base 框，上游
// prevField 同款），内容范围逐字符产框内停靠，结构尾产框外停靠（上游 →：
// nextCharacter 进框首、框尾 nextField 出框；← 镜像）。根号组下钻内容
// 子原子产停靠（⑥）
export function mathCaretStops(raw: string): MathCaretStop[] {
  const stops: MathCaretStop[] = [];
  const push = (pos: number, script: boolean, empty: boolean = false): void => {
    const last = stops.length > 0 ? stops[stops.length - 1] : undefined;
    if (last !== undefined && last.pos === pos && last.script === script) {
      return;
    }
    stops.push({ pos: pos, script: script, empty: empty });
  };
  // 原子表 → 停靠点（五十包重构为递归 walk：分式分子/分母、根号内容全
  // 遍历——根号内角标因此首获框内停靠；分式产 分式首/分子内逐原子/斜杠
  // 位/分母内逐原子 框外停靠，分式尾=下一原子 start 或串尾）
  const walk = (atoms: MathAtom[]): void => {
    let j = 0;
    while (j < atoms.length) {
      const a = atoms[j];
      if (a.fracNum !== undefined && a.fracDen !== undefined) {
        push(a.start, false);
        walk(a.fracNum);
        push(a.slash !== undefined ? a.slash : a.start, false);
        walk(a.fracDen);
        j++;
        continue;
      }
      if (a.sup || a.sub) {
        let op = -1;
        if (a.start < raw.length
          && (raw.charAt(a.start) === '^' || raw.charAt(a.start) === '_')) {
          op = a.start;
        } else if (a.start > 0
          && (raw.charAt(a.start - 1) === '^' || raw.charAt(a.start - 1) === '_')) {
          op = a.start - 1;
        }
        let k = j;
        while (k + 1 < atoms.length && (atoms[k + 1].sup || atoms[k + 1].sub)) {
          k++;
        }
        if (op >= 0) {
          const rng = scriptContentRange(raw, op);
          if (hasIdentBase(raw, op)) {
            push(op, false); // 框外框首（base 尾）；空 base 不产（第十一包③）
          }
          const emptyRun = rng[1] === rng[0];
          for (let p = rng[0]; p <= rng[1]; p++) {
            push(p, true, emptyRun); // 框内逐字符（空内容仅一站并标记，第十二包③）
          }
          push(scriptStructureEnd(raw, op), false); // 框外结构尾
        } else {
          // 无 ^/_ op 的脚本 run（log( 模板底数组，四十七包⑤）：内容 = 首原子
          // 吞 '(' 之后至末尾（逗号已拆为独立隐字符原子，run 末即内容尾），
          // 逐字符脚本停靠 + run 尾框外停靠
          const cs = a.start < raw.length && raw.charAt(a.start) === '('
            ? a.start + 1 : a.start;
          const ce = atoms[k].end > cs && raw.charAt(atoms[k].end - 1) === ','
            ? atoms[k].end - 1 : atoms[k].end;
          for (let p = cs; p <= ce; p++) {
            push(p, true, ce === cs);
          }
          push(atoms[k].end, false);
        }
        j = k + 1;
        continue;
      }
      push(a.start, false);
      if (a.kids !== undefined) {
        // NthRoot 根次组（四十七包④）：根次内容逐字符（脚本态停靠）——
        // 根次首原子吞 "NthRoot("，内容自 op+8 起，末原子尾即逗号位
        if (a.idx !== undefined && a.idx.length > 0) {
          const op = a.start;
          const comma = a.idx[a.idx.length - 1].end;
          if (comma >= op + 8 && comma <= raw.length
            && raw.charAt(comma) === ',') {
            for (let p = op + 8; p <= comma; p++) {
              push(p, true, comma === op + 8);
            }
          }
        }
        walk(a.kids); // 根号内容全遍历（五十包；原逐 kid start push）
      }
      j++;
    }
  };
  push(0, false);
  walk(parseDisplayAtoms(raw));
  push(raw.length, false);
  return stops;
}

// ^/_ 结构的整体终点（框首 ⌫ 移除整个结构用）：'(' 组含右括号，否则为
// 字母数字内容尾；无配对括号时止于 '(' 之后
export function scriptStructureEnd(raw: string, opPos: number): number {
  if (opPos + 1 < raw.length && raw.charAt(opPos + 1) === '(') {
    const m = matchParen(raw, opPos + 1);
    return m > 0 ? m + 1 : opPos + 2;
  }
  let j = opPos + 1;
  while (j < raw.length && isScriptCh(raw.charAt(j))) {
    j++;
  }
  return j;
}

// 光标处恰有脚本 run 终结（结构尾 == caret）→ 返回该 run 的 ^/_ 下标，
// 否则 -1（第十一包④/⑤公共探测：框外键入包裹 ^(…)、框外 ⌫ 进框/删内
// 容末字符、同型 ^/_ 重入；替代旧 scriptEndsAtCaret——旧版只查整串末
// 原子，串中段 run 终结（x^2+3 的 2 后）探测不到）
export function scriptRunEndAtCaret(raw: string, caret: number): number {
  const atoms = parseMathAtoms(raw);
  let j = 0;
  while (j < atoms.length) {
    const a = atoms[j];
    if (a.sup || a.sub) {
      let op = -1;
      if (a.start < raw.length
        && (raw.charAt(a.start) === '^' || raw.charAt(a.start) === '_')) {
        op = a.start;
      } else if (a.start > 0
        && (raw.charAt(a.start - 1) === '^' || raw.charAt(a.start - 1) === '_')) {
        op = a.start - 1;
      }
      let k = j;
      while (k + 1 < atoms.length && (atoms[k + 1].sup || atoms[k + 1].sub)) {
        k++;
      }
      if (op >= 0 && scriptStructureEnd(raw, op) === caret) {
        return op;
      }
      j = k + 1;
      continue;
    }
    j++;
  }
  return -1;
}

// 从配对 ')' 反向扫描配对 '('；无配对返回 -1
export function matchParenBackward(raw: string, close: number): number {
  let depth = 0;
  for (let j = close; j >= 0; j--) {
    const c = raw.charAt(j);
    if (c === ')') {
      depth++;
    } else if (c === '(') {
      depth--;
      if (depth === 0) {
        return j;
      }
    }
  }
  return -1;
}

// '(' 前紧接字母串（函数模板 sqrt( / abs( / sin( …）→ 返回名首下标，否则 -1
export function functionOpenBefore(raw: string, open: number): number {
  let j = open;
  while (j > 0 && isAlphaCh(raw.charAt(j - 1))) {
    j--;
  }
  return j < open ? j : -1;
}


// ---- 第十二包①②：Canvas 排版渲染 ----
// 真机连续两轮反馈根号断开（√ 字形+Overline 两段 → Path 钩形+border 顶
// 线仍脱节），且用户询问能否用 LaTeX/KaTeX——KaTeX 输出 HTML/CSS 必须跑
// 在 WebView 里，与"主功能禁用 Web 组件"冲突；改 Canvas 自绘排版：根号
// 钩形+顶线一条连续折线一次 stroke（天然一体），其余原子按基线精确绘制。
// MathField（编辑态：光标/闪烁/焦点）与 MathText（静态行）共用下面两个
// 自由函数，ForEach/key 渲染机制整体退役。
//
// 版式几何（与旧 Text 版对齐）：编辑态行高 30、基线 22、字号 15/11、
// 占位框 9/8 宽、根号高 22；静态态行高 fontSize+7、基线 fontSize+4、根
// 号高 fontSize+6。光标为 1.5px 竖线（零前进宽度，停靠位置精确）；占位
// 框为虚线描边矩形（caretIn 时框加高、光标画进框内）；循环位上划线为
// 字符上方一条直线。

interface MathPiece {
  kind: number;      // 0 字符 1 占位框 2 根号 3 光标 4 分式（五十包）
  ch: string;
  x: number;         // 绝对 x（根号内容递归定位）
  size: number;      // 字号（框/根号/分式为标称字号：分式基线几何由此推导）
  sup: boolean;
  sub: boolean;
  over: boolean;
  caretIn: boolean;
  w: number;         // 前进宽度
  kids: MathPiece[]; // 根号内容子片
  kw: number;        // 根号内容总宽
  radH: number;      // 根号整体高
  hw: number;        // 根号钩宽（四十七包④ NthRoot 带根次时加宽让位）
  fracNum?: MathPiece[]; // 分式分子片（kind 4 专属，五十包）
  fracDen?: MathPiece[]; // 分式分母片
  slash?: number;    // "/" 原文下标（点触反查下钻分子/分母判别，五十包）
  dy?: number;       // 垂直基线偏移（根次组片专属：非 undefined 时覆盖
                     //  sup/sub 基线几何，mathDraw 按它定位）
  start: number;     // 覆盖原文区间 [start, end)（四十六包点触定位反查用；
  end: number;       //  首片吞进的语法字符并入 start——与 MathAtom 同规约）
}

interface MathLayoutResult {
  pieces: MathPiece[];
  width: number;
}

// 子片树整体水平平移（五十一包）：分式子布局在原点 0 排出后加居中偏移，
// 而根号内容（kind 2 kids）与嵌套分式子表（kind 4）的子片同为"子布局绝
// 对坐标"，必须一并平移——浅移只动顶层会让分子根号内容滞留原位与钩形重
// 叠（五十包真机实证）；点触反查读片坐标处随递归全树一致
function shiftPiecesX(pieces: MathPiece[], dx: number): void {
  for (let q = 0; q < pieces.length; q++) {
    const p = pieces[q];
    p.x += dx;
    if (p.kind === 2 && p.kids.length > 0) {
      shiftPiecesX(p.kids, dx);
    } else if (p.kind === 4) {
      if (p.fracNum !== undefined) {
        shiftPiecesX(p.fracNum, dx);
      }
      if (p.fracDen !== undefined) {
        shiftPiecesX(p.fracDen, dx);
      }
    }
  }
}

// 原子表 → 绘制片（含 x 定位）。坐标系 = **vp**（第十五包③终案——四轮
// 真机证据链闭环：十二包 vp 几何+px 字号 → 几何对/字小；十三包 font×k +
// advance/k → 字对/光标落 33% 处；十四包全部 ×k → 全部画出界不可见，反
// 证坐标制是 vp。Canvas 坐标与 measureText 均为 vp 口径，唯独 font-size
// 是物理 px）。ox/baseY/boxW/boxWs/radH/hookW 均为 vp；charS/scrS 为 vp
// 标称字号（font 内部 ×k），measureText 宽度 vp 原值直用
function mathLayout(ctx: CanvasRenderingContext2D, atoms: MathAtom[],
  ox: number, baseY: number, charS: number, scrS: number,
  boxW: number, boxWs: number, radH: number, hookW: number,
  k: number): MathLayoutResult {
  const pieces: MathPiece[] = [];
  let x = ox;
  for (let j = 0; j < atoms.length; j++) {
    const a = atoms[j];
    if (a.caret) {
      const pc: MathPiece = { kind: 3, ch: '|', x: x, size: 0, sup: a.sup,
        sub: a.sub, over: false, caretIn: false, w: 0, kids: [], kw: 0, radH: 0,
        hw: 0, start: a.start, end: a.end };
      pieces.push(pc);
      continue;
    }
    if (a.fracNum !== undefined && a.fracDen !== undefined) {
      // 堆栈分式（五十包）：分子/分母各自子布局后水平居中，横线跨较宽侧
      // +2。基线几何由 size 推导、mathDraw kind 4 用同式（num −0.62·size /
      // bar −0.34 / den +0.46）；子布局根号高压 radH−6 防溢出 30 行高
      const subRadH = Math.max(10, radH - 6);
      const numLay = mathLayout(ctx, a.fracNum, 0, baseY - charS * 0.62,
        charS, scrS, boxW, boxWs, subRadH, hookW, k);
      const denLay = mathLayout(ctx, a.fracDen, 0, baseY + charS * 0.46,
        charS, scrS, boxW, boxWs, subRadH, hookW, k);
      const fw = Math.max(numLay.width, denLay.width) + 2;
      const nx = x + (fw - numLay.width) / 2;
      const dx = x + (fw - denLay.width) / 2;
      shiftPiecesX(numLay.pieces, nx);
      shiftPiecesX(denLay.pieces, dx);
      const pf: MathPiece = { kind: 4, ch: '', x: x, size: charS,
        sup: false, sub: false, over: false, caretIn: false, w: fw,
        kids: [], kw: 0, radH: 0, hw: 0,
        fracNum: numLay.pieces, fracDen: denLay.pieces, slash: a.slash,
        start: a.start, end: a.end };
      pieces.push(pf);
      x += fw + 3;
      continue;
    }
    if (a.kids !== undefined && a.kids.length > 0) {
      // 根次组（四十七包④ NthRoot）：先按脚本字号排根次，钩宽让位
      //（hookW + 根次宽 + 4），根次片右缘贴上升笔画内侧 1.5、基线取
      // 顶 + 0.62*radH（钩形缺口中部，传统 ⁿ√ 排版位），dy 盖章后并入
      // kids（mathDraw 递归即得垂直定位；点触反查下钻 kids 亦可见）
      let idxPieces: MathPiece[] = [];
      let hw = hookW;
      if (a.idx !== undefined && a.idx.length > 0) {
        const idxLay = mathLayout(ctx, a.idx, 0, 0, scrS, scrS, boxW,
          Math.max(4, boxWs - 2), radH, hookW, k);
        if (idxLay.width > 0) {
          hw = hookW + idxLay.width + 4;
          const top = baseY - radH + 3;
          const tipY = top + radH - 0.5;
          const idxBase = top + radH * 0.62;
          const upX = hw * 0.38 + (hw - 0.5 - hw * 0.38)
            * (tipY - idxBase) / (tipY - (top + 0.5));
          const shift = x + upX - 1.5 - idxLay.width;
          for (let q = 0; q < idxLay.pieces.length; q++) {
            idxLay.pieces[q].x += shift;
            idxLay.pieces[q].dy = idxBase - baseY;
          }
          idxPieces = idxLay.pieces;
        }
      }
      const inner = mathLayout(ctx, a.kids, x + hw + 2, baseY, charS,
        scrS, boxW, boxWs, radH, hookW, k);
      const pr: MathPiece = { kind: 2, ch: '√', x: x, size: 0, sup: false,
        sub: false, over: false, caretIn: false, w: hw + 4 + inner.width,
        kids: idxPieces.concat(inner.pieces), kw: inner.width, radH: radH,
        hw: hw, start: a.start, end: a.end };
      pieces.push(pr);
      x += pr.w;
      continue;
    }
    if (a.dim) {
      const w1 = (a.sup || a.sub) ? boxWs : boxW;
      const pb: MathPiece = { kind: 1, ch: '□', x: x, size: 0, sup: a.sup,
        sub: a.sub, over: a.over, caretIn: a.caretIn === true, w: w1,
        kids: [], kw: 0, radH: 0, hw: 0, start: a.start, end: a.end };
      pieces.push(pb);
      x += w1;
      continue;
    }
    if (a.ch.length === 0) {
      continue; // 零宽收尾原子（根号内容尾光标锚点），不渲染
    }
    const size = (a.sup || a.sub) ? scrS : charS;
    ctx.font = 'normal normal ' + Math.round(size * k).toString() + 'px sans-serif';
    // measureText 与坐标同口径（vp）——原值直用（第十五包③终案）
    const w2 = ctx.measureText(a.ch).width;
    const ps: MathPiece = { kind: 0, ch: a.ch, x: x, size: size, sup: a.sup,
      sub: a.sub, over: a.over, caretIn: false, w: w2, kids: [], kw: 0, radH: 0,
      hw: 0, start: a.start, end: a.end };
    pieces.push(ps);
    x += w2;
  }
  return { pieces: pieces, width: x - ox };
}

// 绘制片 → canvas（vp 坐标系，几何常量用 vp 原值；仅 font 按 k 换算物理
// px，第十五包③终案）。showCaret 控制光标显隐（闪烁由调用方按帧整绘实现）
function mathDraw(ctx: CanvasRenderingContext2D, pieces: MathPiece[],
  baseY: number, charColor: string, dimColor: string, caretColor: string,
  showCaret: boolean, hookW: number, k: number): void {
  for (let j = 0; j < pieces.length; j++) {
    const p = pieces[j];
    if (p.kind === 0) {
      const by = p.dy !== undefined ? baseY + p.dy
        : baseY + (p.sup ? -3 : p.sub ? 2 : 0);
      ctx.font = 'normal normal ' + Math.round(p.size * k).toString() + 'px sans-serif';
      ctx.fillStyle = charColor;
      ctx.textBaseline = 'alphabetic';
      ctx.fillText(p.ch, p.x, by);
      if (p.over) {
        ctx.strokeStyle = charColor;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(p.x, by - p.size * 0.8);
        ctx.lineTo(p.x + p.w, by - p.size * 0.8);
        ctx.stroke();
      }
    } else if (p.kind === 1) {
      // 占位虚线框（基线相对几何与旧 Text 版一致；dy=根次组框随其基线）
      let top = baseY - 6;
      let h = 9;
      if (p.dy !== undefined) {
        top = baseY + p.dy - 5;
        h = 8;
      } else if (p.sup) {
        top = baseY - 8;
        h = 8;
      } else if (p.sub) {
        top = baseY - 3;
        h = 8;
      }
      if (p.caretIn && !p.sup && !p.sub) {
        // 仅 base 大框保留加高（−15/18）；sup/sub 小框第十七包④起不再
        // 加高——框内光标随小框尺寸渲染，用户据此判断实际输入位置
        top = baseY - 15;
        h = 18;
      }
      ctx.strokeStyle = dimColor;
      ctx.lineWidth = 1;
      ctx.setLineDash([2, 2]);
      ctx.strokeRect(p.x + 0.5, top + 0.5, p.w - 1, h - 1);
      ctx.setLineDash([]);
      if (p.caretIn && showCaret) {
        ctx.strokeStyle = caretColor;
        ctx.lineWidth = 1.5;
        const cy = top + h / 2;
        // 框内光标条随框高收紧（第十七包④）：大框 ±5 不变，小框缩到框内
        const half = Math.min(5, h / 2 - 1);
        ctx.beginPath();
        ctx.moveTo(p.x + p.w / 2, cy - half);
        ctx.lineTo(p.x + p.w / 2, cy + half);
        ctx.stroke();
      }
      if (p.over) {
        // 空循环位段：占位框带上划线
        ctx.strokeStyle = charColor;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(p.x, top - 2);
        ctx.lineTo(p.x + p.w, top - 2);
        ctx.stroke();
      }
    } else if (p.kind === 2) {
      // 根号：钩形+顶线一条连续折线一次 stroke（第十二包①一体渲染核心；
      // 钩宽用片内 hw——NthRoot 带根次时已加宽让位，四十七包④）
      const top = baseY - p.radH + 3;
      ctx.strokeStyle = charColor;
      ctx.lineWidth = 1.5;
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      ctx.beginPath();
      ctx.moveTo(p.x + 0.5, top + p.radH * 0.55);
      ctx.lineTo(p.x + p.hw * 0.38, top + p.radH - 0.5);
      ctx.lineTo(p.x + p.hw - 0.5, top + 0.5);
      ctx.lineTo(p.x + p.hw + 2 + p.kw, top + 0.5);
      ctx.stroke();
      mathDraw(ctx, p.kids, baseY, charColor, dimColor, caretColor,
        showCaret, hookW, k);
    } else if (p.kind === 4) {
      // 分式（五十包）：横线一次 stroke + 分子/分母两子表按各自基线递归
      //（基线几何与 mathLayout kind 4 同式推导，size=标称字号）
      const nb = baseY - p.size * 0.62;
      const db = baseY + p.size * 0.46;
      const bb = baseY - p.size * 0.34;
      ctx.strokeStyle = charColor;
      ctx.lineWidth = 1.2;
      ctx.beginPath();
      ctx.moveTo(p.x + 0.5, bb);
      ctx.lineTo(p.x + p.w - 0.5, bb);
      ctx.stroke();
      mathDraw(ctx, p.fracNum !== undefined ? p.fracNum : [], nb,
        charColor, dimColor, caretColor, showCaret, hookW, k);
      mathDraw(ctx, p.fracDen !== undefined ? p.fracDen : [], db,
        charColor, dimColor, caretColor, showCaret, hookW, k);
    } else if (p.kind === 3 && showCaret) {
      let y0 = baseY - 11;
      let y1 = baseY + 2;
      if (p.dy !== undefined) {
        y0 = baseY + p.dy - 6;
        y1 = baseY + p.dy + 1;
      } else if (p.sup) {
        y0 = baseY - 8;
        y1 = baseY - 1;
      } else if (p.sub) {
        y0 = baseY - 3;
        y1 = baseY + 5;
      }
      ctx.strokeStyle = caretColor;
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(p.x + 0.75, y0);
      ctx.lineTo(p.x + 0.75, y1);
      ctx.stroke();
    }
  }
}

// ---- 四十六包：点触定位光标（排版反查）；四十七包①②修正 ----
// 框内站（script=true）→ 脚本片的覆盖区间 [x0, x1]（四十七包①：点距改
// 区间距——片内点按距离 0，稳压同 x 并列的框外站，不再单靠 y 仲裁）；
// run 尾站取末片右缘（空 run 取末片整段——空框 caretIn 光标画在框心）；
// pos 落根号组区间内时下钻 kids（NthRoot 根次片在其中，四十七包④）。
// 框外站查首个 end>pos 的片左缘（被吞语法字符区间并入首片，故 op/内容
// 起点共用同一视觉位）；根号内容尾/空参数框取内容区右缘 = 末个有宽子片
// 右缘（四十七包②：旧版误用根号整体右缘 p.x+p.w，内容尾与「根号外」
// 停靠点同 x、点按内外颠倒）。两端超界自然由「最近距离」钳到首/末停靠点
function scriptStopSpan(pieces: MathPiece[], pos: number,
  empty: boolean): number[] {
  let lastL = -1;
  let lastR = -1;
  let s0 = -1;
  let s1 = -1;
  for (let j = 0; j < pieces.length; j++) {
    const p = pieces[j];
    if ((p.kind === 2 || p.kind === 4) && pos > p.start && pos <= p.end) {
      const inner = scriptStopSpan(p.kids, pos, empty);
      if (inner[0] >= 0) {
        return inner;
      }
      if (p.kind === 4) {
        // 分式内角标（分子/分母内的 ^/_ 片，五十包）：按 slash 判别子表
        const inNum = p.slash === undefined || pos <= p.slash;
        const sub = inNum ? p.fracNum : p.fracDen;
        if (sub !== undefined) {
          const inner2 = scriptStopSpan(sub, pos, empty);
          if (inner2[0] >= 0) {
            return inner2;
          }
        }
      }
      continue;
    }
    if (!(p.sup || p.sub) || p.start > pos || pos > p.end) {
      continue;
    }
    lastL = p.x;
    lastR = p.x + p.w;
    if (p.end > pos && s0 < 0) {
      s0 = p.x;
      s1 = p.x + p.w;
    }
  }
  if (s0 >= 0) {
    return [s0, s1];
  }
  if (lastR >= 0) {
    return empty ? [lastL, lastR] : [lastR, lastR];
  }
  return [-1, -1];
}

function baseStopXRec(pieces: MathPiece[], pos: number): number {
  for (let j = 0; j < pieces.length; j++) {
    const p = pieces[j];
    // 脚本片不跳过：sup/sub 在基线流位上排版（上标紧跟 base 左缘），
    // base 尾停靠（op 位）恰是其 start，视觉 x 即该片左缘
    if (p.end <= pos) {
      continue;
    }
    if (p.kind === 4 && pos > p.start) {
      // 分式内框外停靠（五十包）：按 slash 判别下钻分子/分母；cp=slash
      // 落分子尾（末个有宽片右缘），与 buildDisplayAtoms 下钻规则一致
      const inNum = p.slash === undefined || pos <= p.slash;
      const sub = inNum ? p.fracNum : p.fracDen;
      if (sub !== undefined) {
        const inner = baseStopXRec(sub, pos);
        if (inner >= 0) {
          return inner;
        }
        let fr = -1;
        for (let t = 0; t < sub.length; t++) {
          if (sub[t].w > 0) {
            fr = sub[t].x + sub[t].w;
          }
        }
        if (fr >= 0) {
          return fr;
        }
      }
      return p.x;
    }
    if (p.kind === 2 && pos > p.start) {
      const inner = baseStopXRec(p.kids, pos);
      if (inner >= 0) {
        return inner;
      }
      // 内容尾（含空参数框）：内容区右缘 = 末个有宽子片右缘（根次片排
      // 在最前不会被误取），不是根号整体右缘（四十七包②）
      let cr = -1;
      for (let t = 0; t < p.kids.length; t++) {
        if (p.kids[t].w > 0) {
          cr = p.kids[t].x + p.kids[t].w;
        }
      }
      return cr >= 0 ? cr : p.x + p.w;
    }
    return p.x;
  }
  return -1;
}

// 脚本停靠点是上标还是下标（光标 y 偏好仲裁用）：找覆盖 pos 的脚本原子
//（start<pos≤end，首片吞语法字符故 start 是 ^/_ 的下标）。五十包起递归
// 下钻分式分子/分母与根号内容（walk 产的框内停靠在这些子表里）
function stopIsSup(raw: string, pos: number): boolean {
  return stopIsSupIn(parseDisplayAtoms(raw), pos);
}

function stopIsSupIn(atoms: MathAtom[], pos: number): boolean {
  for (let j = 0; j < atoms.length; j++) {
    const a = atoms[j];
    if (a.fracNum !== undefined && a.fracDen !== undefined) {
      const inNum = a.slash === undefined || pos <= a.slash;
      const sub = inNum ? a.fracNum : a.fracDen;
      if (sub !== undefined && stopIsSupIn(sub, pos)) {
        return true;
      }
      continue;
    }
    if ((a.sup || a.sub) && pos > a.start && pos <= a.end) {
      return a.sup;
    }
    if (a.kids !== undefined && a.kids.length > 0 && pos > a.start
      && stopIsSupIn(a.kids, pos)) {
      return true;
    }
  }
  return false;
}

// 点触定位：layoutX/layoutY 为排版坐标（x 相对内容起点、y 相对 Canvas 顶，
// 行高 30、基线 22——MathField.paint 同参排版）。返回最近停靠点；并列时
// 按 y 仲裁：上区偏爱上标框内站、下区偏爱下标框内站、中区偏爱框外站。
export function mathPickCaretStop(ctx: CanvasRenderingContext2D, raw: string,
  layoutX: number, layoutY: number, k: number): MathCaretStop | null {
  const stops = mathCaretStops(raw);
  if (stops.length === 0) {
    return null;
  }
  const lay = mathLayout(ctx, parseDisplayAtoms(raw), 0, 22, 15, 11, 9, 8,
    22, 9, k);
  const ds: number[] = [];
  for (let i = 0; i < stops.length; i++) {
    if (stops[i].script) {
      const sp = scriptStopSpan(lay.pieces, stops[i].pos,
        stops[i].empty === true);
      const x0 = sp[0] >= 0 ? sp[0] : lay.width;
      const x1 = sp[1] >= 0 ? sp[1] : x0;
      ds.push(layoutX < x0 ? x0 - layoutX : layoutX > x1 ? layoutX - x1 : 0);
    } else {
      const bx = baseStopXRec(lay.pieces, stops[i].pos);
      const sx = bx >= 0 ? bx : lay.width;
      ds.push(Math.abs(sx - layoutX));
    }
  }
  let best = -1;
  for (let i = 0; i < stops.length; i++) {
    if (best < 0 || ds[i] < ds[best]) {
      best = i;
    }
  }
  const ties: number[] = [];
  for (let i = 0; i < stops.length; i++) {
    if (Math.abs(ds[i] - ds[best]) < 0.5) {
      ties.push(i);
    }
  }
  let pick = best;
  if (ties.length > 1) {
    // y 仲裁（四十七包①阈值校准：上标字形/框 canvas y≈[11,22]、下标
    // ≈[16,27]——旧值 15/24 与下标框错位，点下标框被判「中区」偏爱框
    // 外站，光标进不去下标）
    const wantSup = layoutY < 16;
    const wantSub = layoutY > 19;
    for (let t = 0; t < ties.length && pick === best; t++) {
      const st = stops[ties[t]];
      if (!st.script && !wantSup && !wantSub) {
        pick = ties[t];
      } else if (st.script && (wantSup || wantSub)) {
        const sup = stopIsSup(raw, st.pos);
        if ((wantSup && sup) || (wantSub && !sup)) {
          pick = ties[t];
        }
      }
    }
  }
  return stops[pick];
}

// MathText — 静态公式渲染（Canvas 版，第十二包①）：与 MathField 同一
// painter，无光标；用于代数区行内展示对象的值/定义。宽度按实测内容收缩，
// 超宽由 constraintSize 裁剪（行内不做滚动）。
@Component
export struct MathText {
  @Prop @Watch('repaint') raw: string = '';
  @Prop @Watch('repaint') dark: boolean = false;
  @Prop @Watch('repaint') fontSize: number = 13;
  @Prop @Watch('repaint') color: string = '#333333';
  @State private contentW: number = 40;
  @State private contentH: number = 20;
  private settings: RenderingContextSettings = new RenderingContextSettings(true);
  private ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.settings);
  private ready: boolean = false;
  private den: number = canvasDensity();

  repaint(): void {
    if (this.ready) {
      this.paint();
    }
  }

  private paint(): void {
    const k = this.den;
    const fs = this.fontSize;
    const atoms = parseDisplayAtoms(this.raw);
    // 分式行动态加高（五十包）：基线 fs+6、行高 fs+15 容纳分母；无分式
    // 维持 fs+4 / fs+7 原几何
    const hasFrac = atomsHaveFrac(atoms);
    const baseY = fs + (hasFrac ? 6 : 4);
    const lineH = fs + (hasFrac ? 15 : 7);
    const hookW = Math.max(7, Math.round(fs * 0.55));
    this.ctx.clearRect(0, 0, this.contentW + 100, lineH + 40);
    const lay = mathLayout(this.ctx, atoms, 0, baseY,
      fs, fs - 2, 8, 7, fs + 6, hookW, k);
    const total = Math.max(12, Math.ceil(lay.width));
    if (Math.abs(total - this.contentW) > 0.5) {
      this.contentW = total;
    }
    if (Math.abs(lineH - this.contentH) > 0.5) {
      this.contentH = lineH;
    }
    mathDraw(this.ctx, lay.pieces, baseY, this.color, Theme.faint(this.dark),
      Theme.accent(this.dark), false, hookW, k);
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

// MathField — 公式化输入显示（Canvas 版，第十二包①）：显示=行为，光标
// 几何由 buildDisplayAtoms（caretIn/caretScript）+ mathLayout 精确给出；
// 闪烁定时器驱动整帧重绘。
@Component
export struct MathField {
  @Prop @Watch('onDispChange') raw: string = '';
  @Prop @Watch('onDispChange') caret: number = 0;
  @Prop @Watch('onDispChange') caretScript: boolean = false;
  @Prop @Watch('onDispChange') focused: boolean = false;
  @Prop @Watch('onDispChange') dark: boolean = false;
  @Prop placeholder: string = '输入表达式…';
  onActivate: () => void = () => {};
  // 四十六包：点触定位光标——点按坐标反查最近停靠点上报宿主
  //（宿主设 caret/caretScript 并同步底层 TextInput 选区）
  onTapCaret: (pos: number, script: boolean) => void =
    (_p: number, _s: boolean) => {};
  @State private blink: boolean = true;
  @State private contentW: number = 320;
  @State private fieldH: number = 32;
  private settings: RenderingContextSettings = new RenderingContextSettings(true);
  private ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.settings);
  private ready: boolean = false;
  private blinkTimer: number = -1;
  private den: number = canvasDensity();
  private fieldScroller: Scroller = new Scroller();

  aboutToAppear(): void {
    this.restartBlink();
  }

  aboutToDisappear(): void {
    if (this.blinkTimer >= 0) {
      clearInterval(this.blinkTimer);
      this.blinkTimer = -1;
    }
  }

  // 键入/移动/聚焦后光标先常亮、再重新起闪烁相位（上游 同款：光标操作后
  // 必见）。第十六包②「光标经常不可见」根因：blink 定时器只翻 @State 而
  // 无人重绘画布——光标只在属性变化触发的重绘里出现，一半概率撞上
  // blink=false 被画没，直到下一次重绘。现在翻转相后主动重绘
  private restartBlink(): void {
    if (this.blinkTimer >= 0) {
      clearInterval(this.blinkTimer);
    }
    this.blink = true;
    if (this.ready) {
      this.paint();
    }
    this.blinkTimer = setInterval(() => {
      this.blink = !this.blink;
      if (this.ready) {
        this.paint();
      }
    }, 530);
  }

  onDispChange(): void {
    this.restartBlink();
  }

  repaint(): void {
    if (this.ready) {
      this.paint();
    }
  }

  private paint(): void {
    const k = this.den;
    this.ctx.clearRect(0, 0, this.contentW + 100, 40);
    const disp = buildDisplayAtoms(this.raw, this.focused ? this.caret : -1,
      this.caretScript);
    const lay = mathLayout(this.ctx, disp, 8, 22, 15, 11, 9, 8, 22, 9, k);
    const total = Math.max(60, Math.ceil(lay.width + 8) + 8);
    if (Math.abs(total - this.contentW) > 0.5) {
      this.contentW = total;
    }
    mathDraw(this.ctx, lay.pieces, 22, Theme.text(this.dark),
      Theme.faint(this.dark), Theme.accent(this.dark),
      this.focused && this.blink, 9, k);
  }

  build() {
    Stack({ alignContent: Alignment.Start }) {
      if (this.raw.length === 0 && !this.focused) {
        Text(this.placeholder)
          .fontSize(13)
          .fontColor(Theme.faint(this.dark))
          .maxLines(1)
          .margin({ left: 8 })
      }
      Scroll(this.fieldScroller) {
        Canvas(this.ctx)
          .width(this.contentW)
          .height(30)
          .onReady(() => {
            this.ready = true;
            this.paint();
          })
          .onAreaChange(() => this.repaint())
      }
      .scrollable(ScrollDirection.Horizontal)
      .scrollBar(BarState.Off)
      .edgeEffect(EdgeEffect.Spring)
      .width('100%')
      .height('100%')
      .align(Alignment.Start)
    }
    .width('100%')
    .height('100%')
    .onAreaChange((_o: Area, n: Area) => {
      const h = Number(n.height);
      if (h > 0 && Math.abs(h - this.fieldH) > 0.5) {
        this.fieldH = h;
      }
    })
    .onClick((evt: ClickEvent) => {
      // 四十六包 点触定位：点按 x + 滚动偏移 = 内容 x（再减排版左衬 8）；
      // y 换算到 Canvas 内（Canvas 行高 30 在本组件内垂直居中），供
      // 大小框并列停靠点的上下偏好仲裁。先保底激活（未聚焦首点=聚焦），
      // 再上报反查结果
      const off = this.fieldScroller.currentOffset();
      const layoutX = evt.x + off.xOffset - 8;
      const canvasY = evt.y - (this.fieldH - 30) / 2;
      this.onActivate();
      const stop = mathPickCaretStop(this.ctx, this.raw, layoutX, canvasY,
        this.den);
      if (stop !== null) {
        this.onTapCaret(stop.pos, stop.script);
      }
    })
  }
}
