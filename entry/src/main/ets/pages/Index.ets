import { BusinessError } from '@kit.BasicServicesKit';
import { ConfigurationConstant } from '@kit.AbilityKit';
import { preferences } from '@kit.ArkData';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { router } from '@kit.ArkUI';
import dreamember from 'libdreamember.so';
import { CanvasView } from '../views/CanvasView';

/**
 * 梦烬几何 — 原生主界面
 * N3 第一轮：汉堡菜单抽屉（新建/重置视图/设置/关于）+ 设置底部面板
 * （网格/坐标轴开关，经 AppStorage 同步到 CanvasView 再下发内核）
 * + 返回键栈（菜单 → 设置 → 系统默认退后台）。
 * N3 第三轮：深色模式 + 断点双布局（≥640vp 宽屏，菜单加宽）+ 数学键盘弹出时
 * 底部垫高（自绘键盘是浮层，需手动避让）。
 * 本轮：外观三选项改为"深色模式 + 浅色/深色/跟随系统"按钮行（经
 * ApplicationContext.setColorMode 全局生效并持久化）；N4：菜单新增
 * 打开/保存 .ggb（事件打点，CanvasView 经文件选择器 + fs 完成）。
 * 第五包（2026-09-30）：底部"CAS 求值"冒烟面板删除（上游 无此栏，系早期
 * Giac 集成的测试残留）；菜单加"视图"分组，新增 CAS 视图行（上游经典版
 * 的 CAS 入口同位），经 evt.openCas 打开 CanvasView 底部抽屉 CasPanel。
 */
const DOMAIN: number = 0x0001;
const TAG: string = 'DreamEmber';

@Entry
@Component
struct Index {
  private nativeLogged: boolean = false;          // 原生库启动日志只打一次
  @State private menuOpen: boolean = false;       // 汉堡菜单抽屉
  @State private settingsOpen: boolean = false;   // 设置底部面板
  @State private isWide: boolean = false;         // 断点：≥640vp
  // 设置开关（与 CanvasView 共享 AppStorage 键，改动即时同步到内核）
  @StorageLink('opt.showGrid') private setGrid: boolean = true;
  @StorageLink('opt.showAxes') private setAxes: boolean = true;
  // 自绘数学键盘可见 → 页面底部垫高（CanvasView 写入）。第十三包②：
  // 垫高只保留给宽屏（窄屏垫高移入 CanvasView 底栏上方——底栏不动、
  // 键盘盖底栏，避让对齐系统键盘）
  // 第十七包②：ui.mathKbVisible / ui.isWide 页面级垫高门控删除——键盘
  // 垫高（宽/窄屏）整体收进 CanvasView 运行时实测，Index 不再掺和键盘几何
  // 深色模式：跟随系统（EntryAbility 同步）
  @StorageProp('ui.dark') private dark: boolean = false;
  // 外观模式：0=跟随系统 1=浅色 2=深色（持久化，Ability 启动时恢复）
  @StorageLink('ui.themeMode') private themeMode: number = 0;
  // 默认输入方式：0=系统键盘 1=自带键盘（持久化，Ability 启动时恢复；
  // 第十五包②：默认改 1=自带键盘）
  @StorageLink('opt.inputMethod') private inputMethodOpt: number = 1;
  // 反馈②：页面顶栏删除，☰ ↶ ↷ 移至 CanvasStage 画布右上浮层；撤销状态
  // 真源在 CanvasView（@Prop 直传），菜单请求经 CanvasView.onMenuRequest 回调
  // 打开本页抽屉（打开时顺带打点 evt.dismissKb 收键盘）

  // ---- 深浅两套配色 ----
  private cPage(): string {
    return this.dark ? '#17181A' : '#F0F2F5';
  }

  private cSurface(): string {
    return this.dark ? '#232529' : '#FFFFFF';
  }

  private cText(): string {
    return this.dark ? '#E8EAED' : '#333333';
  }

  private cTitle(): string {
    return this.dark ? '#F2F3F5' : '#1C1C1F';
  }

  private cSub(): string {
    return this.dark ? '#9AA0A6' : '#666666';
  }

  private cFaint(): string {
    return this.dark ? '#6E747D' : '#888888';
  }

  private cAccent(): string {
    return this.dark ? '#8FB6FF' : '#007DFF';
  }

  private cDivider(): string {
    return this.dark ? '#3A3D42' : '#EEEEEE';
  }

  private cScrim(): string {
    return this.dark ? 'rgba(0,0,0,0.55)' : 'rgba(0,0,0,0.40)';
  }

  onPageShow(): void {
    if (this.nativeLogged) {
      return;
    }
    this.nativeLogged = true;
    try {
      const v = dreamember.nativeVersion();
      hilog.info(DOMAIN, TAG, 'native layer: %{public}s', v);
    } catch (err) {
      const e = err as BusinessError;
      hilog.error(DOMAIN, TAG, 'native lib load failed: %{public}s', e.message ?? '');
    }
  }

  // 返回键栈：菜单 → 设置 → 无浮层走系统默认（退后台）
  onBackPress(): boolean {
    if (this.menuOpen) {
      this.menuOpen = false;
      return true;
    }
    if (this.settingsOpen) {
      this.settingsOpen = false;
      return true;
    }
    return false;
  }

  // 一次性事件打点：CanvasView 里 @StorageProp + @Watch 接收
  private fireEvt(key: string): void {
    AppStorage.setOrCreate(key, Date.now());
  }

  private confirmNew(): void {
    this.menuOpen = false;
    AlertDialog.show({
      title: '新建',
      message: '清空全部对象并重置视图？',
      autoCancel: true,
      alignment: DialogAlignment.Center,
      primaryButton: {
        value: '取消',
        action: () => {}
      },
      secondaryButton: {
        value: '清空',
        fontColor: '#F44336',
        action: () => {
          this.fireEvt('evt.clearAll');
        }
      }
    });
  }

  // 外观切换：ApplicationContext.setColorMode 全局生效（Ability
  // onConfigurationUpdate 随之把有效深浅同步到 'ui.dark'），并持久化
  private applyThemeMode(mode: number): void {
    this.themeMode = mode;
    const cm = mode === 2 ? ConfigurationConstant.ColorMode.COLOR_MODE_DARK
      : mode === 1 ? ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT
        : ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET;
    try {
      getContext(this).getApplicationContext().setColorMode(cm);
    } catch (err) {
      hilog.warn(DOMAIN, TAG, 'setColorMode failed: %{public}s', JSON.stringify(err));
    }
    try {
      const pref = preferences.getPreferencesSync(
        getContext(this).getApplicationContext(), { name: 'dreamember_settings' });
      pref.putSync('themeMode', mode);
      pref.flush();
    } catch (err) {
      hilog.warn(DOMAIN, TAG, 'save themeMode failed: %{public}s', JSON.stringify(err));
    }
  }

  // 默认输入方式切换（CanvasView @Watch 联动切换键盘），并持久化
  private applyInputMethod(mode: number): void {
    if (this.inputMethodOpt === mode) {
      return;
    }
    this.inputMethodOpt = mode;
    try {
      const pref = preferences.getPreferencesSync(
        getContext(this).getApplicationContext(), { name: 'dreamember_settings' });
      pref.putSync('inputMethod', mode);
      pref.flush();
    } catch (err) {
      hilog.warn(DOMAIN, TAG, 'save inputMethod failed: %{public}s', JSON.stringify(err));
    }
  }

  @Builder
  private MenuRow(title: string, sub: string, onTap: () => void) {
    Row() {
      Column() {
        Text(title)
          .fontSize(16)
          .fontColor(this.cText())
        if (sub.length > 0) {
          Text(sub)
            .fontSize(11)
            .fontColor(this.cFaint())
            .margin({ top: 2 })
        }
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
    }
    .width('100%')
    .height(52)
    .padding({ left: 20, right: 16 })
    .onClick(() => onTap())
  }

  // 外观选项按钮（选中高亮胶囊）
  @Builder
  private themeChip(label: string, mode: number) {
    Text(label)
      .fontSize(13)
      .fontColor(this.themeMode === mode ? '#FFFFFF' : this.cText())
      .backgroundColor(this.themeMode === mode ? this.cAccent() : (this.dark ? '#33363B' : '#E9EBF2'))
      .padding({ left: 12, right: 12, top: 6, bottom: 6 })
      .borderRadius(15)
      .stateStyles({
        pressed: {
          .backgroundColor(this.themeMode === mode ? this.cAccent() : (this.dark ? '#3E424A' : '#DDE1EA'))
        }
      })
      .margin({ left: 8 })
      .onClick(() => {
        if (this.themeMode !== mode) {
          this.applyThemeMode(mode);
        }
      })
  }

  // 外观一行：标签 + 三个选项按钮同行
  @Builder
  private themeRow() {
    Row() {
      Text('深色模式')
        .fontSize(15)
        .fontColor(this.cText())
        .layoutWeight(1)
      this.themeChip('浅色', 1)
      this.themeChip('深色', 2)
      this.themeChip('跟随系统', 0)
    }
    .width('100%')
    .height(44)
  }

  // 默认输入方式一行：标签 + 两个选项按钮同行
  @Builder
  private inputMethodRow() {
    Row() {
      Column() {
        Text('默认键盘')
          .fontSize(15)
          .fontColor(this.cText())
        Text('点输入框时优先弹出的键盘，⌨ 仍可临时切换')
          .fontSize(11)
          .fontColor(this.cFaint())
          .margin({ top: 2 })
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
      this.inputChip('系统键盘', 0)
      this.inputChip('自带键盘', 1)
    }
    .width('100%')
    .padding({ top: 8, bottom: 8 })
  }

  // 输入方式选项按钮（选中高亮胶囊，样式同外观选项）
  @Builder
  private inputChip(label: string, mode: number) {
    Text(label)
      .fontSize(13)
      .fontColor(this.inputMethodOpt === mode ? '#FFFFFF' : this.cText())
      .backgroundColor(this.inputMethodOpt === mode ? this.cAccent() : (this.dark ? '#33363B' : '#E9EBF2'))
      .padding({ left: 12, right: 12, top: 6, bottom: 6 })
      .borderRadius(15)
      .stateStyles({
        pressed: {
          .backgroundColor(this.inputMethodOpt === mode ? this.cAccent() : (this.dark ? '#3E424A' : '#DDE1EA'))
        }
      })
      .margin({ left: 8 })
      .onClick(() => {
        this.applyInputMethod(mode);
      })
  }

  build() {
    Stack({ alignContent: Alignment.Start }) {
      // ---------- 主内容 ----------
      Column() {
        // 画布（C++ kernel + Canvas 渲染 + 手势 + 代数输入条）。反馈②：页面
        // 顶栏删除（无实际内容），☰ ↶ ↷ 由 CanvasStage 画布右上浮层渲染，
        // 菜单请求经 onMenuRequest 回调至此打开抽屉
        Column() {
          CanvasView({
            onMenuRequest: (): void => {
              this.fireEvt('evt.dismissKb');   // 打开菜单收回键盘（上游 行为）
              this.menuOpen = true;
            }
          })
            .width('100%')
            .height('100%')
        }
        .layoutWeight(1)
        .width('100%')

        // CAS 冒烟面板已删除（第五包）：上游 没有底部 CAS 栏，功能迁至
        // ☰ 菜单"视图"→ CAS 视图（evt.openCas → CanvasView 底部抽屉 CasPanel）
        //
        // 第十七包②：页面级键盘垫高（原宽屏 224）删除——垫高改 CanvasView
        // 运行时实测（onInputArea），宽/窄屏统一在组件内部垫，Index 级垫高
        // 与输入行不同组件，反解不收敛
      }
      .width('100%')
      .height('100%')
      .backgroundColor(this.cPage())

      // ---------- 汉堡菜单抽屉 ----------
      if (this.menuOpen) {
        Column()
          .width('100%')
          .height('100%')
          .backgroundColor(this.cScrim())
          .onClick(() => {
            this.menuOpen = false;
          })
          .transition(TransitionEffect.OPACITY.animation({ duration: 180 }))

        Column() {
          Text('梦烬几何')
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .fontColor(this.cTitle())
            .width('100%')
            .padding({ left: 20, top: 24, bottom: 4 })
          Text('原生几何画板')
            .fontSize(11)
            .fontColor(this.cFaint())
            .width('100%')
            .padding({ left: 20, bottom: 12 })

          Divider().color(this.cDivider())

          this.MenuRow('新建', '清空全部对象并重置视图', () => {
            this.confirmNew();
          })
          this.MenuRow('打开', '读取 .ggb 几何文件', () => {
            this.menuOpen = false;
            this.fireEvt('evt.openGgb');
          })
          this.MenuRow('保存', '当前图保存为 .ggb 文件', () => {
            this.menuOpen = false;
            this.fireEvt('evt.saveGgb');
          })

          Divider().color(this.cDivider())

          // 视图分组（上游经典版 的 CAS 入口就在 ☰ 菜单"视图"里，第五包
          // 照此迁入；重置视图同属视图操作归组）
          Text('视图')
            .fontSize(12)
            .fontColor(this.cFaint())
            .width('100%')
            .padding({ left: 20, top: 10, bottom: 2 })
          this.MenuRow('CAS 视图', '符号计算：解方程 / 化简 / 微积分', () => {
            this.menuOpen = false;
            this.fireEvt('evt.openCas');
          })
          this.MenuRow('重置视图', '回到标准缩放与原点居中', () => {
            this.menuOpen = false;
            this.fireEvt('evt.resetView');
          })

          Divider().color(this.cDivider())

          this.MenuRow('设置', '键盘 / 网格 / 坐标轴 / 外观', () => {
            this.menuOpen = false;
            this.settingsOpen = true;
          })
          this.MenuRow('关于', '版本与许可声明', () => {
            this.menuOpen = false;
            router.pushUrl({ url: 'pages/About' });
          })

          Blank()

          // 三十八轮：页脚声明改事实表述（早期 Web 方案遗留文案已撤，现应用
          // 零第三方代码；开源地址见「关于」页）
          Text('原创独立开发 · 源码按 GPL-3.0 协议开源\n基于开源 CAS 库 giac（GPL-3.0+）')
            .fontSize(10)
            .fontColor(this.dark ? '#5F646B' : '#AAAAAA')
            .width('100%')
            .padding({ left: 20, bottom: 20 })
        }
        .width(this.isWide ? 360 : 300)
        .height('100%')
        .backgroundColor(this.cSurface())
        .alignItems(HorizontalAlign.Start)
        .transition(TransitionEffect.asymmetric(
          TransitionEffect.translate({ x: -380 }).animation({ duration: 200 }),
          TransitionEffect.translate({ x: -380 }).animation({ duration: 200 })
        ))
      }

      // ---------- 设置底部面板 ----------
      if (this.settingsOpen) {
        Column()
          .width('100%')
          .height('100%')
          .backgroundColor(this.cScrim())
          .onClick(() => {
            this.settingsOpen = false;
          })
          .transition(TransitionEffect.OPACITY.animation({ duration: 180 }))

        Column() {
          // 面板上方空白：点击即退出设置（真机十四轮；面板本体与"完成"按钮保留，
          // 空白层不遮挡面板区域交互——点行/开关命中子组件）
          Blank()
            .onClick(() => {
              this.settingsOpen = false;
            })

          Column() {
            Row() {
              Text('设置')
                .fontSize(16)
                .fontWeight(FontWeight.Medium)
                .fontColor(this.cText())
                .layoutWeight(1)
              Text('完成')
                .fontSize(15)
                .fontColor(this.cAccent())
                .padding({ left: 12, right: 2 })
                .onClick(() => {
                  this.settingsOpen = false;
                })
            }
            .width('100%')

            Divider().color(this.cDivider()).margin({ top: 10, bottom: 2 })

            Text('外观')
              .fontSize(13)
              .fontColor(this.cSub())
              .width('100%')
              .margin({ top: 6, bottom: 2 })

            this.themeRow()

            Divider().color(this.cDivider()).margin({ top: 4, bottom: 2 })

            Text('视图')
              .fontSize(13)
              .fontColor(this.cSub())
              .width('100%')
              .margin({ top: 6, bottom: 2 })

            Row() {
              Column() {
                Text('显示网格')
                  .fontSize(15)
                  .fontColor(this.cText())
                Text('从刻度延伸的灰色方格网')
                  .fontSize(11)
                  .fontColor(this.cFaint())
                  .margin({ top: 2 })
              }
              .alignItems(HorizontalAlign.Start)
              .layoutWeight(1)
              Toggle({ type: ToggleType.Switch, isOn: this.setGrid })
                .selectedColor('#007DFF')
                .onChange((on: boolean) => {
                  this.setGrid = on;
                })
            }
            .width('100%')
            .height(56)

            Row() {
              Column() {
                Text('显示坐标轴')
                  .fontSize(15)
                  .fontColor(this.cText())
                Text('坐标轴、刻度与轴上数字')
                  .fontSize(11)
                  .fontColor(this.cFaint())
                  .margin({ top: 2 })
              }
              .alignItems(HorizontalAlign.Start)
              .layoutWeight(1)
              Toggle({ type: ToggleType.Switch, isOn: this.setAxes })
                .selectedColor('#007DFF')
                .onChange((on: boolean) => {
                  this.setAxes = on;
                })
            }
            .width('100%')
            .height(56)

            Divider().color(this.cDivider()).margin({ top: 4, bottom: 2 })

            Text('输入')
              .fontSize(13)
              .fontColor(this.cSub())
              .width('100%')
              .margin({ top: 6, bottom: 2 })

            this.inputMethodRow()
          }
          .width(this.isWide ? '60%' : '100%')
          .padding({ left: 16, right: 16, top: 14, bottom: 24 })
          .borderRadius({ topLeft: 16, topRight: 16 })
          .backgroundColor(this.cSurface())
        }
        .width('100%')
        .height('100%')
        .alignItems(HorizontalAlign.Center)
        .transition(TransitionEffect.asymmetric(
          TransitionEffect.translate({ y: 340 }).animation({ duration: 200 }),
          TransitionEffect.translate({ y: 340 }).animation({ duration: 200 })
        ))
      }
    }
    .width('100%')
    .height('100%')
    .onAreaChange((_o: Area, n: Area) => {
      this.isWide = (n.width as number) >= 640;
    })
  }
}
