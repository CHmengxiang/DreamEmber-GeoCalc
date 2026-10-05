// GeoTypes.ets — 代数区行数据模型与行协议解析（自 CanvasView 拆出，轮 A）。
// 行协议 v2（内核 algebraText 输出，\n 分行）：
//   label|type|value|definition|vis|sel[|k=v;k=v]
// 前六列固定语义；第 7 列起 key=value 对（分号分隔，位置无关）：
//   slider → anim=0/1;min=..;max=..;step=..，不等式区域/多边形/角 → fill=百分比
// 解析器保留旧位置列兜底（anim|min|max|step / fill 百分比），旧格式行仍可读。

// @Observed：行数据就地变更（值/显隐/选中/动画态）直接刷新行组件（@ObjectLink），
// 行组件按 label 稳定复用（拖动条/拖点时不重建行）
@Observed
export class AlgebraEntry {
  label: string;
  type: string;
  value: string;
  definition: string;
  vis: string;      // '0' = 隐藏（实/空心按钮）
  sel: string;      // '1' = 画布选中（行高亮）
  anim: string;     // slider 行：'1'=动画播放中
  min: number;      // slider 下限
  max: number;      // slider 上限
  step: number;     // slider 步长
  fillAlpha: number; // 区域填充不透明度 0-1（-1 = 无填充，不显示滑杆；UI 显示透明度 100×(1−fillAlpha)）

  constructor(label: string, type: string, value: string, definition: string) {
    this.label = label;
    this.type = type;
    this.value = value;
    this.definition = definition;
    this.vis = '1';
    this.sel = '0';
    this.anim = '0';
    this.min = 0;
    this.max = 5;
    this.step = 0;
    this.fillAlpha = -1;
  }
}

// key=value 对写回 entry；未知键忽略（协议向前兼容：内核新增属性不破坏旧 UI）
function applyProp(e: AlgebraEntry, k: string, v: string): void {
  if (k === 'fill') {
    const a = parseFloat(v);
    if (!isNaN(a)) {
      e.fillAlpha = a / 100;
    }
  } else if (k === 'anim') {
    e.anim = v;
  } else if (k === 'min') {
    const n = parseFloat(v);
    if (!isNaN(n)) {
      e.min = n;
    }
  } else if (k === 'max') {
    const n = parseFloat(v);
    if (!isNaN(n)) {
      e.max = n;
    }
  } else if (k === 'step') {
    const n = parseFloat(v);
    if (!isNaN(n)) {
      e.step = n;
    }
  }
}

// 解析代数区行协议 → AlgebraEntry[]
export function parseAlgebra(raw: string): AlgebraEntry[] {
  const list: AlgebraEntry[] = [];
  if (raw.length === 0) {
    return list;
  }
  for (const row of raw.split('\n')) {
    if (row.length === 0) {
      continue;
    }
    const parts = row.split('|');
    if (parts.length < 4) {
      continue;
    }
    const e: AlgebraEntry = new AlgebraEntry(parts[0], parts[1], parts[2], parts[3]);
    if (parts.length >= 5) {
      e.vis = parts[4];    // 可见性（实/空心按钮）
    }
    if (parts.length >= 6) {
      e.sel = parts[5];    // 画布选中（行高亮）
    }
    if (parts.length >= 7) {
      const tail = parts[6];
      if (tail.indexOf('=') >= 0) {
        // 协议 v2：key=value;key=value
        for (const kv of tail.split(';')) {
          const eq = kv.indexOf('=');
          if (eq > 0) {
            applyProp(e, kv.substring(0, eq), kv.substring(eq + 1));
          }
        }
      } else if (e.type === 'slider' && parts.length >= 10) {
        // 旧协议兜底：第 7-10 列 = anim|min|max|step
        e.anim = parts[6];
        e.min = parseFloat(parts[7]);
        e.max = parseFloat(parts[8]);
        e.step = parseFloat(parts[9]);
      } else if (e.type === 'function' || e.type === 'polygon' || e.type === 'angle') {
        // 旧协议兜底：第 7 列 = 填充不透明度百分比
        const a = parseFloat(parts[6]);
        if (!isNaN(a)) {
          e.fillAlpha = a / 100;
        }
      }
    }
    list.push(e);
  }
  return list;
}
