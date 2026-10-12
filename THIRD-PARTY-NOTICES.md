# Third-Party Notices / 第三方组件声明

本应用随包分发以下第三方开源组件，各依其原许可证使用。本文件为对应的
版权与许可声明存档。

---

## 1. RaTeX（LaTeX 数学排版引擎）

- **来源**：<https://github.com/erweixin/RaTeX>（v0.1.14）
- **用途**：KaTeX 兼容的纯 Rust 数学排版引擎，将 LaTeX 排版为
  DisplayList JSON（em 坐标几何）；本应用以预编译动态库
  `entry/src/main/cpp/ratex/prebuilt/<abi>/libratex_ffi.so` 随包分发
  （C ABI 仅 3 个导出函数，桥接层为本项目自研）。
- **许可证**：MIT License — Copyright (c) erweixin

> MIT License
>
> Copyright (c) erweixin
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

---

## 2. KaTeX 数学字体（20 个 TTF）

- **来源**：<https://github.com/KaTeX/KaTeX>（未修改的二进制字体）
- **用途**：公式字形轮廓绘制（`entry/src/main/resources/rawfile/fonts/KaTeX_*.ttf`，
  随包 rawfile 分发，许可文本同包于 `entry/src/main/resources/rawfile/fonts/OFL.txt`）。
- **许可证**：SIL Open Font License, Version 1.1（**非** MIT）。
  完整许可文本见上游 <https://github.com/KaTeX/KaTeX/blob/master/LICENSE> 及
  包内 `OFL.txt`；以下为 OFL 1.1 版权头摘要：

> Copyright (c) 2021 The KaTeX Authors
>
> This Font Software is licensed under the SIL Open Font License, Version 1.1.
> This license is copied below, and is also available with a FAQ at:
> <http://scripts.sil.org/OFL>
>
> （完整 OFL 1.1 条文以包内 `OFL.txt` 为准。）

---

## 3. giac（计算机代数系统 / CAS）

- **来源**：© Bernard Parisse / Institut Fourier，<https://www-fourier.ujf-grenoble.fr/~parisse/giac.html>
- **用途**：符号计算（求导、积分、求根、极值等），静态链接进本应用内核
  （源码随本仓库发布于 `entry/src/main/cpp/giac/`）。
- **许可证**：GPL-3.0-or-later。本应用因此整体以 GNU GPL-3.0-or-later 发布。

---

## 其他运行时依赖

- **OpenHarmony SDK / ArkTS / NAPI**：HarmonyOS / OpenHarmony 开源组件，依其
  各自许可（Apache-2.0 等）使用；
- Rust 生态编译进 `libratex_ffi.so` 的 crates（aho-corasick、ab_glyph 等）
  均为 MIT / Apache-2.0 双许可，声明随上游源发行。
