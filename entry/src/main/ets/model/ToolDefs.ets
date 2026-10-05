// ToolDefs.ets — 工具数据化（轮 A 防乱重构）：按钮清单、分组（轮 C）与用法
// 提示独立成数据，增删工具/改提示只动这里，工具面板与画布组件代码不变。
// id 是内核 kernelTool / toolTap 状态机的工具名（'' = 选择/移动）。

export interface ToolDef {
  id: string;
  icon: string;
  name: string;
}

export const TOOLS: ToolDef[] = [
  { id: '', icon: '↖', name: '选择' },
  { id: 'point', icon: '⊕', name: '点' },
  { id: 'segment', icon: '╱', name: '线段' },
  { id: 'line', icon: '─', name: '直线' },
  { id: 'ray', icon: '→', name: '射线' },
  { id: 'vector', icon: '⇢', name: '向量' },
  { id: 'circle', icon: '◯', name: '圆' },
  { id: 'midpoint', icon: '⊙', name: '中点' },
  { id: 'perpBisector', icon: '⊥', name: '中垂线' },
  { id: 'angleBisector', icon: '∠', name: '角平分线' },
  { id: 'angle', icon: '∡', name: '角' },
  { id: 'polygon', icon: '⬟', name: '多边形' },
  { id: 'intersect', icon: '∩', name: '交点' },
  { id: 'tangent', icon: '⊘', name: '切线' },
  { id: 'translate', icon: '»', name: '平移' },
  { id: 'reflect', icon: '⇋', name: '反射' },
  // 第二十三包 工具第一批（上游经典版工具盘补缺，映射既有/扩展命令）
  { id: 'perpLine', icon: '┴', name: '垂线' },
  { id: 'parLine', icon: '∥', name: '平行线' },
  { id: 'vecFromPoint', icon: '⇛', name: '相等向量' },
  { id: 'reflectPoint', icon: '⇄', name: '中心对称' },
  { id: 'dist', icon: '↔', name: '距离' },
  { id: 'area', icon: '▣', name: '面积' },
  { id: 'slope', icon: '⊿', name: '斜率' },
  // 第二十六包 工具第二批（需参数对话框：拾取齐 → "ask|" → 宿主弹参数框
  // → kernelToolParam 完成构造）
  { id: 'segLength', icon: '╱', name: '定长线段' },
  { id: 'circleRadius', icon: '◉', name: '圆（半径）' },
  { id: 'angleSize', icon: '⊾', name: '定角' },
  { id: 'rotate', icon: '↻', name: '旋转' },
  { id: 'dilate', icon: '⤢', name: '位似' },
  // 二十七包 滑动条工具（点画布建 Slider，代数区滑杆驱动参数联动）；
  // 二十七包③起 icon 字段不再用于渲染（工具图标改 ToolIcon Canvas 绘制），
  // 保留作数据完整性
  { id: 'slider', icon: '⌁', name: '滑动条' }
];

// 轮 C 工具分组（上游 工具页样式）：分组头 + 每行 4 个按钮的网格。
// ids 必须覆盖 TOOLS 全部条目（组内顺序即展示顺序）。仅作文档记录——
// ToolsPanel 出于真机兼容（两版 ForEach 渲染均丢工具，表格面板同结构
// 却正常、无法真机定位）改为字面渲染，分组与顺序在该文件内写死，
// 增删工具两处需同步。
export interface ToolGroup {
  name: string;
  ids: string[];
}

export const TOOL_GROUPS: ToolGroup[] = [
  { name: '基础', ids: ['', 'point'] },
  { name: '线', ids: ['segment', 'line', 'ray', 'vector', 'perpLine', 'parLine', 'vecFromPoint', 'segLength'] },
  { name: '圆与多边形', ids: ['circle', 'polygon', 'circleRadius'] },
  { name: '构造', ids: ['midpoint', 'perpBisector', 'angleBisector', 'angle', 'intersect', 'angleSize'] },
  { name: '测量', ids: ['dist', 'area', 'slope'] },
  { name: '变换', ids: ['translate', 'reflect', 'reflectPoint', 'rotate', 'dilate'] },
  { name: '特殊', ids: ['tangent', 'slider'] }
];

// 按 id 查工具（选择工具的用法提示等场景）
export function toolById(id: string): ToolDef | undefined {
  return TOOLS.find((t: ToolDef): boolean => t.id === id);
}

// 工具提示正文（十四轮：跟随按钮的浮层提示文本）
export function toolUsage(id: string): string {
  if (id === '') {
    return '点选对象选中或拖动，空白处平移视图';
  }
  if (id === 'point') {
    return '点按画布创建点';
  }
  if (id === 'segment' || id === 'midpoint') {
    return '点选两个点';
  }
  if (id === 'line') {
    return '点选两点作直线';
  }
  if (id === 'ray') {
    return '点选起点与方向点';
  }
  if (id === 'vector') {
    return '点选起点与终点';
  }
  if (id === 'circle') {
    return '点选圆心与圆上一点';
  }
  if (id === 'perpBisector') {
    return '点选线段两端';
  }
  if (id === 'angleBisector' || id === 'angle') {
    return '依次点选三个点（第二个是顶点）';
  }
  if (id === 'polygon') {
    return '点选顶点，再点第一个顶点闭合';
  }
  if (id === 'intersect') {
    return '依次点选两个对象';
  }
  if (id === 'tangent') {
    return '先点选一个点，再点选圆或函数';
  }
  if (id === 'translate') {
    return '先点选对象，再点选两点作向量';
  }
  if (id === 'reflect') {
    return '先点选对象，再点选镜像线';
  }
  if (id === 'perpLine' || id === 'parLine') {
    return '点选一条直线/线段/射线和一个点（先后不限）';
  }
  if (id === 'vecFromPoint') {
    return '点选一个点和一个向量（先后不限）';
  }
  if (id === 'reflectPoint') {
    return '先点选对象，再点选对称中心';
  }
  if (id === 'dist') {
    return '点选两个点，或一个点和一条直线/线段/射线';
  }
  if (id === 'area') {
    return '点选多边形或圆（结果在代数区）';
  }
  if (id === 'slope') {
    return '点选直线/线段/一次函数（结果在代数区）';
  }
  // 第二十六包 工具第二批（参数在弹窗输入）
  if (id === 'segLength') {
    return '点选起点，在弹窗输入长度';
  }
  if (id === 'circleRadius') {
    return '点选圆心，在弹窗输入半径';
  }
  if (id === 'angleSize') {
    return '点选角边一点，再点顶点，弹窗输入角度';
  }
  if (id === 'rotate') {
    return '先点选对象，再点旋转中心，弹窗输入角度';
  }
  if (id === 'dilate') {
    return '先点选对象，再点位似中心，弹窗输入比例';
  }
  if (id === 'slider') {
    return '点按画布创建滑动条（−5~5），在代数区拖动驱动参数';
  }
  return '';
}
