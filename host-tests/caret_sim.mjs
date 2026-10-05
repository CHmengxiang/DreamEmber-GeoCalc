// 十九包③诊断：机械转译 MathField.ets 的 parseMathAtoms / mathCaretStops /
// buildDisplayAtoms + CanvasView caretMove，实测「大小框之间移动光标」时
// 每一站的渲染几何（kind-3 高度 / caretIn 条），验证用户报告的光标高度不变
'use strict';

// ---- MathField.ets 转译 ----
const DIGITS = '0123456789';
const isDigitCh = (c) => c.length === 1 && DIGITS.indexOf(c) >= 0;
const isAlnumCh = (c) => {
  if (c.length !== 1) return false;
  const u = c.charCodeAt(0);
  return (u >= 48 && u <= 57) || (u >= 65 && u <= 90) || (u >= 97 && u <= 122);
};
const isAlphaCh = (c) => {
  if (c.length !== 1) return false;
  const u = c.charCodeAt(0);
  return (u >= 65 && u <= 90) || (u >= 97 && u <= 122);
};
const isGreekCh = (c) => {
  if (c.length !== 1) return false;
  const u = c.charCodeAt(0);
  return u >= 0x0370 && u <= 0x03FF;
};

function matchParen(raw, open) {
  let depth = 0;
  for (let j = open; j < raw.length; j++) {
    const c = raw.charAt(j);
    if (c === '(') depth++;
    else if (c === ')') { depth--; if (depth === 0) return j; }
  }
  return -1;
}

function hasIdentBase(raw, pos) {
  let j = pos;
  while (j > 0) {
    const c = raw.charAt(j - 1);
    if (isAlnumCh(c) || c === '_' || isGreekCh(c)) { j--; continue; }
    break;
  }
  if (j >= pos) return false;
  const c0 = raw.charAt(j);
  return isAlphaCh(c0) || isGreekCh(c0);
}

function scriptContentRange(raw, opPos) {
  if (opPos + 1 < raw.length && raw.charAt(opPos + 1) === '(') {
    const m = matchParen(raw, opPos + 1);
    return m > 0 ? [opPos + 2, m] : [opPos + 2, opPos + 2];
  }
  let j = opPos + 1;
  while (j < raw.length && isAlnumCh(raw.charAt(j))) j++;
  return [opPos + 1, j];
}

function scriptStructureEnd(raw, opPos) {
  if (opPos + 1 < raw.length && raw.charAt(opPos + 1) === '(') {
    const m = matchParen(raw, opPos + 1);
    return m > 0 ? m + 1 : opPos + 2;
  }
  let j = opPos + 1;
  while (j < raw.length && isAlnumCh(raw.charAt(j))) j++;
  return j;
}

function pushRun(atoms, raw, from, to, sup, sub, over, dim, preStart, postEnd) {
  for (let j = from; j < to; j++) {
    atoms.push({
      ch: raw.charAt(j), sup, sub, over, dim, caret: false,
      start: (j === from ? Math.min(preStart, j) : j), end: j + 1
    });
  }
  if (to > from && atoms.length > 0) {
    const last = atoms[atoms.length - 1];
    last.end = Math.max(last.end, postEnd);
  }
}

function parseMathAtoms(raw) {
  const atoms = [];
  let i = 0;
  while (i < raw.length) {
    const c = raw.charAt(i);
    if (c === '^') {
      if (i + 1 < raw.length && raw.charAt(i + 1) === '(') {
        const m = matchParen(raw, i + 1);
        if (m > 0) {
          if (m === i + 2) {
            atoms.push({ ch: '□', sup: true, sub: false, over: false, dim: true, caret: false, start: i, end: m + 1 });
          } else {
            pushRun(atoms, raw, i + 2, m, true, false, false, false, i, m + 1);
          }
          i = m + 1; continue;
        }
      }
      let j = i + 1;
      while (j < raw.length && isAlnumCh(raw.charAt(j))) j++;
      if (j > i + 1) { pushRun(atoms, raw, i + 1, j, true, false, false, false, i, j); i = j; continue; }
      atoms.push({ ch: '□', sup: true, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 });
      i++; continue;
    }
    if (c === '_') {
      const hasBase = hasIdentBase(raw, i);
      if (i + 1 < raw.length && raw.charAt(i + 1) === '(') {
        const m = matchParen(raw, i + 1);
        if (m > 0) {
          if (m === i + 2) {
            if (hasBase) {
              atoms.push({ ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i, end: m + 1 });
            } else {
              atoms.push({ ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 });
              atoms.push({ ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i + 1, end: m + 1 });
            }
          } else if (!hasBase) {
            atoms.push({ ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 });
            pushRun(atoms, raw, i + 2, m, false, true, false, false, i + 1, m + 1);
          } else {
            pushRun(atoms, raw, i + 2, m, false, true, false, false, i, m + 1);
          }
          i = m + 1; continue;
        }
      }
      let j = i + 1;
      while (j < raw.length && isAlnumCh(raw.charAt(j))) j++;
      if (j > i + 1) {
        if (!hasBase) {
          atoms.push({ ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 });
          pushRun(atoms, raw, i + 1, j, false, true, false, false, i + 1, j);
        } else {
          pushRun(atoms, raw, i + 1, j, false, true, false, false, i, j);
        }
        i = j; continue;
      }
      if (hasBase) {
        atoms.push({ ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i, end: i + 1 });
      } else {
        atoms.push({ ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i, end: i + 1 });
        atoms.push({ ch: '□', sup: false, sub: true, over: false, dim: true, caret: false, start: i + 1, end: i + 1 });
      }
      i++; continue;
    }
    if (raw.startsWith('sqrt(', i)) {
      const m = matchParen(raw, i + 4);
      if (m > 0) {
        const kids = [];
        if (m === i + 5) {
          kids.push({ ch: '□', sup: false, sub: false, over: false, dim: true, caret: false, start: i + 5, end: m + 1 });
        } else {
          pushRun(kids, raw, i + 5, m, false, false, false, false, i + 5, m);
        }
        kids.push({ ch: '', sup: false, sub: false, over: false, dim: false, caret: false, start: m, end: m });
        atoms.push({ ch: '√', sup: false, sub: false, over: false, dim: false, caret: false, start: i, end: m + 1, kids });
        i = m + 1; continue;
      }
    }
    atoms.push({ ch: c, sup: false, sub: false, over: false, dim: false, caret: false, start: i, end: i + 1 });
    i++;
  }
  return atoms;
}

function mathCaretStops(raw) {
  const stops = [];
  const push = (pos, script, empty = false) => {
    const last = stops.length > 0 ? stops[stops.length - 1] : undefined;
    if (last !== undefined && last.pos === pos && last.script === script) return;
    stops.push({ pos, script, empty });
  };
  push(0, false);
  const atoms = parseMathAtoms(raw);
  let j = 0;
  while (j < atoms.length) {
    const a = atoms[j];
    if (a.sup || a.sub) {
      let op = -1;
      if (a.start < raw.length && (raw.charAt(a.start) === '^' || raw.charAt(a.start) === '_')) op = a.start;
      else if (a.start > 0 && (raw.charAt(a.start - 1) === '^' || raw.charAt(a.start - 1) === '_')) op = a.start - 1;
      let k = j;
      while (k + 1 < atoms.length && (atoms[k + 1].sup || atoms[k + 1].sub)) k++;
      if (op >= 0) {
        const rng = scriptContentRange(raw, op);
        if (hasIdentBase(raw, op)) push(op, false);
        const emptyRun = rng[1] === rng[0];
        for (let p = rng[0]; p <= rng[1]; p++) push(p, true, emptyRun);
        push(scriptStructureEnd(raw, op), false);
      } else {
        push(a.start, true); push(a.end, true); push(a.end, false);
      }
      j = k + 1; continue;
    }
    push(a.start, false);
    if (a.kids !== undefined) {
      for (let k2 = 0; k2 < a.kids.length; k2++) push(a.kids[k2].start, false);
    }
    j++;
  }
  push(raw.length, false);
  return stops;
}

function buildDisplayAtoms(raw, caret, caretScript) {
  const atoms = parseMathAtoms(raw);
  const cp = Math.max(0, Math.min(caret, raw.length));
  let csup = false, csub = false;
  if (caretScript) {
    let neighbor;
    for (let j = atoms.length - 1; j >= 0; j--) {
      if (atoms[j].end <= cp && (atoms[j].sup || atoms[j].sub)) { neighbor = atoms[j]; break; }
    }
    if (neighbor === undefined) {
      // 第十九包③修复后口径：前向按 end > cp 匹配（首个未越过的脚本原子）
      for (let j = 0; j < atoms.length; j++) {
        if (atoms[j].end > cp && (atoms[j].sup || atoms[j].sub)) { neighbor = atoms[j]; break; }
      }
    }
    if (neighbor !== undefined) { csup = neighbor.sup; csub = neighbor.sub; }
  }
  const caretAtom = { ch: '|', sup: csup, sub: csub, over: false, dim: false, caret: true, start: cp, end: cp };
  let host;
  for (let j = 0; j < atoms.length; j++) {
    if (atoms[j].end > cp || (atoms[j].start === cp && atoms[j].end === atoms[j].start)) { host = atoms[j]; break; }
  }
  if (host !== undefined) {
    atoms.splice(atoms.indexOf(host), 0, caretAtom);
  } else {
    atoms.push(caretAtom);
  }
  let caretIn = false;
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
          nextA.caretIn = true; atoms.splice(ci, 1); caretIn = true;
        }
      } else if (prevA !== undefined && prevA.dim && !prevA.caret
        && (prevA.sup || prevA.sub) && caretScript
        && cp === prevA.end && cp > prevA.start) {
        prevA.caretIn = true; atoms.splice(ci, 1); caretIn = true;
      }
    }
  }
  return { atoms, csup, csub, caretIn, cp };
}

// ---- 渲染几何（mathDraw kind-3 / kind-1 caretIn，baseY=22）----
function caretGeometry(d) {
  const baseY = 22;
  if (d.caretIn) {
    // 由 buildDisplayAtoms 返回的 atoms 找 caretIn 占位框
    const box = d.atoms.find((a) => a.caretIn === true);
    if (box !== undefined) {
      let top = baseY - 6, h = 9;
      if (box.sup) { top = baseY - 8; h = 8; }
      else if (box.sub) { top = baseY - 3; h = 8; }
      if (box.caretIn && !box.sup && !box.sub) { top = baseY - 15; h = 18; }
      const half = Math.min(5, h / 2 - 1);
      return { kind: 'in-box-bar', y0: top + h / 2 - half, y1: top + h / 2 + half, hgt: half * 2 };
    }
  }
  let y0 = baseY - 11, y1 = baseY + 2;
  if (d.csup) { y0 = baseY - 8; y1 = baseY - 1; }
  else if (d.csub) { y0 = baseY - 3; y1 = baseY + 5; }
  return { kind: d.csup ? 'standalone-sup' : d.csub ? 'standalone-sub' : 'standalone-base', y0, y1, hgt: y1 - y0 };
}

// ---- CanvasView caretMove 转译 ----
function caretMove(raw, pos, script, dir) {
  const stops = mathCaretStops(raw);
  let idx = -1;
  for (let i = 0; i < stops.length; i++) {
    if (stops[i].pos === pos && stops[i].script === script) { idx = i; break; }
  }
  let stop;
  if (idx >= 0) {
    const ni = idx + dir;
    if (ni >= 0 && ni < stops.length) stop = stops[ni];
  } else if (dir > 0) {
    for (let i = 0; i < stops.length; i++) {
      if (stops[i].pos > pos) { stop = stops[i]; break; }
    }
  } else {
    for (let i = stops.length - 1; i >= 0; i--) {
      if (stops[i].pos < pos) { stop = stops[i]; break; }
    }
  }
  return stop;
}

// ---- 场景模拟 ----
function showStops(raw) {
  const stops = mathCaretStops(raw);
  console.log(`  stops(${JSON.stringify(raw)}): ` + stops.map((s) => `${s.pos}/${s.script ? 'in' : 'out'}${s.empty ? '(empty)' : ''}`).join(' '));
}

function walk(raw, startIdx) {
  // startIdx: 起始站序号；来回移动打印每站渲染几何
  const stops = mathCaretStops(raw);
  let pos = stops[startIdx].pos, script = stops[startIdx].script;
  const seq = [1, -1, 1, 1, -1, -1, 1];
  const names = ['→', '←', '→', '→', '←', '←', '→'];
  console.log(`  walk(${JSON.stringify(raw)}):`);
  for (let step = 0; step < seq.length; step++) {
    const stop = caretMove(raw, pos, script, seq[step]);
    if (stop === undefined) { console.log(`    ${names[step]}: (端点不动)`); continue; }
    pos = stop.pos; script = stop.script;
    const d = buildDisplayAtoms(raw, pos, script);
    const g = caretGeometry(d);
    console.log(`    ${names[step]} → pos=${pos} ${script ? '框内' : '框外'} → ${g.kind} y ${g.y0}..${g.y1} 高${g.hgt}`);
  }
}

console.log('=== 场景 A：x^2（键入 2 后，光标在框内）===');
showStops('x^2');
walk('x^2', 3);

console.log('=== 场景 B：x^32（2 前键入 3 后）===');
showStops('x^32');
walk('x^32', 3);

console.log('=== 场景 C：x^(23)（括号包裹形态）===');
showStops('x^(23)');
walk('x^(23)', 2);

console.log('=== 场景 D：x_23（下标有内容）===');
showStops('x_23');
walk('x_23', 3);
