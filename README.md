# 梦烬几何 DreamEmber GeoCalc

> 原生 OpenHarmony 几何图形计算器：作图、函数图像、滑动条、表格，全离线。

**梦烬几何**是一款面向学生、教师和数学爱好者的几何与代数图形计算器，采用纯原生 ArkTS + NAPI C++ 内核开发，全部功能离线可用，不申请任何权限、不收集任何数据。

## 功能特性

- **几何作图**：点、线段、直线、射线、圆、多边形、角、切线、中点、交点、垂线、垂直平分线、角平分线、向量等对象，以及旋转、位似、平移等几何变换；随时度量长度、距离、面积、周长、斜率与角度；
- **代数区**：所有对象集中列出，行内改名、改值、改定义，改一步图就变；
- **函数与命令**：内置数学符号键盘（根号、分式、角度、不等号等），命令提示逐字补全；求导、积分、求根、极值等由本地计算机代数引擎精确计算；
- **滑动条**：数值参数化，拖动滑块图形实时联动，名称/当前值/最小/最大四字段行内编辑；
- **表格区**：类电子表格逐格编辑，提交即生成对象，支持与数据视图联动；
- **双形态布局**：手机竖屏单手操作；平板与 2in1 宽屏自动切换三栏停靠布局，自由窗口任意缩放；
- **深浅色自适应**：跟随系统外观；
- **纯离线**：无广告、无账号、不联网、不收集任何数据，图形文件仅保存在设备本地。

## 系统要求

- OpenHarmony SDK **API 23**（`compileSdkVersion` 23）
- 设备类型：`default`（手机等）、`tablet`与 `2in1`（宽屏自由窗口布局）

## 从源码构建

1. 克隆本仓库：

   ```bash
   git clone https://gitee.com/CH_mengxiang/DreamEmber-GeoCalc.git
   ```

2. 用 **DevEco Studio** 打开工程根目录，等待 SDK 与依赖自动就绪（会自动生成本机文件 `local.properties`，该文件不入库）。

3. 配置签名：`File > Project Structure > Signing Configs`。**仓库不含任何签名材料**（根 `build-profile.json5` 的 `signingConfigs` 已清空），请使用自己的调试/发布证书。

4. 构建（DevEco 菜单 `Build > Build Hap(s)/APP(s)`，或命令行）：

   ```bash
   # Windows 示例：使用 DevEco Studio 自带 hvigor
   node "C:\Program Files\Huawei\DevEco Studio\tools\hvigor\bin\hvigorw.js" \
     --mode module -p module=dreamEmberGeoCalc@default -p product=default \
     -p buildMode=release assembleHap --no-daemon
   ```

   若命令行报找不到 SDK，设置环境变量 `OHOS_BASE_SDK_HOME` 指向 OpenHarmony SDK 目录。

5. 产物位于 `entry/build/default/outputs/default/`（`assembleApp` 的 `.app` 产物在工程根 `build/outputs/default/`）。

C++ 内核（`cpp/kernel/`、`cpp/cas/`）与静态链接的 giac CAS 源码（`cpp/giac/`）由 hvigor 经 CMake 自动编译，首次构建耗时较长，请耐心等待。

## 工程结构

```
DreamEmber-GeoCalc/
├── AppScope/                    # 应用级配置（app.json5、应用图标与名称）
├── entry/                       # entry 模块 dreamEmberGeoCalc
│   └── src/main/
│       ├── ets/                 # ArkTS UI（页面、组件、内核交互编排、数学键盘）
│       ├── cpp/
│       │   ├── kernel/          # 几何内核 C++（对象模型、表达式求值、视图、文件读写）
│       │   ├── cas/             # CAS 薄封装（对接 giac）
│       │   ├── giac/            # 第三方 CAS 库 giac（GPL-3.0+，静态链接）
│       │   ├── kernel_napi.cpp  # NAPI 导出层（约 38 个接口）
│       │   ├── napi_init.cpp
│       │   └── types/           # ArkTS 侧 .d.ts 声明
│       ├── resources/           # 图标、字符串等资源
│       └── module.json5         # 模块配置（零权限声明）
├── host-tests/                  # 内核宿主端单元测试（test16–test25）
├── build-profile.json5          # 工程构建配置（签名段已清空）
├── hvigorfile.ts / hvigor/      # hvigor 构建系统
└── oh-package.json5             # 依赖管理
```

## 宿主端测试

`host-tests/` 内含 10 套内核宿主端单元测试（`test16.cpp`–`test25.cpp`），无需设备，直接用 g++ 在 PC 上编译运行（在 `entry/src/main/cpp/` 目录下执行，以 test16 为例）：

```bash
g++ -std=c++17 -D_USE_MATH_DEFINES -include cmath -include cstring -I . \
  ../../../host-tests/test16.cpp kernel/app.cpp kernel/expr.cpp \
  kernel/geos.cpp kernel/view.cpp kernel/ggbfile.cpp cas/cas_wrapper.cpp \
  -lz -o test16.exe
```

各套件的覆盖范围见文件头部注释；全部通过（0 FAIL）即内核回归绿。

## 许可证

本项目以 **GPL-3.0-or-later** 协议开源，详见 [LICENSE](LICENSE)。

内核静态链接了开源计算机代数库 **giac**（© Bernard Parisse / Institut Fourier，GPL-3.0-or-later；源码随本仓库发布于 `entry/src/main/cpp/giac/`）。依据 GPL 条款，本项目整体以 GNU GPL-3.0-or-later 发布。

## 声明

- 本项目为独立开源项目，与 GeoGebra 官方（GeoGebra GmbH）无任何关联；未使用其商标、图形资源或源代码（开发过程中参考了其公开源码与交互设计）。
- 应用全离线运行，不申请任何系统权限，不收集任何数据。
