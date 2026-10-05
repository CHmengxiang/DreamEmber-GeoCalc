import { AbilityConstant, Configuration, ConfigurationConstant, UIAbility, Want } from '@kit.AbilityKit';
import { preferences } from '@kit.ArkData';
import { webview } from '@kit.ArkWeb';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window, display } from '@kit.ArkUI';
import { deviceInfo } from '@kit.BasicServicesKit';

const DOMAIN = 0x0000;

export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    // N3 深色模式：应用保持跟随系统（NOT_SET），再按持久化的外观设置覆盖
    try {
      this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
    } catch (err) {
      hilog.error(DOMAIN, 'testTag', 'Failed to set colorMode. Cause: %{public}s', JSON.stringify(err));
    }
    this.restoreThemeMode();
    this.restoreInputMethod();
    // 同步系统/应用色深给 ArkUI 组件树（'ui.dark'）
    this.syncColorMode();
    // 记录设备形态信息：deviceTypes=device 下 app 无法从系统直接拿到 phone/tablet 标签，
    // 用 screen density/width 与 marketName 辅助判定 UI 布局，若异常可在 hilog 中核对。
    try {
      hilog.info(DOMAIN, 'testTag',
        'device: marketName=%{public}s productModel=%{public}s sdkApi=%{public}d osFullName=%{public}s',
        deviceInfo.marketName, deviceInfo.productModel, deviceInfo.sdkApiVersion, deviceInfo.osFullName);
    } catch (err) {
      hilog.warn(DOMAIN, 'testTag', 'deviceInfo unavailable: %{public}s', JSON.stringify(err));
    }
    // 预热 Web 内核：在 Web 组件首次创建前完成内核初始化，
    // 消除内核冷启动带来的首帧白屏延迟（官方建议在 Ability onCreate 调用）。
    try {
      webview.WebviewController.initializeWebEngine();
    } catch (err) {
      hilog.error(DOMAIN, 'testTag', 'initializeWebEngine failed. Cause: %{public}s', JSON.stringify(err));
    }
    hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onCreate');
  }

  // 恢复持久化的外观设置（0=跟随系统 1=浅色 2=深色，Index 设置页写入）
  private restoreThemeMode(): void {
    try {
      const pref = preferences.getPreferencesSync(
        this.context.getApplicationContext(), { name: 'dreamember_settings' });
      const mode = pref.getSync('themeMode', 0) as number;
      AppStorage.setOrCreate('ui.themeMode', mode);
      if (mode === 2) {
        this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_DARK);
      } else if (mode === 1) {
        this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT);
      }
    } catch (err) {
      hilog.warn(DOMAIN, 'testTag', 'restoreThemeMode failed. %{public}s', JSON.stringify(err));
    }
  }

  // 恢复持久化的默认输入方式（0=系统键盘 1=自带键盘，Index 设置页写入）。
  // 第十六包③：第十五包把默认翻成 1 但真机未生效——v1 及之前的版本默认
  // 0=系统键盘，且旧包已把当时的默认值写进了持久化（restoreInputMethod
  // 的「已持久化不受影响」把旧默认当成了用户选择，设置页也显示系统键盘
  // 选中）。用 optDefaultsVer 标记区分：无标记 → 持久化值是旧默认残留，
  // 一次性翻到新默认 1=自带键盘并写回；此后用户在设置页的显式选择照常
  // 持久化、永久生效
  private restoreInputMethod(): void {
    try {
      const pref = preferences.getPreferencesSync(
        this.context.getApplicationContext(), { name: 'dreamember_settings' });
      const ver = pref.getSync('optDefaultsVer', 0) as number;
      let mode = pref.getSync('inputMethod', 1) as number;
      if (ver < 2) {
        mode = 1;
        pref.putSync('optDefaultsVer', 2);
        pref.putSync('inputMethod', mode);
        pref.flush();
      }
      AppStorage.setOrCreate('opt.inputMethod', mode);
    } catch (err) {
      hilog.warn(DOMAIN, 'testTag', 'restoreInputMethod failed. %{public}s', JSON.stringify(err));
    }
  }

  // 系统深浅色切换（含首次启动读取），广播给所有组件（AppStorage 'ui.dark'）
  private syncColorMode(colorMode?: ConfigurationConstant.ColorMode): void {
    let dark = false;
    if (colorMode !== undefined) {
      dark = colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK;
    } else {
      try {
        const cfg: Configuration = this.context.config;
        dark = cfg.colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK;
      } catch (err) {
        hilog.warn(DOMAIN, 'testTag', 'read config failed, default light. %{public}s', JSON.stringify(err));
      }
    }
    AppStorage.setOrCreate('ui.dark', dark);
  }

  onConfigurationUpdate(newConfig: Configuration): void {
    this.syncColorMode(newConfig.colorMode);
  }

  onDestroy(): void {
    hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // Main window is created, set main page for this ability
    hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onWindowStageCreate');

    // 第十九包①：导航条（手势条/三键栏）高度 → AppStorage 'ui.navBottomVp'。
    // 页面默认不铺到窗口底（导航条避让区悬空），而 TextInput.customKeyboard
    // 浮层锚在窗口底——键盘避让目标必须按「键盘上沿相对页面底 =
    // 键盘高 − 导航条高」反解（CanvasView.applyKbSpacer 消费）。异常时按 0
    // 兜底（PC/悬浮窗无导航条，行为同旧目标）
    try {
      const mainWin = windowStage.getMainWindowSync();
      const nav = mainWin.getWindowAvoidArea(window.AvoidAreaType.TYPE_NAVIGATION_INDICATOR);
      const den = display.getDefaultDisplaySync().densityPixels;
      AppStorage.setOrCreate('ui.navBottomVp',
        Math.round((nav.bottomRect.height / den) * 10) / 10);
    } catch (err) {
      AppStorage.setOrCreate('ui.navBottomVp', 0);
      hilog.warn(DOMAIN, 'testTag', 'navIndicator query failed. %{public}s', JSON.stringify(err));
    }

    windowStage.loadContent('pages/Index', (err) => {
      if (err.code) {
        hilog.error(DOMAIN, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err));
        return;
      }
      hilog.info(DOMAIN, 'testTag', 'Succeeded in loading the content.');
    });
  }

  onWindowStageDestroy(): void {
    // Main window is destroyed, release UI related resources
    hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground(): void {
    // Ability has brought to foreground
    hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onForeground');
  }

  onBackground(): void {
    // Ability has back to background
    hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onBackground');
  }
}