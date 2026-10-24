// CommandDefs.ets — GGB 式指令面板数据层（六十八包①）。
// 对照安卓 GGB 自带键盘 ••• → 全屏指令搜索页（搜索框「在 所有指令 中搜索」
// + 分类列表：数学函数/所有指令/3D/GeoGebra/代数/优化指令/几何/几何变换/
// 函数与微积分/列表/向量与矩阵/图表）。分类取我们内核真实支持的子集：
// 内核没有 3D/图表/优化指令，这些分类首版不展示；Matrix/Cases/lim 只是
// 显示层模板（六十七包转换器），内核输入侧不支持，待内核补齐再放出。
// 1.0.6.1：矩阵代数批（{{…}} 字面量 + Transpose/Determinant/Invert/
// Identity/Row/Column）与回归批（FitLine/FitPoly/FitExp/Correlation-
// Coefficient）补齐，「向量与矩阵」分类自此有矩阵成员。
//
// 每条指令的 arity/实参形态与 kernel/app.cpp tryCommand 分支逐条核对（行号
// 见 CommandMeta.line 注释约定：录入时的分支锚点），插入串走键盘 onInsert
// 通路（t=ins、back=光标从串尾回退字符数、skip 语义同（）键）。名称用
// kernel ggbName 的官方大小写（cmdName 落库显示同款）；搜索关键词 keys
// 含小写名与中文别名的子串匹配原料。
//
// 数据化（同 ToolDefs.ets 防乱重构思路）：增删指令/改描述只动本文件，
// 面板与键盘组件代码不变。

export interface CommandMeta {
  name: string;       // 官方命令名（kernel cmdName 同款大小写）
  category: string;   // 分类键（取 COMMAND_CATEGORIES 之 id）
  ins: string;        // 插入串（含自动补齐的右括号）
  back: number;       // 插入后光标从串尾回退字符数（0=落在右括号前由宿主定）
  sig: string;        // 语法签名显示串（列表行右侧/详情）
  desc: string;       // 一句话中文描述
  keys: string;       // 搜索关键词（小写命令名 + 空格 + 中文别名）
}

// 分类对齐截图（相对顺序同 GGB：数学函数/所有指令/3D/GeoGebra/代数/优化
// 指令/几何/几何变换/函数与微积分/列表/向量与矩阵/图表；3D/优化指令/图表
// 内核未支持不展示，统计类 GGB 在桌面版单列，这里保留）。'all' 为伪分类
// =全集，对应 GGB 的「所有指令」。六十九包④（真机反馈⑤）：「GeoGebra」
// 分类删除——非 GGB 官方语义（官方该类是特有/实验指令），四条成员并进
// 「所有指令」（category='all' 直接可搜可点，无独立分类页）
export interface CommandCategory {
  id: string;
  name: string;
}

export const COMMAND_CATEGORIES: CommandCategory[] = [
  { id: 'mathfn', name: '数学函数' },
  { id: 'all', name: '所有指令' },
  { id: 'algebra', name: '代数' },
  { id: 'geometry', name: '几何' },
  { id: 'transform', name: '几何变换' },
  { id: 'functions', name: '函数与微积分' },
  { id: 'lists', name: '列表' },
  { id: 'vector', name: '向量与矩阵' },
  { id: 'stats', name: '统计' },
];

// 数学函数分类：内核内置单/双参函数（expr.cpp kFns + CallFnN）。键盘
// f(x) 页已常驻 sin/cos/tan/ln/log/exp 等，这里只收面板点选有价值的补充
export const MATH_FUNCTIONS: CommandMeta[] = [
  { name: 'mod', category: 'mathfn', ins: 'mod(,)', back: 2,
    sig: 'mod( <被除数>, <除数> )', desc: '取余，结果与除数同号',
    keys: 'mod 取余 余数 模' },
  { name: 'atan2', category: 'mathfn', ins: 'atan2(,)', back: 2,
    sig: 'atan2( <y>, <x> )', desc: '由坐标求辐角（象限感知）',
    keys: 'atan2 辐角 反正切' },
  { name: 'gcd', category: 'mathfn', ins: 'gcd(,)', back: 2,
    sig: 'gcd( <整数>, <整数> )', desc: '最大公因数',
    keys: 'gcd 最大公因数 公因数' },
  { name: 'lcm', category: 'mathfn', ins: 'lcm(,)', back: 2,
    sig: 'lcm( <整数>, <整数> )', desc: '最小公倍数',
    keys: 'lcm 最小公倍数 公倍数' },
  { name: 'round', category: 'mathfn', ins: 'round()', back: 1,
    sig: 'round( <数> )', desc: '四舍五入到整数',
    keys: 'round 四舍五入 取整' },
];

// 六十八包④（真机反馈③）：back=插入后光标从串尾回退的字符数。多槽模板
// 首槽光标应紧贴 '(' 之后（首个参数槽内）：Circle(,) 回退 2 落 ',' 前；
// 单参 / 无参命令回退 1 落 ')' 前（sqrt() 同款）。此前多槽模板统一 back=1
// 光标落了第二参数槽（, 与 ) 之间）
export const COMMANDS: CommandMeta[] = [
  // ---- 几何（tryCommand 分支锚点：midpoint 1235 / point 1252 / line-segment
  // 1284 / circle 1387 / perpendicularline 1417 / distance 1458 /
  // perpendicularbisector 1540 / anglebisector 1554 / angle 1572 /
  // ray-vector 1595 / polygon 1641 / tangent 1652 / intersect 1744 /
  // slope 2622 / length 2557,2661 / area 2731 / perimeter 2780 /
  // circumference 2812 / radius 2828）----
  { name: 'Midpoint', category: 'geometry', ins: 'Midpoint(,)', back: 2,
    sig: 'Midpoint( <点>, <点> )', desc: '两点的中点',
    keys: 'midpoint 中点' },
  { name: 'Point', category: 'geometry', ins: 'Point()', back: 1,
    sig: 'Point( <路径> )', desc: '路径（线段/圆/函数等）上的动点',
    keys: 'point 路径点 附着点 动点' },
  { name: 'Line', category: 'geometry', ins: 'Line(,)', back: 2,
    sig: 'Line( <点>, <点> )', desc: '两点确定直线；或点+平行对象',
    keys: 'line 直线' },
  { name: 'Segment', category: 'geometry', ins: 'Segment(,)', back: 2,
    sig: 'Segment( <点>, <点> )', desc: '两点间线段；或定长线段',
    keys: 'segment 线段' },
  { name: 'Ray', category: 'geometry', ins: 'Ray(,)', back: 2,
    sig: 'Ray( <点>, <点> )', desc: '由两点确定的射线',
    keys: 'ray 射线' },
  { name: 'Vector', category: 'vector', ins: 'Vector(,)', back: 2,
    sig: 'Vector( <点>, <点> )', desc: '由两点确定的向量',
    keys: 'vector 向量' },
  { name: 'Circle', category: 'geometry', ins: 'Circle(,)', back: 2,
    sig: 'Circle( <圆心>, <点|半径> )', desc: '过点或给定半径的圆',
    keys: 'circle 圆' },
  { name: 'PerpendicularLine', category: 'geometry', ins: 'PerpendicularLine(,)', back: 2,
    sig: 'PerpendicularLine( <点>, <线|函数> )', desc: '过点作垂线',
    keys: 'perpendicularline 垂线 垂直' },
  { name: 'PerpendicularBisector', category: 'geometry', ins: 'PerpendicularBisector(,)', back: 2,
    sig: 'PerpendicularBisector( <点>, <点> )', desc: '两点（或线段）的中垂线',
    keys: 'perpendicularbisector 中垂线 垂直平分线' },
  { name: 'AngleBisector', category: 'geometry', ins: 'AngleBisector(,,)', back: 3,
    sig: 'AngleBisector( <点>, <顶点>, <点> )', desc: '三点的角平分线',
    keys: 'anglebisector 角平分线' },
  { name: 'Angle', category: 'geometry', ins: 'Angle(,,)', back: 3,
    sig: 'Angle( <点>, <顶点>, <点> )', desc: '三点的角（逆时针，度）',
    keys: 'angle 角 角度' },
  { name: 'Polygon', category: 'geometry', ins: 'Polygon(,,)', back: 3,
    sig: 'Polygon( <点>, …, <点> )', desc: '依次连接各点的多边形',
    keys: 'polygon 多边形' },
  { name: 'Tangent', category: 'geometry', ins: 'Tangent(,)', back: 2,
    sig: 'Tangent( <点>, <圆|函数> )', desc: '过点作圆/函数的切线',
    keys: 'tangent 切线' },
  { name: 'Intersect', category: 'geometry', ins: 'Intersect(,)', back: 2,
    sig: 'Intersect( <对象>, <对象> [, <序号>] )', desc: '两对象的交点',
    keys: 'intersect 交点 相交' },
  { name: 'Distance', category: 'geometry', ins: 'Distance(,)', back: 2,
    sig: 'Distance( <点|线>, <点|线> )', desc: '两点（或点线）间距离',
    keys: 'distance 距离' },
  { name: 'Slope', category: 'geometry', ins: 'Slope()', back: 1,
    sig: 'Slope( <线> )', desc: '直线的斜率',
    keys: 'slope 斜率' },
  { name: 'Length', category: 'geometry', ins: 'Length()', back: 1,
    sig: 'Length( <对象> )', desc: '线段/路径/列表的长度',
    keys: 'length 长度' },
  { name: 'Area', category: 'geometry', ins: 'Area()', back: 1,
    sig: 'Area( <多边形|圆|椭圆> )', desc: '图形的面积',
    keys: 'area 面积' },
  { name: 'Perimeter', category: 'geometry', ins: 'Perimeter()', back: 1,
    sig: 'Perimeter( <多边形|路径> )', desc: '多边形/路径的周长',
    keys: 'perimeter 周长' },
  { name: 'Circumference', category: 'geometry', ins: 'Circumference()', back: 1,
    sig: 'Circumference( <圆|椭圆> )', desc: '圆/椭圆的周长',
    keys: 'circumference 圆周 周长' },
  { name: 'Radius', category: 'geometry', ins: 'Radius()', back: 1,
    sig: 'Radius( <圆> )', desc: '圆的半径',
    keys: 'radius 半径' },

  // ---- 几何变换（translate 1799 / rotate 1810 / reflect 1834 / dilate 1859）----
  { name: 'Translate', category: 'transform', ins: 'Translate(,)', back: 2,
    sig: 'Translate( <对象>, <向量> )', desc: '沿向量平移对象',
    keys: 'translate 平移' },
  { name: 'Rotate', category: 'transform', ins: 'Rotate(,,°)', back: 3,
    sig: 'Rotate( <对象>, <角度>, <中心点> )', desc: '绕中心旋转对象',
    keys: 'rotate 旋转' },
  { name: 'Reflect', category: 'transform', ins: 'Reflect(,)', back: 2,
    sig: 'Reflect( <对象>, <线|点> )', desc: '关于直线/点作对称像',
    keys: 'reflect 反射 对称 镜像' },
  { name: 'Dilate', category: 'transform', ins: 'Dilate(,,)', back: 3,
    sig: 'Dilate( <对象>, <比例>, <中心点> )', desc: '关于中心按比例缩放',
    keys: 'dilate 缩放 位似 放大 缩小' },

  // ---- 函数与微积分（derivative 1934 / integral 2035,2370 / root 2071 /
  // extremum 2197 / nthroot 1989 / floor-ceil 2019）----
  { name: 'Derivative', category: 'functions', ins: 'Derivative()', back: 1,
    sig: 'Derivative( <函数> [, <阶数>] )', desc: '函数的导函数（1..5 阶）',
    keys: 'derivative 导数 求导 微分' },
  { name: 'Integral', category: 'functions', ins: 'Integral(,,)', back: 3,
    sig: 'Integral( <函数>, <下限>, <上限> )', desc: '定积分（附区间填充）',
    keys: 'integral 积分 定积分' },
  { name: 'Root', category: 'functions', ins: 'Root(,,)', back: 3,
    sig: 'Root( <函数>, <起点>, <终点> )', desc: '区间内的函数零点',
    keys: 'root 零点 求根 方程根' },
  { name: 'Extremum', category: 'functions', ins: 'Extremum(,,)', back: 3,
    sig: 'Extremum( <函数>, <起点>, <终点> )', desc: '区间内的函数极值点',
    keys: 'extremum 极值 极值点' },
  { name: 'NthRoot', category: 'functions', ins: 'NthRoot(,)', back: 2,
    sig: 'NthRoot( <被开方数>, <根次> )', desc: 'n 次方根',
    keys: 'nthroot 次方根 开方 根式' },
  { name: 'Floor', category: 'functions', ins: 'Floor()', back: 1,
    sig: 'Floor( <数> )', desc: '向下取整',
    keys: 'floor 下取整 取整' },
  { name: 'Ceil', category: 'functions', ins: 'Ceil()', back: 1,
    sig: 'Ceil( <数> )', desc: '向上取整',
    keys: 'ceil 上取整 取整' },

  // ---- 代数（simplify-expand-factor 2577）----
  { name: 'Simplify', category: 'algebra', ins: 'Simplify()', back: 1,
    sig: 'Simplify( <表达式|函数> )', desc: '化简表达式',
    keys: 'simplify 化简' },
  { name: 'Expand', category: 'algebra', ins: 'Expand()', back: 1,
    sig: 'Expand( <表达式> )', desc: '展开括号',
    keys: 'expand 展开 去括号' },
  { name: 'Factor', category: 'algebra', ins: 'Factor()', back: 1,
    sig: 'Factor( <多项式> )', desc: '因式分解',
    keys: 'factor 因式分解 分解因式' },

  // ---- 列表（sequence 2419 / element-first-last 2486 / sort-unique-reverse
  // 2521 / length(list) 2557）----
  { name: 'Sequence', category: 'lists', ins: 'Sequence(,,,)', back: 3,
    sig: 'Sequence( <表达式>, <变量>, <起点>, <终点> [, <步长>] )',
    desc: '按变量扫区间生成列表',
    keys: 'sequence 序列 数列 列表' },
  { name: 'Element', category: 'lists', ins: 'Element(,)', back: 2,
    sig: 'Element( <列表>, <序号> )', desc: '取列表第 n 项',
    keys: 'element 元素 取项' },
  { name: 'First', category: 'lists', ins: 'First()', back: 1,
    sig: 'First( <列表> )', desc: '取列表首项',
    keys: 'first 首项 第一个' },
  { name: 'Last', category: 'lists', ins: 'Last()', back: 1,
    sig: 'Last( <列表> )', desc: '取列表末项',
    keys: 'last 末项 最后一个' },
  { name: 'Sort', category: 'lists', ins: 'Sort()', back: 1,
    sig: 'Sort( <列表> )', desc: '列表升序排序',
    keys: 'sort 排序 升序' },
  { name: 'Unique', category: 'lists', ins: 'Unique()', back: 1,
    sig: 'Unique( <列表> )', desc: '列表去重（保持原序）',
    keys: 'unique 去重' },
  { name: 'Reverse', category: 'lists', ins: 'Reverse()', back: 1,
    sig: 'Reverse( <列表> )', desc: '列表倒序',
    keys: 'reverse 倒序 反转' },
  { name: 'Take', category: 'lists', ins: 'Take(,)', back: 2,
    sig: 'Take( <列表>, <起点> [, <终点>] )', desc: '取列表一段（1 起含端点）',
    keys: 'take 取段 截取 切片' },
  { name: 'Drop', category: 'lists', ins: 'Drop(,)', back: 2,
    sig: 'Drop( <列表>, <起点> [, <终点>] )', desc: '去掉列表一段（两参=去前 n 项）',
    keys: 'drop 去段 去掉 丢弃' },
  { name: 'Insert', category: 'lists', ins: 'Insert(,,)', back: 3,
    sig: 'Insert( <数或列表>, <列表>, <位置> )', desc: '在指定位次插入元素',
    keys: 'insert 插入' },
  { name: 'Remove', category: 'lists', ins: 'Remove(,)', back: 2,
    sig: 'Remove( <列表>, <列表或数> )', desc: '去掉列表中出现的元素',
    keys: 'remove 移除 删除元素' },
  { name: 'Zip', category: 'lists', ins: 'Zip(,,,)', back: 3,
    sig: 'Zip( <表达式>, <变量1>, <列表1> [, <变量2>, <列表2>…] )',
    desc: '多列表并行迭代生成新列表',
    keys: 'zip 并行 迭代 拉链' },
  { name: 'Flatten', category: 'lists', ins: 'Flatten()', back: 1,
    sig: 'Flatten( <矩阵或列表> )', desc: '矩阵/列表展开为扁平列表',
    keys: 'flatten 展开 扁平化' },
  { name: 'Join', category: 'lists', ins: 'Join(,)', back: 2,
    sig: 'Join( <列表1>, <列表2>, … )', desc: '顺次拼接多个列表',
    keys: 'join 拼接 合并 连接' },

  // ---- 统计（mean-median-min-max-sum-product-sd-variance 2451；单参列表版）----
  { name: 'Mean', category: 'stats', ins: 'Mean()', back: 1,
    sig: 'Mean( <列表> )', desc: '列表的算术平均',
    keys: 'mean 平均 平均值' },
  { name: 'Median', category: 'stats', ins: 'Median()', back: 1,
    sig: 'Median( <列表> )', desc: '列表的中位数',
    keys: 'median 中位数' },
  { name: 'Min', category: 'stats', ins: 'Min()', back: 1,
    sig: 'Min( <列表> )', desc: '列表的最小值',
    keys: 'min 最小值 最小' },
  { name: 'Max', category: 'stats', ins: 'Max()', back: 1,
    sig: 'Max( <列表> )', desc: '列表的最大值',
    keys: 'max 最大值 最大' },
  { name: 'Sum', category: 'stats', ins: 'Sum()', back: 1,
    sig: 'Sum( <列表> )', desc: '列表求和',
    keys: 'sum 求和 总和' },
  { name: 'Product', category: 'stats', ins: 'Product()', back: 1,
    sig: 'Product( <列表> )', desc: '列表求积',
    keys: 'product 求积 乘积' },
  { name: 'SD', category: 'stats', ins: 'SD()', back: 1,
    sig: 'SD( <列表> )', desc: '列表的标准差',
    keys: 'sd 标准差 方差根' },
  { name: 'Variance', category: 'stats', ins: 'Variance()', back: 1,
    sig: 'Variance( <列表> )', desc: '列表的方差',
    keys: 'variance 方差' },
  { name: 'FitLine', category: 'stats', ins: 'FitLine(,)', back: 2,
    sig: 'FitLine( <x 列表>, <y 列表> )', desc: '最小二乘回归直线',
    keys: 'fitline 回归直线 拟合直线 线性回归' },
  { name: 'FitPoly', category: 'stats', ins: 'FitPoly(,,)', back: 3,
    sig: 'FitPoly( <x 列表>, <y 列表>, <阶数> )', desc: '多项式回归（1..8 阶）',
    keys: 'fitpoly 多项式回归 拟合多项式' },
  { name: 'FitExp', category: 'stats', ins: 'FitExp(,)', back: 2,
    sig: 'FitExp( <x 列表>, <y 列表> )', desc: '指数回归 a·e^(b·x)',
    keys: 'fitexp 指数回归 拟合指数' },
  { name: 'CorrelationCoefficient', category: 'stats', ins: 'CorrelationCoefficient(,)', back: 2,
    sig: 'CorrelationCoefficient( <x 列表>, <y 列表> )', desc: 'Pearson 相关系数 r',
    keys: 'correlationcoefficient 相关系数 pmcc 积差相关' },
  { name: 'Frequency', category: 'stats', ins: 'Frequency()', back: 1,
    sig: 'Frequency( <列表> )', desc: '各值出现次数（与 Unique 同序）',
    keys: 'frequency 频数 频率 次数' },
  { name: 'BarChart', category: 'stats', ins: 'BarChart(,)', back: 2,
    sig: 'BarChart( <数据> | <横坐标>, <高度> [, <条宽>] )', desc: '柱状图',
    keys: 'barchart 柱状图 条形图 bar' },
  { name: 'Histogram', category: 'stats', ins: 'Histogram(,)', back: 2,
    sig: 'Histogram( <数据>, <组数> | <组界>, <数据> )', desc: '直方图（等距分组计数）',
    keys: 'histogram 直方图 分组 统计图' },
  { name: 'LineGraph', category: 'stats', ins: 'LineGraph(,)', back: 2,
    sig: 'LineGraph( <x 列表>, <y 列表> )', desc: '折线图',
    keys: 'linegraph 折线图 折线' },
  { name: 'ScatterPlot', category: 'stats', ins: 'ScatterPlot(,)', back: 2,
    sig: 'ScatterPlot( <x 列表>, <y 列表> )', desc: '散点图',
    keys: 'scatterplot 散点图 散点' },
  { name: 'BoxPlot', category: 'stats', ins: 'BoxPlot(,,)', back: 3,
    sig: 'BoxPlot( <y 中心> [, <箱高>], <数据> )', desc: '箱线图（五数概括）',
    keys: 'boxplot 箱线图 盒须图 箱形图 四分位' },
  { name: 'StemPlot', category: 'stats', ins: 'StemPlot(,)', back: 2,
    sig: 'StemPlot( <数据> [, <调整位>] )', desc: '茎叶图',
    keys: 'stemplot 茎叶图 茎叶' },

  // ---- GeoGebra（slider 1885 / startanimation 1908 / stopanimation 1921 /
  // text 2284）----
  { name: 'Slider', category: 'all', ins: 'Slider(,)', back: 2,
    sig: 'Slider( <最小值>, <最大值> [, <步长>] )', desc: '创建滑动条数值',
    keys: 'slider 滑动条 滑块' },
  { name: 'StartAnimation', category: 'all', ins: 'StartAnimation()', back: 1,
    sig: 'StartAnimation()', desc: '启动滑动条/点动画',
    keys: 'startanimation 启动动画 动画' },
  { name: 'StopAnimation', category: 'all', ins: 'StopAnimation()', back: 1,
    sig: 'StopAnimation()', desc: '停止动画',
    keys: 'stopanimation 停止动画 动画' },
  { name: 'Text', category: 'all', ins: 'Text("")', back: 1,
    sig: 'Text( "文本" [, 位置] )', desc: '在绘图区显示文本',
    keys: 'text 文本 文字 标注' },

  // ---- 向量与矩阵（1.0.6.1：内核矩阵 = 列表的列表 {{…},{…}}，GGB 同构；
  // transpose/determinant/invert/identity/row/column 批）----
  { name: 'Transpose', category: 'vector', ins: 'Transpose()', back: 1,
    sig: 'Transpose( <矩阵> )', desc: '矩阵转置',
    keys: 'transpose 转置 矩阵' },
  { name: 'Determinant', category: 'vector', ins: 'Determinant()', back: 1,
    sig: 'Determinant( <方阵> )', desc: '方阵的行列式（短名 det）',
    keys: 'determinant det 行列式 方阵' },
  { name: 'Invert', category: 'vector', ins: 'Invert()', back: 1,
    sig: 'Invert( <方阵> )', desc: '方阵的逆（奇异阵报错）',
    keys: 'invert 逆矩阵 求逆' },
  { name: 'Identity', category: 'vector', ins: 'Identity()', back: 1,
    sig: 'Identity( <阶数> )', desc: 'n 阶单位矩阵',
    keys: 'identity 单位矩阵 恒等' },
  { name: 'Row', category: 'vector', ins: 'Row(,)', back: 2,
    sig: 'Row( <矩阵>, <序号> )', desc: '取矩阵第 n 行（1 起）',
    keys: 'row 行 取行' },
  { name: 'Column', category: 'vector', ins: 'Column(,)', back: 2,
    sig: 'Column( <矩阵>, <序号> )', desc: '取矩阵第 n 列（1 起）',
    keys: 'column 列 取列' },
];

// 面板全量数据：命令 + 数学函数补充，搜索/「所有指令」都查这份
export const ALL_COMMANDS: CommandMeta[] = COMMANDS.concat(MATH_FUNCTIONS);
