import { router } from '@kit.ArkUI';
import { bundleManager, common } from '@kit.AbilityKit';

/**
 * 关于页：展示应用信息与声明
 * N3：深色模式（跟随系统，AppStorage 'ui.dark'）
 * 三十包②：版本号改读 bundle 信息（AppScope versionName）——此前硬编码
 * 1.0.0 已与实际构建（1.0.1→1.0.2）脱节两轮
 * 五十五包：整页重构——顶部加圆角应用图标；声明分三段（原创与商标/引擎
 * 出处/开源许可），开发者改实名与 AGC 一致（过华为开放测试审核）；新增
 * 开发者栏；开源仓库与隐私政策链接可点击跳转浏览器（UIAbilityContext
 * .openLink）。
 */
const REPO_URL = 'https://gitee.com/CH_mengxiang/DreamEmber-GeoCalc';
const PRIVACY_URL =
  'https://gitee.com/CH_mengxiang/DreamEmber-GeoCalc/blob/master/docs/privacy-policy.md';

@Entry
@Component
struct About {
  @StorageProp('ui.dark') private dark: boolean = false;
  @State private verText: string = '';

  aboutToAppear(): void {
    try {
      const info = bundleManager.getBundleInfoForSelfSync(
        bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT);
      if (info.versionName && info.versionName.length > 0) {
        this.verText = '版本 ' + info.versionName;
      }
    } catch (_e) {
    }
  }

  private cPage(): string {
    return this.dark ? '#17181A' : '#FFFFFF';
  }

  private cCard(): string {
    return this.dark ? '#232529' : '#F5F6FA';
  }

  private cText(): string {
    return this.dark ? '#E8EAED' : '#333333';
  }

  private cSub(): string {
    return this.dark ? '#9AA0A6' : '#555555';
  }

  private cFaint(): string {
    return this.dark ? '#6E747D' : '#888888';
  }

  private cLink(): string {
    return this.dark ? '#7FB2F0' : '#3A7BD5';
  }

  // 跳转系统浏览器（openLink：appLinkingOnly=false 允许网页直开；
  // 失败静默——关于页链接非关键路径）
  private openUrl(url: string): void {
    try {
      const ctx = this.getUIContext().getHostContext() as common.UIAbilityContext;
      ctx.openLink(url, { appLinkingOnly: false });
    } catch (_e) {
    }
  }

  build() {
    Column() {
      // 顶栏
      Row() {
        Text('←')
          .fontSize(22)
          .fontColor(this.cText())
          .onClick(() => {
            router.back();
          })
          .padding({ right: 12 })
        Text('关于')
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .fontColor(this.cText())
      }
      .width('100%')
      .height(52)
      .padding({ left: 16 })
      .alignItems(VerticalAlign.Center)

      Scroll() {
        Column() {
          // 应用标识：圆角图标 + 名称 + 版本
          Column() {
            Image($r('app.media.app_icon'))
              .width(84)
              .height(84)
              .borderRadius(20)
              .clip(true)
              .alt($r('app.media.startIcon'))
            Text('梦烬几何')
              .fontSize(26)
              .fontWeight(FontWeight.Bold)
              .fontColor(this.cText())
              .margin({ top: 14 })
            Text('DreamEmber GeoCalc')
              .fontSize(13)
              .fontColor(this.cFaint())
              .margin({ top: 4 })
            if (this.verText.length > 0) {
              Text(this.verText)
                .fontSize(13)
                .fontColor(this.cFaint())
                .margin({ top: 4 })
            }
          }
          .width('100%')
          .alignItems(HorizontalAlign.Center)
          .margin({ top: 32, bottom: 28 })

          // 声明：三段（原创与商标 / 引擎出处 / 开源许可）
          Column() {
            Text('声明')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('本应用为原创独立开发：界面与几何内核均为自主编写，'
              + '未使用 GeoGebra 源代码（开发过程中参考了其公开源码与交互设计）。'
              + 'GeoGebra 是 GeoGebra GmbH 的注册商标，'
              + '本应用与 GeoGebra 官方无隶属关系。')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
            Text('计算机代数引擎采用开源库 giac'
              + '（GPL-3.0+，© Bernard Parisse / Institut Fourier）。')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
              .margin({ top: 10 })
            Text('本项目源码以 GPL-3.0-or-later 协议开源（源码获取见下方开源仓库），'
              + '应用面向非商业的教育学习场景免费提供。')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
              .margin({ top: 10 })
          }
          .width('100%')
          .alignItems(HorizontalAlign.Start)
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())
          .margin({ bottom: 16 })

          Column() {
            Text('功能说明')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('梦烬几何提供交互式数学绘图：函数图像、几何对象、方程求解与 CAS 符号计算。'
              + '支持触屏拖拽、缩放与命令输入。')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
          }
          .width('100%')
          .alignItems(HorizontalAlign.Start)
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())
          .margin({ bottom: 16 })

          Column() {
            Text('开发者')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('杨丰玮')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
          }
          .width('100%')
          .alignItems(HorizontalAlign.Start)
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())
          .margin({ bottom: 16 })

          // 开源仓库（GPL-3.0 合规——源码可得入口，可点击跳浏览器）
          Column() {
            Text('开源仓库')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('Gitee：gitee.com/CH_mengxiang/DreamEmber-GeoCalc')
              .fontSize(14)
              .fontColor(this.cLink())
              .lineHeight(22)
              .onClick(() => this.openUrl(REPO_URL))
            Text('获取源码、提交问题反馈均在上述仓库进行。')
              .fontSize(13)
              .fontColor(this.cFaint())
              .lineHeight(20)
              .margin({ top: 6 })
          }
          .width('100%')
          .alignItems(HorizontalAlign.Start)
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())
          .margin({ bottom: 16 })

          Column() {
            Text('隐私政策')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('查看《梦烬几何隐私政策》')
              .fontSize(14)
              .fontColor(this.cLink())
              .lineHeight(22)
              .onClick(() => this.openUrl(PRIVACY_URL))
            Text('本应用不收集、不上传任何个人数据。')
              .fontSize(13)
              .fontColor(this.cFaint())
              .lineHeight(20)
              .margin({ top: 6 })
          }
          .width('100%')
          .alignItems(HorizontalAlign.Start)
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())

          Blank()
        }
        .constraintSize({ minHeight: '100%' })
        .padding({ left: 16, right: 16, bottom: 24 })
      }
      .layoutWeight(1)
      .align(Alignment.Top)
      .scrollBar(BarState.Off)
      .edgeEffect(EdgeEffect.Spring)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(this.cPage())
  }
}
