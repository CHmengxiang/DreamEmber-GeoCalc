# 文本与 LaTeX 渲染选型调研报告

> 2026-10-07 调研轮（1.0.4 内**只调研不实现**）。目的：为「MathField 公式
> 品质升级 / 画布文本对象内嵌公式 / .ggb LaTeX 语法兼容」选择渲染技术路线。
> 结论摘要见 §4，先看它也行。

## 1. 现状与需求

**现状盘点（本项目）**

- **MathField.ets（1395 行，自研）**：公式输入/显示编辑器——上标/下标、
  根号（含 NthRoot 根次）、log 模板、绝对值、占位框、点触定位光标
  （MathPiece 带原文区间 + 视觉反查停靠点）。**分数目前为线形 "a/b"
  （文件头注释明言 v1），堆栈式分式为已推迟项**。
- **画布文本对象**：内核 GeoText（app.cpp）带 segs/refs，支持对象值内插；
  渲染走 `DrawOp::Text` → ArkTS `ctx.fillText`（14×uiScale px 系统字体
  sans-serif），单行纯文本，无富文本无公式。
- **.ggb 往返**：上游生态的文本/公式常用 LaTeX 语法（`\frac{}{}` 等），
  我们目前遇 LaTeX 字符串只能原样直显——这是「真 LaTeX 语法」兼容的
  实际压力点。
- **字体**：未注册任何自定义字体（无 @ohos.font）；ArkTS 支持运行时注册
  自定义字体供 Canvas 使用；C++ 侧另有 native_drawing NDK（OH_Drawing_*）
  可自绘并可加载自定义字库。

**需求场景（按出现顺序）**

1. MathField 编辑器显示品质（堆栈分式、更多符号）——**编辑态**需要光标
   停靠/点触命中，任何"纯渲染库"都不提供这部分；
2. 画布文本对象内嵌公式（上游 GeoGebra 文本支持 `$...$` LaTeX 片段）；
3. .ggb 文件的 LaTeX 语法兼容（读入不丢样）；
4. 代数区行内公式显示（远期）。

**硬约束**

- 全离线零权限；上架合规：**主功能不得用 Web 组件**；工程 GPL-3.0-or-later
  全开源（三方件须许可兼容并随仓开源/声明）；包体现 42MB 量级，增量预算
  严格；渲染管线 = 内核 DrawCmd → ArkTS Canvas（或 native_drawing NDK）。

## 2. 候选方案矩阵

| 方案 | 保真度 | 编辑器整合 | 包体增量 | 工作量 | 许可 | 结论 |
| --- | --- | --- | --- | --- | --- | --- |
| **A 自研 MathField 演进**（ArkTS 现状延长线） | 中→中上 | ★★★ 已有 | 0 | 每模板 1-2 轮 | 无依赖 | **近期采用** |
| **B 移植 iosMath/SwiftMath 架构**（MIT）+ Latin Modern Math 字体（GFL） | 上（真 LaTeX 子集） | ★★ 需接 | +1~2MB（字库） | 大（专项数轮） | MIT 代码 / GFL 字体（随包需附许可、保留字体名） | **中期候选** |
| **C RaTeX**（纯 Rust，>99.5% KaTeX 语法，MIT） | 上 | ★ 未验证 | +1~3MB（Rust 核心） | 中大（Rust→OHOS NAPI 自行编译） | MIT | **观察项** |
| D WebView + KaTeX/MathJax（MIT/Apache） | 上 | ★ 差 | — | 小 | — | **排除**：违反上架合规（主功能不得用 Web 组件） |
| E 嵌入完整 TeX 引擎（Tectonic=XeTeX in Rust，MIT） | 天花板 | ★ 极差 | +几十 MB（字体集/宏包） | 巨大 | MIT | **排除**：体积与交互时延完全不适配公式编辑器 |
| F MathML 原生渲染 | — | — | — | — | — | **排除**：无成熟原生实现 |
| G OHOS 生态现成三方库 | — | — | — | — | — | **结论：无成熟原生件**；生态主流=WebView 方案（被我们排除）或手写 ArkTS；持续观察 TPC |

## 3. 各方案详述

### A. 自研 MathField 演进（近期推荐）

把已推迟的**堆栈分式**做掉（MathPiece 扩展分式原子：分子/分分子盒、横线、
行高联动、光标停靠分子分母），随后按需求长尾补符号（大括号自适应、矩阵
等再往后排）。优势：编辑态资产（点触定位、原文区间映射、模板光标规则）
是四十六/四十七包刚打磨好的，任何外部渲染库都不提供这些，换渲染=重写编
辑器；零包体增量；无许可问题。上限：渲染品质可对齐上游手机端观感，但
不是完整 LaTeX 语法（.ggb 里 `\frac` 仍不识别）。

### B. 移植 iosMath/SwiftMath 架构（中期候选）

[iosMath](https://github.com/kostub/iosMath)（ObjC，MIT）/
[SwiftMath](https://github.com/mgriebling/SwiftMath)（Swift，MIT）是完整
的 LaTeX 数学子集排版引擎，架构为经典 TeXFormula→盒树（Char/Seq/Frac/
Scripts/BigOperator/Delim 盒）→自绘字形，配 [Latin Modern Math](https://www.gust.org.pl/projects/e-foundry/lm-math)
OpenType 字库（GUST Font License：免费、可随包分发，须保留字体名并附许
可文本）。移植要点：盒树排版核心 → C++ 内核模块（或 ArkTS，推荐 C++，
排版纯计算）；字形绘制经两条候选管线之一：①ArkTS 注册字体（@ohos.font）
后以 glyph 文本+偏移输出为新 DrawCmd op；②C++ native_drawing 自绘。
工作量评估：引擎核心数千行移植 + 字体度量管线 + 与 MathField 编辑态打通
（盒树含位置信息，光标映射反而比视觉反查简单）——**数轮的专项**，非顺手改。

### C. RaTeX（观察项）

[RaTeX](https://github.com/erweixin/RaTeX)：纯 Rust LaTeX 数学渲染引擎，
>99.5% KaTeX 语法覆盖，MIT 许可，明确无 JS/WebView/DOM，已出
iOS/Android/Flutter/Web/服务端包（v0.0.15，2026-09 仍活跃）。对本项目
的吸引力=成熟 KaTeX 兼容语法解析+排版直接复用；接入路径=OpenHarmony
NDK 自带 Rust 目标（aarch64-unknown-linux-ohos），自行编 rust 库 + C ABI
+ NAPI 桥。**未验证项（观察期要弄清的）**：①输出形态——若只出位图/SVG
而无盒树元数据，编辑态光标/点触命中要自己反查（大成本）；②OHOS Rust
工具链在 DevEco 当前版本的实际可用性；③0.0.x 版本 API 稳定性与维护方
活跃度；④字库随行方式与包体。

### D/E/F 排除理由（存档）

- **D WebView+KaTeX/MathJax**：上架合规明确「主功能不得用 Web 组件」，
  公式将成主功能的一部分，直接违反；且引入网络性假设（虽可本地打包 JS）。
- **E Tectonic（XeTeX in Rust，MIT）**：完整 TeX 引擎+字体集几十 MB、
  首编译时延秒级——为静态出版级排版设计，与交互式编辑器场景错配。
- **F MathML**：无成熟 C++/原生 MathML 渲染器，绕道无收益。

### G. OHOS 生态现状

调研口径（2026-10）：鸿蒙渲染 LaTeX 的公开方案只有「三方库（多为 Web
内核封装）」与「WebView 加载 KaTeX/MathJax 页」两类（IT营/掘金/知乎），
**无成熟原生（C++/ArkTS 自绘）公式渲染三方库**；OpenHarmony TPC 暂无
对位项目。结论：别指望现成件，路线只能在 A/B/C 里选。

## 4. 结论与推荐

1. **近期（1.0.x 正常轮次）：方案 A。** 下一功能=堆栈分式（本就是推迟
   项），之后按反馈补符号。理由：编辑态资产不可替代、零许可/包体风险、
   每步可用真机验收。
2. **中期（当出现真需求时立专项）：方案 B。** 触发条件任一：.ggb LaTeX
   兼容成为用户反馈热点、文本对象内嵌公式排期、矩阵/积分等高级排版需求。
   落法=MIT 引擎排版核心移植进内核，Latin Modern Math 字库随包（附 GFL
   许可文本），DrawCmd 管线新增 glyph-run 运算。
3. **C（RaTeX）持续观察**：等它出 OHOS 目标/到 1.0、并确认输出含盒树
   元数据后，重新评估——若成立，可能比 B 省数轮工作量。
4. 本轮不切版本、不出包；方案 A 的堆栈分式建议作为 1.0.4 后续或 1.0.5
   首个功能，届时再走正常轮次流程。

## 5. 风险与开放问题

- RaTeX 输出形态（盒树 vs 位图）未验证——决定 C 路线生死；
- OHOS NDK Rust 目标与 DevEco/hvigor 的集成成熟度未实测；
- GFL 字体随包分发需在「关于」页与仓库 LICENSE 侧附许可声明（合规动作，
  选 B 时一并做）；
- 堆栈分式引入后 MathField 行高/滚动/点触 y 仲裁的联动回归面（四十七包
  刚校准过阈值）；
- 上游 GeoGebra 的 LaTeX 渲染为其自研/分支体系——我们只参考布局交互，
  不搬代码（上架合规红线）。
